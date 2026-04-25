// SPDX-License-Identifier: LGPL-2.1-or-later

#include <array>

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


// Change this type alias to std::streambuf should still have the tests below pass.
using BufferStreambuf = Base::BufferStreambuf;

TEST(BufferStreambuf, emptyBufferIn)
{
    std::array<char, 0> array;
    BufferStreambuf streambuf(array, std::ios::in);
    std::istream stream(&streambuf);

    // Read a byte with get()
    EXPECT_TRUE(stream.good());
    EXPECT_EQ(stream.get(), std::istream::traits_type::eof());
    EXPECT_TRUE(stream.eof());
    EXPECT_TRUE(stream.fail());

    // Peek a byte
    stream.seekg(0, std::ios::beg);
    stream.clear();
    EXPECT_TRUE(stream.good());
    EXPECT_EQ(stream.peek(), std::istream::traits_type::eof());
    EXPECT_TRUE(stream.eof());
    EXPECT_FALSE(stream.fail());

    // Read a char with formatted input
    stream.seekg(0, std::ios::beg);
    stream.clear();
    EXPECT_TRUE(stream.good());
    char c = 123;
    stream >> c;
    EXPECT_EQ(c, 123);
    EXPECT_TRUE(stream.eof());
    EXPECT_TRUE(stream.fail());
}

TEST(BufferStreambuf, emptyBufferOut)
{
    std::array<char, 0> array;
    BufferStreambuf streambuf(array, std::ios::out);
    std::ostream stream(&streambuf);

    // put() a char
    EXPECT_TRUE(stream.good());
    stream.put('a');
    EXPECT_TRUE(stream.fail());

    // write() a string
    stream.seekp(0, std::ios::beg);
    stream.clear();
    EXPECT_TRUE(stream.good());
    stream.write("ab", 2);
    EXPECT_TRUE(stream.fail());

    // Insert a string with formatted output
    stream.seekp(0, std::ios::beg);
    stream.clear();
    EXPECT_TRUE(stream.good());
    stream << "test";
    EXPECT_TRUE(stream.fail());
}

TEST(BufferStreambuf, bufferIn)  // codespell:ignore bufferin
{
    auto array = std::to_array("h\0ello\nline2\r\nline3|9");
    BufferStreambuf streambuf(array, std::ios::in);
    std::istream stream(&streambuf);

    // Check stream size matches the buffer size
    EXPECT_TRUE(stream.good());
    stream.seekg(0, std::ios::end);
    EXPECT_EQ(stream.tellg(), array.size());
    stream.seekg(0, std::ios::beg);

    // Read bytes with get()
    EXPECT_TRUE(stream.good());
    EXPECT_EQ(stream.get(), 'h');
    EXPECT_TRUE(stream.good());
    EXPECT_EQ(stream.get(), '\0');
    EXPECT_TRUE(stream.good());

    // Extract strings with formatted input
    stream.seekg(5, std::ios::beg);
    std::string line;
    stream >> line;
    EXPECT_EQ(line, "o");
    stream >> line;
    EXPECT_EQ(line, "line2");

    // Extract strings with getline()
    std::array<char, 8> aline;
    stream.getline(aline.data(), aline.size(), '|');
    EXPECT_EQ(aline[0], '\r');
    EXPECT_EQ(aline[1], '\n');
    EXPECT_EQ(aline[6], '3');
    EXPECT_EQ(aline[7], '\0');
    EXPECT_FALSE(stream.eof());

    // Extract numbers with formatted input
    unsigned int number = -1;
    stream >> number;
    EXPECT_EQ(number, 9);
    EXPECT_FALSE(stream.fail());
    EXPECT_EQ(stream.get(), '\0');  // Consume string literal null terminator
    EXPECT_FALSE(stream.fail());
    EXPECT_FALSE(stream.eof());
    stream >> number;
    // [istream.formatted.reqmts] precludes the formatted input from working at all when
    // at eof because the "sentry" object will signal failure before conversion is attempted.
    // `number` is therefore left untouched, and the fail & eof bits set as usual.
    EXPECT_EQ(number, 9);
    EXPECT_TRUE(stream.fail());
    EXPECT_TRUE(stream.eof());

    // Seek to and then go before begin
    stream.clear();
    stream.seekg(0, std::ios::beg);
    stream.seekg(-8, std::ios::cur);
    EXPECT_EQ(stream.tellg(), -1);
    EXPECT_TRUE(stream.fail());
    // Seek to and then go past end
    stream.clear();
    stream.seekg(array.size(), std::ios::beg);
    EXPECT_EQ(stream.tellg(), array.size());
    stream.seekg(1, std::ios::cur);
    EXPECT_EQ(stream.tellg(), -1);
    EXPECT_TRUE(stream.fail());
}

TEST(BufferStreambuf, bufferOut)
{
    std::array<char, 12> array;
    BufferStreambuf streambuf(array, std::ios::out | std::ios::ate);
    std::ostream stream(&streambuf);

    // Expect to start at end of array
    EXPECT_TRUE(stream.good());
    EXPECT_EQ(stream.tellp(), array.size());
    // Seek past the end at once
    stream.seekp(array.size() + 4, std::ios::cur);
    EXPECT_EQ(stream.tellp(), -1);
    EXPECT_TRUE(stream.fail());
    // Seek to std::ios::end, which is by definition *not* the buffer end in out-only mode
    // but the current cursor position ([spanbuf.virtuals] 4.3.1)
    stream.clear();
    stream.seekp(3, std::ios::beg);
    stream.seekp(0, std::ios::end);
    EXPECT_EQ(stream.tellp(), 3);
    // Seek to and then go before begin
    stream.clear();
    stream.seekp(0, std::ios::beg);
    stream.seekp(-8, std::ios::cur);
    EXPECT_EQ(stream.tellp(), -1);
    EXPECT_TRUE(stream.fail());
    // Seek to and then go past end
    stream.clear();
    stream.seekp(array.size(), std::ios::beg);
    EXPECT_EQ(stream.tellp(), array.size());
    stream.seekp(1, std::ios::cur);
    EXPECT_EQ(stream.tellp(), -1);
    EXPECT_TRUE(stream.fail());

    // Insert a byte with put()
    stream.clear();
    stream.seekp(0, std::ios::beg);
    stream.put('a');
    EXPECT_EQ(array[0], 'a');

    // Insert strings with formatted output
    stream << "123\n456";
    EXPECT_EQ(array[1], '1');
    EXPECT_EQ(array[4], '\n');
    EXPECT_EQ(array[5], '4');
    stream << "z";
    EXPECT_EQ(array[8], 'z');

    // Insert numbers with formatted output
    stream << 89;
    EXPECT_EQ(array[9], '8');
    EXPECT_EQ(array[10], '9');

    // Insert a string and then overflow the buffer
    stream.write("eof", 3);
    stream.put('X');
    EXPECT_TRUE(stream.fail());

    // Insert a string to overflow the buffer at once
    stream.clear();
    stream.seekp(array.size() - 2, std::ios::beg);
    stream.write("EOF", 3);
    EXPECT_TRUE(stream.fail());
}
