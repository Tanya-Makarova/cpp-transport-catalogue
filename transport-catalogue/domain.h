#pragma once

#include "geo.h"

#include <string>
#include <vector>

namespace domain {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
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
