// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <cstdint>
#include <iterator>
#include <sstream>
#include <string>
#include <zipios++/fcollexceptions.h>
#include <zipios++/zipinputstream.h>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
namespace
{

void appendLittleEndian(std::string& out, uint32_t value, unsigned int bytes)
{
    for (unsigned int i = 0; i < bytes; ++i) {
        out += static_cast<char>((value >> (8U * i)) & 0xFFU);
    }
}

// One stored (uncompressed) entry: the local file header from the zip specification, followed by
// the name and the data
std::string storedEntry(uint16_t flags)
{
    const std::string name = "hello.txt";
    const std::string data = "hello";
    const auto dataSize = static_cast<uint32_t>(data.size());
    std::string zip {};
    appendLittleEndian(zip, 0x04034B50, 4);
    appendLittleEndian(zip, 20, 2);
    appendLittleEndian(zip, flags, 2);
    appendLittleEndian(zip, 0, 2);
    appendLittleEndian(zip, 0, 2);
    appendLittleEndian(zip, 0, 2);
    appendLittleEndian(zip, 0x3610A686, 4);  // CRC-32 of "hello"
    appendLittleEndian(zip, dataSize, 4);
    appendLittleEndian(zip, dataSize, 4);
    appendLittleEndian(zip, static_cast<uint32_t>(name.size()), 2);
    appendLittleEndian(zip, 0, 2);
    return zip + name + data;
}

std::string readFirstEntry(const std::string& zip)
{
    auto input = std::istringstream(zip, std::ios::in | std::ios::binary);
    zipios::ZipInputStream stream(input);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

}  // namespace

TEST(ZipInputStream, TestStoredEntry)
{
    EXPECT_EQ(readFirstEntry(storedEntry(0)), "hello");
}

TEST(ZipInputStream, TestCompressionOptionBits)
{
    // compression options, "zip -1" sets bit 2
    EXPECT_EQ(readFirstEntry(storedEntry(0x4)), "hello");
    EXPECT_EQ(readFirstEntry(storedEntry(0x2)), "hello");
}

TEST(ZipInputStream, TestTrailingDataDescriptor)
{
    // bit 3: the sizes follow the data, which a stream can't read
    EXPECT_THROW(readFirstEntry(storedEntry(0x8)), zipios::FCollException);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
