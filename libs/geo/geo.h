#pragma once

namespace geo {

    // Координаты
    struct Coordinates {
        double lat; // Широта
        double lng; // Долгота
        bool operator==(const Coordinates& other) const {
            return lat == other.lat && lng == other.lng;
        }
        bool operator!=(const Coordinates& other) const {
            return !(*this == other);
        }
    };

    /**
     * Вычисляет расстояние между координатами
     */
    double ComputeDistance(Coordinates from, Coordinates to);

}  // namespace geo
