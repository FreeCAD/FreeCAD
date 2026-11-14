#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <tuple>
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
const Base::NumericLocaleContext deDE {"de_DE", ",", ".", "+", "-", 3, 3, "0"};

TEST(QuantityInput, QuantityInputUnitUsesDisplayedScaleAndSafeFallbacks)
{
    const QuantityInputUnit base {Base::Unit::Length};
    EXPECT_EQ(base.getUnit(), Base::Unit::Length);
    EXPECT_DOUBLE_EQ(base.getScale().getValue(), 1.0);
    EXPECT_EQ(base.getSymbol(), "mm");

    const QuantityInputUnit inch {Base::Unit::Length, "in"};
    EXPECT_EQ(inch.getUnit(), Base::Unit::Length);
    EXPECT_DOUBLE_EQ(inch.getScale().getValue(), 25.4);
    EXPECT_EQ(inch.getSymbol(), "in");

    for (const auto& displaySymbol : {std::string {}, std::string {"bogus"}, std::string {"kg"}}) {
        const QuantityInputUnit fallback {Base::Unit::Length, displaySymbol};
        EXPECT_EQ(fallback.getUnit(), Base::Unit::Length);
        EXPECT_DOUBLE_EQ(fallback.getScale().getValue(), 1.0);
        EXPECT_EQ(fallback.getSymbol(), "mm");
    }

    const QuantityInputUnit dimensionless {Base::Unit::One, "in"};
    EXPECT_TRUE(dimensionless.isDimensionless());
    EXPECT_TRUE(dimensionless.getSymbol().empty());
}

TEST(QuantityInput, AppliesDisplayedScaleToQuantitiesAndExpressions)
{
    tests::initApplication();
    ScopedExpressionOwner owner;
    const auto path = owner.path();
    QuantityConstraints constraints;
    constraints.requiredUnit = Base::Unit::Length;
    const QuantityInputUnit inch {Base::Unit::Length, "in"};
    const QuantityInputUnit millimetre {Base::Unit::Length, "mm"};

    for (const auto& [input, expected] :
         {std::pair {"3", 76.2}, std::pair {"3 mm", 3.0}, std::pair {"3 in", 76.2}}) {
        const auto result = App::interpretQuantityInput(
            input,
            QuantityInputGrammar::Quantity,
            path,
            inch,
            enUs,
            InputPhase::Commit,
            constraints
        );
        ASSERT_EQ(result.status, InputStatus::Acceptable) << input;
        ASSERT_TRUE(result.quantity);
        EXPECT_DOUBLE_EQ(result.quantity->getValue(), expected) << input;
        EXPECT_EQ(result.quantity->getUnit(), Base::Unit::Length);
    }

    const auto interpret = [&](const std::string_view input, const QuantityInputUnit& implicitUnit) {
        return App::interpretQuantityInput(
            input,
            QuantityInputGrammar::Expression,
            path,
            implicitUnit,
            enUs,
            InputPhase::Commit,
            constraints
        );
    };

    for (const auto& [input, expected, expectedExpression] :
         {std::tuple {"1", 25.4, "1 in"},
          std::tuple {"1 + 2", 76.2, "1 in + 2 in"},
          std::tuple {"1 + 25.4 mm", 50.8, "1 in + 25.4 mm"},
          std::tuple {"2 * 3", 152.4, "(2 * 3) in"},
          std::tuple {"(1 + 2) mm", 3.0, "(1 + 2) mm"}}) {
        const auto result = interpret(input, inch);
        ASSERT_EQ(result.status, InputStatus::Acceptable) << input;
        ASSERT_TRUE(result.quantity);
        ASSERT_TRUE(result.expression);
        EXPECT_DOUBLE_EQ(result.quantity->getValue(), expected) << input;
        EXPECT_EQ(result.quantity->getUnit(), Base::Unit::Length);
        EXPECT_EQ(result.expression->toString(), expectedExpression) << input;

        const auto retainedEvaluation = result.expression->eval();
        const auto* retainedQuantity = freecad_cast<App::NumberExpression*>(retainedEvaluation.get());
        ASSERT_NE(retainedQuantity, nullptr) << input;
        EXPECT_DOUBLE_EQ(retainedQuantity->getValue(), result.quantity->getValue()) << input;
        EXPECT_EQ(retainedQuantity->getUnit(), result.quantity->getUnit()) << input;

        const auto serialized = result.expression->toString();
        const auto reparsed
            = App::ExpressionParser::parse(path.getDocumentObject(), serialized.c_str());
        const auto evaluated = reparsed->eval();
        const auto* reparsedQuantity = freecad_cast<App::NumberExpression*>(evaluated.get());
        ASSERT_NE(reparsedQuantity, nullptr) << serialized;
        EXPECT_DOUBLE_EQ(reparsedQuantity->getValue(), result.quantity->getValue()) << serialized;
        EXPECT_EQ(reparsedQuantity->getUnit(), result.quantity->getUnit()) << serialized;
    }

    const auto explicitInches = interpret("1 in", millimetre);
    ASSERT_EQ(explicitInches.status, InputStatus::Acceptable);
    ASSERT_TRUE(explicitInches.quantity);
    EXPECT_DOUBLE_EQ(explicitInches.quantity->getValue(), 25.4);
}

TEST(QuantityInput, EditingAndCommitDistinguishIncompleteNumbers)
{
    const ObjectIdentifier path;
    const QuantityConstraints constraints;

    EXPECT_EQ(
        interpretQuantityInput(
            "-",
            QuantityInputGrammar::QuantityFirst,
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
            QuantityInputGrammar::QuantityFirst,
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
            QuantityInputGrammar::QuantityFirst,
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
            QuantityInputGrammar::QuantityFirst,
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
                QuantityInputGrammar::QuantityFirst,
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
                QuantityInputGrammar::QuantityFirst,
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
                QuantityInputGrammar::QuantityFirst,
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
                QuantityInputGrammar::QuantityFirst,
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
        QuantityInputGrammar::QuantityFirst,
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
        QuantityInputGrammar::QuantityFirst,
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
        QuantityInputGrammar::QuantityFirst,
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
        QuantityInputGrammar::QuantityFirst,
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
        QuantityInputGrammar::QuantityFirst,
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
        QuantityInputGrammar::QuantityFirst,
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
        QuantityInputGrammar::QuantityFirst,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    EXPECT_EQ(quantity.status, InputStatus::Acceptable);
    ASSERT_TRUE(quantity.quantity);
    EXPECT_DOUBLE_EQ(quantity.quantity->getValue(), 3.0);

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
        QuantityInputGrammar::QuantityFirst,
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
        QuantityInputGrammar::Quantity,
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

using Expected = std::variant<double, InputDiagnosticKind>;

void checkWithGrammar(
    std::string_view input,
    const Expected& expected,
    QuantityInputGrammar grammar,
    const App::ObjectIdentifier& path,
    const Base::Unit& unit,
    const Base::NumericLocaleContext& locale,
    InputPhase phase,
    const QuantityConstraints& constraints
)
{
    const auto result = interpretQuantityInput(input, grammar, path, unit, locale, phase, constraints);

    if (const auto* value = std::get_if<double>(&expected)) {
        ASSERT_EQ(result.status, InputStatus::Acceptable) << input;
        ASSERT_TRUE(result.quantity);
        EXPECT_DOUBLE_EQ(result.quantity->getValue(), *value);
        EXPECT_EQ(result.quantity->getUnit(), unit);
    }
    else {
        const auto kind = std::get<InputDiagnosticKind>(expected);
        EXPECT_EQ(result.status, InputStatus::Invalid);
        ASSERT_TRUE(result.diagnostic);
        EXPECT_EQ(result.diagnostic->kind, kind);
    }
}

void checkWithGrammars(
    std::string_view input,
    const Expected& expected,
    std::initializer_list<QuantityInputGrammar> grammars,
    const App::ObjectIdentifier& path,
    const Base::Unit& unit,
    const Base::NumericLocaleContext& locale,
    InputPhase phase,
    const QuantityConstraints& constraints
)
{
    for (const auto grammar : grammars) {
        SCOPED_TRACE(static_cast<int>(grammar));
        checkWithGrammar(input, expected, grammar, path, unit, locale, phase, constraints);
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
        checkWithGrammars(
            input,
            expected,
            {QuantityInputGrammar::Expression, QuantityInputGrammar::QuantityFirst},
            path,
            Base::Unit::Length,
            enUs,
            InputPhase::Commit,
            constraints
        );
    }

    checkWithGrammars(
        "2 * 3 mm",
        6.0,
        {QuantityInputGrammar::Expression, QuantityInputGrammar::QuantityFirst},
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );

    // A unit-bearing denominator has a different meaning in expression syntax compared to quantity
    // syntax. The expression is inverse length, wherease the quantity is just a length.
    QuantityConstraints lengthConstraints;
    lengthConstraints.requiredUnit = Base::Unit::Length;

    checkWithGrammar(
        "1 / 2 mm",
        InputDiagnosticKind::IncompatibleUnit,
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        lengthConstraints
    );
    checkWithGrammar(
        "1 / 2 mm",
        0.5,
        QuantityInputGrammar::QuantityFirst,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        lengthConstraints
    );

    checkWithGrammars(
        "1 / (2 mm)",
        InputDiagnosticKind::IncompatibleUnit,
        {QuantityInputGrammar::Expression, QuantityInputGrammar::QuantityFirst},
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        lengthConstraints
    );

    checkWithGrammars(
        "(1 + 2) mm",
        3.0,
        {QuantityInputGrammar::Expression, QuantityInputGrammar::QuantityFirst},
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        lengthConstraints
    );

    checkWithGrammars(
        "1 + 2",
        3.0,
        {QuantityInputGrammar::Expression, QuantityInputGrammar::QuantityFirst},
        path,
        Base::Unit::One,
        enUs,
        InputPhase::Commit,
        constraints
    );
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

TEST(QuantityInput, UnitsPunctuationOperators)
{
    tests::initApplication();
    ScopedExpressionOwner owner;
    const auto path = owner.path();
    const QuantityConstraints constraints;

    for (const auto& [input, expected] : {
             std::pair {"-1\"+0.25\"", -19.05},
             std::pair {"-1\" + 0.25\"", -19.05},
             std::pair {"-1\" +0.25\"", -19.05},
             std::pair {"-1 \" +0.25\"", -19.05},
             std::pair {"-1in+0.25\"", -19.05},
             std::pair {"1,234.5 mm", 1234.5},
             std::pair {"1'2,500\"", 63804.8},
         }) {
        checkWithGrammars(
            input,
            expected,
            {QuantityInputGrammar::Expression, QuantityInputGrammar::QuantityFirst},
            path,
            Base::Unit::Length,
            enUs,
            InputPhase::Commit,
            constraints
        );
    }

    for (const auto& [input, expected] :
         {std::pair {"1.234,5 mm", 1234.5}, std::pair {"1'2,500\"", 368.3}}) {
        checkWithGrammars(
            input,
            expected,
            {QuantityInputGrammar::Expression, QuantityInputGrammar::QuantityFirst},
            path,
            Base::Unit::Length,
            deDE,
            InputPhase::Commit,
            constraints
        );
    }
}

TEST(QuantityInput, UnaryOperators)
{
    tests::initApplication();
    ScopedExpressionOwner owner;
    const auto path = owner.path();
    const QuantityConstraints constraints;

    for (const auto& [input, expected] :
         {std::pair {"-1\"", -25.4},
          std::pair {"+1\"", 25.4},
          std::pair {"(-1\")", -25.4},
          std::pair {"1\" + -0.25\"", 19.05},
          std::pair {"1\"+-0.25\"", 19.05},
          std::pair {"1\"+ -0.25\"", 19.05}}) {
        checkWithGrammars(
            input,
            expected,
            {QuantityInputGrammar::Expression, QuantityInputGrammar::QuantityFirst},
            path,
            Base::Unit::Length,
            enUs,
            InputPhase::Commit,
            constraints
        );
    }
}

TEST(QuantityInput, QuantityOrExpressionHasValueSemantics)
{
    tests::initApplication();
    ScopedExpressionOwner owner;
    const auto path = owner.path();
    QuantityConstraints constraints;
    constraints.requiredUnit = Base::Unit::Length;

    for (const auto& [input, expected] : {
             std::pair {"10/2 mm", 5.0},
             std::pair {"1 mm + 2", 3.0},
             std::pair {"1 mm + 2 mm", 3.0},
             std::pair {"10 mm / 2 mm", 5.0},
         }) {
        const auto result = interpretQuantityInput(
            input,
            QuantityInputGrammar::QuantityFirst,
            path,
            Base::Unit::Length,
            enUs,
            InputPhase::Commit,
            constraints
        );
        ASSERT_EQ(result.status, InputStatus::Acceptable) << input;
        ASSERT_TRUE(result.quantity);
        EXPECT_DOUBLE_EQ(result.quantity->getValue(), expected) << input;
        EXPECT_EQ(result.quantity->getUnit(), Base::Unit::Length);
        EXPECT_FALSE(result.expression) << input;
    }

    owner.setValue(2.0);
    const auto reference = interpretQuantityInput(
        "Value + 3 mm",
        QuantityInputGrammar::Expression,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    ASSERT_EQ(reference.status, InputStatus::Acceptable);
    ASSERT_TRUE(reference.quantity);
    EXPECT_DOUBLE_EQ(reference.quantity->getValue(), 5.0);
    EXPECT_TRUE(reference.expression);

    const auto quantityPrecedence = interpretQuantityInput(
        "10/2 mm",
        QuantityInputGrammar::QuantityFirst,
        path,
        Base::Unit::Length,
        enUs,
        InputPhase::Commit,
        constraints
    );
    EXPECT_FALSE(quantityPrecedence.expression);
}

TEST(QuantityInput, QuantityOrExpressionAcceptsLocalizedUnarySigns)
{
    const ObjectIdentifier path;
    const QuantityConstraints constraints;
    const Base::NumericLocaleContext custom {"custom", ".", ",", "plus", "minus", 3, 3, "0"};

    for (const auto& [input, expected] : {
             std::pair {"+1 mm", 1.0},
             std::pair {"plus1 mm", 1.0},
             std::pair {"minus1 mm", -1.0},
             std::pair {"1 + 2 mm", 3.0},
         }) {
        const auto result = interpretQuantityInput(
            input,
            QuantityInputGrammar::QuantityFirst,
            path,
            Base::Unit::Length,
            custom,
            InputPhase::Commit,
            constraints
        );
        ASSERT_EQ(result.status, InputStatus::Acceptable) << input;
        ASSERT_TRUE(result.quantity);
        EXPECT_DOUBLE_EQ(result.quantity->getValue(), expected) << input;
    }
}

}  // namespace App::QuantityInputTest
