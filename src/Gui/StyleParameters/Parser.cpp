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
#include "ColorShading.h"
#include "Corners.h"
#include "Gradient.h"
#include "Insets.h"
#include "ParameterManager.h"

#include <Utilities.h>
#include <Base/OkLch.h>
#include <Base/Tools.h>

#include <QColor>
#include <algorithm>
#include <cctype>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace Gui::StyleParameters
{

namespace
{
/// Returns the named argument of type T, throwing Base::ExpressionError when it is not.
template<typename T>
const T& requireArgument(const Tuple& args, const std::string& name, const char* function)
{
    if (const T* value = args.tryGet<T>(name)) {
        return *value;
    }

    const Value* found = args.find(name);
    THROWM(
        Base::ExpressionError,
        fmt::format(
            "{}: '{}' argument must be {}, got {}",
            function,
            name,
            valueTypeName<T>(),
            found ? found->toString() : "nothing"
        )
    );
}

bool isDigitChar(char character)
{
    return std::isdigit(static_cast<unsigned char>(character)) != 0;
}

bool isAlphaChar(char character)
{
    return std::isalpha(static_cast<unsigned char>(character)) != 0;
}

bool isAlnumChar(char character)
{
    return std::isalnum(static_cast<unsigned char>(character)) != 0;
}

bool isSpaceChar(char character)
{
    return std::isspace(static_cast<unsigned char>(character)) != 0;
}

int parseIntOrThrow(const std::string& text, int base = 10)  // NOLINT(*-magic-numbers)
{
    try {
        return std::stoi(text, nullptr, base);
    }
    catch (const std::invalid_argument&) {
        THROWM(Base::ParserError, fmt::format("Invalid integer: {}", text));
    }
    catch (const std::out_of_range&) {
        THROWM(Base::ParserError, fmt::format("Integer out of range: {}", text));
    }
}

double parseDoubleOrThrow(const std::string& text)
{
    try {
        return std::stod(text);
    }
    catch (const std::invalid_argument&) {
        THROWM(Base::ParserError, fmt::format("Invalid number: {}", text));
    }
    catch (const std::out_of_range&) {
        THROWM(Base::ParserError, fmt::format("Number out of range: {}", text));
    }
}

std::optional<size_t> parseIndex(const std::string& text)
{
    try {
        return std::stoul(text);
    }
    catch (const std::exception&) {
        return std::nullopt;
    }
}

/// Shrinks a (width, height) size by one or more insets.
Value contentBox(const Tuple& args)
{
    if (args.size() < 2) {
        THROWM(
            Base::ExpressionError,
            "content_box requires at least 2 arguments: a size tuple and at least one inset"
        );
    }

    const Value& sizeValue = args.at(0);
    Numeric width, height;

    if (sizeValue.holds<Tuple>()) {
        const auto& sizeTuple = sizeValue.get<Tuple>();
        // The Base::TypeError branch below only guards the outer shape of the first argument
        // (tuple vs. numeric vs. neither); it says nothing about what's inside the tuple, so
        // "width"/"height" of the wrong type still need their own check.
        width = requireArgument<Numeric>(sizeTuple, "width", "content_box");
        height = requireArgument<Numeric>(sizeTuple, "height", "content_box");
    }
    else if (sizeValue.holds<Numeric>()) {
        width = sizeValue.get<Numeric>();
        height = sizeValue.get<Numeric>();
    }
    else {
        THROWM(
            Base::TypeError,
            "content_box: first argument must be a (width, height) size tuple or a Numeric"
        );
    }

    for (size_t index = 1; index < args.size(); ++index) {
        const Insets insets(args.at(index));
        width = width - insets.horizontal();
        height = height - insets.vertical();
    }

    return Tuple({
        Tuple::Element::named("width", width),
        Tuple::Element::named("height", height),
    });
}

/// True when a value is a valid linear or radial gradient, going through the same tryFrom
/// validation mapGradientStops uses. Lets callers with more than one gradient argument (e.g.
/// blend) decide between arguments before committing to a mapping.
bool isGradientValue(const Value& value)
{
    return LinearGradient::tryFrom(value).has_value() || RadialGradient::tryFrom(value).has_value();
}

/// Converts a Numeric to a plain fraction, treating a "%" unit as out-of-100 and any other
/// unit as if it were dimensionless.
float asPercent(const Numeric& numeric)
{
    return static_cast<float>(numeric.asFraction().value_or(numeric.value));
}

// Each of these has an ArgumentParser default, so a slot is always present; requireArgument
// only ever throws here when the theme author supplied an explicit, wrongly-typed value —
// a defaulted slot is always a Numeric and passes through untouched.
ColorShading::Parameters parseShadingParams(const Tuple& resolved, const char* function)
{
    return ColorShading::Parameters {
        .range = asPercent(requireArgument<Numeric>(resolved, "range", function)),
        .minLightness = asPercent(requireArgument<Numeric>(resolved, "min", function)),
        .maxLightness = asPercent(requireArgument<Numeric>(resolved, "max", function)),
        .pivot = asPercent(requireArgument<Numeric>(resolved, "pivot", function)),
        .chromaExponent = asPercent(requireArgument<Numeric>(resolved, "q", function)),
    };
}

Base::Color applyShade(
    float position,
    const Base::Color& color,
    const Base::OkLch& oklch,
    const ColorShading::Parameters& shadingParams
)
{
    auto shadeOklch = ColorShading::computeShade(position, oklch, shadingParams);
    if (shadeOklch.lightness == oklch.lightness && shadeOklch.chroma == oklch.chroma) {
        return color;
    }
    return Base::fromOkLch(shadeOklch, color.a);
}

/// Shared implementation of `lighten`/`darken`, distinguished only by the Qt lighter/darker
/// call and the function name used in diagnostics.
Value lightenOrDarken(const Tuple& args, bool lighten)
{
    const char* functionName = lighten ? "lighten" : "darken";

    auto resolved = ArgumentParser {{"color"}, {"amount"}}.resolve(args);

    // In Qt if you want to make color 20% darker or lighter, you need to pass 120 as the value
    // we, however, want users to pass only the relative difference, hence we need to add the
    // 100 required by Qt.
    //
    // NOLINTNEXTLINE(*-magic-numbers)
    auto amount = 100
        + static_cast<int>(requireArgument<Numeric>(resolved, "amount", functionName).value);

    const auto applyToColor = [&](const Base::Color& color) -> Base::Color {
        auto qcolor = color.asValue<QColor>();
        if (lighten) {
            return Base::Color::fromValue(qcolor.lighter(amount));
        }
        return Base::Color::fromValue(qcolor.darker(amount));
    };

    const Value* colorValue = resolved.find("color");
    if (colorValue->holds<Tuple>()) {
        auto mapped = mapGradientStops(*colorValue, applyToColor);
        if (!mapped) {
            THROWM(
                Base::ExpressionError,
                fmt::format("{}: color argument must be a color or gradient", functionName)
            );
        }
        return Value {std::move(*mapped)};
    }

    return applyToColor(requireArgument<Base::Color>(resolved, "color", functionName));
}

Value lighten(const Tuple& args)
{
    return lightenOrDarken(args, true);
}

Value darken(const Tuple& args)
{
    return lightenOrDarken(args, false);
}

Value blend(const Tuple& args)
{
    auto resolved = ArgumentParser {{"from"}, {"to"}, {"amount"}}.resolve(args);

    auto amount = Base::fromPercent(
        static_cast<long>(requireArgument<Numeric>(resolved, "amount", "blend").value)
    );

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

    bool fromIsGradient = fromValue->holds<Tuple>() && isGradientValue(*fromValue);
    bool toIsGradient = toValue->holds<Tuple>() && isGradientValue(*toValue);

    if (fromValue->holds<Tuple>() && !fromIsGradient) {
        THROWM(Base::ExpressionError, "blend: 'from' argument must be a color or gradient");
    }
    if (toValue->holds<Tuple>() && !toIsGradient) {
        THROWM(Base::ExpressionError, "blend: 'to' argument must be a color or gradient");
    }

    if (fromIsGradient && toIsGradient) {
        THROWM(Base::ExpressionError, "Cannot blend two gradients");
    }

    if (fromIsGradient) {
        const auto& targetColor = requireArgument<Base::Color>(resolved, "to", "blend");
        auto mapped = mapGradientStops(*fromValue, [&](const Base::Color& stopColor) {
            return blendColors(stopColor, targetColor);
        });
        return Value {std::move(*mapped)};
    }

    if (toIsGradient) {
        const auto& sourceColor = requireArgument<Base::Color>(resolved, "from", "blend");
        auto mapped = mapGradientStops(*toValue, [&](const Base::Color& stopColor) {
            return blendColors(sourceColor, stopColor);
        });
        return Value {std::move(*mapped)};
    }

    if (!fromValue->holds<Base::Color>()) {
        THROWM(Base::ExpressionError, "Expected color as from argument");
    }

    if (!toValue->holds<Base::Color>()) {
        THROWM(Base::ExpressionError, "Expected color as to argument");
    }

    return blendColors(fromValue->get<Base::Color>(), toValue->get<Base::Color>());
}

Value shade(const Tuple& args)
{
    auto resolved = ArgumentParser {
        {.name = "color"},
        {.name = "lightness"},
        {.name = "range", .defaultValue = Numeric {0.8, ""}},
        {.name = "min", .defaultValue = Numeric {0.17, ""}},
        {.name = "max", .defaultValue = Numeric {0.97, ""}},
        {.name = "pivot", .defaultValue = Numeric {0.5, ""}},
        {.name = "q", .defaultValue = Numeric {0.1, ""}},
    }.resolve(args);

    auto position = asPercent(requireArgument<Numeric>(resolved, "lightness", "shade"));
    auto shadingParams = parseShadingParams(resolved, "shade");

    const auto applyToColor = [&](const Base::Color& color) -> Base::Color {
        auto oklch = Base::toOkLch(color);
        return applyShade(position, color, oklch, shadingParams);
    };

    const Value* colorValue = resolved.find("color");
    if (colorValue->holds<Tuple>()) {
        auto mapped = mapGradientStops(*colorValue, applyToColor);
        if (!mapped) {
            THROWM(Base::ExpressionError, "shade: color argument must be a color or gradient");
        }
        return Value {std::move(*mapped)};
    }

    return applyToColor(requireArgument<Base::Color>(resolved, "color", "shade"));
}

Value shades(const Tuple& args)
{
    auto resolved = ArgumentParser {
        {.name = "color"},
        {.name = "shades"},
        {.name = "range", .defaultValue = Numeric {0.8, ""}},
        {.name = "min", .defaultValue = Numeric {0.17, ""}},
        {.name = "max", .defaultValue = Numeric {0.97, ""}},
        {.name = "pivot", .defaultValue = Numeric {0.5, ""}},
        {.name = "q", .defaultValue = Numeric {0.1, ""}},
    }.resolve(args);

    const auto& shadesSpec = requireArgument<Tuple>(resolved, "shades", "shades");
    auto shadingParams = parseShadingParams(resolved, "shades");

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
        if (!isGradientValue(*colorValue)) {
            THROWM(Base::ExpressionError, "shades: color argument must be a color or gradient");
        }

        Tuple result;
        for (const auto& element : shadesSpec.elements) {
            float position = asPercent(element.value->get<Numeric>());
            auto shadedGradient
                = mapGradientStops(*colorValue, [&](const Base::Color& stopColor) -> Base::Color {
                      auto oklch = Base::toOkLch(stopColor);
                      return applyShade(position, stopColor, oklch, shadingParams);
                  });
            appendElement(result, element, std::move(*shadedGradient));
        }
        return result;
    }

    const auto& baseColor = requireArgument<Base::Color>(resolved, "color", "shades");
    auto baseOklch = Base::toOkLch(baseColor);

    Tuple result;
    for (const auto& element : shadesSpec.elements) {
        float position = asPercent(element.value->get<Numeric>());
        auto shadeColor = applyShade(position, baseColor, baseOklch, shadingParams);
        appendElement(result, element, shadeColor);
    }
    return result;
}

using StyleFunction = Value (*)(const Tuple&);

/// Table of all style functions taking a single Tuple argument, built once and reused —
/// see FunctionCall::evaluate for the "coalesce" special case that bypasses it.
const std::unordered_map<std::string_view, StyleFunction>& styleFunctions()
{
    static const std::unordered_map<std::string_view, StyleFunction> table = {
        {"lighten", lighten},
        {"darken", darken},
        {"blend", blend},
        {"shade", shade},
        {"shades", shades},
        {"content_box", contentBox},
        {"padding", [](const Tuple& args) -> Value { return Padding(args).tuple(); }},
        {"margins", [](const Tuple& args) -> Value { return Margins(args).tuple(); }},
        {"border_colors", [](const Tuple& args) -> Value { return BorderColors(args).tuple(); }},
        {"border_thickness", [](const Tuple& args) -> Value { return BorderThickness(args).tuple(); }},
        {"border_radius", [](const Tuple& args) -> Value { return Corners(args).tuple(); }},
        {"linear_gradient", [](const Tuple& args) -> Value { return LinearGradient(args).tuple(); }},
        {"radial_gradient", [](const Tuple& args) -> Value { return RadialGradient(args).tuple(); }},
    };
    return table;
}

}  // namespace

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
    if (functionName == "coalesce") {
        if (arguments.elements.empty()) {
            THROWM(Base::ExpressionError, "coalesce requires at least one argument");
        }
        for (const auto& element : arguments.elements) {
            Value result = element.expression->evaluate(context);
            if (!result.holds<std::string>() || !result.get<std::string>().starts_with("@")) {
                return result;
            }
        }
        return arguments.elements.back().expression->evaluate(context);
    }

    auto argsValue = arguments.evaluate(context);
    const auto& args = argsValue.get<Tuple>();

    const auto& table = styleFunctions();
    if (auto entry = table.find(functionName); entry != table.end()) {
        return entry->second(args);
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

    if (std::ranges::all_of(member, isDigitChar)) {
        const std::optional<size_t> index = parseIndex(member);
        const Value* element = index ? tuple.tryAt(*index) : nullptr;

        if (!element) {
            THROWM(Base::ExpressionError, fmt::format("Tuple has no element at index '{}'", member));
        }

        return *element;
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
        constexpr size_t hexDigitCount = 6;

        const size_t start = pos;

        // Format is #RRGGBB
        pos++;
        if (input.size() - pos < hexDigitCount) {
            THROWM(
                Base::ParserError,
                fmt::format("Invalid hexadecimal color, expected #RRGGBB, got '{}'", input.substr(start))
            );
        }

        int r = parseIntOrThrow(input.substr(pos, 2), hexadecimalBase);
        pos += 2;
        int g = parseIntOrThrow(input.substr(pos, 2), hexadecimalBase);
        pos += 2;
        int b = parseIntOrThrow(input.substr(pos, 2), hexadecimalBase);
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

    if (input[pos] == '#') {
        return parseHexadecimalColor();
    }

    if (peekString(rgbFunction) || peekString(rgbaFunction)) {
        return parseFunctionStyleColor();
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
    while (pos < input.size() && (isAlnumChar(input[pos]) || input[pos] == '_')) {
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
    return pos < input.size() && isAlphaChar(input[pos]);
}

std::unique_ptr<Expr> Parser::parseFunctionCall()
{
    skipWhitespace();
    size_t start = pos;
    while (pos < input.size() && (isAlnumChar(input[pos]) || input[pos] == '_')) {
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
    if (pos >= input.size() || !isAlnumChar(input[pos])) {
        pos = saved;
        return false;
    }

    // Skip identifier characters
    while (pos < input.size() && (isAlnumChar(input[pos]) || input[pos] == '_')) {
        ++pos;
    }

    // Skip whitespace between identifier and colon
    while (pos < input.size() && isSpaceChar(input[pos])) {
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
            while (pos < input.size() && (isAlnumChar(input[pos]) || input[pos] == '_')) {
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
    while (pos < input.size() && (isDigitChar(input[pos]) || input[pos] == '.')) {
        ++pos;
    }
    return parseIntOrThrow(input.substr(start, pos - start));
}

std::unique_ptr<Expr> Parser::parseNumber()
{
    skipWhitespace();
    size_t start = pos;
    while (pos < input.size() && (isDigitChar(input[pos]) || input[pos] == '.')) {
        ++pos;
    }

    const double value = parseDoubleOrThrow(input.substr(start, pos - start));
    return std::make_unique<Number>(value, parseUnit());
}

std::string Parser::parseUnit()
{
    skipWhitespace();
    size_t start = pos;
    while (pos < input.size() && (isAlphaChar(input[pos]) || input[pos] == '%')) {
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
    while (pos < input.size() && (isAlnumChar(input[pos]) || input[pos] == '_')) {
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
    while (pos < input.size() && isSpaceChar(input[pos])) {
        ++pos;
    }
}

}  // namespace Gui::StyleParameters
