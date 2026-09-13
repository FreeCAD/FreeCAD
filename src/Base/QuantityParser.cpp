// SPDX-License-Identifier: LGPL-2.1-or-later

#include "QuantityParser.h"

#include "Exception.h"
#include "Quantity.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

using Base::ParserError;
using Base::Quantity;

namespace
{
enum class TokenKind
{
    End,
    Number,
    Unit,
    Name,
    Plus,
    Minus,
    Multiply,
    Divide,
    Power,
    LeftParen,
    RightParen
};

struct Token
{
    TokenKind kind;
    double number {};
    std::optional<Quantity> unit;
    std::string text;
};

std::vector<Token> lexQuantity(std::string_view input)
{
    std::vector<Token> tokens;
    std::size_t cursor = 0;
    while (cursor < input.size()) {
        const auto ch = static_cast<unsigned char>(input[cursor]);
        if (std::isspace(ch)) {
            ++cursor;
            continue;
        }
        if (ch == '[') {
            const auto end = input.find(']', cursor + 1);
            cursor = end == std::string_view::npos ? input.size() : end + 1;
            continue;
        }
        if (input.substr(cursor, 3) == "\xE2\x88\x92") {
            tokens.push_back({TokenKind::Minus, 0, std::nullopt, {}});
            cursor += 3;
            continue;
        }
        TokenKind punctuation;
        bool isPunctuation = true;
        switch (ch) {
            case '+':
                punctuation = TokenKind::Plus;
                break;
            case '-':
                punctuation = TokenKind::Minus;
                break;
            case '*':
                punctuation = TokenKind::Multiply;
                break;
            case '/':
                punctuation = TokenKind::Divide;
                break;
            case '^':
                punctuation = TokenKind::Power;
                break;
            case '(':
                punctuation = TokenKind::LeftParen;
                break;
            case ')':
                punctuation = TokenKind::RightParen;
                break;
            default:
                isPunctuation = false;
        }
        if (isPunctuation) {
            tokens.push_back({punctuation, 0, std::nullopt, {}});
            ++cursor;
            continue;
        }
        if (std::isdigit(ch) || ch == '.' || ch == ',') {
            const auto begin = cursor;
            bool separator = false;
            while (cursor < input.size() && std::isdigit(static_cast<unsigned char>(input[cursor]))) {
                ++cursor;
            }
            if (cursor < input.size() && (input[cursor] == '.' || input[cursor] == ',')) {
                separator = true;
                ++cursor;
                while (cursor < input.size()
                       && std::isdigit(static_cast<unsigned char>(input[cursor]))) {
                    ++cursor;
                }
            }
            if (cursor < input.size() && (input[cursor] == 'e' || input[cursor] == 'E')) {
                const auto exponent = cursor++;
                if (cursor < input.size() && (input[cursor] == '+' || input[cursor] == '-')) {
                    ++cursor;
                }
                const auto digits = cursor;
                while (cursor < input.size()
                       && std::isdigit(static_cast<unsigned char>(input[cursor]))) {
                    ++cursor;
                }
                if (cursor == digits) {
                    cursor = exponent;
                }
            }
            auto text = std::string(input.substr(begin, cursor - begin));
            if (!separator && text.empty()) {
                throw ParserError("Invalid number");
            }
            std::replace(text.begin(), text.end(), ',', '.');
            char* end = nullptr;
            const auto value = std::strtod(text.c_str(), &end);
            if (end != text.c_str() + text.size()) {
                throw ParserError("Invalid number");
            }
            tokens.push_back({TokenKind::Number, value, std::nullopt, {}});
            continue;
        }
        std::string text;
        if (input.substr(cursor, 5) == "log10") {
            text = "log10";
            cursor += 5;
        }
        else {
            const auto begin = cursor;
            while (cursor < input.size()) {
                const auto current = static_cast<unsigned char>(input[cursor]);
                if (std::isspace(current)
                    || std::string_view("+-*/^()[]").find(current) != std::string_view::npos) {
                    break;
                }
                if (cursor > begin && std::isdigit(current)) {
                    break;
                }
                ++cursor;
            }
            text = std::string(input.substr(begin, cursor - begin));
        }
        if (auto unit = Quantity::lookupUnit(text)) {
            tokens.push_back({TokenKind::Unit, 0, std::move(unit), std::move(text)});
        }
        else {
            tokens.push_back({TokenKind::Name, 0, std::nullopt, std::move(text)});
        }
    }
    tokens.push_back({TokenKind::End, 0, std::nullopt, {}});
    return tokens;
}

class QuantityParser
{
public:
    explicit QuantityParser(std::string_view input)
        : input(input)
        , tokens(lexQuantity(input))
    {}

    Quantity parse()
    {
        if (peek().kind == TokenKind::End) {
            return Quantity(std::numeric_limits<double>::min());
        }
        if (auto value = tryParse([this] { return parseQuantities(); })) {
            return *value;
        }
        if (auto value = tryParse([this] { return Quantity(parseNumber()); })) {
            return *value;
        }
        if (auto value = tryParse([this] { return parseUnit(); })) {
            return *value;
        }
        throw ParserError(fmt::format("Invalid quantity expression: {}", input));
    }

private:
    template<typename Function>
    std::optional<Quantity> tryParse(Function function)
    {
        const auto saved = position;
        try {
            auto result = function();
            if (peek().kind != TokenKind::End) {
                throw ParserError("Unexpected token");
            }
            return result;
        }
        catch (const ParserError&) {
            position = saved;
            return std::nullopt;
        }
    }

    Quantity parseQuantities()
    {
        auto result = parseQuantity();
        for (int count = 1; peek().kind != TokenKind::End; ++count) {
            if (count == 3) {
                throw ParserError("Too many adjacent quantities");
            }
            result += parseQuantity();
        }
        return result;
    }

    Quantity parseQuantity()
    {
        const auto number = parseNumber();
        if (accept(TokenKind::Divide)) {
            return Quantity(number) / parseUnit();
        }
        if (!startsUnit()) {
            throw ParserError("Expected unit");
        }
        return Quantity(number) * parseUnit();
    }

    double parseNumber(int minimumBindingPower = 0)
    {
        double left;
        if (accept(TokenKind::Plus)) {
            left = parseNumber(30);
        }
        else if (accept(TokenKind::Minus)) {
            left = -parseNumber(30);
        }
        else if (accept(TokenKind::LeftParen)) {
            left = parseNumber();
            expect(TokenKind::RightParen);
        }
        else if (peek().kind == TokenKind::Number) {
            left = consume().number;
        }
        else if (peek().kind == TokenKind::Name) {
            const auto function = consume().text;
            if (function == "pi") {
                left = std::numbers::pi;
            }
            else if (function == "e") {
                left = std::numbers::e;
            }
            else {
                expect(TokenKind::LeftParen);
                const auto argument = parseNumber();
                expect(TokenKind::RightParen);
                if (function == "acos") {
                    left = std::acos(argument);
                }
                else if (function == "asin") {
                    left = std::asin(argument);
                }
                else if (function == "atan") {
                    left = std::atan(argument);
                }
                else if (function == "abs") {
                    left = std::fabs(argument);
                }
                else if (function == "exp") {
                    left = std::exp(argument);
                }
                else if (function == "log") {
                    left = std::log(argument);
                }
                else if (function == "log10") {
                    left = std::log10(argument);
                }
                else if (function == "sin") {
                    left = std::sin(argument);
                }
                else if (function == "sinh") {
                    left = std::sinh(argument);
                }
                else if (function == "tan") {
                    left = std::tan(argument);
                }
                else if (function == "tanh") {
                    left = std::tanh(argument);
                }
                else if (function == "sqrt") {
                    left = std::sqrt(argument);
                }
                else if (function == "cos") {
                    left = std::cos(argument);
                }
                else {
                    throw ParserError("Unknown numeric function");
                }
            }
        }
        else {
            throw ParserError("Expected number");
        }

        while (true) {
            const auto kind = peek().kind;
            const int leftPower = kind == TokenKind::Plus || kind == TokenKind::Minus ? 10
                : kind == TokenKind::Multiply || kind == TokenKind::Divide            ? 20
                : kind == TokenKind::Power                                            ? 40
                                                                                      : -1;
            if (leftPower < minimumBindingPower) {
                break;
            }
            if ((kind == TokenKind::Multiply || kind == TokenKind::Divide)
                && startsUnitExpression(1)) {
                break;
            }
            consume();
            const auto right = parseNumber(kind == TokenKind::Power ? leftPower : leftPower + 1);
            if (kind == TokenKind::Plus) {
                left += right;
            }
            else if (kind == TokenKind::Minus) {
                left -= right;
            }
            else if (kind == TokenKind::Multiply) {
                left *= right;
            }
            else if (kind == TokenKind::Divide) {
                left /= right;
            }
            else {
                left = std::pow(left, right);
            }
        }
        return left;
    }

    Quantity parseUnit(int minimumBindingPower = 0)
    {
        Quantity left;
        if (peek().kind == TokenKind::Unit) {
            left = *consume().unit;
        }
        else if (
            peek().kind == TokenKind::Number && peek().number == 1.0 && peek(1).kind == TokenKind::Divide
        ) {
            consume();
            consume();
            left = Quantity(1.0) / parseUnit(1);
        }
        else if (accept(TokenKind::LeftParen)) {
            left = parseUnit();
            expect(TokenKind::RightParen);
        }
        else {
            throw ParserError("Expected unit");
        }

        while (true) {
            const auto kind = peek().kind;
            const int power = kind == TokenKind::Multiply || kind == TokenKind::Divide ? 10
                : kind == TokenKind::Power                                             ? 20
                                                                                       : -1;
            if (power < minimumBindingPower) {
                break;
            }
            consume();
            if (kind == TokenKind::Power) {
                // Lower-precedence arithmetic terminates a unit exponent.
                // Parentheses deliberately reset the numeric binding power.
                left = left.pow(parseNumber(40));
            }
            else {
                const auto right = parseUnit(power + 1);
                left = kind == TokenKind::Multiply ? left * right : left / right;
            }
        }
        return left;
    }

    bool startsUnit(std::size_t offset = 0) const
    {
        return peek(offset).kind == TokenKind::Unit || peek(offset).kind == TokenKind::LeftParen
            || (peek(offset).kind == TokenKind::Number && peek(offset).number == 1.0
                && peek(offset + 1).kind == TokenKind::Divide);
    }
    bool startsUnitExpression(std::size_t offset)
    {
        const auto saved = position;
        position = std::min(position + offset, tokens.size() - 1);
        try {
            parseUnit();
            position = saved;
            return true;
        }
        catch (const ParserError&) {
            position = saved;
            return false;
        }
    }
    bool accept(TokenKind kind)
    {
        if (peek().kind != kind) {
            return false;
        }
        ++position;
        return true;
    }
    void expect(TokenKind kind)
    {
        if (!accept(kind)) {
            throw ParserError("Unexpected token");
        }
    }
    const Token& consume()
    {
        return tokens[position++];
    }
    const Token& peek(std::size_t offset = 0) const
    {
        return tokens[std::min(position + offset, tokens.size() - 1)];
    }

    std::string_view input;
    std::vector<Token> tokens;
    std::size_t position {};
};
}  // namespace

namespace Base::QuantityParserSupport
{
Quantity parse(std::string_view input)
{
    return ::QuantityParser(input).parse();
}
}  // namespace Base::QuantityParserSupport
