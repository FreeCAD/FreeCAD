#include <gtest/gtest.h>

#include "Base/Quantity.h"

#include "App/Application.h"
#include "App/Document.h"
#include "App/DocumentObject.h"
#include "App/Expression.h"
#include "App/ExpressionParser.h"

#include "src/App/InitApplication.h"

// clang-format off

class ExpressionParserTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _doc_name = App::GetApplication().getUniqueDocumentName("test");
        _this_doc = App::GetApplication().newDocument(_doc_name.c_str(), "testUser");
        _this_obj = _this_doc -> addObject("Sketcher::SketchObject");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_doc_name.c_str());
    }

    std::string doc_name() { return _doc_name; }
    App::Document* this_doc() { return _this_doc; }
    App::DocumentObject* this_obj() { return _this_obj; }

    Base::Quantity parse_expression_text_as_quantity(const char* expression_text) {
        const auto expression = App::ExpressionParser::parse(this_obj(), expression_text);
        return App::any_cast<Base::Quantity>(expression->getValueAsAny());
    }

    Base::Quantity parse_quantity_text_as_quantity(const char* quantity_text) {
        return Base::Quantity::parse(quantity_text);
    }

private:
    std::string _doc_name;
    App::Document* _this_doc {};
    App::DocumentObject* _this_obj {};
};

// https://github.com/FreeCAD/FreeCAD/issues/11965
TEST_F(ExpressionParserTest, functionPARSEQUANT)
{

    EXPECT_ANY_THROW(App::ExpressionParser::parse(this_obj(), "parsequant()")) << "should not parse empty";

    EXPECT_NO_THROW(App::ExpressionParser::parse(this_obj(), "parsequant(1 mm)")) << "should parse simple quantity";
    EXPECT_NO_THROW(App::ExpressionParser::parse(this_obj(), "parsequant(<<(1 + 2) m>>)")) << "should parse literal quantity";
    EXPECT_NO_THROW(App::ExpressionParser::parse(this_obj(), "parsequant(str(1 m + 2 mm))")) << "should parse str-function quantity";

    EXPECT_ANY_THROW(parse_quantity_text_as_quantity("parsequant(1 mm)")) << "should not treat parsequant-function as quantity";
    EXPECT_EQ(parse_quantity_text_as_quantity("1 mm"), parse_quantity_text_as_quantity("1 mm")) << "equality sanity check";
    EXPECT_NE(parse_quantity_text_as_quantity("1 mm"), parse_quantity_text_as_quantity("2 mm")) << "inequality sanity check";

    std::array<std::pair<const char*,const char*>, 12> expression_vs_quantity_list = {{
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
    }};

    for ( const auto& expression_vs_quantity_pair : expression_vs_quantity_list ) {
        auto expression_text = expression_vs_quantity_pair.first;
        auto quantity_text = expression_vs_quantity_pair.second;
        auto expression_result = parse_expression_text_as_quantity(expression_text);
        auto quantity_result = parse_quantity_text_as_quantity(quantity_text);
        EXPECT_EQ(expression_result, quantity_result) << "mismatch:"
            " expression_text='" + std::string(expression_text) + "'"
            " quantity_text='" + std::string(quantity_text) + "'"
            " expression_representation='" + expression_result.getUserString() + "'"
            " quantity_representation='" + quantity_result.getUserString() + "'"
        ;
    }

}

TEST_F(ExpressionParserTest, isTokenAConstant)
{
    std::array<std::string, 7> constants {"pi", "e", "True", "False", "true", "false", "None"};
    for (const auto & constant : constants) {
        EXPECT_TRUE(App::ExpressionParser::isTokenAConstant(constant))
          << "\"" << constant << "\" did not evaluate as a constant";
    }

    std::array<std::string, 6> notConstants {"PI", "E", "TRUE", "FALSE", "NONE", "none"};
    for (const auto & nonConstant : notConstants) {
        EXPECT_FALSE(App::ExpressionParser::isTokenAConstant(nonConstant))
          << "\"" << nonConstant << "\" evaluated as a constant";
    }
}

// clang-format on
