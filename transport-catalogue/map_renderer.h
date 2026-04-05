#pragma once

#include <vector>

#include "domain.h"
#include "libs/svg/svg.h"

namespace renderer {

    struct RenderSettings {
        double width = 1200.0;
        double height = 1200.0;
        double padding = 50.0;
        double line_width = 14.0;
        std::vector<svg::Color> color_palette = {
            std::string("green"),
            svg::Rgb{255, 160, 0},
            std::string("red")
        };
        double stop_radius = 5.0;
        int bus_label_font_size = 20;
        svg::Point bus_label_offset = { 7.0, 15.0 };
        int stop_label_font_size = 20;
        svg::Point stop_label_offset = { 7.0, -3.0 };
        svg::Color underlayer_color = svg::Rgba{ 255, 255, 255, 0.85 };
        double underlayer_width = 3.0;
    };

    class MapRenderer {
    public:
        explicit MapRenderer(RenderSettings settings = {});

        svg::Document Render(const std::vector<const domain::Bus*>& buses) const;

    private:
        RenderSettings settings_;
    };

} // namespace renderer
