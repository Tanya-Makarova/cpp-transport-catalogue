#pragma once

#include "domain.h"
#include "geo.h"
#include "svg.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace renderer {

struct RenderSettings {
    double width = 0.0;
    double height = 0.0;
    double padding = 0.0;
    double line_width = 0.0;
    double stop_radius = 0.0;
    uint32_t bus_label_font_size = 0;
    svg::Point bus_label_offset;
    uint32_t stop_label_font_size = 0;
    svg::Point stop_label_offset;
    svg::Color underlayer_color;
    double underlayer_width = 0.0;
    std::vector<svg::Color> color_palette;
};

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt begin, PointInputIt end,
                    double max_width, double max_height, double padding)
        : padding_(padding) {
        if (begin == end) {
            return;
        }

        const auto [left, right] = std::minmax_element(begin, end,
            [](const auto& lhs, const auto& rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left->lng;
        const double max_lon = right->lng;

        const auto [bottom, top] = std::minmax_element(begin, end,
            [](const auto& lhs, const auto& rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom->lat;
        max_lat_ = top->lat;

        std::optional<double> width_zoom;
        std::optional<double> height_zoom;
        constexpr double epsilon = 1e-6;
        if (std::abs(max_lon - min_lon_) >= epsilon) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }
        if (std::abs(max_lat_ - min_lat) >= epsilon) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }
        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        }
    }

    svg::Point operator()(geo::Coordinates coordinates) const {
        return {(coordinates.lng - min_lon_) * zoom_coeff_ + padding_,
                (max_lat_ - coordinates.lat) * zoom_coeff_ + padding_};
    }

private:
    double padding_ = 0.0;
    double min_lon_ = 0.0;
    double max_lat_ = 0.0;
    double zoom_coeff_ = 0.0;
};

class MapRenderer {
public:
    explicit MapRenderer(RenderSettings settings);
    svg::Document Render(const std::vector<const domain::Bus*>& buses) const;

private:
    RenderSettings settings_;
};

}  // namespace renderer
