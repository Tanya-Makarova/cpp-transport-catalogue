#include "map_renderer.h"

#include <algorithm>
#include <string>
#include <utility>

namespace renderer {

namespace {

svg::Text MakeLabel(svg::Point position, svg::Point offset, uint32_t font_size,
                    const std::string& caption, bool bold) {
    svg::Text label;
    label.SetPosition(position)
        .SetOffset(offset)
        .SetFontSize(font_size)
        .SetFontFamily("Verdana")
        .SetData(caption);
    if (bold) {
        label.SetFontWeight("bold");
    }
    return label;
}

void AddLabel(svg::Document& document, svg::Point position, svg::Point offset,
              uint32_t font_size, const std::string& caption, bool bold,
              const svg::Color& color, const RenderSettings& settings) {
    svg::Text underlayer = MakeLabel(position, offset, font_size, caption, bold);
    underlayer.SetFillColor(settings.underlayer_color)
        .SetStrokeColor(settings.underlayer_color)
        .SetStrokeWidth(settings.underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    document.Add(std::move(underlayer));

    svg::Text label = MakeLabel(position, offset, font_size, caption, bold);
    label.SetFillColor(color);
    document.Add(std::move(label));
}

}  // namespace

MapRenderer::MapRenderer(RenderSettings settings) : settings_(std::move(settings)) {
}

svg::Document MapRenderer::Render(const std::vector<const domain::Bus*>& buses) const {
    const BusList drawable_buses = GetDrawableBuses(buses);
    const StopList drawable_stops = GetDrawableStops(drawable_buses);
    std::vector<geo::Coordinates> coordinates;
    coordinates.reserve(drawable_stops.size());
    for (const domain::Stop* stop : drawable_stops) {
        coordinates.push_back(stop->coordinates);
    }
    const SphereProjector projector(coordinates.begin(), coordinates.end(),
                                    settings_.width, settings_.height, settings_.padding);
    svg::Document document;
    RenderBusLines(document, drawable_buses, projector);
    RenderBusLabels(document, drawable_buses, projector);
    RenderStopCircles(document, drawable_stops, projector);
    RenderStopLabels(document, drawable_stops, projector);
    return document;
}

MapRenderer::BusList MapRenderer::GetDrawableBuses(const BusList& buses) {
    BusList drawable_buses;
    for (const domain::Bus* bus : buses) {
        if (!bus->stops.empty()) {
            drawable_buses.push_back(bus);
        }
    }
    std::sort(drawable_buses.begin(), drawable_buses.end(),
              [](const domain::Bus* lhs, const domain::Bus* rhs) {
                  return lhs->name < rhs->name;
              });
    return drawable_buses;
}

MapRenderer::StopList MapRenderer::GetDrawableStops(const BusList& buses) {
    StopList stops;
    for (const domain::Bus* bus : buses) {
        stops.insert(stops.end(), bus->stops.begin(), bus->stops.end());
    }
    std::sort(stops.begin(), stops.end(),
              [](const domain::Stop* lhs, const domain::Stop* rhs) {
                  return lhs->name < rhs->name;
              });
    stops.erase(std::unique(stops.begin(), stops.end(),
                            [](const domain::Stop* lhs, const domain::Stop* rhs) {
                                return lhs->name == rhs->name;
                            }), stops.end());
    return stops;
}

void MapRenderer::RenderBusLines(svg::Document& document, const BusList& buses,
                                 const SphereProjector& projector) const {
    for (size_t i = 0; i < buses.size(); ++i) {
        const domain::Bus* bus = buses[i];
        svg::Polyline line;
        for (const domain::Stop* stop : bus->stops) {
            line.AddPoint(projector(stop->coordinates));
        }
        if (!bus->is_roundtrip) {
            for (size_t stop_index = bus->stops.size() - 1; stop_index > 0; --stop_index) {
                line.AddPoint(projector(bus->stops[stop_index - 1]->coordinates));
            }
        }
        line.SetFillColor(svg::NoneColor)
            .SetStrokeColor(settings_.color_palette[i % settings_.color_palette.size()])
            .SetStrokeWidth(settings_.line_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        document.Add(std::move(line));
    }
}

void MapRenderer::RenderBusLabels(svg::Document& document, const BusList& buses,
                                  const SphereProjector& projector) const {
    for (size_t i = 0; i < buses.size(); ++i) {
        const domain::Bus* bus = buses[i];
        const svg::Color& color = settings_.color_palette[i % settings_.color_palette.size()];
        const auto add_bus_label = [&](const domain::Stop* stop) {
            AddLabel(document, projector(stop->coordinates), settings_.bus_label_offset,
                     settings_.bus_label_font_size, bus->name, true, color, settings_);
        };
        add_bus_label(bus->stops.front());
        if (!bus->is_roundtrip && bus->stops.back() != bus->stops.front()) {
            add_bus_label(bus->stops.back());
        }
    }
}

void MapRenderer::RenderStopCircles(svg::Document& document, const StopList& stops,
                                    const SphereProjector& projector) const {
    for (const domain::Stop* stop : stops) {
        svg::Circle circle;
        circle.SetCenter(projector(stop->coordinates))
            .SetRadius(settings_.stop_radius)
            .SetFillColor("white");
        document.Add(std::move(circle));
    }
}

void MapRenderer::RenderStopLabels(svg::Document& document, const StopList& stops,
                                   const SphereProjector& projector) const {
    for (const domain::Stop* stop : stops) {
        AddLabel(document, projector(stop->coordinates), settings_.stop_label_offset,
                 settings_.stop_label_font_size, stop->name, false, std::string("black"), settings_);
    }
}

}  // namespace renderer
