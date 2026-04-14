#pragma once

#include <iosfwd>

#include "libs/json/json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

namespace tc {

    class JsonReader {
    public:

        /**
         * Конструктор
         */
        explicit JsonReader(TransportCatalogue& catalogue)
            : catalogue_(catalogue) {}

        /**
         * Загружает данные в транспортный каталог из JSON-документа
         */
        void LoadData(const json::Document& doc);

        /**
         * Обрабатывает запросы к транспортному каталогу и формирует JSON-ответ
         */
        static json::Document ProcessRequests(const json::Document& doc,
                                              const tc::RequestHandler& handler);

        /**
         * Печатает JSON-документ в поток output
         */
        static void PrintDoc(const json::Document& doc, std::ostream& output);

        /**
         * Возвращает настройки рендеринга из JSON-документа
         */
        static renderer::RenderSettings LoadRenderSettings(const json::Document& doc);

        static bool HasMapRequests(const json::Document& doc);

    private:
        TransportCatalogue& catalogue_;  // Транспортный каталог

        /**
         * Описание расстояния между двумя остановками
         */
        struct DistanceDescription {
            std::string from;   // Начальная остановка
            std::string to;     // Конечная остановка
            int distance = 0;   // Расстояние между остановками
        };

        // ===================================
        // Получение значения из JSON по ключу
        // ===================================

        /**
         * Возвращает узел JSON-словаря по ключу
         */
        static const json::Node& GetNode(const json::Dict& dict, std::string_view key);

        /**
         * Возвращает строковое значение по ключу
         */
        static const std::string& GetString(const json::Dict& dict, std::string_view key);

        /**
         * Возвращает целочисленное значение по ключу
         */
        static int GetInt(const json::Dict& dict, std::string_view key);

        /**
         * Возвращает дробное значение по ключу
         */
        static double GetDouble(const json::Dict& dict, std::string_view key);

        /**
         * Возвращает булево значение по ключу
         */
        static bool GetBool(const json::Dict& dict, std::string_view key);

        /**
         * Возвращает массив по ключу
         */
        static const json::Array& GetArray(const json::Dict& dict, std::string_view key);

        /**
         * Возвращает словарь по ключу
         */
        static const json::Dict& GetDict(const json::Dict& dict, std::string_view key);

        // ===================================
        // Загрузка данных в каталог
        // ===================================

        /**
         * Загружает остановки из base_requests в каталог.
         * Собирает список расстояний между остановками
         */
        void LoadStops(const json::Array& base_requests, std::vector<DistanceDescription>& distances);

        /**
         * Загружает расстояния между остановками в каталог
         */
        void LoadDistances(const std::vector<DistanceDescription>& distances);

        /**
         * Загружает маршруты из base_requests в каталог
         */
        void LoadBuses(const json::Array& base_requests);

        /**
         * Загружает объединенные остановки из base_requests в каталог
         */
        void LoadStopMerges(const json::Array& base_requests);

        // ===================================
        // Формирование JSON-ответов
        // ===================================

        /**
         * Формирует JSON-ответ на запрос Stop
         */
        static json::Dict MakeStopResponse(const std::string& stop_name,
                                           const RequestHandler& handler);

        /**
         * Формирует JSON-ответ на запрос Bus
         */
        static json::Dict MakeBusResponse(const std::string& bus_name,
                                          const RequestHandler& handler);

        // ===================================
        // Парсинг JSON
        // ===================================

        /**
         * Парсит цвет в RGB(A)
         */
        static svg::Color ParseColor(const json::Node& node);

        /**
         * Парсит настройки рендеринга
         */
        static renderer::RenderSettings ParseRenderSettings(const json::Dict& dict);
    };

} // namespace tc
