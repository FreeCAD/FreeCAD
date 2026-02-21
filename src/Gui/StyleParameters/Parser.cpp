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
#include "Corners.h"
#include "Gradient.h"
#include "Insets.h"
#include "ParameterManager.h"

#include <Utilities.h>
#include <Base/OkLch.h>
#include <Base/Tools.h>

#include <QColor>
#include <algorithm>
#include <cmath>
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

        // In Qt if you want to make color 20% darker or lighter, you need to pass 120 as the value
        // we, however, want users to pass only the relative difference, hence we need to add the
        // 100 required by Qt.
        //
        // NOLINTNEXTLINE(*-magic-numbers)
        auto amount = 100 + static_cast<int>(resolved.get<Numeric>("amount").value);

        const auto applyToColor = [&](const Base::Color& color) -> Base::Color {
            auto qcolor = color.asValue<QColor>();
            if (functionName == "lighten") {
                return Base::Color::fromValue(qcolor.lighter(amount));
            }
            return Base::Color::fromValue(qcolor.darker(amount));
        };

        const Value* colorValue = resolved.find("color");
        if (colorValue->holds<Tuple>()) {
            return Gradient::mapStopColors(colorValue->get<Tuple>(), applyToColor);
        }

        return applyToColor(resolved.get<Base::Color>("color"));
    };

    const auto blend = [&args]() -> Value {
        auto resolved = ArgumentParser {{"from"}, {"to"}, {"amount"}}.resolve(args);

        auto amount = Base::fromPercent(static_cast<long>(resolved.get<Numeric>("amount").value));

        const auto blendColors =
            [amount](const Base::Color& first, const Base::Color& second) -> Base::Color {
            return Base::Color(
                (1 - amount) * first.r + amount * second.r,
                (1 - amount) * first.g + amount * second.g,
                (1 - amount) * first.b + amount * second.b
            );
        };

        const Value* fromValue = resolved.find("from");
        const Value* toValue = resolved.find("to");

        bool fromIsGradient = fromValue->holds<Tuple>();
        bool toIsGradient = toValue->holds<Tuple>();

        if (fromIsGradient && toIsGradient) {
            THROWM(Base::ExpressionError, "Cannot blend two gradients");
        }

        if (fromIsGradient) {
            const auto& targetColor = toValue->get<Base::Color>();
            return Gradient::mapStopColors(fromValue->get<Tuple>(), [&](const Base::Color& stopColor) {
                return blendColors(stopColor, targetColor);
            });
        }

        if (toIsGradient) {
            const auto& sourceColor = fromValue->get<Base::Color>();
            return Gradient::mapStopColors(toValue->get<Tuple>(), [&](const Base::Color& stopColor) {
                return blendColors(sourceColor, stopColor);
            });
        }

        return blendColors(fromValue->get<Base::Color>(), toValue->get<Base::Color>());
    };

    const auto lightnessFromNumeric = [](const Numeric& numeric) -> float {
        if (numeric.unit == "%") {
            return static_cast<float>(numeric.value / 100.0);
        }
        return static_cast<float>(numeric.value);
    };

    const auto shade = [&args, &lightnessFromNumeric]() -> Value {
        auto resolved = ArgumentParser {{"color"}, {"lightness"}}.resolve(args);
        auto targetLightness = lightnessFromNumeric(resolved.get<Numeric>("lightness"));

        const auto applyToColor = [&](const Base::Color& color) -> Base::Color {
            auto oklch = Base::toOkLch(color);
            oklch.lightness = targetLightness;
            return Base::fromOkLch(oklch, color.a);
        };

        const Value* colorValue = resolved.find("color");
        if (colorValue->holds<Tuple>()) {
            return Gradient::mapStopColors(colorValue->get<Tuple>(), applyToColor);
        }

        return applyToColor(resolved.get<Base::Color>("color"));
    };

    const auto shades = [&args, &lightnessFromNumeric]() -> Value {
        auto resolved = ArgumentParser {
            {.name = "color"},
            {.name = "shades"},
            {.name = "range", .defaultValue = Numeric {0.6, ""}},
            {.name = "min", .defaultValue = Numeric {0.13, ""}},
            {.name = "max", .defaultValue = Numeric {0.97, ""}},
        }.resolve(args);

        const auto& shadesSpec = resolved.get<Tuple>("shades");
        float range = lightnessFromNumeric(resolved.get<Numeric>("range"));
        float minLightness = lightnessFromNumeric(resolved.get<Numeric>("min"));
        float maxLightness = lightnessFromNumeric(resolved.get<Numeric>("max"));

        constexpr float anchorPosition = 0.5F;
        constexpr float minExponent = 0.1F;
        constexpr float maxExponent = 10.0F;

        const auto computeLightnessRange = [&](float anchorLightness) -> std::pair<float, float> {
            float halfRange = range / 2.0F;
            float high = std::min(maxLightness, anchorLightness + halfRange);
            float low = std::max(minLightness, anchorLightness - halfRange);
            return {high, low};
        };

        const auto computeExponent = [&](float anchorLightness, float high, float low) -> float {
            float totalRange = high - low;
            if (totalRange < 1e-6F) {
                return 1.0F;
            }
            float ratio = (high - anchorLightness) / totalRange;
            ratio = std::clamp(ratio, 0.01F, 0.99F);
            float exponent = std::log(ratio) / std::log(anchorPosition);
            return std::clamp(exponent, minExponent, maxExponent);
        };

        const auto lightnessForPosition =
            [](float position, float exponent, float high, float low) -> float {
            return high - (high - low) * std::pow(position, exponent);
        };

        const auto applyShade =
            [&](float position, const Base::Color& color, const Base::OkLch& oklch) -> Base::Color {
            if (std::abs(position - anchorPosition) < 1e-3F) {
                return color;
            }
            auto [high, low] = computeLightnessRange(oklch.lightness);
            float exponent = computeExponent(oklch.lightness, high, low);
            auto shadeOklch = oklch;
            shadeOklch.lightness = lightnessForPosition(position, exponent, high, low);
            return Base::fromOkLch(shadeOklch, color.a);
        };

        const auto appendElement = [](Tuple& result, const Tuple::Element& spec, Value shadeValue) {
            if (spec.name) {
                result.elements.push_back(Tuple::Element::named(*spec.name, std::move(shadeValue)));
            }
            else {
                result.elements.push_back(Tuple::Element::unnamed(std::move(shadeValue)));
            }
        };

        const Value* colorValue = resolved.find("color");
        if (colorValue->holds<Tuple>()) {
            const auto& gradientTuple = colorValue->get<Tuple>();

            Tuple result;
            for (const auto& element : shadesSpec.elements) {
                float position = lightnessFromNumeric(element.value->get<Numeric>());
                auto shadedGradient = Gradient::mapStopColors(
                    gradientTuple,
                    [&](const Base::Color& stopColor) -> Base::Color {
                        auto oklch = Base::toOkLch(stopColor);
                        return applyShade(position, stopColor, oklch);
                    }
                );
                appendElement(result, element, std::move(shadedGradient));
            }
            return result;
        }

        const auto& baseColor = resolved.get<Base::Color>("color");
        auto baseOklch = Base::toOkLch(baseColor);

        Tuple result;
        for (const auto& element : shadesSpec.elements) {
            float position = lightnessFromNumeric(element.value->get<Numeric>());
            auto shadeColor = applyShade(position, baseColor, baseOklch);
            appendElement(result, element, shadeColor);
        }
        return result;
    };

    std::map<std::string, std::function<Value()>> functions = {
        {"lighten", lightenOrDarken},
        {"darken", lightenOrDarken},
        {"blend", blend},
        {"shade", shade},
        {"shades", shades},
        {"padding", [&args]() -> Value { return Padding(args).tuple(); }},
        {"margins", [&args]() -> Value { return Margins(args).tuple(); }},
        {"border_thickness", [&args]() -> Value { return BorderThickness(args).tuple(); }},
        {"border_radius", [&args]() -> Value { return Corners(args).tuple(); }},
        {"linear_gradient", [&args]() -> Value { return LinearGradient(args).tuple(); }},
        {"radial_gradient", [&args]() -> Value { return RadialGradient(args).tuple(); }},
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
    while (pos < input.size() && (isalnum(input[pos]) || input[pos] == '_')) {
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

    // Check for `identifier :` pattern (identifiers may start with digits, e.g. shade names like 050)
    if (pos >= input.size() || !isalnum(input[pos])) {
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
