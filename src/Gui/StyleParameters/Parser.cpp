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

#include "Parser.h"
#include "ParameterManager.h"

#include <Utilities.h>
#include <Base/Tools.h>

#include <QColor>
#include <algorithm>
#include <variant>

namespace Gui::StyleParameters
{

Value ParameterReference::evaluate(const EvaluationContext& context) const
{
    return context.manager->resolve(name, context.context).value_or("@" + name);
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
    auto argsValue = arguments.evaluate(context);
    const auto& args = argsValue.get<Tuple>();

    const auto lightenOrDarken = [this, &args]() -> Value {
        auto resolved = ArgumentParser {{"color"}, {"amount"}}.resolve(args);

        auto color = resolved.get<Base::Color>("color").asValue<QColor>();

        // In Qt if you want to make color 20% darker or lighter, you need to pass 120 as the value
        // we, however, want users to pass only the relative difference, hence we need to add the
        // 100 required by Qt.
        //
        // NOLINTNEXTLINE(*-magic-numbers)
        auto amount = 100 + static_cast<int>(resolved.get<Numeric>("amount").value);

        if (functionName == "lighten") {
            return Base::Color::fromValue(color.lighter(amount));
        }

        if (functionName == "darken") {
            return Base::Color::fromValue(color.darker(amount));
        }

        return {};
    };

    const auto blend = [&args]() -> Value {
        auto resolved = ArgumentParser {{"from"}, {"to"}, {"amount"}}.resolve(args);

        auto firstColor = resolved.get<Base::Color>("from");
        auto secondColor = resolved.get<Base::Color>("to");
        auto amount = Base::fromPercent(static_cast<long>(resolved.get<Numeric>("amount").value));

        return Base::Color(
            (1 - amount) * firstColor.r + amount * secondColor.r,
            (1 - amount) * firstColor.g + amount * secondColor.g,
            (1 - amount) * firstColor.b + amount * secondColor.b
        );
    };

    std::map<std::string, std::function<Value()>> functions = {
        {"lighten", lightenOrDarken},
        {"darken", lightenOrDarken},
        {"blend", blend},
    };

    if (functions.contains(functionName)) {
        return functions.at(functionName)();
    }

    THROWM(Base::ExpressionError, fmt::format("Unknown function '{}'", functionName));
}

Value BinaryOp::evaluate(const EvaluationContext& context) const
{
    Value lval = left->evaluate(context);
    Value rval = right->evaluate(context);

    switch (op) {
        case Operator::Add:
            return lval + rval;
        case Operator::Subtract:
            return lval - rval;
        case Operator::Multiply:
            return lval * rval;
        case Operator::Divide:
            return lval / rval;
        default:
            THROWM(Base::ExpressionError, "Unknown operator");
    }
}

Value TupleLiteral::evaluate(const EvaluationContext& context) const
{
    Tuple tuple;
    for (const auto& elem : elements) {
        tuple.elements.push_back(
            {elem.name, std::make_shared<const Value>(elem.expression->evaluate(context))}
        );
    }
    return tuple;
}

Value UnaryOp::evaluate(const EvaluationContext& context) const
{
    Value val = operand->evaluate(context);

    switch (op) {
        case Operator::Add:
            return val;
        case Operator::Subtract:
            return -val;
        default:
            THROWM(Base::ExpressionError, "Unknown unary operator");
    }
}

Value MemberAccess::evaluate(const EvaluationContext& context) const
{
    Value val = object->evaluate(context);
    if (!val.holds<Tuple>()) {
        THROWM(Base::ExpressionError, "Member access requires a tuple");
    }

    const auto& tuple = val.get<Tuple>();

    if (const Value* found = tuple.find(member)) {
        return *found;
    }

    if (std::ranges::all_of(member, ::isdigit)) {
        return tuple.at(std::stoul(member));
    }

    THROWM(Base::ExpressionError, fmt::format("Tuple has no member '{}'", member));
}

std::unique_ptr<Expr> Parser::parse()
{
    auto expr = parseExpression();
    skipWhitespace();
    if (pos != input.size()) {
        THROWM(
            Base::ParserError,
            fmt::format("Unexpected characters at end of input: {}", input.substr(pos))
        );
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

    std::unique_ptr<Expr> expr;

    if (match('(')) {
        // Disambiguation: tuple vs grouped expression
        // 1. If we see `identifier:` pattern → definitely a tuple
        if (peekNamedElement()) {
            expr = parseTuple();
        }
        else {
            // 2. Otherwise parse first expression
            expr = parseExpression();
            skipWhitespace();

            // If followed by `,` → reinterpret as tuple with this as first element
            if (pos < input.size() && input[pos] == ',') {
                ++pos;
                TupleLiteral::Element first;
                first.expression = std::move(expr);
                expr = parseTuple(std::move(first));
            }
            else {
                // If followed by `)` → grouped expression (backward compatible)
                if (!match(')')) {
                    THROWM(Base::ParserError, fmt::format("Expected ')', got '{}'", input[pos]));
                }
            }
        }
    }
    else if (peekColor()) {
        expr = parseColor();
    }
    else if (peekParameter()) {
        expr = parseParameter();
    }
    else if (peekFunction()) {
        expr = parseFunctionCall();
    }
    else {
        expr = parseNumber();
    }

    while (pos < input.size() && input[pos] == '.') {
        ++pos;
        expr = std::make_unique<MemberAccess>(std::move(expr), parseMember());
    }

    return expr;
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

        return std::make_unique<Color>(Base::Color(r / 255.0, g / 255.0, b / 255.0));
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
            THROWM(
                Base::ParserError,
                fmt::format("Expected ')' after color arguments, got '{}'", input[pos])
            );
        }
        return std::make_unique<Color>(Base::Color(r / 255.0, g / 255.0, b / 255.0, a / 255.0));
    };

    skipWhitespace();

    try {
        if (input[pos] == '#') {
            return parseHexadecimalColor();
        }

        if (peekString(rgbFunction) || peekString(rgbaFunction)) {
            return parseFunctionStyleColor();
        }
    }
    catch (std::invalid_argument&) {
        THROWM(
            Base::ParserError,
            "Invalid color format, expected #RRGGBB or rgb(r,g,b) or rgba(r,g,b,a)"
        );
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
        THROWM(
            Base::ParserError,
            fmt::format("Expected parameter name after '@', got '{}'", input[pos])
        );
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
        THROWM(Base::ParserError, fmt::format("Expected '(' after function name, got '{}'", input[pos]));
    }

    auto arguments = parseTuple();
    return std::make_unique<FunctionCall>(functionName, std::move(*arguments));
}

bool Parser::peekNamedElement()
{
    size_t saved = pos;
    skipWhitespace();

    // Check for `identifier :` pattern
    if (pos >= input.size() || !isalpha(input[pos])) {
        pos = saved;
        return false;
    }

    // Skip identifier characters
    while (pos < input.size() && (isalnum(input[pos]) || input[pos] == '_')) {
        ++pos;
    }

    // Skip whitespace between identifier and colon
    while (pos < input.size() && isspace(input[pos])) {
        ++pos;
    }

    // Check for colon
    bool found = pos < input.size() && input[pos] == ':';
    pos = saved;
    return found;
}

std::unique_ptr<TupleLiteral> Parser::parseTuple(std::optional<TupleLiteral::Element> firstElement)
{
    auto tuple = std::make_unique<TupleLiteral>();

    const auto parseElement = [this]() {
        TupleLiteral::Element elem;

        // Check if this element has a name
        if (peekNamedElement()) {
            skipWhitespace();
            size_t start = pos;
            while (pos < input.size() && (isalnum(input[pos]) || input[pos] == '_')) {
                ++pos;
            }
            elem.name = input.substr(start, pos - start);
            skipWhitespace();
            ++pos;  // consume ':'
        }

        elem.expression = parseExpression();
        return elem;
    };

    if (firstElement) {
        // Called from unnamed-element path: first element already parsed, comma already consumed
        tuple->elements.push_back(std::move(*firstElement));
        // Parse the second element immediately (comma was already consumed by caller)
        tuple->elements.push_back(parseElement());
    }

    // Parse remaining elements
    while (true) {
        skipWhitespace();
        if (pos < input.size() && input[pos] == ')') {
            ++pos;
            return tuple;
        }

        if (!tuple->elements.empty() && !match(',')) {
            if (pos >= input.size()) {
                THROWM(Base::ParserError, "Expected ')' to close tuple");
            }
            THROWM(Base::ParserError, fmt::format("Expected ',' or ')' in tuple, got '{}'", input[pos]));
        }

        tuple->elements.push_back(parseElement());
    }
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

std::string Parser::parseMember()
{
    size_t start = pos;
    while (pos < input.size() && (isalnum(input[pos]) || input[pos] == '_')) {
        ++pos;
    }
    if (start == pos) {
        THROWM(Base::ParserError, "Expected member name after '.'");
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
