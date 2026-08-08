// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Gui/Inventor/SoMouseWheelEvent.h>

TEST(SoMouseWheelEvent, defaultConstructedIsNotPrecise)
{
    SoMouseWheelEvent event;

    EXPECT_EQ(event.getDelta(), 0);
    EXPECT_FALSE(event.isPrecise());
}

TEST(SoMouseWheelEvent, pixelDeltaIsScaledToDevicePixelsAndFlippedToGlOrientation)
{
    SoMouseWheelEvent event;

    event.setPixelDelta(SbVec2f(3.0F, -10.0F), 2.0F);

    EXPECT_EQ(event.getPixelDelta(), SbVec2f(6.0F, 20.0F));
    EXPECT_TRUE(event.isPrecise());
}

TEST(SoMouseWheelEvent, nullPixelDeltaIsNotPreciseEvenWithWheelDelta)
{
    SoMouseWheelEvent event;

    event.setDelta(120);
    event.setPixelDelta(SbVec2f(0.0F, 0.0F), 2.0F);

    EXPECT_EQ(event.getDelta(), 120);
    EXPECT_FALSE(event.isPrecise());
}
