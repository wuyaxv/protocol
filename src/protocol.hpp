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
    Border,    ///< Border characters: '+', '-', '|', custom chars
    FieldText, ///< Characters of a field label (field >= 0)
    FieldBg    ///< Whitespace inside a field box (field >= 0)
};

/** One character cell of the diagram grid. */
struct Cell {
    char ch = ' ';
    Role role = Role::Empty;
    int field = -1; ///< Index of the protocol field this cell belongs to (-1 = none)
};

/** A rendered protocol diagram: a grid of annotated character cells. */
struct Grid {
    std::vector<std::vector<Cell>> rows;

    /** Width of the widest row, in cells. */
    std::size_t width() const {
        std::size_t w = 0;
        for (const auto& r : rows) w = std::max(w, r.size());
        return w;
    }
    std::size_t height() const { return rows.size(); }
};

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

    /**
     * Lays the protocol out into an annotated character grid.
     * @throw ProtocolException never thrown here; parsing happened at
     *        construction time. The layout itself does not fail.
     */
    Grid layout() const;

    /** Converts the protocol to a plain RFC-like ASCII diagram. */
    std::string to_string() const;

  private:
    struct Field {
        std::string text;
        int len = 0;     ///< Length in bits
        bool mf = false; ///< "More Fragments": the field continues on next line
        int id = 0;      ///< Index in the original spec (shared by all fragments)
    };

    std::vector<Field> field_list_;

    void parse_spec(const std::string& spec);
    std::vector<Field> process_field_list() const;
};

} // namespace protocol

#endif // PROTOCOL_CORE_HPP
