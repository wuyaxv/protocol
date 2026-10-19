/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * Unit tests for the core library: spec parsing (escapes, variable-length
 * fields, pointer options), the layout engine, pointer resolution and the
 * grid helpers. Framework-free: each CHECK failure prints and counts; the
 * process exits non-zero if anything failed.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include <algorithm>
#include <cstdio>
#include <string>

#include "font.hpp"
#include "protocol.hpp"
#include "render_png.hpp"
#include "render_svg.hpp"
#include "theme.hpp"

using namespace protocol;

namespace {

int g_failures = 0;
int g_checks = 0;

/** Finds the line of `text` containing `needle`; empty string if absent. */
std::string find_line(const std::string& text, const std::string& needle) {
    std::size_t pos = text.find(needle);
    if (pos == std::string::npos) return "";
    std::size_t b = text.rfind('\n', pos);
    std::size_t e = text.find('\n', pos);
    return text.substr(b == std::string::npos ? 0 : b + 1,
                       (e == std::string::npos ? text.size() : e) -
                           (b == std::string::npos ? 0 : b + 1));
}

#define CHECK(cond)                                                                              \
    do {                                                                                         \
        ++g_checks;                                                                              \
        if (!(cond)) {                                                                           \
            ++g_failures;                                                                        \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                          \
        }                                                                                        \
    } while (0)

#define CHECK_THROWS(expr)                                                                       \
    do {                                                                                         \
        ++g_checks;                                                                              \
        bool threw = false;                                                                      \
        try {                                                                                    \
            expr;                                                                                \
        } catch (const ProtocolException&) {                                                     \
            threw = true;                                                                        \
        }                                                                                        \
        if (!threw) {                                                                            \
            ++g_failures;                                                                        \
            std::printf("FAIL %s:%d: expected ProtocolException: %s\n", __FILE__, __LINE__,      \
                        #expr);                                                                  \
        }                                                                                        \
    } while (0)

// ---------------------------------------------------------------- escapes

void test_split_escaped() {
    auto v = detail::split_escaped("a,b,c", ',');
    CHECK(v.size() == 3 && v[0] == "a" && v[2] == "c");

    // Escaped separator does not split.
    v = detail::split_escaped("a\\,b,c", ',');
    CHECK(v.size() == 2 && v[0] == "a\\,b" && v[1] == "c");

    // Python-compatible edge cases still hold.
    v = detail::split_escaped("", ',');
    CHECK(v.size() == 1 && v[0].empty());
    v = detail::split_escaped("a,", ',');
    CHECK(v.size() == 2 && v[1].empty());
    v = detail::split_escaped(",a", ',');
    CHECK(v.size() == 2 && v[0].empty());

    // Escaped backslash before a separator: "\\," IS a split point.
    v = detail::split_escaped("a\\\\,b", ',');
    CHECK(v.size() == 2 && v[0] == "a\\\\" && v[1] == "b");

    // Trailing lone backslash is kept and does not swallow the separator.
    v = detail::split_escaped("a\\,b", ':');
    CHECK(v.size() == 1 && v[0] == "a\\,b");
}

void test_unescape() {
    CHECK(detail::unescape("plain") == "plain");
    CHECK(detail::unescape("\\,") == ",");
    CHECK(detail::unescape("\\:") == ":");
    CHECK(detail::unescape("\\?") == "?");
    CHECK(detail::unescape("\\=") == "=");
    CHECK(detail::unescape("\\\\") == "\\");
    CHECK(detail::unescape("a\\~b") == "a~b");
    CHECK(detail::unescape("trailing\\") == "trailing\\"); // lone backslash kept
    CHECK(detail::unescape("\\x\\y") == "xy");
}

void test_ends_with_unescaped() {
    CHECK(detail::ends_with_unescaped("8~", '~'));
    CHECK(!detail::ends_with_unescaped("8\\~", '~'));
    CHECK(detail::ends_with_unescaped("8\\\\~", '~')); // escaped backslash, then '~'
    CHECK(!detail::ends_with_unescaped("8", '~'));
    CHECK(!detail::ends_with_unescaped("~", '8'));
}

// ------------------------------------------------- spec parsing with escapes

void test_spec_escape_field_names() {
    Protocol p("Seq\\, Nr:8,What\\:ify\\?:8,Code:16");
    std::string out = p.to_string();
    CHECK(out.find("Seq, Nr") != std::string::npos);
    CHECK(out.find("What:ify?") != std::string::npos);
    CHECK(out.find("Code") != std::string::npos);
}

void test_spec_escape_question_mark_only() {
    // An escaped '?' must not be treated as the options separator.
    Protocol p("Question\\?:16,End:16");
    std::string out = p.to_string();
    CHECK(out.find("Question?") != std::string::npos);
    CHECK(p.bits_per_line == 32); // no options were parsed
}

void test_spec_escape_in_options() {
    // Escaped comma inside an option value (separator character).
    Protocol p("A:8,B:8?bits=16,sepchar=\\,");
    std::string out = p.to_string();
    CHECK(out.find(",       A       ,       B       ,") != std::string::npos);
    CHECK(out.find('|') == std::string::npos); // no default separators remain
}

void test_spec_escape_backslash_name() {
    Protocol p("Back\\\\slash:16,End:16");
    CHECK(p.to_string().find("Back\\slash") != std::string::npos);
}

void test_unescaped_still_fails() {
    CHECK_THROWS(Protocol("A,B:8"));   // stray field without length
    CHECK_THROWS(Protocol("A:8?x?y")); // two unescaped option separators
}

// ----------------------------------------------------- variable-length fields

void test_varlen_parsing() {
    Protocol p1("Payload:32~");
    CHECK(p1.to_string().find('~') != std::string::npos);
    Protocol p2("Payload:32*"); // '*' alias
    CHECK(p2.to_string().find('~') != std::string::npos);
    // A field that is not marked keeps solid edges.
    Protocol p3("Payload:32");
    CHECK(p3.to_string().find('~') == std::string::npos);
}

void test_varlen_layout_basic() {
    Protocol p("Type:8,Data:16~,Code:8");
    std::string out = p.to_string();
    // The two separators around Data are dashed; the outer ones are not.
    CHECK(out.find("|      Type     ~              Data             ~      Code     |") !=
          std::string::npos);
    // The horizontal borders stay solid.
    CHECK(out.find("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+") != std::string::npos);
}

void test_varlen_multiline() {
    // 64-bit variable field: all interior lines get '~' edges.
    Protocol p("Big:64~");
    std::string out = p.to_string();
    CHECK(out.find("~                              Big                              ~") !=
          std::string::npos);
    CHECK(out.find('|') == std::string::npos); // no solid verticals at all
}

void test_varlen_errors() {
    CHECK_THROWS(Protocol("A:~"));   // missing nominal length
    CHECK_THROWS(Protocol("A:x~"));  // non-numeric nominal length
    CHECK_THROWS(Protocol("A:0~"));  // zero nominal length
}

// ------------------------------------------------------------ pointer specs

void test_parse_pointer_spec() {
    Pointer p = parse_pointer_spec("Pointer@3");
    CHECK(p.label == "Pointer" && p.target == "3");
    p = parse_pointer_spec("Pointer:3"); // ':' also accepted
    CHECK(p.label == "Pointer" && p.target == "3");
    p = parse_pointer_spec("@2"); // empty label is fine
    CHECK(p.label.empty() && p.target == "2");
    p = parse_pointer_spec(" Seq Nr @ 2 "); // whitespace trimmed
    CHECK(p.label == "Seq Nr" && p.target == "2");
    p = parse_pointer_spec("a@b@4"); // splits on the LAST separator
    CHECK(p.label == "a@b" && p.target == "4");
    p = parse_pointer_spec("L\\,x@2"); // escapes resolved after splitting
    CHECK(p.label == "L,x" && p.target == "2");

    CHECK_THROWS(parse_pointer_spec("noseparator"));
    CHECK_THROWS(parse_pointer_spec("label@"));
}

void test_pointer_by_row() {
    Protocol p("A:16,B:16,C:32");
    p.pointers.push_back(parse_pointer_spec("Ptr@2"));
    std::string out = p.to_string();
    // Row 2 is the content line holding C; the arrow sits on that line.
    std::string arrow_line = find_line(out, "Ptr ---->");
    CHECK(!arrow_line.empty());
    CHECK(arrow_line.find('C') != std::string::npos);
    // The other content line has no arrow but gets the blank margin.
    std::string first_line = find_line(out, "A");
    CHECK(first_line.find("---->") == std::string::npos);
}

void test_pointer_by_field_name() {
    // Field-name targets draw an elbow arrow: a lane row below the field's
    // box, a shaft from the left margin, a '+' corner and a '^' arrowhead in
    // the border under the field's center.
    Protocol p("Type:8,Code:8,Checksum:16");
    p.pointers.push_back(parse_pointer_spec("Sum@checksum")); // case-insensitive
    Grid g = p.layout();
    // Layout: 2 number rows, border, content, border (+ '^'), lane row.
    CHECK(g.rows.size() == 4 + 2);
    CHECK(g.left_margin == 3 + 6); // label "Sum" (3) + space + shaft + head
    // Checksum spans cells 33..63 on the content row; center = 48.
    // The '^' sits on the border row below, shifted by the margin.
    const std::vector<Cell>& border = g.rows[4];
    CHECK(border[g.left_margin + 48].ch == '^' &&
          border[g.left_margin + 48].role == Role::Pointer);
    // The lane row carries the label, the shaft and the corner.
    const std::vector<Cell>& lane = g.rows[5];
    CHECK(lane[0].ch == 'S' && lane[1].ch == 'u' && lane[2].ch == 'm');
    CHECK(lane[g.left_margin + 48].ch == '+' &&
          lane[g.left_margin + 48].role == Role::Pointer);
    CHECK(lane[g.left_margin + 47].ch == '-'); // shaft reaches the corner
}

void test_pointer_elbow_keeps_ascii_layout() {
    // The lane row must not corrupt the diagram: the row above it is a border
    // with exactly one '^', and all rows stay aligned (left_margin padding).
    Protocol p("A:16,B:16,C:32");
    p.pointers.push_back(parse_pointer_spec("P@B"));
    std::string out = p.to_string();
    std::string caret_line = find_line(out, "^");
    CHECK(!caret_line.empty());
    CHECK(caret_line.find('-') != std::string::npos); // it is a border line
    std::string lane = find_line(out, "P ---");
    CHECK(!lane.empty());
    std::size_t last_nonspace = lane.find_last_not_of(' ');
    CHECK(last_nonspace != std::string::npos && lane[last_nonspace] == '+');
}

void test_pointer_elbow_two_fields_same_row() {
    // Two field pointers under the same row get two stacked lane rows.
    Protocol p("A:16,B:16");
    p.pointers.push_back(parse_pointer_spec("PA@A"));
    p.pointers.push_back(parse_pointer_spec("PB@B"));
    Grid g = p.layout();
    std::size_t carets = 0, lanes = 0;
    for (const auto& row : g.rows) {
        for (const Cell& c : row)
            if (c.ch == '^') ++carets;
        if (!row.empty() && row.front().ch == 'P') ++lanes;
    }
    CHECK(carets == 2);
    CHECK(lanes == 2);
}

void test_pointer_elbow_wrapped_field() {
    // Fragmented field: the arrow aims at the row where the label is printed.
    Protocol p("A:24,B:48,C:8");
    p.pointers.push_back(parse_pointer_spec("P@B"));
    Grid g = p.layout();
    CHECK(g.warnings.empty());
    std::string out = p.to_string();
    std::size_t label_pos = out.find("|                               B");
    std::size_t caret_pos = out.find('^');
    CHECK(label_pos != std::string::npos && caret_pos != std::string::npos);
    CHECK(caret_pos > label_pos); // arrowhead below the label row
}

void test_pointer_elbow_last_row() {
    // Pointing at a field on the last content row: the lane row goes at the
    // very bottom of the grid.
    Protocol p("A:32,B:16");
    p.pointers.push_back(parse_pointer_spec("Last@B"));
    Grid g = p.layout();
    const std::vector<Cell>& last = g.rows.back();
    bool has_corner = false;
    for (const Cell& c : last) has_corner = has_corner || (c.ch == '+' && c.role == Role::Pointer);
    CHECK(has_corner);
}

void test_pointer_row_target_unchanged() {
    // Row numbers keep the classic horizontal arrow at the row's start.
    Protocol p("A:32,B:32");
    p.pointers.push_back(parse_pointer_spec("Ptr@2"));
    std::string out = p.to_string();
    CHECK(find_line(out, "Ptr ---->").find('B') != std::string::npos);
    CHECK(out.find('^') == std::string::npos); // no elbow arrows
}

void test_pointer_clamping() {
    Protocol p("A:32,B:32");
    p.pointers.push_back(parse_pointer_spec("End@99"));
    Grid g = p.layout();
    CHECK(g.warnings.size() == 1);
    CHECK(g.warnings[0].find("clamped to 2") != std::string::npos);
    // The arrow ended up on the last content row (the one holding B).
    std::string arrow_line = find_line(p.to_string(), "End ---->");
    CHECK(!arrow_line.empty());
    CHECK(arrow_line.find('B') != std::string::npos);
}

void test_pointer_unknown_field() {
    Protocol p("A:32");
    p.pointers.push_back(parse_pointer_spec("X@Nope"));
    CHECK_THROWS(p.layout());
}

void test_pointer_same_row_merge() {
    Protocol p("A:32,B:32");
    p.pointers.push_back(parse_pointer_spec("First@1"));
    p.pointers.push_back(parse_pointer_spec("Also@1"));
    std::string out = p.to_string();
    CHECK(out.find("First, Also ---->") != std::string::npos);
}

void test_pointer_right_alignment() {
    Protocol p("A:32,B:32");
    p.pointers.push_back(parse_pointer_spec("LongLabel@1"));
    p.pointers.push_back(parse_pointer_spec("S@2"));
    std::string out = p.to_string();
    CHECK(out.find("LongLabel ---->") != std::string::npos);
    CHECK(out.find("        S ---->") != std::string::npos); // right-aligned (8 spaces)
}

void test_pointer_spec_option() {
    Protocol p("Type:8,Data:16,Code:8?pointer=Ptr@Data");
    std::string out = p.to_string();
    // Field-name target from a spec option: elbow arrow (shaft + '^').
    CHECK(out.find('^') != std::string::npos);
    CHECK(!find_line(out, "Ptr ---").empty());
}

void test_pointer_from_spec_with_escape() {
    Protocol p("A:8,B:8?pointer=One\\, Two@1");
    CHECK(p.to_string().find("One, Two ---->") != std::string::npos);
}

// ------------------------------------------------------------- grid helpers

void test_pad_grid_left() {
    Protocol p("A:32");
    Grid g = p.layout();
    std::size_t w0 = g.rows[0].size();
    pad_grid_left(g, 5);
    CHECK(g.left_margin == 5);
    CHECK(g.rows[0].size() == w0 + 5);
    for (const auto& row : g.rows)
        for (int i = 0; i < 5; ++i) CHECK(row[i].role == Role::Empty && row[i].ch == ' ');
}

void test_content_rows_with_varlen_and_pointer() {
    // Variable-length field edges must not disturb elbow routing: pointing at
    // "Data" puts the arrowhead in the border below Data's label row.
    Protocol p("H:16,Data:32~,T:16");
    p.pointers.push_back(parse_pointer_spec("D@Data"));
    Grid g = p.layout();
    CHECK(g.warnings.empty());
    std::string out = p.to_string();
    CHECK(out.find('^') != std::string::npos);
    CHECK(!find_line(out, "D ---").empty());
    std::size_t label_pos = out.find("Data");
    CHECK(out.find('^') > label_pos); // arrowhead below the label
}

// ----------------------------------------------------------- fill option

void test_fill_option() {
    Protocol p1("A:8,B:8"); // default: fills enabled
    CHECK(p1.layout().fill_fields);
    Protocol p2("A:8,B:8?fill=n");
    CHECK(!p2.layout().fill_fields);
    Protocol p3("A:8,B:8?fill=y");
    CHECK(p3.layout().fill_fields);
    Protocol p4("A:8,B:8?fill=0");
    CHECK(!p4.layout().fill_fields);
    CHECK_THROWS(Protocol("A:8?fill=maybe"));

    // Disabling fills must not change the plain ASCII output.
    CHECK(p1.to_string() == p2.to_string());
}

// ------------------------------------------------------------------- fonts

void test_bitmap_font() {
    RasterFont f = RasterFont::bitmap(2);
    CHECK(!f.is_ttf());
    CHECK(f.cell_w() == 16 && f.cell_h() == 24);
    const GlyphBitmap& a = f.glyph('A');
    CHECK(a.w == 16 && a.h == 16 && !a.alpha.empty());
    bool any = false;
    for (std::uint8_t v : a.alpha) any = any || v > 0;
    CHECK(any);
    CHECK(f.glyph(' ').alpha.empty() || true); // space may be empty, must not crash
}

void test_explicit_font_failure() {
    RasterFont f;
    std::string used;
    CHECK(!load_png_font("/nonexistent/font.ttf", 2, f, used));
}

void test_system_font_detection() {
    // On a machine without any mono font this may find nothing; the fallback
    // must still succeed.
    RasterFont f;
    std::string used;
    CHECK(load_png_font("", 2, f, used));
    CHECK(f.cell_w() > 0 && f.cell_h() > 0);
    if (!used.empty()) {
        CHECK(f.is_ttf());
        CHECK(f.glyph('S').h > 0); // real glyphs rasterize
    } else {
        CHECK(!f.is_ttf()); // bitmap fallback
    }
}

// ------------------------------------------------------- svg render options

void test_svg_ascii_and_fill_modes() {
    Protocol p("Type:8,Payload:32~,CRC:16");
    std::vector<Grid> grids{p.layout()};

    Theme theme = *find_theme("light");
    SvgOptions pretty;
    std::string svg_pretty = render_svg(grids, theme, pretty);
    CHECK(svg_pretty.find("stroke-dasharray") != std::string::npos); // '~' dashed

    SvgOptions ascii;
    ascii.ascii = true;
    std::string svg_ascii = render_svg(grids, theme, ascii);
    CHECK(svg_ascii.find("stroke-dasharray") == std::string::npos); // literal '~'
    CHECK(svg_ascii.find("+-+-+-+-+-+-+-+-") != std::string::npos);  // borders as text

    // Field fills disabled: no fill rectangles for the fields.
    Grid g2 = p.layout();
    g2.fill_fields = false;
    std::string svg_nofill = render_svg(std::vector<Grid>{g2}, theme, pretty);
    CHECK(svg_nofill.find(theme.field_fill(0).to_hex()) == std::string::npos);

    // Custom font family is prepended to the stack.
    SvgOptions ff;
    ff.font_family = "Comic Code";
    std::string svg_ff = render_svg(grids, theme, ff);
    CHECK(svg_ff.find("'Comic Code',") != std::string::npos);
}

void test_png_bitmap_render_modes() {
    Protocol p("Type:8,Data:16");
    std::vector<Grid> grids{p.layout()};
    Theme theme = *find_theme("light");
    PngOptions o;
    o.force_bitmap = true;
    std::vector<std::uint8_t> pretty = render_png(grids, theme, o);
    CHECK(pretty.size() > 8);
    CHECK(pretty[0] == 0x89 && pretty[1] == 'P' && pretty[2] == 'N' && pretty[3] == 'G');
    o.ascii = true;
    std::vector<std::uint8_t> ascii = render_png(grids, theme, o);
    CHECK(ascii.size() > 8 && ascii != pretty); // modes differ
}

// ------------------------------------------------------------ color parsing

void test_parse_color_names() {
    Color c;
    CHECK(parse_color("white", c) && c.r == 255 && c.g == 255 && c.b == 255);
    CHECK(parse_color("black", c) && c.r == 0 && c.g == 0 && c.b == 0);
    CHECK(parse_color("White", c));                    // case-insensitive
    CHECK(parse_color("TOMATO", c) && c.r == 0xff && c.g == 0x63 && c.b == 0x47);
    CHECK(parse_color("grey", c) && parse_color("gray", c)); // aliases
    CHECK(parse_color("aqua", c) && parse_color("cyan", c));
    CHECK(parse_color("fuchsia", c) && parse_color("magenta", c));
    CHECK(parse_color("  navy  ", c) && c.b == 0x80);  // whitespace tolerated
}

void test_parse_color_hex() {
    Color c;
    CHECK(parse_color("#ffffff", c) && c.r == 255 && c.g == 255 && c.b == 255);
    CHECK(parse_color("#000000", c) && c.r == 0 && c.g == 0 && c.b == 0);
    CHECK(parse_color("#1a2B3c", c) && c.r == 0x1a && c.g == 0x2b && c.b == 0x3c);
    CHECK(parse_color("#fff", c) && c.r == 255 && c.g == 255 && c.b == 255);
    CHECK(parse_color("#0f0", c) && c.g == 255 && c.r == 0); // short form expands
    CHECK(parse_color("#123", c) && c.r == 0x11 && c.g == 0x22 && c.b == 0x33);

    Color bad;
    CHECK(!parse_color("#ff", bad));       // too short
    CHECK(!parse_color("#ffff", bad));     // 4 digits unsupported
    CHECK(!parse_color("#fffff", bad));    // 5 digits
    CHECK(!parse_color("#gggggg", bad));   // invalid hex digits
    CHECK(!parse_color("#", bad));
}

void test_parse_color_invalid() {
    Color c;
    CHECK(!parse_color("notacolor", c));
    CHECK(!parse_color("", c));
    CHECK(!parse_color("   ", c));
    CHECK(!parse_color("red1", c));
    // Bare hex without '#' is not accepted (ambiguous with names).
    CHECK(!parse_color("ff0000", c));
}

void test_color_names_list() {
    std::vector<std::string> names = color_names();
    CHECK(names.size() >= 48);
    CHECK(std::find(names.begin(), names.end(), "white") != names.end());
    CHECK(std::find(names.begin(), names.end(), "slategray") != names.end());
    CHECK(std::is_sorted(names.begin(), names.end()));
}

} // namespace

int main() {
    test_split_escaped();
    test_unescape();
    test_ends_with_unescaped();
    test_spec_escape_field_names();
    test_spec_escape_question_mark_only();
    test_spec_escape_in_options();
    test_spec_escape_backslash_name();
    test_unescaped_still_fails();
    test_varlen_parsing();
    test_varlen_layout_basic();
    test_varlen_multiline();
    test_varlen_errors();
    test_parse_pointer_spec();
    test_pointer_by_row();
    test_pointer_by_field_name();
    test_pointer_elbow_keeps_ascii_layout();
    test_pointer_elbow_two_fields_same_row();
    test_pointer_elbow_wrapped_field();
    test_pointer_elbow_last_row();
    test_pointer_row_target_unchanged();
    test_pointer_clamping();
    test_pointer_unknown_field();
    test_pointer_same_row_merge();
    test_pointer_right_alignment();
    test_pointer_spec_option();
    test_pointer_from_spec_with_escape();
    test_pad_grid_left();
    test_content_rows_with_varlen_and_pointer();
    test_fill_option();
    test_bitmap_font();
    test_explicit_font_failure();
    test_system_font_detection();
    test_svg_ascii_and_fill_modes();
    test_png_bitmap_render_modes();
    test_parse_color_names();
    test_parse_color_hex();
    test_parse_color_invalid();
    test_color_names_list();

    std::printf("unit tests: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
