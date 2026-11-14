#include "json_reader.h"

#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace json_reader {

namespace {

using transport_catalogue::TransportCatalogue;

svg::Color ParseColor(const json::Node& node) {
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

svg::Point ParsePoint(const json::Node& node) {
    const json::Array& coordinates = node.AsArray();
    return {coordinates.at(0).AsDouble(), coordinates.at(1).AsDouble()};
}

renderer::RenderSettings ParseRenderSettings(const json::Dict& request) {
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

void AddBaseRequests(const json::Array& requests, TransportCatalogue& catalogue) {
    // Stop coordinates must exist before distances and bus stop lists are resolved.
    for (const json::Node& node : requests) {
        const json::Dict& request = node.AsMap();
        if (request.at("type").AsString() == "Stop") {
            catalogue.AddStop(request.at("name").AsString(),
                              {request.at("latitude").AsDouble(),
                               request.at("longitude").AsDouble()});
        }
    }

    for (const json::Node& node : requests) {
        const json::Dict& request = node.AsMap();
        if (request.at("type").AsString() == "Stop") {
            const std::string& from = request.at("name").AsString();
            if (const auto distances = request.find("road_distances"); distances != request.end()) {
                for (const auto& [to, distance] : distances->second.AsMap()) {
                    catalogue.SetDistance(from, to, distance.AsInt());
                }
            }
        }
    }

    for (const json::Node& node : requests) {
        const json::Dict& request = node.AsMap();
        if (request.at("type").AsString() == "Bus") {
            std::vector<std::string> stop_names;
            for (const json::Node& stop : request.at("stops").AsArray()) {
                stop_names.push_back(stop.AsString());
            }
            catalogue.AddBus(request.at("name").AsString(), stop_names,
                             request.at("is_roundtrip").AsBool());
        }
    }
}

json::Node AnswerRequest(const json::Dict& request, const TransportCatalogue& catalogue) {
    json::Dict answer{{"request_id", request.at("id")}};
    const std::string& name = request.at("name").AsString();
    const std::string& type = request.at("type").AsString();

    if (type == "Bus") {
        if (const domain::Bus* bus = catalogue.FindBus(name)) {
            const domain::BusStat stat = catalogue.GetBusStat(*bus);
            answer.emplace("curvature", stat.curvature);
            answer.emplace("route_length", stat.route_length);
            answer.emplace("stop_count", stat.stop_count);
            answer.emplace("unique_stop_count", stat.unique_stop_count);
        } else {
            answer.emplace("error_message", "not found");
        }
    } else if (type == "Stop") {
        if (const domain::Stop* stop = catalogue.FindStop(name)) {
            json::Array buses;
            buses.reserve(stop->buses.size());
            for (const std::string& bus_name : stop->buses) {
                buses.emplace_back(bus_name);
            }
            answer.emplace("buses", std::move(buses));
        } else {
            answer.emplace("error_message", "not found");
        }
    }
    return answer;
}

}  // namespace

void Process(std::istream& input, std::ostream& output) {
    const json::Document document = json::Load(input);
    const json::Dict& root = document.GetRoot().AsMap();

    TransportCatalogue catalogue;
    AddBaseRequests(root.at("base_requests").AsArray(), catalogue);

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
                const RequestHandler handler(catalogue, renderer);
                std::ostringstream svg_output;
                handler.RenderMap().Render(svg_output);
                rendered_map = svg_output.str();
            }
            responses.emplace_back(json::Dict{
                {"request_id", request_data.at("id")}, {"map", *rendered_map}});
        } else {
            responses.push_back(AnswerRequest(request_data, catalogue));
        }
    }
    json::Print(json::Document{json::Node{std::move(responses)}}, output);
}

}  // namespace json_reader
