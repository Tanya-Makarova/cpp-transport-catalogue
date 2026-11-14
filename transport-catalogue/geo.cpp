#include "geo.h"

#include <algorithm>
#include <cmath>

namespace geo {

double ComputeDistance(Coordinates from, Coordinates to) {
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kEarthRadius = 6371000.0;
    const double from_lat = from.lat * kPi / 180.0;
    const double to_lat = to.lat * kPi / 180.0;
    const double longitude_delta = (from.lng - to.lng) * kPi / 180.0;
    const double cosine = std::sin(from_lat) * std::sin(to_lat)
        + std::cos(from_lat) * std::cos(to_lat) * std::cos(longitude_delta);
    return kEarthRadius * std::acos(std::clamp(cosine, -1.0, 1.0));
}

}  // namespace geo
