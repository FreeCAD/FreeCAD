// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <cstdint>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <zlib.h>
#include <zipios/zipiosexceptions.hpp>

#include <Base/Writer.h>
#include <Base/ZipReader.h>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
namespace
{

using Entries = std::vector<std::pair<std::string, std::string>>;

void appendLittleEndian(std::string& out, uint32_t value, unsigned int bytes)
{
    for (unsigned int i = 0; i < bytes; ++i) {
        out += static_cast<char>((value >> (8U * i)) & 0xFFU);
    }
}

// A zip archive with stored (uncompressed) entries, built byte by byte after the zip specification,
// so that the order of the entries and their flags can be anything
std::string storedZip(const Entries& entries, uint16_t flags = 0)
{
    std::string local {};
    std::string central {};
    for (const auto& [name, data] : entries) {
        const auto crc = static_cast<uint32_t>(
            crc32(0, reinterpret_cast<const Bytef*>(data.data()), static_cast<uInt>(data.size()))
        );
        const auto size = static_cast<uint32_t>(data.size());
        const auto nameSize = static_cast<uint32_t>(name.size());
        const auto offset = static_cast<uint32_t>(local.size());
        appendLittleEndian(local, 0x04034B50, 4);
        appendLittleEndian(local, 20, 2);
        appendLittleEndian(local, flags, 2);
        appendLittleEndian(local, 0, 2);
        appendLittleEndian(local, 0, 4);
        appendLittleEndian(local, crc, 4);
        appendLittleEndian(local, size, 4);
        appendLittleEndian(local, size, 4);
        appendLittleEndian(local, nameSize, 2);
        appendLittleEndian(local, 0, 2);
        local += name + data;

        appendLittleEndian(central, 0x02014B50, 4);
        appendLittleEndian(central, 20, 2);
        appendLittleEndian(central, 20, 2);
        appendLittleEndian(central, flags, 2);
        appendLittleEndian(central, 0, 2);
        appendLittleEndian(central, 0, 4);
        appendLittleEndian(central, crc, 4);
        appendLittleEndian(central, size, 4);
        appendLittleEndian(central, size, 4);
        appendLittleEndian(central, nameSize, 2);
        appendLittleEndian(central, 0, 6);
        appendLittleEndian(central, 0, 2);
        appendLittleEndian(central, 0, 4);
        appendLittleEndian(central, offset, 4);
        central += name;
    }
    std::string end {};
    appendLittleEndian(end, 0x06054B50, 4);
    appendLittleEndian(end, 0, 4);
    appendLittleEndian(end, static_cast<uint32_t>(entries.size()), 2);
    appendLittleEndian(end, static_cast<uint32_t>(entries.size()), 2);
    appendLittleEndian(end, static_cast<uint32_t>(central.size()), 4);
    appendLittleEndian(end, static_cast<uint32_t>(local.size()), 4);
    appendLittleEndian(end, 0, 2);
    return local + central + end;
}

// A zip archive with compressed entries, written like FreeCAD writes its documents
std::string compressedZip(const Entries& entries)
{
    auto out = std::ostringstream(std::ios::out | std::ios::binary);
    {
        Base::ZipWriter writer(out);
        for (const auto& [name, data] : entries) {
            writer.putNextEntry(name.c_str());
            writer.Stream() << data;
        }
    }
    return out.str();
}

std::string read(std::istream& stream)
{
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

std::string read(const Base::ZipReader& reader, const std::string& name)
{
    auto stream = reader.getInputStream(name);
    return stream ? read(*stream) : std::string("<missing>");
}

std::string largeData(char first)
{
    std::string data {};
    for (int i = 0; i < 300000; ++i) {
        data += static_cast<char>(first + (i % 23));
    }
    return data;
}

}  // namespace

TEST(ZipReader, TestEntryNames)
{
    std::istringstream input(storedZip({{"b", "2"}, {"a", "1"}, {"c", "3"}}));
    Base::ZipReader reader(input);
    EXPECT_EQ(reader.entryNames(), (std::vector<std::string> {"b", "a", "c"}));
    EXPECT_TRUE(reader.hasEntry("a"));
    EXPECT_FALSE(reader.hasEntry("d"));
    EXPECT_EQ(reader.getInputStream("d"), nullptr);
}

TEST(ZipReader, TestReadInAnyOrder)
{
    std::istringstream input(storedZip({{"b", "second"}, {"a", "first"}, {"c", "third"}}));
    Base::ZipReader reader(input);
    EXPECT_EQ(read(reader, "c"), "third");
    EXPECT_EQ(read(reader, "a"), "first");
    EXPECT_EQ(read(reader, "b"), "second");
    EXPECT_EQ(read(reader, "a"), "first");
}

TEST(ZipReader, TestCompressedEntries)
{
    const std::string a = largeData('a');
    const std::string b = largeData('A');
    auto reader = Base::ZipReader(
        std::make_unique<std::istringstream>(compressedZip({{"a", a}, {"b", b}}))
    );
    EXPECT_EQ(read(reader, "b"), b);
    EXPECT_EQ(read(reader, "a"), a);
}

TEST(ZipReader, TestStreamsAtTheSameTime)
{
    const std::string a = largeData('a');
    const std::string b = largeData('A');
    std::istringstream input(compressedZip({{"a", a}, {"b", b}}));
    Base::ZipReader reader(input);
    auto streamA = reader.getInputStream("a");
    auto streamB = reader.getInputStream("b");
    std::string readA {};
    std::string readB {};
    std::vector<char> chunk(1000);
    while (*streamA || *streamB) {
        streamA->read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        readA.append(chunk.data(), static_cast<std::size_t>(streamA->gcount()));
        streamB->read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        readB.append(chunk.data(), static_cast<std::size_t>(streamB->gcount()));
    }
    EXPECT_EQ(readA, a);
    EXPECT_EQ(readB, b);
}

TEST(ZipReader, TestCompressionOptionBits)
{
    // compression options, "zip -1" sets bit 2
    std::istringstream input(storedZip({{"a", "data"}}, 0x4));
    Base::ZipReader reader(input);
    EXPECT_EQ(read(reader, "a"), "data");
}

TEST(ZipReader, TestTrailingDataDescriptor)
{
    // bit 3: the sizes follow the data, which can't be read
    std::istringstream input(storedZip({{"a", "data"}}, 0x8));
    EXPECT_THROW(
        {
            Base::ZipReader reader(input);
            read(reader, "a");
        },
        zipios::Exception
    );
}

TEST(ZipReader, TestNotAZipArchive)
{
    std::istringstream input("this is not a zip archive");
    EXPECT_THROW(Base::ZipReader reader(input), zipios::Exception);
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
