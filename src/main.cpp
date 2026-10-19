/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * Command-line interface. A C++ rewrite of the original Python tool by
 * Luis MartinGarcia (https://github.com/luismartingarcia/protocol), with
 * added support for color themes and PNG/SVG image generation.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

#include "protocol.hpp"
#include "render_png.hpp"
#include "render_svg.hpp"
#include "render_text.hpp"
#include "specs.hpp"
#include "theme.hpp"

namespace {

const char* APPLICATION_NAME = "Protocol";
const char* APPLICATION_VERSION = "1.1.0";

enum class OutputFormat { Text, Svg, Png };
enum class ColorMode { Auto, Always, Never };

struct Options {
    std::vector<protocol::Protocol> protocols;
    int bits_per_line = -1;          // -1 = not set
    bool skip_numbers = false;
    char hdr_char_start = 0;         // 0 = not set
    char hdr_char_end = 0;
    char hdr_char_fill_odd = 0;
    char hdr_char_fill_even = 0;
    char hdr_char_sep = 0;
    std::string output_file;
    std::string format_str;
    std::string theme_name;
    bool theme_set = false;
    int scale = 2;
    bool transparent = false;
    ColorMode color_mode = ColorMode::Auto;
    std::vector<protocol::Pointer> pointers; // pointer arrows (repeatable)
};

void display_usage(const char* prog) {
    std::printf("Usage: %s {<protocol> or <spec>} [OPTIONS]\n", prog);
}

void display_help(const char* prog) {
    std::printf("\n%s v%s (C++ edition)\n", APPLICATION_NAME, APPLICATION_VERSION);
    std::printf("A header generator for network protocols: RFC-like ASCII diagrams,\n");
    std::printf("SVG vector images and PNG bitmap images, with multiple color themes.\n");
    std::printf("Based on the original Python tool by Luis MartinGarcia.\n");
    std::printf("This software comes with ABSOLUTELY NO WARRANTY.\n\n");
    display_usage(prog);
    std::printf("\nPARAMETERS:\n");
    std::printf(" <protocol>          : Name of an existing protocol (see --list)\n");
    std::printf(" <spec>              : Field by field specification, e.g. \"Type:8,Code:8,Data:16\"\n");
    std::printf("                       Suffix a length with '~' or '*' for a variable-length\n");
    std::printf("                       field (dashed edges); use \\, \\: \\? \\\\ to escape\n");
    std::printf("                       special characters inside field names.\n");
    std::printf("\nOPTIONS:\n");
    std::printf(" -b, --bits <n>      : Number of bits per line\n");
    std::printf(" -f, --file <file>   : Read protocol specs from a text file\n");
    std::printf(" -h, --help          : Displays this help information\n");
    std::printf(" -n, --no-numbers    : Do not print bit numbers on top of the header\n");
    std::printf(" -V, --version       : Displays current version\n");
    std::printf(" -o, --output <file> : Write output to a file (.txt, .svg or .png)\n");
    std::printf("     --format <fmt>  : Output format: text, svg or png (default: text)\n");
    std::printf(" -t, --theme <name>  : Color theme (default: none for text, light for svg/png)\n");
    std::printf("     --color <mode>  : ANSI colors for terminal text output: auto, always, never\n");
    std::printf("     --pointer <l@t> : Draw a pointer arrow: label @ row number or field name\n");
    std::printf("                       (repeatable; rows count field lines starting at 1)\n");
    std::printf("     --scale <n>     : PNG pixel scale, from 1 to 8 (default: 2)\n");
    std::printf("     --transparent   : Transparent canvas background for PNG/SVG output\n");
    std::printf("     --list          : List built-in protocols\n");
    std::printf("     --list-themes   : List available color themes\n");
    std::printf(" --evenchar  <char>  : Character for the even positions of horizontal table borders\n");
    std::printf(" --oddchar   <char>  : Character for the odd positions of horizontal table borders\n");
    std::printf(" --startchar <char>  : Character that starts horizontal table borders\n");
    std::printf(" --endchar   <char>  : Character that ends horizontal table borders\n");
    std::printf(" --sepchar   <char>  : Character that separates protocol fields\n");
    std::printf("\nEXAMPLES:\n");
    std::printf(" %s tcp\n", prog);
    std::printf(" %s ip tcp -o headers.svg --theme dark\n", prog);
    std::printf(" %s \"Type:8,Code:8,Checksum:16?bits=32\" -t ocean\n", prog);
    std::printf(" %s ethernet --format png -o ethernet.png --scale 3 --theme candy\n", prog);
}

void list_protocols() {
    std::printf("Built-in protocols:\n");
    for (const auto& kv : protocol::builtin_protocols())
        std::printf("  %s\n", kv.first.c_str());
}

void list_themes() {
    std::printf("Available color themes:\n");
    for (const protocol::Theme& t : protocol::available_themes())
        std::printf("  %-14s : %s\n", t.name.c_str(), t.description.c_str());
}

[[noreturn]] void fatal(const std::string& msg) {
    std::fprintf(stderr, "ERROR: %s\n", msg.c_str());
    std::exit(1);
}

int parse_config_file(const std::string& filename, Options& opts) {
    std::ifstream f(filename);
    if (!f) {
        std::fprintf(stderr,
                     "Error while reading file %s. Please make sure it exists and it's readable.\n",
                     filename.c_str());
        std::exit(1);
    }
    int count = 0;
    std::string line;
    while (std::getline(f, line)) {
        // Trim whitespace.
        std::size_t b = line.find_first_not_of(" \t\r\n");
        if (b == std::string::npos) continue; // empty line
        std::size_t e = line.find_last_not_of(" \t\r\n");
        line = line.substr(b, e - b + 1);
        if (line.empty() || line.front() == '#') continue;
        try {
            opts.protocols.emplace_back(line);
        } catch (const protocol::ProtocolException& ex) {
            fatal(ex.what());
        }
        ++count;
    }
    return count;
}

/** Resolves a protocol name to its spec (exact or unique-prefix match). */
std::string resolve_protocol_name(const std::string& name) {
    const auto& all = protocol::builtin_protocols();
    auto it = all.find(name);
    if (it != all.end()) return it->second;
    std::vector<std::string> matches;
    for (const auto& kv : all) {
        if (kv.first.compare(0, name.size(), name) == 0) matches.push_back(kv.first);
    }
    if (matches.empty()) {
        std::fprintf(stderr, "ERROR: supplied protocol '%s' does not exist.\n", name.c_str());
        std::exit(1);
    }
    if (matches.size() > 1) {
        std::fprintf(stderr, "Ambiguous protocol specifier '%s'. Did you mean any of these?\n",
                     name.c_str());
        for (const auto& m : matches) std::fprintf(stderr, "  %s\n", m.c_str());
        std::exit(1);
    }
    return all.at(matches.front());
}

void parse_cmd_line_args(int argc, char** argv, Options& opts) {
    if (argc == 1) {
        display_usage(argv[0]);
        std::exit(1);
    }
    bool skip_arg = false;
    for (int i = 1; i < argc; ++i) {
        if (skip_arg) {
            skip_arg = false;
            continue;
        }
        std::string arg = argv[i];
        auto need_param = [&](const char* flag) -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "ERROR: Expected parameter after %s\n", flag);
                display_usage(argv[0]);
                std::exit(1);
            }
            skip_arg = true;
            return argv[i + 1];
        };

        if (arg == "-f" || arg == "--file") {
            const char* file = need_param(arg.c_str());
            if (parse_config_file(file, opts) <= 0)
                fatal(std::string("No protocol specifications found in the supplied file (") +
                      file + ")");
        } else if (arg == "-b" || arg == "--bits") {
            const char* v = need_param(arg.c_str());
            try {
                opts.bits_per_line = std::stoi(v);
            } catch (...) {
                fatal(std::string("Invalid number of bits per line supplied (") + v + ")");
            }
            if (opts.bits_per_line <= 0)
                fatal(std::string("Invalid number of bits per line supplied (") + v + ")");
        } else if (arg == "-n" || arg == "--no-numbers") {
            opts.skip_numbers = true;
        } else if (arg == "-o" || arg == "--output") {
            opts.output_file = need_param(arg.c_str());
        } else if (arg == "--format") {
            opts.format_str = need_param(arg.c_str());
            for (auto& c : opts.format_str) c = static_cast<char>(std::tolower(c));
            if (opts.format_str != "text" && opts.format_str != "svg" && opts.format_str != "png")
                fatal("Invalid output format (" + opts.format_str + "). Use text, svg or png.");
        } else if (arg == "-t" || arg == "--theme") {
            opts.theme_name = need_param(arg.c_str());
            opts.theme_set = true;
        } else if (arg == "--color") {
            std::string mode = need_param(arg.c_str());
            for (auto& c : mode) c = static_cast<char>(std::tolower(c));
            if (mode == "auto") opts.color_mode = ColorMode::Auto;
            else if (mode == "always") opts.color_mode = ColorMode::Always;
            else if (mode == "never") opts.color_mode = ColorMode::Never;
            else fatal("Invalid color mode (" + mode + "). Use auto, always or never.");
        } else if (arg == "--scale") {
            const char* v = need_param(arg.c_str());
            try {
                opts.scale = std::stoi(v);
            } catch (...) {
                fatal(std::string("Invalid PNG scale supplied (") + v + ")");
            }
            if (opts.scale < 1 || opts.scale > 8)
                fatal(std::string("Invalid PNG scale supplied (") + v + "), use a value 1-8");
        } else if (arg == "--transparent") {
            opts.transparent = true;
        } else if (arg == "--pointer") {
            const char* v = need_param(arg.c_str());
            try {
                opts.pointers.push_back(protocol::parse_pointer_spec(v));
            } catch (const protocol::ProtocolException& ex) {
                fatal(ex.what());
            }
        } else if (arg == "--list") {
            list_protocols();
            std::exit(0);
        } else if (arg == "--list-themes") {
            list_themes();
            std::exit(0);
        } else if (arg == "--oddchar" || arg == "--evenchar" || arg == "--startchar" ||
                   arg == "--endchar" || arg == "--sepchar") {
            const char* v = need_param(arg.c_str());
            if (std::strlen(v) != 1) {
                std::fprintf(stderr, "ERROR: A single character is expected after %s\n",
                             arg.c_str());
                display_usage(argv[0]);
                std::exit(1);
            }
            if (arg == "--oddchar") opts.hdr_char_fill_odd = v[0];
            else if (arg == "--evenchar") opts.hdr_char_fill_even = v[0];
            else if (arg == "--startchar") opts.hdr_char_start = v[0];
            else if (arg == "--endchar") opts.hdr_char_end = v[0];
            else opts.hdr_char_sep = v[0];
        } else if (arg == "-h" || arg == "--help") {
            display_help(argv[0]);
            std::exit(0);
        } else if (arg == "-V" || arg == "--version") {
            std::printf("%s v%s\n", APPLICATION_NAME, APPLICATION_VERSION);
            std::exit(0);
        } else if (!arg.empty() && arg.front() == '-') {
            std::fprintf(stderr, "ERROR: Invalid option supplied (%s)\n", arg.c_str());
            std::exit(1);
        } else {
            // Protocol name or protocol spec.
            std::string spec;
            if (arg.find(':') != std::string::npos)
                spec = arg;
            else
                spec = resolve_protocol_name(arg);
            try {
                opts.protocols.emplace_back(spec);
            } catch (const protocol::ProtocolException& ex) {
                fatal(ex.what());
            }
        }
    }
    if (opts.protocols.empty()) fatal("Missing protocol");
}

OutputFormat detect_format(const Options& opts) {
    if (!opts.format_str.empty()) {
        if (opts.format_str == "svg") return OutputFormat::Svg;
        if (opts.format_str == "png") return OutputFormat::Png;
        return OutputFormat::Text;
    }
    if (!opts.output_file.empty()) {
        const std::string& f = opts.output_file;
        auto dot = f.find_last_of('.');
        if (dot != std::string::npos) {
            std::string ext = f.substr(dot + 1);
            for (auto& c : ext) c = static_cast<char>(std::tolower(c));
            if (ext == "svg") return OutputFormat::Svg;
            if (ext == "png") return OutputFormat::Png;
            if (ext == "txt" || ext == "text" || ext == "ascii") return OutputFormat::Text;
        }
    }
    return OutputFormat::Text;
}

bool write_bytes(const std::string& path, const std::vector<std::uint8_t>& data) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(f);
}

bool write_text(const std::string& path, const std::string& data) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(data.data(), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(f);
}

} // namespace

int main(int argc, char** argv) {
    Options opts;
    parse_cmd_line_args(argc, argv, opts);

    // Apply global overrides to every protocol (same semantics as the
    // original tool), plus any pointer annotations.
    for (auto& p : opts.protocols) {
        if (opts.bits_per_line > 0) p.bits_per_line = opts.bits_per_line;
        if (opts.skip_numbers) {
            p.do_print_top_tens = false;
            p.do_print_top_units = false;
        }
        if (opts.hdr_char_start) p.hdr_char_start = opts.hdr_char_start;
        if (opts.hdr_char_end) p.hdr_char_end = opts.hdr_char_end;
        if (opts.hdr_char_fill_even) p.hdr_char_fill_even = opts.hdr_char_fill_even;
        if (opts.hdr_char_fill_odd) p.hdr_char_fill_odd = opts.hdr_char_fill_odd;
        if (opts.hdr_char_sep) p.hdr_char_sep = opts.hdr_char_sep;
        for (const auto& ptr : opts.pointers) p.pointers.push_back(ptr);
    }

    const OutputFormat format = detect_format(opts);

    // Resolve the theme: default is "none" for text and "light" for images.
    std::string theme_name = opts.theme_set ? opts.theme_name
                                            : (format == OutputFormat::Text ? "none" : "light");
    const protocol::Theme* theme = protocol::find_theme(theme_name);
    if (!theme) {
        std::fprintf(stderr, "ERROR: Unknown theme '%s'. Available themes:\n",
                     theme_name.c_str());
        for (const protocol::Theme& t : protocol::available_themes())
            std::fprintf(stderr, "  %s\n", t.name.c_str());
        return 1;
    }

    // Lay out every protocol (this resolves pointer annotations; out-of-range
    // row numbers are clamped with a warning, unknown field names are fatal).
    std::vector<protocol::Grid> grids;
    grids.reserve(opts.protocols.size());
    try {
        for (const auto& p : opts.protocols) grids.push_back(p.layout());
    } catch (const protocol::ProtocolException& ex) {
        std::fprintf(stderr, "ERROR: %s\n", ex.what());
        return 1;
    }
    for (const auto& g : grids)
        for (const std::string& w : g.warnings) std::fprintf(stderr, "warning: %s\n", w.c_str());

    // Align stacked diagrams that have different pointer margins.
    int max_margin = 0;
    for (const auto& g : grids) max_margin = std::max(max_margin, g.left_margin);
    for (auto& g : grids)
        if (g.left_margin < max_margin) protocol::pad_grid_left(g, max_margin - g.left_margin);

    if (format == OutputFormat::Text) {
        bool use_color = theme->name != "none" &&
                         (opts.color_mode == ColorMode::Always ||
                          (opts.color_mode == ColorMode::Auto && opts.output_file.empty() &&
                           isatty(fileno(stdout))));
        std::string out;
        for (std::size_t i = 0; i < grids.size(); ++i) {
            out += use_color ? protocol::render_text_ansi(grids[i], *theme)
                             : protocol::render_text(grids[i]);
            out += '\n';
            if (grids.size() > 1 && i + 1 < grids.size()) out += '\n';
        }
        if (!opts.output_file.empty()) {
            if (!write_text(opts.output_file, out)) {
                std::fprintf(stderr, "ERROR: Cannot write output file %s\n",
                             opts.output_file.c_str());
                return 1;
            }
        } else {
            std::fputs(out.c_str(), stdout);
        }
        return 0;
    }

    // Image formats: stack all protocol diagrams in one canvas.

    if (format == OutputFormat::Svg) {
        protocol::SvgOptions svg_opts;
        svg_opts.transparent = opts.transparent;
        std::string svg = protocol::render_svg(grids, *theme, svg_opts);
        if (!opts.output_file.empty()) {
            if (!write_text(opts.output_file, svg)) {
                std::fprintf(stderr, "ERROR: Cannot write output file %s\n",
                             opts.output_file.c_str());
                return 1;
            }
        } else {
            std::fputs(svg.c_str(), stdout);
        }
        return 0;
    }

    // PNG
    protocol::PngOptions png_opts;
    png_opts.scale = opts.scale;
    png_opts.transparent = opts.transparent;
    std::vector<std::uint8_t> png = protocol::render_png(grids, *theme, png_opts);
    if (png.empty()) {
        std::fprintf(stderr, "ERROR: Failed to encode PNG image\n");
        return 1;
    }
    if (!opts.output_file.empty()) {
        if (!write_bytes(opts.output_file, png)) {
            std::fprintf(stderr, "ERROR: Cannot write output file %s\n", opts.output_file.c_str());
            return 1;
        }
    } else {
        std::fwrite(png.data(), 1, png.size(), stdout);
    }
    return 0;
}
