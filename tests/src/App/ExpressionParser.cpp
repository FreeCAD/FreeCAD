// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include "Base/Quantity.h"

#include "App/Application.h"
#include "App/Document.h"
#include "App/DocumentObject.h"
#include "App/Expression.h"
#include "App/ExpressionParser.h"

#include "src/App/InitApplication.h"

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
        thisObj = thisDoc->addObject("Sketcher::SketchObject");
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

private:
    std::string docName;
    App::Document* thisDoc {};
    App::DocumentObject* thisObj {};
};

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

}  // namespace App::ExpressionParser::Test
