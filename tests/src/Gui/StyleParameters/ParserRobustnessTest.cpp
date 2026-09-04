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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

#include <Gui/StyleParameters/ParameterManager.h>
#include <Gui/StyleParameters/Parser.h>

using namespace Gui::StyleParameters;

class ParserRobustnessTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        source = std::make_unique<InMemoryParameterSource>(
            std::list<Parameter> {{.name = "Tuple3", .value = "(10, 20, 30)"}},
            ParameterSource::Metadata {.name = "Robustness Source"}
        );
        manager.addSource(source.get());
    }

    Gui::StyleParameters::ParameterManager manager;
    std::unique_ptr<InMemoryParameterSource> source;
};

// Regression: std::stoul threw std::out_of_range past every catch site on the resolve path.
TEST_F(ParserRobustnessTest, HugeMemberIndexIsAnExpressionError)
{
    Parser parser("@Tuple3.99999999999999999999");
    auto expr = parser.parse();

    EXPECT_THROW(expr->evaluate({.manager = &manager, .context = {}}), Base::ExpressionError);
}

// Regression: std::stoi in rgb() threw std::out_of_range; only invalid_argument was caught.
TEST_F(ParserRobustnessTest, HugeColorComponentIsAParserError)
{
    EXPECT_THROW(
        {
            Parser parser("rgb(99999999999,0,0)");
            parser.parse();
        },
        Base::ParserError
    );
}

// Regression: std::stod threw std::out_of_range for an over-long literal.
TEST_F(ParserRobustnessTest, OverlongNumericLiteralIsAParserError)
{
    const std::string literal(400, '9');

    EXPECT_THROW(
        {
            Parser parser(literal);
            parser.parse();
        },
        Base::ParserError
    );
}

TEST_F(ParserRobustnessTest, NonAsciiInputDoesNotCrash)
{
    // Bytes >= 0x80 are negative as char; passing them to isdigit and friends is UB.
    EXPECT_NO_THROW({
        try {
            Parser parser("10px + \xc3\xa9");
            parser.parse();
        }
        catch (const Base::Exception&) {
            // A parse error is fine; undefined behaviour is not.
        }
    });
}

TEST_F(ParserRobustnessTest, EveryFailureThroughResolveIsContained)
{
    InMemoryParameterSource broken(
        std::list<Parameter> {
            {.name = "A", .value = "@Tuple3.99999999999999999999"},
            {.name = "B", .value = "rgb(99999999999,0,0)"},
            {.name = "C", .value = std::string(400, '9')},
            {.name = "D", .value = "#fff"},
        },
        ParameterSource::Metadata {.name = "Broken Source"}
    );
    manager.addSource(&broken);

    for (const char* name : {"A", "B", "C", "D"}) {
        EXPECT_NO_THROW({
            const auto resolved = manager.resolve(name);
            EXPECT_TRUE(resolved.has_value());
        }) << "resolving "
           << name;
    }
}

// Regression: substr(pos, 2) silently clips once fewer than 2 characters remain, so the third
// component's substr(pos, 2) call can start past the end of the string. #fff is the shortest
// input that overshoots: after two clipped reads pos lands one past input.size(), and
// substr throws std::out_of_range because its precondition is pos <= size(), not pos < size().
TEST_F(ParserRobustnessTest, ShortHexColorLiteralIsAParserError)
{
    EXPECT_THROW(
        {
            Parser parser("#fff");
            parser.parse();
        },
        Base::ParserError
    );
}

TEST_F(ParserRobustnessTest, TwoDigitHexColorLiteralIsAParserError)
{
    EXPECT_THROW(
        {
            Parser parser("#ff");
            parser.parse();
        },
        Base::ParserError
    );
}

TEST_F(ParserRobustnessTest, BareHashIsAParserError)
{
    EXPECT_THROW(
        {
            Parser parser("#");
            parser.parse();
        },
        Base::ParserError
    );
}

// The length guard must not reject well-formed input.
TEST_F(ParserRobustnessTest, FullLengthHexColorLiteralStillParses)
{
    Parser parser("#ff8000");
    auto expr = parser.parse();
    auto result = expr->evaluate({.manager = &manager, .context = {}});

    ASSERT_TRUE(result.holds<Base::Color>());
    const auto& color = result.get<Base::Color>();
    EXPECT_DOUBLE_EQ(color.r, 1.0);
    EXPECT_NEAR(color.g, 128 / 255.0, 1e-6);
    EXPECT_DOUBLE_EQ(color.b, 0.0);
}
