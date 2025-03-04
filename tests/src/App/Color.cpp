#include <gtest/gtest.h>
#include <cmath>
#include "Base/Color.h"

// NOLINTBEGIN
TEST(TestColor, equal)
{
    int red = 85;
    int green = 170;
    int blue = 255;

    Base::Color color(red / 255.0F, green / 255.0F, blue / 255.0F);
    uint32_t packed = color.getPackedValue();

    Base::Color color2 {packed};
    EXPECT_EQ(color, color2);
}

TEST(TestColor, round)
{
    for (int index = 0; index < 256; index++) {
        float value = static_cast<float>(index) / 255.0F;
        EXPECT_EQ(std::lround(value * 255.0F), index);
    }
}

TEST(TestColor, packedValue)
{
    int red = 85;
    int green = 170;
    int blue = 255;

    Base::Color color(red / 255.0F, green / 255.0F, blue / 255.0F);
    uint32_t packed = color.getPackedValue();
    Base::Color color2 {packed};

    EXPECT_EQ(std::lround(color2.r * 255.0F), 85);
    EXPECT_EQ(std::lround(color2.g * 255.0F), 170);
    EXPECT_EQ(std::lround(color2.b * 255.0F), 255);
}

TEST(TestColor, packedRGB)
{
    int red = 85;
    int green = 170;
    int blue = 255;

    Base::Color color(red / 255.0F, green / 255.0F, blue / 255.0F);
    uint32_t packed = color.getPackedRGB();
    Base::Color color2;
    color2.setPackedRGB(packed);

    EXPECT_EQ(std::lround(color2.r * 255.0F), 85);
    EXPECT_EQ(std::lround(color2.g * 255.0F), 170);
    EXPECT_EQ(std::lround(color2.b * 255.0F), 255);
}

TEST(TestColor, packedARGB)
{
    int red = 85;
    int green = 170;
    int blue = 255;
    int alpha = 127;

    Base::Color color(red / 255.0F, green / 255.0F, blue / 255.0F, alpha / 255.0F);
    uint32_t packed = color.getPackedARGB();
    Base::Color color2;
    color2.setPackedARGB(packed);

    EXPECT_EQ(std::lround(color2.r * 255.0F), 85);
    EXPECT_EQ(std::lround(color2.g * 255.0F), 170);
    EXPECT_EQ(std::lround(color2.b * 255.0F), 255);
    EXPECT_EQ(std::lround(color2.a * 255.0F), 127);
}

struct IntColor
{
    IntColor(int red, int green, int blue, int alpha = 0)
        : red_ {red}
        , green_ {green}
        , blue_ {blue}
        , alpha_ {alpha}
    {}

    float redF() const
    {
        return static_cast<float>(red_) / 255.0F;
    }

    float greenF() const
    {
        return static_cast<float>(green_) / 255.0F;
    }

    float blueF() const
    {
        return static_cast<float>(blue_) / 255.0F;
    }

    float alphaF() const
    {
        return static_cast<float>(alpha_) / 255.0F;
    }

    int red() const
    {
        return red_;
    }

    int green() const
    {
        return green_;
    }

    int blue() const
    {
        return blue_;
    }

    int alpha() const
    {
        return alpha_;
    }

private:
    int red_ {};
    int green_ {};
    int blue_ {};
    int alpha_ {};
};

TEST(TestColor, asPackedRGB)
{
    int red = 85;
    int green = 170;
    int blue = 255;

    IntColor intColor {red, green, blue};
    uint32_t packed = Base::Color::asPackedRGB<IntColor>(intColor);


    EXPECT_EQ(packed, 1437269760);
}

TEST(TestColor, fromPackedRGB)
{
    int red = 85;
    int green = 170;
    int blue = 255;

    Base::Color color(red / 255.0F, green / 255.0F, blue / 255.0F);

    IntColor intColor = Base::Color::fromPackedRGB<IntColor>(color.getPackedRGB());

    EXPECT_EQ(intColor.red(), red);
    EXPECT_EQ(intColor.green(), green);
    EXPECT_EQ(intColor.blue(), blue);
}

TEST(TestColor, asPackedRGBA)
{
    int red = 85;
    int green = 170;
    int blue = 255;
    int alpha = 127;

    IntColor intColor {red, green, blue, alpha};
    uint32_t packed = Base::Color::asPackedRGBA<IntColor>(intColor);


    EXPECT_EQ(packed, 1437269887);
}

TEST(TestColor, fromPackedRGBA)
{
    int red = 85;
    int green = 170;
    int blue = 255;
    int alpha = 127;

    Base::Color color(red / 255.0F, green / 255.0F, blue / 255.0F, alpha / 255.0F);

    IntColor intColor = Base::Color::fromPackedRGBA<IntColor>(color.getPackedValue());

    EXPECT_EQ(intColor.red(), red);
    EXPECT_EQ(intColor.green(), green);
    EXPECT_EQ(intColor.blue(), blue);
    EXPECT_EQ(intColor.alpha(), alpha);
}

TEST(TestColor, setValue)
{
    int red = 85;
    int green = 170;
    int blue = 255;

    IntColor intColor {red, green, blue};
    Base::Color color {};
    color.setValue<IntColor>(intColor);

    EXPECT_FLOAT_EQ(color.r, 85.0F / 255.0F);
    EXPECT_FLOAT_EQ(color.g, 170.0F / 255.0F);
    EXPECT_FLOAT_EQ(color.b, 255.0F / 255.0F);
}

TEST(TestColor, asValue)
{
    int red = 85;
    int green = 170;
    int blue = 255;

    Base::Color color(red / 255.0F, green / 255.0F, blue / 255.0F);

    IntColor intColor = color.asValue<IntColor>();

    EXPECT_EQ(intColor.red(), 85);
    EXPECT_EQ(intColor.green(), 170);
    EXPECT_EQ(intColor.blue(), 255);
}

TEST(TestColor, asHexString)
{
    int red = 85;
    int green = 170;
    int blue = 255;

    Base::Color color(red / 255.0F, green / 255.0F, blue / 255.0F);

    EXPECT_EQ(color.asHexString(), "#55AAFF");
}

TEST(TestColor, fromHexString)
{
    Base::Color color;
    EXPECT_FALSE(color.fromHexString(std::string("")));
    EXPECT_FALSE(color.fromHexString(std::string("abcdef")));
    EXPECT_TRUE(color.fromHexString(std::string("#abcdef")));
    EXPECT_TRUE(color.fromHexString(std::string("#ABCDEF")));
    EXPECT_FALSE(color.fromHexString(std::string("#abcde")));
    EXPECT_FALSE(color.fromHexString(std::string("#abcdeff")));
    EXPECT_FALSE(color.fromHexString(std::string("#abcdeffff")));
    EXPECT_TRUE(color.fromHexString(std::string("#55AAFF")));
    EXPECT_FLOAT_EQ(color.r, 85.0F / 255.0F);
    EXPECT_FLOAT_EQ(color.g, 170.0F / 255.0F);
    EXPECT_FLOAT_EQ(color.b, 255.0F / 255.0F);
    EXPECT_FLOAT_EQ(color.a, 255.0F / 255.0F);
}
// NOLINTEND
