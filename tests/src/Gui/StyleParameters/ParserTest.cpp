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

#include <QColor>

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
            },
            ParameterSource::Metadata {"Test Source"});

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
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 42.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10.5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 10.5);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("2.5em");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 2.5);
        EXPECT_EQ(length.unit, "em");
    }

    {
        Parser parser("100%");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
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
        EXPECT_TRUE(std::holds_alternative<QColor>(result));
        auto color = std::get<QColor>(result);
        EXPECT_EQ(color.red(), 255);
        EXPECT_EQ(color.green(), 0);
        EXPECT_EQ(color.blue(), 0);
    }

    {
        Parser parser("#00ff00");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<QColor>(result));
        auto color = std::get<QColor>(result);
        EXPECT_EQ(color.red(), 0);
        EXPECT_EQ(color.green(), 255);
        EXPECT_EQ(color.blue(), 0);
    }

    {
        Parser parser("#0000ff");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<QColor>(result));
        auto color = std::get<QColor>(result);
        EXPECT_EQ(color.red(), 0);
        EXPECT_EQ(color.green(), 0);
        EXPECT_EQ(color.blue(), 255);
    }

    {
        Parser parser("rgb(255, 0, 0)");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<QColor>(result));
        auto color = std::get<QColor>(result);
        EXPECT_EQ(color.red(), 255);
        EXPECT_EQ(color.green(), 0);
        EXPECT_EQ(color.blue(), 0);
    }

    {
        Parser parser("rgba(255, 0, 0, 128)");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<QColor>(result));
        auto color = std::get<QColor>(result);
        EXPECT_EQ(color.red(), 255);
        EXPECT_EQ(color.green(), 0);
        EXPECT_EQ(color.blue(), 0);
        EXPECT_EQ(color.alpha(), 128);
    }
}

// Test parameter reference parsing
TEST_F(ParserTest, ParseParameterReferences)
{
    {
        Parser parser("@TestParam");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 10.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("@TestColor");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<QColor>(result));
        auto color = std::get<QColor>(result);
        EXPECT_EQ(color.red(), 255);
        EXPECT_EQ(color.green(), 0);
        EXPECT_EQ(color.blue(), 0);
    }

    {
        Parser parser("@TestNumber");
        auto expr = parser.parse();
        auto result = expr->evaluate({.manager = &manager, .context = {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
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
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px + 5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("10 - 5");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 5.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px - 5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 5.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("10 * 5");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 50.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px * 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 20.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("10 / 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 5.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px / 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
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
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 30.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("(10px + 5px) * 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 30.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("@TestParam + 5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("@TestParam * @TestNumber");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
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
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 10.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("-10");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, -10.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("-10px");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
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
        EXPECT_TRUE(std::holds_alternative<QColor>(result));
        auto color = std::get<QColor>(result);
        // The result should be lighter than the original red
        EXPECT_GT(color.lightness(), QColor("#ff0000").lightness());
    }

    {
        Parser parser("darken(#ff0000, 20)");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<QColor>(result));
        auto color = std::get<QColor>(result);
        // The result should be darker than the original red
        EXPECT_LT(color.lightness(), QColor("#ff0000").lightness());
    }

    {
        Parser parser("lighten(@TestColor, 20)");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<QColor>(result));
        auto color = std::get<QColor>(result);
        // The result should be lighter than the original red
        EXPECT_GT(color.lightness(), QColor("#ff0000").lightness());
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
        Base::ParserError);

    // Invalid RGB format
    EXPECT_THROW(
        {
            Parser parser("rgb(invalid)");
            parser.parse();
        },
        Base::ParserError);

    // Missing closing parenthesis
    EXPECT_THROW(
        {
            Parser parser("(10 + 5");
            parser.parse();
        },
        Base::ParserError);

    // Invalid function
    EXPECT_THROW(
        {
            Parser parser("invalid()");
            auto expr = parser.parse();
            expr->evaluate({&manager, {}});
        },
        Base::ExpressionError);

    // Division by zero
    EXPECT_THROW(
        {
            Parser parser("10 / 0");
            auto expr = parser.parse();
            expr->evaluate({&manager, {}});
        },
        Base::RuntimeError);

    // Unit mismatch
    EXPECT_THROW(
        {
            Parser parser("10px + 5em");
            auto expr = parser.parse();
            expr->evaluate({&manager, {}});
        },
        Base::RuntimeError);

    // Unary operation on color
    EXPECT_THROW(
        {
            Parser parser("-@TestColor");
            auto expr = parser.parse();
            expr->evaluate({&manager, {}});
        },
        Base::ExpressionError);

    // Function with wrong number of arguments
    EXPECT_THROW(
        {
            Parser parser("lighten(#ff0000)");
            auto expr = parser.parse();
            expr->evaluate({&manager, {}});
        },
        Base::ExpressionError);

    // Function with wrong argument type
    EXPECT_THROW(
        {
            Parser parser("lighten(10px, 20)");
            auto expr = parser.parse();
            expr->evaluate({&manager, {}});
        },
        Base::ExpressionError);
}

// Test whitespace handling
TEST_F(ParserTest, ParseWhitespace)
{
    {
        Parser parser("  10  +  5  ");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10px+5px");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 15.0);
        EXPECT_EQ(length.unit, "px");
    }

    {
        Parser parser("rgb(255,0,0)");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<QColor>(result));
        auto color = std::get<QColor>(result);
        EXPECT_EQ(color.red(), 255);
        EXPECT_EQ(color.green(), 0);
        EXPECT_EQ(color.blue(), 0);
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
        Base::ParserError);

    // Just whitespace
    EXPECT_THROW(
        {
            Parser parser("   ");
            parser.parse();
        },
        Base::ParserError);

    // Single number
    {
        Parser parser("42");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 42.0);
        EXPECT_EQ(length.unit, "");
    }

    // Single color
    {
        Parser parser("#ff0000");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<QColor>(result));
        auto color = std::get<QColor>(result);
        EXPECT_EQ(color.red(), 255);
        EXPECT_EQ(color.green(), 0);
        EXPECT_EQ(color.blue(), 0);
    }

    // Single parameter reference
    {
        Parser parser("@TestParam");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
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
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 14.0);  // 2 + (3 * 4) = 2 + 12 = 14
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("10 - 3 * 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 4.0);  // 10 - (3 * 2) = 10 - 6 = 4
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("20 / 4 + 3");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 8.0);  // (20 / 4) + 3 = 5 + 3 = 8
        EXPECT_EQ(length.unit, "");
    }
}

// Test nested parentheses
TEST_F(ParserTest, ParseNestedParentheses)
{
    {
        Parser parser("((2 + 3) * 4)");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 20.0);  // (5) * 4 = 20
        EXPECT_EQ(length.unit, "");
    }

    {
        Parser parser("(10 - (3 + 2)) * 2");
        auto expr = parser.parse();
        auto result = expr->evaluate({&manager, {}});
        EXPECT_TRUE(std::holds_alternative<Length>(result));
        auto length = std::get<Length>(result);
        EXPECT_DOUBLE_EQ(length.value, 10.0);  // (10 - 5) * 2 = 5 * 2 = 10
        EXPECT_EQ(length.unit, "");
    }
}
