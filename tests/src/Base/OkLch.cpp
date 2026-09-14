#include <gtest/gtest.h>
#include <cmath>

#include "Base/Color.h"
#include "Base/OkLch.h"

// NOLINTBEGIN

class OkLchTest: public ::testing::Test
{
};

// Round-trip: fromOkLch(toOkLch(color)) should approximate the original color.
TEST_F(OkLchTest, RoundTripBlack)
{
    Base::Color black(0.0f, 0.0f, 0.0f);
    auto oklch = Base::toOkLch(black);
    auto result = Base::fromOkLch(oklch);
    EXPECT_NEAR(result.r, 0.0f, 0.01f);
    EXPECT_NEAR(result.g, 0.0f, 0.01f);
    EXPECT_NEAR(result.b, 0.0f, 0.01f);
}

TEST_F(OkLchTest, RoundTripWhite)
{
    Base::Color white(1.0f, 1.0f, 1.0f);
    auto oklch = Base::toOkLch(white);
    auto result = Base::fromOkLch(oklch);
    EXPECT_NEAR(result.r, 1.0f, 0.01f);
    EXPECT_NEAR(result.g, 1.0f, 0.01f);
    EXPECT_NEAR(result.b, 1.0f, 0.01f);
}

TEST_F(OkLchTest, RoundTripRed)
{
    Base::Color red(1.0f, 0.0f, 0.0f);
    auto oklch = Base::toOkLch(red);
    auto result = Base::fromOkLch(oklch);
    EXPECT_NEAR(result.r, 1.0f, 0.01f);
    EXPECT_NEAR(result.g, 0.0f, 0.01f);
    EXPECT_NEAR(result.b, 0.0f, 0.01f);
}

TEST_F(OkLchTest, RoundTripGreen)
{
    Base::Color green(0.0f, 1.0f, 0.0f);
    auto oklch = Base::toOkLch(green);
    auto result = Base::fromOkLch(oklch);
    EXPECT_NEAR(result.r, 0.0f, 0.01f);
    EXPECT_NEAR(result.g, 1.0f, 0.01f);
    EXPECT_NEAR(result.b, 0.0f, 0.01f);
}

TEST_F(OkLchTest, RoundTripBlue)
{
    Base::Color blue(0.0f, 0.0f, 1.0f);
    auto oklch = Base::toOkLch(blue);
    auto result = Base::fromOkLch(oklch);
    EXPECT_NEAR(result.r, 0.0f, 0.01f);
    EXPECT_NEAR(result.g, 0.0f, 0.01f);
    EXPECT_NEAR(result.b, 1.0f, 0.01f);
}

TEST_F(OkLchTest, RoundTripMidGray)
{
    Base::Color gray(0.5f, 0.5f, 0.5f);
    auto oklch = Base::toOkLch(gray);
    auto result = Base::fromOkLch(oklch);
    EXPECT_NEAR(result.r, 0.5f, 0.01f);
    EXPECT_NEAR(result.g, 0.5f, 0.01f);
    EXPECT_NEAR(result.b, 0.5f, 0.01f);
}

// Known values: black has L=0, white has L~1, gray has low chroma.
TEST_F(OkLchTest, BlackHasZeroLightness)
{
    auto oklch = Base::toOkLch(Base::Color(0.0f, 0.0f, 0.0f));
    EXPECT_FLOAT_EQ(oklch.lightness, 0.0f);
}

TEST_F(OkLchTest, WhiteHasFullLightness)
{
    auto oklch = Base::toOkLch(Base::Color(1.0f, 1.0f, 1.0f));
    EXPECT_NEAR(oklch.lightness, 1.0f, 0.01f);
}

TEST_F(OkLchTest, GrayHasLowChroma)
{
    auto oklch = Base::toOkLch(Base::Color(0.5f, 0.5f, 0.5f));
    EXPECT_NEAR(oklch.chroma, 0.0f, 0.01f);
}

// Gamut mapping: fromOkLch with very high chroma should return valid sRGB.
TEST_F(OkLchTest, GamutMappingClampsToValidSrgb)
{
    Base::OkLch outOfGamut = {.lightness = 0.7f, .chroma = 0.5f, .hue = 120.0f};
    auto result = Base::fromOkLch(outOfGamut);
    EXPECT_GE(result.r, 0.0f);
    EXPECT_LE(result.r, 1.0f);
    EXPECT_GE(result.g, 0.0f);
    EXPECT_LE(result.g, 1.0f);
    EXPECT_GE(result.b, 0.0f);
    EXPECT_LE(result.b, 1.0f);
}

// Alpha is preserved through conversion.
TEST_F(OkLchTest, AlphaPreserved)
{
    Base::Color semiTransparent(1.0f, 0.0f, 0.0f, 0.5f);
    auto oklch = Base::toOkLch(semiTransparent);
    auto result = Base::fromOkLch(oklch, semiTransparent.a);
    EXPECT_NEAR(result.a, 0.5f, 0.01f);
}

// NOLINTEND
