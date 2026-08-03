#include <gtest/gtest.h>

#include <Base/InputSource.h>

#include <sstream>
#include <string>

TEST(StdInputStream, SanitizesNullAndInvalidUtf8)
{
    // bytes: 'A' 0xC3 0x28 'B' 0x00 'C' 0x80
    // - 0xC3 0x28 is an invalid 2-byte sequence; we replace the lead byte with '?'
    // - 0x00 is replaced with '?'
    // - 0x80 is a stray continuation byte; replaced with '?'
    std::string bytes;
    bytes.push_back('A');
    bytes.push_back(static_cast<char>(0xC3));
    bytes.push_back(static_cast<char>(0x28));
    bytes.push_back('B');
    bytes.push_back(static_cast<char>(0x00));
    bytes.push_back('C');
    bytes.push_back(static_cast<char>(0x80));

    std::istringstream in(std::string(bytes.data(), bytes.size()), std::ios::binary);
    Base::StdInputStream stream(in);

    XMLByte out[32] {};
    const XMLSize_t n = stream.readBytes(out, sizeof(out));
    ASSERT_EQ(n, bytes.size());

    // Ensure no embedded NUL remains.
    for (XMLSize_t i = 0; i < n; ++i) {
        EXPECT_NE(out[i], static_cast<XMLByte>(0)) << "NUL byte remained at index " << i;
    }

    EXPECT_EQ(out[0], static_cast<XMLByte>('A'));
    EXPECT_EQ(out[1], static_cast<XMLByte>('?'));  // invalid lead byte replaced
    EXPECT_EQ(out[2], static_cast<XMLByte>('('));  // ASCII preserved
    EXPECT_EQ(out[3], static_cast<XMLByte>('B'));
    EXPECT_EQ(out[4], static_cast<XMLByte>('?'));  // embedded NUL replaced
    EXPECT_EQ(out[5], static_cast<XMLByte>('C'));
    EXPECT_EQ(out[6], static_cast<XMLByte>('?'));  // stray continuation replaced
}
