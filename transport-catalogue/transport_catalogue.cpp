#include "transport_catalogue.h"

#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace transport_catalogue {

void TransportCatalogue::AddStop(std::string name, geo::Coordinates coordinates) {
    stops_.push_back({std::move(name), coordinates, {}, {}});
    domain::Stop& stop = stops_.back();
    stop_index_[stop.name] = &stop;
}

void TransportCatalogue::SetDistance(const std::string& from, const std::string& to, int distance) {
    stop_index_.at(from)->road_distances[stop_index_.at(to)] = distance;
}

void TransportCatalogue::AddBus(std::string name, const std::vector<std::string>& stop_names,
                                bool is_roundtrip) {
    buses_.push_back({std::move(name), {}, is_roundtrip});
    domain::Bus& bus = buses_.back();
    bus.stops.reserve(stop_names.size());
    for (const std::string& stop_name : stop_names) {
        domain::Stop* stop = stop_index_.at(stop_name);
        bus.stops.push_back(stop);
        stop->buses.insert(bus.name);
    }
    bus_index_[bus.name] = &bus;
}

const domain::Stop* TransportCatalogue::FindStop(const std::string& name) const {
    const auto it = stop_index_.find(name);
    return it == stop_index_.end() ? nullptr : it->second;
}

const domain::Bus* TransportCatalogue::FindBus(const std::string& name) const {
    const auto it = bus_index_.find(name);
    return it == bus_index_.end() ? nullptr : it->second;
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
    if (const auto it = from->road_distances.find(to); it != from->road_distances.end()) {
        return it->second;
    }
    if (const auto it = to->road_distances.find(from); it != to->road_distances.end()) {
        return it->second;
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
