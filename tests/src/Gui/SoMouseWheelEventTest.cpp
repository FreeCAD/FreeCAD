// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Gui/Inventor/SoMouseWheelEvent.h>

TEST(SoMouseWheelEvent, toGlPixelDeltaScalesByDevicePixelRatioAndFlipsY)
{
    EXPECT_EQ(SoMouseWheelEvent::toGlPixelDelta(SbVec2f(3.0F, -10.0F), 2.0F), SbVec2f(6.0F, 20.0F));
}
