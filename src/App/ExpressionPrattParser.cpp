// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ExpressionPrattParser.h"

#include "DocumentObject.h"

#include <fmt/format.h>

#include <Base/Exception.h>

namespace App::ExpressionParser::Pratt
{

namespace
{
constexpr int unitProductBindingPower = 10;
constexpr int unitPowerBindingPower = 20;

void expect(TokenStream& tokens, TokenKind kind, const char* description)
{
    if (tokens.peek().kind != kind) {
        throw Base::ParserError(fmt::format("Expected {} at column {}",
                                            description,
                                            tokens.peek().column));
    }
    tokens.take();
}

OperatorExpression::Operator binaryOperator(TokenKind kind)
{
    switch (kind) {
        case TokenKind::Plus:
            return OperatorExpression::ADD;
        case TokenKind::Minus:
            return OperatorExpression::SUB;
        case TokenKind::Multiply:
            return OperatorExpression::MUL;
        case TokenKind::Divide:
            return OperatorExpression::DIV;
        case TokenKind::Modulo:
            return OperatorExpression::MOD;
        case TokenKind::Power:
            return OperatorExpression::POW;
        case TokenKind::Equal:
            return OperatorExpression::EQ;
        case TokenKind::NotEqual:
            return OperatorExpression::NEQ;
        case TokenKind::Less:
            return OperatorExpression::LT;
        case TokenKind::Greater:
            return OperatorExpression::GT;
        case TokenKind::LessEqual:
            return OperatorExpression::LTE;
        case TokenKind::GreaterEqual:
            return OperatorExpression::GTE;
        default:
            throw Base::ParserError("Token is not a binary operator");
    }
}

const char* constantName(const std::string& name)
{
    if (name == "pi") {
        return "pi";
    }
    if (name == "e") {
        return "e";
    }
    if (name == "True") {
        return "True";
    }
    if (name == "False") {
        return "False";
    }
    if (name == "None") {
        return "None";
    }
    throw Base::ParserError(fmt::format("Unknown constant '{}'", name));
}

ExpressionPtr numberExpression(const DocumentObject* owner, const Token& token)
{
    if (token.kind == TokenKind::Integer) {
        return std::make_unique<NumberExpression>(
            owner,
            Base::Quantity(static_cast<double>(std::get<long long>(token.value))));
    }
    if (token.kind == TokenKind::Number) {
        return std::make_unique<NumberExpression>(owner,
                                                  Base::Quantity(std::get<double>(token.value)));
    }
    if (token.kind == TokenKind::Constant) {
        return std::make_unique<ConstantExpression>(owner,
                                                    constantName(token.lexeme),
                                                    Base::Quantity(std::get<double>(token.value)));
    }
    throw Base::ParserError("Token is not a numeric expression");
}

bool isNumberToken(TokenKind kind)
{
    return kind == TokenKind::Integer || kind == TokenKind::Number
        || kind == TokenKind::Constant;
}

bool isIdOrCell(TokenKind kind)
{
    return kind == TokenKind::Name || kind == TokenKind::CellAddress;
}

std::string tokenString(const Token& token)
{
    return std::get<std::string>(token.value);
}
}  // namespace

ExpressionPtr Parser::parse()
{
    auto expression = parseExpression();
    if (tokens.peek().kind != TokenKind::End) {
        throw Base::ParserError(fmt::format("Unexpected token '{}' at column {}",
                                            tokens.peek().lexeme,
                                            tokens.peek().column));
    }
    return expression;
}

ExpressionPtr Parser::parseExpression(int minimumBindingPower)
{
    auto left = parsePrimary();

    while (true) {
        const auto kind = tokens.peek().kind;
        const auto powers = infixBindingPower(kind);
        if (!powers || powers->left <= minimumBindingPower) {
            break;
        }

        tokens.take();
        if (kind == TokenKind::Question) {
            auto trueExpression = parseExpression();
            expect(tokens, TokenKind::Colon, "':'");
            auto falseExpression = parseExpression(powers->right);
            left = std::make_unique<ConditionalExpression>(owner,
                                                           left.release(),
                                                           trueExpression.release(),
                                                           falseExpression.release());
            continue;
        }

        auto right = parseExpression(powers->right);
        left = std::make_unique<OperatorExpression>(owner,
                                                    left.release(),
                                                    binaryOperator(kind),
                                                    right.release());
    }

    return left;
}

ExpressionPtr Parser::parsePrimary()
{
    if (tokens.peek().kind == TokenKind::Function) {
        return parseFunction();
    }
    if (tokens.peek().kind == TokenKind::Name
        || tokens.peek().kind == TokenKind::CellAddress || tokens.peek().kind == TokenKind::Dot
        || (tokens.peek().kind == TokenKind::String
            && (tokens.peek(1).kind == TokenKind::Dot
                || tokens.peek(1).kind == TokenKind::Hash))) {
        return parseIdentifier();
    }

    const auto token = tokens.take();
    switch (token.kind) {
        case TokenKind::Integer:
        case TokenKind::Number:
        case TokenKind::Constant: {
            auto number = numberExpression(owner, token);
            return tryParseUnitSuffix(std::move(number));
        }
        case TokenKind::String:
            return std::make_unique<StringExpression>(owner, std::get<std::string>(token.value));
        case TokenKind::Plus:
        case TokenKind::Minus: {
            auto operand = parseExpression(BindingPowers::prefix);
            const auto operation = token.kind == TokenKind::Minus ? OperatorExpression::NEG
                                                                  : OperatorExpression::POS;
            const double identity = token.kind == TokenKind::Minus ? -1.0 : 1.0;
            return std::make_unique<OperatorExpression>(
                owner,
                operand.release(),
                operation,
                new NumberExpression(owner, Base::Quantity(identity)));
        }
        case TokenKind::LeftParen: {
            auto expression = parseExpression();
            expect(tokens, TokenKind::RightParen, "')'");
            return expression;
        }
        default:
            throw Base::ParserError(fmt::format("Unexpected token '{}' at column {}",
                                                token.lexeme,
                                                token.column));
    }
}

ExpressionPtr Parser::parseIdentifier()
{
    ObjectIdentifier path(owner);

    if (tokens.peek().kind == TokenKind::Dot) {
        tokens.take();
        if (tokens.peek().kind == TokenKind::String) {
            auto subObject = tokenString(tokens.take());
            expect(tokens, TokenKind::Dot, "'.'");
            if (!isIdOrCell(tokens.peek().kind)) {
                throw Base::ParserError("Expected property name after sub-object");
            }
            auto property = tokenString(tokens.take());
            path = ObjectIdentifier(owner, true);
            path.setDocumentObjectName(owner,
                                       false,
                                       ObjectIdentifier::String(std::move(subObject), true),
                                       true);
            path.addComponent(ObjectIdentifier::SimpleComponent(property));
        }
        else {
            if (!isIdOrCell(tokens.peek().kind)) {
                throw Base::ParserError("Expected property name after '.'");
            }
            auto property = tokenString(tokens.take());
            path = ObjectIdentifier(owner, true);
            path.setDocumentObjectName(owner);
            path.addComponent(ObjectIdentifier::SimpleComponent(property));
        }
    }
    else {
        const auto first = tokens.take();
        const bool firstIsString = first.kind == TokenKind::String;
        const auto firstText = tokenString(first);

        if (tokens.peek().kind == TokenKind::Hash) {
            if (first.kind != TokenKind::Name && first.kind != TokenKind::String) {
                throw Base::ParserError("Cell address cannot name a document");
            }
            tokens.take();
            if (tokens.peek().kind != TokenKind::Name
                && tokens.peek().kind != TokenKind::CellAddress
                && tokens.peek().kind != TokenKind::String) {
                throw Base::ParserError("Expected document object after '#'");
            }
            const auto objectToken = tokens.take();
            auto objectName = ObjectIdentifier::String(tokenString(objectToken),
                                                       objectToken.kind == TokenKind::String);
            expect(tokens, TokenKind::Dot, "'.'");

            path.setDocumentName(ObjectIdentifier::String(firstText, firstIsString, !firstIsString),
                                 true);
            if (tokens.peek().kind == TokenKind::String) {
                auto subObject = tokenString(tokens.take());
                expect(tokens, TokenKind::Dot, "'.'");
                if (!isIdOrCell(tokens.peek().kind)) {
                    throw Base::ParserError("Expected property name after sub-object");
                }
                path.setDocumentObjectName(std::move(objectName),
                                           true,
                                           ObjectIdentifier::String(std::move(subObject), true));
            }
            else {
                path.setDocumentObjectName(std::move(objectName), true);
            }
            if (!isIdOrCell(tokens.peek().kind)) {
                throw Base::ParserError("Expected property name after document object");
            }
            path.addComponent(ObjectIdentifier::SimpleComponent(tokenString(tokens.take())));
        }
        else if (tokens.peek().kind == TokenKind::Dot) {
            tokens.take();
            auto objectName = ObjectIdentifier::String(firstText, firstIsString);
            if (tokens.peek().kind == TokenKind::String) {
                auto subObject = tokenString(tokens.take());
                expect(tokens, TokenKind::Dot, "'.'");
                if (!isIdOrCell(tokens.peek().kind)) {
                    throw Base::ParserError("Expected property name after sub-object");
                }
                path.setDocumentObjectName(std::move(objectName),
                                           true,
                                           ObjectIdentifier::String(std::move(subObject), true),
                                           true);
            }
            else {
                if (!isIdOrCell(tokens.peek().kind)) {
                    throw Base::ParserError("Expected property name after object");
                }
                objectName.checkImport(owner);
                path.addComponent(ObjectIdentifier::SimpleComponent(objectName));
            }
            path.addComponent(ObjectIdentifier::SimpleComponent(tokenString(tokens.take())));
            path.resolveAmbiguity();
        }
        else {
            if (!isIdOrCell(first.kind)) {
                throw Base::ParserError("String is not an unqualified identifier");
            }
            path << ObjectIdentifier::SimpleComponent(firstText);
        }
    }

    while (tokens.peek().kind == TokenKind::Dot && tokens.peek(1).kind == TokenKind::Name) {
        tokens.take();
        path.addComponent(ObjectIdentifier::SimpleComponent(tokenString(tokens.take())));
    }

    ExpressionPtr expression = std::make_unique<VariableExpression>(owner, path);
    while (tokens.peek().kind == TokenKind::LeftBracket) {
        expression = parseIndexer(std::move(expression));
        while (tokens.peek().kind == TokenKind::Dot && tokens.peek(1).kind == TokenKind::Name) {
            tokens.take();
            expression->addComponent(Expression::createComponent(tokenString(tokens.take())));
        }
    }
    return expression;
}

ExpressionPtr Parser::parseIndexer(ExpressionPtr expression)
{
    expect(tokens, TokenKind::LeftBracket, "'['");

    ExpressionPtr begin;
    ExpressionPtr end;
    ExpressionPtr step;
    if (tokens.peek().kind != TokenKind::Colon) {
        begin = parseExpression();
    }

    if (tokens.peek().kind == TokenKind::RightBracket) {
        if (!begin) {
            throw Base::ParserError("Empty index is not allowed");
        }
        tokens.take();
        expression->addComponent(Expression::createComponent(begin.release()));
        return expression;
    }

    expect(tokens, TokenKind::Colon, "':'");
    if (tokens.peek().kind != TokenKind::Colon
        && tokens.peek().kind != TokenKind::RightBracket) {
        end = parseExpression();
    }
    if (tokens.peek().kind == TokenKind::Colon) {
        tokens.take();
        if (tokens.peek().kind == TokenKind::RightBracket) {
            throw Base::ParserError("Slice step is required after the second ':'");
        }
        step = parseExpression();
    }
    expect(tokens, TokenKind::RightBracket, "']'");
    if (!begin && !end && !step) {
        throw Base::ParserError("Empty slice is not allowed");
    }
    expression->addComponent(
        Expression::createComponent(begin.release(), end.release(), step.release(), true));
    return expression;
}

ExpressionPtr Parser::parseFunction()
{
    const auto token = tokens.take();
    const auto& function = std::get<FunctionToken>(token.value);
    auto arguments = parseArguments();
    expect(tokens, TokenKind::RightParen, "')'");

    std::vector<Expression*> rawArguments;
    rawArguments.reserve(arguments.size());
    for (auto& argument : arguments) {
        rawArguments.push_back(argument.release());
    }
    const auto ownedPointers = rawArguments;

    try {
        return std::make_unique<FunctionExpression>(owner,
                                                    function.function,
                                                    std::string(function.name),
                                                    std::move(rawArguments));
    }
    catch (...) {
        for (auto* argument : ownedPointers) {
            delete argument;
        }
        throw;
    }
}

std::vector<ExpressionPtr> Parser::parseArguments()
{
    if (tokens.peek().kind == TokenKind::RightParen) {
        throw Base::ParserError(fmt::format("Expected function argument at column {}",
                                            tokens.peek().column));
    }

    const auto parseArgument = [this]() -> ExpressionPtr {
        if (isIdOrCell(tokens.peek().kind) && tokens.peek(1).kind == TokenKind::Colon
            && isIdOrCell(tokens.peek(2).kind)) {
            auto begin = tokenString(tokens.take());
            tokens.take();
            auto end = tokenString(tokens.take());
            return std::make_unique<RangeExpression>(owner, begin, end);
        }
        return parseExpression();
    };

    std::vector<ExpressionPtr> arguments;
    arguments.push_back(parseArgument());
    while (tokens.peek().kind == TokenKind::Comma
           || tokens.peek().kind == TokenKind::Semicolon) {
        tokens.take();
        arguments.push_back(parseArgument());
    }
    return arguments;
}

ExpressionPtr Parser::tryParseUnitSuffix(ExpressionPtr quantityIntroducer)
{
    if (!nextTokenStartsUnitAtom()) {
        return quantityIntroducer;
    }

    const auto start = tokens.position();
    const bool startsWithUsBuildingUnit = tokens.peek().kind == TokenKind::UsBuildingUnit;
    ExpressionPtr unit;
    try {
        unit = parseUnitExpression();
    }
    catch (const Base::ParserError&) {
        tokens.rewind(start);
        return quantityIntroducer;
    }

    auto quantity = std::make_unique<OperatorExpression>(owner,
                                                         quantityIntroducer.release(),
                                                         OperatorExpression::UNIT,
                                                         unit.release());

    // Preserve the special feet-and-inches production: num USUNIT num USUNIT.
    if (startsWithUsBuildingUnit && isNumberToken(tokens.peek().kind)
        && tokens.peek(1).kind == TokenKind::UsBuildingUnit) {
        auto secondNumber = numberExpression(owner, tokens.take());
        auto secondUnit = parseUnitAtom();
        auto secondQuantity = std::make_unique<OperatorExpression>(owner,
                                                                  secondNumber.release(),
                                                                  OperatorExpression::UNIT,
                                                                  secondUnit.release());
        return std::make_unique<OperatorExpression>(owner,
                                                    quantity.release(),
                                                    OperatorExpression::ADD,
                                                    secondQuantity.release());
    }

    return quantity;
}

ExpressionPtr Parser::parseUnitExpression(int minimumBindingPower)
{
    auto left = parseUnitAtom();

    while (true) {
        const auto kind = tokens.peek().kind;
        if (kind == TokenKind::Power && unitPowerBindingPower > minimumBindingPower) {
            const auto operatorPosition = tokens.position();
            tokens.take();

            bool negative = false;
            if (tokens.peek().kind == TokenKind::Minus) {
                negative = true;
                tokens.take();
            }

            const auto exponentToken = tokens.peek();
            long long exponent = 0;
            if (exponentToken.kind == TokenKind::Integer) {
                exponent = std::get<long long>(exponentToken.value);
            }
            else if (exponentToken.kind == TokenKind::Number
                     && std::get<double>(exponentToken.value) == 1.0) {
                exponent = 1;
            }
            else {
                tokens.rewind(operatorPosition);
                break;
            }
            tokens.take();
            if (negative) {
                exponent = -exponent;
            }

            left = std::make_unique<OperatorExpression>(
                owner,
                left.release(),
                OperatorExpression::POW,
                new NumberExpression(owner, Base::Quantity(static_cast<double>(exponent))));
            continue;
        }

        if ((kind == TokenKind::Multiply || kind == TokenKind::Divide)
            && unitProductBindingPower > minimumBindingPower) {
            const auto operatorPosition = tokens.position();
            tokens.take();
            try {
                auto right = parseUnitExpression(unitProductBindingPower);
                const auto operation = kind == TokenKind::Multiply ? OperatorExpression::MUL
                                                                   : OperatorExpression::DIV;
                left = std::make_unique<OperatorExpression>(owner,
                                                            left.release(),
                                                            operation,
                                                            right.release());
                continue;
            }
            catch (const Base::ParserError&) {
                tokens.rewind(operatorPosition);
            }
        }
        break;
    }

    return left;
}

ExpressionPtr Parser::parseUnitAtom()
{
    const auto token = tokens.take();
    if (token.kind == TokenKind::Unit || token.kind == TokenKind::UsBuildingUnit) {
        return std::make_unique<UnitExpression>(owner,
                                                std::get<Base::Quantity>(token.value),
                                                token.lexeme);
    }
    if (token.kind == TokenKind::LeftParen) {
        auto expression = parseUnitExpression();
        expect(tokens, TokenKind::RightParen, "')'");
        return expression;
    }
    throw Base::ParserError(fmt::format("Expected unit at column {}", token.column));
}

bool Parser::nextTokenStartsUnitAtom(std::size_t offset)
{
    const auto kind = tokens.peek(offset).kind;
    return kind == TokenKind::Unit || kind == TokenKind::UsBuildingUnit
        || kind == TokenKind::LeftParen;
}

}  // namespace App::ExpressionParser::Pratt
