#include "svg.h"

namespace svg {

    using namespace std::literals;

    struct ColorPrinter {
        std::ostream& out;

        void operator()(std::monostate) const {
            out << "none";
        }

        void operator()(const std::string& color) const {
            out << color;
        }

        void operator()(Rgb color) const {
            out << "rgb("sv
                << static_cast<int>(color.red) << ","
                << static_cast<int>(color.green) << ","
                << static_cast<int>(color.blue) << ")";
        }

        void operator()(Rgba color) const {
            out << "rgba("sv
                << static_cast<int>(color.red) << ","
                << static_cast<int>(color.green) << ","
                << static_cast<int>(color.blue) << ","
                << color.opacity << ")";
        }
    };

    std::ostream& operator<<(std::ostream& out, const Color& color) {
        std::visit(ColorPrinter{ out }, color);
        return out;
    }

    std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
        switch (line_cap) {
            case StrokeLineCap::BUTT:
                out << "butt";
                break;
            case StrokeLineCap::ROUND:
                out << "round";
                break;
            case StrokeLineCap::SQUARE:
                out << "square";
                break;
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
        switch (line_join) {
            case StrokeLineJoin::ARCS:
                out << "arcs";
                break;
            case StrokeLineJoin::BEVEL:
                out << "bevel";
                break;
            case StrokeLineJoin::MITER:
                out << "miter";
                break;
            case StrokeLineJoin::MITER_CLIP:
                out << "miter-clip";
                break;
            case StrokeLineJoin::ROUND:
                out << "round";
                break;
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

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\" "sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    // ---------- Polyline ------------------

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\""sv;

        bool first = true;
        for (const auto& p : points_) {
            if (!first) out << " "sv;
            out << p.x << "," << p.y;
            first = false;
        }
        out << "\"";
        RenderAttrs(out);
        out << "/>"sv;
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

        out << "<text";
        RenderAttrs(out);

        // x и y
        out << " x=\"" << position_.x << "\" y=\"" << position_.y << "\"";
        // dx и dy
        out << " dx=\"" << offset_.x << "\" dy=\"" << offset_.y << "\"";
        // font-size
        out << " font-size=\"" << font_size_ << "\"";
        // font-family
        if (!font_family_.empty()) {
            out << " font-family=\"" << font_family_ << "\"";
        }
        // font-weight
        if (!font_weight_.empty()) {
            out << " font-weight=\"" << font_weight_ << "\"";
        }

        out << ">";

        // Экранирование текста
        for (char c : data_) {
            switch (c) {
                case '"':  out << "&quot;"; break;
                case '\'': out << "&apos;"; break;
                case '<':  out << "&lt;";   break;
                case '>':  out << "&gt;";   break;
                case '&':  out << "&amp;";  break;
                default:   out << c;        break;
            }
        }

        out << "</text>";
    }

    // ---------- Document ------------------

    void Document::Render(std::ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv;

        RenderContext ctx(out, 2, 2);  // отступ 2 пробела

        for (const auto& obj : objects_) {
            obj->Render(ctx);
        }

        out << "</svg>"sv << std::endl;
    }

    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.push_back(std::move(obj));
    }

}  // namespace svg
