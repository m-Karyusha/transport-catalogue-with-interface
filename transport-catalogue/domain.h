#pragma once

#include <string>
#include <vector>

#include "libs/geo/geo.h"

namespace domain {

    // Остановка
    struct Stop {
        std::string name;
        geo::Coordinates coords;
    };

    // Маршрут
    struct Bus {
        std::string name;
        std::vector<const Stop*> stops;     // указатели на остановки на маршруте
        bool is_roundtrip;                  // true - кольцевой, false - линейный
    };
}
