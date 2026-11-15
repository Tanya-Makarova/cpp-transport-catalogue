#pragma once

#include "json.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

#include <iosfwd>

namespace json_reader {

class JSONReader {
public:
    void Process(std::istream& input, std::ostream& output);

private:
    static svg::Color ParseColor(const json::Node& node);
    static svg::Point ParsePoint(const json::Node& node);
    static renderer::RenderSettings ParseRenderSettings(const json::Dict& request);

    void AddBaseRequests(const json::Array& requests);
    void AddStops(const json::Array& requests);
    void AddDistances(const json::Array& requests);
    void AddBuses(const json::Array& requests);
    json::Node AnswerRequest(const json::Dict& request) const;

    transport_catalogue::TransportCatalogue catalogue_;
};

}  // namespace json_reader
