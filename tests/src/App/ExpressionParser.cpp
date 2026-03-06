// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Base/Quantity.h"

#include "App/Application.h"
#include "App/Document.h"
#include "App/DocumentObject.h"
#include "App/Expression.h"
#include "App/ExpressionParser.h"

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
        thisObj = thisDoc->addObject("Sketcher::SketchObject", "Sketch");
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

}  // namespace App::ExpressionParser::Test
