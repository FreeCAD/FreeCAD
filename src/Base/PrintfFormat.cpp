// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2026 FreeCAD Project Association                        *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 ***************************************************************************/

#include <cctype>
#include <cstring>
#include <format>
#include <string>

#include "PrintfFormat.h"

namespace
{

bool isDigit(char c)
{
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

// One printf conversion specification, e.g. "%-8.3f".
struct Spec
{
    bool leftAlign = false;
    bool plusSign = false;
    bool spaceSign = false;
    bool altForm = false;
    bool zeroPad = false;
    int width = -1;
    int precision = -1;
    char conversion = 0;
};

[[noreturn]] void fail(const std::string& what)
{
    throw std::format_error("invalid printf format string: " + what);
}

// Parses the spec after '%'; p points past the '%' on entry and past the
// conversion letter on exit.
Spec parseSpec(const char*& p)
{
    Spec spec;
    for (;; ++p) {
        switch (*p) {
            case '-':
                spec.leftAlign = true;
                continue;
            case '+':
                spec.plusSign = true;
                continue;
            case ' ':
                spec.spaceSign = true;
                continue;
            case '#':
                spec.altForm = true;
                continue;
            case '0':
                spec.zeroPad = true;
                continue;
            default:
                break;
        }
        break;
    }
    if (*p == '*') {
        fail("runtime '*' width is not supported");
    }
    if (isDigit(*p)) {
        spec.width = 0;
        while (isDigit(*p)) {
            spec.width = spec.width * 10 + (*p++ - '0');
        }
    }
    if (*p == '.') {
        ++p;
        if (*p == '*') {
            fail("runtime '*' precision is not supported");
        }
        spec.precision = 0;
        while (isDigit(*p)) {
            spec.precision = spec.precision * 10 + (*p++ - '0');
        }
    }
    // length modifiers carry no information for std::format: the argument
    // brings its own type
    while (std::strchr("hlLqzjt", *p) != nullptr) {
        ++p;
    }
    if (*p == '\0') {
        fail("unterminated conversion specification");
    }
    spec.conversion = *p++;
    return spec;
}

void appendFormatSpec(std::string& out, const Spec& spec)
{
    const char conv = spec.conversion;
    std::string fmt;  // the part after ':' in "{:...}"

    switch (conv) {
        case 'd':
        case 'i':
        case 'u':
        case 'o':
        case 'x':
        case 'X':
        case 'c':
        case 's':
        case 'p':
        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
        case 'a':
        case 'A':
            break;
        default:
            fail(std::string("unsupported conversion '%") + conv + "'");
    }

    const bool integer = std::strchr("diuoxX", conv) != nullptr;
    const bool floating = std::strchr("eEfFgGaA", conv) != nullptr;

    int width = spec.width;
    bool zeroPad = spec.zeroPad && !spec.leftAlign;
    int precision = spec.precision;

    if (precision >= 0 && integer) {
        // printf: minimum number of digits, zero-padded.  std::format has
        // no integer precision; the equivalent of the "%8.8X" style used in
        // FreeCAD is zero padding to the same width.
        if (spec.leftAlign || (width > 0 && width != precision)) {
            fail("integer precision combined with differing width");
        }
        width = precision;
        precision = -1;
        zeroPad = true;
    }

    if (spec.leftAlign) {
        fmt += '<';
    }
    else if (conv == 's' && width > 0) {
        // printf right-aligns everything; std::format left-aligns strings
        fmt += '>';
    }
    if (spec.plusSign) {
        fmt += '+';
    }
    else if (spec.spaceSign) {
        fmt += ' ';
    }
    if (spec.altForm) {
        fmt += '#';
    }
    if (zeroPad && (integer || floating)) {
        fmt += '0';
    }
    if (width > 0) {
        fmt += std::to_string(width);
    }
    if (precision >= 0) {
        if (!floating && conv != 's') {
            fail(std::string("precision is not supported for '%") + conv + "'");
        }
        fmt += '.';
        fmt += std::to_string(precision);
    }

    switch (conv) {
        case 'i':
        case 'u':
            fmt += 'd';
            break;
        case 'p':
            // the default presentation for pointers is the address in hex,
            // same as printf %p
            break;
        default:
            fmt += conv;
            break;
    }

    out += '{';
    if (!fmt.empty()) {
        out += ':';
        out += fmt;
    }
    out += '}';
}

}  // namespace

std::string Base::printfToFormatString(const char* fmt)
{
    if (fmt == nullptr) {
        fail("null format string");
    }
    std::string out;
    out.reserve(std::strlen(fmt));
    for (const char* p = fmt; *p != '\0';) {
        char c = *p++;
        if (c == '{' || c == '}') {
            // literal braces must be escaped for std::format
            out += c;
            out += c;
            continue;
        }
        if (c != '%') {
            out += c;
            continue;
        }
        if (*p == '%') {
            out += '%';
            ++p;
            continue;
        }
        appendFormatSpec(out, parseSpec(p));
    }
    return out;
}
