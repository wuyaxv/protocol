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
#include <cstdio>
#include <string>

#include "protocol.hpp"

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
    Protocol p("Type:8,Code:8,Checksum:16");
    p.pointers.push_back(parse_pointer_spec("Sum@checksum")); // case-insensitive
    Grid g = p.layout();
    std::string out = p.to_string();
    CHECK(out.find("Sum ---->") != std::string::npos);
    CHECK(g.left_margin == 3 + 6); // label "Sum" (3) + space + 4 dashes + '>'
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
    CHECK(out.find("Ptr ---->") != std::string::npos);
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
    // Variable-length field edges must not disturb content-row detection;
    // pointing at "Data" lands on the first row where Data starts.
    Protocol p("H:16,Data:32~,T:16");
    p.pointers.push_back(parse_pointer_spec("D@Data"));
    Grid g = p.layout();
    CHECK(g.warnings.empty());
    std::string arrow_line = find_line(p.to_string(), "D ---->");
    CHECK(!arrow_line.empty());
    CHECK(arrow_line.find('H') != std::string::npos); // first content row holds H + Data start
    CHECK(arrow_line.find('~') != std::string::npos); // Data's dashed edge is on it
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
    test_pointer_clamping();
    test_pointer_unknown_field();
    test_pointer_same_row_merge();
    test_pointer_right_alignment();
    test_pointer_spec_option();
    test_pointer_from_spec_with_escape();
    test_pad_grid_left();
    test_content_rows_with_varlen_and_pointer();

    std::printf("unit tests: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
