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

#include <stdexcept>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <Gui/StyleParameters/Diagnostics.h>

#include "DiagnosticsCapture.h"

using namespace Gui::StyleParameters;
using ::testing::ElementsAre;

TEST(DiagnosticsTest, DeliversFormattedMessageToObserver)
{
    DiagnosticsCapture capture;

    Diagnostics::report("Argument '{}' must be {}", "width", "numeric");

    EXPECT_THAT(capture.messages(), ElementsAre("Argument 'width' must be numeric"));
}

TEST(DiagnosticsTest, DropsExactRepeats)
{
    DiagnosticsCapture capture;

    Diagnostics::report("same message");
    Diagnostics::report("same message");
    Diagnostics::report("different message");

    EXPECT_THAT(capture.messages(), ElementsAre("same message", "different message"));
}

TEST(DiagnosticsTest, ClearAllowsMessageAgain)
{
    DiagnosticsCapture capture;

    Diagnostics::report("repeatable");
    Diagnostics::clear();
    Diagnostics::report("repeatable");

    EXPECT_THAT(capture.messages(), ElementsAre("repeatable", "repeatable"));
}

TEST(DiagnosticsTest, ObserverStopsReceivingAfterSubscriptionEnds)
{
    std::vector<std::string> received;

    {
        auto subscription = Diagnostics::observe([&received](const std::string& message) {
            received.push_back(message);
        });
        Diagnostics::report("while subscribed");
    }

    Diagnostics::clear();
    Diagnostics::report("after unsubscribed");

    EXPECT_THAT(received, ElementsAre("while subscribed"));
}

TEST(DiagnosticsTest, SupportsMultipleSimultaneousObservers)
{
    DiagnosticsCapture first;
    DiagnosticsCapture second;

    Diagnostics::report("broadcast");

    EXPECT_THAT(first.messages(), ElementsAre("broadcast"));
    EXPECT_THAT(second.messages(), ElementsAre("broadcast"));
}

TEST(DiagnosticsTest, ThrowingObserverDoesNotPreventOthersFromReceiving)
{
    Diagnostics::clear();

    // Registered first, so it runs before `capture` below and would otherwise poison the loop.
    auto throwing = Diagnostics::observe([](const std::string&) {
        throw std::runtime_error("misbehaving observer");
    });

    DiagnosticsCapture capture;

    EXPECT_NO_THROW(Diagnostics::report("boom"));
    EXPECT_THAT(capture.messages(), ElementsAre("boom"));
}

TEST(DiagnosticsTest, NoActiveResolutionScopeMeansNoPrefix)
{
    DiagnosticsCapture capture;

    Diagnostics::report("bad value");

    EXPECT_THAT(capture.messages(), ElementsAre("bad value"));
}

TEST(DiagnosticsTest, ResolutionScopePrefixesTheTokenName)
{
    DiagnosticsCapture capture;

    {
        Diagnostics::ResolutionScope scope("MyToken");
        Diagnostics::report("bad value");
    }

    EXPECT_THAT(capture.messages(), ElementsAre("MyToken: bad value"));
}

TEST(DiagnosticsTest, SameDefectUnderTwoScopesProducesTwoDistinctMessages)
{
    DiagnosticsCapture capture;

    {
        Diagnostics::ResolutionScope scope("TokenA");
        Diagnostics::report("bad value");
    }
    {
        Diagnostics::ResolutionScope scope("TokenB");
        Diagnostics::report("bad value");
    }

    EXPECT_THAT(capture.messages(), ElementsAre("TokenA: bad value", "TokenB: bad value"));
}

TEST(DiagnosticsTest, NestedResolutionScopeReportsTheInnermostToken)
{
    DiagnosticsCapture capture;

    {
        Diagnostics::ResolutionScope outer("Outer");
        {
            Diagnostics::ResolutionScope inner("Inner");
            Diagnostics::report("bad value");
        }
        Diagnostics::report("bad value, inner ended");
    }

    EXPECT_THAT(capture.messages(), ElementsAre("Inner: bad value", "Outer: bad value, inner ended"));
}
