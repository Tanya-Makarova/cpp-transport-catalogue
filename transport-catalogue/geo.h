#pragma once

#include <algorithm>
#include <cmath>

namespace geo {

struct Coordinates {
    double lat;
    double lng;
    bool operator==(const Coordinates& other) const {
        return lat == other.lat && lng == other.lng;
    }
    bool operator!=(const Coordinates& other) const {
        return !(*this == other);
    }
};

inline double ComputeDistance(Coordinates from, Coordinates to) {
    using namespace std;
    if (from == to) {
        return 0;
    }
    static const double dr = 3.1415926535 / 180.;
    static const int kEarthRadius = 6371000;
    const double cosine = sin(from.lat * dr) * sin(to.lat * dr)
        + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr);
    return acos(clamp(cosine, -1.0, 1.0)) * kEarthRadius;
}

}  // namespace geo

// Для кольцевого маршрута A>B>C>A — сумма расстояний AB, BC, CA.
// Для некольцевого маршрута A-B-C — сумма расстояний AB, BC, CB, BA.
