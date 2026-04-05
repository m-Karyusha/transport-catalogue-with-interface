#pragma once

#include <string>
#include <vector>

#include "transport_catalogue.h"
#include "map_renderer.h"

namespace tc {

        /**
         * Класс-обработчик запросов к транспортному каталогу
         */
        class RequestHandler {
        public:

            /**
             * Информация об остановке для ответа на запрос Stop
             */
            struct StopInfo {
                bool found = false;             // Найдена ли остановка
                std::vector<std::string> buses; // Найдена ли остановка
            };

            /**
             * Информация о маршруте для ответа на запрос Bus
             */
            struct BusInfo {
                bool found = false;         // Найден ли маршрут
                int stop_count = 0;         // Общее количество остановок на маршруте
                int unique_stop_count = 0;  // Количество уникальных остановок
                double route_length = 0.0;  // Длина маршрута (по дорогам)
                double curvature = 0.0;     // Извилистость маршрута
            };

            /**
             * Конструктор
             */
            explicit RequestHandler(const TransportCatalogue& db);

            /**
             * Возвращает информацию об остановке по её имени
             */
            StopInfo GetStopInfo(std::string_view stop_name) const;

            /**
             * Возвращает информацию о маршруте по его имени
             */
            BusInfo GetBusInfo(std::string_view bus_name) const;

            /**
             * Возвращает список всех маршрутов для рендеринга
             */
            std::vector<const domain::Bus*> GetBusesForRender() const;

            /**
             * Возвращает список всех маршрутов для рендеринга
             */
            std::string RenderMap(const renderer::MapRenderer& renderer) const;

        private:
            const TransportCatalogue& db_;  // Транспортный каталог
        };

} // namespace tc
