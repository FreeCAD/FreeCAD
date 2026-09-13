// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Gui/Navigation/NavigationStyle.h>

using Gui::NavigationStyle;
using WheelAction = Gui::NavigationStyle::WheelAction;

namespace
{
constexpr bool Touchpad = true;
constexpr bool WheelMouse = false;
constexpr bool ScrollPans = true;
constexpr bool ScrollZooms = false;
constexpr bool Shift = true;
constexpr bool NoShift = false;
constexpr bool Ctrl = true;
constexpr bool NoCtrl = false;
}  // namespace

TEST(NavigationStyleWheelAction, anOrdinaryWheelZoomsWhateverThePreferenceSays)
{
    EXPECT_EQ(NavigationStyle::wheelAction(WheelMouse, ScrollPans, NoShift, NoCtrl), WheelAction::Zoom);
    EXPECT_EQ(NavigationStyle::wheelAction(WheelMouse, ScrollZooms, NoShift, NoCtrl), WheelAction::Zoom);
}

TEST(NavigationStyleWheelAction, aTouchpadZoomsWhenScrollPanningIsOff)
{
    EXPECT_EQ(NavigationStyle::wheelAction(Touchpad, ScrollZooms, NoShift, NoCtrl), WheelAction::Zoom);
}

TEST(NavigationStyleWheelAction, aTouchpadPansWhenScrollPanningIsOn)
{
    EXPECT_EQ(NavigationStyle::wheelAction(Touchpad, ScrollPans, NoShift, NoCtrl), WheelAction::Pan);
}

TEST(NavigationStyleWheelAction, shiftOrbitsInsteadOfPanning)
{
    EXPECT_EQ(NavigationStyle::wheelAction(Touchpad, ScrollPans, Shift, NoCtrl), WheelAction::Orbit);
}

TEST(NavigationStyleWheelAction, ctrlZoomsEvenWhileScrollPanningIsOn)
{
    EXPECT_EQ(NavigationStyle::wheelAction(Touchpad, ScrollPans, NoShift, Ctrl), WheelAction::Zoom);
}

TEST(NavigationStyleWheelAction, ctrlWinsOverShift)
{
    EXPECT_EQ(NavigationStyle::wheelAction(Touchpad, ScrollPans, Shift, Ctrl), WheelAction::Zoom);
}

TEST(NavigationStyleWheelAction, modifiersNeverDivertAnOrdinaryWheelFromZooming)
{
    EXPECT_EQ(NavigationStyle::wheelAction(WheelMouse, ScrollPans, Shift, NoCtrl), WheelAction::Zoom);
    EXPECT_EQ(NavigationStyle::wheelAction(WheelMouse, ScrollPans, NoShift, Ctrl), WheelAction::Zoom);
}
