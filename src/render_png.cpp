/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * PNG renderer. Rasterizes the character grid with an embedded public-domain
 * 8x8 bitmap font and encodes PNG with stb_image_write (public domain).
 * No external library dependencies.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "render_png.hpp"

#include <algorithm>
#include <cstring>

#include "border_lines.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <font8x8_basic.h>

namespace protocol {

namespace {

struct Canvas {
    int w = 0, h = 0;
    std::vector<std::uint8_t> px; // RGBA

    void set(int x, int y, Color c) {
        if (x < 0 || y < 0 || x >= w || y >= h) return;
        std::uint8_t* p = &px[(static_cast<std::size_t>(y) * w + x) * 4];
        p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = c.a;
    }

    void fill_rect(int x, int y, int rw, int rh, Color c) {
        int x1 = std::min(x + rw, w), y1 = std::min(y + rh, h);
        for (int yy = std::max(y, 0); yy < y1; ++yy)
            for (int xx = std::max(x, 0); xx < x1; ++xx)
                set(xx, yy, c);
    }
};

void draw_glyph(Canvas& cv, int x, int y, char ch, int scale, Color color) {
    unsigned char code = static_cast<unsigned char>(ch);
    if (code > 127) code = '?';
    const unsigned char* glyph = font8x8_basic[code];
    for (int gy = 0; gy < 8; ++gy) {
        std::uint8_t bits = static_cast<std::uint8_t>(glyph[gy]);
        if (!bits) continue;
        for (int gx = 0; gx < 8; ++gx) {
            if ((bits >> gx) & 1U) {
                cv.fill_rect(x + gx * scale, y + gy * scale, scale, scale, color);
            }
        }
    }
}

/** Draws a border cell as continuous line segments so that boxes look
 *  properly closed (font glyphs would leave gaps between character rows).
 *  '~' cells (variable-length field edges) are drawn as dashed verticals. */
void draw_border_cell(Canvas& cv, int x, int y, int cell_w, int cell_h, const BorderSeg& seg,
                      Color color) {
    const int t = std::max(1, cell_w / 8); // line thickness in px
    const int cx = x + cell_w / 2;
    const int cy = y + cell_h / 2;
    if (seg.hline) cv.fill_rect(x, cy - t / 2, cell_w, t, color);
    int y0 = seg.vline_up ? y : cy;
    int y1 = seg.vline_down ? y + cell_h : cy;
    if (y1 > y0) {
        if (seg.dashed) {
            const int dash = 2 * std::max(1, cell_w / 8);
            for (int yy = y0; yy < y1; yy += dash * 2)
                cv.fill_rect(cx - t / 2, yy, t, std::min(dash, y1 - yy), color);
        } else {
            cv.fill_rect(cx - t / 2, y0, t, y1 - y0, color);
        }
    }
}

void png_write_to_vector(void* ctx, void* data, int size) {
    auto* out = static_cast<std::vector<std::uint8_t>*>(ctx);
    const std::uint8_t* b = static_cast<const std::uint8_t*>(data);
    out->insert(out->end(), b, b + size);
}

} // namespace

std::vector<std::uint8_t> render_png(const std::vector<Grid>& grids, const Theme& theme,
                                     const PngOptions& opts) {
    const int scale = std::max(1, std::min(8, opts.scale));
    const int cell_w = 8 * scale;
    const int cell_h = 12 * scale; // 8px glyph + vertical padding
    const int glyph_yoff = 2 * scale;
    const int pad_x = cell_w;
    const int pad_y = cell_h / 2;
    const int gap = cell_h;

    std::size_t max_cols = 0, total_rows = 0;
    for (const Grid& g : grids) {
        max_cols = std::max(max_cols, g.width());
        total_rows += g.height();
    }
    if (!grids.empty()) total_rows += grids.size() - 1;

    const int width = static_cast<int>(max_cols) * cell_w + 2 * pad_x;
    const int height = static_cast<int>(total_rows) * cell_h + 2 * pad_y;
    if (width <= 0 || height <= 0) return {};

    Canvas cv;
    cv.w = width;
    cv.h = height;
    cv.px.assign(static_cast<std::size_t>(width) * height * 4, 0);
    if (!opts.transparent)
        cv.fill_rect(0, 0, width, height, theme.background);

    int y_off = pad_y;
    for (const Grid& grid : grids) {
        for (std::size_t r = 0; r < grid.rows.size(); ++r) {
            const auto& row = grid.rows[r];
            // Field backgrounds.
            for (std::size_t col = 0; col < row.size(); ++col) {
                const Cell& c = row[col];
                if (c.role == Role::FieldBg || c.role == Role::FieldText) {
                    cv.fill_rect(pad_x + static_cast<int>(col) * cell_w, y_off, cell_w, cell_h,
                                 theme.field_fill(c.field));
                }
            }
            // Border lines and glyphs.
            for (std::size_t col = 0; col < row.size(); ++col) {
                const Cell& c = row[col];
                if (c.ch == ' ') continue;
                Color color;
                switch (c.role) {
                    case Role::Border: color = theme.border; break;
                    case Role::NumTen:
                    case Role::NumUnit: color = theme.numbers; break;
                    case Role::FieldText: color = theme.text; break;
                    case Role::Pointer: color = theme.pointer; break;
                    default: continue;
                }
                int x = pad_x + static_cast<int>(col) * cell_w;
                BorderSeg seg = border_segments(grid.rows, r, col);
                if (seg.any()) {
                    draw_border_cell(cv, x, y_off, cell_w, cell_h, seg, color);
                } else {
                    draw_glyph(cv, x, y_off + glyph_yoff, c.ch, scale, color);
                }
            }
            y_off += cell_h;
        }
        y_off += gap;
    }

    std::vector<std::uint8_t> out;
    out.reserve(static_cast<std::size_t>(width) * height / 8 + 1024);
    if (!stbi_write_png_to_func(png_write_to_vector, &out, width, height, 4, cv.px.data(),
                                width * 4))
        return {};
    return out;
}

} // namespace protocol
