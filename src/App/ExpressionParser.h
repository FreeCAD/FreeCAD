// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "ExpressionNodes.h"

namespace App::ExpressionParser
{

/** Tokens produced by the expression lexer. */
enum class TokenKind
{
    End,
    Function,
    Number,
    Integer,
    Constant,
    Name,
    CellAddress,
    Unit,
    UsBuildingUnit,
    String,
    Plus,
    Minus,
    Multiply,
    Divide,
    Modulo,
    Power,
    Equal,
    NotEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    Question,
    Colon,
    Comma,
    Semicolon,
    Dot,
    Hash,
    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
};

struct FunctionToken
{
    FunctionExpression::Function function {FunctionExpression::NONE};
    std::string name;
};

using TokenValue = std::variant<std::monostate,
                                long long,
                                double,
                                std::string,
                                Base::Quantity,
                                FunctionToken>;

struct Token
{
    TokenKind kind {TokenKind::End};
    TokenValue value;
    std::string lexeme;
    std::size_t column {0};
    std::optional<Base::Quantity> unitCandidate;
};

struct BindingPower
{
    int left;
    int right;

    constexpr bool operator==(const BindingPower&) const = default;
};

// parseExpression() stops when left <= minimumBindingPower and parses the
// right operand with right as its new minimum.  Equal powers are therefore
// left associative; lowering right by one makes an operator right associative.

namespace BindingPowers
{
inline constexpr int ternary = 10;
inline constexpr int comparison = 20;
inline constexpr int additive = 30;
inline constexpr int multiplicative = 40;
inline constexpr int quantity = 50;
inline constexpr int power = 60;
inline constexpr int prefix = 70;
}  // namespace BindingPowers

/** Return the expression operator binding powers.
 *
 * All current binary operators, including power, are left associative.  The
 * asymmetric ternary pair makes the conditional operator right associative.
 */
constexpr std::optional<BindingPower> infixBindingPower(TokenKind kind)
{
    using namespace BindingPowers;
    switch (kind) {
        case TokenKind::Question:
            return BindingPower {ternary, ternary - 1};
        case TokenKind::Equal:
        case TokenKind::NotEqual:
        case TokenKind::Less:
        case TokenKind::Greater:
        case TokenKind::LessEqual:
        case TokenKind::GreaterEqual:
            return BindingPower {comparison, comparison};
        case TokenKind::Plus:
        case TokenKind::Minus:
            return BindingPower {additive, additive};
        case TokenKind::Multiply:
        case TokenKind::Divide:
        case TokenKind::Modulo:
            return BindingPower {multiplicative, multiplicative};
        case TokenKind::Power:
            return BindingPower {power, power};
        default:
            return std::nullopt;
    }
}

/** Cursor contract between the lexer and parser. */
class TokenStream
{
public:
    virtual ~TokenStream() = default;
    virtual const Token& peek(std::size_t offset = 0) = 0;
    virtual Token take() = 0;
    virtual std::size_t position() const = 0;
    virtual void rewind(std::size_t position) = 0;
};

/** Handwritten expression parser implementation. */
class Parser
{
public:
    Parser(const DocumentObject* owner, TokenStream& tokens)
        : owner(owner)
        , tokens(tokens)
    {}

    ExpressionPtr parse();
    std::unique_ptr<UnitExpression> parseUnit();
    ObjectIdentifier parsePath();

private:
    ExpressionPtr parseExpression(int minimumBindingPower = 0);
    ExpressionPtr parsePrimary();
    ExpressionPtr parseIdentifier();
    ExpressionPtr parseFunction();
    ExpressionPtr parseIndexer(ExpressionPtr expression);
    std::vector<ExpressionPtr> parseArguments();

    // This operation is transactional: failure rewinds to the initial cursor.
    // A unit operator is consumed only if parsing the following unit atom also
    // succeeds.  In particular, '/' is retained for the expression parser in
    // `24 V / (2 A)`, but consumed by the unit parser in `24 V / (kg * s)`.
    ExpressionPtr tryParseUnitSuffix(ExpressionPtr quantityIntroducer);
    ExpressionPtr parseUnitExpression(int minimumBindingPower = 0);
    ExpressionPtr parseUnitAtom();
    bool nextTokenStartsUnitAtom(std::size_t offset = 0);

    const DocumentObject* owner;
    TokenStream& tokens;
};

}  // namespace App::ExpressionParser
