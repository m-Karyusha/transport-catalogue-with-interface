#include "transport_catalogue.h"

#include <deque>
#include <unordered_map>
#include <set>
#include <unordered_set>

namespace tc {
    struct TransportCatalogue::Impl {
        std::deque<Stop> stops_storage_;
        std::deque<Bus>  buses_storage_;

        std::unordered_map<std::string_view, const Stop*> stop_by_name_;
        std::unordered_map<std::string_view, const Bus*>  bus_by_name_;

        std::unordered_map<const Stop*, std::set<std::string_view>> buses_by_stop_;

        struct StopPairHasher {
            size_t operator()(const std::pair<const Stop*, const Stop*>& p) const noexcept {
                auto h1 = std::hash<const void*>{}(p.first);
                auto h2 = std::hash<const void*>{}(p.second);
                return h1 * 37 + h2;
            }
        };

        using PairStop = std::pair<const Stop*, const Stop*>;

        std::unordered_map<PairStop, double, StopPairHasher> distances_;

        // Для объединения остановок
        std::vector<domain::StopMergeRequest> pending_merges_;
        std::unordered_map<std::string_view, std::string> stop_redirect_;  // старая -> новая
    };

    TransportCatalogue::TransportCatalogue()
        : impl_(std::make_unique<Impl>()) {
    }

    TransportCatalogue::~TransportCatalogue() = default;

    void TransportCatalogue::AddStop(const std::string& name, Coordinates coords) {
        // Добавить остановку в хранилище
        impl_->stops_storage_.push_back({ name, coords });

        // Сохранить указатель в map для быстрого поиска
        const Stop* ptr = &impl_->stops_storage_.back();
        impl_->stop_by_name_[ptr->name] = ptr;
    }

    void TransportCatalogue::AddBus(const std::string& name,
                                    std::vector<std::string_view> stop_names,
                                    bool is_roundtrip) {
        std::vector<const Stop*> stops;
        stops.reserve(stop_names.size());

        // Преобразовать имена в указатели
        for (const auto& stop_name : stop_names) {
            auto it = impl_->stop_by_name_.find(stop_name);
            // Проверка на успешный поиск остановки
            if (it == impl_->stop_by_name_.end()) {
                continue;
            }
            stops.emplace_back(it->second);
        }

        // Добавить автобус в хранилище
        impl_->buses_storage_.push_back({name, std::move(stops), is_roundtrip});

        // Сохранить указатель в map
        const Bus* ptr = &impl_->buses_storage_.back();
        impl_->bus_by_name_[ptr->name] = ptr;

        // Добавить маршруты в список маршрутов через каждую из остановок
        for (const Stop* stop : ptr->stops) {
            impl_->buses_by_stop_[stop].emplace(ptr->name);
        }
    }

    const Stop* TransportCatalogue::FindStop(std::string_view name) const {
        auto it = impl_->stop_by_name_.find(name);
        return (it != impl_->stop_by_name_.end()) ? it->second : nullptr;
    }

    const Bus* TransportCatalogue::FindBus(std::string_view name) const {
        auto it = impl_->bus_by_name_.find(name);
        return (it != impl_->bus_by_name_.end()) ? it->second : nullptr;
    }

    std::optional<TransportCatalogue::BusStat> TransportCatalogue::GetBusStat(std::string_view bus_name) const {
        const Bus* bus = FindBus(bus_name);
        if (!bus) {
            return std::nullopt;
        }

        BusStat stat;

        if (bus->is_roundtrip) {
            stat.stops_on_route = bus->stops.size();
        } else {
            stat.stops_on_route = bus->stops.empty() ? 0 : bus->stops.size() * 2 - 1;
        }

        // Подсчет уникальных остановок
        std::unordered_set<const Stop*> unique {
            bus->stops.begin(), bus->stops.end()
        };

        stat.unique_stops = unique.size();

        // Подсчет длины маршрута
        double road_length = 0.0;
        double geo_length = 0.0;

        // Прямой проход
        for (size_t i = 0; i + 1 < bus->stops.size(); ++i) {
            AccumulateRoute(bus->stops[i], bus->stops[i + 1], road_length, geo_length);
        }

        // Обратный проход
        if (!bus->is_roundtrip) {
            for (size_t i = bus->stops.size(); i > 1; --i) {
                AccumulateRoute(bus->stops[i - 1], bus->stops[i - 2], road_length, geo_length);
            }
        }

        stat.route_length = road_length;

        if (geo_length > 0) {
            stat.curvature = road_length / geo_length;
        } else {
            stat.curvature = 0;
        }

        return stat;
    }

    std::vector<std::string> TransportCatalogue::GetBusesByStop(std::string_view stop_name) const {
        std::vector<std::string> result;

        const Stop* stop = FindStop(stop_name);
        if (!stop) {
            return result;
        }

        auto it = impl_->buses_by_stop_.find(stop);
        if (it == impl_->buses_by_stop_.end() || it->second.empty()) {
            return result;
        }

        result.reserve(it->second.size());
        for (std::string_view sv : it->second) {
            result.emplace_back(sv);
        }

        return result;
    }

    std::vector<const Bus*> TransportCatalogue::GetAllBuses() const {
        std::vector<const Bus*> result;
        result.reserve(impl_->buses_storage_.size());

        for (const auto& bus : impl_->buses_storage_) {
            result.push_back(&bus);
        }

        return result;
    }

    void TransportCatalogue::SetDistance(const Stop* from, const Stop* to, double meters) {
        if (!from || !to) {
            return;
        }

        if (impl_->stop_by_name_.count(from->name) == 0 ||
            impl_->stop_by_name_.count(to->name) == 0) {
            return;
        }

        impl_->distances_[{from, to}] = meters;
    }

    double TransportCatalogue::GetDistance(const Stop* from, const Stop* to) const {
        auto it = impl_->distances_.find({ from, to });

        if (it != impl_->distances_.end()) {
            return it->second;
        }

        /// TODO
        // Если виртуальная остановка - используем географическое расстояние
        if (from->is_virtual || to->is_virtual) {
            return ComputeDistance(from->coords, to->coords);
        }

        it = impl_->distances_.find({ to, from });

        if (it != impl_->distances_.end()) {
            return it->second;
        }

        return 0;
    }

    void TransportCatalogue::Clear() {
        impl_->stops_storage_.clear();
        impl_->buses_storage_.clear();

        impl_->stop_by_name_.clear();
        impl_->bus_by_name_.clear();

        impl_->buses_by_stop_.clear();
        impl_->distances_.clear();

        impl_->pending_merges_.clear();
        impl_->stop_redirect_.clear();
    }

    void TransportCatalogue::AccumulateRoute(const Stop* from, const Stop* to, double& road_length, double& geo_length) const {
        road_length += GetDistance(from, to);
        geo_length += ComputeDistance(from->coords, to->coords);
    }

    // Для объединенных остановок

    void TransportCatalogue::AddStopMergeRequest(const domain::StopMergeRequest& request) {
        impl_->pending_merges_.push_back(request);
    }

    void TransportCatalogue::ApplyStopMerges() {
        for (const auto& req : impl_->pending_merges_) {
            // Проверяем, что все остановки существуют
            std::vector<const Stop*> existing_stops;
            for (const auto& stop_name : req.stops) {
                const Stop* stop = FindStop(stop_name);
                if (!stop) {
                    // Можно выбросить исключение или залогировать
                    continue;
                }
                existing_stops.push_back(stop);
            }

            if (existing_stops.empty()) continue;

            // Вычисляем среднюю координату если нужно
            Coordinates avg_coords;
            if (req.combine_coords) {
                double sum_lat = 0, sum_lng = 0;
                for (const auto* stop : existing_stops) {
                    sum_lat += stop->coords.lat;
                    sum_lng += stop->coords.lng;
                }
                avg_coords.lat = sum_lat / existing_stops.size();
                avg_coords.lng = sum_lng / existing_stops.size();
            }

            // Создаем виртуальную остановку
            std::string virtual_name = req.merged_name;
            if (virtual_name.empty()) {
                // Автоматическое создание имени через " / "
                virtual_name = req.stops[0];
                for (size_t i = 1; i < req.stops.size(); ++i) {
                    virtual_name += " / " + req.stops[i];
                }
            }

            // Добавляем виртуальную остановку
            AddStop(virtual_name, avg_coords);
            const Stop* virtual_stop = FindStop(virtual_name);

            // Настраиваем перенаправление
            for (const auto* stop : existing_stops) {
                // Сохраняем информацию о том, что это виртуальная остановка
                const_cast<Stop*>(virtual_stop)->is_virtual = true;
                const_cast<Stop*>(virtual_stop)->merged_stops.push_back(stop->name);

                // Перенаправляем старую остановку на новую
                impl_->stop_redirect_[stop->name] = virtual_name;
            }
        }

        // Обновляем маршруты: заменяем старые остановки на виртуальные
        for (auto& bus : impl_->buses_storage_) {
            std::vector<const Stop*> new_stops;
            for (const Stop* stop : bus.stops) {
                auto it = impl_->stop_redirect_.find(stop->name);
                if (it != impl_->stop_redirect_.end()) {
                    // Заменяем на виртуальную остановку
                    const Stop* virtual_stop = FindStop(it->second);
                    if (virtual_stop) {
                        new_stops.push_back(virtual_stop);
                    } else {
                        new_stops.push_back(stop);
                    }
                } else {
                    new_stops.push_back(stop);
                }
            }
            // Обновляем указатели (нужен неконстантный доступ)
            const_cast<Bus*>(&bus)->stops = std::move(new_stops);
        }
    }

    const Stop* TransportCatalogue::GetCanonicalStop(std::string_view stop_name) const {
        auto it = impl_->stop_redirect_.find(stop_name);
        if (it != impl_->stop_redirect_.end()) {
            return FindStop(it->second);
        }
        return FindStop(stop_name);
    }

}  // namespace tc
