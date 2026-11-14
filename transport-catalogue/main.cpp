#include "json_reader.h"
#include "transport_catalogue.h"

#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using transport_catalogue::TransportCatalogue;

std::string_view Trim(std::string_view text) {
    const size_t first = text.find_first_not_of(" \t\r\n");
    if (first == text.npos) {
        return {};
    }
    const size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::vector<std::string> Split(std::string_view text, char delimiter) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= text.size()) {
        const size_t end = text.find(delimiter, start);
        const std::string_view part = Trim(text.substr(start, end - start));
        if (!part.empty()) {
            parts.emplace_back(part);
        }
        if (end == text.npos) {
            break;
        }
        start = end + 1;
    }
    return parts;
}

struct StopCommand {
    std::string name;
    geo::Coordinates coordinates;
    std::vector<std::pair<std::string, int>> distances;
};

struct BusCommand {
    std::string name;
    std::vector<std::string> stops;
    bool is_roundtrip = false;
};

StopCommand ParseStop(std::string_view name, std::string_view description) {
    const std::vector<std::string> parts = Split(description, ',');
    StopCommand command;
    command.name = name;
    command.coordinates = {std::stod(parts.at(0)), std::stod(parts.at(1))};
    for (size_t i = 2; i < parts.size(); ++i) {
        const std::string_view part = parts[i];
        const size_t separator = part.find("m to ");
        const int distance = std::stoi(std::string(part.substr(0, separator)));
        command.distances.emplace_back(std::string(Trim(part.substr(separator + 5))), distance);
    }
    return command;
}

BusCommand ParseBus(std::string_view name, std::string_view description) {
    const bool is_roundtrip = description.find('>') != description.npos;
    return {std::string(name), Split(description, is_roundtrip ? '>' : '-'), is_roundtrip};
}

void ProcessText(std::istream& input, std::ostream& output) {
    int base_count = 0;
    input >> base_count;
    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::vector<StopCommand> stops;
    std::vector<BusCommand> buses;
    for (int i = 0; i < base_count; ++i) {
        std::string line;
        std::getline(input, line);
        const size_t colon = line.find(':');
        const std::string_view header = Trim(std::string_view(line).substr(0, colon));
        const std::string_view description = Trim(std::string_view(line).substr(colon + 1));
        if (header.substr(0, 5) == "Stop ") {
            stops.push_back(ParseStop(Trim(header.substr(5)), description));
        } else if (header.substr(0, 4) == "Bus ") {
            buses.push_back(ParseBus(Trim(header.substr(4)), description));
        }
    }

    TransportCatalogue catalogue;
    for (const StopCommand& stop : stops) {
        catalogue.AddStop(stop.name, stop.coordinates);
    }
    for (const StopCommand& stop : stops) {
        for (const auto& [destination, distance] : stop.distances) {
            catalogue.SetDistance(stop.name, destination, distance);
        }
    }
    for (const BusCommand& bus : buses) {
        catalogue.AddBus(bus.name, bus.stops, bus.is_roundtrip);
    }

    int stat_count = 0;
    input >> stat_count;
    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    for (int i = 0; i < stat_count; ++i) {
        std::string line;
        std::getline(input, line);
        const std::string_view request = Trim(line);
        if (request.substr(0, 4) == "Bus ") {
            const std::string_view name = Trim(request.substr(4));
            const domain::Bus* bus = catalogue.FindBus(std::string(name));
            if (bus == nullptr) {
                output << "Bus " << name << ": not found\n";
            } else {
                const domain::BusStat stat = catalogue.GetBusStat(*bus);
                output << "Bus " << name << ": " << stat.stop_count << " stops on route, "
                       << stat.unique_stop_count << " unique stops, " << stat.route_length
                       << " route length, " << stat.curvature << " curvature\n";
            }
        } else if (request.substr(0, 5) == "Stop ") {
            const std::string_view name = Trim(request.substr(5));
            const domain::Stop* stop = catalogue.FindStop(std::string(name));
            if (stop == nullptr) {
                output << "Stop " << name << ": not found\n";
            } else if (stop->buses.empty()) {
                output << "Stop " << name << ": no buses\n";
            } else {
                output << "Stop " << name << ": buses";
                for (const std::string& bus_name : stop->buses) {
                    output << ' ' << bus_name;
                }
                output << '\n';
            }
        }
    }
}

}  // namespace

int main() {
    std::cin >> std::ws;
    if (std::cin.peek() == '{') {
        json_reader::Process(std::cin, std::cout);
    } else {
        ProcessText(std::cin, std::cout);
    }
}
