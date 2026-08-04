// SPDX-License-Identifier: LGPL-2.1-or-later

#include "QuantityInput.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <limits>
#include <string>

#include <App/Expression.h>
#include <App/ExpressionParser.h>
#include <App/ObjectIdentifier.h>
#include <Base/Exception.h>
#include <Base/NumericFormatting.h>
#include <Base/NumericInput.h>

namespace
{
std::size_t firstInputCharacter(std::string_view input)
{
    std::size_t position = 0;
    while (position < input.size()
           && std::isspace(static_cast<unsigned char>(input[position]))) {
        ++position;
    }
    return position;
}

bool startsNumericInput(
    std::string_view input,
    const std::size_t position,
    const Base::NumericLocaleContext& locale
)
{
    if (position >= input.size()) {
        return false;
    }
    int digit = 0;
    std::size_t digitLength = 0;
    if (Base::localizedDigitAt(input, position, locale, digit, digitLength)
        || input[position] == '.') {
        return true;
    }
    if (!locale.decimalSeparator.empty()
        && input.substr(position, locale.decimalSeparator.size()) == locale.decimalSeparator) {
        return true;
    }
    const std::string_view positiveSign {
        locale.positiveSign.data(), locale.positiveSign.size()
    };
    const std::string_view negativeSign {
        locale.negativeSign.data(), locale.negativeSign.size()
    };
    for (const auto sign : {std::string_view {"+"}, std::string_view {"-"}, positiveSign,
                            negativeSign}) {
        if (!sign.empty() && input.substr(position, sign.size()) == sign) {
            const auto next = position + sign.size();
            if (next == input.size()) {
                return true;
            }

            int nextDigit = 0;
            std::size_t nextDigitLength = 0;
            return Base::localizedDigitAt(
                       input, next, locale, nextDigit, nextDigitLength
                   )
                || input[next] == '.'
                || (!locale.decimalSeparator.empty()
                    && input.substr(next, locale.decimalSeparator.size())
                        == locale.decimalSeparator);
        }
    }
    return false;
}

App::InputDiagnosticKind inputDiagnosticKind(Base::NumericDiagnosticKind kind)
{
    switch (kind) {
        case Base::NumericDiagnosticKind::IncompleteSign:
        case Base::NumericDiagnosticKind::IncompleteDecimal:
        case Base::NumericDiagnosticKind::IncompleteGrouping:
        case Base::NumericDiagnosticKind::IncompleteExponent:
            return App::InputDiagnosticKind::IncompleteNumber;
        case Base::NumericDiagnosticKind::InvalidGrouping:
        case Base::NumericDiagnosticKind::UnexpectedSeparator:
            return App::InputDiagnosticKind::MalformedGrouping;
        case Base::NumericDiagnosticKind::ExpectedDigit:
        case Base::NumericDiagnosticKind::InvalidLiteral:
            return App::InputDiagnosticKind::InvalidNumber;
        case Base::NumericDiagnosticKind::OutOfRange:
            return App::InputDiagnosticKind::OutOfRange;
    }
    return App::InputDiagnosticKind::InvalidNumber;
}

App::QuantityInputResult invalid(
    App::InputDiagnosticKind kind,
    const std::size_t offset = 0,
    const std::size_t length = 1
)
{
    App::QuantityInputResult result;
    result.status = App::InputStatus::Invalid;
    result.diagnostic = App::InputDiagnostic {kind, offset, length};
    return result;
}

App::QuantityInputResult incomplete(
    const App::InputPhase phase,
    const App::InputDiagnosticKind kind,
    const std::size_t offset = 0,
    const std::size_t length = 1
)
{
    if (phase == App::InputPhase::Commit) {
        return invalid(kind, offset, length);
    }

    App::QuantityInputResult result;
    result.status = App::InputStatus::Incomplete;
    result.diagnostic = App::InputDiagnostic {kind, offset, length};
    return result;
}

std::string canonicalQuantityText(const Base::Quantity& quantity)
{
    char number[128] {};
    const auto conversion = std::to_chars(
        std::begin(number), std::end(number), quantity.getValue(), std::chars_format::general,
        std::numeric_limits<double>::max_digits10
    );
    std::string result;
    if (conversion.ec == std::errc {}) {
        result.assign(number, conversion.ptr);
    }
    else {
        result = "nan";
    }
    const auto unit = quantity.getUnit().getString();
    if (!unit.empty()) {
        result += ' ';
        result += unit;
    }
    return result;
}

App::Expression* applyDefaultUnitToDimensionlessTerm(
    App::Expression* expression,
    const Base::Unit& defaultUnit
)
{
    if (!expression) {
        return nullptr;
    }

    try {
        auto evaluated = expression->eval();
        auto* number = freecad_cast<App::NumberExpression*>(evaluated.get());
        if (number && number->getUnit() == Base::Unit::One) {
            // Keep the original subtree intact. In particular, replacing a variable
            // expression with its current value would silently remove its dependency.
            auto unit = std::make_unique<App::UnitExpression>(
                expression->getOwner(), Base::Quantity(1.0, defaultUnit), defaultUnit.getString()
            );
            return new App::OperatorExpression(
                expression->getOwner(),
                expression->copy().release(),
                App::OperatorExpression::UNIT,
                unit.release()
            );
        }
    }
    catch (const Base::UnitsMismatchError&) {
        // A nested additive term may not be evaluable until its own default unit is applied.
    }

    return nullptr;
}

class DefaultUnitForAdditiveTerms final: public App::ExpressionVisitor
{
public:
    explicit DefaultUnitForAdditiveTerms(const Base::Unit& defaultUnit)
        : defaultUnit(defaultUnit)
    {}

    void visit(App::Expression& expression) override
    {
        auto* operation = freecad_cast<App::OperatorExpression*>(&expression);
        if (!operation
            || (operation->getOperator() != App::OperatorExpression::ADD
                && operation->getOperator() != App::OperatorExpression::SUB)) {
            return;
        }

        if (auto* replacement
            = applyDefaultUnitToDimensionlessTerm(operation->getLeft(), defaultUnit)) {
            operation->setLeft(replacement);
        }
        if (auto* replacement
            = applyDefaultUnitToDimensionlessTerm(operation->getRight(), defaultUnit)) {
            operation->setRight(replacement);
        }
    }

private:
    const Base::Unit& defaultUnit;
};

void applyDefaultUnitPolicy(App::Expression& expression, const Base::Unit& defaultUnit)
{
    if (defaultUnit == Base::Unit::One) {
        return;
    }

    DefaultUnitForAdditiveTerms visitor(defaultUnit);
    expression.visit(visitor);
}

}  // namespace

App::QuantityInputResult App::interpretQuantityInput(
    const std::string_view input,
    const QuantityInputGrammar grammar,
    const ObjectIdentifier& path,
    const Base::Unit& defaultUnit,
    const Base::NumericLocaleContext& locale,
    const InputPhase phase,
    const QuantityConstraints& constraints
)
{
    const auto first = firstInputCharacter(input);
    if (first == input.size()) {
        return incomplete(phase, InputDiagnosticKind::IncompleteNumber, first);
    }

    // Scan an initial numeric token before invoking either parser. This is what preserves the
    // distinction between an unfinished edit ("-", "12,", "1e") and a syntax failure.
    if (startsNumericInput(input, first, locale)) {
        const auto scan = Base::scanLocalizedNumber(
            input.substr(first), locale, Base::NumericSyntaxContext::Standalone
        );
        if (scan.status == Base::LocalizedNumberResult::Status::Incomplete) {
            return incomplete(
                phase,
                InputDiagnosticKind::IncompleteNumber,
                first + (scan.diagnostic ? scan.diagnostic->offset : 0),
                scan.diagnostic ? scan.diagnostic->length : 1
            );
        }
        if (scan.status == Base::LocalizedNumberResult::Status::Invalid) {
            return invalid(
                scan.diagnostic
                    ? inputDiagnosticKind(scan.diagnostic->kind)
                    : InputDiagnosticKind::InvalidNumber,
                first + (scan.diagnostic ? scan.diagnostic->offset : 0),
                scan.diagnostic ? scan.diagnostic->length : 1
            );
        }
    }

    Base::Quantity quantity;
    std::shared_ptr<Expression> parsedExpression;
    try {
        if (grammar == QuantityInputGrammar::Expression) {
            const auto* owner = path.getDocumentObject();
            if (!owner) {
                return invalid(InputDiagnosticKind::ExpressionSyntax);
            }
            const std::string inputString(input);
            auto parsed = App::ExpressionParser::parseUserInput(
                owner, inputString.c_str(), locale
            );
            parsedExpression = std::shared_ptr<Expression>(std::move(parsed));
            // Apply the field-unit policy before evaluation. Dimensionless additive terms are
            // replaced as complete expressions; explicit units and multiplicative terms remain
            // untouched.
            applyDefaultUnitPolicy(*parsedExpression, defaultUnit);
            const auto evaluated = parsedExpression->eval();
            auto* number = freecad_cast<NumberExpression*>(evaluated.get());
            if (!number) {
                return invalid(InputDiagnosticKind::Evaluation);
            }
            quantity = number->getQuantity();
        }
        else {
            quantity = Base::Quantity::parseUserInput(std::string(input), locale);
        }
    }
    catch (const Base::ParserError&) {
        return invalid(InputDiagnosticKind::ExpressionSyntax);
    }
    catch (const Base::UnitsMismatchError&) {
        return invalid(InputDiagnosticKind::IncompatibleUnit);
    }
    catch (const Base::Exception&) {
        return invalid(InputDiagnosticKind::Evaluation);
    }

    if (quantity.isDimensionless()) {
        quantity.setUnit(defaultUnit);
    }

    if (constraints.requiredUnit && !quantity.isDimensionlessOrUnit(*constraints.requiredUnit)) {
        return invalid(InputDiagnosticKind::IncompatibleUnit);
    }
    if (constraints.minimum && quantity.getValue() < *constraints.minimum) {
        return invalid(InputDiagnosticKind::OutOfRange);
    }
    if (constraints.maximum && quantity.getValue() > *constraints.maximum) {
        return invalid(InputDiagnosticKind::OutOfRange);
    }

    QuantityInputResult result;
    result.status = InputStatus::Acceptable;
    result.quantity = quantity;
    result.expression = std::move(parsedExpression);
    result.normalizedText = canonicalQuantityText(quantity);
    return result;
}
