// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "ExpressionParser.h"
#include "ExpressionTokenizer.h"

using namespace App;


// Code below inspired by blog entry:
// https://john.nachtimwald.com/2009/07/04/qcompleter-and-comma-separated-tags/

std::string ExpressionTokenizer::perform(std::string_view prefix, std::size_t posBytes)
{
    std::string completionPrefix;

    // Compute start; if prefix starts with =, start parsing from offset 1.
    const bool hasEqualsPrefix = !prefix.empty() && prefix.front() == '=';
    const int start = hasEqualsPrefix ? 1 : 0;  // bytes
    const std::string_view parseInput = hasEqualsPrefix ? prefix.substr(1) : prefix;
    const std::size_t adjustedPosBytes = (posBytes >= static_cast<std::size_t>(start))
        ? (posBytes - static_cast<std::size_t>(start))
        : 0;

    // Tokenize prefix (byte offsets)
    std::vector<std::tuple<int, int, std::string>> tokens =
        ExpressionParser::tokenize(std::string(parseInput));

    // No tokens
    if (tokens.empty()) {
        return {};
    }

    prefixEndBytes = static_cast<int>(prefix.size());

    // Pop those trailing tokens depending on the given position, which may be
    // in the middle of a token, and we shall include that token.
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        int tokenType = std::get<0>(*it);
        int location = std::get<1>(*it);
        int tokenLength = static_cast<int>(std::get<2>(*it).size());
        if (static_cast<std::size_t>(location) >= adjustedPosBytes) {
            // Include the immediately followed '.' or '#', because we'll be
            // inserting these separators too, in ExpressionCompleteModel::pathFromIndex()
            if (it != tokens.begin() && tokenType != '.' && tokenType != '#') {
                --it;
                location = std::get<1>(*it);
                tokenLength = static_cast<int>(std::get<2>(*it).size());
            }
            tokens.resize(it - tokens.begin() + 1); // Invalidates it, but we already calculated tokenLength
            prefixEndBytes = start + location + tokenLength;
            break;
        }
    }

    int trim = 0;
    if (prefixEndBytes > static_cast<int>(posBytes)) {
        trim = prefixEndBytes - static_cast<int>(posBytes);
    }

    // Extract last tokens that can be rebuilt to a variable
    long i = static_cast<long>(tokens.size()) - 1;

    // First, check if we have unclosing string starting from the end
    bool stringing = false;
    for (; i >= 0; --i) {
        int token = std::get<0>(tokens[i]);
        if (token == ExpressionParser::STRING) {
            stringing = false;
            break;
        }

        if (token == ExpressionParser::LT && i > 0
            && std::get<0>(tokens[i - 1]) == ExpressionParser::LT) {
            --i;
            stringing = true;
            break;
        }
    }

    // Not an unclosed string and the last character is a space
    if (!stringing && !prefix.empty() && prefixEndBytes > 0
        && prefixEndBytes <= static_cast<int>(prefix.size())
        && static_cast<unsigned char>(prefix[static_cast<std::size_t>(prefixEndBytes - 1)]) == 32) {
            return {};
    }

    if (!stringing) {
        i = static_cast<long>(tokens.size()) - 1;
        for (; i >= 0; --i) {
            int token = std::get<0>(tokens[i]);
            if (token != '.' && token != '#' && token != ExpressionParser::IDENTIFIER
                && token != ExpressionParser::INTEGER && token != ExpressionParser::STRING
                && token != ExpressionParser::UNIT && token != ExpressionParser::ONE) {
                break;
            }
        }
        ++i;
    }

    // Set prefix start for use when replacing later
    if (i == static_cast<long>(tokens.size())) {
        prefixStartBytes = prefixEndBytes;
    }
    else {
        prefixStartBytes = start + std::get<1>(tokens[i]);
    }

    // Build prefix from tokens
    while (i < static_cast<long>(tokens.size())) {
        completionPrefix += std::get<2>(tokens[i]);
        ++i;
    }

    if (trim) {
        if (trim < static_cast<int>(completionPrefix.size())) {
            completionPrefix.resize(completionPrefix.size() - static_cast<std::size_t>(trim));
        }
    }

    return completionPrefix;
}
