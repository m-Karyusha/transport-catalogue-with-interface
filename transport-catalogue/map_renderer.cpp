#include "map_renderer.h"

#include <algorithm>
#include <map>
#include <optional>
#include <string>

namespace renderer {

    namespace detail {

        // Погрешность для сравнения вещественных чисел с нулем
        const double EPSILON = 1e-6;

        /**
         * Проверяет, что значение близко к нулю с учетом погрешности.
         * Используется для предотвращения деления на ноль при вычислении масштаба.
         */
        static bool IsZero(double value) {
            return std::abs(value) < EPSILON;
        }

        /**
         * Класс выполняет проецирование географических координат в координаты SVG-документа.
         */
        class SphereProjector {
        public:
            /**
             * Конструктор.
             */
            template <typename PointInputIt>
            SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                            double max_width, double max_height, double padding)
                : padding_(padding) {
                if (points_begin == points_end) {
                    return;
                }

                // Минимальная и максимальная долгота (границы по X)
                const auto [left_it, right_it] = std::minmax_element(
                    points_begin, points_end,
                    [](auto lhs, auto rhs) {
                    return lhs.lng < rhs.lng;
                });

                min_lon_ = left_it->lng;
                const double max_lon = right_it->lng;

                // Минимальная и максимальная широта (границы по Y)
                const auto [bottom_it, top_it] = std::minmax_element(
                    points_begin, points_end,
                    [](auto lhs, auto rhs) {
                    return lhs.lat < rhs.lat;
                });

                const double min_lat = bottom_it->lat;
                max_lat_ = top_it->lat;

                // Масштаб по X
                std::optional<double> width_zoom;
                if (!IsZero(max_lon - min_lon_)) {
                    width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
                }

                // Масштаб по Y
                std::optional<double> height_zoom;
                if (!IsZero(max_lat_ - min_lat)) {
                    height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
                }

                // Выбор минимального масштаба
                if (width_zoom && height_zoom) {
                    zoom_coeff_ = std::min(*width_zoom, *height_zoom);
                } else if (width_zoom) {
                    zoom_coeff_ = *width_zoom;
                } else if (height_zoom) {
                    zoom_coeff_ = *height_zoom;
                }
            }

            /**
             * Преобразует географические координаты в точку на SVG.
             */
            svg::Point operator()(geo::Coordinates coords) const {
                return {
                    (coords.lng - min_lon_) * zoom_coeff_ + padding_,   // Смещение по X
                    (max_lat_ - coords.lat) * zoom_coeff_ + padding_    // Смещение по Y
                };
            }

        private:
            double padding_ = 0.0;      // Отступ от краёв SVG
            double min_lon_ = 0.0;      // Минимальная долгота
            double max_lat_ = 0.0;      // Максимальная широта
            double zoom_coeff_ = 0.0;   // Коэффициент масштабирования
        };

        /**
         * Проверяет, есть ли у маршрута остановки
         */
        static bool HasStops(const domain::Bus* bus) {
            return bus != nullptr && !bus->stops.empty();
        }

        /**
         * Возвращает координаты остановок, входящих хотя бы в один отображаемый маршрут.
         */
        static std::vector<geo::Coordinates> CollectRoutePoints(const std::vector<const domain::Bus*>& buses) {
            std::vector<geo::Coordinates> result;

            for (const domain::Bus* bus : buses) {
                if (!HasStops(bus)) {
                    continue;
                }

                for (const domain::Stop* stop : bus->stops) {
                    if (stop != nullptr) {
                        result.push_back(stop->coords);
                    }
                }
            }

            return result;
        }

        /**
         * Вычисляет экранные координаты всех остановок,
         * которые участвуют хотя бы в одном маршруте.
         * Остановки возвращаются в лексикографическом порядке.
         */
        static std::map<std::string, svg::Point> PrepareStopPoints(
            const std::vector<const domain::Bus*>& buses,
            const SphereProjector& projector) {

            std::map<std::string, const domain::Stop*> used_stops;
            for (const domain::Bus* bus : buses) {
                if (!HasStops(bus)) {
                    continue;
                }

                for (const domain::Stop* stop : bus->stops) {
                    if (stop != nullptr) {
                        used_stops.emplace(stop->name, stop);
                    }
                }
            }

            std::map<std::string, svg::Point> result;
            for (const auto& [stop_name, stop] : used_stops) {
                result.emplace(stop_name, projector(stop->coords));
            }

            return result;
        }

        /**
         * Назначает маршрутам цвета из палитры.
         */
        static std::map<std::string, svg::Color> ComputeBusColors(const std::vector<const domain::Bus*>& buses,
                                                                  const std::vector<svg::Color>& palette) {
            std::map<std::string, svg::Color> result;
            size_t color_index = 0;

            for (const domain::Bus* bus : buses) {
                if (!HasStops(bus)) {
                    continue;
                }

                result[bus->name] = palette[color_index % palette.size()];
                ++color_index;
            }

            return result;
        }

        /**
         * Создаёт ломаную линию маршрута.
         * Для некольцевого маршрута добавляет обратный путь.
         */
        static svg::Polyline MakeRoutePolyline(const domain::Bus& bus,
                                const SphereProjector& projector,
                                const svg::Color& color,
                                double line_width) {
            svg::Polyline polyline;

            for (const domain::Stop* stop : bus.stops) {
                polyline.AddPoint(projector(stop->coords));
            }

            if (!bus.is_roundtrip) {
                for (size_t i = bus.stops.size(); i > 1; --i) {
                    polyline.AddPoint(projector(bus.stops[i - 2]->coords));
                }
            }

            polyline
                .SetFillColor(svg::NoneColor)
                .SetStrokeColor(color)
                .SetStrokeWidth(line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            return polyline;
        }

        /**
         * Создает подложку и основную надпись для подписи маршрута.
         */
        static std::pair<svg::Text, svg::Text> MakeBusLabel(svg::Point pos,
                                             const std::string& bus_name,
                                             const svg::Color& color,
                                             const RenderSettings& settings) {
            svg::Text underlayer;
            underlayer.SetPosition(pos)
                .SetOffset(settings.bus_label_offset)
                .SetFontSize(settings.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus_name)
                .SetFillColor(settings.underlayer_color)
                .SetStrokeColor(settings.underlayer_color)
                .SetStrokeWidth(settings.underlayer_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            svg::Text label;
            label.SetPosition(pos)
                .SetOffset(settings.bus_label_offset)
                .SetFontSize(settings.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus_name)
                .SetFillColor(color);

            return { underlayer, label };
        }

        /**
         * Создает подложку и основную надпись для подписи остановки.
         */
        static std::pair<svg::Text, svg::Text> MakeStopLabel(svg::Point pos,
                                              const std::string& stop_name,
                                              const RenderSettings& settings) {
            svg::Text underlayer;
            underlayer.SetPosition(pos)
                .SetOffset(settings.stop_label_offset)
                .SetFontSize(settings.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(stop_name)
                .SetFillColor(settings.underlayer_color)
                .SetStrokeColor(settings.underlayer_color)
                .SetStrokeWidth(settings.underlayer_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            svg::Text label;
            label.SetPosition(pos)
                .SetOffset(settings.stop_label_offset)
                .SetFontSize(settings.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(stop_name)
                .SetFillColor(std::string("black"));

            return { underlayer, label };
        }

        /**
         * Возвращает кружок остановки на карте.
         */
        static svg::Circle MakeStopCircle(svg::Point center, double radius) {
            svg::Circle circle;
            circle.SetCenter(center)
                .SetRadius(radius)
                .SetFillColor(std::string("white"));
            return circle;
        }

        using StopPoints = std::map<std::string, svg::Point>;
        using BusColors = std::map<std::string, svg::Color>;

        /**
         * Создает проектор для преобразования географических координат
         * остановок в координаты SVG-документа.
         */
        static SphereProjector MakeProjector(const std::vector<const domain::Bus*>& buses,
                                             const RenderSettings& settings) {
            const std::vector<geo::Coordinates> route_points = CollectRoutePoints(buses);

            return SphereProjector(
                route_points.begin(),
                route_points.end(),
                settings.width,
                settings.height,
                settings.padding
            );
        }

        /**
         * Рисует слой ломаных линий автобусных маршрутов.
         */
        static void RenderBusLines(svg::Document& doc,
                                   const std::vector<const domain::Bus*>& buses,
                                   const SphereProjector& projector,
                                   const BusColors& bus_colors,
                                   const RenderSettings& settings) {
            for (const domain::Bus* bus : buses) {
                if (!HasStops(bus)) {
                    continue;
                }

                doc.Add(MakeRoutePolyline(
                    *bus,
                    projector,
                    bus_colors.at(bus->name),
                    settings.line_width
                ));
            }
        }

        /**
         * Рисует слой названий маршрутов у конечных остановок.
         */
        static void RenderBusLabels(svg::Document& doc,
                                    const std::vector<const domain::Bus*>& buses,
                                    const StopPoints& stop_points,
                                    const BusColors& bus_colors,
                                    const RenderSettings& settings) {
            for (const domain::Bus* bus : buses) {
                if (!HasStops(bus)) {
                    continue;
                }

                const auto& color = bus_colors.at(bus->name);
                const domain::Stop* first_stop = bus->stops.front();
                const svg::Point first_point = stop_points.at(first_stop->name);

                {
                    auto [underlayer, label] = MakeBusLabel(first_point, bus->name, color, settings);
                    doc.Add(underlayer);
                    doc.Add(label);
                }

                if (!bus->is_roundtrip) {
                    const domain::Stop* last_stop = bus->stops.back();
                    if (last_stop != first_stop) {
                        const svg::Point last_point = stop_points.at(last_stop->name);
                        auto [underlayer, label] = MakeBusLabel(last_point, bus->name, color, settings);
                        doc.Add(underlayer);
                        doc.Add(label);
                    }
                }
            }
        }

        /**
         * Рисует слой кружков, обозначающих остановки.
         */
        static void RenderStopPoints(svg::Document& doc,
                                     const StopPoints& stop_points,
                                     const RenderSettings& settings) {
            for (const auto& [stop_name, point] : stop_points) {
                doc.Add(MakeStopCircle(point, settings.stop_radius));
            }
        }

        /**
         * Рисует слой названий остановок.
         */
        static void RenderStopLabels(svg::Document& doc,
                                     const StopPoints& stop_points,
                                     const RenderSettings& settings) {
            for (const auto& [stop_name, point] : stop_points) {
                auto [underlayer, label] = MakeStopLabel(point, stop_name, settings);
                doc.Add(underlayer);
                doc.Add(label);
            }
        }

    } // namespace detail

    MapRenderer::MapRenderer(RenderSettings settings)
        : settings_(std::move(settings)) {}

    svg::Document MapRenderer::Render(const std::vector<const domain::Bus*>& buses) const {
        svg::Document doc;

        const detail::SphereProjector projector = detail::MakeProjector(buses, settings_);
        const detail::StopPoints stop_points = detail::PrepareStopPoints(buses, projector);
        const detail::BusColors bus_colors = detail::ComputeBusColors(buses, settings_.color_palette);

        detail::RenderBusLines(doc, buses, projector, bus_colors, settings_);
        detail::RenderBusLabels(doc, buses, stop_points, bus_colors, settings_);
        detail::RenderStopPoints(doc, stop_points, settings_);
        detail::RenderStopLabels(doc, stop_points, settings_);

        return doc;
    }

} // namespace renderer
