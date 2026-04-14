#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <optional>

#include "domain.h"
#include "libs/geo/geo.h"

namespace tc {

    using Coordinates = geo::Coordinates;
    using Stop = domain::Stop;
    using Bus = domain::Bus;

    class TransportCatalogue {
    public:
        /**
         * Конструктор
         */
        TransportCatalogue();

        /**
         * Деструктор
         */
        ~TransportCatalogue();

        /**
         * Удаленный конструктор копирования
         */
        TransportCatalogue(const TransportCatalogue&) = delete;
        TransportCatalogue& operator=(const TransportCatalogue&) = delete;

        /**
         * Добавляет остановку
         */
        void AddStop(const std::string& name, Coordinates coords);

        /**
         * Добавляет маршрут
         */
        void AddBus(const std::string& name,
                    std::vector<std::string_view> stop_names,
                    bool is_roundtrip);

        /**
         * Ищет остановку по названию
         */
        const Stop* FindStop(std::string_view name) const;

        /**
         * Добавляет маршрут по названию
         */
        const Bus* FindBus(std::string_view name) const;

        /**
         * Возвращает список маршрутов через остановку
         */
        std::vector<std::string> GetBusesByStop(std::string_view stop_name) const;

        /**
         * Возвращает список всех маршрутов
         */
        std::vector<const Bus*> GetAllBuses() const;

        // Вид статистики по маршруту
        struct BusStat {
            size_t stops_on_route = 0;
            size_t unique_stops = 0;
            double route_length = 0.0;
            double curvature = 0.0;
        };

        /**
         *  Возвращает статистику по маршруту
         */
        std::optional<BusStat> GetBusStat(std::string_view bus_name) const;

        /**
         * Устанавливает расстояние в метрах от одной остановки до другой.
         */
        void SetDistance(const Stop* from, const Stop* to, double meters);

        /**
         * Возвращает расстояние в метрах между двумя остановками.
         */
        double GetDistance(const Stop* from, const Stop* to) const;

        /**
         * Полностью очищает каталог
         */
        void Clear();

        /**
         * Обработка запроса на объединение остановок
         */
        void AddStopMergeRequest(const domain::StopMergeRequest& request);

        /**
         * Применение всех объединений
         */
        void ApplyStopMerges();

        /**
         * Получить реальную остановку для отображения (учитывая объединения)
         */
        const Stop* GetCanonicalStop(std::string_view stop_name) const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
        void AccumulateRoute(const Stop* from, const Stop* to, double& road_length, double& geo_length) const;
    };

}  // namespace tc
