#pragma once

#include "domain.h"

#include <deque>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace transport_catalogue {

class TransportCatalogue {
public:
    void AddStop(std::string name, geo::Coordinates coordinates);
    void SetDistance(std::string_view from, std::string_view to, int distance);
    void AddBus(std::string name, const std::vector<std::string>& stop_names, bool is_roundtrip);

    const domain::Stop* FindStop(std::string_view name) const;
    const domain::Bus* FindBus(std::string_view name) const;
    const std::set<std::string_view>* GetBusesForStop(std::string_view stop_name) const;
    std::vector<const domain::Bus*> GetAllBuses() const;
    domain::BusStat GetBusStat(const domain::Bus& bus) const;

private:
    int GetDistance(const domain::Stop* from, const domain::Stop* to) const;

    std::deque<domain::Stop> stops_;
    std::unordered_map<std::string_view, domain::Stop*> stopname_to_stop_;
    std::unordered_map<std::string_view, std::set<std::string_view>> stopname_to_buses_;
    std::deque<domain::Bus> buses_;
    std::unordered_map<std::string_view, domain::Bus*> busname_to_bus_;
    std::unordered_map<const domain::Stop*,
                       std::unordered_map<const domain::Stop*, int>> distances_;
};

}  // namespace transport_catalogue
