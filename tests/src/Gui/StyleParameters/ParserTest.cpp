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

#include <Gui/Utilities.h>

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
                {"TestParam", "10px"},
                {"TestColor", "#ff0000"},
                {"TestNumber", "5"},
                {"TestTupleParam", "(left: 1px, right: 2px, top: 3px, bottom: 4px)"},
                {"TestIndexedTuple", "(10, 20, 30)"},
                {"TestNestedTuple", "((x: 1, y: 2), (x: 3, y: 4))"},
            },
            ParameterSource::Metadata {"Test Source"}
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
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 42.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10.5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 10.5);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("2.5em");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 2.5);
        EXPECT_EQ(length.unit, "em");
    }

    {
        Parser parser("100%");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
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
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result);
        EXPECT_EQ(color.r, 1);
        EXPECT_EQ(color.g, 0);
        EXPECT_EQ(color.b, 0);
    }

    {
        Parser parser("#00ff00");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result);
        EXPECT_EQ(color.r, 0);
        EXPECT_EQ(color.g, 1);
        EXPECT_EQ(color.b, 0);
    }

    {
        Parser parser("#0000ff");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result);
        EXPECT_EQ(color.r, 0);
        EXPECT_EQ(color.g, 0);
        EXPECT_EQ(color.b, 1);
    }

    {
        Parser parser("rgb(255, 0, 0)");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result);
        EXPECT_EQ(color.r, 1);
        EXPECT_EQ(color.g, 0);
        EXPECT_EQ(color.b, 0);
    }

    {
        Parser parser("rgba(255, 0, 0, 128)");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
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
        auto result = expr->evaluate({&manager, {}});
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
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px + 5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("10 - 5");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 5.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px - 5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 5.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("10 * 5");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 50.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px * 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 20.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("10 / 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 5.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px / 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
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
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 30.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("(10px + 5px) * 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 30.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("@TestParam + 5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("@TestParam * @TestNumber");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
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
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 10.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("-10");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, -10.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("-10px");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
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
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result).asValue<QColor>();
        // The result should be lighter than the original red
        EXPECT_GT(color.lightness(), QColor(0xff0000).lightness());
    }

    {
        Parser parser("darken(#ff0000, 20)");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Base::Color>(result));
        auto color = std::get<Base::Color>(result).asValue<QColor>();
        // The result should be darker than the original red
        EXPECT_LT(color.lightness(), QColor(0xff0000).lightness());
    }

    {
        Parser parser("lighten(@TestColor, 20)");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
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
            expr->evaluate({&manager, {}});
        },
        Base::ExpressionError
    );

    // Division by zero
    EXPECT_THROW(
        {
            Parser parser("10 / 0");
            auto expr = parser.parse();
            expr->evaluate({&manager, {}});
        },
        Base::RuntimeError
    );

    // Unit mismatch
    EXPECT_THROW(
        {
            Parser parser("10px + 5em");
            auto expr = parser.parse();
            expr->evaluate({&manager, {}});
        },
        Base::RuntimeError
    );

    // Unary operation on color
    EXPECT_THROW(
        {
            Parser parser("-@TestColor");
            auto expr = parser.parse();
            expr->evaluate({&manager, {}});
        },
        Base::ExpressionError
    );

    // Function with wrong number of arguments
    EXPECT_THROW(
        {
            Parser parser("lighten(#ff0000)");
            auto expr = parser.parse();
            expr->evaluate({&manager, {}});
        },
        Base::ExpressionError
    );

    // Function with wrong argument type
    EXPECT_THROW(
        {
            Parser parser("lighten(10px, 20)");
            auto expr = parser.parse();
            expr->evaluate({&manager, {}});
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
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px+5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("rgb(255,0,0)");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
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
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 42.0);
        EXPECT_EQ(length.unit, "");
    }

    // Single color
    {
        Parser parser("#ff0000");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
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
        auto result = expr->evaluate({&manager, {}});
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
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 14.0);  // 2 + (3 * 4) = 2 + 12 = 14
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10 - 3 * 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 4.0);  // 10 - (3 * 2) = 10 - 6 = 4
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("20 / 4 + 3");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
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
    auto result = expr->evaluate({&manager, {}});
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
    auto result = expr->evaluate({&manager, {}});
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
    auto result = expr->evaluate({&manager, {}});
    EXPECT_TRUE(result.holds<Tuple>());
    const auto& tuple = result.get<Tuple>();
    EXPECT_EQ(tuple.size(), 2);

    auto* x = tuple.find("x");
    ASSERT_NE(x, nullptr);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(*x).value, 10.0);
    EXPECT_DOUBLE_EQ(std::get<Numeric>(tuple.at(1)).value, 20.0);
}

// Test single named element is a tuple, not a grouped expression
TEST_F(ParserTest, ParseSingleNamedElementIsTuple)
{
    Parser parser("(x: 10)");
    auto expr = parser.parse();
    auto result = expr->evaluate({&manager, {}});
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
    auto result = expr->evaluate({&manager, {}});
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
    auto result = expr->evaluate({&manager, {}});
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
    auto result = expr->evaluate({&manager, {}});
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
    auto result = expr->evaluate({&manager, {}});
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
        auto result = expr->evaluate({&manager, {}});
        auto str = result.toString();
        EXPECT_EQ(str, "(x: 10, y: 20)");
    }

    {
        Parser parser("(10, 20, 30)");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        auto str = result.toString();
        EXPECT_EQ(str, "(10, 20, 30)");
    }

    {
        Parser parser("(x: 10, 20)");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        auto str = result.toString();
        EXPECT_EQ(str, "(x: 10, 20)");
    }
}

// Test Value::holds and Value::get
TEST_F(ParserTest, ValueHoldsAndGet)
{
    Value numericValue = Numeric {42, "px"};
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
    auto tupleValue = expr->evaluate({&manager, {}});
    EXPECT_TRUE(tupleValue.holds<Tuple>());
    EXPECT_FALSE(tupleValue.holds<Numeric>());
    EXPECT_EQ(tupleValue.get<Tuple>().size(), 2);
}

// Test Tuple::at out of bounds
TEST_F(ParserTest, TupleAtOutOfBounds)
{
    Parser parser("(10, 20)");
    auto expr = parser.parse();
    auto result = expr->evaluate({&manager, {}});
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
    auto result = expr->evaluate({&manager, {}});
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

        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        EXPECT_DOUBLE_EQ(std::get<Numeric>(result).value, 30.0);
    }

    {
        Parser parser("(42)");
        auto expr = parser.parse();

        auto result = expr->evaluate({&manager, {}});
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
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 20.0);  // (5) * 4 = 20
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("(10 - (3 + 2)) * 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
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

        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));

        auto length = std::get<Numeric>(result);
        EXPECT_DOUBLE_EQ(length.value, 1.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("@TestTupleParam.right");
        auto expr = parser.parse();

        auto result = expr->evaluate({&manager, {}});
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

        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        EXPECT_DOUBLE_EQ(std::get<Numeric>(result).value, 10.0);
    }

    {
        Parser parser("@TestIndexedTuple.2");
        auto expr = parser.parse();

        auto result = expr->evaluate({&manager, {}});
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
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        EXPECT_DOUBLE_EQ(std::get<Numeric>(result).value, 10.0);
    }

    {
        Parser parser("(10, 20, 30).1");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Numeric>(result));
        EXPECT_DOUBLE_EQ(std::get<Numeric>(result).value, 20.0);
    }
}

// Test member access in arithmetic expressions
TEST_F(ParserTest, MemberAccessInArithmetic)
{
    Parser parser("@TestTupleParam.left + @TestTupleParam.right");
    auto expr = parser.parse();

    auto result = expr->evaluate({&manager, {}});
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

    auto result = expr->evaluate({&manager, {}});
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
            expr->evaluate({&manager, {}});
        },
        Base::ExpressionError
    );

    // Index out of bounds
    EXPECT_THROW(
        {
            Parser parser("@TestIndexedTuple.5");
            auto expr = parser.parse();
            expr->evaluate({&manager, {}});
        },
        Base::RuntimeError
    );

    // Member access on non-tuple
    EXPECT_THROW(
        {
            Parser parser("@TestNumber.foo");
            auto expr = parser.parse();
            expr->evaluate({&manager, {}});
        },
        Base::ExpressionError
    );
}

// Tuple arithmetic tests

TEST_F(ParserTest, TupleElementWiseAdd)
{
    Parser parser("(1px, 2px) + (3px, 4px)");
    auto expr = parser.parse();

    auto result = expr->evaluate({&manager, {}});
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

    auto result = expr->evaluate({&manager, {}});
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

    auto result = expr->evaluate({&manager, {}});
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

    auto result = expr->evaluate({&manager, {}});
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

    auto result = expr->evaluate({&manager, {}});
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

    auto result = expr->evaluate({&manager, {}});
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

    auto result = expr->evaluate({&manager, {}});
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

    auto result = expr->evaluate({&manager, {}});
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

    auto result = expr->evaluate({&manager, {}});
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

    auto result = expr->evaluate({&manager, {}});
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

    auto result = expr->evaluate({&manager, {}});
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

    auto result = expr->evaluate({&manager, {}});
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
            expr->evaluate({&manager, {}});
        },
        Base::ExpressionError
    );
}

TEST_F(ParserTest, TupleNestedScalarMultiply)
{
    Parser parser("((1, 2), (3, 4)) * 2");
    auto expr = parser.parse();

    auto result = expr->evaluate({&manager, {}});
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
    struct NameValuePair
    {
        std::optional<std::string> name;
        Value value;
    };

protected:
    static Tuple makeTuple(std::vector<NameValuePair> elements)
    {
        Tuple tuple;
        for (auto& [name, value] : elements) {
            tuple.elements.emplace_back(name, std::make_shared<const Value>(std::move(value)));
        }
        return tuple;
    }
};

TEST_F(ArgumentParserTest, AllPositional)
{
    auto args = makeTuple({
        {.name = std::nullopt, .value = Numeric {1, ""}},
        {.name = std::nullopt, .value = Numeric {2, ""}},
    });
    auto resolved = ArgumentParser {{"x"}, {"y"}}.resolve(args);

    ASSERT_NE(resolved.find("x"), nullptr);
    ASSERT_NE(resolved.find("y"), nullptr);
    EXPECT_DOUBLE_EQ(resolved.find("x")->get<Numeric>().value, 1.0);
    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 2.0);
}

TEST_F(ArgumentParserTest, AllNamed)
{
    auto args = makeTuple({
        {.name = std::string("x"), .value = Numeric {10, ""}},
        {.name = std::string("y"), .value = Numeric {20, ""}},
    });
    auto resolved = ArgumentParser {{"x"}, {"y"}}.resolve(args);

    EXPECT_DOUBLE_EQ(resolved.find("x")->get<Numeric>().value, 10.0);
    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 20.0);
}

TEST_F(ArgumentParserTest, AllNamedReversedOrder)
{
    auto args = makeTuple({
        {.name = std::string("y"), .value = Numeric {20, ""}},
        {.name = std::string("x"), .value = Numeric {10, ""}},
    });
    auto resolved = ArgumentParser {{"x"}, {"y"}}.resolve(args);

    EXPECT_DOUBLE_EQ(resolved.find("x")->get<Numeric>().value, 10.0);
    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 20.0);
}

TEST_F(ArgumentParserTest, MixedPositionalThenNamed)
{
    // f(1, y: 2) with signature (x, y)
    auto args = makeTuple({
        {.name = std::nullopt, .value = Numeric {1, ""}},
        {.name = std::string("y"), .value = Numeric {2, ""}},
    });
    auto resolved = ArgumentParser {{"x"}, {"y"}}.resolve(args);

    EXPECT_DOUBLE_EQ(resolved.find("x")->get<Numeric>().value, 1.0);
    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 2.0);
}

TEST_F(ArgumentParserTest, NamedThenPositionalFillsRemainingSlot)
{
    // f(y: 2, 1) with signature (x, y) — positional 1 fills unclaimed x
    auto args = makeTuple({
        {.name = std::string("y"), .value = Numeric {2, ""}},
        {.name = std::nullopt, .value = Numeric {1, ""}},
    });
    auto resolved = ArgumentParser {{"x"}, {"y"}}.resolve(args);

    EXPECT_DOUBLE_EQ(resolved.find("x")->get<Numeric>().value, 1.0);
    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 2.0);
}

TEST_F(ArgumentParserTest, DefaultValueUsedWhenMissing)
{
    auto args = makeTuple({
        {.name = std::nullopt, .value = Numeric {1, ""}},
    });
    auto resolved = ArgumentParser {{"x"}, {"y", Numeric {99, ""}}}.resolve(args);

    EXPECT_DOUBLE_EQ(resolved.find("x")->get<Numeric>().value, 1.0);
    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 99.0);
}

TEST_F(ArgumentParserTest, DefaultValueOverriddenByPositional)
{
    auto args = makeTuple({
        {.name = std::nullopt, .value = Numeric {1, ""}},
        {.name = std::nullopt, .value = Numeric {2, ""}},
    });
    auto resolved = ArgumentParser {{"x"}, {"y", Numeric {99, ""}}}.resolve(args);

    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 2.0);
}

TEST_F(ArgumentParserTest, DefaultValueOverriddenByName)
{
    auto args = makeTuple({
        {.name = std::nullopt, .value = Numeric {1, ""}},
        {.name = std::string("y"), .value = Numeric {2, ""}},
    });
    auto resolved = ArgumentParser {{"x"}, {"y", Numeric {99, ""}}}.resolve(args);

    EXPECT_DOUBLE_EQ(resolved.find("y")->get<Numeric>().value, 2.0);
}

TEST_F(ArgumentParserTest, ResolvedTupleHasCorrectOrder)
{
    auto args = makeTuple({
        {.name = std::string("y"), .value = Numeric {2, ""}},
        {.name = std::nullopt, .value = Numeric {1, ""}},
    });
    auto resolved = ArgumentParser {{"x"}, {"y"}}.resolve(args);

    // at(0) is x, at(1) is y — matches declaration order
    EXPECT_DOUBLE_EQ(resolved.at(0).get<Numeric>().value, 1.0);
    EXPECT_DOUBLE_EQ(resolved.at(1).get<Numeric>().value, 2.0);
}

TEST_F(ArgumentParserTest, MixedTypes)
{
    auto args = makeTuple({
        {.name = std::nullopt, .value = Base::Color(1.0, 0.0, 0.0)},
        {.name = std::nullopt, .value = Numeric {20, ""}},
    });
    auto resolved = ArgumentParser {{"color"}, {"amount"}}.resolve(args);

    EXPECT_TRUE(resolved.find("color")->holds<Base::Color>());
    EXPECT_TRUE(resolved.find("amount")->holds<Numeric>());
}

TEST_F(ArgumentParserTest, ErrorOnUnknownName)
{
    auto args = makeTuple({
        {.name = std::string("unknown"), .value = Numeric {1, ""}},
    });
    ArgumentParser parser {{"x"}, {"y"}};
    EXPECT_THROW(parser.resolve(args), Base::ExpressionError);
}

TEST_F(ArgumentParserTest, ErrorOnDuplicateName)
{
    auto args = makeTuple({
        {.name = std::string("x"), .value = Numeric {1, ""}},
        {.name = std::string("x"), .value = Numeric {2, ""}},
    });
    ArgumentParser parser {{"x"}, {"y"}};
    EXPECT_THROW(parser.resolve(args), Base::ExpressionError);
}

TEST_F(ArgumentParserTest, ErrorOnMissingRequired)
{
    auto args = makeTuple({
        {.name = std::nullopt, .value = Numeric {1, ""}},
    });
    ArgumentParser parser {{"x"}, {"y"}};
    EXPECT_THROW(parser.resolve(args), Base::ExpressionError);
}

TEST_F(ArgumentParserTest, ErrorOnExcessPositional)
{
    auto args = makeTuple({
        {.name = std::nullopt, .value = Numeric {1, ""}},
        {.name = std::nullopt, .value = Numeric {2, ""}},
        {.name = std::nullopt, .value = Numeric {3, ""}},
    });
    ArgumentParser parser {{"x"}, {"y"}};
    EXPECT_THROW(parser.resolve(args), Base::ExpressionError);
}

TEST_F(ArgumentParserTest, TypedGetSuccess)
{
    auto args = makeTuple({
        {.name = std::nullopt, .value = Base::Color(1.0, 0.0, 0.0)},
        {.name = std::nullopt, .value = Numeric {20, ""}},
    });
    auto resolved = ArgumentParser {{"color"}, {"amount"}}.resolve(args);

    EXPECT_NO_THROW(resolved.get<Base::Color>("color"));
    EXPECT_NO_THROW(resolved.get<Numeric>("amount"));
    EXPECT_DOUBLE_EQ(resolved.get<Numeric>("amount").value, 20.0);
}

TEST_F(ArgumentParserTest, TypedGetWrongType)
{
    auto args = makeTuple({
        {.name = std::nullopt, .value = Numeric {10, "px"}},
        {.name = std::nullopt, .value = Numeric {20, ""}},
    });
    auto resolved = ArgumentParser {{"color"}, {"amount"}}.resolve(args);

    EXPECT_THROW(resolved.get<Base::Color>("color"), Base::ExpressionError);
}

TEST_F(ArgumentParserTest, TypedGetMissingName)
{
    auto args = makeTuple({{.name = std::nullopt, .value = Numeric {10, ""}}});
    auto resolved = ArgumentParser {{"x"}}.resolve(args);

    EXPECT_THROW(resolved.get<Numeric>("nonexistent"), Base::ExpressionError);
}
