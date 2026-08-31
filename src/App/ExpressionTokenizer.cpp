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

#include "ExpressionLexer.h"
#include "ExpressionTokenizer.h"

using namespace App;
namespace Pratt = App::ExpressionParser::Pratt;


// Code below inspired by blog entry:
// https://john.nachtimwald.com/2009/07/04/qcompleter-and-comma-separated-tags/

QString ExpressionTokenizer::perform(const QString& prefix, int pos)
{
    using Pratt::TokenKind;

    // The lexer uses UTF-8 byte offsets but we need QString character offsets.
    // because due to UTF-8 encoding a std::string may be longer than a QString
    // See https://forum.freecad.org/viewtopic.php?f=3&t=69931
    auto tokenizeExpression = [](const QString& expr) {
        auto result = Pratt::scanExpressionTokensTolerant(expr.toStdString().c_str());
        std::vector<std::tuple<TokenKind, int, QString>> tokens;
        std::transform(
            result.cbegin(),
            result.cend(),
            std::back_inserter(tokens),
            [&](const Pratt::Token& item) {
                return std::make_tuple(
                    item.kind,
                    QString::fromStdString(expr.toStdString().substr(0, item.column)).size(),
                    QString::fromStdString(item.lexeme));
            });
        if (!tokens.empty() && std::get<0>(tokens.back()) == TokenKind::End) {
            tokens.pop_back();
        }
        return tokens;
    };

    QString completionPrefix;

    // Compute start; if prefix starts with =, start parsing from offset 1.
    int start = (prefix.size() > 0 && prefix.at(0) == QChar::fromLatin1('=')) ? 1 : 0;

    // Tokenize prefix
    std::vector<std::tuple<TokenKind, int, QString>> tokens = tokenizeExpression(prefix.mid(start));

    // No tokens
    if (tokens.empty()) {
        return {};
    }

    prefixEnd = prefix.size();

    // Pop those trailing tokens depending on the given position, which may be
    // in the middle of a token, and we shall include that token.
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        TokenKind tokenType = std::get<0>(*it);
        int location = std::get<1>(*it);
        int tokenLength = static_cast<int> (std::get<2>(*it).size());
        if (location >= pos) {
            // Include the immediately followed '.' or '#', because we'll be
            // inserting these separators too, in ExpressionCompleteModel::pathFromIndex()
            if (it != tokens.begin() && tokenType != TokenKind::Dot && tokenType != TokenKind::Hash) {
                --it;
                location = std::get<1>(*it);
                tokenLength = static_cast<int>(std::get<2>(*it).size());
            }
            tokens.resize(it - tokens.begin() + 1);
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
        TokenKind token = std::get<0>(tokens[i]);
        if (token == TokenKind::String) {
            stringing = false;
            break;
        }

        if (token == TokenKind::Less && i > 0
            && std::get<0>(tokens[i - 1]) == TokenKind::Less) {
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
            TokenKind token = std::get<0>(tokens[i]);
            const bool isOne = token == TokenKind::Number && std::get<2>(tokens[i]) == QStringLiteral("1");
            if (token != TokenKind::Dot && token != TokenKind::Hash && token != TokenKind::Name
                && token != TokenKind::Integer && token != TokenKind::String
                && token != TokenKind::Unit && !isOne) {
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

    // Build prefix from tokens
    while (i < static_cast<long>(tokens.size())) {
        completionPrefix += std::get<2>(tokens[i]);
        ++i;
    }

    if (trim && trim < int(completionPrefix.size())) {
        completionPrefix.resize(completionPrefix.size() - trim);
    }

    return completionPrefix;
}
