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

#include "DiagnosticsCapture.h"

using namespace Gui::StyleParameters;
using ::testing::Contains;
using ::testing::HasSubstr;

TEST(CornersTest, FromColorDegradesToZeroAndReports)
{
    DiagnosticsCapture capture;

    const Corners corners {Value {Base::Color(1.0F, 0.0F, 0.0F)}};

    EXPECT_DOUBLE_EQ(corners.topLeft().value, 0.0);
    EXPECT_DOUBLE_EQ(corners.bottomRight().value, 0.0);
    EXPECT_EQ(corners.tuple().kind, TupleKind::Corners);
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("must be numeric")));
}

TEST(CornersTest, TryFromColorReturnsNullopt)
{
    DiagnosticsCapture capture;

    EXPECT_FALSE(Corners::tryFrom(Value {Base::Color(1.0F, 0.0F, 0.0F)}).has_value());
}

TEST(CornersTest, DiagonalPairingForTwoValues)
{
    DiagnosticsCapture capture;

    const Tuple args({
        Tuple::Element::unnamed(Numeric {.value = 4.0, .unit = "px"}),
        Tuple::Element::unnamed(Numeric {.value = 8.0, .unit = "px"}),
    });
    const Corners corners {args};

    EXPECT_DOUBLE_EQ(corners.topLeft().value, 4.0);
    EXPECT_DOUBLE_EQ(corners.topRight().value, 8.0);
    EXPECT_DOUBLE_EQ(corners.bottomRight().value, 4.0);
    EXPECT_DOUBLE_EQ(corners.bottomLeft().value, 8.0);
}

TEST(CornersTest, TooManyPositionalArgumentsDegradesToZero)
{
    DiagnosticsCapture capture;

    Tuple args;
    for (int index = 0; index < 5; ++index) {
        args.elements.push_back(
            Tuple::Element::unnamed(Numeric {.value = static_cast<double>(index), .unit = "px"})
        );
    }

    const Corners corners {args};

    EXPECT_DOUBLE_EQ(corners.topLeft().value, 0.0);
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("1-4 positional arguments")));
}

TEST(CornersTest, RejectsAPaddingTuple)
{
    DiagnosticsCapture capture;

    const Tuple padding(
        {
            Tuple::Element::named("top", Numeric {.value = 1.0, .unit = "px"}),
            Tuple::Element::named("right", Numeric {.value = 1.0, .unit = "px"}),
            Tuple::Element::named("bottom", Numeric {.value = 1.0, .unit = "px"}),
            Tuple::Element::named("left", Numeric {.value = 1.0, .unit = "px"}),
        },
        TupleKind::Padding
    );

    EXPECT_FALSE(Corners::tryFrom(Value {padding}).has_value());
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("Expected Corners tuple, got Padding")));
}

TEST(CornersTest, RawArgumentTupleFromColorDegradesToZeroAndReports)
{
    // Mirrors Parser.cpp's border_radius(...) function, which calls Corners(args) with the
    // raw Generic argument tuple (e.g. border_radius(#ff0000)). The raw-tuple constructor
    // must validate element types too, not just wrap whatever expand() produces — otherwise
    // a Color would be stored where a Numeric corner is expected.
    DiagnosticsCapture capture;

    const Tuple args({Tuple::Element::unnamed(Base::Color(1.0F, 0.0F, 0.0F))});
    const Corners corners {args};

    const Value* topLeftElement = corners.tuple().find("top_left");
    ASSERT_NE(topLeftElement, nullptr);
    EXPECT_TRUE(topLeftElement->holds<Numeric>());
    EXPECT_DOUBLE_EQ(corners.topLeft().value, 0.0);
    EXPECT_EQ(corners.tuple().kind, TupleKind::Corners);
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("must be numeric")));
}
