/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "render_text.hpp"

namespace protocol {

std::string render_text(const Grid& grid) {
    std::string out;
    for (std::size_t r = 0; r < grid.rows.size(); ++r) {
        for (const Cell& c : grid.rows[r]) out += c.ch;
        if (r + 1 < grid.rows.size()) out += '\n';
    }
    return out;
}

namespace {

struct Style {
    bool has_fg = false;
    bool has_bg = false;
    Color fg, bg;
    bool operator==(const Style& o) const {
        return has_fg == o.has_fg && has_bg == o.has_bg &&
               (!has_fg || (fg.r == o.fg.r && fg.g == o.fg.g && fg.b == o.fg.b)) &&
               (!has_bg || (bg.r == o.bg.r && bg.g == o.bg.g && bg.b == o.bg.b));
    }
    bool operator!=(const Style& o) const { return !(*this == o); }
};

Style style_for(const Cell& cell, const Theme& theme) {
    Style s;
    switch (cell.role) {
        case Role::Border:
            s.has_fg = true;
            s.fg = theme.border;
            break;
        case Role::NumTen:
        case Role::NumUnit:
            s.has_fg = true;
            s.fg = theme.numbers;
            break;
        case Role::FieldText:
            s.has_fg = true;
            s.fg = theme.text;
            s.has_bg = true;
            s.bg = theme.field_fill(cell.field);
            break;
        case Role::FieldBg:
            s.has_fg = true;
            s.fg = theme.field_fill(cell.field);
            s.has_bg = true;
            s.bg = theme.field_fill(cell.field);
            break;
        case Role::Pointer:
            s.has_fg = true;
            s.fg = theme.pointer;
            break;
        case Role::Empty:
        default:
            break;
    }
    return s;
}

} // namespace

std::string render_text_ansi(const Grid& grid, const Theme& theme) {
    std::string out;
    Style current;
    bool styled = false;
    auto apply = [&](const Style& s) {
        if (s.has_fg) {
            out += "\x1b[38;2;" + std::to_string(s.fg.r) + ";" + std::to_string(s.fg.g) + ";" +
                   std::to_string(s.fg.b) + "m";
        }
        if (s.has_bg) {
            out += "\x1b[48;2;" + std::to_string(s.bg.r) + ";" + std::to_string(s.bg.g) + ";" +
                   std::to_string(s.bg.b) + "m";
        }
    };
    for (std::size_t r = 0; r < grid.rows.size(); ++r) {
        for (const Cell& c : grid.rows[r]) {
            Style s = style_for(c, theme);
            if (s != current) {
                if (styled) out += "\x1b[0m"; // reset before applying a new style
                current = s;
                styled = s.has_fg || s.has_bg;
                if (styled) apply(s);
            }
            out += c.ch;
        }
        if (styled) {
            out += "\x1b[0m";
            current = Style{};
            styled = false;
        }
        if (r + 1 < grid.rows.size()) out += '\n';
    }
    return out;
}

} // namespace protocol
