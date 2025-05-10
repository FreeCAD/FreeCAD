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

#include "StyleParameters.h"

#include <QColor>
#include <QRegularExpression>
#include <QString>
#include <ranges>
#include <variant>

// helper type for the visitor
template<class... Ts>
struct overloads: Ts...
{
    using Ts::operator()...;
};

namespace Gui::StyleParameters
{

enum class Operator
{
    Add,
    Subtract,
    Multiply,
    Divide
};


class Parser;

struct EvaluationContext
{
    const ParameterManager* manager;
    ParameterManager::ResolveContext context;
};

// Abstract Syntax Tree (AST) Base
struct Expr
{
    virtual Value evaluate(const EvaluationContext& context) const = 0;
    virtual ~Expr() = default;
};

struct ParameterReference: public Expr
{
    std::string name;

    explicit ParameterReference(std::string name)
        : name(std::move(name))
    {}

    Value evaluate(const EvaluationContext& context) const override;
};

struct Number: public Expr
{
    Length value;

    Number(double value, std::string unit)
        : value({value, std::move(unit)})
    {}

    Value evaluate(const EvaluationContext& context) const override
    {
        return value;
    }
};

struct Color: public Expr
{
    QColor color;

    Color(QColor color)
        : color(std::move(color))
    {}

    Value evaluate(const EvaluationContext& context) const override
    {
        return color;
    }
};

struct FunctionCall: public Expr
{
    std::string functionName;
    std::vector<std::unique_ptr<Expr>> arguments;

    FunctionCall(std::string functionName, std::vector<std::unique_ptr<Expr>> arguments)
        : functionName(std::move(functionName))
        , arguments(std::move(arguments))
    {}

    Value evaluate(const EvaluationContext& context) const override
    {
        if (arguments.size() != 2) {
            throw std::runtime_error(fmt::format("Function '{}' expects 2 arguments, got {}",
                                                 functionName,
                                                 arguments.size()));
        }

        auto colorArg = arguments[0]->evaluate(context);
        auto amountArg = arguments[1]->evaluate(context);

        if (!std::holds_alternative<QColor>(colorArg)) {
            throw std::runtime_error(fmt::format("'{}' is not supported for colors", functionName));
        }

        auto color = std::get<QColor>(colorArg);
        auto amount = 100 + std::get<Length>(amountArg).value;

        if (functionName == "lighten") {
            return color.lighter(amount);
        }

        if (functionName == "darken") {
            return color.darker(amount);
        }

        throw new std::runtime_error(fmt::format("Unknown function '{}'", functionName));
    }
};

struct BinaryOp: public Expr
{
    std::unique_ptr<Expr> left, right;
    Operator op;

    BinaryOp(std::unique_ptr<Expr> left, Operator op, std::unique_ptr<Expr> right)
        : left(std::move(left))
        , right(std::move(right))
        , op(op)
    {}

    Value evaluate(const EvaluationContext& context) const override
    {
        Value lval = left->evaluate(context);
        Value rval = right->evaluate(context);

        if (!std::holds_alternative<Length>(lval) || !std::holds_alternative<Length>(rval)) {
            throw std::runtime_error("Math operations are supported only on lengths");
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
                throw std::runtime_error("Unknown operator");
        }
    }
};

struct UnaryOp: public Expr
{
    Operator op;
    std::unique_ptr<Expr> operand;

    UnaryOp(Operator op, std::unique_ptr<Expr> operand)
        : op(op)
        , operand(std::move(operand))
    {}

    Value evaluate(const EvaluationContext& context) const override
    {
        Value val = operand->evaluate(context);
        if (std::holds_alternative<QColor>(val)) {
            throw std::runtime_error("Unary operations on colors are not supported");
        }

        auto v = std::get<Length>(val);
        switch (op) {
            case Operator::Add:
                return v;
            case Operator::Subtract:
                return -v;
            default:
                throw std::runtime_error("Unknown unary operator");
        }
    }
};

class Parser
{
    const std::string& input;
    size_t pos = 0;

public:
    explicit Parser(const std::string& input)
        : input(input)
    {}

    std::unique_ptr<Expr> parse()
    {
        auto expr = parseExpression();
        skipWhitespace();
        if (pos != input.size()) {
            throw std::runtime_error(
                fmt::format("Unexpected characters at end of input: {}", input.substr(pos)));
        }
        return expr;
    }

private:
    std::unique_ptr<Expr> parseExpression()
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

    std::unique_ptr<Expr> parseTerm()
    {
        auto expr = parseFactor();
        while (true) {
            skipWhitespace();
            if (match('*')) {
                expr =
                    std::make_unique<BinaryOp>(std::move(expr), Operator::Multiply, parseFactor());
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

    std::unique_ptr<Expr> parseFactor()
    {
        skipWhitespace();
        if (match('+') || match('-')) {
            Operator op = (input[pos - 1] == '+') ? Operator::Add : Operator::Subtract;
            return std::make_unique<UnaryOp>(op, parseFactor());
        }
        if (match('(')) {
            auto expr = parseExpression();
            if (!match(')')) {
                throw std::runtime_error("Expected ')'");
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

    bool peekColor()
    {
        skipWhitespace();
        // clang-format off
        return input[pos] == '#'
            || input.compare(pos, 4, "rgb(") == 0
            || input.compare(pos, 5, "rgba(") == 0;
        // clang-format on
    }

    std::unique_ptr<Expr> parseColor()
    {
        skipWhitespace();
        if (input[pos] == '#') {
            pos++;
            int r = std::stoi(input.substr(pos, 2), nullptr, 16);
            pos += 2;
            int g = std::stoi(input.substr(pos, 2), nullptr, 16);
            pos += 2;
            int b = std::stoi(input.substr(pos, 2), nullptr, 16);
            pos += 2;
            return std::make_unique<Color>(QColor(r, g, b));
        }
        else if (input.compare(pos, 4, "rgb(") == 0 || input.compare(pos, 5, "rgba(") == 0) {
            bool hasAlpha = input[pos + 3] == 'a';
            pos += hasAlpha ? 5 : 4;
            int r = parseInt();
            if (!match(',')) {
                throw std::runtime_error("Expected ',' after red");
            }
            int g = parseInt();
            if (!match(',')) {
                throw std::runtime_error("Expected ',' after green");
            }
            int b = parseInt();
            int a = 255;
            if (hasAlpha) {
                if (!match(',')) {
                    throw std::runtime_error("Expected ',' after blue");
                }
                a = parseInt();
            }
            if (!match(')')) {
                throw std::runtime_error("Expected ')' after color arguments");
            }
            return std::make_unique<Color>(QColor(r, g, b, a));
        }
        throw std::runtime_error("Unknown color format");
    }


    bool peekParameter()
    {
        skipWhitespace();
        return pos < input.size() && input[pos] == '@';
    }

    std::unique_ptr<Expr> parseParameter()
    {
        skipWhitespace();
        if (!match('@')) {
            throw std::runtime_error("Expected '@' for parameter");
        }
        size_t start = pos;
        while (pos < input.size() && (isalnum(input[pos]) || input[pos] == '_')) {
            ++pos;
        }
        if (start == pos) {
            throw std::runtime_error("Expected parameter name after '@'");
        }
        return std::make_unique<ParameterReference>(input.substr(start, pos - start));
    }

    bool peekFunction()
    {
        skipWhitespace();
        return pos < input.size() && isalpha(input[pos]);
    }

    std::unique_ptr<Expr> parseFunctionCall()
    {
        skipWhitespace();
        size_t start = pos;
        while (pos < input.size() && isalnum(input[pos])) {
            ++pos;
        }
        std::string functionName = input.substr(start, pos - start);

        if (!match('(')) {
            throw std::runtime_error("Expected '(' after function name");
        }

        std::vector<std::unique_ptr<Expr>> arguments;
        if (!match(')')) {
            do {
                arguments.push_back(parseExpression());
            } while (match(','));
            if (!match(')')) {
                throw std::runtime_error("Expected ')' after function arguments");
            }
        }

        return std::make_unique<FunctionCall>(functionName, std::move(arguments));
    }

    int parseInt()
    {
        skipWhitespace();
        size_t start = pos;
        while (pos < input.size() && (isdigit(input[pos]) || input[pos] == '.')) {
            ++pos;
        }
        return std::stoi(input.substr(start, pos - start));
    }

    std::unique_ptr<Expr> parseNumber()
    {
        skipWhitespace();
        size_t start = pos;
        while (pos < input.size() && (isdigit(input[pos]) || input[pos] == '.')) {
            ++pos;
        }
        double value = std::stod(input.substr(start, pos - start));
        std::string unit = parseUnit();
        return std::make_unique<Number>(value, unit);
    }

    std::string parseUnit()
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

    bool match(char expected)
    {
        skipWhitespace();
        if (pos < input.size() && input[pos] == expected) {
            ++pos;
            return true;
        }
        return false;
    }

    void skipWhitespace()
    {
        while (pos < input.size() && isspace(input[pos])) {
            ++pos;
        }
    }
};

Value ParameterReference::evaluate(const EvaluationContext& context) const
{
    return context.manager->resolve(name);
}

Length Length::operator+(const Length& rhs) const
{
    ensureEqualUnits(rhs);
    return {value + rhs.value, unit};
}

Length Length::operator-(const Length& rhs) const
{
    ensureEqualUnits(rhs);
    return {value - rhs.value, unit};
}

Length Length::operator-() const
{
    return {-value, unit};
}

Length Length::operator/(const Length& rhs) const
{
    if (rhs.value == 0) {
        throw std::runtime_error("Division by zero");
    }

    if (rhs.unit.empty() || unit.empty()) {
        return {value / rhs.value, unit};
    }

    ensureEqualUnits(rhs);
    return {value / rhs.value, unit};
}

Length Length::operator*(const Length& rhs) const
{
    if (rhs.unit.empty() || unit.empty()) {
        return {value * rhs.value, unit};
    }

    ensureEqualUnits(rhs);
    return {value * rhs.value, unit};
}

void Length::ensureEqualUnits(const Length& rhs) const
{
    if (unit != rhs.unit) {
        throw std::runtime_error(
            fmt::format("Units mismatch left expression is '{}', right expression is '{}'",
                        unit,
                        rhs.unit));
    }
}
std::string Value::toString() const
{
    if (std::holds_alternative<StyleParameters::Length>(*this)) {
        auto [value, unit] = std::get<StyleParameters::Length>(*this);
        return fmt::format("{}{}", value, unit);
    }

    if (std::holds_alternative<QColor>(*this)) {
        auto color = std::get<QColor>(*this);
        return fmt::format("#{:0>6x}", 0xFFFFFF & color.rgb());
    }

    return std::get<std::string>(*this);
}

ParameterManager::ParameterManager()
{
    reload();
}

void ParameterManager::reload()
{
    _parameters = {};

    const auto addUserColor = [this](const std::string& name, unsigned long fallback) {
        unsigned long color = hGrp->GetUnsigned(name.c_str(), fallback);

        _parameters[name] = {
            .name = name,
            .value = fmt::format("#{:0>6x}", 0x00FFFFFF & (color >> 8)),
            .source = ParameterSource::Predefined,
        };
    };

    for (const auto& [token, value] : hUserTokensGrp->GetASCIIMap()) {
        _parameters[token] = {
            .name = token,
            .value = value,
            .source = ParameterSource::User,
        };
    }

    for (const auto& [token, value] : hThemeTokensGrp->GetASCIIMap()) {
        _parameters[token] = {
            .name = token,
            .value = value,
            .source = ParameterSource::Theme,
        };
    }

    addUserColor("ThemeAccentColor1", 0);
    addUserColor("ThemeAccentColor2", 0);
    addUserColor("ThemeAccentColor3", 0);
}

std::string ParameterManager::replacePlaceholders(const std::string& expression,
                                                  ResolveContext context) const
{
    static const QRegularExpression regex(QStringLiteral("@(\\w+)"));

    auto substituteWithCallback =
        [](const QRegularExpression& regex,
           const QString& input,
           std::function<QString(const QRegularExpressionMatch&)> callback) {
            QRegularExpressionMatchIterator it = regex.globalMatch(input);

            QString result;
            qsizetype lastIndex = 0;

            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();

                qsizetype start = match.capturedStart();
                qsizetype end = match.capturedEnd();

                result += input.mid(lastIndex, start - lastIndex);
                result += callback(match);

                lastIndex = end;
            }

            // Append any remaining text after the last match
            result += input.mid(lastIndex);

            return result;
        };

    // clang-format off
    return substituteWithCallback(
        regex,
        QString::fromStdString(expression),
        [&](const QRegularExpressionMatch& match) {
            auto tokenName = match.captured(1).toStdString();

            auto tokenValue = resolve(tokenName, context);
            context.visited.erase(tokenName);

            return QString::fromStdString(tokenValue.toString());
        }
    ).toStdString();
    // clang-format on
}

std::string ParameterManager::expression(const std::string& name) const
{
    return _parameters.at(name).value;
}

Value ParameterManager::resolve(const std::string& name, ResolveContext context) const
{
    if (!_parameters.contains(name)) {
        Base::Console().warning("Requested non-existent design token '%s'.", name);
        return std::string {};
    }

    if (context.visited.contains(name)) {
        Base::Console().warning("The design token '%s' contains circular-reference.", name);
        return expression(name);
    }

    const Parameter& token = _parameters.at(name);

    if (!_resolved.contains(token.name)) {
        context.visited.insert(token.name);
        try {
            _resolved[token.name] = evaluate(token.value, context);
        }
        catch (std::exception&) {
            // in case of being unable to parse it, we need to treat it as a generic value
            _resolved[token.name] = replacePlaceholders(token.value, context);
        }
        context.visited.erase(token.name);
    }

    return _resolved[token.name];
}

Value ParameterManager::evaluate(const std::string& expression, ResolveContext context) const
{
    Parser parser(expression);
    return parser.parse()->evaluate({.manager = this, .context = context});
}

Parameter ParameterManager::parameter(const std::string& parameter) const
{
    return _parameters.at(parameter);
}

void ParameterManager::remove(const Parameter& parameter)
{
    switch (parameter.source) {
        case ParameterSource::Theme:
            hThemeTokensGrp->RemoveASCII(parameter.name.c_str());
            break;
        case ParameterSource::User:
            hUserTokensGrp->RemoveASCII(parameter.name.c_str());
            break;
    }

    _parameters.erase(parameter.name);
    _resolved = {};
}

void ParameterManager::define(const Parameter& parameter)
{
    switch (parameter.source) {
        case ParameterSource::Theme:
            hThemeTokensGrp->SetASCII(parameter.name.c_str(), parameter.value.c_str());
            break;
        case ParameterSource::User:
            hUserTokensGrp->SetASCII(parameter.name.c_str(), parameter.value.c_str());
            break;
    }

    _parameters[parameter.name] = parameter;
    _resolved = {};
}

}  // namespace Gui::StyleParameter
