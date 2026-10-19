/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * PNG renderer. Rasterizes the character grid with a modern TrueType
 * monospace font found on the system (via stb_truetype), falling back to an
 * embedded 8x8 bitmap font. Encodes PNG with stb_image_write (both public
 * domain). No external library dependencies.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "render_png.hpp"

#include <algorithm>
#include <stdexcept>

#include "border_lines.hpp"
#include "font.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace protocol {

namespace {

struct Canvas {
    int w = 0, h = 0;
    std::vector<std::uint8_t> px; // RGBA, non-premultiplied

    void set(int x, int y, Color c) {
        if (x < 0 || y < 0 || x >= w || y >= h) return;
        std::uint8_t* p = &px[(static_cast<std::size_t>(y) * w + x) * 4];
        p[0] = c.r; p[1] = c.g; p[2] = c.b; p[3] = c.a;
    }

    /** Alpha-composites a glyph coverage value (0..255) of color c. */
    void blend(int x, int y, Color c, std::uint8_t coverage) {
        if (x < 0 || y < 0 || x >= w || y >= h || coverage == 0) return;
        std::uint8_t* p = &px[(static_cast<std::size_t>(y) * w + x) * 4];
        float sa = (coverage / 255.0f) * (c.a / 255.0f);
        float da = p[3] / 255.0f;
        float oa = sa + da * (1.0f - sa);
        if (oa <= 0.0f) {
            p[0] = p[1] = p[2] = p[3] = 0;
            return;
        }
        p[0] = static_cast<std::uint8_t>((c.r * sa + p[0] * da * (1.0f - sa)) / oa);
        p[1] = static_cast<std::uint8_t>((c.g * sa + p[1] * da * (1.0f - sa)) / oa);
        p[2] = static_cast<std::uint8_t>((c.b * sa + p[2] * da * (1.0f - sa)) / oa);
        p[3] = static_cast<std::uint8_t>(oa * 255.0f);
    }

    void fill_rect(int x, int y, int rw, int rh, Color c) {
        int x1 = std::min(x + rw, w), y1 = std::min(y + rh, h);
        for (int yy = std::max(y, 0); yy < y1; ++yy)
            for (int xx = std::max(x, 0); xx < x1; ++xx)
                set(xx, yy, c);
    }
};

/** Draws a border cell as continuous line segments so that boxes look
 *  properly closed. '~' cells (variable-length field edges) are dashed. */
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
            const int t2 = std::max(1, cell_w / 8);
            const int dash = 2 * t2;
            const int gap = 3 * t2; // airier gaps between dashes
            for (int yy = y0; yy < y1; yy += dash + gap)
                cv.fill_rect(cx - t / 2, yy, t, std::min(dash, y1 - yy), color);
        } else {
            cv.fill_rect(cx - t / 2, y0, t, y1 - y0, color);
        }
    }
}

void draw_glyph(Canvas& cv, RasterFont& font, int cell_x, int cell_y, char ch, Color color) {
    const GlyphBitmap& gb = font.glyph(ch);
    for (int gy = 0; gy < gb.h; ++gy)
        for (int gx = 0; gx < gb.w; ++gx)
            cv.blend(cell_x + gb.xoff + gx, cell_y + gb.yoff + gy, color,
                     gb.alpha[gy * gb.w + gx]);
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

    RasterFont font;
    if (opts.force_bitmap) {
        font = RasterFont::bitmap(scale);
    } else {
        std::string used_path;
        if (!load_png_font(opts.font_path, scale, font, used_path)) {
            throw std::runtime_error("Cannot load font file '" + opts.font_path + "'");
        }
    }

    const int cell_w = font.cell_w();
    const int cell_h = font.cell_h();
    const int pad_x = cell_w;
    const int pad_y = cell_h / 2;
    const int gap = cell_h; // vertical gap between stacked diagrams

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
            if (grid.fill_fields) {
                for (std::size_t col = 0; col < row.size(); ++col) {
                    const Cell& c = row[col];
                    if (c.role == Role::FieldBg || c.role == Role::FieldText) {
                        cv.fill_rect(pad_x + static_cast<int>(col) * cell_w, y_off, cell_w,
                                     cell_h, theme.field_fill(c.field));
                    }
                }
            }
            // Border lines (pretty mode) and glyphs.
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
                BorderSeg seg;
                if (!opts.ascii) seg = border_segments(grid.rows, r, col);
                if (seg.any()) {
                    draw_border_cell(cv, x, y_off, cell_w, cell_h, seg, color);
                } else {
                    draw_glyph(cv, font, x, y_off, c.ch, color);
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
