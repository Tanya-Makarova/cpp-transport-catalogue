#pragma once

#include "geo.h"

#include <cstddef>
#include <deque>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace transport_catalogue {

struct BusInfo {
    std::string_view name;
    std::size_t stop_count = 0;
    std::size_t unique_stop_count = 0;
    int route_length = 0;
    double curvature = 0.0;
};

class TransportCatalogue {
    struct Stop {
        std::string name;
        geo::Coordinates coordinates;
    };

    struct Bus {
        std::string name;
        std::vector<const Stop*> stops;
    };

public:
    TransportCatalogue() = default;

    void AddStop(std::string new_name, geo::Coordinates coordinates);
    void AddBus(std::string new_name, const std::vector<std::string_view>& new_stops);
    void SetDistance(std::string_view from, std::string_view to, int distance);

    const Stop* FindStop(std::string_view name_of_stop) const;
    const Bus* FindBus(std::string_view name_of_bus) const;
    std::optional<BusInfo> GetInfo(std::string_view name_of_bus) const;
    const std::set<std::string_view>* GetBusesForStop(std::string_view stop_name) const;

private:
    int GetDistance(const Stop* from, const Stop* to) const;

    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
    std::unordered_map<std::string_view, std::set<std::string_view>> stopname_to_buses_;

    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Bus*> busname_to_bus_;

    std::unordered_map<const Stop*, std::unordered_map<const Stop*, int>> distances_;
};

}  // namespace transport_catalogue
