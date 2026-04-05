#include "request_handler.h"

#include <algorithm>
#include <sstream>

namespace tc {

    RequestHandler::RequestHandler(const TransportCatalogue& db)
        : db_(db) {}

    RequestHandler::StopInfo RequestHandler::GetStopInfo(std::string_view stop_name) const {
        StopInfo info;

        if (db_.FindStop(stop_name) == nullptr) {
            return info;
        }

        info.found = true;
        info.buses = db_.GetBusesByStop(stop_name);
        return info;
    }

    RequestHandler::BusInfo RequestHandler::GetBusInfo(std::string_view bus_name) const {
        BusInfo info;

        const auto stat = db_.GetBusStat(bus_name);
        if (!stat) {
            return info;
        }

        info.found = true;
        info.stop_count = static_cast<int>(stat->stops_on_route);
        info.unique_stop_count = static_cast<int>(stat->unique_stops);
        info.route_length = stat->route_length;
        info.curvature = stat->curvature;

        return info;
    }

    std::vector<const domain::Bus*> RequestHandler::GetBusesForRender() const {
        std::vector<const domain::Bus*> buses = db_.GetAllBuses();

        std::sort(buses.begin(), buses.end(),
                  [](const domain::Bus* lhs, const domain::Bus* rhs) {
            return lhs->name < rhs->name;
        });

        return buses;
    }

    std::string RequestHandler::RenderMap(const renderer::MapRenderer& renderer) const {
        svg::Document doc = renderer.Render(GetBusesForRender());

        std::ostringstream out;
        doc.Render(out);
        return out.str();
    }

} // namespace tc
