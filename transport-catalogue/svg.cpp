#include "svg.h"

namespace svg {

using namespace std::literals;

namespace detail {

void RenderEscaped(std::ostream& out, const std::string& value) {
    for (char ch : value) {
        switch (ch) {
            case '"': out << "&quot;"; break;
            case '\'': out << "&apos;"; break;
            case '<': out << "&lt;"; break;
            case '>': out << "&gt;"; break;
            case '&': out << "&amp;"; break;
            default: out.put(ch);
        }
    }
}

void RenderEscapedColor(std::ostream& out, const Color& color) {
    if (const auto* name = std::get_if<std::string>(&color)) {
        RenderEscaped(out, *name);
    } else {
        out << color;
    }
}

}  // namespace detail

std::ostream& operator<<(std::ostream& out, const Color& color) {
    if (std::holds_alternative<std::monostate>(color)) {
        return out << "none";
    }
    if (const auto* name = std::get_if<std::string>(&color)) {
        return out << *name;
    }
    if (const auto* rgb = std::get_if<Rgb>(&color)) {
        return out << "rgb(" << static_cast<int>(rgb->red) << ','
                   << static_cast<int>(rgb->green) << ','
                   << static_cast<int>(rgb->blue) << ')';
    }
    const auto& rgba = std::get<Rgba>(color);
    return out << "rgba(" << static_cast<int>(rgba.red) << ','
               << static_cast<int>(rgba.green) << ','
               << static_cast<int>(rgba.blue) << ',' << rgba.opacity << ')';
}

std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
    switch (line_cap) {
        case StrokeLineCap::BUTT: out << "butt"; break;
        case StrokeLineCap::ROUND: out << "round"; break;
        case StrokeLineCap::SQUARE: out << "square"; break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
    switch (line_join) {
        case StrokeLineJoin::ARCS: out << "arcs"; break;
        case StrokeLineJoin::BEVEL: out << "bevel"; break;
        case StrokeLineJoin::MITER: out << "miter"; break;
        case StrokeLineJoin::MITER_CLIP: out << "miter-clip"; break;
        case StrokeLineJoin::ROUND: out << "round"; break;
    }
    return out;
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\""sv;
    out << " r=\""sv << radius_ << '"';
    RenderAttrs(out);
    out << " />"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\"";
    bool first = true;
    for (const Point& point : points_) {
        if (!first) {
            out << ' ';
        }
        first = false;
        out << point.x << ',' << point.y;
    }
    out << '"';
    RenderAttrs(out);
    out << " />";
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos) {
    position_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
}

Text& Text::SetData(std::string data) {
    data_ = std::move(data);
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text x=\"" << position_.x << "\" y=\"" << position_.y
        << "\" dx=\"" << offset_.x << "\" dy=\"" << offset_.y
        << "\" font-size=\"" << font_size_ << '"';
    RenderAttrs(out);
    if (!font_family_.empty()) {
        out << " font-family=\"";
        detail::RenderEscaped(out, font_family_);
        out << '"';
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\"";
        detail::RenderEscaped(out, font_weight_);
        out << '"';
    }
    out << '>';
    detail::RenderEscaped(out, data_);
    out << "</text>";
}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.push_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";
    const RenderContext context(out, 2, 2);
    for (const auto& object : objects_) {
        object->Render(context);
    }
    out << "</svg>\n";
}

}  // namespace svg
