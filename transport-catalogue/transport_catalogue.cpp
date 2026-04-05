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
    }

    void TransportCatalogue::AccumulateRoute(const Stop* from, const Stop* to, double& road_length, double& geo_length) const {
        road_length += GetDistance(from, to);
        geo_length += ComputeDistance(from->coords, to->coords);
    }


}  // namespace tc
