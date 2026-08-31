// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include <gtest/gtest.h>

#include <src/App/InitApplication.h>

#include "App/Document.h"
#include "App/DocumentObject.h"
#include "App/Expression.h"
#include "App/ExpressionParser.h"
#include "App/ExpressionTokenizer.h"

// +------------------------------------------------+
// | Note: For more expression related tests, see:  |
// |       src/Mod/Spreadsheet/TestSpreadsheet.py   |
// +------------------------------------------------+

class Expression: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};

// clang-format off
TEST_F(Expression, tokenize)
{
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral(""), 10), QString());
    // 0.0000 deg-
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-"), 10), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-s"), 11), QStringLiteral("s"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QString::fromUtf8("0.00000 \xC2\xB0-ss"), 12), QStringLiteral("ss"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("0.00000 deg"), 5), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("0.00000 deg"), 11), QStringLiteral("deg"));
}

TEST_F(Expression, tokenizeCompletion)
{
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("My Cube"), 7), QStringLiteral("MyCube"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("My Cube0"), 8), QStringLiteral("MyCube0"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("My Cube 0"), 9), QStringLiteral("MyCube0"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("My Cube1"), 8), QStringLiteral("MyCube1"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("My Cube 1"), 9), QStringLiteral("MyCube1"));
}

// Token categories and source locations are covered by ExpressionParser lexer tests.

TEST_F(Expression, tokenizePi_rad)
{
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("p"), 1), QStringLiteral("p"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("pi"), 2), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("pi "), 3), QString());
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("pi r"), 4), QStringLiteral("r"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("pi ra"), 5), QStringLiteral("ra"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("pi rad"), 6), QStringLiteral("rad"));
    EXPECT_EQ(App::ExpressionTokenizer().perform(QStringLiteral("pi rad"), 2), QString());
}

TEST_F(Expression, toString)
{
    App::UnitExpression expr{nullptr, Base::Quantity{}, "pi rad"};
    EXPECT_EQ(expr.toString(), "pi rad");
}

TEST_F(Expression, test_pi_rad)
{
    auto constant = std::make_unique<App::ConstantExpression>(nullptr, "pi");
    auto unit = std::make_unique<App::UnitExpression>(nullptr, Base::Quantity{}, "rad");
    auto op = std::make_unique<App::OperatorExpression>(nullptr, constant.get(), App::OperatorExpression::UNIT, unit.get());
    EXPECT_EQ(op->toString(), "pi rad");
    op.release();
}

TEST_F(Expression, test_e_rad)
{
    auto constant = std::make_unique<App::ConstantExpression>(nullptr, "e");
    auto unit = std::make_unique<App::UnitExpression>(nullptr, Base::Quantity{}, "rad");
    auto op = std::make_unique<App::OperatorExpression>(nullptr, constant.get(), App::OperatorExpression::UNIT, unit.get());
    EXPECT_EQ(op->toString(), "e rad");
    op.release();
}

class Evaluate: public ::testing::Test
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
        _this_obj = _this_doc->addObject("App::VarSet");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_doc_name.c_str());
    }

    App::DocumentObject* this_obj() { return _this_obj; }

private:
    std::string _doc_name;
    App::Document* _this_doc {};
    App::DocumentObject* _this_obj {};
};

// Various test for evaluating and simplifying expressions
// Each "evaluate" test has an equivalent "simplify" test
TEST_F(Evaluate, test_evaluate_simple)
{
    App::ExpressionPtr e = App::ExpressionParser::parse(this_obj(), "1 + 2");
    App::ExpressionPtr evaluated = e->eval();
    EXPECT_EQ(e->toString(), "1 + 2");
    EXPECT_EQ(evaluated->toString(), "3");
}

TEST_F(Evaluate, test_simplify_simple)
{
    App::ExpressionPtr e = App::ExpressionParser::parse(this_obj(), "1 + 2");
    App::ExpressionPtr simplified = e->simplify();
    EXPECT_EQ(e->toString(), "1 + 2");
    EXPECT_EQ(simplified->toString(), "3");
}

TEST_F(Evaluate, test_evaluate_function)
{
    App::ExpressionPtr e = App::ExpressionParser::parse(this_obj(), "sqrt(4)");
    App::ExpressionPtr evaluated = e->eval();
    EXPECT_EQ(e->toString(), "sqrt(4)");
    EXPECT_EQ(evaluated->toString(), "2");
}

TEST_F(Evaluate, test_simplify_function)
{
    App::ExpressionPtr e = App::ExpressionParser::parse(this_obj(), "sqrt(4)");
    App::ExpressionPtr simplified = e->simplify();
    EXPECT_EQ(e->toString(), "sqrt(4)");
    EXPECT_EQ(simplified->toString(), "2");
}

TEST_F(Evaluate, test_evaluate_refer_to_var)
{
    auto* prop = freecad_cast<App::PropertyFloat*>(this_obj()->addDynamicProperty("App::PropertyFloat", "Var"));
    prop->setValue(2.0);
    App::ExpressionPtr e = App::ExpressionParser::parse(this_obj(), "sqrt(2 + Var)");
    App::ExpressionPtr evaluated = e->eval();
    EXPECT_EQ(e->toString(), "sqrt(2 + Var)");
    EXPECT_EQ(evaluated->toString(), "2");
}

TEST_F(Evaluate, test_simplify_refer_to_var)
{
    auto* prop = freecad_cast<App::PropertyFloat*>(this_obj()->addDynamicProperty("App::PropertyFloat", "Var"));
    prop->setValue(2.0);
    App::ExpressionPtr e = App::ExpressionParser::parse(this_obj(), "sqrt(2 + Var)");
    App::ExpressionPtr simplified = e->simplify();
    EXPECT_EQ(e->toString(), "sqrt(2 + Var)");
    EXPECT_EQ(simplified->toString(), "sqrt(2 + Var)");
}
// clang-format on
