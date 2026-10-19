/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * SVG renderer.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef PROTOCOL_RENDER_SVG_HPP
#define PROTOCOL_RENDER_SVG_HPP

#include <string>
#include <vector>

#include "protocol.hpp"
#include "theme.hpp"

namespace protocol {

struct SvgOptions {
    double font_size = 14.0;  ///< Base font size in px (SVG scales cleanly anyway)
    bool transparent = false; ///< Omit the canvas background rectangle
    bool ascii = false;       ///< Render characters literally (terminal look)
    std::string font_family;  ///< Preferred font family; empty = modern default stack
};

/** Renders one or more protocol grids (stacked vertically) as an SVG image. */
std::string render_svg(const std::vector<Grid>& grids, const Theme& theme,
                       const SvgOptions& opts = SvgOptions{});

} // namespace protocol

#endif // PROTOCOL_RENDER_SVG_HPP
