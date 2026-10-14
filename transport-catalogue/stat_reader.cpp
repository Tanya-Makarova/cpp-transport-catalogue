#include "stat_reader.h"

#include <istream>
#include <ostream>
#include <string>

namespace stat_reader {

void ParseAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue,
                       std::string_view request,
                       std::ostream& output) {
    constexpr std::string_view bus_prefix = "Bus ";
    constexpr std::string_view stop_prefix = "Stop ";

    if (request.substr(0, bus_prefix.size()) == bus_prefix) {
        const std::string_view bus_name = request.substr(bus_prefix.size());
        const auto info = transport_catalogue.GetInfo(bus_name);

        if (!info) {
            output << "Bus " << bus_name << ": not found\n";
        } else {
            output << "Bus " << info->name << ": " << info->stop_count << " stops on route, "
                   << info->unique_stop_count << " unique stops, " << info->route_length
                   << " route length, " << info->curvature << " curvature\n";
        }
    } else if (request.substr(0, stop_prefix.size()) == stop_prefix) {
        const std::string_view stop_name = request.substr(stop_prefix.size());
        const auto* buses = transport_catalogue.GetBusesForStop(stop_name);

        if (buses == nullptr) {
            output << "Stop " << stop_name << ": not found\n";
        } else if (buses->empty()) {
            output << "Stop " << stop_name << ": no buses\n";
        } else {
            output << "Stop " << stop_name << ": buses";
            for (std::string_view bus_name : *buses) {
                output << ' ' << bus_name;
            }
            output << '\n';
        }
    }
}

void ReadAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue,
                      std::istream& input,
                      std::ostream& output) {
    int stat_request_count = 0;
    input >> stat_request_count >> std::ws;

    for (int i = 0; i < stat_request_count; ++i) {
        std::string line;
        std::getline(input, line);
        ParseAndPrintStat(transport_catalogue, line, output);
    }
}

}  // namespace stat_reader
