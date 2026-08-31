// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Base/Quantity.h"

#include "App/Application.h"
#include "App/Document.h"
#include "App/DocumentObject.h"
#include "App/Expression.h"
#include "App/ExpressionParser.h"
#include "App/ExpressionPrattParser.h"

#include "src/App/InitApplication.h"

using namespace testing;

// the following two PrintTo functions are optional but provide for *much*
// nicer error messages and make debugging test failures much easier
namespace Base
{
void PrintTo(const Quantity& qty, std::ostream* os)
{
    *os << qty.toString(QuantityFormat(QuantityFormat::NumberFormat::Fixed));
}
}  // namespace Base

namespace boost
{
void PrintTo(const boost::any& e, std::ostream* os)
{
    *os << "any (";
    if (e.type() == typeid(Base::Quantity)) {
        const auto& qty = App::any_cast<const Base::Quantity>(e);
        *os << "Quantity=";
        PrintTo(qty, os);
    }
    else if (e.type() == typeid(double)) {
        *os << "double=" << App::any_cast<const double>(e);
    }
    else if (e.type() == typeid(long)) {
        *os << "long=" << App::any_cast<const long>(e);
    }
    else if (e.type() == typeid(std::string)) {
        *os << "string=\"" << App::any_cast<const std::string>(e) << "\"";
    }
    else if (e.type() == typeid(Base::ParserError)) {
        const auto& err = App::any_cast<const Base::ParserError>(e);
        *os << "ParserError=\"" << err.what() << "\"";
    }
    else {
        *os << "unknown type=" << e.type().name();
    }
    *os << ")";
}
}  // namespace boost

namespace App::ExpressionParser::Test
{

class ExpressionParserTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        docName = App::GetApplication().getUniqueDocumentName("test");
        thisDoc = App::GetApplication().newDocument(docName.c_str(), "testUser");
        thisObj = thisDoc->addObject("App::FeaturePython", "Sketch");
        thisObj->addDynamicProperty("App::PropertyPlacement", "Placement");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(docName.c_str());
    }

    // clang-format off
    std::string doc_name() { return docName; }
    App::Document* this_doc() { return thisDoc; }
    App::DocumentObject* this_obj() { return thisObj; }
    // clang-format on

    Base::Quantity parse_expression_text_as_quantity(const char* expression_text)
    {
        const auto expression = parse(thisObj, expression_text);
        return App::any_cast<Base::Quantity>(expression->getValueAsAny());
    }

    Base::Quantity parse_quantity_text_as_quantity(const char* quantity_text)
    {
        return Base::Quantity::parse(quantity_text);
    }

    boost::any parseExpr(const char* text)
    {
        try {
            const auto expression = parse(thisObj, text);
            ExpressionPtr simplified = expression->simplify();
            return simplified->getValueAsAny();
        }
        catch (const Base::ParserError& e) {
            return e;
        }
        catch (const Expression::Exception& e) {
            return e;
        }
        catch (const Base::ExpressionError& e) {
            return e;
        }
        catch (const Base::RuntimeError& e) {
            return e;
        }
        catch (const Base::Exception& e) {
            // provide a much friendlier error message before failing
            EXPECT_TRUE(false) << "Unexpected Base::Exception when parsing \"" << text
                               << "\": " << e.what();
            throw;
        }
    }

    std::string simplifiedValue(const char* text, bool usePratt)
    {
        auto expression = usePratt ? Pratt::Detail::parseFlexTokenStream(thisObj, text)
                                   : parse(thisObj, text);
        const auto value = expression->simplify()->getValueAsAny();
        std::ostringstream output;
        boost::PrintTo(value, &output);
        return output.str();
    }

    Base::Quantity parsePrattQuantity(const char* text)
    {
        auto expression = Pratt::Detail::parseFlexTokenStream(thisObj, text);
        return App::any_cast<Base::Quantity>(expression->simplify()->getValueAsAny());
    }

    std::string parsedText(const char* text, bool usePratt)
    {
        const auto expression = usePratt ? Pratt::Detail::parseFlexTokenStream(thisObj, text)
                                         : parse(thisObj, text);
        return expression->toString();
    }

    template<typename Callable>
    static std::string exceptionCategory(Callable&& callable)
    {
        try {
            callable();
            return "none";
        }
        catch (const Base::ParserError&) {
            return "ParserError";
        }
        catch (const Expression::Exception&) {
            return "Expression::Exception";
        }
        catch (const Base::ExpressionError&) {
            return "ExpressionError";
        }
        catch (const Base::RuntimeError&) {
            return "RuntimeError";
        }
        catch (const Base::Exception&) {
            return "Base::Exception";
        }
    }

private:
    std::string docName;
    App::Document* thisDoc {};
    App::DocumentObject* thisObj {};
};

// clang-format off
static constexpr auto IsQuantity = [](auto m) { return AnyWith<Base::Quantity>(m); };
static constexpr auto IsDouble = [](auto m) { return AnyWith<double>(m); };
static constexpr auto IsLong = [](auto m) { return AnyWith<long>(m); };
static constexpr auto IsString = [](auto m) { return AnyWith<std::string>(m); };
static constexpr auto IsParserError = [](auto m) { return AnyWith<Base::ParserError>(m); };
static constexpr auto IsRuntimeError = [](auto m) { return AnyWith<Base::RuntimeError>(m); };
static constexpr auto IsExpressionException = [](auto m) { return AnyWith<Expression::Exception>(m); };
static constexpr auto IsExpressionError = [](auto m) { return AnyWith<Base::ExpressionError>(m); };
static Base::Quantity mm3(double val) { return Base::Quantity(val, Base::Unit::Volume); }
static Base::Quantity mm2(double val) { return Base::Quantity(val, Base::Unit::Area); }
static Base::Quantity mm(double val) { return Base::Quantity(val, Base::Unit::Length); }
// clang-format on

// https://github.com/FreeCAD/FreeCAD/issues/11965
TEST_F(ExpressionParserTest, functionPARSEQUANT)
{
    // clang-format off
    EXPECT_ANY_THROW(App::ExpressionParser::parse(this_obj(), "parsequant()")) << "should not parse empty";

    EXPECT_NO_THROW(App::ExpressionParser::parse(this_obj(), "parsequant(1 mm)")) << "should parse simple quantity";
    EXPECT_NO_THROW(App::ExpressionParser::parse(this_obj(), "parsequant(<<(1 + 2) m>>)")) << "should parse literal quantity";
    EXPECT_NO_THROW(App::ExpressionParser::parse(this_obj(), "parsequant(str(1 m + 2 mm))")) << "should parse str-function quantity";

    EXPECT_ANY_THROW(parse_quantity_text_as_quantity("parsequant(1 mm)")) << "should not treat parsequant-function as quantity";
    EXPECT_EQ(parse_quantity_text_as_quantity("1 mm"), parse_quantity_text_as_quantity("1 mm")) << "equality sanity check";
    EXPECT_NE(parse_quantity_text_as_quantity("1 mm"), parse_quantity_text_as_quantity("2 mm")) << "inequality sanity check";

    std::initializer_list<std::pair<const char*,const char*>> expression_vs_quantity_list = {
        // length
        { "1 mm", "1 mm" },
        { "parsequant(1 mm)", "1 mm" },
        { "parsequant(<<(1 + 2) m>>)", "3000 mm" },
        { "parsequant(str(1 m + 2 mm))", "1002 mm"},
        // angle
        { "10 deg", "10 deg" },
        { "parsequant(10 deg)", "10 deg" },
        { "parsequant(<<(10 + 20) deg>>)", "30 deg" },
        { "parsequant(str(10 deg + 20 deg))", "30 deg" },
        // mass
        { "10 g", "10 g" },
        { "parsequant(10 g)", "10 g" },
        { "parsequant(<<(10 + 20) kg>>)", "30000 g" },
        { "parsequant(str(10 kg + 20010 g))", "30.01 kg" },
    };

    for (const auto& [expression_text, quantity_text] : expression_vs_quantity_list) {
        auto expression_result = parse_expression_text_as_quantity(expression_text);
        auto quantity_result = parse_quantity_text_as_quantity(quantity_text);

        EXPECT_EQ(expression_result, quantity_result) << "mismatch:"
            " expression_text='" + std::string(expression_text) + "'"
            " quantity_text='" + std::string(quantity_text) + "'"
            " expression_representation='" + expression_result.getUserString() + "'"
            " quantity_representation='" + quantity_result.getUserString() + "'"
        ;
    }
    // clang-format on
}

TEST_F(ExpressionParserTest, isTokenAConstant)
{
    for (const auto& constant : {"pi", "e", "True", "False", "true", "false", "None"}) {
        EXPECT_TRUE(isTokenAConstant(constant))
            << "\"" << constant << "\" did not evaluate as a constant";
    }

    for (const auto& nonConstant : {"PI", "E", "TRUE", "FALSE", "NONE", "none"}) {
        EXPECT_FALSE(isTokenAConstant(nonConstant))
            << "\"" << nonConstant << "\" evaluated as a constant";
    }
}

TEST_F(ExpressionParserTest, simpleExpressionsParse)
{
    static constexpr auto IsMm = [](auto m) {
        return IsQuantity(mm(m));
    };

    EXPECT_THAT(parseExpr("0 mm"), IsMm(0));
    EXPECT_THAT(parseExpr("-5 mm"), IsMm(-5)) << "leading minus sign";
    EXPECT_THAT(parseExpr("+5 mm"), IsMm(5)) << "leading plus sign";
    EXPECT_THAT(parseExpr("1.25 mm"), IsMm(1.25)) << "decimal";
    EXPECT_THAT(parseExpr("1.0e3 mm"), IsMm(1000)) << "scientific notation";
    EXPECT_THAT(parseExpr("1mm + 1mm"), IsMm(2)) << "adding same units";
    EXPECT_THAT(parseExpr("5mm +- 1mm"), IsMm(4))
        << "adding same units with poorly separate negation sign";
    EXPECT_THAT(parseExpr("1mm * 3"), IsMm(3)) << "multiplying units by factor";
    EXPECT_THAT(parseExpr("1 m"), IsMm(1000)) << "different units";
    EXPECT_THAT(parseExpr("1 m + 25 mm"), IsMm(1025)) << "addition with unit conversion";
    EXPECT_THAT(parseExpr("(5mm + 1cm) / 3"), IsMm(5))
        << "parenthesised arithmetic with different units";

    EXPECT_THAT(parseExpr("10 deg"), IsQuantity(Base::Quantity(10, Base::Unit::Angle)))
        << "angle literal";
    EXPECT_THAT(parseExpr("360 deg + pi rad"), IsQuantity(Base::Quantity(540, Base::Unit::Angle)))
        << "mixed angle units";

    EXPECT_THAT(parseExpr("True mm"), IsQuantity(mm(1))) << "boolean constant treated as scalar";
}

TEST_F(ExpressionParserTest, badExpressionsDoNotParse)
{
    EXPECT_THAT(parseExpr("mm"), IsExpressionException(_))
        << "only units - cannot evaludate to a value";
    EXPECT_THAT(parseExpr("-"), IsParserError(_)) << "only operator";
    EXPECT_THAT(parseExpr("10 bogusunits"), IsParserError(_));
    EXPECT_THAT(parseExpr("1.25 mm kg"), IsParserError(_)) << "units separated by space";
    EXPECT_THAT(parseExpr("*1"), IsParserError(_)) << "operator then unit";
    EXPECT_THAT(parseExpr("sqrt"), IsRuntimeError(_))
        << "function without arguments fails ObjectIdentifier resolution";
    EXPECT_THAT(parseExpr("sqrt()"), IsParserError(_)) << "function with empty arguments";
    EXPECT_THAT(parseExpr("sqrt(1, 2)"), IsExpressionError(_))
        << "function with too many arguments";
}

TEST_F(ExpressionParserTest, expressionsWithMultiplyDivideParse)
{
    EXPECT_THAT(parseExpr("1 mm * 2 cm"), IsQuantity(mm2(20))) << "mixed-unit multiplication";
    EXPECT_THAT(parseExpr("2 cm * 1 mm"), IsQuantity(mm2(20))) << "multiplication commutativity";
    EXPECT_THAT(parseExpr("(1 m - 750 mm) * 3"), IsQuantity(mm(750)))
        << "binary arithmetic surrounded by another operator";
    EXPECT_THAT(parseExpr("(5 mm)^2"), IsQuantity(mm2(25)))
        << "power operator applied to parenthesised unit";
    EXPECT_THAT(
        parseExpr("24 V / (2 A)"),
        IsQuantity(Base::Quantity(12'000'000, Base::Unit::ElectricalResistance))
    ) << "division of electrical quantities";
}

TEST_F(ExpressionParserTest, expressionsWithUnitsMultipliedDontParse_LikelyBug)
{
    // https://github.com/FreeCAD/FreeCAD/issues/14471
    // https://github.com/FreeCAD/FreeCAD/issues/26470
    EXPECT_THAT(parseExpr("10 mm * kg"), IsParserError(_));
    EXPECT_THAT(parseExpr("1234000.00 mm*kg/s^2"), IsParserError(_));
}

TEST_F(ExpressionParserTest, dimensionlessExpressionsParseAsLongOrDouble)
{
    EXPECT_THAT(parseExpr("0"), IsLong(0)) << "bare zero parses as long";
    EXPECT_THAT(parseExpr("1 / 2"), IsDouble(0.5)) << "simple fraction parses as double";
    EXPECT_THAT(parseExpr("40 mm / (2 cm)"), IsLong(2.0))
        << "dimensionless ratio of like units parses as long";
}

TEST_F(ExpressionParserTest, expressionsWithFunctionsParse)
{
    EXPECT_THAT(parseExpr("sqrt(9 mm^2)"), IsQuantity(mm(3)))
        << "function invocation with unit exponent";
    EXPECT_THAT(parseExpr("pow(2 mm, 3)"), IsQuantity(mm3(8)))
        << "power function returning a cubic unit";
    EXPECT_THAT(parseExpr("sum(1 mm, 2 mm, 3 mm)"), IsQuantity(mm(6)))
        << "aggregate function operating on quantities";
    EXPECT_THAT(parseExpr("list(1 mm, 2 mm)[0]"), IsParserError(_))
        << "indexing into list-valued expression";
    EXPECT_THAT(parseExpr("<<(Line 1\\nLine 2)>>"), IsString("(Line 1\nLine 2)"))
        << "multiline literal string";
    EXPECT_THAT(parseExpr("atan2(1 mm, 1 mm)"), IsQuantity(Base::Quantity(45, Base::Unit::Angle)))
        << "trigonometric helper";
}

TEST_F(ExpressionParserTest, DISABLED_expressionsParseAsPyObjectWrapper)
{
    // PyObjectWrapper is internal to Expression.cpp, so we can't test for it
    // directly, even though some expressions return this
    // I've left this test here to document this fact, and as a way to ask for
    // help during code review on why this type exists / why it is private
    // The below all parse as PyObjectWrapper currently
    EXPECT_THAT(parseExpr("vector(1 mm, 2 mm, 3 mm)"), Not(IsParserError(_)))
        << "vector-valued expression of lengths";
    EXPECT_THAT(parseExpr("vector(1, 0, 0) * 5 mm"), Not(IsParserError(_)))
        << "mixed dimensionless and dimensional operands";
}

TEST_F(ExpressionParserTest, expressionsThatLookValidButDoNotParse)
{
    // Documenting current behaviour
    // Note, for some/all of these, it's not clear to me whether it's intended that
    // it doesn't parse
    EXPECT_THAT(parseExpr("pow(2, 3) * mm^3"), IsParserError(_))
        << "combining dimensionless math with explicit units";
    EXPECT_THAT(parseExpr("(vector(1 mm,2 mm,3 mm)[0]) * 2"), IsParserError(_))
        << "extract component before arithmetic";
}

TEST_F(ExpressionParserTest, canParseProperties)
{
    EXPECT_THAT(parseExpr("Placement.Base.x"), IsQuantity(mm(0)))
        << "self-reference to owning object's placement";

    this_obj()->addDynamicProperty("App::PropertyFloat", "Foo");
    EXPECT_THAT(parseExpr("Sketch.Foo"), IsDouble(0)) << "Property on object";

    this_obj()->addDynamicProperty("App::PropertyQuantity", "Bar");
    EXPECT_THAT(parseExpr("Sketch.Bar"), IsQuantity(Base::Quantity()))
        << "PropertyQuantity on object";
}

TEST_F(ExpressionParserTest, arithmeticPrecedenceAndAssociativity)
{
    EXPECT_THAT(parseExpr("2 + 3 * 4"), IsLong(14));
    EXPECT_THAT(parseExpr("8 / 4 / 2"), IsLong(1)) << "division is left associative";
    EXPECT_THAT(parseExpr("2^3^2"), IsLong(64)) << "power is currently left associative";
    EXPECT_THAT(parseExpr("-2^2"), IsLong(4)) << "unary minus currently binds tighter than power";
    EXPECT_THAT(parseExpr("-(2^2)"), IsLong(-4));
    EXPECT_THAT(parseExpr("1 ? 2 : 0 ? 3 : 4"), IsLong(2))
        << "conditional expressions are right associative";
}

TEST_F(ExpressionParserTest, comparisonAndConditionalProductions)
{
    for (const auto* expression : {
             "1 == 1", "1 != 2", "1 < 2", "2 > 1", "1 <= 1", "1 >= 1", "1 ? 2 : 3"}) {
        EXPECT_NO_THROW(parse(this_obj(), expression)) << expression;
    }
}

TEST_F(ExpressionParserTest, numericStringConstantAndFunctionProductions)
{
    EXPECT_THAT(parseExpr("42"), IsLong(42));
    EXPECT_THAT(parseExpr(".5"), IsDouble(0.5));
    EXPECT_THAT(parseExpr("1e2"), IsLong(100));
    EXPECT_THAT(parseExpr("pi > 3"), IsLong(1));
    EXPECT_THAT(parseExpr("<<(hello)>>"), IsString("(hello)"));
    EXPECT_NO_THROW(parse(this_obj(), "sum(1, 2; 3)"));
}

TEST_F(ExpressionParserTest, unitProductions)
{
    EXPECT_THAT(parseExpr("2 mm^2"), IsQuantity(mm2(2)));
    EXPECT_THAT(parseExpr("(2 mm)^2"), IsQuantity(mm2(4)));
    EXPECT_THAT(parseExpr("2 mm/s * 3"),
                IsQuantity(Base::Quantity(6, Base::Unit::Velocity)));
    const auto usBuildingUnit = parse_expression_text_as_quantity("1' 2\"");
    EXPECT_NEAR(usBuildingUnit.getValue(), 355.6, 1e-12) << "US building units";
    EXPECT_EQ(usBuildingUnit.getUnit(), Base::Unit::Length);
    EXPECT_NO_THROW(parseUnit(this_obj(), "kg*m/s^2"));
    EXPECT_NO_THROW(parseUnit(this_obj(), "m^-2"));
    EXPECT_NO_THROW(parseUnit(this_obj(), "(kg*m)/s^2"));
}

TEST_F(ExpressionParserTest, identifierPathAndIndexerProductions)
{
    for (const auto* expression : {
             "Foo", ".Foo", "Sketch.Foo", "Sketch.<<(Sub object)>>.Foo",
             "Doc#Sketch.Foo", "Doc#Sketch.<<(Sub object)>>.Foo", "Foo.Bar.Baz",
             "Foo[0]", "Foo[1:]", "Foo[:2]", "Foo[::2]", "Foo[1:2]",
             "Foo[1::2]", "Foo[:2:3]", "Foo[1:2:3]", "Foo[0][1].Bar"}) {
        EXPECT_NO_THROW(parse(this_obj(), expression)) << expression;
    }
}

TEST_F(ExpressionParserTest, intentionalGrammarRestrictions)
{
    for (const auto* expression : {
             "list(1, 2)[0]", "(Foo)[0]", "1[0]", "Foo[]", "Foo[:]", "sqrt()",
             "1 mm kg", "10 mm * kg", "24 V / (2 A) kg"}) {
        EXPECT_THROW(parse(this_obj(), expression), Base::ParserError) << expression;
    }
}

TEST(ExpressionPrattParserTest, bindingPowersMatchCurrentGrammar)
{
    using namespace App::ExpressionParser::Pratt;

    EXPECT_EQ(infixBindingPower(TokenKind::Question),
              (std::optional<BindingPower> {{BindingPowers::ternary,
                                             BindingPowers::ternary - 1}}));
    EXPECT_EQ(infixBindingPower(TokenKind::Power),
              (std::optional<BindingPower> {{BindingPowers::power, BindingPowers::power}}));
    EXPECT_GT(BindingPowers::prefix, BindingPowers::power);
    EXPECT_GT(BindingPowers::quantity, BindingPowers::multiplicative);
}

TEST_F(ExpressionParserTest, prattScalarArithmeticMatchesGeneratedParser)
{
    for (const auto* expression : {
             "0", "42", ".5", "1e2", "pi", "True", "false", "None", "+5", "-5",
             "--5", "2 + 3 * 4", "(2 + 3) * 4", "8 / 4 / 2", "10 % 3", "2^3^2",
             "-2^2", "-(2^2)", "1 == 1", "1 != 2", "1 < 2", "2 > 1", "1 <= 1",
             "1 >= 1", "1 ? 2 : 3", "0 ? 2 : 3", "1 ? 2 : 0 ? 3 : 4",
             "1 + 2 > 2 ? 10 / 2 : 0", "\xE2\x88\x92" "2 + 5"}) {
        EXPECT_EQ(simplifiedValue(expression, true), simplifiedValue(expression, false))
            << expression;
    }
}

TEST_F(ExpressionParserTest, unitSpellingsAreContextualNames)
{
    for (const auto* expression : {"mm.Foo", "in.Bar"}) {
        EXPECT_EQ(parsedText(expression, true), expression);
    }

    EXPECT_EQ(parsePrattQuantity("10 mm * kg"), Base::Quantity::parse("10 mm*kg"));
}

TEST_F(ExpressionParserTest, prattIdentifierPathsMatchGeneratedParser)
{
    for (const auto* expression : {
             "Foo", "A1", ".Foo", ".A1", ".<<(Sub object)>>.Foo", "Sketch.Foo",
             "Sketch.A1", "Sketch.<<(Sub object)>>.Foo", "Doc#Sketch.Foo",
             "<<(Document label)>>#Sketch.Foo", "Doc#Sketch.<<(Sub object)>>.Foo",
             "Foo.Bar.Baz", "Foo[0]", "Foo[1:]", "Foo[:2]", "Foo[::2]",
             "Foo[1:2]", "Foo[1::2]", "Foo[:2:3]", "Foo[1:2:3]",
             "Foo[0][1].Bar", "Foo[1:2].Bar.Baz[0]"}) {
        EXPECT_EQ(parsedText(expression, true), parsedText(expression, false)) << expression;
    }
}

TEST_F(ExpressionParserTest, prattIdentifierValuesMatchGeneratedParser)
{
    this_obj()->addDynamicProperty("App::PropertyFloat", "Foo");
    this_obj()->addDynamicProperty("App::PropertyQuantity", "Bar");

    for (const auto* expression : {"Placement.Base.x", "Sketch.Foo", "Sketch.Bar"}) {
        EXPECT_EQ(simplifiedValue(expression, true), simplifiedValue(expression, false))
            << expression;
    }
}

TEST_F(ExpressionParserTest, prattRangesMatchGeneratedParser)
{
    for (const auto* expression : {"sum(A1:B2)", "sum(A1:B2, C3:D4; 5)"}) {
        EXPECT_EQ(parsedText(expression, true), parsedText(expression, false)) << expression;
    }
}

TEST_F(ExpressionParserTest, prattPathEntryPointMatchesVariablePaths)
{
    for (const auto* pathText : {
             "Foo", "A1", ".Foo", ".<<(Sub object)>>.Foo", "Sketch.Foo",
             "Sketch.<<(Sub object)>>.Foo", "Doc#Sketch.Foo",
             "Doc#Sketch.<<(Sub object)>>.Foo", "Foo.Bar.Baz"}) {
        const auto generated = parse(this_obj(), pathText);
        const auto* variable = freecad_cast<VariableExpression*>(generated.get());
        ASSERT_NE(variable, nullptr) << pathText;
        EXPECT_EQ(Pratt::Detail::parseFlexPath(this_obj(), pathText).toString(),
                  variable->getPath().toString())
            << pathText;
    }
    const auto generated = parse(this_obj(), "Foo[0]");
    const auto* variable = freecad_cast<VariableExpression*>(generated.get());
    ASSERT_NE(variable, nullptr);
    EXPECT_EQ(Pratt::Detail::parseFlexPath(this_obj(), "Foo[0]").toString(),
              variable->getPath().toString());
    EXPECT_THROW(Pratt::Detail::parseFlexPath(this_obj(), "Foo[1 + 2]"),
                 Base::ParserError);
}

TEST_F(ExpressionParserTest, prattUnitEntryPointMatchesGeneratedParser)
{
    for (const auto* unitText : {"mm", "kg*m/s^2", "m^-2", "(kg*m)/s^2", "1/mm"}) {
        EXPECT_EQ(Pratt::Detail::parseFlexUnit(this_obj(), unitText)->getQuantity(),
                  parseUnit(this_obj(), unitText)->getQuantity())
            << unitText;
    }

    for (const auto* unitText : {"", "2 mm", "mm + kg", "2/mm"}) {
        EXPECT_EQ(exceptionCategory(
                      [this, unitText] { Pratt::Detail::parseFlexUnit(this_obj(), unitText); }),
                  exceptionCategory([this, unitText] { parseUnit(this_obj(), unitText); }))
            << unitText;
    }
}

TEST_F(ExpressionParserTest, prattFailuresMatchGeneratedParserCategories)
{
    for (const auto* expression : {
             "", "-", "+", "1 +", "(1", "1)", "1 ? 2", "1 ? : 3", "1 == ",
             "sqrt()", "sqrt(1, 2)", "sum(1,)", "1 mm kg", "Foo[]", "Foo[:]",
             "Foo[::]", "Foo[1:2:]", "list(1, 2)[0]", "(Foo)[0]", "1[0]"}) {
        EXPECT_EQ(exceptionCategory(
                      [this, expression] {
                          Pratt::Detail::parseFlexTokenStream(this_obj(), expression);
                      }),
                  exceptionCategory([this, expression] { parse(this_obj(), expression); }))
            << expression;
    }
}

TEST_F(ExpressionParserTest, prattFunctionsMatchGeneratedParser)
{
    for (const auto* expression : {
             "abs(-5)", "sqrt(9)", "sqrt(9) * 2", "pow(2, 3)", "mod(10, 3)",
             "sum(1, 2, 3)", "sum(1; 2, 3)", "sum(abs(-1), pow(2, 3), sqrt(9))",
             "not(False)", "and(True, 1, 2 > 1)", "or(False; 0; 3)",
             "sqrt(9 mm^2)", "pow(2 mm, 3)", "atan2(1 mm, 1 mm)",
             "<<(Line 1\\nLine 2)>>", "str(1 m + 2 mm)",
             "parsequant(<<(1 + 2) m>>)", "parsequant(str(1 m + 2 mm))"}) {
        EXPECT_EQ(simplifiedValue(expression, true), simplifiedValue(expression, false))
            << expression;
    }
}

TEST_F(ExpressionParserTest, prattFunctionsPreserveSyntaxRestrictions)
{
    EXPECT_THROW(Pratt::Detail::parseFlexTokenStream(this_obj(), "sqrt()"),
                 Base::ParserError);
    EXPECT_THROW(Pratt::Detail::parseFlexTokenStream(this_obj(), "sum()"),
                 Base::ParserError);
    EXPECT_THROW(Pratt::Detail::parseFlexTokenStream(this_obj(), "list(1, 2)[0]"),
                 Base::ParserError);
    EXPECT_THROW(Pratt::Detail::parseFlexTokenStream(this_obj(), "(Foo)[0]"),
                 Base::ParserError);
    EXPECT_THROW(Pratt::Detail::parseFlexTokenStream(this_obj(), "1[0]"),
                 Base::ParserError);
    EXPECT_THROW(Pratt::Detail::parseFlexTokenStream(this_obj(), "Foo[:]"),
                 Base::ParserError);
}

TEST_F(ExpressionParserTest, prattQuantitiesMatchGeneratedParser)
{
    for (const auto* expression : {
             "0 mm", "-5 mm", "+5 mm", "1.25 mm", "1e3 mm", "1mm + 1mm",
             "1 mm * 3", "1 mm * 2 cm", "2 cm * 1 mm", "2 mm^2", "(2 mm)^2",
             "2 mm/s * 3", "24 V / (2 A)", "360 deg + pi rad", "1' 2\""}) {
        EXPECT_EQ(simplifiedValue(expression, true), simplifiedValue(expression, false))
            << expression;
    }
}

TEST_F(ExpressionParserTest, prattUnitContinuationIsLocal)
{
    EXPECT_EQ(parsePrattQuantity("10 mm * kg"), Base::Quantity::parse("10 mm*kg"));
    EXPECT_EQ(parsePrattQuantity("1234000 mm*kg/s^2"),
              Base::Quantity::parse("1234000 mm*kg/s^2"));
    EXPECT_EQ(parsePrattQuantity("10 mm * 3"), Base::Quantity::parse("30 mm"));
    EXPECT_EQ(parsePrattQuantity("24 V / (2 A)"),
              Base::Quantity(12'000'000, Base::Unit::ElectricalResistance));
}

TEST_F(ExpressionParserTest, productionParserCanOptIntoPratt)
{
    const auto preferences = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Expression");
    struct PreferenceCleaner
    {
        ParameterGrp::handle preferences;
        ~PreferenceCleaner()
        {
            preferences->RemoveBool("UsePrattParser");
        }
    } cleaner {preferences};

    preferences->RemoveBool("UsePrattParser");
    EXPECT_THROW(parse(this_obj(), "10 mm * kg"), Base::ParserError);

    preferences->SetBool("UsePrattParser", true);
    const auto expression = parse(this_obj(), "10 mm * kg");
    EXPECT_EQ(App::any_cast<Base::Quantity>(expression->simplify()->getValueAsAny()),
              Base::Quantity::parse("10 mm*kg"));
    EXPECT_EQ(parse(this_obj(), "mm.Foo")->toString(), "mm.Foo");
    EXPECT_EQ(parse(this_obj(), "in.Bar")->toString(), "in.Bar");
}

}  // namespace App::ExpressionParser::Test
