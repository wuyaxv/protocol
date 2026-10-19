/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * Core layout engine. This is a faithful port of the layout algorithm of the
 * original Python implementation by Luis MartinGarcia, extended so that every
 * character cell carries a semantic Role (border, number, field label, field
 * background, ...) which the SVG/PNG/ANSI renderers use for coloring.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#include "protocol.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>

namespace protocol {

namespace {

/** Trims whitespace at both ends. */
std::string trim(const std::string& s) {
    std::size_t b = s.find_first_not_of(" \t\n\r\f\v");
    if (b == std::string::npos) return "";
    std::size_t e = s.find_last_not_of(" \t\n\r\f\v");
    return s.substr(b, e - b + 1);
}

/** Returns the position of the first unescaped occurrence of `target`
 *  at or after `start`, or npos. A character preceded by an odd number of
 *  backslashes is escaped and skipped. */
std::size_t find_unescaped(const std::string& s, char target, std::size_t start = 0) {
    for (std::size_t i = start; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i; // skip the escaped character
            continue;
        }
        if (s[i] == target) return i;
    }
    return std::string::npos;
}

/** Returns the position of the last unescaped occurrence of `target`. */
std::size_t find_last_unescaped(const std::string& s, char target) {
    std::size_t found = std::string::npos;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
            continue;
        }
        if (s[i] == target) found = i;
    }
    return found;
}

std::string to_lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

/** Parses an integer with the same acceptance rules as Python's int():
 *  optional surrounding whitespace, optional sign, one or more digits. */
int py_int(const std::string& raw, bool& ok) {
    ok = false;
    std::size_t b = raw.find_first_not_of(" \t\n\r\f\v");
    if (b == std::string::npos) return 0;
    std::size_t e = raw.find_last_not_of(" \t\n\r\f\v");
    std::string s = raw.substr(b, e - b + 1);
    std::size_t i = 0;
    bool neg = false;
    if (i < s.size() && (s[i] == '+' || s[i] == '-')) {
        neg = (s[i] == '-');
        ++i;
    }
    if (i >= s.size()) return 0;
    long long v = 0;
    for (; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return 0;
        v = v * 10 + (s[i] - '0');
        if (v > 1000000000LL) return 0; // avoid overflow; diagrams never need this
    }
    ok = true;
    return static_cast<int>(neg ? -v : v);
}

/** Centers a string exactly like Python's str.center(width) with space fill. */
std::string py_center(const std::string& s, std::size_t width) {
    if (s.size() >= width) return s;
    std::size_t marg = width - s.size();
    std::size_t left = marg / 2 + ((marg & width & 1U) ? 1U : 0U);
    std::size_t right = marg - left;
    return std::string(left, ' ') + s + std::string(right, ' ');
}

void push_char(std::vector<Cell>& row, char ch, Role role, int field) {
    row.push_back(Cell{ch, role, field});
}

void push_spaces(std::vector<Cell>& row, long count, Role role, int field) {
    for (long i = 0; i < count; ++i) row.push_back(Cell{' ', role, field});
}

/** Appends a centered field label to a row. The label characters are tagged
 *  FieldText; the padding whitespace is tagged FieldBg (both for field p). */
void push_centered_field(std::vector<Cell>& row, const std::string& text, long width, int p) {
    std::string centered = py_center(text, static_cast<std::size_t>(std::max(0L, width)));
    for (char c : centered) {
        // Whitespace introduced by centering is field background; real label
        // characters are field text. (Labels never contain leading/trailing
        // spaces in practice; borders are handled separately anyway.)
        Role r = (c == ' ') ? Role::FieldBg : Role::FieldText;
        row.push_back(Cell{c, r, p});
    }
}

} // namespace

namespace detail {

std::vector<std::string> split_escaped(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (std::size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '\\' && i + 1 < s.size()) {
            cur += c;
            cur += s[++i]; // keep the escape sequence; unescape later
        } else if (c == sep) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    out.push_back(cur);
    return out;
}

std::string unescape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            out += s[++i];
        } else {
            out += s[i]; // includes a trailing lone backslash, kept literally
        }
    }
    return out;
}

bool ends_with_unescaped(const std::string& s, char c) {
    if (s.empty() || s.back() != c) return false;
    std::size_t backslashes = 0;
    for (std::size_t i = s.size() - 1; i > 0 && s[i - 1] == '\\'; --i) ++backslashes;
    return (backslashes % 2) == 0;
}

} // namespace detail

void pad_grid_left(Grid& grid, int extra) {
    if (extra <= 0) return;
    for (auto& row : grid.rows)
        row.insert(row.begin(), static_cast<std::size_t>(extra), Cell{' ', Role::Empty, -1});
    grid.left_margin += extra;
}

Pointer parse_pointer_spec(const std::string& s) {
    std::size_t pos = find_last_unescaped(s, '@');
    if (pos == std::string::npos) pos = find_last_unescaped(s, ':');
    if (pos == std::string::npos)
        throw ProtocolException("FATAL: Invalid pointer specification (" + s +
                                "), expected \"label@row\" or \"label@field\".");
    Pointer p;
    p.label = detail::unescape(trim(s.substr(0, pos)));
    p.target = detail::unescape(trim(s.substr(pos + 1)));
    if (p.target.empty())
        throw ProtocolException("FATAL: Invalid pointer specification (" + s +
                                "), missing target after the separator.");
    return p;
}

void Protocol::parse_spec(const std::string& spec) {
    std::string fields_part = spec;
    std::string opts_part;
    bool has_opts = false;

    // The '?' separating fields from options must not be escaped ("\?" is a
    // literal question mark inside a field name).
    std::size_t qpos = find_unescaped(spec, '?');
    if (qpos != std::string::npos) {
        if (find_unescaped(spec, '?', qpos + 1) != std::string::npos)
            throw ProtocolException("FATAL: Character '?' may only be used as an option separator.");
        fields_part = spec.substr(0, qpos);
        opts_part = spec.substr(qpos + 1);
        has_opts = true;
    }

    // Parse field spec (escape-aware: "\,", "\:" etc. do not split).
    for (const std::string& item : detail::split_escaped(fields_part, ',')) {
        std::vector<std::string> parts = detail::split_escaped(item, ':');
        if (parts.size() != 2)
            throw ProtocolException("FATAL: Invalid field_list specification (" + spec + ")");
        std::string name = detail::unescape(parts[0]);
        // A trailing '~' or '*' marks a variable-length (undefined length)
        // field; the number is its nominal display length.
        std::string len_raw = parts[1];
        bool varlen = false;
        if (detail::ends_with_unescaped(len_raw, '~') ||
            detail::ends_with_unescaped(len_raw, '*')) {
            varlen = true;
            len_raw.pop_back();
        }
        bool ok = false;
        int bits = py_int(detail::unescape(len_raw), ok);
        if (!ok)
            throw ProtocolException("FATAL: Invalid field_list specification (" + spec + ")");
        if (bits <= 0)
            throw ProtocolException("FATAL: Fields must be at least one bit long (" + spec + ")");
        field_list_.push_back(
            Field{name, bits, false, static_cast<int>(field_list_.size()), varlen});
    }

    // Parse options
    if (has_opts) {
        for (const std::string& opt : detail::split_escaped(opts_part, ',')) {
            std::vector<std::string> kv = detail::split_escaped(opt, '=');
            if (kv.size() != 2)
                throw ProtocolException("FATAL: Invalid options specification (" + opt + ")");
            std::string var = to_lower(detail::unescape(kv[0]));
            const std::string& raw_value = kv[1];
            std::string value = detail::unescape(raw_value);
            if (var == "bits") {
                bool ok = false;
                int b = py_int(value, ok);
                if (!ok)
                    throw ProtocolException("FATAL: Invalid options specification (" + opt + ")");
                if (b <= 0)
                    throw ProtocolException("FATAL: Invalid value for 'bits' option (" + value + ")");
                bits_per_line = b;
            } else if (var == "numbers") {
                std::string v = to_lower(value);
                if (v == "0" || v == "n" || v == "no" || v == "none" || v == "false") {
                    do_print_top_tens = false;
                    do_print_top_units = false;
                } else if (v == "1" || v == "y" || v == "yes" || v == "true") {
                    do_print_top_tens = true;
                    do_print_top_units = true;
                } else {
                    throw ProtocolException("FATAL: Invalid value for 'numbers' option (" + value + ")");
                }
            } else if (var == "oddchar" || var == "evenchar" || var == "startchar" ||
                       var == "endchar" || var == "sepchar") {
                if (value.size() != 1)
                    throw ProtocolException("FATAL: Invalid value for '" + var + "' option (" + value + ")");
                if (var == "oddchar") hdr_char_fill_odd = value[0];
                else if (var == "evenchar") hdr_char_fill_even = value[0];
                else if (var == "startchar") hdr_char_start = value[0];
                else if (var == "endchar") hdr_char_end = value[0];
                else hdr_char_sep = value[0];
            } else if (var == "pointer") {
                // Pass the raw (still escaped) value; parse_pointer_spec
                // resolves escapes itself after splitting label and target.
                pointers.push_back(parse_pointer_spec(raw_value));
            }
            // Unknown options are silently ignored (same as the original tool).
        }
    }
}

std::vector<Protocol::Field> Protocol::process_field_list() const {
    std::vector<Field> fields = field_list_; // work on a copy
    std::vector<Field> new_fields;
    int bits_in_line = 0;
    std::size_t i = 0;
    while (i < fields.size()) {
        Field& field = fields[i];
        std::string field_text = field.text;
        int field_len = field.len;
        field.mf = false;

        int available_in_line = bits_per_line - bits_in_line;

        // If we have enough space on this line for the field, keep it as is.
        if (available_in_line >= field_len) {
            new_fields.push_back(field);
            bits_in_line += field_len;
            ++i;
            if (bits_in_line == bits_per_line) bits_in_line = 0;
        } else {
            // Case 1: perfectly aligned field whose length is a multiple of
            // the line length.
            if (bits_in_line == 0 && field_len % bits_per_line == 0) {
                new_fields.push_back(field);
                ++i;
                bits_in_line = 0;
            } else {
                // Case 2: split the field into two fragments.
                if (available_in_line >= field_len - available_in_line) {
                    new_fields.push_back(Field{field_text, available_in_line, true, field.id, field.varlen});
                    field.text = "";
                    field.len = field_len - available_in_line;
                    field.mf = false;
                } else {
                    new_fields.push_back(Field{"", available_in_line, true, field.id, field.varlen});
                    field.text = field_text;
                    field.len = field_len - available_in_line;
                    field.mf = false;
                }
                bits_in_line = 0;
                continue;
            }
        }
    }
    return new_fields;
}

Grid Protocol::layout() const {
    std::vector<Field> proto_fields = process_field_list();
    Grid grid;

    // Top bit-number rows.
    if (do_print_top_tens) {
        std::vector<Cell> row;
        for (int i = 0; i < bits_per_line; ++i) {
            std::string s = std::to_string(i);
            push_char(row, ' ', Role::NumTen, -1);
            push_char(row, (i % 10 == 0) ? s.front() : ' ', Role::NumTen, -1);
        }
        grid.rows.push_back(std::move(row));
    }
    if (do_print_top_units) {
        std::vector<Cell> row;
        for (int i = 0; i < bits_per_line; ++i) {
            std::string s = std::to_string(i);
            push_char(row, ' ', Role::NumUnit, -1);
            push_char(row, s.back(), Role::NumUnit, -1);
        }
        grid.rows.push_back(std::move(row));
    }

    // Helper that produces the horizontal border line covering `width` bits.
    auto horizontal = [&](int width) {
        std::vector<Cell> row;
        if (width <= 0) return row;
        push_char(row, hdr_char_start, Role::Border, -1);
        for (int i = 0; i < width - 1; ++i) {
            push_char(row, hdr_char_fill_even, Role::Border, -1);
            push_char(row, hdr_char_fill_odd, Role::Border, -1);
        }
        push_char(row, hdr_char_fill_even, Role::Border, -1);
        push_char(row, hdr_char_end, Role::Border, -1);
        return row;
    };

    grid.rows.push_back(horizontal(bits_per_line));

    // Print all protocol fields.
    int bits_in_line = 0;
    std::vector<Cell> current_line;
    std::size_t fields_done = 0;
    for (std::size_t p = 0; p < proto_fields.size(); ++p) {
        std::string field_text = proto_fields[p].text;
        int field_len = proto_fields[p].len;
        bool field_mf = proto_fields[p].mf;
        int field_id = proto_fields[p].id;

        // Truncate the label if it is too long, ending it with a dot.
        if (static_cast<long>(field_text.size()) > static_cast<long>(field_len) * 2 - 1) {
            field_text = field_text.substr(0, static_cast<std::size_t>(field_len * 2 - 1));
            if (field_text.size() > 1)
                field_text = field_text.substr(0, field_text.size() - 1) + ".";
        }

        if (bits_per_line - bits_in_line >= field_len) {
            // The whole field fits in the current line.
            if (bits_in_line == 0) push_char(current_line, hdr_char_sep, Role::Border, -1);

            push_centered_field(current_line, field_text, field_len * 2L - 1, field_id);

            bits_in_line += field_len;
            ++fields_done;

            if (bits_in_line == bits_per_line) {
                push_char(current_line, hdr_char_sep, Role::Border, -1);
                grid.rows.push_back(current_line);
                current_line.clear();
                bits_in_line = 0;
                // When a field is fragmented we may need to suppress the
                // floor of the field so the current line connects with the
                // one that follows.
                if (field_mf && p + 1 < proto_fields.size() &&
                    proto_fields[p + 1].len > bits_per_line - field_len) {
                    std::vector<Cell> merged;
                    // Border covering the previous fields on this line.
                    std::vector<Cell> left = horizontal(bits_per_line - field_len);
                    if (left.empty()) push_char(merged, hdr_char_start, Role::Border, -1);
                    else merged = std::move(left);

                    if (proto_fields[p + 1].len >= bits_per_line) {
                        // The next field reaches the end of its line.
                        push_spaces(merged, 2L * field_len - 1, Role::FieldBg, proto_fields[p + 1].id);
                        push_char(merged, hdr_char_end, Role::Border, -1);
                    } else {
                        // The next field only covers part of the space.
                        long inner = 2L * (proto_fields[p + 1].len - (bits_per_line - field_len)) - 1;
                        push_spaces(merged, inner, Role::FieldBg, proto_fields[p + 1].id);
                        std::vector<Cell> right = horizontal(bits_per_line - proto_fields[p + 1].len);
                        for (const Cell& c : right) merged.push_back(c);
                    }
                    grid.rows.push_back(std::move(merged));
                } else {
                    grid.rows.push_back(horizontal(bits_per_line));
                }
            } else if (fields_done == proto_fields.size()) {
                push_char(current_line, hdr_char_sep, Role::Border, -1);
                grid.rows.push_back(current_line);
                grid.rows.push_back(horizontal(bits_in_line));
            } else {
                push_char(current_line, hdr_char_sep, Role::Border, -1);
            }
        } else {
            // The field does not fit in the current line. We must be at the
            // beginning of a fresh line with a length that is a multiple of
            // the number of bits per line (process_field_list guarantees it).
            if (bits_in_line == 0 && bits_per_line > 0 && field_len % bits_per_line == 0) {
                int lines_to_print = (field_len / bits_per_line) * 2 - 1;
                int central_line = lines_to_print / 2;
                for (int i = 0; i < lines_to_print; ++i) {
                    char start_line = (i % 2 == 1) ? hdr_char_start : hdr_char_sep;
                    char end_line = (i % 2 == 1) ? hdr_char_end : hdr_char_sep;
                    std::vector<Cell> row;
                    push_char(row, start_line, Role::Border, -1);
                    if (i == central_line)
                        push_centered_field(row, field_text, bits_per_line * 2L - 1, field_id);
                    else
                        push_spaces(row, bits_per_line * 2L - 1, Role::FieldBg, field_id);
                    push_char(row, end_line, Role::Border, -1);
                    grid.rows.push_back(std::move(row));
                    if (i == lines_to_print - 1)
                        grid.rows.push_back(horizontal(bits_per_line));
                }
            }
            // Any other case is impossible after process_field_list().
        }
    }

    // ---- Variable-length fields: dashed edges -------------------------
    // Replace every border cell that is horizontally adjacent to a
    // variable-length field's interior with VARLEN_CHAR ('~'), producing
    // RFC-style dashed box sides. This uniform post-pass covers field
    // lines, multi-line big fields and merged border lines alike.
    {
        std::set<int> varlen_ids;
        for (const Field& f : proto_fields)
            if (f.varlen) varlen_ids.insert(f.id);
        if (!varlen_ids.empty()) {
            for (auto& row : grid.rows) {
                for (std::size_t i = 0; i < row.size(); ++i) {
                    const Cell& c = row[i];
                    if ((c.role == Role::FieldBg || c.role == Role::FieldText) &&
                        c.field >= 0 && varlen_ids.count(c.field)) {
                        if (i > 0 && row[i - 1].role == Role::Border)
                            row[i - 1].ch = VARLEN_CHAR;
                        if (i + 1 < row.size() && row[i + 1].role == Role::Border)
                            row[i + 1].ch = VARLEN_CHAR;
                    }
                }
            }
        }
    }

    // ---- Pointer annotations -------------------------------------------
    if (!pointers.empty()) attach_pointers(grid);

    return grid;
}

void Protocol::attach_pointers(Grid& grid) const {
    // Content rows are the diagram lines that display field interiors (this
    // covers normal field lines, multi-line big-field interiors and merged
    // continuation lines). Row numbers seen by users count these, 1-based.
    std::vector<std::size_t> content_rows;
    std::map<int, std::size_t> field_first_row;
    for (std::size_t r = 0; r < grid.rows.size(); ++r) {
        bool has_field = false;
        for (const Cell& c : grid.rows[r]) {
            if (c.field >= 0 && (c.role == Role::FieldBg || c.role == Role::FieldText)) {
                has_field = true;
                auto it = field_first_row.find(c.field);
                if (it == field_first_row.end() || r < it->second)
                    field_first_row[c.field] = r;
            }
        }
        if (has_field) content_rows.push_back(r);
    }
    if (content_rows.empty()) return; // no place to point at

    // Resolve every pointer target to a grid row.
    std::vector<std::pair<std::size_t, std::string>> by_row; // row -> label
    for (const Pointer& p : pointers) {
        std::size_t row = 0;
        bool is_int = false;
        int rownum = py_int(p.target, is_int);
        if (is_int) {
            // Smart placement: clamp out-of-range row numbers instead of
            // failing, and tell the user about it on stderr (via warnings).
            int n = static_cast<int>(content_rows.size());
            int clamped = std::max(1, std::min(n, rownum));
            if (clamped != rownum) {
                grid.warnings.push_back("pointer '" +
                                        (p.label.empty() ? p.target : p.label) + "': row " +
                                        std::to_string(rownum) + " out of range (1-" +
                                        std::to_string(n) + "), clamped to " +
                                        std::to_string(clamped));
            }
            row = content_rows[static_cast<std::size_t>(clamped) - 1];
        } else {
            // Field name target (case-insensitive).
            std::string want = to_lower(trim(p.target));
            int fid = -1;
            for (const Field& f : field_list_) {
                if (to_lower(f.text) == want) {
                    fid = f.id;
                    break;
                }
            }
            auto it = field_first_row.find(fid);
            if (fid < 0 || it == field_first_row.end()) {
                std::string avail;
                for (const Field& f : field_list_) {
                    if (!avail.empty()) avail += ", ";
                    avail += "'" + f.text + "'";
                }
                throw ProtocolException("FATAL: pointer refers to unknown field '" + p.target +
                                        "' (available fields: " + avail + ")");
            }
            row = it->second;
        }
        // Several pointers may share a row: merge their labels.
        auto existing = std::find_if(by_row.begin(), by_row.end(),
                                     [&](const auto& e) { return e.first == row; });
        if (existing == by_row.end())
            by_row.push_back({row, p.label});
        else if (existing->second.empty())
            existing->second = p.label;
        else if (!p.label.empty())
            existing->second += ", " + p.label;
    }

    // Build the left margin: right-aligned label + " " + 4-dash shaft + '>'.
    std::size_t max_label = 0;
    for (const auto& e : by_row) max_label = std::max(max_label, e.second.size());
    const int margin = static_cast<int>(max_label + 6);
    for (std::size_t r = 0; r < grid.rows.size(); ++r) {
        std::vector<Cell> prefix;
        const std::string* lbl = nullptr;
        for (const auto& e : by_row)
            if (e.first == r) lbl = &e.second;
        if (lbl != nullptr) {
            for (std::size_t i = lbl->size(); i < max_label; ++i)
                prefix.push_back(Cell{' ', Role::Empty, -1});
            for (char c : *lbl) prefix.push_back(Cell{c, Role::Pointer, -1});
            prefix.push_back(Cell{' ', Role::Empty, -1});
            for (int i = 0; i < 4; ++i) prefix.push_back(Cell{'-', Role::Pointer, -1});
            prefix.push_back(Cell{'>', Role::Pointer, -1});
        } else {
            for (int i = 0; i < margin; ++i) prefix.push_back(Cell{' ', Role::Empty, -1});
        }
        grid.rows[r].insert(grid.rows[r].begin(), prefix.begin(), prefix.end());
    }
    grid.left_margin = margin;
}

std::string Protocol::to_string() const {
    Grid grid = layout();
    std::string out;
    for (std::size_t r = 0; r < grid.rows.size(); ++r) {
        for (const Cell& c : grid.rows[r]) out += c.ch;
        if (r + 1 < grid.rows.size()) out += '\n';
    }
    return out;
}

} // namespace protocol
