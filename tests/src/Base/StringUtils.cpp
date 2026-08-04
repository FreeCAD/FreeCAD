// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include <Base/StringUtils.h>

TEST(StringUtils, TestTrimmed)
{
    EXPECT_EQ(Base::StringUtils::trimmed(""), "");
    EXPECT_EQ(Base::StringUtils::trimmed("plain"), "plain");
    EXPECT_EQ(Base::StringUtils::trimmed("  value\t\r\n"), "value");
    EXPECT_EQ(Base::StringUtils::trimmed("\t \r\n"), "");
}

TEST(StringUtils, TestLowercaseAscii)
{
    EXPECT_EQ(Base::StringUtils::lowercaseAscii("ABC_xyz-123"), "abc_xyz-123");
    EXPECT_EQ(Base::StringUtils::lowercaseAscii("already lower"), "already lower");
    EXPECT_EQ(Base::StringUtils::lowercaseAscii("Symbols_!?"), "symbols_!?");
}

TEST(StringUtils, TestParseLong)
{
    long value = 0;

    EXPECT_TRUE(Base::StringUtils::parseLong("  -42\t", value));
    EXPECT_EQ(value, -42);
    EXPECT_TRUE(Base::StringUtils::parseLong("+17", value));
    EXPECT_EQ(value, 17);

    EXPECT_FALSE(Base::StringUtils::parseLong("", value));
    EXPECT_FALSE(Base::StringUtils::parseLong("12.5", value));
    EXPECT_FALSE(Base::StringUtils::parseLong("10 lines", value));
}

TEST(StringUtils, TestParseDouble)
{
    double value = 0.0;

    EXPECT_TRUE(Base::StringUtils::parseDouble("  -12.5\t", value));
    EXPECT_DOUBLE_EQ(value, -12.5);
    EXPECT_TRUE(Base::StringUtils::parseDouble("1.25e2", value));
    EXPECT_DOUBLE_EQ(value, 125.0);

    EXPECT_FALSE(Base::StringUtils::parseDouble("", value));
    EXPECT_FALSE(Base::StringUtils::parseDouble("12,5", value));
    EXPECT_FALSE(Base::StringUtils::parseDouble("10 mm", value));
}

TEST(StringUtils, TestParseBool)
{
    bool value = false;

    EXPECT_TRUE(Base::StringUtils::parseBool(" TrUe ", value));
    EXPECT_TRUE(value);
    EXPECT_TRUE(Base::StringUtils::parseBool("off", value));
    EXPECT_FALSE(value);
    EXPECT_TRUE(Base::StringUtils::parseBool("yes", value));
    EXPECT_TRUE(value);
    EXPECT_TRUE(Base::StringUtils::parseBool("0", value));
    EXPECT_FALSE(value);

    EXPECT_FALSE(Base::StringUtils::parseBool("sometimes", value));
    EXPECT_FALSE(Base::StringUtils::parseBool("", value));
}

TEST(StringUtils, TestFormatDouble)
{
    const double values[] = {
        0.1,
        std::nextafter(1.0, 2.0),
        std::numeric_limits<double>::min(),
        std::numeric_limits<double>::max(),
    };

    for (const double value : values) {
        double parsed = 0.0;
        EXPECT_TRUE(Base::StringUtils::parseDouble(Base::StringUtils::formatDouble(value), parsed));
        EXPECT_EQ(parsed, value);
    }
}
