// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#ifdef _MSC_VER
# pragma warning(disable : 4996)
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


class StringStreambufTest: public ::testing::Test
{
};

TEST_F(StringStreambufTest, OutputOverwritesAndSeeks)
{
    std::string buffer(4, 'x');
    Base::StringOStreambuf sbuf(buffer);
    std::ostream out(&sbuf);

    out.write("abcd", 4);
    EXPECT_EQ("abcd", buffer);

    out.seekp(2);
    out.write("Z", 1);
    EXPECT_EQ("abZd", buffer);
}

TEST_F(StringStreambufTest, RoundTripBinaryData)
{
    std::string buffer;
    Base::StringOStreambuf obuf(buffer);
    std::ostream out(&obuf);

    const std::string payload("abc\0def", 7);
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    out.seekp(0);
    out.write("X", 1);

    Base::StringIStreambuf ibuf(buffer);
    std::istream in(nullptr);
    in.rdbuf(&ibuf);

    std::string readBack(buffer.size(), '\0');
    in.read(readBack.data(), static_cast<std::streamsize>(readBack.size()));
    EXPECT_EQ(buffer, readBack);

    in.clear();
    in.seekg(3);
    EXPECT_EQ(0, in.get());
}
