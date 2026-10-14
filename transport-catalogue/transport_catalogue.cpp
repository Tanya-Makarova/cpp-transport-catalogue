#include "transport_catalogue.h"

#include <cassert>
#include <unordered_set>
#include <utility>

namespace transport_catalogue {

void TransportCatalogue::AddStop(std::string new_name, geo::Coordinates coordinates) {
    stops_.push_back({std::move(new_name), coordinates});
    Stop& stop = stops_.back();
    stopname_to_stop_[stop.name] = &stop;
    stopname_to_buses_[stop.name];
}

const TransportCatalogue::Stop* TransportCatalogue::FindStop(std::string_view name_of_stop) const {
    const auto it = stopname_to_stop_.find(name_of_stop);
    return it == stopname_to_stop_.end() ? nullptr : it->second;
}

void TransportCatalogue::AddBus(std::string new_name,
                                const std::vector<std::string_view>& new_stops) {
    buses_.push_back({std::move(new_name), {}});
    Bus& bus = buses_.back();
    bus.stops.reserve(new_stops.size());

    for (std::string_view stop_name : new_stops) {
        const Stop* stop = FindStop(stop_name);
        assert(stop != nullptr);
        bus.stops.push_back(stop);
        stopname_to_buses_[stop->name].insert(bus.name);
    }
    busname_to_bus_[bus.name] = &bus;
}

const TransportCatalogue::Bus* TransportCatalogue::FindBus(std::string_view name_of_bus) const {
    const auto it = busname_to_bus_.find(name_of_bus);
    return it == busname_to_bus_.end() ? nullptr : it->second;
}

const std::set<std::string_view>* TransportCatalogue::GetBusesForStop(
    std::string_view stop_name) const {
    const auto it = stopname_to_buses_.find(stop_name);
    return it == stopname_to_buses_.end() ? nullptr : &it->second;
}

void TransportCatalogue::SetDistance(std::string_view from,
                                     std::string_view to,
                                     int distance) {
    const Stop* from_stop = FindStop(from);
    const Stop* to_stop = FindStop(to);
    assert(from_stop != nullptr);
    assert(to_stop != nullptr);

    distances_[from_stop][to_stop] = distance;
}

int TransportCatalogue::GetDistance(const Stop* from, const Stop* to) const {
    if (const auto from_it = distances_.find(from); from_it != distances_.end()) {
        if (const auto to_it = from_it->second.find(to); to_it != from_it->second.end()) {
            return to_it->second;
        }
    }

    // If the direct distance was not specified, use the reverse direction.
    if (const auto to_it = distances_.find(to); to_it != distances_.end()) {
        if (const auto from_it = to_it->second.find(from); from_it != to_it->second.end()) {
            return from_it->second;
        }
    }

    assert(false);
    return 0;
}

std::optional<BusInfo> TransportCatalogue::GetInfo(std::string_view name_of_bus) const {
    const Bus* route = FindBus(name_of_bus);
    if (route == nullptr) {
        return std::nullopt;
    }

    BusInfo info;
    info.name = route->name;
    info.stop_count = route->stops.size();

    std::unordered_set<const Stop*> unique_stops(route->stops.begin(), route->stops.end());
    info.unique_stop_count = unique_stops.size();

    double geographical_length = 0.0;
    for (std::size_t i = 1; i < route->stops.size(); ++i) {
        const Stop* from = route->stops[i - 1];
        const Stop* to = route->stops[i];

        info.route_length += GetDistance(from, to);
        geographical_length += geo::ComputeDistance(from->coordinates, to->coordinates);
    }

    if (geographical_length != 0.0) {
        info.curvature = info.route_length / geographical_length;
    }

    return info;
}

}  // namespace transport_catalogue
