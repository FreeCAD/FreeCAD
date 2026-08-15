// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <cmath>

#include <Gui/Navigation/NavigationStyle.h>
#include <Gui/SoTouchEvents.h>

namespace
{
constexpr bool TouchTiltDisabled = true;
constexpr bool TouchTiltAllowed = false;

SoGesturePinchEvent updateEvent(double deltaZoom, double deltaAngle, bool native = false)
{
    SoGesturePinchEvent event;
    event.state = SoGestureEvent::SbGSUpdate;
    event.deltaZoom = deltaZoom;
    event.deltaAngle = deltaAngle;
    event.fromNativeGesture = native;
    return event;
}
}  // namespace

TEST(NavigationStylePinchAction, onlyUpdateEventsMoveTheCamera)
{
    for (const auto state : {SoGestureEvent::SbGSStart, SoGestureEvent::SbGSEnd}) {
        SCOPED_TRACE(static_cast<int>(state));
        SoGesturePinchEvent event;
        event.state = state;
        event.deltaZoom = 2.0;
        event.deltaAngle = 1.0;

        const auto action = Gui::NavigationStyle::pinchAction(&event, TouchTiltAllowed);

        EXPECT_FALSE(action.zoom);
        EXPECT_FALSE(action.rotate);
    }
}

TEST(NavigationStylePinchAction, aRotateOnlyEventDoesNotZoom)
{
    const auto event = updateEvent(0.0, 0.5);

    const auto action = Gui::NavigationStyle::pinchAction(&event, TouchTiltAllowed);

    EXPECT_FALSE(action.zoom);
    EXPECT_TRUE(action.rotate);
}

TEST(NavigationStylePinchAction, aZoomOnlyEventDoesNotRotate)
{
    const auto event = updateEvent(1.5, 0.0);

    const auto action = Gui::NavigationStyle::pinchAction(&event, TouchTiltAllowed);

    EXPECT_TRUE(action.zoom);
    EXPECT_FALSE(action.rotate);
}

TEST(NavigationStylePinchAction, aNegativeZoomFactorIsIgnored)
{
    const auto event = updateEvent(-0.5, 0.0);

    const auto action = Gui::NavigationStyle::pinchAction(&event, TouchTiltAllowed);

    EXPECT_FALSE(action.zoom);
}

TEST(NavigationStylePinchAction, pinchingOutZoomsIn)
{
    const auto event = updateEvent(2.0, 0.0);

    const auto action = Gui::NavigationStyle::pinchAction(&event, TouchTiltAllowed);

    ASSERT_TRUE(action.zoom);
    EXPECT_FLOAT_EQ(action.zoomLogFactor, -std::log(2.0F));
}

TEST(NavigationStylePinchAction, pinchingInZoomsOutBySymmetricAmount)
{
    const auto out = updateEvent(2.0, 0.0);
    const auto in = updateEvent(0.5, 0.0);

    const auto zoomIn = Gui::NavigationStyle::pinchAction(&out, TouchTiltAllowed);
    const auto zoomOut = Gui::NavigationStyle::pinchAction(&in, TouchTiltAllowed);

    EXPECT_FLOAT_EQ(zoomOut.zoomLogFactor, -zoomIn.zoomLogFactor);
}

TEST(NavigationStylePinchAction, aTouchscreenPinchObeysTheTiltPreference)
{
    const auto event = updateEvent(2.0, 0.5);

    const auto action = Gui::NavigationStyle::pinchAction(&event, TouchTiltDisabled);

    EXPECT_TRUE(action.zoom);
    EXPECT_FALSE(action.rotate);
}

TEST(NavigationStylePinchAction, aTrackpadRotateIsNotBlockedByTheTouchscreenTiltPreference)
{
    const auto event = updateEvent(2.0, 0.5, true);

    const auto action = Gui::NavigationStyle::pinchAction(&event, TouchTiltDisabled);

    EXPECT_TRUE(action.zoom);
    ASSERT_TRUE(action.rotate);
    EXPECT_FLOAT_EQ(action.rotateAngle, 0.5F);
}
