#include "transport_catalogue.h"

#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace transport_catalogue {

void TransportCatalogue::AddStop(std::string name, geo::Coordinates coordinates) {
    stops_.push_back({std::move(name), coordinates});
    domain::Stop& stop = stops_.back();
    stopname_to_stop_[stop.name] = &stop;
    stopname_to_buses_.try_emplace(stop.name);
}

void TransportCatalogue::SetDistance(std::string_view from, std::string_view to, int distance) {
    distances_[stopname_to_stop_.at(from)][stopname_to_stop_.at(to)] = distance;
}

void TransportCatalogue::AddBus(std::string name, const std::vector<std::string>& stop_names,
                                bool is_roundtrip) {
    buses_.push_back({std::move(name), {}, is_roundtrip});
    domain::Bus& bus = buses_.back();
    bus.stops.reserve(stop_names.size());
    for (const std::string& stop_name : stop_names) {
        domain::Stop* stop = stopname_to_stop_.at(stop_name);
        bus.stops.push_back(stop);
        stopname_to_buses_.at(stop->name).insert(bus.name);
    }
    busname_to_bus_[bus.name] = &bus;
}

const domain::Stop* TransportCatalogue::FindStop(std::string_view name) const {
    const auto it = stopname_to_stop_.find(name);
    return it == stopname_to_stop_.end() ? nullptr : it->second;
}

const domain::Bus* TransportCatalogue::FindBus(std::string_view name) const {
    const auto it = busname_to_bus_.find(name);
    return it == busname_to_bus_.end() ? nullptr : it->second;
}

const std::set<std::string_view>* TransportCatalogue::GetBusesForStop(
    std::string_view stop_name) const {
    const auto it = stopname_to_buses_.find(stop_name);
    return it == stopname_to_buses_.end() ? nullptr : &it->second;
}

std::vector<const domain::Bus*> TransportCatalogue::GetAllBuses() const {
    std::vector<const domain::Bus*> buses;
    buses.reserve(buses_.size());
    for (const domain::Bus& bus : buses_) {
        buses.push_back(&bus);
    }
    return buses;
}

int TransportCatalogue::GetDistance(const domain::Stop* from, const domain::Stop* to) const {
    if (from == to) {
        return 0;
    }
    if (const auto from_it = distances_.find(from); from_it != distances_.end()) {
        if (const auto to_it = from_it->second.find(to); to_it != from_it->second.end()) {
            return to_it->second;
        }
    }
    if (const auto to_it = distances_.find(to); to_it != distances_.end()) {
        if (const auto from_it = to_it->second.find(from); from_it != to_it->second.end()) {
            return from_it->second;
        }
    }
    throw std::out_of_range("Road distance is missing");
}

domain::BusStat TransportCatalogue::GetBusStat(const domain::Bus& bus) const {
    domain::BusStat stat;
    if (bus.stops.empty()) {
        return stat;
    }

    const int listed_stops = static_cast<int>(bus.stops.size());
    stat.stop_count = bus.is_roundtrip ? listed_stops : 2 * listed_stops - 1;
    stat.unique_stop_count = static_cast<int>(
        std::unordered_set<const domain::Stop*>(bus.stops.begin(), bus.stops.end()).size());

    double geographic_length = 0.0;
    for (size_t i = 1; i < bus.stops.size(); ++i) {
        const domain::Stop* from = bus.stops[i - 1];
        const domain::Stop* to = bus.stops[i];
        stat.route_length += GetDistance(from, to);
        geographic_length += geo::ComputeDistance(from->coordinates, to->coordinates);
        if (!bus.is_roundtrip) {
            stat.route_length += GetDistance(to, from);
            geographic_length += geo::ComputeDistance(to->coordinates, from->coordinates);
        }
    }
    if (geographic_length > 0.0) {
        stat.curvature = stat.route_length / geographic_length;
    }
    return stat;
}

}  // namespace transport_catalogue
