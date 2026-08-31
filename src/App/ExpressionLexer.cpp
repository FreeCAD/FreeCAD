// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ExpressionLexer.h"

#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <numbers>

#include <fmt/format.h>

namespace App::ExpressionParser::Pratt
{
namespace
{
std::string decodeString(const std::string& input)
{
    std::string output;
    bool escaped = false;
    for (auto cursor = input.begin() + 2; cursor != input.end() - 2; ++cursor) {
        if (escaped) {
            switch (*cursor) {
                case 't': output += '\t'; break;
                case 'n': output += '\n'; break;
                case 'r': output += '\r'; break;
                case '\\': output += '\\'; break;
                case '\'': output += '\''; break;
                case '"': output += '"'; break;
            }
            escaped = false;
        }
        else if (*cursor == '\\') {
            escaped = true;
        }
        else {
            output += *cursor;
        }
    }
    return output;
}

bool isCellAddress(const std::string& text)
{
    std::size_t cursor = !text.empty() && text.front() == '$' ? 1 : 0;
    const auto letters = cursor;
    while (cursor < text.size()
           && std::isalpha(static_cast<unsigned char>(text[cursor]))) {
        ++cursor;
    }
    if (cursor == letters || cursor - letters > 2) {
        return false;
    }
    if (cursor < text.size() && text[cursor] == '$') {
        ++cursor;
    }
    const auto digits = cursor;
    while (cursor < text.size()
           && std::isdigit(static_cast<unsigned char>(text[cursor]))) {
        ++cursor;
    }
    return cursor == text.size() && cursor > digits;
}

class Lexer
{
public:
    Lexer(const char* buffer, const FunctionLookup& lookupFunction, bool tolerant = false)
        : input(buffer)
        , lookupFunction(lookupFunction)
        , tolerant(tolerant)
    {}

    std::vector<Token> scan()
    {
        while (cursor < input.size()) {
            try {
                scanOne();
            }
            catch (const Base::ParserError&) {
                if (!tolerant) {
                    throw;
                }
                break;
            }
        }
        tokens.push_back(Token {TokenKind::End, {}, {}, input.size(), std::nullopt});
        return tokens;
    }

private:
    void push(TokenKind kind, std::string lexeme, std::size_t column)
    {
        tokens.push_back(Token {kind, {}, std::move(lexeme), column, std::nullopt});
    }

    void scanOne()
    {
        const auto column = cursor;
        const auto current = static_cast<unsigned char>(input[cursor]);
        if (std::isspace(current)) {
            ++cursor;
            return;
        }
        if (input.compare(cursor, 2, "<<") == 0) {
            scanString(column);
            return;
        }
        if (input.compare(cursor, 3, "\xE2\x88\x92") == 0) {
            push(TokenKind::Minus, input.substr(cursor, 3), column);
            cursor += 3;
            return;
        }
        if (current == '\'' || current == '"') {
            const auto quantity = current == '\'' ? Base::Quantity::Foot : Base::Quantity::Inch;
            tokens.push_back(Token {TokenKind::UsBuildingUnit,
                                    quantity,
                                    input.substr(cursor++, 1),
                                    column,
                                    std::nullopt});
            return;
        }
        if (scanComparison(column) || scanNumber(column)) {
            return;
        }
        if (const auto kind = punctuation(current)) {
            push(*kind, input.substr(cursor++, 1), column);
            return;
        }
        if (std::isalpha(current) || current == '_' || current == '$' || current >= 0x80) {
            scanName(column);
            return;
        }
        throw Base::ParserError(fmt::format("Unsupported character at column {}", column));
    }

    void scanString(std::size_t column)
    {
        auto end = cursor + 2;
        bool escaped = false;
        while (end < input.size()) {
            if (!escaped && end + 1 < input.size() && input[end] == '>'
                && input[end + 1] == '>') {
                end += 2;
                auto lexeme = input.substr(cursor, end - cursor);
                tokens.push_back(Token {TokenKind::String,
                                        decodeString(lexeme),
                                        lexeme,
                                        column,
                                        std::nullopt});
                cursor = end;
                return;
            }
            if (!escaped && input[end] == '\n') {
                throw Base::ParserError(
                    fmt::format("Unescaped newline in string at column {}", column));
            }
            if (!escaped && input[end] == '>') {
                throw Base::ParserError(
                    fmt::format("Unescaped '>' in string at column {}", column));
            }
            if (escaped) {
                escaped = false;
            }
            else if (input[end] == '\\') {
                escaped = true;
            }
            ++end;
        }
        if (tolerant) {
            // Completion needs the historical token shape for an unfinished
            // literal string: two '<' tokens followed by its partial content.
            push(TokenKind::Less, "<", column);
            push(TokenKind::Less, "<", column + 1);
            cursor = column + 2;
            return;
        }
        throw Base::ParserError(fmt::format("Unterminated string at column {}", column));
    }

    bool scanComparison(std::size_t column)
    {
        const auto text = input.substr(cursor, 2);
        std::optional<TokenKind> kind;
        if (text == "==") kind = TokenKind::Equal;
        else if (text == "!=") kind = TokenKind::NotEqual;
        else if (text == "<=") kind = TokenKind::LessEqual;
        else if (text == ">=") kind = TokenKind::GreaterEqual;
        if (!kind) {
            return false;
        }
        push(*kind, text, column);
        cursor += 2;
        return true;
    }

    bool scanNumber(std::size_t column)
    {
        std::size_t comma = cursor;
        while (comma < input.size()
               && std::isdigit(static_cast<unsigned char>(input[comma]))) {
            ++comma;
        }
        if (comma < input.size() && input[comma] == ',' && comma + 1 < input.size()
            && std::isdigit(static_cast<unsigned char>(input[comma + 1]))) {
            auto end = comma + 2;
            while (end < input.size()
                   && std::isdigit(static_cast<unsigned char>(input[end]))) {
                ++end;
            }
            scanExponent(end);
            auto lexeme = input.substr(cursor, end - cursor);
            auto normalized = lexeme;
            normalized[normalized.find(',')] = '.';
            pushNumber(std::move(lexeme), normalized, column, true);
            cursor = end;
            return true;
        }

        const auto current = static_cast<unsigned char>(input[cursor]);
        if (!std::isdigit(current)
            && !(current == '.' && cursor + 1 < input.size()
                 && std::isdigit(static_cast<unsigned char>(input[cursor + 1])))) {
            return false;
        }
        const char* begin = input.c_str() + cursor;
        char* end = nullptr;
        errno = 0;
        std::strtod(begin, &end);
        if (end == begin || errno == ERANGE) {
            throw Base::ParserError(fmt::format("Invalid number at column {}", column));
        }
        cursor += static_cast<std::size_t>(end - begin);
        auto lexeme = input.substr(column, cursor - column);
        pushNumber(lexeme, lexeme, column, lexeme.find_first_of(".eE") != std::string::npos);
        return true;
    }

    void scanExponent(std::size_t& position) const
    {
        if (position >= input.size() || (input[position] != 'e' && input[position] != 'E')) {
            return;
        }
        const auto exponent = position++;
        if (position < input.size() && (input[position] == '+' || input[position] == '-')) {
            ++position;
        }
        const auto digits = position;
        while (position < input.size()
               && std::isdigit(static_cast<unsigned char>(input[position]))) {
            ++position;
        }
        if (position == digits) {
            position = exponent;
        }
    }

    void pushNumber(std::string lexeme,
                    const std::string& normalized,
                    std::size_t column,
                    bool floatingPoint)
    {
        char* end = nullptr;
        errno = 0;
        const double value = std::strtod(normalized.c_str(), &end);
        if (errno == ERANGE || end != normalized.c_str() + normalized.size()) {
            throw Base::ParserError(fmt::format("Invalid number at column {}", column));
        }
        if (!floatingPoint && value != 1.0) {
            errno = 0;
            const auto integer = std::strtoll(normalized.c_str(), &end, 10);
            if (errno == ERANGE || end != normalized.c_str() + normalized.size()) {
                throw Base::ParserError(fmt::format("Invalid integer at column {}", column));
            }
            tokens.push_back(
                Token {TokenKind::Integer, integer, std::move(lexeme), column, std::nullopt});
        }
        else {
            tokens.push_back(
                Token {TokenKind::Number, value, std::move(lexeme), column, std::nullopt});
        }
    }

    void scanName(std::size_t column)
    {
        ++cursor;
        while (cursor < input.size()) {
            const auto character = static_cast<unsigned char>(input[cursor]);
            if (!std::isalnum(character) && character != '_' && character != '@'
                && character != '$' && character < 0x80) {
                break;
            }
            ++cursor;
        }
        auto name = input.substr(column, cursor - column);
        if (isCellAddress(name)) {
            tokens.push_back(
                Token {TokenKind::CellAddress, name, name, column, std::nullopt});
            return;
        }
        auto afterWhitespace = cursor;
        while (afterWhitespace < input.size()
               && (input[afterWhitespace] == ' ' || input[afterWhitespace] == '\t')) {
            ++afterWhitespace;
        }
        if (afterWhitespace < input.size() && input[afterWhitespace] == '(') {
            const auto function = lookupFunction(name);
            tokens.push_back(Token {TokenKind::Function,
                                    FunctionToken {function,
                                                   function == FunctionExpression::NONE
                                                       ? name
                                                       : std::string()},
                                    input.substr(column, afterWhitespace + 1 - column),
                                    column,
                                    std::nullopt});
            cursor = afterWhitespace + 1;
            return;
        }
        if (pushConstant(name, column)) {
            return;
        }
        Token token {TokenKind::Name, name, name, column, std::nullopt};
        token.unitCandidate = Base::Quantity::lookupUnit(name);
        tokens.push_back(std::move(token));
    }

    bool pushConstant(const std::string& name, std::size_t column)
    {
        const char* canonical = nullptr;
        double value = 0;
        if (name == "pi") { canonical = "pi"; value = std::numbers::pi; }
        else if (name == "e") { canonical = "e"; value = std::numbers::e; }
        else if (name == "None") canonical = "None";
        else if (name == "True" || name == "true") { canonical = "True"; value = 1; }
        else if (name == "False" || name == "false") canonical = "False";
        if (!canonical) {
            return false;
        }
        tokens.push_back(
            Token {TokenKind::Constant, value, canonical, column, std::nullopt});
        return true;
    }

    static std::optional<TokenKind> punctuation(unsigned char character)
    {
        switch (character) {
            case '+': return TokenKind::Plus;
            case '-': return TokenKind::Minus;
            case '*': return TokenKind::Multiply;
            case '/': return TokenKind::Divide;
            case '%': return TokenKind::Modulo;
            case '^': return TokenKind::Power;
            case '<': return TokenKind::Less;
            case '>': return TokenKind::Greater;
            case '?': return TokenKind::Question;
            case ':': return TokenKind::Colon;
            case ',': return TokenKind::Comma;
            case ';': return TokenKind::Semicolon;
            case '.': return TokenKind::Dot;
            case '#': return TokenKind::Hash;
            case '(': return TokenKind::LeftParen;
            case ')': return TokenKind::RightParen;
            case '[': return TokenKind::LeftBracket;
            case ']': return TokenKind::RightBracket;
            default: return std::nullopt;
        }
    }

    std::string input;
    const FunctionLookup& lookupFunction;
    bool tolerant;
    std::vector<Token> tokens;
    std::size_t cursor {0};
};
}  // namespace

std::vector<Token> scanTokens(const char* buffer, const FunctionLookup& lookupFunction)
{
    return Lexer(buffer, lookupFunction).scan();
}

std::vector<Token> scanTokensTolerant(const char* buffer, const FunctionLookup& lookupFunction)
{
    return Lexer(buffer, lookupFunction, true).scan();
}

}  // namespace App::ExpressionParser::Pratt
