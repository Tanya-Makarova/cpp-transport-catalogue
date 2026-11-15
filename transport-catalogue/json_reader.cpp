#include "json_reader.h"

#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace json_reader {

svg::Color JSONReader::ParseColor(const json::Node& node) {
    if (node.IsString()) {
        return node.AsString();
    }
    const json::Array& components = node.AsArray();
    const auto red = static_cast<uint8_t>(components.at(0).AsInt());
    const auto green = static_cast<uint8_t>(components.at(1).AsInt());
    const auto blue = static_cast<uint8_t>(components.at(2).AsInt());
    if (components.size() == 3) {
        return svg::Rgb{red, green, blue};
    }
    return svg::Rgba{red, green, blue, components.at(3).AsDouble()};
}

svg::Point JSONReader::ParsePoint(const json::Node& node) {
    const json::Array& coordinates = node.AsArray();
    return {coordinates.at(0).AsDouble(), coordinates.at(1).AsDouble()};
}

renderer::RenderSettings JSONReader::ParseRenderSettings(const json::Dict& request) {
    renderer::RenderSettings settings;
    settings.width = request.at("width").AsDouble();
    settings.height = request.at("height").AsDouble();
    settings.padding = request.at("padding").AsDouble();
    settings.line_width = request.at("line_width").AsDouble();
    settings.stop_radius = request.at("stop_radius").AsDouble();
    settings.bus_label_font_size = static_cast<uint32_t>(request.at("bus_label_font_size").AsInt());
    settings.bus_label_offset = ParsePoint(request.at("bus_label_offset"));
    settings.stop_label_font_size = static_cast<uint32_t>(request.at("stop_label_font_size").AsInt());
    settings.stop_label_offset = ParsePoint(request.at("stop_label_offset"));
    settings.underlayer_color = ParseColor(request.at("underlayer_color"));
    settings.underlayer_width = request.at("underlayer_width").AsDouble();
    for (const json::Node& color : request.at("color_palette").AsArray()) {
        settings.color_palette.push_back(ParseColor(color));
    }
    return settings;
}

void JSONReader::AddBaseRequests(const json::Array& requests) {
    AddStops(requests);
    AddDistances(requests);
    AddBuses(requests);
}

void JSONReader::AddStops(const json::Array& requests) {
    for (const json::Node& node : requests) {
        const json::Dict& request = node.AsMap();
        if (request.at("type").AsString() == "Stop") {
            catalogue_.AddStop(request.at("name").AsString(),
                               {request.at("latitude").AsDouble(),
                                request.at("longitude").AsDouble()});
        }
    }
}

void JSONReader::AddDistances(const json::Array& requests) {
    for (const json::Node& node : requests) {
        const json::Dict& request = node.AsMap();
        if (request.at("type").AsString() == "Stop") {
            const std::string& from = request.at("name").AsString();
            if (const auto distances = request.find("road_distances"); distances != request.end()) {
                for (const auto& [to, distance] : distances->second.AsMap()) {
                    catalogue_.SetDistance(from, to, distance.AsInt());
                }
            }
        }
    }
}

void JSONReader::AddBuses(const json::Array& requests) {
    for (const json::Node& node : requests) {
        const json::Dict& request = node.AsMap();
        if (request.at("type").AsString() == "Bus") {
            std::vector<std::string> stop_names;
            for (const json::Node& stop : request.at("stops").AsArray()) {
                stop_names.push_back(stop.AsString());
            }
            catalogue_.AddBus(request.at("name").AsString(), stop_names,
                              request.at("is_roundtrip").AsBool());
        }
    }
}

json::Node JSONReader::AnswerRequest(const json::Dict& request) const {
    json::Dict answer{{"request_id", request.at("id")}};
    const std::string& name = request.at("name").AsString();
    const std::string& type = request.at("type").AsString();

    if (type == "Bus") {
        if (const domain::Bus* bus = catalogue_.FindBus(name)) {
            const domain::BusStat stat = catalogue_.GetBusStat(*bus);
            answer.emplace("curvature", stat.curvature);
            answer.emplace("route_length", stat.route_length);
            answer.emplace("stop_count", stat.stop_count);
            answer.emplace("unique_stop_count", stat.unique_stop_count);
        } else {
            answer.emplace("error_message", "not found");
        }
    } else if (type == "Stop") {
        if (catalogue_.FindStop(name)) {
            json::Array buses;
            const auto* bus_names = catalogue_.GetBusesForStop(name);
            buses.reserve(bus_names->size());
            for (const std::string_view bus_name : *bus_names) {
                buses.emplace_back(std::string(bus_name));
            }
            answer.emplace("buses", std::move(buses));
        } else {
            answer.emplace("error_message", "not found");
        }
    }
    return answer;
}

void JSONReader::Process(std::istream& input, std::ostream& output) {
    const json::Document document = json::Load(input);
    const json::Dict& root = document.GetRoot().AsMap();

    AddBaseRequests(root.at("base_requests").AsArray());

    json::Array responses;
    const json::Array& requests = root.at("stat_requests").AsArray();
    responses.reserve(requests.size());
    std::optional<std::string> rendered_map;
    for (const json::Node& request : requests) {
        const json::Dict& request_data = request.AsMap();
        if (request_data.at("type").AsString() == "Map") {
            if (!rendered_map) {
                const renderer::MapRenderer renderer(
                    ParseRenderSettings(root.at("render_settings").AsMap()));
                const RequestHandler handler(catalogue_, renderer);
                std::ostringstream svg_output;
                handler.RenderMap().Render(svg_output);
                rendered_map = svg_output.str();
            }
            responses.emplace_back(json::Dict{
                {"request_id", request_data.at("id")}, {"map", *rendered_map}});
        } else {
            responses.push_back(AnswerRequest(request_data));
        }
    }
    json::Print(json::Document{json::Node{std::move(responses)}}, output);
}

}  // namespace json_reader
