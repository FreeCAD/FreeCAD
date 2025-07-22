// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include "Base/Stream.h"


class TextOutputStreamTest: public ::testing::Test
{
protected:
    void SetUp() override
    {}

    void TearDown() override
    {}
};

TEST_F(TextOutputStreamTest, singleLineCharStar)
{
    // Arrange
    const std::string testString("Single line const char *");
    std::ostringstream ss;
    Base::TextOutputStream tos(ss);

    // Act
    tos << testString;

    // Assert - the number of newlines in the string, a colon, the string, a newline
    EXPECT_EQ(std::string("0:") + testString + "\n", ss.str());
}

TEST_F(TextOutputStreamTest, multiLineCharStar)
{
    // Arrange
    const std::string testString("Multi-line\nconst char *");
    std::ostringstream ss;
    Base::TextOutputStream tos(ss);

    // Act
    tos << testString;  // Testing it with a string instead of a const char * -- failing right now

    // Assert - the number of newlines in the string, a colon, the string, a newline
    EXPECT_EQ(std::string("1:") + testString + "\n", ss.str());
}

TEST_F(TextOutputStreamTest, singleLineCharStarWithCarriageReturns)
{
    // Arrange
    const std::string testString("Single-line\rconst char *");
    std::ostringstream ss;
    Base::TextOutputStream tos(ss);

    // Act
    tos << testString;

    // Assert - the number of newlines in the string, a colon, the string, a newline. Carriage
    // returns are left alone because they aren't followed by a newline
    EXPECT_EQ(std::string("0:") + testString + "\n", ss.str());
}

TEST_F(TextOutputStreamTest, multiLineCharStarWithCarriageReturnsAndNewlines)
{
    // Arrange
    const std::string testString("Multi-line\r\nconst char *");
    const std::string testStringWithoutCR("Multi-line\nconst char *");
    std::ostringstream ss;
    Base::TextOutputStream tos(ss);

    // Act
    tos << testString;

    // Assert - the number of newlines in the string, a colon, the string, a newline, but the string
    // has been stripped of the carriage returns.
    EXPECT_EQ(std::string("1:") + testStringWithoutCR + "\n", ss.str());
}


class TextStreamIntegrationTest: public ::testing::Test
{
protected:
    void SetUp() override
    {}

    void TearDown() override
    {}
};

TEST_F(TextStreamIntegrationTest, OutputThenInputSimpleMultiLine)
{
    // Arrange
    std::string multiLineString("One\nTwo\nThree");

    // Act
    std::ostringstream ssO;
    Base::TextOutputStream tos(ssO);
    tos << multiLineString;
    std::istringstream ssI(ssO.str());
    Base::TextInputStream tis(ssI);
    std::string result;
    tis >> result;

    // Assert
    EXPECT_EQ(multiLineString, result);
}

TEST_F(TextStreamIntegrationTest, OutputThenInputMultiLineWithCarriageReturns)
{
    // Arrange
    std::string multiLineString("One\r\nTwo\r\nThree");
    std::string multiLineStringResult("One\nTwo\nThree");

    // Act
    std::ostringstream ssO;
    Base::TextOutputStream tos(ssO);
    tos << multiLineString;
    std::istringstream ssI(ssO.str());
    Base::TextInputStream tis(ssI);
    std::string result;
    tis >> result;

    // Assert
    EXPECT_EQ(multiLineStringResult, result);
}
