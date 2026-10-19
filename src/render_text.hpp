/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * Plain-text (RFC-like ASCII) renderer with optional ANSI truecolor output.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef PROTOCOL_RENDER_TEXT_HPP
#define PROTOCOL_RENDER_TEXT_HPP

#include <string>

#include "protocol.hpp"
#include "theme.hpp"

namespace protocol {

/** Renders a grid as plain RFC-like ASCII text (no trailing newline). */
std::string render_text(const Grid& grid);

/** Renders a grid as ANSI-colored terminal text using a theme. */
std::string render_text_ansi(const Grid& grid, const Theme& theme);

} // namespace protocol

#endif // PROTOCOL_RENDER_TEXT_HPP
