/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef PROTOCOL_RENDER_PNG_HPP
#define PROTOCOL_RENDER_PNG_HPP

#include <cstdint>
#include <vector>

#include "protocol.hpp"
#include "theme.hpp"

namespace protocol {

struct PngOptions {
    int scale = 2;          ///< Pixel scale of one character cell (1..8)
    bool transparent = false; ///< Transparent canvas background
};

/**
 * Renders one or more protocol grids (stacked vertically) and returns the
 * encoded PNG file bytes. Returns an empty vector on failure.
 */
std::vector<std::uint8_t> render_png(const std::vector<Grid>& grids, const Theme& theme,
                                     const PngOptions& opts = PngOptions{});

} // namespace protocol

#endif // PROTOCOL_RENDER_PNG_HPP
