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

#include <Gui/StyleParameters/ParameterManager.h>
#include <Gui/StyleParameters/Parser.h>
#include <Gui/StyleParameters/Value.h>

#include "DiagnosticsCapture.h"

using namespace Gui::StyleParameters;
using ::testing::Contains;
using ::testing::HasSubstr;
using ::testing::IsEmpty;

TEST(ValueAccessTest, StyleDefaultsAreEmptyValues)
{
    EXPECT_DOUBLE_EQ(styleDefault<Numeric>().value, 0.0);
    EXPECT_EQ(styleDefault<Numeric>().unit, "");
    EXPECT_FLOAT_EQ(styleDefault<Base::Color>().a, 0.0F);
    EXPECT_EQ(styleDefault<std::string>(), "");
    EXPECT_EQ(styleDefault<Tuple>().size(), 0U);
}

TEST(ValueAccessTest, ValueGetWrongTypeReturnsDefaultAndReports)
{
    DiagnosticsCapture capture;
    const Value value {Base::Color(1.0F, 0.0F, 0.0F)};

    EXPECT_DOUBLE_EQ(value.get<Numeric>().value, 0.0);
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("numeric")));
}

TEST(ValueAccessTest, ValueGetWithFallbackReturnsFallback)
{
    const Value value {Base::Color(1.0F, 0.0F, 0.0F)};

    EXPECT_DOUBLE_EQ(value.get<Numeric>(Numeric {.value = 8.0, .unit = "px"}).value, 8.0);
}

TEST(ValueAccessTest, ValueTryGetReturnsNullOnMismatchAndReportsNothing)
{
    DiagnosticsCapture capture;
    const Value value {Base::Color(1.0F, 0.0F, 0.0F)};

    EXPECT_EQ(value.tryGet<Numeric>(), nullptr);
    EXPECT_NE(value.tryGet<Base::Color>(), nullptr);
    EXPECT_THAT(capture.messages(), IsEmpty());
}

TEST(ValueAccessTest, TupleGetMissingNameReturnsDefaultAndReports)
{
    DiagnosticsCapture capture;
    const Tuple tuple({Tuple::Element::named("width", Numeric {.value = 4.0, .unit = "px"})});

    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("height").value, 0.0);
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("Missing argument 'height'")));
}

TEST(ValueAccessTest, TupleGetWrongTypeReturnsDefaultAndReports)
{
    DiagnosticsCapture capture;
    const Tuple tuple({Tuple::Element::named("width", Base::Color(1.0F, 0.0F, 0.0F))});

    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("width").value, 0.0);
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("'width' must be numeric")));
}

TEST(ValueAccessTest, TupleGetWithFallbackReturnsFallback)
{
    const Tuple tuple({Tuple::Element::named("width", Base::Color(1.0F, 0.0F, 0.0F))});

    EXPECT_DOUBLE_EQ(tuple.get<Numeric>("width", Numeric {.value = 8.0, .unit = "px"}).value, 8.0);
}

TEST(ValueAccessTest, TupleAtOutOfRangeReturnsDefaultAndReports)
{
    DiagnosticsCapture capture;
    const Tuple tuple({Tuple::Element::unnamed(Numeric {.value = 1.0, .unit = ""})});

    EXPECT_EQ(tuple.tryAt(1), nullptr);
    EXPECT_DOUBLE_EQ(tuple.at(1).get<Numeric>().value, 0.0);
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("out of range")));
}

// Regression: shades() read every element of its spec tuple with an unchecked get<Numeric>,
// throwing std::bad_variant_access past every catch site on the resolve path.
TEST(ValueAccessTest, ShadesWithNonNumericSpecIsContainedByResolve)
{
    DiagnosticsCapture capture;
    Gui::StyleParameters::ParameterManager manager;
    InMemoryParameterSource source(
        std::list<Parameter> {{.name = "Broken", .value = "shades(#ff0000, (050: #ffffff))"}},
        ParameterSource::Metadata {.name = "Test Source"}
    );
    manager.addSource(&source);

    EXPECT_NO_THROW({
        const auto resolved = manager.resolve("Broken");
        ASSERT_TRUE(resolved.has_value());
        EXPECT_TRUE(resolved->holds<Tuple>());
    });
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("Expected numeric value")));
}

// Regression: with exactly one gradient operand, blend() called Value::get<Base::Color>() on
// the other operand without checking it, throwing std::bad_variant_access past every catch
// site on the resolve path.
TEST(ValueAccessTest, BlendWithNonColorOperandIsContainedByResolve)
{
    Gui::StyleParameters::ParameterManager manager;
    InMemoryParameterSource source(
        std::list<Parameter> {
            {.name = "Broken", .value = "blend(linear_gradient(#ff0000, #0000ff), 10px, 50)"},
        },
        ParameterSource::Metadata {.name = "Test Source"}
    );
    manager.addSource(&source);

    EXPECT_NO_THROW({
        const auto resolved = manager.resolve("Broken");
        ASSERT_TRUE(resolved.has_value());
        EXPECT_TRUE(resolved->holds<std::string>());
    });
}
