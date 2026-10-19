/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * Color theme support.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef PROTOCOL_THEME_HPP
#define PROTOCOL_THEME_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace protocol {

/** RGBA color. */
struct Color {
    std::uint8_t r = 0, g = 0, b = 0, a = 255;

    /** "#rrggbb" (alpha is ignored; SVG uses separate opacity). */
    std::string to_hex() const;
};

/**
 * A color theme. Field fills are cycled over the protocol fields so
 * neighbouring fields get different colors. If field_texts is empty,
 * `text` is used for every field label.
 */
struct Theme {
    std::string name;
    std::string description;
    Color background;             ///< Canvas background (SVG/PNG)
    Color border;                 ///< Border lines and separators
    Color numbers;                ///< Bit-number rows
    Color text;                   ///< Default field label color
    Color pointer;                ///< Pointer labels and arrows
    std::vector<Color> field_fills; ///< Per-field background fills (cycled)
    bool dark = false;            ///< True if this is a dark theme

    /** Fill color for field i (cycles through field_fills). */
    Color field_fill(int i) const {
        if (field_fills.empty()) return background;
        return field_fills[static_cast<std::size_t>(i) % field_fills.size()];
    }
};

/** Returns all available themes. */
const std::vector<Theme>& available_themes();

/** Finds a theme by name (exact, case-insensitive). Returns nullptr if unknown. */
const Theme* find_theme(const std::string& name);

/** Computes a readable text color (black or white) over the given background. */
Color contrast_text(Color bg);

/**
 * Parses a color given either as a well-known name (case-insensitive, e.g.
 * "white", "tomato", "slategray"; aliases like grey/aqua/fuchsia included)
 * or as hexadecimal "#RGB" / "#RRGGBB". Returns false on unknown input.
 */
bool parse_color(const std::string& s, Color& out);

/** Returns the supported color names (sorted), for --list-colors. */
std::vector<std::string> color_names();

} // namespace protocol

#endif // PROTOCOL_THEME_HPP
