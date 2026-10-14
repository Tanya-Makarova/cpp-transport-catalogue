#include "input_reader.h"

#include <istream>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace input_reader {
namespace {

std::string_view Trim(std::string_view text) {
    const std::size_t start = text.find_first_not_of(' ');
    if (start == text.npos) {
        return {};
    }
    return text.substr(start, text.find_last_not_of(' ') + 1 - start);
}

std::vector<std::string_view> Split(std::string_view text, char delimiter) {
    std::vector<std::string_view> result;

    std::size_t pos = 0;
    while ((pos = text.find_first_not_of(' ', pos)) < text.size()) {
        std::size_t delimiter_pos = text.find(delimiter, pos);
        if (delimiter_pos == text.npos) {
            delimiter_pos = text.size();
        }
        if (const std::string_view part = Trim(text.substr(pos, delimiter_pos - pos));
            !part.empty()) {
            result.push_back(part);
        }
        pos = delimiter_pos + 1;
    }

    return result;
}

struct StopDescription {
    geo::Coordinates coordinates;
    std::vector<std::pair<std::string_view, int>> distances;
};

StopDescription ParseStopDescription(std::string_view description) {
    const std::vector<std::string_view> parts = Split(description, ',');
    if (parts.size() < 2) {
        throw std::invalid_argument("Invalid stop description");
    }

    StopDescription result{
        {std::stod(std::string(parts[0])), std::stod(std::string(parts[1]))},
        {}
    };
    result.distances.reserve(parts.size() - 2);

    constexpr std::string_view distance_separator = "m to ";
    for (std::size_t i = 2; i < parts.size(); ++i) {
        const std::size_t separator_pos = parts[i].find(distance_separator);
        if (separator_pos == parts[i].npos) {
            throw std::invalid_argument("Invalid road distance");
        }

        const int distance = std::stoi(std::string(parts[i].substr(0, separator_pos)));
        const std::string_view destination = Trim(parts[i].substr(
            separator_pos + distance_separator.size()));
        if (destination.empty()) {
            throw std::invalid_argument("Empty destination stop name");
        }
        result.distances.emplace_back(destination, distance);
    }

    return result;
}

std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    std::vector<std::string_view> stops = Split(route, '-');
    if (stops.empty()) {
        return {};
    }

    std::vector<std::string_view> result(stops.begin(), stops.end());
    result.insert(result.end(), std::next(stops.rbegin()), stops.rend());
    return result;
}

CommandDescription ParseCommandDescription(std::string_view line) {
    const std::size_t colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    const std::size_t space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    const std::size_t name_start = line.find_first_not_of(' ', space_pos);
    if (name_start >= colon_pos) {
        return {};
    }

    return {
        std::string(line.substr(0, space_pos)),
        std::string(line.substr(name_start, colon_pos - name_start)),
        std::string(line.substr(colon_pos + 1))
    };
}

}  // namespace

void InputReader::ReadInput(std::istream& input) {
    int base_request_count = 0;
    input >> base_request_count >> std::ws;
    if (base_request_count < 0) {
        throw std::invalid_argument("Negative request count");
    }

    commands_.clear();
    commands_.reserve(static_cast<std::size_t>(base_request_count));

    for (int i = 0; i < base_request_count; ++i) {
        std::string line;
        std::getline(input, line);
        ParseLine(line);
    }
}

void InputReader::ParseLine(std::string_view line) {
    CommandDescription command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands(transport_catalogue::TransportCatalogue& catalogue) const {
    // First add all stops, because distances may refer to stops declared later.
    for (const CommandDescription& action : commands_) {
        if (action.command == "Stop") {
            const StopDescription description = ParseStopDescription(action.description);
            catalogue.AddStop(action.id, description.coordinates);
        }
    }

    // Then add all directed road distances.
    for (const CommandDescription& action : commands_) {
        if (action.command == "Stop") {
            const StopDescription description = ParseStopDescription(action.description);
            for (const auto& [destination, distance] : description.distances) {
                catalogue.SetDistance(action.id, destination, distance);
            }
        }
    }

    // Finally build routes from stable pointers to already existing stops.
    for (const CommandDescription& action : commands_) {
        if (action.command == "Bus") {
            catalogue.AddBus(action.id, ParseRoute(action.description));
        }
    }
}

}  // namespace input_reader
