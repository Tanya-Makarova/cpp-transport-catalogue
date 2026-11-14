#include "map_renderer.h"

#include <algorithm>
#include <map>
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
    std::vector<const domain::Bus*> drawable_buses;
    std::map<std::string, const domain::Stop*> drawable_stops;
    std::vector<geo::Coordinates> coordinates;
    for (const domain::Bus* bus : buses) {
        if (bus->stops.empty()) {
            continue;
        }
        drawable_buses.push_back(bus);
        for (const domain::Stop* stop : bus->stops) {
            coordinates.push_back(stop->coordinates);
            drawable_stops.emplace(stop->name, stop);
        }
    }
    std::sort(drawable_buses.begin(), drawable_buses.end(),
        [](const domain::Bus* lhs, const domain::Bus* rhs) { return lhs->name < rhs->name; });

    const SphereProjector projector(coordinates.begin(), coordinates.end(),
                                    settings_.width, settings_.height, settings_.padding);
    svg::Document document;
    size_t color_index = 0;
    for (const domain::Bus* bus : drawable_buses) {
        svg::Polyline line;
        for (const domain::Stop* stop : bus->stops) {
            line.AddPoint(projector(stop->coordinates));
        }
        if (!bus->is_roundtrip) {
            for (size_t i = bus->stops.size() - 1; i > 0; --i) {
                line.AddPoint(projector(bus->stops[i - 1]->coordinates));
            }
        }
        line.SetFillColor(svg::NoneColor)
            .SetStrokeColor(settings_.color_palette[color_index % settings_.color_palette.size()])
            .SetStrokeWidth(settings_.line_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        document.Add(std::move(line));
        ++color_index;
    }

    for (size_t i = 0; i < drawable_buses.size(); ++i) {
        const domain::Bus* bus = drawable_buses[i];
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

    for (const auto& [name, stop] : drawable_stops) {
        svg::Circle circle;
        circle.SetCenter(projector(stop->coordinates))
            .SetRadius(settings_.stop_radius)
            .SetFillColor("white");
        document.Add(std::move(circle));
    }

    for (const auto& [name, stop] : drawable_stops) {
        AddLabel(document, projector(stop->coordinates), settings_.stop_label_offset,
                 settings_.stop_label_font_size, name, false, std::string("black"), settings_);
    }
    return document;
}

}  // namespace renderer
