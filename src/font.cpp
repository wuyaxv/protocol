/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "font.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <cstdlib>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <font8x8_basic.h>

namespace protocol {

struct RasterFont::Impl {
    stbtt_fontinfo info{};
    std::vector<std::uint8_t> data; // font file bytes (stb references them)
    float fscale = 0.0f;
};

namespace fs = std::filesystem;

namespace {

std::string expand_home(const std::string& path) {
    if (!path.empty() && path[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) return std::string(home) + path.substr(1);
    }
    return path;
}

std::string to_lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool has_font_extension(const std::string& name) {
    std::string l = to_lower(name);
    auto ends = [&](const char* ext) {
        return l.size() >= 4 && l.compare(l.size() - 4, 4, ext) == 0;
    };
    return ends(".ttf") || ends(".otf") || ends(".ttc");
}

} // namespace

std::vector<std::string> find_system_mono_fonts() {
    // Monospace font file names, most modern first.
    static const char* preference[] = {
        "jetbrainsmono-regular", "firacode-regular", "firamono-regular",
        "cascadiamono",          "cascadiacode",     "sfnsmono",
        "sfmono",                "menlo",            "consola",
        "dejavusansmono",        "liberationmono",   "ubuntumono",
        "notosansmono",          "notomono",         "sourcecodepro",
        "couriernew",
    };

    // Directories to scan (platform-dependent; nonexistent ones are skipped).
    std::vector<std::string> dirs;
#if defined(_WIN32)
    const char* windir = std::getenv("WINDIR");
    dirs.push_back(windir ? std::string(windir) + "\\Fonts" : "C:\\Windows\\Fonts");
    const char* local = std::getenv("LOCALAPPDATA");
    if (local) dirs.push_back(std::string(local) + "\\Microsoft\\Windows\\Fonts");
#elif defined(__APPLE__)
    dirs = {"/System/Library/Fonts", "/Library/Fonts", "~/Library/Fonts",
            "/System/Library/Fonts/Supplemental"};
#else
    dirs = {"/usr/share/fonts", "/usr/local/share/fonts", "~/.local/share/fonts", "~/.fonts"};
#endif

    // Collect all font files once (shallow recursive scan, depth <= 3).
    std::vector<std::string> files;
    for (const std::string& d0 : dirs) {
        std::string d = expand_home(d0);
        std::error_code ec;
        if (!fs::is_directory(d, ec)) continue;
        fs::recursive_directory_iterator it(
            d, fs::directory_options::skip_permission_denied, ec), end;
        int base_depth = 0;
        (void)base_depth;
        for (; it != end; it.increment(ec)) {
            if (ec) { ec.clear(); continue; }
            if (it.depth() > 3) {
                it.disable_recursion_pending();
                continue;
            }
            if (!it->is_regular_file(ec)) continue;
            std::string name = it->path().filename().string();
            if (has_font_extension(name)) files.push_back(it->path().string());
        }
    }

    // Order candidates by font preference.
    std::vector<std::string> ordered;
    for (const char* pref : preference) {
        for (const std::string& f : files) {
            if (to_lower(fs::path(f).filename().string()).find(pref) != std::string::npos)
                ordered.push_back(f);
        }
    }
    return ordered;
}

bool RasterFont::load_ttf(const std::string& path, int scale, RasterFont& out) {
    std::ifstream f(expand_home(path), std::ios::binary);
    if (!f) return false;
    std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(f)),
                                   std::istreambuf_iterator<char>());
    if (data.size() < 100) return false;

    auto impl = std::make_shared<Impl>();
    impl->data = std::move(data);
    int offset = stbtt_GetFontOffsetForIndex(impl->data.data(), 0);
    if (offset < 0 || !stbtt_InitFont(&impl->info, impl->data.data(), offset))
        return false;

    // Target cell width: 8 * scale px. Monospace advances are ~0.6 em.
    float px = 8.0f * scale / 0.6f;
    impl->fscale = stbtt_ScaleForPixelHeight(&impl->info, px);

    int a = 0, d = 0, g = 0;
    stbtt_GetFontVMetrics(&impl->info, &a, &d, &g);
    int ascent = static_cast<int>(std::ceil(a * impl->fscale));
    int descent = static_cast<int>(std::floor(d * impl->fscale));
    int linegap = static_cast<int>(std::lround(g * impl->fscale));

    int adv = 0, lsb = 0;
    stbtt_GetCodepointHMetrics(&impl->info, '0', &adv, &lsb);
    int cell_w = static_cast<int>(std::ceil(adv * impl->fscale));
    if (cell_w <= 0) return false;

    int line_h = ascent - descent + linegap;
    int cell_h = std::max(line_h, static_cast<int>(std::ceil(px * 1.25f)));

    out.is_ttf_ = true;
    out.scale_ = scale;
    out.cell_w_ = cell_w;
    out.cell_h_ = cell_h;
    out.baseline_ = ascent + (cell_h - line_h) / 2;
    out.impl_ = std::move(impl);
    out.cache_.clear();
    return true;
}

RasterFont RasterFont::bitmap(int scale) {
    RasterFont f;
    f.is_ttf_ = false;
    f.scale_ = scale;
    f.cell_w_ = 8 * scale;
    f.cell_h_ = 12 * scale;
    f.baseline_ = 0;
    return f;
}

const GlyphBitmap& RasterFont::glyph(char ch) {
    auto it = cache_.find(ch);
    if (it != cache_.end()) return it->second;

    GlyphBitmap gb;
    unsigned char code = static_cast<unsigned char>(ch);
    if (code < 32 || code > 126) code = '?';

    if (is_ttf_) {
        int w = 0, h = 0, x0 = 0, y0 = 0;
        unsigned char* bmp = stbtt_GetCodepointBitmap(&impl_->info, 0, impl_->fscale, code,
                                                      &w, &h, &x0, &y0);
        if (bmp) {
            if (w > 0 && h > 0) {
                gb.w = w;
                gb.h = h;
                gb.xoff = x0;
                gb.yoff = baseline_ + y0;
                gb.alpha.assign(bmp, bmp + static_cast<std::size_t>(w) * h);
            }
            stbtt_FreeBitmap(bmp, nullptr);
        }
    } else {
        // Expand the 8x8 bitmap to device pixels (scale^2 blocks).
        const int s = scale_;
        const unsigned char* src = font8x8_basic[code];
        gb.w = 8 * s;
        gb.h = 8 * s;
        gb.xoff = 0;
        gb.yoff = 2 * s; // vertical breathing room inside the cell
        gb.alpha.assign(static_cast<std::size_t>(gb.w) * gb.h, 0);
        for (int gy = 0; gy < 8; ++gy) {
            std::uint8_t bits = src[gy];
            if (!bits) continue;
            for (int gx = 0; gx < 8; ++gx) {
                if ((bits >> gx) & 1U) {
                    for (int dy = 0; dy < s; ++dy)
                        for (int dx = 0; dx < s; ++dx)
                            gb.alpha[(gy * s + dy) * gb.w + gx * s + dx] = 255;
                }
            }
        }
    }
    auto res = cache_.emplace(ch, std::move(gb));
    return res.first->second;
}

bool load_png_font(const std::string& explicit_path, int scale, RasterFont& out,
                   std::string& used_path) {
    used_path.clear();
    if (!explicit_path.empty()) {
        if (RasterFont::load_ttf(explicit_path, scale, out)) {
            used_path = explicit_path;
            return true;
        }
        return false; // explicit font must load
    }
    for (const std::string& path : find_system_mono_fonts()) {
        if (RasterFont::load_ttf(path, scale, out)) {
            used_path = path;
            return true;
        }
    }
    out = RasterFont::bitmap(scale);
    return true;
}

} // namespace protocol
