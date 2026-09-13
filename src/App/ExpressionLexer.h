// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ExpressionParser.h"

namespace App::ExpressionParser
{

using FunctionLookup = std::function<FunctionExpression::Function(const std::string&)>;

/** Tokenize an expression. */
std::vector<Token> scanTokens(const char* buffer, const FunctionLookup& lookupFunction);

/** Tokenize editor input, retaining tokens before malformed or incomplete trailing text. */
std::vector<Token> scanTokensTolerant(const char* buffer, const FunctionLookup& lookupFunction);

/** Tokenize using FreeCAD's registered expression functions. */
std::vector<Token> scanExpressionTokens(const DocumentObject* owner, const char* buffer);

/** Tokenize incomplete editor input using FreeCAD's registered expression functions. */
std::vector<Token> scanExpressionTokensTolerant(const char* buffer);

}  // namespace App::ExpressionParser
