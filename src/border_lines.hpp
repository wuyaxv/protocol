/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * Decides how border characters ('+', '-', '|') are drawn as continuous
 * line primitives by the SVG/PNG renderers, so boxes look properly closed
 * instead of showing per-character glyph gaps.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef PROTOCOL_BORDER_LINES_HPP
#define PROTOCOL_BORDER_LINES_HPP

#include "protocol.hpp"

namespace protocol {

/** Line segments needed to render one border cell. */
struct BorderSeg {
    bool hline = false;      ///< Horizontal line through the cell center
    bool vline_up = false;   ///< Vertical line from cell top to center
    bool vline_down = false; ///< Vertical line from center to cell bottom
    bool dashed = false;     ///< Draw the vertical segments dashed ('~' cells)
    bool any() const { return hline || vline_up || vline_down; }
};

namespace border_detail {
inline bool is_horiz(const Cell& c) {
    if (c.role == Role::Border && (c.ch == '-' || c.ch == '+')) return true;
    // A pointer arrowhead '^' sits inside a horizontal border line; it is
    // drawn as a glyph, but neighbours should keep their horizontal line.
    if (c.role == Role::Pointer && c.ch == '^') return true;
    return false;
}
inline bool is_vert(const Cell& c) {
    return c.role == Role::Border && (c.ch == '|' || c.ch == '+' || c.ch == VARLEN_CHAR);
}
} // namespace border_detail

/**
 * Computes the line segments for one cell. Cells that are not drawable line
 * primitives (plain characters, custom border characters like '*', digits,
 * field labels) return an "empty" segment set and must be rendered as text
 * glyphs instead.
 */
inline BorderSeg border_segments(const std::vector<std::vector<Cell>>& rows, std::size_t r,
                                 std::size_t c) {
    BorderSeg seg;
    const Cell& cell = rows[r][c];
    if (cell.role != Role::Border) return seg;
    switch (cell.ch) {
        case '-':
            seg.hline = true;
            break;
        case '|':
            seg.vline_up = seg.vline_down = true;
            break;
        case VARLEN_CHAR: // '~': dashed vertical edge of a variable-length field
            seg.vline_up = seg.vline_down = true;
            seg.dashed = true;
            break;
        case '+': {
            const std::vector<Cell>& row = rows[r];
            bool left = c > 0 && border_detail::is_horiz(row[c - 1]);
            bool right = c + 1 < row.size() && border_detail::is_horiz(row[c + 1]);
            seg.hline = left || right;
            if (!seg.hline) {
                // Isolated '+': a vertical connector in a merged border line.
                seg.vline_up = seg.vline_down = true;
            } else {
                // Junction: connect to separator rows above/below if present.
                seg.vline_up = r > 0 && c < rows[r - 1].size() &&
                               border_detail::is_vert(rows[r - 1][c]);
                seg.vline_down = r + 1 < rows.size() && c < rows[r + 1].size() &&
                                 border_detail::is_vert(rows[r + 1][c]);
            }
            break;
        }
        default:
            break;
    }
    return seg;
}

} // namespace protocol

#endif // PROTOCOL_BORDER_LINES_HPP
