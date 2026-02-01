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
#include <tuple>

#include "ExpressionParser.h"
#include "ExpressionTokenizer.h"

using namespace App;


// Code below inspired by blog entry:
// https://john.nachtimwald.com/2009/07/04/qcompleter-and-comma-separated-tags/

QString ExpressionTokenizer::perform(const QString& prefix, int pos)
{
    // ExpressionParser::tokenize() only supports std::string but we need a tuple QString
    // because due to UTF-8 encoding a std::string may be longer than a QString
    // See https://forum.freecad.org/viewtopic.php?f=3&t=69931
    auto tokenizeExpression = [](const QString& expr) {
        std::vector<std::tuple<int, int, std::string>> result =
            ExpressionParser::tokenize(expr.toStdString());
        std::vector<std::tuple<int, int, QString>> tokens;
        std::transform(
            result.cbegin(),
            result.cend(),
            std::back_inserter(tokens),
            [&](const std::tuple<int, int, std::string>& item) {
                return std::make_tuple(
                    std::get<0>(item),
                    QString::fromStdString(expr.toStdString().substr(0, std::get<1>(item))).size(),
                    QString::fromStdString(std::get<2>(item)));
            });
        return tokens;
    };

    // Compute start; if prefix starts with =, start parsing from offset 1.
    int start = (prefix.size() > 0 && prefix.at(0) == QChar::fromLatin1('=')) ? 1 : 0;

    // Tokenize prefix
    std::vector<std::tuple<int, int, QString>> tokens = tokenizeExpression(prefix.mid(start));

    // No tokens
    if (tokens.empty()) {
        return {};
    }

    prefixEnd = prefix.size();

    // Pop those trailing tokens depending on the given position, which may be
    // in the middle of a token, and we shall include that token.
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        int tokenType = std::get<0>(*it);
        int location = std::get<1>(*it);
        int tokenLength = static_cast<int> (std::get<2>(*it).size());
        if (location >= pos) {
            // Include the immediately followed '.' or '#', because we'll be
            // inserting these separators too, in ExpressionCompleteModel::pathFromIndex()
            if (it != tokens.begin() && tokenType != '.' && tokenType != '#') {
                it = it - 1;
            }
            tokens.resize(it - tokens.begin() + 1); // Invalidates it, but we already calculated tokenLength
            prefixEnd = start + location + tokenLength;
            break;
        }
    }

    int trim = 0;
    if (prefixEnd > pos) {
        trim = prefixEnd - pos;
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
    if (!stringing && !prefix.isEmpty() && prefixEnd > 0 && prefixEnd <= prefix.size()
        && prefix[prefixEnd - 1] == QChar(32)) {
        return {};
    }

    if (!stringing) {
        i = static_cast<long>(tokens.size()) - 1;
        for (; i >= 0; --i) {
            int token = std::get<0>(tokens[i]);

            // Allow Visual Name Suffix: IDENTIFIER < INTEGER >
            // We are iterating backwards. If we see GT, look back for INT then LT.
            if (token == ExpressionParser::GT) {
                bool isSuffix = false;
                if (i >= 2) {
                    int prev1 = std::get<0>(tokens[i - 1]);
                    int prev2 = std::get<0>(tokens[i - 2]);

                    // Check for pattern: < INT >
                    // Added ExpressionParser::ONE (259) to the check
                    if ((prev1 == ExpressionParser::INTEGER || prev1 == ExpressionParser::NUM
                         || prev1 == ExpressionParser::ONE)
                        && prev2 == ExpressionParser::LT) {
                        isSuffix = true;
                        i -= 2;  // Skip over the INTEGER/ONE and LT
                    }
                }
                if (isSuffix) {
                    continue;
                }
            }

            if (token == ')') {
                int nesting = 1;
                long j = i - 1;
                while (j >= 0) {
                    int t = std::get<0>(tokens[j]);
                    if (t == ')') {
                        nesting++;
                    }
                    else if (t == '(') {
                        nesting--;
                    }

                    if (nesting == 0) {
                        break;
                    }
                    j--;
                }

                // If we found the matching '(', check if the token before it is "Origin"
                if (j > 0) {
                    int prevToken = std::get<0>(tokens[j - 1]);
                    if (prevToken == ExpressionParser::IDENTIFIER) {
                        // If the pattern is "Identifier (...)", we treat it as a potential
                        // disambiguated name (like "Origin (Body)" or "Group (Item)")
                        i = j - 1;
                        continue;
                    }
                }
                // If it was "hiddenref" or "sin", we fall through here.
                // The check below (token != IDENTIFIER...) will catch the ')' and break the loop.
                // This ensures "hiddenref(...)" is NOT treated as a single token.
            }

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
        prefixStart = prefixEnd;
    }
    else {
        prefixStart = start + std::get<1>(tokens[i]);
    }

    QString completionPrefix = prefix.mid(prefixStart, prefixEnd - prefixStart);

    if (trim && trim < int(completionPrefix.size())) {
        completionPrefix.resize(completionPrefix.size() - trim);
    }

    return completionPrefix;
}
