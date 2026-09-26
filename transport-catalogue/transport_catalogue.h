#pragma once
// хранение остановок и маршрутов, поиск по имени, получение статистики по маршрутам. Не занимается вводом и выводом
#include <deque>
#include <cstddef>
#include <unordered_map>
#include <string_view>
#include <vector>
#include <string>
#include <optional>
#include <set>

namespace transport_catalogue {

struct BusInfo {
	std::string_view name;
	std::size_t R = 0;
	std::size_t U = 0;
	double L = 0.0;
};

class TransportCatalogue {
	struct Stop
	{
		std::string name;
		double latitude;
		double longitude;

	};

	struct Bus {
    	std::string name;
    	std::vector<const Stop*> stops;  // Bus 750 -> [&Tolstopaltsevo, &Marushkino, &Rasskazovka, ...]
	};

public:
	TransportCatalogue() = default;

	void AddStop(std::string new_name, double new_latitude, double new_longitude);
	void AddBus(std::string new_name, const std::vector<std::string_view>& new_stops);
	const Stop* FindStop(std::string_view name_of_stop) const;
	const Bus* FindBus(std::string_view name_of_bus) const;
	std::optional<BusInfo> GetInfo(std::string_view name_of_bus) const;
	const std::set<std::string_view>* GetBusesForStop(std::string_view stop_name) const;
private:
	// База данных остановок
	std::deque<Stop> stops_;
	// Для быстрого поиска оп втремени
	std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
	std::unordered_map<std::string_view, std::set<std::string_view>> stopname_to_buses_;
	// База данных автобусов
	std::deque<Bus> buses_;
	// Для быстрого поиска маршрутов
	std::unordered_map<std::string_view, Bus*> busname_to_bus_;
};

}  // namespace transport_catalogue
