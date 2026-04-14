#pragma once

#include <string>
#include <vector>

#include "libs/geo/geo.h"

namespace domain {

    // Остановка
    struct Stop {
        std::string name;
        geo::Coordinates coords;
        bool is_virtual = false;  // виртуальная остановка (объединение)
        std::vector<std::string> merged_stops;  // исходные остановки, если виртуальная
    };

    // Маршрут
    struct Bus {
        std::string name;
        std::vector<const Stop*> stops;     // указатели на остановки на маршруте
        bool is_roundtrip;                  // true - кольцевой, false - линейный
    };

    // Запрос объединения
    struct StopMergeRequest {
        std::string merged_name;           // Название объединенной остановки
        std::vector<std::string> stops;    // Список объединяемых остановок
        bool combine_coords = true;        // true - объединить остановки, false - нет
    };
}
