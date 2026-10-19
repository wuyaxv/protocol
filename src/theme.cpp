/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * Built-in color themes.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "theme.hpp"

#include <algorithm>
#include <cstdio>

namespace protocol {

std::string Color::to_hex() const {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", r, g, b);
    return buf;
}

Color contrast_text(Color bg) {
    // WCAG relative luminance, simplified.
    double lum = 0.2126 * bg.r + 0.7152 * bg.g + 0.0722 * bg.b;
    return lum > 140.0 ? Color{0x1a, 0x1a, 0x1a, 255} : Color{0xf5, 0xf5, 0xf5, 255};
}

namespace {

Color rgb(int r, int g, int b) {
    return Color{static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g),
                 static_cast<std::uint8_t>(b), 255};
}

std::vector<Theme> build_themes() {
    std::vector<Theme> t;

    // Classic black-on-white, no field coloring (like the plain ASCII output).
    t.push_back(Theme{
        "none", "Plain black on white (classic RFC look, no field colors)",
        rgb(255, 255, 255), rgb(0, 0, 0), rgb(0x55, 0x55, 0x55), rgb(0, 0, 0), rgb(0x00, 0x00, 0x00),
        {rgb(255, 255, 255)}, false});

    // RFC style with subtle gray alternation.
    t.push_back(Theme{
        "rfc", "RFC document style: white background, light gray alternation",
        rgb(255, 255, 255), rgb(0x22, 0x22, 0x22), rgb(0x77, 0x77, 0x77), rgb(0x11, 0x11, 0x11), rgb(0xb0, 0x00, 0x20),
        {rgb(255, 255, 255), rgb(0xf2, 0xf2, 0xf2)}, false});

    // Light pastel theme.
    t.push_back(Theme{
        "light", "Light theme with pastel field colors",
        rgb(0xfa, 0xfa, 0xfa), rgb(0x3a, 0x3a, 0x3a), rgb(0x8a, 0x8a, 0x8a), rgb(0x24, 0x24, 0x24), rgb(0xc0, 0x39, 0x2b),
        {rgb(0xd6, 0xe4, 0xf0), rgb(0xdc, 0xee, 0xd8), rgb(0xfd, 0xf1, 0xd7),
         rgb(0xf6, 0xe0, 0xe9), rgb(0xe5, 0xde, 0xf0), rgb(0xd9, 0xec, 0xec)},
        false});

    // Dark theme (Catppuccin-inspired).
    t.push_back(Theme{
        "dark", "Dark theme with vibrant accents",
        rgb(0x1e, 0x1e, 0x2e), rgb(0xcd, 0xd6, 0xf4), rgb(0x7f, 0x84, 0x9c), rgb(0xf5, 0xf5, 0xf5), rgb(0xf3, 0x8b, 0xa8),
        {rgb(0x45, 0x5a, 0x80), rgb(0x4a, 0x69, 0x56), rgb(0x6b, 0x57, 0x3e),
         rgb(0x6e, 0x48, 0x5e), rgb(0x4f, 0x47, 0x6b), rgb(0x3f, 0x5d, 0x6b)},
        true});

    // Solarized light.
    t.push_back(Theme{
        "solarized", "Solarized light palette",
        rgb(0xfd, 0xf6, 0xe3), rgb(0x65, 0x7b, 0x83), rgb(0x93, 0xa1, 0xa1), rgb(0x07, 0x36, 0x42), rgb(0xdc, 0x32, 0x2f),
        {rgb(0xe9, 0xe2, 0xcb), rgb(0xda, 0xe8, 0xd4), rgb(0xf2, 0xe4, 0xc6),
         rgb(0xee, 0xdd, 0xda), rgb(0xdd, 0xe3, 0xe8), rgb(0xe2, 0xdd, 0xe8)},
        false});

    // Solarized dark.
    t.push_back(Theme{
        "solarized-dark", "Solarized dark palette",
        rgb(0x00, 0x2b, 0x36), rgb(0x83, 0x94, 0x96), rgb(0x58, 0x6e, 0x75), rgb(0xee, 0xe8, 0xd5), rgb(0xdc, 0x32, 0x2f),
        {rgb(0x07, 0x3a, 0x47), rgb(0x0a, 0x40, 0x3e), rgb(0x44, 0x3a, 0x23),
         rgb(0x47, 0x2c, 0x30), rgb(0x32, 0x33, 0x55), rgb(0x24, 0x40, 0x52)},
        true});

    // Monokai.
    t.push_back(Theme{
        "monokai", "Monokai editor palette",
        rgb(0x27, 0x28, 0x22), rgb(0xf8, 0xf8, 0xf2), rgb(0x90, 0x90, 0x80), rgb(0xf8, 0xf8, 0xf2), rgb(0xf9, 0x26, 0x72),
        {rgb(0x52, 0x2e, 0x3d), rgb(0x3d, 0x4a, 0x2a), rgb(0x2a, 0x4a, 0x52),
         rgb(0x52, 0x40, 0x24), rgb(0x42, 0x38, 0x54), rgb(0x46, 0x46, 0x3c)},
        true});

    // Ocean (deep blue).
    t.push_back(Theme{
        "ocean", "Deep ocean blues",
        rgb(0x0f, 0x26, 0x40), rgb(0x9f, 0xc5, 0xe8), rgb(0x5d, 0x7f, 0x9f), rgb(0xea, 0xf2, 0xfb), rgb(0xff, 0x6b, 0x6b),
        {rgb(0x1b, 0x3a, 0x5c), rgb(0x16, 0x45, 0x5c), rgb(0x1c, 0x50, 0x58),
         rgb(0x2a, 0x3f, 0x6e), rgb(0x14, 0x33, 0x4d), rgb(0x23, 0x49, 0x63)},
        true});

    // Forest (greens).
    t.push_back(Theme{
        "forest", "Forest greens",
        rgb(0x16, 0x26, 0x1a), rgb(0xb8, 0xd4, 0xbe), rgb(0x6d, 0x8a, 0x74), rgb(0xec, 0xf5, 0xee), rgb(0xe0, 0xaf, 0x68),
        {rgb(0x22, 0x3d, 0x2a), rgb(0x2a, 0x45, 0x2c), rgb(0x33, 0x44, 0x24),
         rgb(0x1f, 0x3c, 0x38), rgb(0x2c, 0x36, 0x46), rgb(0x3b, 0x45, 0x2a)},
        true});

    // Sunset (warm light theme).
    t.push_back(Theme{
        "sunset", "Warm sunset tones on light background",
        rgb(0xff, 0xf7, 0xef), rgb(0x6b, 0x45, 0x2e), rgb(0xa8, 0x8a, 0x72), rgb(0x3d, 0x28, 0x1c), rgb(0xd1, 0x49, 0x5b),
        {rgb(0xff, 0xe3, 0xc2), rgb(0xff, 0xd1, 0xba), rgb(0xf9, 0xec, 0xc4),
         rgb(0xf8, 0xd5, 0xd8), rgb(0xe8, 0xe0, 0xf4), rgb(0xd4, 0xea, 0xdc)},
        false});

    // Candy (bright, playful).
    t.push_back(Theme{
        "candy", "Bright candy colors",
        rgb(0xff, 0xff, 0xff), rgb(0x4a, 0x4a, 0x68), rgb(0x9a, 0x9a, 0xb8), rgb(0x2d, 0x2d, 0x44), rgb(0xef, 0x47, 0x6f),
        {rgb(0xff, 0xc8, 0xdd), rgb(0xbd, 0xe0, 0xfe), rgb(0xcd, 0xda, 0xfd),
         rgb(0xcf, 0xfb, 0xc0), rgb(0xfc, 0xf6, 0xbd), rgb(0xf1, 0xc0, 0xe8)},
        false});

    // Grayscale.
    t.push_back(Theme{
        "grayscale", "Grayscale alternation, print friendly",
        rgb(255, 255, 255), rgb(0x33, 0x33, 0x33), rgb(0x88, 0x88, 0x88), rgb(0x1a, 0x1a, 0x1a), rgb(0x11, 0x11, 0x11),
        {rgb(0xee, 0xee, 0xee), rgb(0xdc, 0xdc, 0xdc), rgb(0xca, 0xca, 0xca),
         rgb(0xf6, 0xf6, 0xf6)}, false});

    return t;
}

} // namespace

const std::vector<Theme>& available_themes() {
    static const std::vector<Theme> themes = build_themes();
    return themes;
}

const Theme* find_theme(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const Theme& t : available_themes())
        if (t.name == lower) return &t;
    return nullptr;
}

namespace {

/** Well-known color names (standard CSS named colors) with hex values. */
const std::pair<const char*, std::uint32_t> NAMED_COLORS[] = {
    {"aliceblue", 0xf0f8ff}, {"aqua", 0x00ffff},    {"aquamarine", 0x7fffd4},
    {"beige", 0xf5f5dc},     {"black", 0x000000},   {"blue", 0x0000ff},
    {"brown", 0xa52a2a},     {"chartreuse", 0x7fff00}, {"chocolate", 0xd2691e},
    {"coral", 0xff7f50},     {"crimson", 0xdc143c}, {"cyan", 0x00ffff},
    {"darkgray", 0xa9a9a9},  {"deeppink", 0xff1493}, {"dimgray", 0x696969},
    {"dodgerblue", 0x1e90ff}, {"forestgreen", 0x228b22}, {"fuchsia", 0xff00ff},
    {"gainsboro", 0xdcdcdc}, {"ghostwhite", 0xf8f8ff}, {"gold", 0xffd700},
    {"gray", 0x808080},      {"green", 0x008000},   {"grey", 0x808080},
    {"hotpink", 0xff69b4},   {"indigo", 0x4b0082},  {"ivory", 0xfffff0},
    {"khaki", 0xf0e68c},     {"lavender", 0xe6e6fa}, {"lightgray", 0xd3d3d3},
    {"lime", 0x00ff00},      {"magenta", 0xff00ff}, {"maroon", 0x800000},
    {"mintcream", 0xf5fffa}, {"navy", 0x000080},    {"olive", 0x808000},
    {"orange", 0xffa500},    {"pink", 0xffc0cb},    {"purple", 0x800080},
    {"red", 0xff0000},       {"royalblue", 0x4169e1}, {"salmon", 0xfa8072},
    {"seagreen", 0x2e8b57},  {"sienna", 0xa0522d},  {"silver", 0xc0c0c0},
    {"skyblue", 0x87ceeb},   {"slategray", 0x708090}, {"snow", 0xfffafa},
    {"steelblue", 0x4682b4}, {"teal", 0x008080},    {"tomato", 0xff6347},
    {"turquoise", 0x40e0d0}, {"violet", 0xee82ee},  {"white", 0xffffff},
    {"whitesmoke", 0xf5f5f5}, {"yellow", 0xffff00},
};

int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

} // namespace

bool parse_color(const std::string& raw, Color& out) {
    // Trim whitespace.
    std::size_t b = raw.find_first_not_of(" \t\n\r");
    if (b == std::string::npos) return false;
    std::string s = raw.substr(b, raw.find_last_not_of(" \t\n\r") - b + 1);
    if (s.empty()) return false;

    if (s.front() == '#') {
        // Hexadecimal: #RGB or #RRGGBB.
        std::string hex = s.substr(1);
        if (hex.size() == 3) {
            int r = hex_digit(hex[0]), g = hex_digit(hex[1]), bl = hex_digit(hex[2]);
            if (r < 0 || g < 0 || bl < 0) return false;
            out = Color{static_cast<std::uint8_t>(r * 17), static_cast<std::uint8_t>(g * 17),
                        static_cast<std::uint8_t>(bl * 17), 255};
            return true;
        }
        if (hex.size() == 6) {
            int v[6];
            for (int i = 0; i < 6; ++i) {
                v[i] = hex_digit(hex[i]);
                if (v[i] < 0) return false;
            }
            out = Color{static_cast<std::uint8_t>(v[0] * 16 + v[1]),
                        static_cast<std::uint8_t>(v[2] * 16 + v[3]),
                        static_cast<std::uint8_t>(v[4] * 16 + v[5]), 255};
            return true;
        }
        return false;
    }

    // Named color (case-insensitive).
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto& nc : NAMED_COLORS) {
        if (lower == nc.first) {
            out = Color{static_cast<std::uint8_t>((nc.second >> 16) & 0xff),
                        static_cast<std::uint8_t>((nc.second >> 8) & 0xff),
                        static_cast<std::uint8_t>(nc.second & 0xff), 255};
            return true;
        }
    }
    return false;
}

std::vector<std::string> color_names() {
    std::vector<std::string> names;
    for (const auto& nc : NAMED_COLORS) names.push_back(nc.first);
    std::sort(names.begin(), names.end());
    return names;
}

} // namespace protocol
