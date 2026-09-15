// SPDX-License-Identifier: LGPL-2.1-or-later

#include "SelectionFilterParser.h"

#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include "SelectionFilter.h"

namespace Gui
{
namespace
{
enum class TokenKind
{
    End,
    Identifier,
    Select,
    Subelement,
    Count,
    Slice,
    Namespace,
    Number
};

struct Token
{
    TokenKind kind {TokenKind::End};
    std::string text;
    int number {};
};

class Lexer
{
public:
    explicit Lexer(std::string_view input)
        : input(input)
    {}

    Token next()
    {
        while (position < input.size()) {
            const char ch = input[position];
            if (ch == ' ' || ch == '\t' || ch == '\n') {
                ++position;
                continue;
            }
            if (input.substr(position, 2) == "..") {
                position += 2;
                return {.kind = TokenKind::Slice, .text = {}, .number = 0};
            }
            if (input.substr(position, 2) == "::") {
                position += 2;
                return {.kind = TokenKind::Namespace, .text = {}, .number = 0};
            }
            if (isIdentifierStart(ch)) {
                const auto start = position++;
                while (position < input.size() && isIdentifierContinue(input[position])) {
                    ++position;
                }
                std::string text(input.substr(start, position - start));
                if (text == "SELECT") {
                    return {.kind = TokenKind::Select, .text = {}, .number = 0};
                }
                if (text == "SUBELEMENT") {
                    return {.kind = TokenKind::Subelement, .text = {}, .number = 0};
                }
                if (text == "COUNT") {
                    return {.kind = TokenKind::Count, .text = {}, .number = 0};
                }
                return {.kind = TokenKind::Identifier, .text = std::move(text)};
            }
            if (isDigit(ch)) {
                const auto start = position++;
                while (position < input.size() && isDigit(input[position])) {
                    ++position;
                }
                std::string text(input.substr(start, position - start));
                return {
                    .kind = TokenKind::Number,
                    .text = text,
                    .number = static_cast<int>(std::strtol(text.c_str(), nullptr, 10))
                };
            }

            // Flex's default rule consumed and echoed unmatched characters. Consume them here too,
            // without the surprising output side effect, to retain the accepted language.
            ++position;
        }
        return {};
    }

private:
    static bool isIdentifierStart(char ch)
    {
        return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
    }

    static bool isIdentifierContinue(char ch)
    {
        return isIdentifierStart(ch) || (ch >= '0' && ch <= '9');
    }

    static bool isDigit(char ch)
    {
        return ch >= '0' && ch <= '9';
    }

    std::string_view input;
    std::size_t position {};
};

class Parser
{
public:
    explicit Parser(std::string_view input)
        : lexer(input)
        , current(lexer.next())
    {}

    std::shared_ptr<Node_Block> parse()
    {
        auto first = parseMatchLine();
        if (!first) {
            return {};
        }

        auto block = std::make_shared<Node_Block>(first.release());
        while (current.kind != TokenKind::End) {
            auto object = parseMatchLine();
            if (!object) {
                return {};
            }
            block->Objects.emplace_back(object.release());
        }
        return block;
    }

private:
    bool consume(TokenKind kind)
    {
        if (current.kind != kind) {
            return false;
        }
        current = lexer.next();
        return true;
    }

    std::unique_ptr<Node_Object> parseMatchLine()
    {
        if (!consume(TokenKind::Select) || current.kind != TokenKind::Identifier) {
            return {};
        }
        std::string type = std::move(current.text);
        current = lexer.next();
        if (consume(TokenKind::Namespace)) {
            if (current.kind != TokenKind::Identifier) {
                return {};
            }
            type += "::";
            type += current.text;
            current = lexer.next();
        }

        std::string subelement;
        std::string* subelementPtr = nullptr;
        if (consume(TokenKind::Subelement)) {
            if (current.kind != TokenKind::Identifier) {
                return {};
            }
            subelement = std::move(current.text);
            subelementPtr = &subelement;
            current = lexer.next();
        }

        std::unique_ptr<Node_Slice> slice;
        if (consume(TokenKind::Count)) {
            if (current.kind != TokenKind::Number) {
                return {};
            }
            const int minimum = current.number;
            current = lexer.next();
            if (consume(TokenKind::Slice)) {
                if (current.kind == TokenKind::Number) {
                    slice = std::make_unique<Node_Slice>(minimum, current.number);
                    current = lexer.next();
                }
                else {
                    slice = std::make_unique<Node_Slice>(minimum);
                }
            }
            else {
                slice = std::make_unique<Node_Slice>(minimum, minimum);
            }
        }

        return std::make_unique<Node_Object>(&type, subelementPtr, slice.release());
    }

    Lexer lexer;
    Token current;
};
}  // namespace

std::shared_ptr<Node_Block> parseSelectionFilter(std::string_view input, std::string& error)
{
    auto ast = Parser(input).parse();
    if (!ast) {
        error = "syntax error\n";
    }
    else {
        error.clear();
    }
    return ast;
}
}  // namespace Gui
