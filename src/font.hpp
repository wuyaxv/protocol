/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * Font handling for the PNG renderer: modern TrueType monospace fonts are
 * located on the system (or given explicitly via --font) and rasterized with
 * the embedded stb_truetype; if no usable font is found, an embedded 8x8
 * bitmap font is used as a fallback.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef PROTOCOL_FONT_HPP
#define PROTOCOL_FONT_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace protocol {

/** An 8-bit coverage bitmap for one glyph, in device pixels. */
struct GlyphBitmap {
    int w = 0, h = 0;
    int xoff = 0; ///< X offset from the cell's left edge
    int yoff = 0; ///< Y offset from the cell's top edge
    std::vector<std::uint8_t> alpha; ///< row-major coverage (0..255)
};

/**
 * A monospace font ready to rasterize diagram characters onto a fixed cell
 * grid. Either a TrueType font (smooth, anti-aliased) or the embedded 8x8
 * bitmap font (pixel look). All metrics are in device pixels.
 */
class RasterFont {
  public:
    /** Loads a TrueType font from a file. Returns false on failure. */
    static bool load_ttf(const std::string& path, int scale, RasterFont& out);

    /** Creates a font backed by the embedded 8x8 bitmap font. */
    static RasterFont bitmap(int scale);

    bool is_ttf() const { return is_ttf_; }
    int cell_w() const { return cell_w_; }
    int cell_h() const { return cell_h_; }
    int scale() const { return scale_; }

    /** Coverage bitmap for a character (empty for space). Cached. */
    const GlyphBitmap& glyph(char ch);

  private:
    bool is_ttf_ = false;
    int scale_ = 1;
    int cell_w_ = 0, cell_h_ = 0;
    int baseline_ = 0;

    struct Impl; // stb_truetype state (pimpl, keeps the header clean)
    std::shared_ptr<Impl> impl_;

    std::map<char, GlyphBitmap> cache_;
};

/**
 * Returns candidate paths of monospace fonts present on the system, ordered
 * by preference (modern fonts first: JetBrains Mono, Fira Code, Cascadia,
 * SF Mono, Menlo, Consolas, DejaVu, ...). Only existing files are returned.
 */
std::vector<std::string> find_system_mono_fonts();

/** Loads the best available monospace font: `explicit_path` if given, else
 *  the first usable system font, else the embedded bitmap font.
 *  Returns false only if an explicit path was given but failed to load. */
bool load_png_font(const std::string& explicit_path, int scale, RasterFont& out,
                   std::string& used_path);

} // namespace protocol

#endif // PROTOCOL_FONT_HPP
