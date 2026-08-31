// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ExpressionPrattParser.h"

namespace App::ExpressionParser::Pratt
{

using FunctionLookup = std::function<FunctionExpression::Function(const std::string&)>;

/** Tokenize an expression. */
std::vector<Token> scanTokens(const char* buffer, const FunctionLookup& lookupFunction);

/** Tokenize using FreeCAD's registered expression functions. */
std::vector<Token> scanExpressionTokens(const DocumentObject* owner, const char* buffer);

}  // namespace App::ExpressionParser::Pratt
