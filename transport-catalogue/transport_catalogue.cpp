#include "transport_catalogue.h"
#include "geo.h"

#include <unordered_set>
#include <utility>

namespace transport_catalogue {

void TransportCatalogue::AddStop(std::string new_name, double new_latitude, double new_longitude){
    stops_.push_back({std::move(new_name), new_latitude, new_longitude});
    Stop& stop = stops_.back();
    stopname_to_stop_[stop.name] = &stop;
    stopname_to_buses_[stop.name];
}

const TransportCatalogue::Stop* TransportCatalogue::FindStop(std::string_view name_of_stop) const {
    auto it = stopname_to_stop_.find(name_of_stop);
    if(it == stopname_to_stop_.end()){
        return nullptr;
    }
    return it->second;
}

void TransportCatalogue::AddBus(std::string new_name, const std::vector<std::string_view>& new_stops){
    buses_.push_back({std::move(new_name), {}});
    Bus& bus = buses_.back();
    bus.stops.reserve(new_stops.size());

    for(std::string_view stop_name : new_stops){
        const Stop* stop = FindStop(stop_name);
        if (stop != nullptr) {
            bus.stops.push_back(stop);
            stopname_to_buses_[stop->name].insert(bus.name);
        }
    }
    busname_to_bus_[bus.name] = &bus;
}

const TransportCatalogue::Bus* TransportCatalogue::FindBus(std::string_view name_of_bus) const {
    auto it = busname_to_bus_.find(name_of_bus);
    if (it == busname_to_bus_.end()) {
        return nullptr;
    }
    return it->second;
}

const std::set<std::string_view>* TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {
    auto it = stopname_to_buses_.find(stop_name);
    if (it == stopname_to_buses_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::optional<BusInfo> TransportCatalogue::GetInfo(std::string_view name_of_bus) const {
    const Bus* route = FindBus(name_of_bus);
    if (route == nullptr) {
        return std::nullopt;
    }

    BusInfo info; 
    info.name = route->name;
    info.R = route->stops.size();
    
    std::unordered_set<std::string_view> pool;
    for(const Stop* stop : route->stops){
            pool.insert(stop->name);
    }
    info.U = pool.size();

    for(std::size_t i = 1; i < route->stops.size(); ++i){
        const Stop* from = route->stops[i - 1];
        const Stop* to = route->stops[i];

        info.L += geo::ComputeDistance({from->latitude, from->longitude},
                                       {to->latitude, to->longitude});
    }

    return info;
}

}  // namespace transport_catalogue
