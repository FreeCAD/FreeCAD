// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <gtest/gtest.h>

#include <Base/OkLch.h>
#include <Gui/Utilities.h>

#include <Gui/StyleParameters/Corners.h>
#include <Gui/StyleParameters/Gradient.h>
#include <Gui/StyleParameters/Insets.h>
#include <Gui/StyleParameters/Parser.h>
#include <Gui/StyleParameters/ParameterManager.h>

using namespace Gui::StyleParameters;

class ParserTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a simple parameter manager for testing
        auto source = std::make_unique<InMemoryParameterSource>(
            std::list<Parameter> {
                {.name = "TestParam", .value = "10px"},
                {.name = "TestColor", .value = "#ff0000"},
                {.name = "TestNumber", .value = "5"},
                {.name = "TestTupleParam", .value = "(left: 1px, right: 2px, top: 3px, bottom: 4px)"},
                {.name = "TestIndexedTuple", .value = "(10, 20, 30)"},
                {.name = "TestNestedTuple", .value = "((x: 1, y: 2), (x: 3, y: 4))"},
            },
            ParameterSource::Metadata {.name = "Test Source"}
        );

        manager.addSource(source.get());
        sources.push_back(std::move(source));
    }

    Gui::StyleParameters::ParameterManager manager;
    std::vector<std::unique_ptr<ParameterSource>> sources;
};

// Test number parsing
TEST_F(ParserTest, ParseNumbers)
{
    {
        Parser parser("42");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 42.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10.5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 10.5);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("2.5em");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 2.5);
        EXPECT_EQ(length.unit, "em");
    }

    {
        Parser parser("100%");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 100.0);
        EXPECT_EQ(length.unit, "%");
    }
}

// Test color parsing
TEST_F(ParserTest, ParseColors)
{
    {
        Parser parser("#ff0000");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result);
        EXPECT_EQ(color.r, 1);
        EXPECT_EQ(color.g, 0);
        EXPECT_EQ(color.b, 0);
    }

    {
        Parser parser("#00ff00");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result);
        EXPECT_EQ(color.r, 0);
        EXPECT_EQ(color.g, 1);
        EXPECT_EQ(color.b, 0);
    }

    {
        Parser parser("#0000ff");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result);
        EXPECT_EQ(color.r, 0);
        EXPECT_EQ(color.g, 0);
        EXPECT_EQ(color.b, 1);
    }

    {
        Parser parser("rgb(255, 0, 0)");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result);
        EXPECT_EQ(color.r, 1);
        EXPECT_EQ(color.g, 0);
        EXPECT_EQ(color.b, 0);
    }

    {
        Parser parser("rgba(255, 0, 0, 128)");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result);
        EXPECT_DOUBLE_EQ(color.r, 1);
        EXPECT_DOUBLE_EQ(color.g, 0);
        EXPECT_DOUBLE_EQ(color.b, 0);
        EXPECT_NEAR(color.a, 128 / 255.0, 1e-6);
    }
}

// Test parameter reference parsing
TEST_F(ParserTest, ParseParameterReferences)
{
    {
        Parser parser("@TestParam");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 10.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("@TestColor");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result);
        EXPECT_EQ(color.r, 1);
        EXPECT_EQ(color.g, 0);
        EXPECT_EQ(color.b, 0);
    }

    {
        Parser parser("@TestNumber");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 5.0);
        EXPECT_EQ(length.unit, "");
    }
}

// Test arithmetic operations
TEST_F(ParserTest, ParseArithmeticOperations)
{
    {
        Parser parser("10 + 5");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px + 5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("10 - 5");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 5.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px - 5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 5.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("10 * 5");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 50.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px * 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 20.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("10 / 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 5.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px / 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 5.0);
        EXPECT_EQ(length.unit, "px");
    }
}

// Test complex expressions
TEST_F(ParserTest, ParseComplexExpressions)
{
    {
        Parser parser("(10 + 5) * 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 30.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("(10px + 5px) * 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 30.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("@TestParam + 5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("@TestParam * @TestNumber");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 50.0);
        EXPECT_EQ(length.unit, "px");
    }
}

// Test unary operations
TEST_F(ParserTest, ParseUnaryOperations)
{
    {
        Parser parser("+10");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 10.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("-10");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, -10.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("-10px");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, -10.0);
        EXPECT_EQ(length.unit, "px");
    }
}

// Test function calls
TEST_F(ParserTest, ParseFunctionCalls)
{
    {
        Parser parser("lighten(#ff0000, 20)");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result).asValue<QColor>();
        // The result should be lighter than the original red
        EXPECT_GT(color.lightness(), QColor(0xff0000).lightness());
    }

    {
        Parser parser("darken(#ff0000, 20)");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result).asValue<QColor>();
        // The result should be darker than the original red
        EXPECT_LT(color.lightness(), QColor(0xff0000).lightness());
    }

    {
        Parser parser("lighten(@TestColor, 20)");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result).asValue<QColor>();
        // The result should be lighter than the original red
        EXPECT_GT(color.lightness(), QColor(0xff0000).lightness());
    }
}

// Test error cases
TEST_F(ParserTest, ParseErrors)
{
    // Invalid color format
    EXPECT_THROW(
        {
            Parser parser("#invalid");
            parser.parse();
        },
        Base::ParserError
    );

    // Invalid RGB format
    EXPECT_THROW(
        {
            Parser parser("rgb(invalid)");
            parser.parse();
        },
        Base::ParserError
    );

    // Missing closing parenthesis
    EXPECT_THROW(
        {
            Parser parser("(10 + 5");
            parser.parse();
        },
        Base::ParserError
    );

    // Invalid function
    EXPECT_THROW(
        {
            Parser parser("invalid()");
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::ExpressionError
    );

    // Division by zero
    EXPECT_THROW(
        {
            Parser parser("10 / 0");
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::RuntimeError
    );

    // Unit mismatch
    EXPECT_THROW(
        {
            Parser parser("10px + 5em");
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::RuntimeError
    );

    // Unary operation on color
    EXPECT_THROW(
        {
            Parser parser("-@TestColor");
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::ExpressionError
    );

    // Function with wrong number of arguments
    EXPECT_THROW(
        {
            Parser parser("lighten(#ff0000)");
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::ExpressionError
    );

    // Function with wrong argument type
    EXPECT_THROW(
        {
            Parser parser("lighten(10px, 20)");
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::ExpressionError
    );
}

// Test whitespace handling
TEST_F(ParserTest, ParseWhitespace)
{
    {
        Parser parser("  10  +  5  ");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px+5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("rgb(255,0,0)");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result);
        EXPECT_EQ(color.r, 1);
        EXPECT_EQ(color.g, 0);
        EXPECT_EQ(color.b, 0);
    }
}

// Test edge cases
TEST_F(ParserTest, ParseEdgeCases)
{
    // Empty input
    EXPECT_THROW(
        {
            Parser parser("");
            parser.parse();
        },
        Base::ParserError
    );

    // Just whitespace
    EXPECT_THROW(
        {
            Parser parser("   ");
            parser.parse();
        },
        Base::ParserError
    );

    // Single number
    {
        Parser parser("42");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 42.0);
        EXPECT_EQ(length.unit, "");
    }

    // Single color
    {
        Parser parser("#ff0000");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result);
        EXPECT_EQ(color.r, 1);
        EXPECT_EQ(color.g, 0);
        EXPECT_EQ(color.b, 0);
    }

    // Single parameter reference
    {
        Parser parser("@TestParam");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 10.0);
        EXPECT_EQ(length.unit, "px");
    }
}

// Test operator precedence
TEST_F(ParserTest, ParseOperatorPrecedence)
{
    {
        Parser parser("2 + 3 * 4");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 14.0);  // 2 + (3 * 4) = 2 + 12 = 14
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10 - 3 * 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 4.0);  // 10 - (3 * 2) = 10 - 6 = 4
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("20 / 4 + 3");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 8.0);  // (20 / 4) + 3 = 5 + 3 = 8
        EXPECT_EQ(length.unit, "");
    }
}

// Test unnamed tuple
TEST_F(ParserTest, ParseUnnamedTuple)
{
    Parser parser("(10, 20, 30)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 3);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(tuple.at(0)).value, 10.0);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(tuple.at(1)).value, 20.0);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(tuple.at(2)).value, 30.0);
}

// Test named tuple
TEST_F(ParserTest, ParseNamedTuple)
{
    Parser parser("(x: 10, y: 20)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);

    auto* x = tuple.find("x");
    ASSERT_NE(x, nullptr);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(*x).value, 10.0);

    auto* y = tuple.find("y");
    ASSERT_NE(y, nullptr);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(*y).value, 20.0);
}

// Test mixed named/unnamed tuple
TEST_F(ParserTest, ParseMixedTuple)
{
    Parser parser("(x: 10, 20)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);

    auto* x = tuple.find("x");
    ASSERT_NE(x, nullptr);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(*x).value, 10.0);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(tuple.at(1)).value, 20.0);
}

// Test named tuple with numeric-starting names (e.g. shade keys like 050, 100)
TEST_F(ParserTest, ParseNamedTupleWithNumericNames)
{
    Parser parser("(050: 0.05, 100: 0.1)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);

    auto* shade050 = tuple.find("050");
    ASSERT_NE(shade050, nullptr);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(*shade050).value, 0.05);

    auto* shade100 = tuple.find("100");
    ASSERT_NE(shade100, nullptr);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(*shade100).value, 0.1);
}

// Test single named element is a tuple, not a grouped expression
TEST_F(ParserTest, ParseSingleNamedElementIsTuple)
{
    Parser parser("(x: 10)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 1);

    auto* x = tuple.find("x");
    ASSERT_NE(x, nullptr);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(*x).value, 10.0);
}

// Test expressions inside tuple elements
TEST_F(ParserTest, ParseTupleWithExpressions)
{
    Parser parser("(x: 10 + 5, y: @TestParam)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);

    auto* x = tuple.find("x");
    ASSERT_NE(x, nullptr);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(*x).value, 15.0);

    auto* y = tuple.find("y");
    ASSERT_NE(y, nullptr);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(*y).value, 10.0);
    EXPECT_EQ(std::get<Numeric>(*y).unit, "px");
}

// Test mixed types in tuple
TEST_F(ParserTest, ParseTupleWithMixedTypes)
{
    Parser parser("(color: #ff0000, opacity: 0.5)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);

    auto* color = tuple.find("color");
    ASSERT_NE(color, nullptr);
    EXPECT_TRUE(std::holds_alternative<Base::Color>(*color));
    auto c = std::get<Base::Color>(*color);
    EXPECT_EQ(c.r, 1);
    EXPECT_EQ(c.g, 0);
    EXPECT_EQ(c.b, 0);

    auto* opacity = tuple.find("opacity");
    ASSERT_NE(opacity, nullptr);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(*opacity).value, 0.5);
}

// Test nested tuples
TEST_F(ParserTest, ParseNestedTuples)
{
    Parser parser("((1, 2), (3, 4))");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());
    const auto& outer = result.get<Tuple>();
    EXPECT_EQ(outer.size(), 2);

    EXPECT_TRUE(outer.at(0).holds<Tuple>());
    const auto& inner1 = outer.at(0).get<Tuple>();
    EXPECT_EQ(inner1.size(), 2);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(inner1.at(0)).value, 1.0);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(inner1.at(1)).value, 2.0);

    EXPECT_TRUE(outer.at(1).holds<Tuple>());
    const auto& inner2 = outer.at(1).get<Tuple>();
    EXPECT_EQ(inner2.size(), 2);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(inner2.at(0)).value, 3.0);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(inner2.at(1)).value, 4.0);
}

// Test complex expressions in tuple elements
TEST_F(ParserTest, ParseTupleWithComplexExpressions)
{
    Parser parser("(x: lighten(#ff0000, 20), y: 10px * 2)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);

    auto* x = tuple.find("x");
    ASSERT_NE(x, nullptr);
    EXPECT_TRUE(std::holds_alternative<Base::Color>(*x));

    auto* y = tuple.find("y");
    ASSERT_NE(y, nullptr);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(*y).value, 20.0);
    EXPECT_EQ(std::get<Numeric>(*y).unit, "px");
}

// Test tuple toString roundtrip
TEST_F(ParserTest, TupleToString)
{
    {
        Parser parser("(x: 10, y: 20)");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        auto str = result.toString();
        EXPECT_EQ(str, "(x: 10, y: 20)");
    }

    {
        Parser parser("(10, 20, 30)");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        auto str = result.toString();
        EXPECT_EQ(str, "(10, 20, 30)");
    }

    {
        Parser parser("(x: 10, 20)");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        auto str = result.toString();
        EXPECT_EQ(str, "(x: 10, 20)");
    }
}

// Test Value::holds and Value::get
TEST_F(ParserTest, ValueHoldsAndGet)
{
    Value numericValue = Numeric {.value = 42, .unit = "px"};
    EXPECT_TRUE(numericValue.holds<Numeric>());
    EXPECT_FALSE(numericValue.holds<Tuple>());
    EXPECT_DOUBLE_EQ(numericValue.get<Numeric>().value, 42.0);

    Value colorValue = Base::Color(1.0, 0.0, 0.0);
    EXPECT_TRUE(colorValue.holds<Base::Color>());
    EXPECT_FALSE(colorValue.holds<Tuple>());

    Value stringValue = std::string("hello");
    EXPECT_TRUE(stringValue.holds<std::string>());
    EXPECT_FALSE(stringValue.holds<Tuple>());

    Parser parser("(1, 2)");
    auto expr = parser.parse();
    auto tupleValue = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(tupleValue.holds<Tuple>());
    EXPECT_FALSE(tupleValue.holds<Numeric>());
    EXPECT_EQ(tupleValue.get<Tuple>().size(), 2);
}

// Test Tuple::at out of bounds
TEST_F(ParserTest, TupleAtOutOfBounds)
{
    Parser parser("(10, 20)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    const auto& tuple = result.get<Tuple>();

    EXPECT_NO_THROW(tuple.at(0));
    EXPECT_NO_THROW(tuple.at(1));
    EXPECT_THROW(tuple.at(2), Base::RuntimeError);
}

// Test Tuple::find nonexistent name
TEST_F(ParserTest, TupleFindNonexistent)
{
    Parser parser("(x: 10, y: 20)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    const auto& tuple = result.get<Tuple>();

    EXPECT_NE(tuple.find("x"), nullptr);
    EXPECT_NE(tuple.find("y"), nullptr);
    EXPECT_EQ(tuple.find("z"), nullptr);
}

// Test error: missing ')' in tuple
TEST_F(ParserTest, TupleMissingClosingParen)
{
    EXPECT_THROW(
        {
            Parser parser("(x: 10, y: 20");
            parser.parse();
        },
        Base::ParserError
    );
}

// Test that grouped expressions still work (backward compatibility)
TEST_F(ParserTest, GroupedExpressionStillWorks)
{
    {
        Parser parser("(10 + 5) * 2");
        auto expr = parser.parse();

        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        EXPECT_DOUBLE_EQ(std::get<Numeric>(result).value, 30.0);
    }

    {
        Parser parser("(42)");
        auto expr = parser.parse();

        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        EXPECT_DOUBLE_EQ(std::get<Numeric>(result).value, 42.0);
    }
}

// Test nested parentheses
TEST_F(ParserTest, ParseNestedParentheses)
{
    {
        Parser parser("((2 + 3) * 4)");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 20.0);  // (5) * 4 = 20
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("(10 - (3 + 2)) * 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 10.0);  // (10 - 5) * 2 = 5 * 2 = 10
        EXPECT_EQ(length.unit, "");
    }
}

// Test member access on tuple parameters (named)
TEST_F(ParserTest, MemberAccessNamed)
{
    {
        Parser parser("@TestTupleParam.left");
        auto expr = parser.parse();

        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));

        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 1.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("@TestTupleParam.right");
        auto expr = parser.parse();

        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));

        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 2.0);
        EXPECT_EQ(length.unit, "px");
    }
}

// Test member access on tuple parameters (indexed)
TEST_F(ParserTest, MemberAccessIndexed)
{
    {
        Parser parser("@TestIndexedTuple.0");
        auto expr = parser.parse();

        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        EXPECT_DOUBLE_EQ(std::get<Numeric>(result).value, 10.0);
    }

    {
        Parser parser("@TestIndexedTuple.2");
        auto expr = parser.parse();

        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        EXPECT_DOUBLE_EQ(std::get<Numeric>(result).value, 30.0);
    }
}

// Test member access on inline tuples
TEST_F(ParserTest, MemberAccessInlineTuple)
{
    {
        Parser parser("(x: 10, y: 20).x");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        EXPECT_DOUBLE_EQ(std::get<Numeric>(result).value, 10.0);
    }

    {
        Parser parser("(10, 20, 30).1");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        EXPECT_DOUBLE_EQ(std::get<Numeric>(result).value, 20.0);
    }
}

// Test member access in arithmetic expressions
TEST_F(ParserTest, MemberAccessInArithmetic)
{
    Parser parser("@TestTupleParam.left + @TestTupleParam.right");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(std::holds_alternative<Numeric>(result));

    auto length = std::get<Numeric>(result);
    EXPECT_DOUBLE_EQ(length.value, 3.0);
    EXPECT_EQ(length.unit, "px");
}

// Test chained member access on nested tuples
TEST_F(ParserTest, MemberAccessChained)
{
    Parser parser("@TestNestedTuple.0.x");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(std::holds_alternative<Numeric>(result));
    EXPECT_DOUBLE_EQ(std::get<Numeric>(result).value, 1.0);
}

// Test member access error cases
TEST_F(ParserTest, MemberAccessErrors)
{
    // Named member not found
    EXPECT_THROW(
        {
            Parser parser("@TestTupleParam.nonexistent");
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::ExpressionError
    );

    // Index out of bounds
    EXPECT_THROW(
        {
            Parser parser("@TestIndexedTuple.5");
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::RuntimeError
    );

    // Member access on non-tuple
    EXPECT_THROW(
        {
            Parser parser("@TestNumber.foo");
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::ExpressionError
    );
}

// Tuple arithmetic tests

TEST_F(ParserTest, TupleElementWiseAdd)
{
    Parser parser("(1px, 2px) + (3px, 4px)");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());

    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);
    EXPECT_DOUBLE_EQ(tuple.at(0).get<Numeric>().value, 4.0);
    EXPECT_EQ(tuple.at(0).get<Numeric>().unit, "px");
    EXPECT_DOUBLE_EQ(tuple.at(1).get<Numeric>().value, 6.0);
    EXPECT_EQ(tuple.at(1).get<Numeric>().unit, "px");
}

TEST_F(ParserTest, TupleElementWiseSubtract)
{
    Parser parser("(10, 20) - (3, 7)");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());

    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);
    EXPECT_DOUBLE_EQ(tuple.at(0).get<Numeric>().value, 7.0);
    EXPECT_DOUBLE_EQ(tuple.at(1).get<Numeric>().value, 13.0);
}

TEST_F(ParserTest, TupleScalarMultiplyTupleFirst)
{
    Parser parser("(1px, 2px, 3px) * 2");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());

    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 3);
    EXPECT_DOUBLE_EQ(tuple.at(0).get<Numeric>().value, 2.0);
    EXPECT_EQ(tuple.at(0).get<Numeric>().unit, "px");
    EXPECT_DOUBLE_EQ(tuple.at(1).get<Numeric>().value, 4.0);
    EXPECT_DOUBLE_EQ(tuple.at(2).get<Numeric>().value, 6.0);
}

TEST_F(ParserTest, TupleScalarMultiplyScalarFirst)
{
    Parser parser("2 * (1px, 2px, 3px)");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());

    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 3);
    EXPECT_DOUBLE_EQ(tuple.at(0).get<Numeric>().value, 2.0);
    EXPECT_EQ(tuple.at(0).get<Numeric>().unit, "px");
    EXPECT_DOUBLE_EQ(tuple.at(1).get<Numeric>().value, 4.0);
    EXPECT_DOUBLE_EQ(tuple.at(2).get<Numeric>().value, 6.0);
}

TEST_F(ParserTest, TupleScalarDivide)
{
    Parser parser("(10, 20) / 2");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());

    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);
    EXPECT_DOUBLE_EQ(tuple.at(0).get<Numeric>().value, 5.0);
    EXPECT_DOUBLE_EQ(tuple.at(1).get<Numeric>().value, 10.0);
}

TEST_F(ParserTest, TupleUnaryNegate)
{
    Parser parser("-(1px, 2px)");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());

    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);
    EXPECT_DOUBLE_EQ(tuple.at(0).get<Numeric>().value, -1.0);
    EXPECT_EQ(tuple.at(0).get<Numeric>().unit, "px");
    EXPECT_DOUBLE_EQ(tuple.at(1).get<Numeric>().value, -2.0);
    EXPECT_EQ(tuple.at(1).get<Numeric>().unit, "px");
}

TEST_F(ParserTest, TupleAddPreservesNames)
{
    Parser parser("(x: 1, y: 2) + (x: 3, y: 4)");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());

    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);

    auto* x = tuple.find("x");
    ASSERT_NE(x, nullptr);
    EXPECT_DOUBLE_EQ(x->get<Numeric>().value, 4.0);

    auto* y = tuple.find("y");
    ASSERT_NE(y, nullptr);
    EXPECT_DOUBLE_EQ(y->get<Numeric>().value, 6.0);
}

TEST_F(ParserTest, TupleAddMatchesByName)
{
    Parser parser("(x: 10, y: 20) + (y: 30, x: 5)");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());

    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);

    auto* x = tuple.find("x");
    ASSERT_NE(x, nullptr);
    EXPECT_DOUBLE_EQ(x->get<Numeric>().value, 15.0);

    auto* y = tuple.find("y");
    ASSERT_NE(y, nullptr);
    EXPECT_DOUBLE_EQ(y->get<Numeric>().value, 50.0);
}

TEST_F(ParserTest, TupleParamArithmetic)
{
    Parser parser("@TestTupleParam + @TestTupleParam");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());

    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 4);
    EXPECT_DOUBLE_EQ(tuple.at(0).get<Numeric>().value, 2.0);
    EXPECT_DOUBLE_EQ(tuple.at(1).get<Numeric>().value, 4.0);
    EXPECT_DOUBLE_EQ(tuple.at(2).get<Numeric>().value, 6.0);
    EXPECT_DOUBLE_EQ(tuple.at(3).get<Numeric>().value, 8.0);
}

TEST_F(ParserTest, TupleUnionMixedNamedUnnamed)
{
    // (x: 5, 10) + (y: 10, 5, 20) → (x: 5, y: 10, 15, 20)
    Parser parser("(x: 5, 10) + (y: 10, 5, 20)");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());

    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 4);

    auto* x = tuple.find("x");
    ASSERT_NE(x, nullptr);
    EXPECT_DOUBLE_EQ(x->get<Numeric>().value, 5.0);

    auto* y = tuple.find("y");
    ASSERT_NE(y, nullptr);
    EXPECT_DOUBLE_EQ(y->get<Numeric>().value, 10.0);

    EXPECT_DOUBLE_EQ(tuple.at(2).get<Numeric>().value, 15.0);
    EXPECT_DOUBLE_EQ(tuple.at(3).get<Numeric>().value, 20.0);
}

TEST_F(ParserTest, TupleUnionNamedDifferentSizes)
{
    // (x: 1, y: 2) + (x: 3, y: 4, z: 5) → (x: 4, y: 6, z: 5)
    Parser parser("(x: 1, y: 2) + (x: 3, y: 4, z: 5)");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());

    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 3);

    EXPECT_DOUBLE_EQ(tuple.find("x")->get<Numeric>().value, 4.0);
    EXPECT_DOUBLE_EQ(tuple.find("y")->get<Numeric>().value, 6.0);
    EXPECT_DOUBLE_EQ(tuple.find("z")->get<Numeric>().value, 5.0);
}

TEST_F(ParserTest, TupleUnionUnnamedDifferentSizes)
{
    // (1, 2) + (3, 4, 5) → (4, 6, 5)
    Parser parser("(1, 2) + (3, 4, 5)");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());

    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 3);

    EXPECT_DOUBLE_EQ(tuple.at(0).get<Numeric>().value, 4.0);
    EXPECT_DOUBLE_EQ(tuple.at(1).get<Numeric>().value, 6.0);
    EXPECT_DOUBLE_EQ(tuple.at(2).get<Numeric>().value, 5.0);
}

TEST_F(ParserTest, TupleAddNumericError)
{
    EXPECT_THROW(
        {
            Parser parser("(1, 2) + 5");
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::ExpressionError
    );
}

TEST_F(ParserTest, TupleNestedScalarMultiply)
{
    Parser parser("((1, 2), (3, 4)) * 2");
    auto expr = parser.parse();

    auto result = expr->evaluate({.manager = &manager, .context = {}});
    EXPECT_TRUE(result.holds<Tuple>());

    const auto& outer = result.get<Tuple>();
    EXPECT_EQ(outer.size(), 2);

    EXPECT_TRUE(outer.at(0).holds<Tuple>());
    const auto& inner1 = outer.at(0).get<Tuple>();
    EXPECT_DOUBLE_EQ(inner1.at(0).get<Numeric>().value, 2.0);
    EXPECT_DOUBLE_EQ(inner1.at(1).get<Numeric>().value, 4.0);

    EXPECT_TRUE(outer.at(1).holds<Tuple>());
    const auto& inner2 = outer.at(1).get<Tuple>();
    EXPECT_DOUBLE_EQ(inner2.at(0).get<Numeric>().value, 6.0);
    EXPECT_DOUBLE_EQ(inner2.at(1).get<Numeric>().value, 8.0);
}

// ArgumentParser tests

class ArgumentParserTest: public ::testing::Test
{
};

TEST_F(ArgumentParserTest, AllPositional)
{
    Tuple args({
        Tuple::Element::unnamed(Numeric {.value = 1, .unit = ""}),
        Tuple::Element::unnamed(Numeric {.value = 2, .unit = ""}),
    });
    auto resolved = ArgumentParser {{.name = "x"}, {.name = "y"}}.resolve(args);

    ASSERT_NE(resolved.find("x"), nullptr);
    ASSERT_NE(resolved.find("y"), nullptr);
    EXPECT_DOUBLE_EQ(resolved.find("x")->get<Numeric>().value, 1.0);
    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 2.0);
}

TEST_F(ArgumentParserTest, AllNamed)
{
    Tuple args({
        Tuple::Element::named("x", Numeric {.value = 10, .unit = ""}),
        Tuple::Element::named("y", Numeric {.value = 20, .unit = ""}),
    });
    auto resolved = ArgumentParser {{.name = "x"}, {.name = "y"}}.resolve(args);

    EXPECT_DOUBLE_EQ(resolved.find("x")->get<Numeric>().value, 10.0);
    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 20.0);
}

TEST_F(ArgumentParserTest, AllNamedReversedOrder)
{
    Tuple args({
        Tuple::Element::named("y", Numeric {.value = 20, .unit = ""}),
        Tuple::Element::named("x", Numeric {.value = 10, .unit = ""}),
    });
    auto resolved = ArgumentParser {{.name = "x"}, {.name = "y"}}.resolve(args);

    EXPECT_DOUBLE_EQ(resolved.find("x")->get<Numeric>().value, 10.0);
    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 20.0);
}

TEST_F(ArgumentParserTest, MixedPositionalThenNamed)
{
    // f(1, y: 2) with signature (x, y)
    Tuple args({
        Tuple::Element::unnamed(Numeric {.value = 1, .unit = ""}),
        Tuple::Element::named("y", Numeric {.value = 2, .unit = ""}),
    });
    auto resolved = ArgumentParser {{.name = "x"}, {.name = "y"}}.resolve(args);

    EXPECT_DOUBLE_EQ(resolved.find("x")->get<Numeric>().value, 1.0);
    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 2.0);
}

TEST_F(ArgumentParserTest, NamedThenPositionalFillsRemainingSlot)
{
    // f(y: 2, 1) with signature (x, y) — positional 1 fills unclaimed x
    Tuple args({
        Tuple::Element::named("y", Numeric {.value = 2, .unit = ""}),
        Tuple::Element::unnamed(Numeric {.value = 1, .unit = ""}),
    });
    auto resolved = ArgumentParser {{.name = "x"}, {.name = "y"}}.resolve(args);

    EXPECT_DOUBLE_EQ(resolved.find("x")->get<Numeric>().value, 1.0);
    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 2.0);
}

TEST_F(ArgumentParserTest, DefaultValueUsedWhenMissing)
{
    Tuple args({
        Tuple::Element::unnamed(Numeric {.value = 1, .unit = ""}),
    });
    auto resolved = ArgumentParser {
        {.name = "x"},
        {.name = "y", .defaultValue = Numeric {.value = 99, .unit = ""}}
    }.resolve(args);

    EXPECT_DOUBLE_EQ(resolved.find("x")->get<Numeric>().value, 1.0);
    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 99.0);
}

TEST_F(ArgumentParserTest, DefaultValueOverriddenByPositional)
{
    Tuple args({
        Tuple::Element::unnamed(Numeric {.value = 1, .unit = ""}),
        Tuple::Element::unnamed(Numeric {.value = 2, .unit = ""}),
    });
    auto resolved = ArgumentParser {
        {.name = "x"},
        {.name = "y", .defaultValue = Numeric {.value = 99, .unit = ""}}
    }.resolve(args);

    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 2.0);
}

TEST_F(ArgumentParserTest, DefaultValueOverriddenByName)
{
    Tuple args({
        Tuple::Element::unnamed(Numeric {.value = 1, .unit = ""}),
        Tuple::Element::named("y", Numeric {.value = 2, .unit = ""}),
    });
    auto resolved = ArgumentParser {
        {.name = "x"},
        {.name = "y", .defaultValue = Numeric {.value = 99, .unit = ""}}
    }.resolve(args);

    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 2.0);
}

TEST_F(ArgumentParserTest, ResolvedTupleHasCorrectOrder)
{
    Tuple args({
        Tuple::Element::named("y", Numeric {.value = 2, .unit = ""}),
        Tuple::Element::unnamed(Numeric {.value = 1, .unit = ""}),
    });
    auto resolved = ArgumentParser {{.name = "x"}, {.name = "y"}}.resolve(args);

    // at(0) is x, at(1) is y — matches declaration order
    EXPECT_DOUBLE_EQ(resolved.at(0).get<Numeric>().value, 1.0);
    EXPECT_DOUBLE_EQ(resolved.at(1).get<Numeric>().value, 2.0);
}

TEST_F(ArgumentParserTest, MixedTypes)
{
    Tuple args({
        Tuple::Element::unnamed(Base::Color(1.0, 0.0, 0.0)),
        Tuple::Element::unnamed(Numeric {.value = 20, .unit = ""}),
    });
    auto resolved = ArgumentParser {{.name = "color"}, {.name = "amount"}}.resolve(args);

    EXPECT_TRUE(resolved.find("color")->holds<Base::Color>());
    EXPECT_TRUE(resolved.find("amount")->holds<Numeric>());
}

TEST_F(ArgumentParserTest, ErrorOnUnknownName)
{
    Tuple args({
        Tuple::Element::named("unknown", Numeric {.value = 1, .unit = ""}),
    });
    ArgumentParser parser {{"x"}, {"y"}};
    EXPECT_THROW(parser.resolve(args), Base::ExpressionError);
}

TEST_F(ArgumentParserTest, ErrorOnDuplicateName)
{
    Tuple args({
        Tuple::Element::named("x", Numeric {.value = 1, .unit = ""}),
        Tuple::Element::named("x", Numeric {.value = 2, .unit = ""}),
    });
    ArgumentParser parser {{"x"}, {"y"}};
    EXPECT_THROW(parser.resolve(args), Base::ExpressionError);
}

TEST_F(ArgumentParserTest, ErrorOnMissingRequired)
{
    Tuple args({
        Tuple::Element::unnamed(Numeric {.value = 1, .unit = ""}),
    });
    ArgumentParser parser {{"x"}, {"y"}};
    EXPECT_THROW(parser.resolve(args), Base::ExpressionError);
}

TEST_F(ArgumentParserTest, ErrorOnExcessPositional)
{
    Tuple args({
        Tuple::Element::unnamed(Numeric {.value = 1, .unit = ""}),
        Tuple::Element::unnamed(Numeric {.value = 2, .unit = ""}),
        Tuple::Element::unnamed(Numeric {.value = 3, .unit = ""}),
    });
    ArgumentParser parser {{"x"}, {"y"}};
    EXPECT_THROW(parser.resolve(args), Base::ExpressionError);
}

TEST_F(ArgumentParserTest, TypedGetSuccess)
{
    Tuple args({
        Tuple::Element::unnamed(Base::Color(1.0, 0.0, 0.0)),
        Tuple::Element::unnamed(Numeric {.value = 20, .unit = ""}),
    });
    auto resolved = ArgumentParser {{.name = "color"}, {.name = "amount"}}.resolve(args);

    EXPECT_NO_THROW(resolved.get<Base::Color>("color"));
    EXPECT_NO_THROW(resolved.get<Numeric>("amount"));
    EXPECT_DOUBLE_EQ(resolved.get<Numeric>("amount").value, 20.0);
}

TEST_F(ArgumentParserTest, TypedGetWrongType)
{
    Tuple args({
        Tuple::Element::unnamed(Numeric {.value = 10, .unit = "px"}),
        Tuple::Element::unnamed(Numeric {.value = 20, .unit = ""}),
    });
    auto resolved = ArgumentParser {{.name = "color"}, {.name = "amount"}}.resolve(args);

    EXPECT_THROW(resolved.get<Base::Color>("color"), Base::ExpressionError);
}

TEST_F(ArgumentParserTest, TypedGetMissingName)
{
    Tuple args({Tuple::Element::unnamed(Numeric {.value = 10, .unit = ""})});
    auto resolved = ArgumentParser {{.name = "x"}}.resolve(args);

    EXPECT_THROW(resolved.get<Numeric>("nonexistent"), Base::ExpressionError);
}

// Insets / shorthand constructor tests

TEST_F(ParserTest, PaddingShorthand1Arg)
{
    Parser parser("padding(10px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::Padding);
    EXPECT_EQ(tuple.size(), 4);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("right").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("left").value, 10.0);
}

TEST_F(ParserTest, PaddingShorthand2Args)
{
    Parser parser("padding(10px, 5px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::Padding);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("right").value, 5.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("left").value, 5.0);
}

TEST_F(ParserTest, PaddingShorthand3Args)
{
    Parser parser("padding(10px, 5px, 20px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::Padding);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("right").value, 5.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom").value, 20.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("left").value, 5.0);
}

TEST_F(ParserTest, PaddingShorthand4Args)
{
    Parser parser("padding(10px, 5px, 20px, 15px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::Padding);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("right").value, 5.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom").value, 20.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("left").value, 15.0);
}

TEST_F(ParserTest, PaddingNamedVerticalHorizontal)
{
    Parser parser("padding(vertical: 10px, horizontal: 5px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("right").value, 5.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("left").value, 5.0);
}

TEST_F(ParserTest, PaddingNamedExplicitSides)
{
    Parser parser("padding(top: 1px, right: 2px, bottom: 3px, left: 4px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top").value, 1.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("right").value, 2.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom").value, 3.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("left").value, 4.0);
}

TEST_F(ParserTest, PaddingMixedNamedOverride)
{
    // Start with uniform 10px, then override top
    Parser parser("padding(10px, top: 20px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top").value, 20.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("right").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("left").value, 10.0);
}

TEST_F(ParserTest, MarginsFunction)
{
    Parser parser("margins(5px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    EXPECT_EQ(result.get<Tuple>().kind, TupleKind::Margins);
}

TEST_F(ParserTest, InsetsConvertsTupleArgToTargetKind)
{
    // padding(margins(...)) should re-tag the margins tuple as padding
    Parser parser("padding(margins(1px, 2px, 3px, 4px))");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::Padding);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top").value, 1.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("right").value, 2.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom").value, 3.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("left").value, 4.0);
}

TEST_F(ParserTest, InsetsConvertsGenericTupleArg)
{
    Parser parser("padding((top: 5px, right: 10px, bottom: 15px, left: 20px))");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::Padding);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top").value, 5.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("left").value, 20.0);
}

TEST_F(ParserTest, BorderThicknessFunction)
{
    Parser parser("border_thickness(1px, 2px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    EXPECT_EQ(result.get<Tuple>().kind, TupleKind::BorderThickness);
    EXPECT_DOUBLE_EQ(result.get<Tuple>().get<Numeric>("top").value, 1.0);
    EXPECT_DOUBLE_EQ(result.get<Tuple>().get<Numeric>("right").value, 2.0);
}

// Kind propagation through arithmetic

TEST_F(ParserTest, KindPreservedThroughAdd)
{
    Parser parser("padding(10px) + padding(5px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    EXPECT_EQ(result.get<Tuple>().kind, TupleKind::Padding);
    EXPECT_DOUBLE_EQ(result.get<Tuple>().get<Numeric>("top").value, 15.0);
}

TEST_F(ParserTest, KindPreservedThroughScalarMultiply)
{
    Parser parser("padding(10px) * 2");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    EXPECT_EQ(result.get<Tuple>().kind, TupleKind::Padding);
    EXPECT_DOUBLE_EQ(result.get<Tuple>().get<Numeric>("top").value, 20.0);
}

TEST_F(ParserTest, KindPreservedThroughNegate)
{
    Parser parser("-padding(10px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    EXPECT_EQ(result.get<Tuple>().kind, TupleKind::Padding);
    EXPECT_DOUBLE_EQ(result.get<Tuple>().get<Numeric>("top").value, -10.0);
}

TEST_F(ParserTest, KindMismatchError)
{
    EXPECT_THROW(
        {
            Parser parser("padding(10px) + margins(5px)");
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::ExpressionError
    );
}

TEST_F(ParserTest, TypedKindPlusGenericKeepsKind)
{
    Parser parser("padding(10px) + (top: 5px, right: 5px, bottom: 5px, left: 5px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    EXPECT_EQ(result.get<Tuple>().kind, TupleKind::Padding);
    EXPECT_DOUBLE_EQ(result.get<Tuple>().get<Numeric>("top").value, 15.0);
}

// Insets C++ wrapper tests

TEST_F(ParserTest, InsetsPaddingWrapper)
{
    Parser parser("padding(1px, 2px, 3px, 4px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());

    Padding padding(result.get<Tuple>());
    EXPECT_DOUBLE_EQ(padding.top().value, 1.0);
    EXPECT_DOUBLE_EQ(padding.right().value, 2.0);
    EXPECT_DOUBLE_EQ(padding.bottom().value, 3.0);
    EXPECT_DOUBLE_EQ(padding.left().value, 4.0);
    EXPECT_DOUBLE_EQ(padding.horizontal().value, 6.0);
    EXPECT_DOUBLE_EQ(padding.vertical().value, 4.0);
}

TEST_F(ParserTest, InsetsWrongKindThrows)
{
    Parser parser("padding(10px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());

    EXPECT_THROW(Margins(result.get<Tuple>()), Base::TypeError);
}

// Typed resolve via ParameterManager

TEST_F(ParserTest, ResolveTypedPadding)
{
    auto source = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {{.name = "TestPadding", .value = "padding(1px, 2px, 3px, 4px)"}},
        ParameterSource::Metadata {.name = "Insets Source"}
    );

    Gui::StyleParameters::ParameterManager mgr;
    mgr.addSource(source.get());

    // Construct a default Padding for the definition
    Tuple defaultTuple(
        {
            Tuple::Element::named("top", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("right", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("bottom", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("left", Numeric {.value = 0, .unit = "px"}),
        },
        TupleKind::Padding
    );

    ParameterDefinition<Padding> def {.name = "TestPadding", .defaultValue = Padding(defaultTuple)};
    auto resolved = mgr.resolve(def);

    EXPECT_DOUBLE_EQ(resolved.top().value, 1.0);
    EXPECT_DOUBLE_EQ(resolved.right().value, 2.0);
    EXPECT_DOUBLE_EQ(resolved.bottom().value, 3.0);
    EXPECT_DOUBLE_EQ(resolved.left().value, 4.0);
}

TEST_F(ParserTest, ResolveGenericTupleAsPadding)
{
    auto source = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {
            {.name = "ButtonPadding", .value = "(top: 10px, right: 5px, bottom: 10px, left: 20px)"}
        },
        ParameterSource::Metadata {.name = "Generic Source"}
    );

    Gui::StyleParameters::ParameterManager mgr;
    mgr.addSource(source.get());

    Tuple defaultTuple(
        {
            Tuple::Element::named("top", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("right", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("bottom", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("left", Numeric {.value = 0, .unit = "px"}),
        },
        TupleKind::Padding
    );

    ParameterDefinition<Padding> def {.name = "ButtonPadding", .defaultValue = Padding(defaultTuple)};
    auto resolved = mgr.resolve(def);

    EXPECT_DOUBLE_EQ(resolved.top().value, 10.0);
    EXPECT_DOUBLE_EQ(resolved.right().value, 5.0);
    EXPECT_DOUBLE_EQ(resolved.bottom().value, 10.0);
    EXPECT_DOUBLE_EQ(resolved.left().value, 20.0);
}

TEST_F(ParserTest, ResolveGenericTupleWithGroupNames)
{
    auto source = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {{.name = "ButtonPadding", .value = "(horizontal: 10px, vertical: 20px)"}},
        ParameterSource::Metadata {.name = "Generic Source"}
    );

    Gui::StyleParameters::ParameterManager mgr;
    mgr.addSource(source.get());

    Tuple defaultTuple(
        {
            Tuple::Element::named("top", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("right", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("bottom", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("left", Numeric {.value = 0, .unit = "px"}),
        },
        TupleKind::Padding
    );

    ParameterDefinition<Padding> def {.name = "ButtonPadding", .defaultValue = Padding(defaultTuple)};
    auto resolved = mgr.resolve(def);

    EXPECT_DOUBLE_EQ(resolved.top().value, 20.0);
    EXPECT_DOUBLE_EQ(resolved.right().value, 10.0);
    EXPECT_DOUBLE_EQ(resolved.bottom().value, 20.0);
    EXPECT_DOUBLE_EQ(resolved.left().value, 10.0);
}

TEST_F(ParserTest, ResolveGenericTupleWithPositionalShorthand)
{
    auto source = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {{.name = "ButtonPadding", .value = "(10px, 5px)"}},
        ParameterSource::Metadata {.name = "Generic Source"}
    );

    Gui::StyleParameters::ParameterManager mgr;
    mgr.addSource(source.get());

    Tuple defaultTuple(
        {
            Tuple::Element::named("top", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("right", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("bottom", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("left", Numeric {.value = 0, .unit = "px"}),
        },
        TupleKind::Padding
    );

    ParameterDefinition<Padding> def {.name = "ButtonPadding", .defaultValue = Padding(defaultTuple)};
    auto resolved = mgr.resolve(def);

    EXPECT_DOUBLE_EQ(resolved.top().value, 10.0);
    EXPECT_DOUBLE_EQ(resolved.right().value, 5.0);
    EXPECT_DOUBLE_EQ(resolved.bottom().value, 10.0);
    EXPECT_DOUBLE_EQ(resolved.left().value, 5.0);
}

TEST_F(ParserTest, ResolveTypedPaddingFallsBackOnWrongKind)
{
    auto source = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {{.name = "TestMargins", .value = "margins(10px)"}},
        ParameterSource::Metadata {.name = "Insets Source"}
    );

    Gui::StyleParameters::ParameterManager mgr;
    mgr.addSource(source.get());

    Tuple defaultTuple(
        {
            Tuple::Element::named("top", Numeric {.value = 5, .unit = "px"}),
            Tuple::Element::named("right", Numeric {.value = 5, .unit = "px"}),
            Tuple::Element::named("bottom", Numeric {.value = 5, .unit = "px"}),
            Tuple::Element::named("left", Numeric {.value = 5, .unit = "px"}),
        },
        TupleKind::Padding
    );

    ParameterDefinition<Padding> def {.name = "TestMargins", .defaultValue = Padding(defaultTuple)};
    auto resolved = mgr.resolve(def);

    // Should return the default because kind mismatch
    EXPECT_DOUBLE_EQ(resolved.top().value, 5.0);
}

TEST_F(ParserTest, ResolveTypedPaddingFallsBackOnMissing)
{
    Gui::StyleParameters::ParameterManager mgr;

    Tuple defaultTuple(
        {
            Tuple::Element::named("top", Numeric {.value = 7, .unit = "px"}),
            Tuple::Element::named("right", Numeric {.value = 7, .unit = "px"}),
            Tuple::Element::named("bottom", Numeric {.value = 7, .unit = "px"}),
            Tuple::Element::named("left", Numeric {.value = 7, .unit = "px"}),
        },
        TupleKind::Padding
    );

    ParameterDefinition<Padding> def {.name = "NonExistent", .defaultValue = Padding(defaultTuple)};
    auto resolved = mgr.resolve(def);

    EXPECT_DOUBLE_EQ(resolved.top().value, 7.0);
}

// Corners / border_radius tests

TEST_F(ParserTest, BorderRadiusShorthand1Arg)
{
    Parser parser("border_radius(10px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::Corners);
    EXPECT_EQ(tuple.size(), 4);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top_left").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top_right").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom_right").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom_left").value, 10.0);
}

TEST_F(ParserTest, BorderRadiusShorthand2Args)
{
    // 2 values: top-left/bottom-right, top-right/bottom-left (diagonal pairing)
    Parser parser("border_radius(10px, 5px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::Corners);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top_left").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top_right").value, 5.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom_right").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom_left").value, 5.0);
}

TEST_F(ParserTest, BorderRadiusShorthand3Args)
{
    // 3 values: top-left, top-right/bottom-left, bottom-right
    Parser parser("border_radius(10px, 5px, 20px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::Corners);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top_left").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top_right").value, 5.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom_right").value, 20.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom_left").value, 5.0);
}

TEST_F(ParserTest, BorderRadiusShorthand4Args)
{
    Parser parser("border_radius(10px, 5px, 20px, 15px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::Corners);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top_left").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top_right").value, 5.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom_right").value, 20.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom_left").value, 15.0);
}

TEST_F(ParserTest, BorderRadiusNamedOverride)
{
    Parser parser("border_radius(10px, top_left: 20px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top_left").value, 20.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top_right").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom_right").value, 10.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom_left").value, 10.0);
}

TEST_F(ParserTest, BorderRadiusConvertsTupleArg)
{
    // Single tuple argument gets re-tagged
    Parser parser(
        "border_radius((top_left: 1px, top_right: 2px, bottom_right: 3px, bottom_left: 4px))"
    );
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::Corners);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("top_left").value, 1.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("bottom_right").value, 3.0);
}

TEST_F(ParserTest, BorderRadiusKindPreservedThroughArithmetic)
{
    Parser parser("border_radius(10px) + border_radius(5px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    EXPECT_EQ(result.get<Tuple>().kind, TupleKind::Corners);
    EXPECT_DOUBLE_EQ(result.get<Tuple>().get<Numeric>("top_left").value, 15.0);
}

TEST_F(ParserTest, BorderRadiusKindMismatchWithInsetsThrows)
{
    EXPECT_THROW(
        {
            Parser parser("border_radius(10px) + padding(5px)");
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::ExpressionError
    );
}

TEST_F(ParserTest, CornersWrapper)
{
    Parser parser("border_radius(1px, 2px, 3px, 4px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());

    Corners radii(result.get<Tuple>());
    EXPECT_DOUBLE_EQ(radii.topLeft().value, 1.0);
    EXPECT_DOUBLE_EQ(radii.topRight().value, 2.0);
    EXPECT_DOUBLE_EQ(radii.bottomRight().value, 3.0);
    EXPECT_DOUBLE_EQ(radii.bottomLeft().value, 4.0);
}

TEST_F(ParserTest, CornersWrongKindThrows)
{
    Parser parser("padding(10px)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());

    EXPECT_THROW(Corners(result.get<Tuple>()), Base::TypeError);
}

TEST_F(ParserTest, ResolveTypedCorners)
{
    auto source = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {{.name = "TestRadius", .value = "border_radius(1px, 2px, 3px, 4px)"}},
        ParameterSource::Metadata {.name = "Corners Source"}
    );

    Gui::StyleParameters::ParameterManager mgr;
    mgr.addSource(source.get());

    Tuple defaultTuple(
        {
            Tuple::Element::named("top_left", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("top_right", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("bottom_right", Numeric {.value = 0, .unit = "px"}),
            Tuple::Element::named("bottom_left", Numeric {.value = 0, .unit = "px"}),
        },
        TupleKind::Corners
    );

    ParameterDefinition<Corners> def {.name = "TestRadius", .defaultValue = Corners(defaultTuple)};
    auto resolved = mgr.resolve(def);

    EXPECT_DOUBLE_EQ(resolved.topLeft().value, 1.0);
    EXPECT_DOUBLE_EQ(resolved.topRight().value, 2.0);
    EXPECT_DOUBLE_EQ(resolved.bottomRight().value, 3.0);
    EXPECT_DOUBLE_EQ(resolved.bottomLeft().value, 4.0);
}

// Gradient tests

TEST_F(ParserTest, LinearGradientMinimal)
{
    Parser parser("linear_gradient(#ff0000, #0000ff)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::LinearGradient);

    // Default geometry: top to bottom
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("x1").value, 0.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("y1").value, 0.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("x2").value, 0.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("y2").value, 1.0);

    // Stops gathered in named "stops" element
    const auto* stopsValue = tuple.find("stops");
    ASSERT_NE(stopsValue, nullptr);
    ASSERT_TRUE(stopsValue->holds<Tuple>());
    const auto& stopsTuple = stopsValue->get<Tuple>();
    EXPECT_EQ(stopsTuple.size(), 2);

    // First stop: position 0, red
    const auto& firstStop = stopsTuple.at(0).get<Tuple>();
    EXPECT_DOUBLE_EQ(firstStop.at(0).get<Numeric>().value, 0.0);
    EXPECT_DOUBLE_EQ(firstStop.at(1).get<Base::Color>().r, 1.0);
    EXPECT_DOUBLE_EQ(firstStop.at(1).get<Base::Color>().g, 0.0);
    EXPECT_DOUBLE_EQ(firstStop.at(1).get<Base::Color>().b, 0.0);

    // Second stop: position 1, blue
    const auto& secondStop = stopsTuple.at(1).get<Tuple>();
    EXPECT_DOUBLE_EQ(secondStop.at(0).get<Numeric>().value, 1.0);
    EXPECT_DOUBLE_EQ(secondStop.at(1).get<Base::Color>().r, 0.0);
    EXPECT_DOUBLE_EQ(secondStop.at(1).get<Base::Color>().g, 0.0);
    EXPECT_DOUBLE_EQ(secondStop.at(1).get<Base::Color>().b, 1.0);
}

TEST_F(ParserTest, LinearGradientExplicitStops)
{
    Parser parser("linear_gradient((0, #ff0000), (0.5, #00ff00), (1, #0000ff))");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::LinearGradient);

    const auto& stopsTuple = tuple.get<Tuple>("stops");
    EXPECT_EQ(stopsTuple.size(), 3);

    EXPECT_DOUBLE_EQ(stopsTuple.at(0).get<Tuple>().at(0).get<Numeric>().value, 0.0);
    EXPECT_DOUBLE_EQ(stopsTuple.at(1).get<Tuple>().at(0).get<Numeric>().value, 0.5);
    EXPECT_DOUBLE_EQ(stopsTuple.at(2).get<Tuple>().at(0).get<Numeric>().value, 1.0);

    // Middle stop is green
    EXPECT_DOUBLE_EQ(stopsTuple.at(1).get<Tuple>().at(1).get<Base::Color>().g, 1.0);
}

TEST_F(ParserTest, LinearGradientCustomGeometry)
{
    Parser parser("linear_gradient(x1: 0, y1: 0, x2: 1, y2: 0, #ff0000, #0000ff)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::LinearGradient);

    // Custom geometry: left to right
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("x1").value, 0.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("y1").value, 0.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("x2").value, 1.0);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("y2").value, 0.0);

    const auto& stopsTuple = tuple.get<Tuple>("stops");
    EXPECT_EQ(stopsTuple.size(), 2);
}

TEST_F(ParserTest, LinearGradientMixedStops)
{
    // Bare color + explicit (position, color) tuples
    Parser parser("linear_gradient(#ff0000, (0.5, #00ff00), #0000ff)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());

    const auto& stopsTuple = result.get<Tuple>().get<Tuple>("stops");
    EXPECT_EQ(stopsTuple.size(), 3);

    // Bare #ff0000 at index 0 → position 0/2 = 0.0
    EXPECT_DOUBLE_EQ(stopsTuple.at(0).get<Tuple>().at(0).get<Numeric>().value, 0.0);
    // Explicit (0.5, #00ff00)
    EXPECT_DOUBLE_EQ(stopsTuple.at(1).get<Tuple>().at(0).get<Numeric>().value, 0.5);
    // Bare #0000ff at index 2 → position 2/2 = 1.0
    EXPECT_DOUBLE_EQ(stopsTuple.at(2).get<Tuple>().at(0).get<Numeric>().value, 1.0);
}

TEST_F(ParserTest, RadialGradientMinimal)
{
    Parser parser("radial_gradient(#ff0000, #0000ff)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::RadialGradient);

    // Default geometry
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("cx").value, 0.5);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("cy").value, 0.5);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("radius").value, 0.5);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("fx").value, 0.5);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("fy").value, 0.5);

    const auto& stopsTuple = tuple.get<Tuple>("stops");
    EXPECT_EQ(stopsTuple.size(), 2);
}

TEST_F(ParserTest, RadialGradientCustomGeometry)
{
    Parser parser("radial_gradient(cx: 0.3, cy: 0.3, radius: 0.8, (0, #ff0000), (1, #0000ff))");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();

    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("cx").value, 0.3);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("cy").value, 0.3);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("radius").value, 0.8);
    // fx/fy default to cx/cy when not specified
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("fx").value, 0.3);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("fy").value, 0.3);
}

TEST_F(ParserTest, RadialGradientExplicitFocalPoint)
{
    Parser parser("radial_gradient(cx: 0.5, cy: 0.5, fx: 0.2, fy: 0.8, #ff0000, #0000ff)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();

    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("fx").value, 0.2);
    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("fy").value, 0.8);
}

TEST_F(ParserTest, GradientTooFewStops)
{
    EXPECT_THROW(
        {
            Parser parser("linear_gradient(#ff0000)");
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::ExpressionError
    );

    EXPECT_THROW(
        {
            Parser parser("radial_gradient()");
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::ExpressionError
    );
}

TEST_F(ParserTest, GradientKindMismatchThrows)
{
    // Build a RadialGradient tuple, then try to wrap it as LinearGradient
    Parser parser("radial_gradient(#ff0000, #0000ff)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());

    EXPECT_THROW(LinearGradient(result.get<Tuple>()), Base::TypeError);
}

TEST_F(ParserTest, GradientWrapper)
{
    Parser parser("linear_gradient(x1: 0.1, y1: 0.2, x2: 0.3, y2: 0.4, #ff0000, #0000ff)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());

    LinearGradient gradient(result.get<Tuple>());
    EXPECT_DOUBLE_EQ(gradient.x1(), 0.1);
    EXPECT_DOUBLE_EQ(gradient.y1(), 0.2);
    EXPECT_DOUBLE_EQ(gradient.x2(), 0.3);
    EXPECT_DOUBLE_EQ(gradient.y2(), 0.4);

    auto stops = gradient.colorStops();
    ASSERT_EQ(stops.size(), 2);
    EXPECT_DOUBLE_EQ(stops[0].position.value, 0.0);
    EXPECT_DOUBLE_EQ(stops[0].color.r, 1.0);
    EXPECT_DOUBLE_EQ(stops[1].position.value, 1.0);
    EXPECT_DOUBLE_EQ(stops[1].color.b, 1.0);
}

TEST_F(ParserTest, RadialGradientWrapper)
{
    Parser parser("radial_gradient(cx: 0.3, cy: 0.4, radius: 0.7, fx: 0.1, fy: 0.2, #ff0000, #0000ff)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());

    RadialGradient gradient(result.get<Tuple>());
    EXPECT_DOUBLE_EQ(gradient.cx(), 0.3);
    EXPECT_DOUBLE_EQ(gradient.cy(), 0.4);
    EXPECT_DOUBLE_EQ(gradient.radius(), 0.7);
    EXPECT_DOUBLE_EQ(gradient.fx(), 0.1);
    EXPECT_DOUBLE_EQ(gradient.fy(), 0.2);

    auto stops = gradient.colorStops();
    ASSERT_EQ(stops.size(), 2);
}

TEST_F(ParserTest, GradientStopsAccessibleViaMemberAccess)
{
    Parser parser("linear_gradient(#ff0000, #0000ff).stops");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Tuple>());
    EXPECT_EQ(result.get<Tuple>().size(), 2);
}

TEST_F(ParserTest, ResolveTypedLinearGradient)
{
    auto source = std::make_unique<InMemoryParameterSource>(
        std::list<Parameter> {
            {.name = "TestGradient", .value = "linear_gradient(#ff0000, #00ff00, #0000ff)"}
        },
        ParameterSource::Metadata {.name = "Gradient Source"}
    );

    Gui::StyleParameters::ParameterManager mgr;
    mgr.addSource(source.get());

    // Build a default LinearGradient tuple
    Tuple defaultStopsTuple({
        Tuple::Element::unnamed(Tuple({
            Tuple::Element::unnamed(Numeric {.value = 0, .unit = ""}),
            Tuple::Element::unnamed(Base::Color(0, 0, 0)),
        })),
        Tuple::Element::unnamed(Tuple({
            Tuple::Element::unnamed(Numeric {.value = 1, .unit = ""}),
            Tuple::Element::unnamed(Base::Color(1, 1, 1)),
        })),
    });

    Tuple defaultTuple(
        {
            Tuple::Element::named("x1", Numeric {.value = 0, .unit = ""}),
            Tuple::Element::named("y1", Numeric {.value = 0, .unit = ""}),
            Tuple::Element::named("x2", Numeric {.value = 0, .unit = ""}),
            Tuple::Element::named("y2", Numeric {.value = 1, .unit = ""}),
            Tuple::Element::named("stops", std::move(defaultStopsTuple)),
        },
        TupleKind::LinearGradient
    );

    ParameterDefinition<LinearGradient> def {
        .name = "TestGradient",
        .defaultValue = LinearGradient(defaultTuple),
    };
    auto resolved = mgr.resolve(def);

    auto stops = resolved.colorStops();
    ASSERT_EQ(stops.size(), 3);
    EXPECT_DOUBLE_EQ(stops[0].position.value, 0.0);
    EXPECT_DOUBLE_EQ(stops[1].position.value, 0.5);
    EXPECT_DOUBLE_EQ(stops[2].position.value, 1.0);
    EXPECT_DOUBLE_EQ(stops[1].color.g, 1.0);  // middle stop is green
}

// Gradient color function tests

TEST_F(ParserTest, LightenGradient)
{
    Parser parser("lighten(linear_gradient(#ff0000, #0000ff), 20)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});

    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::LinearGradient);

    LinearGradient gradient(tuple);
    auto stops = gradient.colorStops();
    ASSERT_EQ(stops.size(), 2);

    // Each stop should be lighter than the original
    auto originalRed = QColor(0xff, 0x00, 0x00);
    auto originalBlue = QColor(0x00, 0x00, 0xff);
    EXPECT_GT(stops[0].color.asValue<QColor>().lightness(), originalRed.lightness());
    EXPECT_GT(stops[1].color.asValue<QColor>().lightness(), originalBlue.lightness());

    // Positions should be preserved
    EXPECT_DOUBLE_EQ(stops[0].position.value, 0.0);
    EXPECT_DOUBLE_EQ(stops[1].position.value, 1.0);
}

TEST_F(ParserTest, DarkenGradient)
{
    Parser parser("darken(radial_gradient(#ff0000, #00ff00), 20)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});

    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::RadialGradient);

    RadialGradient gradient(tuple);
    auto stops = gradient.colorStops();
    ASSERT_EQ(stops.size(), 2);

    // Each stop should be darker than the original
    auto originalRed = QColor(0xff, 0x00, 0x00);
    auto originalGreen = QColor(0x00, 0xff, 0x00);
    EXPECT_LT(stops[0].color.asValue<QColor>().lightness(), originalRed.lightness());
    EXPECT_LT(stops[1].color.asValue<QColor>().lightness(), originalGreen.lightness());

    // Geometry should be preserved
    EXPECT_DOUBLE_EQ(gradient.cx(), 0.5);
    EXPECT_DOUBLE_EQ(gradient.cy(), 0.5);
    EXPECT_DOUBLE_EQ(gradient.radius(), 0.5);
}

TEST_F(ParserTest, BlendGradientWithColor)
{
    Parser parser("blend(linear_gradient(#ff0000, #0000ff), #000000, 50)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});

    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::LinearGradient);

    LinearGradient gradient(tuple);
    auto stops = gradient.colorStops();
    ASSERT_EQ(stops.size(), 2);

    // 50% blend of red with black should be ~half red
    EXPECT_NEAR(stops[0].color.r, 0.5, 0.02);
    EXPECT_NEAR(stops[0].color.g, 0.0, 0.02);
    EXPECT_NEAR(stops[0].color.b, 0.0, 0.02);

    // 50% blend of blue with black should be ~half blue
    EXPECT_NEAR(stops[1].color.r, 0.0, 0.02);
    EXPECT_NEAR(stops[1].color.g, 0.0, 0.02);
    EXPECT_NEAR(stops[1].color.b, 0.5, 0.02);
}

TEST_F(ParserTest, BlendColorWithGradient)
{
    Parser parser("blend(#000000, linear_gradient(#ff0000, #0000ff), 50)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});

    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::LinearGradient);

    LinearGradient gradient(tuple);
    auto stops = gradient.colorStops();
    ASSERT_EQ(stops.size(), 2);

    // 50% blend of black with red should be ~half red
    EXPECT_NEAR(stops[0].color.r, 0.5, 0.02);
    EXPECT_NEAR(stops[0].color.g, 0.0, 0.02);
    EXPECT_NEAR(stops[0].color.b, 0.0, 0.02);

    // 50% blend of black with blue should be ~half blue
    EXPECT_NEAR(stops[1].color.r, 0.0, 0.02);
    EXPECT_NEAR(stops[1].color.g, 0.0, 0.02);
    EXPECT_NEAR(stops[1].color.b, 0.5, 0.02);
}

TEST_F(ParserTest, BlendTwoGradientsThrows)
{
    EXPECT_THROW(
        {
            Parser parser(
                "blend(linear_gradient(#ff0000, #0000ff), linear_gradient(#00ff00, #ffff00), 50)"
            );
            auto expr = parser.parse();
            expr->evaluate({.manager = &manager, .context = {}});
        },
        Base::ExpressionError
    );
}

// Shade function tests

TEST_F(ParserTest, ShadeBasicColor)
{
    Parser parser("shade(#ff0000, 0.8)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Base::Color>());

    auto shaded = result.get<Base::Color>();
    auto oklch = Base::toOkLch(shaded);
    EXPECT_NEAR(oklch.lightness, 0.8f, 0.02f);

    // Hue should be approximately preserved
    auto originalOklch = Base::toOkLch(Base::Color(1.0f, 0.0f, 0.0f));
    EXPECT_NEAR(oklch.hue, originalOklch.hue, 5.0f);
}

TEST_F(ParserTest, ShadeWithPercentage)
{
    Parser parser("shade(#ff0000, 80%)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Base::Color>());

    auto shaded = result.get<Base::Color>();
    auto oklch = Base::toOkLch(shaded);
    EXPECT_NEAR(oklch.lightness, 0.8f, 0.02f);
}

TEST_F(ParserTest, ShadeBlack)
{
    Parser parser("shade(#000000, 0.5)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Base::Color>());

    auto shaded = result.get<Base::Color>();
    // Should produce a non-black color with L ~ 0.5
    auto oklch = Base::toOkLch(shaded);
    EXPECT_NEAR(oklch.lightness, 0.5f, 0.02f);
    EXPECT_TRUE(shaded.r > 0.0f || shaded.g > 0.0f || shaded.b > 0.0f);
}

TEST_F(ParserTest, ShadeWhite)
{
    Parser parser("shade(#ffffff, 0.3)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});
    ASSERT_TRUE(result.holds<Base::Color>());

    auto shaded = result.get<Base::Color>();
    // Should produce a darker color
    auto oklch = Base::toOkLch(shaded);
    EXPECT_NEAR(oklch.lightness, 0.3f, 0.02f);
}

TEST_F(ParserTest, ShadeGradient)
{
    Parser parser("shade(linear_gradient(#ff0000, #0000ff), 0.5)");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});

    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.kind, TupleKind::LinearGradient);

    LinearGradient gradient(tuple);
    auto stops = gradient.colorStops();
    ASSERT_EQ(stops.size(), 2);

    // Each stop should have lightness ~ 0.5
    auto firstOklch = Base::toOkLch(stops[0].color);
    auto secondOklch = Base::toOkLch(stops[1].color);
    EXPECT_NEAR(firstOklch.lightness, 0.5f, 0.05f);
    EXPECT_NEAR(secondOklch.lightness, 0.5f, 0.05f);

    // Positions should be preserved
    EXPECT_DOUBLE_EQ(stops[0].position.value, 0.0);
    EXPECT_DOUBLE_EQ(stops[1].position.value, 1.0);
}

TEST_F(ParserTest, ShadesBasic)
{
    Parser parser("shades(#ff0000, (light: 0.8, dark: 0.3))");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});

    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);

    // "light" shade
    const auto* lightValue = tuple.find("light");
    ASSERT_NE(lightValue, nullptr);
    ASSERT_TRUE(lightValue->holds<Base::Color>());
    auto lightOklch = Base::toOkLch(lightValue->get<Base::Color>());
    EXPECT_NEAR(lightOklch.lightness, 0.8f, 0.02f);

    // "dark" shade
    const auto* darkValue = tuple.find("dark");
    ASSERT_NE(darkValue, nullptr);
    ASSERT_TRUE(darkValue->holds<Base::Color>());
    auto darkOklch = Base::toOkLch(darkValue->get<Base::Color>());
    EXPECT_NEAR(darkOklch.lightness, 0.3f, 0.02f);
}

TEST_F(ParserTest, ShadesWithPercentage)
{
    Parser parser("shades(#ff0000, (light: 80%, dark: 30%))");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});

    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);

    auto lightOklch = Base::toOkLch(tuple.find("light")->get<Base::Color>());
    EXPECT_NEAR(lightOklch.lightness, 0.8f, 0.02f);

    auto darkOklch = Base::toOkLch(tuple.find("dark")->get<Base::Color>());
    EXPECT_NEAR(darkOklch.lightness, 0.3f, 0.02f);
}

TEST_F(ParserTest, ShadesPreservesHue)
{
    Parser parser("shades(#ff0000, (a: 0.9, b: 0.5, c: 0.2))");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});

    ASSERT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 3);

    auto originalOklch = Base::toOkLch(Base::Color(1.0f, 0.0f, 0.0f));

    for (const auto& element : tuple.elements) {
        ASSERT_TRUE(element.value->holds<Base::Color>());
        auto shadeOklch = Base::toOkLch(element.value->get<Base::Color>());
        EXPECT_NEAR(shadeOklch.hue, originalOklch.hue, 5.0f);
    }
}
