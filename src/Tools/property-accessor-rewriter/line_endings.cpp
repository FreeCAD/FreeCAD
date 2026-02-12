// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "line_endings.h"

#include <cstdint>
#include <stdexcept>


namespace LineEndings
{

const char* MixedLineEndingsError = "Mixed or unknown line endings detected.";

enum class LineEndingType : uint8_t
{
    Dos,
    Unix
};


LineEndingType detect(const std::string& source)
{
    bool hasCRLF = false;
    bool hasLF = false;

    for (size_t i = 0; i < source.size(); ++i) {
        if (source[i] == '\r') {
            if (i + 1 < source.size() && source[i + 1] == '\n') {
                hasCRLF = true;
                ++i;
            }
            else {
                throw std::runtime_error(MixedLineEndingsError);
            }
        }
        else if (source[i] == '\n') {
            hasLF = true;
        }
    }

    if (hasCRLF && !hasLF) {
        return LineEndingType::Dos;
    }

    if (hasLF && !hasCRLF) {
        return LineEndingType::Unix;
    }

    throw std::runtime_error(MixedLineEndingsError);
}

std::string findLineEnding(const std::string& source)
{
    switch (detect(source)) {
        case LineEndingType::Dos:
            return "\r\n";
        case LineEndingType::Unix:
            return "\n";
        default:
            throw std::runtime_error("Unknown line ending type.");
    }
}

}  // namespace LineEndings
