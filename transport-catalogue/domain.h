#pragma once

#include "geo.h"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace domain {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
    std::unordered_map<const Stop*, int> road_distances;
    std::set<std::string> buses;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_roundtrip = false;
};

struct BusStat {
    int stop_count = 0;
    int unique_stop_count = 0;
    int route_length = 0;
    double curvature = 0.0;
};

}  // namespace domain
