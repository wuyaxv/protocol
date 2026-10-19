/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * Rewritten in C++ based on the original Python tool by Luis MartinGarcia
 * (https://github.com/luismartingarcia/protocol).
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
#ifndef PROTOCOL_CORE_HPP
#define PROTOCOL_CORE_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace protocol {

/** Exception raised for invalid protocol specifications. */
class ProtocolException : public std::runtime_error {
  public:
    explicit ProtocolException(const std::string& msg) : std::runtime_error(msg) {}
};

/** Semantic role of a single character cell in the rendered diagram. */
enum class Role : std::uint8_t {
    Empty,     ///< Whitespace outside of any field
    NumTen,    ///< Bit-number row (tens)
    NumUnit,   ///< Bit-number row (units)
    Border,    ///< Border characters: '+', '-', '|', '~' (dashed), custom chars
    FieldText, ///< Characters of a field label (field >= 0)
    FieldBg,   ///< Whitespace inside a field box (field >= 0)
    Pointer    ///< Pointer annotation (label text and arrow) left of the diagram
};

/** Border character used for the vertical edges of variable-length fields. */
constexpr char VARLEN_CHAR = '~';

/** One character cell of the diagram grid. */
struct Cell {
    char ch = ' ';
    Role role = Role::Empty;
    int field = -1; ///< Index of the protocol field this cell belongs to (-1 = none)
};

/** A rendered protocol diagram: a grid of annotated character cells. */
struct Grid {
    std::vector<std::vector<Cell>> rows;
    int left_margin = 0;                 ///< Cells reserved on the left (pointer arrows)
    std::vector<std::string> warnings;   ///< Non-fatal issues found during layout

    /** Width of the widest row, in cells. */
    std::size_t width() const {
        std::size_t w = 0;
        for (const auto& r : rows) w = std::max(w, r.size());
        return w;
    }
    std::size_t height() const { return rows.size(); }
};

/** Prepends `extra` empty cells to every row of the grid (for aligning
 *  stacked diagrams that have different pointer margins). */
void pad_grid_left(Grid& grid, int extra);

namespace detail {
/**
 * Splits `s` on `sep` honoring backslash escapes: an escaped separator
 * ("\,") does not split. Escape sequences are kept intact in the tokens;
 * resolve them afterwards with unescape(). Splitting semantics otherwise
 * match Python's str.split (empty input yields one empty element, ...).
 */
std::vector<std::string> split_escaped(const std::string& s, char sep);

/** Resolves backslash escapes: "\x" -> "x" for any x; a trailing lone
 *  backslash is kept literally. */
std::string unescape(const std::string& s);

/** True if the last character of `s` is `c` and is not backslash-escaped. */
bool ends_with_unescaped(const std::string& s, char c);
} // namespace detail

/**
 * A pointer annotation request, e.g. "Pointer@3" (row) or "Pointer@Type"
 * (field name). The target is resolved against the laid-out diagram.
 */
struct Pointer {
    std::string label;  ///< Text written before the arrow (may be empty)
    std::string target; ///< 1-based content-row number or a field name
};

/**
 * Parses a pointer specification of the form "label@target" or
 * "label:target" (split on the last '@' or ':').
 * @throw ProtocolException on malformed input.
 */
Pointer parse_pointer_spec(const std::string& s);

/**
 * Represents a network protocol header. Built from a textual specification,
 * it can be laid out into a Grid (for graphical renderers) or converted to
 * a plain ASCII string (RFC-like diagram).
 */
class Protocol {
  public:
    explicit Protocol(const std::string& spec) { parse_spec(spec); }

    // Diagram appearance options (same semantics as the original tool).
    char hdr_char_start = '+';     ///< Character that starts a border line
    char hdr_char_end = '+';       ///< Character that ends a border line
    char hdr_char_fill_odd = '+';  ///< Fill character for border odd positions
    char hdr_char_fill_even = '-'; ///< Fill character for border even positions
    char hdr_char_sep = '|';       ///< Field separator character
    int bits_per_line = 32;        ///< Number of bits per line
    bool do_print_top_tens = true;  ///< Print bit tens row on top
    bool do_print_top_units = true; ///< Print bit units row on top

    /** Pointer annotations to draw on the left of the diagram. Populated
     *  from the spec's `pointer=` options and/or from the command line. */
    std::vector<Pointer> pointers;

    /**
     * Lays the protocol out into an annotated character grid.
     * @throw ProtocolException if a pointer annotation refers to a
     *        nonexistent field name.
     */
    Grid layout() const;

    /** Converts the protocol to a plain RFC-like ASCII diagram. */
    std::string to_string() const;

  private:
    struct Field {
        std::string text;
        int len = 0;       ///< Length in bits (nominal display length if varlen)
        bool mf = false;   ///< "More Fragments": the field continues on next line
        int id = 0;        ///< Index in the original spec (shared by all fragments)
        bool varlen = false; ///< Variable/undefined length: dashed edges ('~')
    };

    std::vector<Field> field_list_;

    void parse_spec(const std::string& spec);
    std::vector<Field> process_field_list() const;

    /** Resolves and draws pointer annotations (left margin with arrows).
     *  @throw ProtocolException if a pointer refers to an unknown field. */
    void attach_pointers(Grid& grid) const;
};

} // namespace protocol

#endif // PROTOCOL_CORE_HPP
