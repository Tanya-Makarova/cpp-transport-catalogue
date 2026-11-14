#pragma once

#include "domain.h"

#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

namespace transport_catalogue {

class TransportCatalogue {
public:
    void AddStop(std::string name, geo::Coordinates coordinates);
    void SetDistance(const std::string& from, const std::string& to, int distance);
    void AddBus(std::string name, const std::vector<std::string>& stop_names, bool is_roundtrip);

    const domain::Stop* FindStop(const std::string& name) const;
    const domain::Bus* FindBus(const std::string& name) const;
    std::vector<const domain::Bus*> GetAllBuses() const;
    domain::BusStat GetBusStat(const domain::Bus& bus) const;

private:
    int GetDistance(const domain::Stop* from, const domain::Stop* to) const;

    std::deque<domain::Stop> stops_;
    std::deque<domain::Bus> buses_;
    std::unordered_map<std::string, domain::Stop*> stop_index_;
    std::unordered_map<std::string, domain::Bus*> bus_index_;
};

}  // namespace transport_catalogue
