// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"

#include "Parser.h"
#include "ParameterManager.h"

#include <Base/Tools.h>

#ifndef _PreComp_
#include <QColor>
#include <QRegularExpression>
#include <QString>
#include <ranges>
#include <variant>
#endif

namespace Gui::StyleParameters
{

Value ParameterReference::evaluate(const EvaluationContext& context) const
{
    return context.manager->resolve(name, context.context);
}

Value Number::evaluate([[maybe_unused]] const EvaluationContext& context) const
{
    return value;
}

Value Color::evaluate([[maybe_unused]] const EvaluationContext& context) const
{
    return color;
}

Value FunctionCall::evaluate(const EvaluationContext& context) const
{
    const auto lightenOrDarken = [this](const EvaluationContext& context) -> Value {
        if (arguments.size() != 2) {
            THROWM(Base::ExpressionError,
                   fmt::format("Function '{}' expects 2 arguments, got {}",
                               functionName,
                               arguments.size()));
        }

        auto colorArg = arguments[0]->evaluate(context);
        auto amountArg = arguments[1]->evaluate(context);

        if (!std::holds_alternative<QColor>(colorArg)) {
            THROWM(Base::ExpressionError,
                   fmt::format("'{}' is not supported for colors", functionName));
        }

        auto color = std::get<QColor>(colorArg);

        // In Qt if you want to make color 20% darker or lighter, you need to pass 120 as the value
        // we, however, want users to pass only the relative difference, hence we need to add the
        // 100 required by Qt.
        //
        // NOLINTNEXTLINE(*-magic-numbers)
        auto amount = 100 + static_cast<int>(std::get<Length>(amountArg).value);

        if (functionName == "lighten") {
            return color.lighter(amount);
        }

        if (functionName == "darken") {
            return color.darker(amount);
        }

        return {};
    };

    const auto blend = [this](const EvaluationContext& context) -> Value {
        if (arguments.size() != 3) {
            THROWM(Base::ExpressionError,
                   fmt::format("Function '{}' expects 3 arguments, got {}",
                               functionName,
                               arguments.size()));
        }

        auto firstColorArg = arguments[0]->evaluate(context);
        auto secondColorArg = arguments[1]->evaluate(context);
        auto amountArg = arguments[2]->evaluate(context);

        if (!std::holds_alternative<QColor>(firstColorArg)) {
            THROWM(Base::ExpressionError,
                   fmt::format("first argument of '{}' must be color", functionName));
        }

        if (!std::holds_alternative<QColor>(secondColorArg)) {
            THROWM(Base::ExpressionError,
                   fmt::format("second argument of '{}' must be color", functionName));
        }

        auto firstColor = std::get<QColor>(firstColorArg);
        auto secondColor = std::get<QColor>(secondColorArg);

        auto amount = Base::fromPercent(std::get<Length>(amountArg).value);

        return QColor::fromRgbF(
            (1 - amount) * firstColor.redF() + amount * secondColor.redF(),
            (1 - amount) * firstColor.greenF() + amount * secondColor.greenF(),
            (1 - amount) * firstColor.blueF() + amount * secondColor.blueF()
        );
    };

    std::map<std::string, std::function<Value(const EvaluationContext&)>> functions = {
        {"lighten", lightenOrDarken},
        {"darken", lightenOrDarken},
        {"blend", blend},
    };

    if (functions.contains(functionName)) {
        auto function = functions.at(functionName);
        return function(context);
    }

    THROWM(Base::ExpressionError, fmt::format("Unknown function '{}'", functionName));
}

Value BinaryOp::evaluate(const EvaluationContext& context) const
{
    Value lval = left->evaluate(context);
    Value rval = right->evaluate(context);

    if (!std::holds_alternative<Length>(lval) || !std::holds_alternative<Length>(rval)) {
        THROWM(Base::ExpressionError, "Math operations are supported only on lengths");
    }

    auto lhs = std::get<Length>(lval);
    auto rhs = std::get<Length>(rval);

    switch (op) {
        case Operator::Add:
            return lhs + rhs;
        case Operator::Subtract:
            return lhs - rhs;
        case Operator::Multiply:
            return lhs * rhs;
        case Operator::Divide:
            return lhs / rhs;
        default:
            THROWM(Base::ExpressionError, "Unknown operator");
    }
}

Value UnaryOp::evaluate(const EvaluationContext& context) const
{
    Value val = operand->evaluate(context);
    if (std::holds_alternative<QColor>(val)) {
        THROWM(Base::ExpressionError, "Unary operations on colors are not supported");
    }

    auto v = std::get<Length>(val);
    switch (op) {
        case Operator::Add:
            return v;
        case Operator::Subtract:
            return -v;
        default:
            THROWM(Base::ExpressionError, "Unknown unary operator");
    }
}

std::unique_ptr<Expr> Parser::parse()
{
    auto expr = parseExpression();
    skipWhitespace();
    if (pos != input.size()) {
        THROWM(Base::ParserError,
               fmt::format("Unexpected characters at end of input: {}", input.substr(pos)));
    }
    return expr;
}

bool Parser::peekString(const char* function) const
{
    return input.compare(pos, strlen(function), function) == 0;
}

std::unique_ptr<Expr> Parser::parseExpression()
{
    auto expr = parseTerm();
    while (true) {
        skipWhitespace();
        if (match('+')) {
            expr = std::make_unique<BinaryOp>(std::move(expr), Operator::Add, parseTerm());
        }
        else if (match('-')) {
            expr = std::make_unique<BinaryOp>(std::move(expr), Operator::Subtract, parseTerm());
        }
        else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseTerm()
{
    auto expr = parseFactor();
    while (true) {
        skipWhitespace();
        if (match('*')) {
            expr = std::make_unique<BinaryOp>(std::move(expr), Operator::Multiply, parseFactor());
        }
        else if (match('/')) {
            expr = std::make_unique<BinaryOp>(std::move(expr), Operator::Divide, parseFactor());
        }
        else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseFactor()
{
    skipWhitespace();
    if (match('+') || match('-')) {
        Operator op = (input[pos - 1] == '+') ? Operator::Add : Operator::Subtract;
        return std::make_unique<UnaryOp>(op, parseFactor());
    }
    if (match('(')) {
        auto expr = parseExpression();
        if (!match(')')) {
            THROWM(Base::ParserError, fmt::format("Expected ')', got '{}'", input[pos]));
        }
        return expr;
    }
    if (peekColor()) {
        return parseColor();
    }
    if (peekParameter()) {
        return parseParameter();
    }
    if (peekFunction()) {
        return parseFunctionCall();
    }
    return parseNumber();
}

bool Parser::peekColor()
{
    skipWhitespace();
    // clang-format off
    return input[pos] == '#'
        || peekString(rgbFunction)
        || peekString(rgbaFunction);
    // clang-format on
}

std::unique_ptr<Expr> Parser::parseColor()
{
    const auto parseHexadecimalColor = [&]() {
        constexpr int hexadecimalBase = 16;

            // Format is #RRGGBB
            pos++;
            int r = std::stoi(input.substr(pos, 2), nullptr, hexadecimalBase);
            pos += 2;
            int g = std::stoi(input.substr(pos, 2), nullptr, hexadecimalBase);
            pos += 2;
            int b = std::stoi(input.substr(pos, 2), nullptr, hexadecimalBase);
            pos += 2;

            return std::make_unique<Color>(QColor(r, g, b));
    };

    const auto parseFunctionStyleColor = [&]() {
        bool hasAlpha = peekString(rgbaFunction);

        pos += hasAlpha ? strlen(rgbaFunction) : strlen(rgbFunction);

        int r = parseInt();
        if (!match(',')) {
                THROWM(Base::ParserError, fmt::format("Expected ',' after red, got '{}'", input[pos]));
        }
        int g = parseInt();
        if (!match(',')) {
                THROWM(Base::ParserError, fmt::format("Expected ',' after green, got '{}'", input[pos]));
        }
        int b = parseInt();
        int a = 255;  // NOLINT(*-magic-numbers)
        if (hasAlpha) {
            if (!match(',')) {
                THROWM(Base::ParserError, fmt::format("Expected ',' after blue, got '{}'", input[pos]));
            }
            a = parseInt();
        }
        if (!match(')')) {
            THROWM(Base::ParserError, fmt::format("Expected ')' after color arguments, got '{}'", input[pos]));
        }
        return std::make_unique<Color>(QColor(r, g, b, a));
    };

    skipWhitespace();

    try {
        if (input[pos] == '#') {
            return parseHexadecimalColor();
        }

        if (peekString(rgbFunction) || peekString(rgbaFunction)) {
            return parseFunctionStyleColor();
        }
    } catch (std::invalid_argument&) {
        THROWM(Base::ParserError, "Invalid color format, expected #RRGGBB or rgb(r,g,b) or rgba(r,g,b,a)");
    }

    THROWM(Base::ParserError, "Unknown color format");
}

bool Parser::peekParameter()
{
    skipWhitespace();
    return pos < input.size() && input[pos] == '@';
}

std::unique_ptr<Expr> Parser::parseParameter()
{
    skipWhitespace();
    if (!match('@')) {
        THROWM(Base::ParserError, fmt::format("Expected '@' for parameter, got '{}'", input[pos]));
    }
    size_t start = pos;
    while (pos < input.size() && (isalnum(input[pos]) || input[pos] == '_')) {
        ++pos;
    }
    if (start == pos) {
        THROWM(Base::ParserError,
               fmt::format("Expected parameter name after '@', got '{}'", input[pos]));
    }
    return std::make_unique<ParameterReference>(input.substr(start, pos - start));
}

bool Parser::peekFunction()
{
    skipWhitespace();
    return pos < input.size() && isalpha(input[pos]);
}

std::unique_ptr<Expr> Parser::parseFunctionCall()
{
    skipWhitespace();
    size_t start = pos;
    while (pos < input.size() && isalnum(input[pos])) {
        ++pos;
    }
    std::string functionName = input.substr(start, pos - start);

    if (!match('(')) {
        THROWM(Base::ParserError,
               fmt::format("Expected '(' after function name, got '{}'", input[pos]));
    }

    std::vector<std::unique_ptr<Expr>> arguments;
    if (!match(')')) {
        do {  // NOLINT(*-avoid-do-while)
            arguments.push_back(parseExpression());
        } while (match(','));

        if (!match(')')) {
            THROWM(Base::ParserError,
                   fmt::format("Expected ')' after function arguments, got '{}'", input[pos]));
        }
    }

    return std::make_unique<FunctionCall>(functionName, std::move(arguments));
}

int Parser::parseInt()
{
    skipWhitespace();
    size_t start = pos;
    while (pos < input.size() && (isdigit(input[pos]) || input[pos] == '.')) {
        ++pos;
    }
    return std::stoi(input.substr(start, pos - start));
}

std::unique_ptr<Expr> Parser::parseNumber()
{
    skipWhitespace();
    size_t start = pos;
    while (pos < input.size() && (isdigit(input[pos]) || input[pos] == '.')) {
        ++pos;
    }

    std::string number = input.substr(start, pos - start);

    try {
        double value = std::stod(number);
        std::string unit = parseUnit();
        return std::make_unique<Number>(value, unit);
    }
    catch (std::invalid_argument&) {
        THROWM(Base::ParserError, fmt::format("Invalid number: {}", number));
    }
}

std::string Parser::parseUnit()
{
    skipWhitespace();
    size_t start = pos;
    while (pos < input.size() && (isalpha(input[pos]) || input[pos] == '%')) {
        ++pos;
    }
    if (start == pos) {
        return "";
    }
    return input.substr(start, pos - start);
}

bool Parser::match(char expected)
{
    skipWhitespace();
    if (pos < input.size() && input[pos] == expected) {
        ++pos;
        return true;
    }
    return false;
}

void Parser::skipWhitespace()
{
    while (pos < input.size() && isspace(input[pos])) {
        ++pos;
    }
}

}  // namespace Gui::StyleParameters