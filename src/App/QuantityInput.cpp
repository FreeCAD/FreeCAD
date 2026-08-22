// SPDX-License-Identifier: LGPL-2.1-or-later

#include "QuantityInput.h"

#include <algorithm>
#include <cctype>
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

class AdditiveOperatorDetector final : public App::ExpressionVisitor
{
public:
    void visit(App::Expression& expression) override
    {
        const auto* operation = freecad_cast<const App::OperatorExpression*>(&expression);
        if (operation
            && (operation->getOperator() == App::OperatorExpression::ADD
                || operation->getOperator() == App::OperatorExpression::SUB)) {
            additive = true;
        }
    }

    bool additive {false};
};

bool hasAdditiveOperator(
    const std::string_view input,
    const App::ObjectIdentifier& path,
    const Base::NumericLocaleContext& locale
)
{
    try {
        // An unbound input has no owner to resolve. Avoid asking ObjectIdentifier to consult the
        // application singleton in that case; quantity parsing is valid before application
        // initialization as well.
        const auto* owner = path.getOwner() ? path.getDocumentObject() : nullptr;
        const std::string inputString(input);
        auto expression = App::ExpressionParser::parseUserInput(owner, inputString.c_str(), locale);
        AdditiveOperatorDetector detector;
        expression->visit(detector);
        return detector.additive;
    }
    catch (const Base::Exception&) {
        // The quantity parser remains authoritative for quantity-only syntax, including
        // bracketed comments and juxtaposed quantities that are not expression syntax.
        return false;
    }
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

std::unique_ptr<App::Expression> rewriteOwnerlessQuantityExpression(
    const App::Expression& expression,
    const App::DocumentObject* owner
)
{
    const auto* operation = freecad_cast<const App::OperatorExpression*>(&expression);
    if (!operation) {
        return expression.copy();
    }

    auto left = operation->getLeft()
        ? rewriteOwnerlessQuantityExpression(*operation->getLeft(), owner)
        : nullptr;
    auto right = operation->getRight()
        ? rewriteOwnerlessQuantityExpression(*operation->getRight(), owner)
        : nullptr;

    // In a quantity field, `1/2 mm` has historically meant `(1/2) mm`, just as
    // `1/2"` means half an inch. The expression parser's normal precedence reads
    // this as the inverse of a length. Preserve the quantity-field rule structurally
    // by moving the explicit unit outside the denominator before evaluation.
    auto* rewrittenUnitOperation = right
        ? freecad_cast<App::OperatorExpression*>(right.get())
        : nullptr;
    if (operation->getOperator() == App::OperatorExpression::DIV
        && rewrittenUnitOperation
        && rewrittenUnitOperation->getOperator() == App::OperatorExpression::UNIT
        && rewrittenUnitOperation->getLeft()
        && rewrittenUnitOperation->getRight()) {
        auto denominator = rewrittenUnitOperation->getLeft()->copy();
        auto unit = rewrittenUnitOperation->getRight()->copy();
        if (unit && denominator) {
            auto quotient = std::make_unique<App::OperatorExpression>(
                owner,
                left.release(),
                App::OperatorExpression::DIV,
                denominator.release()
            );
            return std::make_unique<App::OperatorExpression>(
                owner,
                quotient.release(),
                App::OperatorExpression::MUL,
                unit.release()
            );
        }
    }

    return std::make_unique<App::OperatorExpression>(
        owner,
        left.release(),
        operation->getOperator(),
        right.release()
    );
}

App::QuantityInputResult invalid(
    App::InputDiagnosticKind kind,
    const std::size_t offsetBytes = 0,
    const std::size_t lengthBytes = 1
)
{
    App::QuantityInputResult result;
    result.status = App::InputStatus::Invalid;
    result.diagnostic = App::InputDiagnostic {kind, offsetBytes, lengthBytes};
    return result;
}

App::QuantityInputResult incomplete(
    const App::InputPhase phase,
    const App::InputDiagnosticKind kind,
    const std::size_t offsetBytes = 0,
    const std::size_t lengthBytes = 1
)
{
    if (phase == App::InputPhase::Commit) {
        return invalid(kind, offsetBytes, lengthBytes);
    }

    App::QuantityInputResult result;
    result.status = App::InputStatus::Incomplete;
    result.diagnostic = App::InputDiagnostic {kind, offsetBytes, lengthBytes};
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
        auto* value = freecad_cast<App::NumberExpression*>(evaluated.get());
        if (value && value->getQuantity().isDimensionless()) {
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
    catch (const Base::Exception&) {
        // Other evaluation failures can also mean that a nested additive term needs its
        // dimensionless children normalized before the expression can be evaluated.
    }

    return nullptr;
}

void applyDefaultUnitPolicyToExpression(
    App::Expression& expression,
    const Base::Unit& defaultUnit
)
{
    auto* operation = freecad_cast<App::OperatorExpression*>(&expression);
    if (!operation) {
        return;
    }

    // An explicit unit operator owns the complete subtree below it. Applying the field's default
    // unit inside that subtree would turn `(1 + 2) mm` into `(1 mm + 2 mm) mm`.
    if (operation->getOperator() == App::OperatorExpression::UNIT) {
        return;
    }

    if (operation->getOperator() == App::OperatorExpression::ADD
        || operation->getOperator() == App::OperatorExpression::SUB) {
        const auto applyToOperand = [&](App::Expression* operand, const bool left) {
            if (!operand) {
                return;
            }

            // A complete dimensionless operand inherits the field unit at this additive
            // boundary. Do not descend into multiplicative, divisive, power, or explicit-unit
            // subtrees: their internal arithmetic must retain its canonical dimensional meaning.
            if (auto* replacement = applyDefaultUnitToDimensionlessTerm(operand, defaultUnit)) {
                if (left) {
                    operation->setLeft(replacement);
                }
                else {
                    operation->setRight(replacement);
                }
                return;
            }

            // A nested additive expression may contain both dimensionless and dimensional terms
            // (for example `1 + (2 + 3 mm)`). It is its own additive boundary and can be
            // normalized recursively, but only after the complete operand was found not to be
            // dimensionless.
            const auto* nested = freecad_cast<const App::OperatorExpression*>(operand);
            if (nested && (nested->getOperator() == App::OperatorExpression::ADD
                           || nested->getOperator() == App::OperatorExpression::SUB)) {
                applyDefaultUnitPolicyToExpression(*operand, defaultUnit);
            }
        };

        applyToOperand(operation->getLeft(), true);
        applyToOperand(operation->getRight(), false);
        return;
    }
}

void applyDefaultUnitPolicy(App::Expression& expression, const Base::Unit& defaultUnit)
{
    if (defaultUnit == Base::Unit::One) {
        return;
    }

    applyDefaultUnitPolicyToExpression(expression, defaultUnit);
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
                first + (scan.diagnostic ? scan.diagnostic->offsetBytes : 0),
                scan.diagnostic ? scan.diagnostic->lengthBytes : 1
            );
        }
        if (scan.status == Base::LocalizedNumberResult::Status::Invalid) {
            return invalid(
                scan.diagnostic
                    ? inputDiagnosticKind(scan.diagnostic->kind)
                    : InputDiagnosticKind::InvalidNumber,
                first + (scan.diagnostic ? scan.diagnostic->offsetBytes : 0),
                scan.diagnostic ? scan.diagnostic->lengthBytes : 1
            );
        }
    }

    if (grammar == QuantityInputGrammar::Quantity && hasAdditiveOperator(input, path, locale)) {
        return invalid(InputDiagnosticKind::ExpressionSyntax, first);
    }

    Base::Quantity quantity;
    std::shared_ptr<Expression> parsedExpression;
    try {
        if (grammar == QuantityInputGrammar::Expression) {
            const auto* owner = path.getOwner() ? path.getDocumentObject() : nullptr;
            const std::string inputString(input);
            auto parsed = App::ExpressionParser::parseUserInput(owner, inputString.c_str(), locale);
            parsedExpression = std::shared_ptr<Expression>(std::move(parsed));
            if (!owner) {
                parsedExpression = std::shared_ptr<Expression>(
                    rewriteOwnerlessQuantityExpression(*parsedExpression, owner).release()
                );
            }
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
            if (!owner) {
                parsedExpression.reset();
            }
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
    return result;
}
