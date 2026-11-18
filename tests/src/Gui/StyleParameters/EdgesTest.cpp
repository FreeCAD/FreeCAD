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

#include <Gui/StyleParameters/Corners.h>
#include <Gui/StyleParameters/Insets.h>

#include "DiagnosticsCapture.h"

using namespace Gui::StyleParameters;
using ::testing::Contains;
using ::testing::HasSubstr;

namespace
{
Value colorValue()
{
    return Base::Color(1.0F, 0.0F, 0.0F);
}

Tuple colorEdges()
{
    return Tuple(
        {
            Tuple::Element::named("top", colorValue()),
            Tuple::Element::named("right", colorValue()),
            Tuple::Element::named("bottom", colorValue()),
            Tuple::Element::named("left", colorValue()),
        },
        TupleKind::BorderColors
    );
}
}  // namespace

TEST(EdgesTest, PaddingFromColorDegradesToZeroAndReports)
{
    DiagnosticsCapture capture;

    const Padding padding {colorValue()};

    EXPECT_DOUBLE_EQ(padding.top().value, 0.0);
    EXPECT_DOUBLE_EQ(padding.left().value, 0.0);
    EXPECT_EQ(padding.tuple().kind, TupleKind::Padding);
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("must be numeric")));
}

TEST(EdgesTest, PaddingTryFromColorReturnsNullopt)
{
    EXPECT_FALSE(Padding::tryFrom(colorValue()).has_value());
}

TEST(EdgesTest, PaddingFromBorderColorsDegradesToZero)
{
    const Padding padding {Value {colorEdges()}};

    EXPECT_DOUBLE_EQ(padding.top().value, 0.0);
    EXPECT_EQ(padding.tuple().kind, TupleKind::Padding);
    EXPECT_FALSE(Padding::tryFrom(Value {colorEdges()}).has_value());
}

TEST(EdgesTest, PaddingFromNumericExpandsToAllSides)
{
    const Padding padding {Value {Numeric {.value = 10.0, .unit = "px"}}};

    EXPECT_DOUBLE_EQ(padding.top().value, 10.0);
    EXPECT_DOUBLE_EQ(padding.right().value, 10.0);
    EXPECT_DOUBLE_EQ(padding.bottom().value, 10.0);
    EXPECT_DOUBLE_EQ(padding.left().value, 10.0);
    EXPECT_TRUE(Padding::tryFrom(Value {Numeric {.value = 10.0, .unit = "px"}}).has_value());
}

TEST(EdgesTest, BorderColorsFromColorExpandsToAllSides)
{
    const BorderColors colors {colorValue()};

    EXPECT_FLOAT_EQ(colors.top().r, 1.0F);
    EXPECT_FLOAT_EQ(colors.left().r, 1.0F);
    EXPECT_EQ(colors.tuple().kind, TupleKind::BorderColors);
    EXPECT_TRUE(BorderColors::tryFrom(colorValue()).has_value());
}

TEST(EdgesTest, TooManyPositionalArgumentsDegradesToZero)
{
    DiagnosticsCapture capture;

    const Tuple args({
        Tuple::Element::unnamed(Numeric {.value = 1.0, .unit = "px"}),
        Tuple::Element::unnamed(Numeric {.value = 2.0, .unit = "px"}),
        Tuple::Element::unnamed(Numeric {.value = 3.0, .unit = "px"}),
        Tuple::Element::unnamed(Numeric {.value = 4.0, .unit = "px"}),
        Tuple::Element::unnamed(Numeric {.value = 5.0, .unit = "px"}),
    });

    const Padding padding {args};

    EXPECT_DOUBLE_EQ(padding.top().value, 0.0);
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("1-4 positional arguments")));
}

TEST(EdgesTest, PaddingAcceptsAnExistingPaddingTuple)
{
    const Padding original {Value {Numeric {.value = 7.0, .unit = "px"}}};
    const Padding copy {Value {original.tuple()}};

    EXPECT_DOUBLE_EQ(copy.top().value, 7.0);
    EXPECT_EQ(copy.tuple().kind, TupleKind::Padding);
}

TEST(EdgesTest, InsetsAcceptAnyEdgeKind)
{
    const Padding padding {Value {Numeric {.value = 3.0, .unit = "px"}}};
    const auto insets = Insets::tryFrom(Value {padding.tuple()});

    ASSERT_TRUE(insets.has_value());
    EXPECT_DOUBLE_EQ(insets->horizontal().value, 6.0);
}

TEST(EdgesTest, PaddingOfNestedBorderColorsArgumentDegradesToZero)
{
    // Regression test for padding(border_colors(#ff0000)): the raw-argument-tuple
    // constructor (the one Parser.cpp's padding()/margins()/etc. functions use) must
    // validate element types too, not just the Value constructor. Checking the tuple's
    // stored element (not just the top() accessor, which degrades on its own regardless
    // of what was stored) is what actually proves the tuple itself holds numerics.
    DiagnosticsCapture capture;

    const Tuple args({Tuple::Element::unnamed(colorEdges())});
    const Padding padding {args};

    const Value* topElement = padding.tuple().find("top");
    ASSERT_NE(topElement, nullptr);
    EXPECT_TRUE(topElement->holds<Numeric>());
    EXPECT_DOUBLE_EQ(padding.top().value, 0.0);
    EXPECT_EQ(padding.tuple().kind, TupleKind::Padding);
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("must be numeric")));
}

TEST(EdgesTest, PaddingTryFromCornersReturnsNulloptWithMessage)
{
    // Corners has no top/right/bottom/left elements at all, so expand() would silently
    // backfill all four sides with zero — a differently-shaped tuple must be rejected
    // before that happens, not waved through as zero padding with no diagnostic.
    DiagnosticsCapture capture;

    const Corners corners {Value {Numeric {.value = 4.0, .unit = "px"}}};

    EXPECT_FALSE(Padding::tryFrom(Value {corners.tuple()}).has_value());
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("got Corners")));
}

TEST(EdgesTest, InsetsTryFromLinearGradientReturnsNullopt)
{
    const Tuple gradient(
        {
            Tuple::Element::named("x1", Numeric {.value = 0.0, .unit = ""}),
            Tuple::Element::named("y1", Numeric {.value = 0.0, .unit = ""}),
            Tuple::Element::named("x2", Numeric {.value = 0.0, .unit = ""}),
            Tuple::Element::named("y2", Numeric {.value = 1.0, .unit = ""}),
        },
        TupleKind::LinearGradient
    );

    EXPECT_FALSE(Insets::tryFrom(Value {gradient}).has_value());
}

TEST(EdgesTest, PaddingFromCornersDegradesToZeroAndReports)
{
    DiagnosticsCapture capture;

    const Corners corners {Value {Numeric {.value = 4.0, .unit = "px"}}};
    const Padding padding {Value {corners.tuple()}};

    EXPECT_DOUBLE_EQ(padding.top().value, 0.0);
    EXPECT_EQ(padding.tuple().kind, TupleKind::Padding);
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("got Corners")));
}

TEST(EdgesTest, PaddingFromMarginsTupleCoerces)
{
    // The four edge kinds share element type Numeric and are structurally interchangeable, so
    // the kind gate must coerce between them rather than demand an exact match.
    DiagnosticsCapture capture;

    const Margins margins {Value {Numeric {.value = 8.0, .unit = "px"}}};
    const auto padding = Padding::tryFrom(Value {margins.tuple()});

    ASSERT_TRUE(padding.has_value());
    EXPECT_DOUBLE_EQ(padding->top().value, 8.0);
    EXPECT_EQ(padding->tuple().kind, TupleKind::Padding);
}
