#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Expression.h>
#include <App/ExpressionParser.h>
#include <App/ObjectIdentifier.h>
#include <App/Property.h>
#include "App/QuantityInput.h"
#include "Base/NumericFormatting.h"
#include <src/App/InitApplication.h>

namespace
{
class ScopedExpressionOwner
{
public:
    ScopedExpressionOwner()
        : documentName {App::GetApplication().getUniqueDocumentName("quantity_input")}
        , document {App::GetApplication().newDocument(documentName.c_str(), "testUser")}
        , object {document->addObject("App::GeoFeature", "Owner")}
        , property {object->addDynamicProperty("App::PropertyFloat", "Value", "Test")}
    {}

    ~ScopedExpressionOwner()
    {
        App::GetApplication().closeDocument(documentName.c_str());
    }

    App::ObjectIdentifier path() const
    {
        return App::ObjectIdentifier(*property);
    }

    void setValue(const double value)
    {
        freecad_cast<App::PropertyFloat*>(property)->setValue(value);
    }

private:
    std::string documentName;
    App::Document* document;
    App::DocumentObject* object;
    App::Property* property;
};
}  // namespace

namespace App::QuantityInputTest
{

const Base::NumericLocaleContext enUs {"en_US", ".", ",", "+", "-", 3, 3, "0"};

TEST(QuantityInput, EditingAndCommitDistinguishIncompleteNumbers)
{
    const ObjectIdentifier path;
    const QuantityConstraints constraints;

    EXPECT_EQ(
        interpretQuantityInput(
            "-",
            QuantityInputGrammar::Quantity,
            path,
            Base::Unit::Length,
            enUs,
            InputPhase::Editing,
            constraints
        )
            .status,
        InputStatus::Incomplete
    );
    EXPECT_EQ(
        interpretQuantityInput(
            "-",
            QuantityInputGrammar::Quantity,
            path,
            Base::Unit::Length,
            enUs,
            InputPhase::Commit,
            constraints
        )
            .status,
        InputStatus::Invalid
    );
    EXPECT_EQ(
        interpretQuantityInput(
            "1e",
            QuantityInputGrammar::Quantity,
            path,
            Base::Unit::Length,
            enUs,
            InputPhase::Editing,
            constraints
        )
            .status,
        InputStatus::Incomplete
    );
    EXPECT_EQ(
        interpretQuantityInput(
            "1e",
            QuantityInputGrammar::Quantity,
            path,
            Base::Unit::Length,
            enUs,
            InputPhase::Commit,
            constraints
        )
            .status,
        InputStatus::Invalid
    );

    for (const auto input : {"", "+", "1e-"}) {
        EXPECT_EQ(
            interpretQuantityInput(
                input,
                QuantityInputGrammar::Quantity,
                path,
                Base::Unit::Length,
                enUs,
                InputPhase::Editing,
                constraints
            )
                .status,
            InputStatus::Incomplete
        ) << input;
        EXPECT_EQ(
            interpretQuantityInput(
                input,
                QuantityInputGrammar::Quantity,
                path,
                Base::Unit::Length,
                enUs,
                InputPhase::Commit,
                constraints
            )
                .status,
            InputStatus::Invalid
        ) << input;
    }

    for (const auto input : {"1,2", "1.2.3"}) {
        EXPECT_EQ(
            interpretQuantityInput(
                input,
                QuantityInputGrammar::Quantity,
                path,
                Base::Unit::Length,
                enUs,
                InputPhase::Editing,
                constraints
            )
                .status,
            InputStatus::Invalid
        ) << input;
        EXPECT_EQ(
            interpretQuantityInput(
                input,
                QuantityInputGrammar::Quantity,
                path,
                Base::Unit::Length,
                enUs,
                InputPhase::Commit,
                constraints
            )
                .status,
            InputStatus::Invalid
        ) << input;
    }
}

TEST(QuantityInput, TrailingGroupingSeparatorFollowsTheInputPhase)
{
    const ObjectIdentifier path;
    const QuantityConstraints constraints;

    const auto editing = interpretQuantityInput(
        "12,",
        QuantityInputGrammar::Quantity,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Editing,
        constraints
    );
    ASSERT_EQ(editing.status, InputStatus::Incomplete);
    ASSERT_TRUE(editing.diagnostic);
    EXPECT_EQ(editing.diagnostic->kind, InputDiagnosticKind::IncompleteNumber);

    const auto commit = interpretQuantityInput(
        "12,",
        QuantityInputGrammar::Quantity,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(commit.status, InputStatus::Invalid);
    ASSERT_TRUE(commit.diagnostic);
    EXPECT_EQ(commit.diagnostic->kind, InputDiagnosticKind::IncompleteNumber);
}

TEST(QuantityInput, ReportsGroupingAndUnitDiagnostics)
{
    const ObjectIdentifier path;
    const QuantityConstraints constraints;

    const auto malformed = interpretQuantityInput(
        "12,34,567 mm",
        QuantityInputGrammar::Quantity,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(malformed.status, InputStatus::Invalid);
    ASSERT_TRUE(malformed.diagnostic);
    EXPECT_EQ(malformed.diagnostic->kind, InputDiagnosticKind::MalformedGrouping);

    const auto plainSpace = interpretQuantityInput(
        "12 345 mm",
        QuantityInputGrammar::Quantity,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(plainSpace.status, InputStatus::Invalid);
    ASSERT_TRUE(plainSpace.diagnostic);
    EXPECT_EQ(plainSpace.diagnostic->kind, InputDiagnosticKind::MalformedGrouping);

    QuantityConstraints restricted;
    restricted.requiredUnit = Base::Unit::TimeSpan;
    const auto incompatible = interpretQuantityInput(
        "10 mm",
        QuantityInputGrammar::Quantity,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        restricted
    );
    ASSERT_EQ(incompatible.status, InputStatus::Invalid);
    ASSERT_TRUE(incompatible.diagnostic);
    EXPECT_EQ(incompatible.diagnostic->kind, InputDiagnosticKind::IncompatibleUnit);
}

TEST(QuantityInput, AcceptsGroupedQuantityAndNormalizesIt)
{
    const ObjectIdentifier path;
    const QuantityConstraints constraints;
    const auto result = interpretQuantityInput(
        "12,345.67 mm",
        QuantityInputGrammar::Quantity,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );

    ASSERT_EQ(result.status, InputStatus::Acceptable);
    ASSERT_TRUE(result.quantity);
    EXPECT_DOUBLE_EQ(result.quantity->getValue(), 12345.67);
}

TEST(QuantityInput, ReportsExpressionEvaluationAndRangeDiagnostics)
{
    tests::initApplication();
    ScopedExpressionOwner owner;
    const auto path = owner.path();
    const QuantityConstraints constraints;

    const auto syntax = interpretQuantityInput(
        "1 +",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(syntax.status, InputStatus::Invalid);
    ASSERT_TRUE(syntax.diagnostic);
    EXPECT_EQ(syntax.diagnostic->kind, InputDiagnosticKind::ExpressionSyntax);

    const auto evaluation = interpretQuantityInput(
        "str(1)",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(evaluation.status, InputStatus::Invalid);
    ASSERT_TRUE(evaluation.diagnostic);
    EXPECT_EQ(evaluation.diagnostic->kind, InputDiagnosticKind::Evaluation);

    QuantityConstraints bounded;
    bounded.maximum = 10.0;
    const auto outOfRange = interpretQuantityInput(
        "11 mm",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        bounded
    );
    ASSERT_EQ(outOfRange.status, InputStatus::Invalid);
    ASSERT_TRUE(outOfRange.diagnostic);
    EXPECT_EQ(outOfRange.diagnostic->kind, InputDiagnosticKind::OutOfRange);
}

TEST(QuantityInput, GrammarSelectionIsExplicit)
{
    tests::initApplication();
    ScopedExpressionOwner owner;
    const auto path = owner.path();
    const QuantityConstraints constraints;

    const auto quantity = interpretQuantityInput(
        "1+2 mm",
        QuantityInputGrammar::Quantity,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    EXPECT_EQ(quantity.status, InputStatus::Invalid);

    const auto expression = interpretQuantityInput(
        "1+2 mm",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(expression.status, InputStatus::Acceptable);
    ASSERT_TRUE(expression.quantity);
    EXPECT_DOUBLE_EQ(expression.quantity->getValue(), 3.0);

    const auto comment = interpretQuantityInput(
        "1 mm [original input 12,34,567]",
        QuantityInputGrammar::Quantity,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(comment.status, InputStatus::Acceptable);
    ASSERT_TRUE(comment.quantity);
    EXPECT_DOUBLE_EQ(comment.quantity->getValue(), 1.0);
}

TEST(QuantityInput, PositiveSignsAndCompactAdditionRemainExpressionSyntax)
{
    tests::initApplication();
    ScopedExpressionOwner owner;
    const auto path = owner.path();
    const QuantityConstraints constraints;

    const auto positive = interpretQuantityInput(
        "+1 mm",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(positive.status, InputStatus::Acceptable);
    ASSERT_TRUE(positive.quantity);
    EXPECT_DOUBLE_EQ(positive.quantity->getValue(), 1.0);

    const auto compact = interpretQuantityInput(
        "1+2 mm",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(compact.status, InputStatus::Acceptable);
    ASSERT_TRUE(compact.quantity);
    EXPECT_DOUBLE_EQ(compact.quantity->getValue(), 3.0);
    EXPECT_EQ(compact.quantity->getUnit(), Base::Unit::Length);

    const Base::NumericLocaleContext custom {"custom", ".", ",", "plus", "minus", 3, 3, "0"};
    const auto localizedPlus = interpretQuantityInput(
        "plus1 mm",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        custom,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(localizedPlus.status, InputStatus::Acceptable);
    ASSERT_TRUE(localizedPlus.quantity);
    EXPECT_DOUBLE_EQ(localizedPlus.quantity->getValue(), 1.0);

    const auto localizedMinus = interpretQuantityInput(
        "minus1 mm",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        custom,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(localizedMinus.status, InputStatus::Acceptable);
    ASSERT_TRUE(localizedMinus.quantity);
    EXPECT_DOUBLE_EQ(localizedMinus.quantity->getValue(), -1.0);

    const Base::NumericLocaleContext persian {"fa_IR", "٫", "٬", "+", "−", 3, 3, "۰"};
    const auto localizedCompact = interpretQuantityInput(
        "۱+۲ mm",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        persian,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(localizedCompact.status, InputStatus::Acceptable);
    ASSERT_TRUE(localizedCompact.quantity);
    EXPECT_DOUBLE_EQ(localizedCompact.quantity->getValue(), 3.0);

    for (const auto& [input, expected] :
         {std::pair {"1-2 mm", -1.0},
          std::pair {"1+-2 mm", -1.0},
          std::pair {"1e+2 mm", 100.0},
          std::pair {"1e-2 mm", 0.01}}) {
        const auto result = interpretQuantityInput(
            input,
            QuantityInputGrammar::Expression,
            path,
            Base::Unit::Length,
            enUs,
            InputPhase::Commit,
            constraints
        );
        ASSERT_EQ(result.status, InputStatus::Acceptable) << input;
        ASSERT_TRUE(result.quantity);
        EXPECT_DOUBLE_EQ(result.quantity->getValue(), expected);
    }

    for (const auto& [input, expected] :
         {std::pair {"-(1 mm)", -1.0},
          std::pair {"+(1 mm)", 1.0},
          std::pair {"-sqrt(4) * 1 mm", -2.0},
          std::pair {"-(1 + 2) mm", -3.0}}) {
        const auto result = interpretQuantityInput(
            input,
            QuantityInputGrammar::Expression,
            path,
            Base::Unit::Length,
            enUs,
            InputPhase::Commit,
            constraints
        );
        ASSERT_EQ(result.status, InputStatus::Acceptable) << input;
        ASSERT_TRUE(result.quantity);
        EXPECT_DOUBLE_EQ(result.quantity->getValue(), expected);
        EXPECT_EQ(result.quantity->getUnit(), Base::Unit::Length);
    }
}

TEST(QuantityInput, DefaultUnitAppliesToBareAdditiveTerms)
{
    tests::initApplication();
    ScopedExpressionOwner owner;
    const auto path = owner.path();
    const QuantityConstraints constraints;

    for (const auto& [input, expected] :
         {std::pair {"1 + 2 mm", 3.0},
          std::pair {"1 mm + 2", 3.0},
          std::pair {"1 + 2", 3.0},
          std::pair {"(1 * 2) + 3 mm", 5.0},
          std::pair {"sqrt(4) + 3 mm", 5.0},
          std::pair {"1 + (2 + 3 mm)", 6.0},
          std::pair {"(1 + 2) mm + 3", 6.0},
          std::pair {"1 mm + (2 + 3)", 6.0},
          std::pair {"(1 + 2) * 3 mm", 9.0},
          std::pair {"3 mm * (1 + 2)", 9.0},
          std::pair {"(4 - 1) / 3 * 6 mm", 6.0}}) {
        const auto result = interpretQuantityInput(
            input,
            QuantityInputGrammar::Expression,
            path,
            Base::Unit::Length,
            enUs,
            InputPhase::Commit,
            constraints
        );
        ASSERT_EQ(result.status, InputStatus::Acceptable) << input;
        ASSERT_TRUE(result.quantity);
        EXPECT_DOUBLE_EQ(result.quantity->getValue(), expected);
        EXPECT_EQ(result.quantity->getUnit(), Base::Unit::Length);
    }

    const auto multiplied = interpretQuantityInput(
        "2 * 3 mm",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(multiplied.status, InputStatus::Acceptable);
    ASSERT_TRUE(multiplied.quantity);
    EXPECT_DOUBLE_EQ(multiplied.quantity->getValue(), 6.0);
    EXPECT_EQ(multiplied.quantity->getUnit(), Base::Unit::Length);

    // A unit-bearing denominator is explicit expression syntax. Both forms are inverse length
    // and therefore incompatible with this length field.
    QuantityConstraints lengthConstraints;
    lengthConstraints.requiredUnit = Base::Unit::Length;

    const auto dividedNumber = interpretQuantityInput(
        "1 / 2 mm",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        lengthConstraints
    );
    EXPECT_EQ(dividedNumber.status, InputStatus::Invalid);
    ASSERT_TRUE(dividedNumber.diagnostic);
    EXPECT_EQ(dividedNumber.diagnostic->kind, InputDiagnosticKind::IncompatibleUnit);

    const auto dividedQuantity = interpretQuantityInput(
        "1 / (2 mm)",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        lengthConstraints
    );
    EXPECT_EQ(dividedQuantity.status, InputStatus::Invalid);
    ASSERT_TRUE(dividedQuantity.diagnostic);
    EXPECT_EQ(dividedQuantity.diagnostic->kind, InputDiagnosticKind::IncompatibleUnit);

    const auto parenthesized = interpretQuantityInput(
        "(1 + 2) mm",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        lengthConstraints
    );
    ASSERT_EQ(parenthesized.status, InputStatus::Acceptable);
    ASSERT_TRUE(parenthesized.quantity);
    EXPECT_DOUBLE_EQ(parenthesized.quantity->getValue(), 3.0);
    EXPECT_EQ(parenthesized.quantity->getUnit(), Base::Unit::Length);

    const auto dimensionless = interpretQuantityInput(
        "1 + 2",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::One,
        enUs,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(dimensionless.status, InputStatus::Acceptable);
    ASSERT_TRUE(dimensionless.quantity);
    EXPECT_DOUBLE_EQ(dimensionless.quantity->getValue(), 3.0);
    EXPECT_EQ(dimensionless.quantity->getUnit(), Base::Unit::One);
}

TEST(QuantityInput, RejectsMalformedGroupingAfterExpressionOperand)
{
    const ObjectIdentifier path;
    const QuantityConstraints constraints;
    const auto result = interpretQuantityInput(
        "1 mm + 12,34,567 mm",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );

    EXPECT_EQ(result.status, InputStatus::Invalid);
}

TEST(QuantityInput, DefaultUnitPreservesExpressionDependencies)
{
    tests::initApplication();
    ScopedExpressionOwner owner;
    owner.setValue(2.0);
    const auto path = owner.path();
    const QuantityConstraints constraints;

    const auto result = interpretQuantityInput(
        "Value + 3 mm",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(result.status, InputStatus::Acceptable);
    ASSERT_TRUE(result.quantity);
    ASSERT_TRUE(result.expression);
    EXPECT_DOUBLE_EQ(result.quantity->getValue(), 5.0);
    EXPECT_EQ(result.quantity->getUnit(), Base::Unit::Length);

    const auto dependencies = result.expression->getDeps();
    const auto objectDependencies = dependencies.find(path.getDocumentObject());
    ASSERT_NE(objectDependencies, dependencies.end());
    EXPECT_TRUE(objectDependencies->second.contains("Value"));

    owner.setValue(4.0);
    const auto reevaluated = result.expression->eval();
    const auto* quantity = freecad_cast<App::NumberExpression*>(reevaluated.get());
    ASSERT_NE(quantity, nullptr);
    EXPECT_DOUBLE_EQ(quantity->getValue(), 7.0);
    EXPECT_EQ(quantity->getUnit(), Base::Unit::Length);

    const auto updatedDependencies = result.expression->getDeps();
    const auto updatedObjectDependencies = updatedDependencies.find(path.getDocumentObject());
    ASSERT_NE(updatedObjectDependencies, updatedDependencies.end());
    EXPECT_TRUE(updatedObjectDependencies->second.contains("Value"));
}

TEST(QuantityInput, DefaultUnitDoesNotRewriteNestedMultiplicativeSubexpressions)
{
    tests::initApplication();
    ScopedExpressionOwner owner;
    owner.setValue(2.0);
    const auto path = owner.path();
    const QuantityConstraints constraints;

    const auto result = interpretQuantityInput(
        "(Value + 1) * 3 mm",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(result.status, InputStatus::Acceptable);
    ASSERT_TRUE(result.quantity);
    ASSERT_TRUE(result.expression);
    EXPECT_DOUBLE_EQ(result.quantity->getValue(), 9.0);
    EXPECT_EQ(result.quantity->getUnit(), Base::Unit::Length);

    const auto dependencies = result.expression->getDeps();
    const auto objectDependencies = dependencies.find(path.getDocumentObject());
    ASSERT_NE(objectDependencies, dependencies.end());
    EXPECT_TRUE(objectDependencies->second.contains("Value"));

    owner.setValue(4.0);
    const auto reevaluated = result.expression->eval();
    const auto* quantity = freecad_cast<App::NumberExpression*>(reevaluated.get());
    ASSERT_NE(quantity, nullptr);
    EXPECT_DOUBLE_EQ(quantity->getValue(), 15.0);
    EXPECT_EQ(quantity->getUnit(), Base::Unit::Length);
}

TEST(QuantityInput, DefaultUnitExpressionsRoundTripThroughSerialization)
{
    tests::initApplication();
    ScopedExpressionOwner owner;
    owner.setValue(2.0);
    const auto path = owner.path();
    const QuantityConstraints constraints;

    for (const auto& [input, expected] :
         {std::pair {"Value + 3 mm", 5.0}, std::pair {"sqrt(4) + 3 mm", 5.0}}) {
        const auto result = interpretQuantityInput(
            input,
            QuantityInputGrammar::Expression,
            path,
            Base::Unit::Length,
            enUs,
            InputPhase::Commit,
            constraints
        );
        ASSERT_EQ(result.status, InputStatus::Acceptable) << input;
        ASSERT_TRUE(result.expression);

        const auto serialized = result.expression->toString();
        const auto reparsed
            = App::ExpressionParser::parse(path.getDocumentObject(), serialized.c_str());
        const auto evaluated = reparsed->eval();
        const auto* quantity = freecad_cast<App::NumberExpression*>(evaluated.get());
        ASSERT_NE(quantity, nullptr) << serialized;
        EXPECT_DOUBLE_EQ(quantity->getValue(), expected) << serialized;
        EXPECT_EQ(quantity->getUnit(), Base::Unit::Length) << serialized;

        if (std::string_view {input} == "Value + 3 mm") {
            const auto dependencies = reparsed->getDeps();
            const auto objectDependencies = dependencies.find(path.getDocumentObject());
            ASSERT_NE(objectDependencies, dependencies.end());
            EXPECT_TRUE(objectDependencies->second.contains("Value"));
        }
    }
}

TEST(QuantityInput, EditingAndCommitExposeTheSameInvalidGrouping)
{
    const ObjectIdentifier path;
    const QuantityConstraints constraints;
    for (const auto phase : {InputPhase::Editing, InputPhase::Commit}) {
        const auto result = interpretQuantityInput(
            "12,34,567 mm",
            QuantityInputGrammar::Expression,
            path,
            Base::Unit::Length,
            enUs,
            phase,
            constraints
        );
        EXPECT_EQ(result.status, InputStatus::Invalid);
        ASSERT_TRUE(result.diagnostic);
        EXPECT_EQ(result.diagnostic->kind, InputDiagnosticKind::MalformedGrouping);
    }
}

TEST(QuantityInput, SharedLocalizedInputCorpus)
{
    tests::initApplication();
    ScopedExpressionOwner owner;
    const ObjectIdentifier quantityPath;
    const auto expressionPath = owner.path();
    const QuantityConstraints constraints;

    struct ConsumerCase
    {
        std::string_view input;
        QuantityInputGrammar grammar;
        ObjectIdentifier path;
        InputStatus status;
        std::optional<double> value;
    };

    const std::vector<ConsumerCase> cases {
        {"12,345.67 mm", QuantityInputGrammar::Quantity, quantityPath, InputStatus::Acceptable, 12345.67},
        {"12,34,567 mm", QuantityInputGrammar::Quantity, quantityPath, InputStatus::Invalid, std::nullopt},
        {"12 345 mm", QuantityInputGrammar::Quantity, quantityPath, InputStatus::Invalid, std::nullopt},
        {"1+2 mm", QuantityInputGrammar::Expression, expressionPath, InputStatus::Acceptable, 3.0},
        {"1 + 2 mm", QuantityInputGrammar::Expression, expressionPath, InputStatus::Acceptable, 3.0},
        {"12,34,567 mm", QuantityInputGrammar::Expression, expressionPath, InputStatus::Invalid, std::nullopt}
    };

    for (const auto& testCase : cases) {
        const auto result = interpretQuantityInput(
            testCase.input,
            testCase.grammar,
            testCase.path,
            Base::Unit::Length,
            enUs,
            InputPhase::Commit,
            constraints
        );
        ASSERT_EQ(result.status, testCase.status) << testCase.input;
        if (testCase.value) {
            ASSERT_TRUE(result.quantity);
            EXPECT_DOUBLE_EQ(result.quantity->getValue(), *testCase.value);
        }
    }
}

}  // namespace App::QuantityInputTest
