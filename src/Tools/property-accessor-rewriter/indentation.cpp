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

#include "indentation.h"

namespace Indentation
{

constexpr size_t TabWidthUnix = 8;

std::string stripOuterBraces(const std::string& body)
{
    std::string trimmed = body;
    size_t start = 0;
    size_t end = trimmed.size();

    auto isSpace = [](char c) {
        return c == ' ' || c == '\t';
    };

    auto isLineChar = [](char c) {
        return c == '\n' || c == '\r';
    };

    auto isWhiteSpace = [&](char c) {
        return isSpace(c) || isLineChar(c);
    };

    auto trimLeadingWS = [&]() {
        while (start < end && isWhiteSpace(trimmed[start])) {
            ++start;
        }
    };
    auto trimTrailingWS = [&]() {
        while (end > start && isWhiteSpace(trimmed[end - 1])) {
            --end;
        }
    };

    auto trimLine = [&]() {
        while (start < end && isSpace(trimmed[start])) {
            start++;
        }
        while (start < end && isLineChar(trimmed[start])) {
            start++;
        }
    };

    trimLeadingWS();
    trimTrailingWS();

    if (end <= start) {
        return "";
    }

    if (trimmed[start] == '{' && trimmed[end - 1] == '}') {
        ++start;
        --end;
        trimLine();
        trimTrailingWS();
    }

    return trimmed.substr(start, end - start);
}


std::size_t countIndentColumns(const std::string& input, std::size_t tabWidth = TabWidthUnix)
{
    std::size_t cols = 0;
    for (char c : input) {
        if (c == ' ') {
            cols += 1;
        }
        else if (c == '\t') {
            cols += tabWidth - (cols % tabWidth);
        }
        else {
            break;
        }
    }
    return cols;
}

std::string computeIndentationMethod(const std::string& linesOfBody)
{
    std::size_t cols = countIndentColumns(linesOfBody);
    return std::string(cols, ' ');
}

}  // namespace Indentation
