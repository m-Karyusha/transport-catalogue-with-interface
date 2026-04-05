#include "json_reader.h"

#include <string>
#include <string_view>
#include <vector>

#include "request_handler.h"

namespace tc {

    const json::Node& JsonReader::GetNode(const json::Dict& dict, std::string_view key) {
        return dict.at(std::string(key));
    }

    const std::string& JsonReader::GetString(const json::Dict& dict, std::string_view key) {
        return GetNode(dict, key).AsString();
    }

    int JsonReader::GetInt(const json::Dict& dict, std::string_view key) {
        return GetNode(dict, key).AsInt();
    }

    double JsonReader::GetDouble(const json::Dict& dict, std::string_view key) {
        return GetNode(dict, key).AsDouble();
    }

    bool JsonReader::GetBool(const json::Dict& dict, std::string_view key) {
        return GetNode(dict, key).AsBool();
    }

    const json::Array& JsonReader::GetArray(const json::Dict& dict, std::string_view key) {
        return GetNode(dict, key).AsArray();
    }

    const json::Dict& JsonReader::GetDict(const json::Dict& dict, std::string_view key) {
        return GetNode(dict, key).AsMap();
    }

    // ===================================
    // Загрузка данных в каталог
    // ===================================

    void JsonReader::LoadStops(const json::Array& base_requests,
                               std::vector<DistanceDescription>& distances) {
        for (const auto& request_node : base_requests) {
            const auto& request = request_node.AsMap();

            // Только Stop-запросы
            if (GetString(request, "type") != "Stop") {
                continue;
            }

            const std::string& stop_name = GetString(request, "name");
            const double latitude = GetDouble(request, "latitude");
            const double longitude = GetDouble(request, "longitude");

            // Добавить остановку в каталог
            catalogue_.AddStop(stop_name, { latitude, longitude });

            // Сохранить расстояния
            const auto& road_distances = GetDict(request, "road_distances");
            for (const auto& [other_stop, distance_node] : road_distances) {
                distances.push_back({
                    stop_name,
                    other_stop,
                    distance_node.AsInt()
                });
            }
        }
    }

    void JsonReader::LoadDistances(const std::vector<DistanceDescription>& distances) {
        for (const auto& dist : distances) {
            const tc::Stop* from = catalogue_.FindStop(dist.from);
            const tc::Stop* to = catalogue_.FindStop(dist.to);

            // Установить расстояние, только если обе остановки существуют
            if (from != nullptr && to != nullptr) {
                catalogue_.SetDistance(from, to, dist.distance);
            }
        }
    }

    void JsonReader::LoadBuses(const json::Array& base_requests) {
        for (const auto& request_node : base_requests) {
            const auto& request = request_node.AsMap();

            // Только Bus-запросы
            if (GetString(request, "type") != "Bus") {
                continue;
            }

            const std::string& bus_name = GetString(request, "name");
            const bool is_roundtrip = GetBool(request, "is_roundtrip");

            std::vector<std::string_view> stops;
            const auto& stops_array = GetArray(request, "stops");
            stops.reserve(stops_array.size());

            // Список остановок маршрута
            for (const auto& stop_node : stops_array) {
                stops.emplace_back(stop_node.AsString());
            }

            // Добавить маршрут в каталог
            catalogue_.AddBus(bus_name, std::move(stops), is_roundtrip);
        }
    }

    // ===================================
    // Формирование JSON-ответов
    // ===================================

    json::Dict JsonReader::MakeStopResponse(const std::string& stop_name,
                                            const RequestHandler& handler) {
        json::Dict result;

        // Получить информацию об остановке
        const RequestHandler::StopInfo info = handler.GetStopInfo(stop_name);
        if (!info.found) {
            result["error_message"] = std::string("not found");
            return result;
        }

        json::Array buses_array;
        buses_array.reserve(info.buses.size());

        // Сформировать список маршрутов
        for (const auto& bus : info.buses) {
            buses_array.emplace_back(bus);
        }

        result["buses"] = std::move(buses_array);
        return result;
    }

    json::Dict JsonReader::MakeBusResponse(const std::string& bus_name,
                                           const RequestHandler& handler) {
        json::Dict result;

        // Получить информацию о маршруте
        const RequestHandler::BusInfo info = handler.GetBusInfo(bus_name);
        if (!info.found) {
            result["error_message"] = std::string("not found");
            return result;
        }

        // Заполнить статистику маршрута
        result["curvature"] = info.curvature;
        result["route_length"] = info.route_length;
        result["stop_count"] = info.stop_count;
        result["unique_stop_count"] = info.unique_stop_count;

        return result;
    }

    // ===================================
    // Парсинг JSON
    // ===================================

    svg::Color JsonReader::ParseColor(const json::Node& node) {
        if (node.IsString()) {
            return node.AsString();
        }

        const auto& arr = node.AsArray();
        if (arr.size() == 3) {
            return svg::Rgb{
                static_cast<uint8_t>(arr[0].AsInt()),
                static_cast<uint8_t>(arr[1].AsInt()),
                static_cast<uint8_t>(arr[2].AsInt())
            };
        }

        return svg::Rgba{
            static_cast<uint8_t>(arr[0].AsInt()),
            static_cast<uint8_t>(arr[1].AsInt()),
            static_cast<uint8_t>(arr[2].AsInt()),
            arr[3].AsDouble()
        };
    }

    renderer::RenderSettings JsonReader::ParseRenderSettings(const json::Dict& dict) {
        renderer::RenderSettings settings;

        settings.width = GetDouble(dict, "width");
        settings.height = GetDouble(dict, "height");
        settings.padding = GetDouble(dict, "padding");
        settings.line_width = GetDouble(dict, "line_width");

        settings.color_palette.clear();
        for (const auto& color_node : GetArray(dict, "color_palette")) {
            settings.color_palette.push_back(ParseColor(color_node));
        }

        settings.stop_radius = GetDouble(dict, "stop_radius");

        settings.bus_label_font_size = GetInt(dict, "bus_label_font_size");
        {
            const auto& offset = GetArray(dict, "bus_label_offset");
            settings.bus_label_offset = {
                offset[0].AsDouble(),
                offset[1].AsDouble()
            };
        }

        settings.stop_label_font_size = GetInt(dict, "stop_label_font_size");
        {
            const auto& offset = GetArray(dict, "stop_label_offset");
            settings.stop_label_offset = {
                offset[0].AsDouble(),
                offset[1].AsDouble()
            };
        }

        settings.underlayer_color = ParseColor(GetNode(dict, "underlayer_color"));
        settings.underlayer_width = GetDouble(dict, "underlayer_width");

        return settings;
    }

    void JsonReader::LoadData(const json::Document& doc) {
        const auto& root = doc.GetRoot().AsMap();
        const auto& base_requests = GetArray(root, "base_requests");

        std::vector<DistanceDescription> distances;

        // 1 — остановки
        LoadStops(base_requests, distances);
        // 2 — расстояния
        LoadDistances(distances);
        // 3 — маршруты
        LoadBuses(base_requests);
    }

    json::Document JsonReader::ProcessRequests(const json::Document& doc,
                                               const tc::RequestHandler& handler) {
        const auto& root = doc.GetRoot().AsMap();
        const auto& stat_requests = GetArray(root, "stat_requests");

        json::Array responses;
        responses.reserve(stat_requests.size());

        for (const auto& request_node : stat_requests) {
            const auto& request = request_node.AsMap();

            const std::string& type = GetString(request, "type");

            // Ответ в зависимости от типа запроса
            if (type == "Stop") {
                const std::string& name = GetString(request, "name");
                responses.emplace_back(MakeStopResponse(name, handler));
            } else if (type == "Bus") {
                const std::string& name = GetString(request, "name");
                responses.emplace_back(MakeBusResponse(name, handler));
            }
        }

        return json::Document{
            std::move(responses)
        };
    }

    void JsonReader::PrintDoc(const json::Document& doc, std::ostream& output) {
        json::Print(doc, output);
    }

    renderer::RenderSettings JsonReader::LoadRenderSettings(const json::Document& doc) {
        const auto& root = doc.GetRoot().AsMap();
        return ParseRenderSettings(GetDict(root, "render_settings"));
    }

    bool JsonReader::HasMapRequests(const json::Document& doc) {
        const auto& root = doc.GetRoot().AsMap();

        // Если нет stat_requests, то нет Map
        if (root.count("stat_requests") == 0) {
            return false;
        }

        const auto& stat_requests = root.at("stat_requests").AsArray();

        for (const auto& request_node : stat_requests) {
            const auto& request = request_node.AsMap();

            if (request.at("type").AsString() == "Map") {
                return true;
            }
        }

        return false;
    }

} // namespace tc
