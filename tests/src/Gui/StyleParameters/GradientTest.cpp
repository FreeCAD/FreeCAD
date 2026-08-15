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

#include <Gui/StyleParameters/Gradient.h>

#include "DiagnosticsCapture.h"

using namespace Gui::StyleParameters;
using ::testing::Contains;
using ::testing::HasSubstr;

namespace
{
Tuple twoStopGradient()
{
    return LinearGradient(Tuple({
                              Tuple::Element::unnamed(Base::Color(1.0F, 0.0F, 0.0F)),
                              Tuple::Element::unnamed(Base::Color(0.0F, 0.0F, 1.0F)),
                          }))
        .tuple();
}
}  // namespace

TEST(GradientTest, TooFewStopsDegradesToTransparentAndReports)
{
    DiagnosticsCapture capture;

    const LinearGradient gradient {
        Value {Tuple({Tuple::Element::unnamed(Base::Color(1.0F, 0.0F, 0.0F))})}
    };

    const auto stops = gradient.colorStops();
    ASSERT_EQ(stops.size(), 2U);
    EXPECT_FLOAT_EQ(stops[0].color.a, 0.0F);
    EXPECT_FLOAT_EQ(stops[1].color.a, 0.0F);
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("at least 2 color stops")));
}

TEST(GradientTest, TryFromTooFewStopsReturnsNullopt)
{
    DiagnosticsCapture capture;

    EXPECT_FALSE(
        LinearGradient::tryFrom(Value {Tuple({Tuple::Element::unnamed(Base::Color(1.0F, 0.0F, 0.0F))})})
            .has_value()
    );
}

TEST(GradientTest, KindMismatchDegradesAndReports)
{
    DiagnosticsCapture capture;

    const Tuple radial = RadialGradient(Tuple({
                                            Tuple::Element::unnamed(Base::Color(1.0F, 0.0F, 0.0F)),
                                            Tuple::Element::unnamed(Base::Color(0.0F, 0.0F, 1.0F)),
                                        }))
                             .tuple();

    EXPECT_FALSE(LinearGradient::tryFrom(Value {radial}).has_value());

    const LinearGradient degraded {Value {radial}};
    EXPECT_EQ(degraded.tuple().kind, TupleKind::LinearGradient);
    EXPECT_FLOAT_EQ(degraded.colorStops()[0].color.a, 0.0F);
    EXPECT_THAT(
        capture.messages(),
        Contains(HasSubstr("Expected LinearGradient tuple, got RadialGradient"))
    );
}

TEST(GradientTest, NonNumericGeometryParameterDegradesAndReports)
{
    DiagnosticsCapture capture;

    // Mirrors linear_gradient(x1: #ffffff, #000000, #ffffff): a color supplied where the
    // numeric x1 geometry parameter is expected, alongside two otherwise-valid stops.
    const Tuple args({
        Tuple::Element::named("x1", Base::Color(1.0F, 1.0F, 1.0F)),
        Tuple::Element::unnamed(Base::Color(0.0F, 0.0F, 0.0F)),
        Tuple::Element::unnamed(Base::Color(1.0F, 1.0F, 1.0F)),
    });

    std::optional<LinearGradient> gradient;
    EXPECT_NO_THROW({ gradient.emplace(Tuple {args}); });
    ASSERT_TRUE(gradient.has_value());

    EXPECT_DOUBLE_EQ(gradient->x1(), 0.0);
    EXPECT_EQ(gradient->tuple().kind, TupleKind::LinearGradient);

    const auto stops = gradient->colorStops();
    ASSERT_EQ(stops.size(), 2U);

    EXPECT_THAT(capture.messages(), Contains(HasSubstr("'x1' must be a number")));
}

TEST(GradientTest, WellFormedGradientRoundTrips)
{
    DiagnosticsCapture capture;

    const auto gradient = LinearGradient::tryFrom(Value {twoStopGradient()});

    ASSERT_TRUE(gradient.has_value());
    EXPECT_DOUBLE_EQ(gradient->x1(), 0.0);
    EXPECT_DOUBLE_EQ(gradient->y2(), 1.0);

    const auto stops = gradient->colorStops();
    ASSERT_EQ(stops.size(), 2U);
    EXPECT_FLOAT_EQ(stops[0].color.r, 1.0F);
    EXPECT_FLOAT_EQ(stops[1].color.b, 1.0F);
}

TEST(GradientTest, MalformedLinearGradientTupleDegradesAndReports)
{
    DiagnosticsCapture capture;

    // Already tagged LinearGradient, but with a "stops" element that fails isWellFormed's
    // check (a single well-formed stop, one short of the required 2) rather than going
    // through the Generic-args expansion path.
    const Tuple malformed(
        {
            Tuple::Element::named("x1", Numeric {.value = 0.0, .unit = ""}),
            Tuple::Element::named("y1", Numeric {.value = 0.0, .unit = ""}),
            Tuple::Element::named("x2", Numeric {.value = 0.0, .unit = ""}),
            Tuple::Element::named("y2", Numeric {.value = 1.0, .unit = ""}),
            Tuple::Element::named(
                "stops",
                Tuple({Tuple::Element::unnamed(Tuple({
                    Tuple::Element::unnamed(Numeric {.value = 0.0, .unit = ""}),
                    Tuple::Element::unnamed(Base::Color(1.0F, 0.0F, 0.0F)),
                }))})
            ),
        },
        TupleKind::LinearGradient
    );

    EXPECT_FALSE(LinearGradient::tryFrom(Value {malformed}).has_value());

    const LinearGradient degraded {Value {malformed}};
    ASSERT_EQ(degraded.colorStops().size(), 2U);
    EXPECT_FLOAT_EQ(degraded.colorStops()[0].color.a, 0.0F);
    EXPECT_THAT(capture.messages(), Contains(HasSubstr("Malformed LinearGradient tuple")));
}

// MalformedStopIsSkippedNotFatal and MapStopColorsSkipsMalformedStops used to live here. Both
// existed solely to construct a LinearGradient wrapper holding a malformed stop (one bypassing
// isWellFormed via the raw Tuple constructor, the other feeding a hand-built malformed tuple
// straight into the static Gradient::mapStopColors) and then exercise the skip branches that
// tolerated it. Once TypedGradient(Tuple) validates and mapStopColors became a member that only
// ever runs on an already-validated tuple_, that state is no longer representable -- there is no
// way to hold a LinearGradient/RadialGradient with a malformed stop, so the skip branches (and
// these tests) were deleted rather than kept as dead code.

TEST(GradientTest, MalformedKindTaggedTupleNeverProducesIllFormedWrapperThroughAnyRoute)
{
    // Already tagged LinearGradient, but with a "stops" element that fails isWellFormed's check
    // (a single well-formed stop, one short of the required 2). Before TypedGradient(Tuple)
    // validated, this exact shape passed straight through the raw-Tuple constructor unvalidated
    // because tuple.kind already matched Kind.
    const Tuple malformed(
        {
            Tuple::Element::named("x1", Numeric {.value = 0.0, .unit = ""}),
            Tuple::Element::named("y1", Numeric {.value = 0.0, .unit = ""}),
            Tuple::Element::named("x2", Numeric {.value = 0.0, .unit = ""}),
            Tuple::Element::named("y2", Numeric {.value = 1.0, .unit = ""}),
            Tuple::Element::named(
                "stops",
                Tuple({Tuple::Element::unnamed(Tuple({
                    Tuple::Element::unnamed(Numeric {.value = 0.0, .unit = ""}),
                    Tuple::Element::unnamed(Base::Color(1.0F, 0.0F, 0.0F)),
                }))})
            ),
        },
        TupleKind::LinearGradient
    );

    // Route 1: tryFrom must reject it outright.
    {
        DiagnosticsCapture capture;
        EXPECT_FALSE(LinearGradient::tryFrom(Value {malformed}).has_value());
    }

    // Route 2: the Value constructor is total, so it must degrade to a fully transparent,
    // well-formed gradient rather than keep the malformed stops.
    {
        DiagnosticsCapture capture;
        const LinearGradient viaValue {Value {malformed}};
        const auto stops = viaValue.colorStops();
        ASSERT_EQ(stops.size(), 2U);
        EXPECT_FLOAT_EQ(stops[0].color.a, 0.0F);
        EXPECT_FLOAT_EQ(stops[1].color.a, 0.0F);
    }

    // Route 3: the raw-Tuple constructor. This is the route that used to skip validation
    // whenever the tuple's kind already matched -- it must degrade exactly like the Value
    // constructor, never hand back the malformed tuple as-is.
    {
        DiagnosticsCapture capture;
        const LinearGradient viaTuple {Tuple {malformed}};
        const auto stops = viaTuple.colorStops();
        ASSERT_EQ(stops.size(), 2U);
        EXPECT_FLOAT_EQ(stops[0].color.a, 0.0F);
        EXPECT_FLOAT_EQ(stops[1].color.a, 0.0F);
        EXPECT_THAT(capture.messages(), Contains(HasSubstr("Malformed LinearGradient tuple")));
    }
}
