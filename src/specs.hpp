/*
 * protocol - A simple header generator for network protocols (C++ edition)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef PROTOCOL_SPECS_HPP
#define PROTOCOL_SPECS_HPP

#include <map>
#include <string>

namespace protocol {

/** Returns the table of built-in protocol specifications (name -> spec). */
const std::map<std::string, std::string>& builtin_protocols();

} // namespace protocol

#endif // PROTOCOL_SPECS_HPP
