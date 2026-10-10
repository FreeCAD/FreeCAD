// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include <zlib.h>

#include <Base/FileInfo.h>
#include <Base/ZipOutputStream.h>
#include <Base/ZipReader.h>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
namespace
{

std::string lines()
{
    std::string data {};
    for (int i = 0; i < 20000; ++i) {
        data += "line " + std::to_string(i % 97) + "\n";
    }
    return data;
}

std::string read(const Base::ZipReader& reader, const std::string& name)
{
    auto stream = reader.getInputStream(name);
    return stream ? std::string(std::istreambuf_iterator<char>(*stream), {}) : "<missing>";
}

std::string gunzip(const std::string& data)
{
    z_stream zs {};
    inflateInit2(&zs, 16 + MAX_WBITS);
    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
    zs.avail_in = static_cast<uInt>(data.size());
    std::string result {};
    std::vector<char> chunk(4096);
    int status = Z_OK;
    while (status == Z_OK) {
        zs.next_out = reinterpret_cast<Bytef*>(chunk.data());
        zs.avail_out = static_cast<uInt>(chunk.size());
        status = inflate(&zs, Z_NO_FLUSH);
        result.append(chunk.data(), chunk.size() - zs.avail_out);
    }
    inflateEnd(&zs);
    return status == Z_STREAM_END ? result : "<invalid>";
}

}  // namespace

TEST(ZipOutputStream, TestFlush)
{
    // Zipios itself fails a flush, std::endl flushes
    auto out = std::ostringstream(std::ios::out | std::ios::binary);
    {
        Base::ZipOutputStream zip(out);
        zip.putNextEntry("a.txt");
        zip << "first" << std::endl << "second" << std::endl;
        zip.flush();
        EXPECT_TRUE(zip.good());
        zip.putNextEntry("b.txt");
        zip << "third" << std::endl;
    }
    std::istringstream input(out.str());
    Base::ZipReader reader(input);
    EXPECT_EQ(read(reader, "a.txt"), "first\nsecond\n");
    EXPECT_EQ(read(reader, "b.txt"), "third\n");
}

TEST(ZipOutputStream, TestLevels)
{
    const std::string data = lines();
    std::vector<std::size_t> sizes;
    for (int level : {0, 1, 9}) {
        auto out = std::ostringstream(std::ios::out | std::ios::binary);
        {
            Base::ZipOutputStream zip(out);
            zip.setLevel(level);
            zip.putNextEntry("data.txt");
            zip << data;
        }
        sizes.push_back(out.str().size());
        std::istringstream input(out.str());
        Base::ZipReader reader(input);
        EXPECT_EQ(read(reader, "data.txt"), data) << "level " << level;
    }
    EXPECT_GT(sizes[0], data.size());
    EXPECT_LT(sizes[1], data.size() / 4);
    EXPECT_LE(sizes[2], sizes[1]);
}

TEST(ZipOutputStream, TestNamesAreNotLookedUpOnDisk)
{
    // an existing folder, Zipios would refuse to compress an entry named after it
    const std::string name = Base::FileInfo::getTempPath() + "entry";
    Base::FileInfo(name).createDirectory();
    auto out = std::ostringstream(std::ios::out | std::ios::binary);
    {
        Base::ZipOutputStream zip(out);
        zip.setLevel(9);
        zip.putNextEntry(name);
        zip << "data";
        zip.putNextEntry("folder/file.txt");
        zip << "more";
    }
    Base::FileInfo(name).deleteDirectory();
    std::istringstream input(out.str());
    Base::ZipReader reader(input);
    EXPECT_EQ(read(reader, name), "data");
    EXPECT_EQ(read(reader, "folder/file.txt"), "more");
}

TEST(ZipOutputStream, TestEmptyEntry)
{
    // stored: an empty deflated entry without data is rejected by unzip, 7-Zip and Zipios
    auto out = std::ostringstream(std::ios::out | std::ios::binary);
    {
        Base::ZipOutputStream zip(out);
        zip.putNextEntry("empty");
        zip.putNextEntry("data.txt");
        zip << "data";
    }
    const std::string archive = out.str();
    const int method = static_cast<unsigned char>(archive[8])
        | (static_cast<unsigned char>(archive[9]) << 8);
    EXPECT_EQ(method, 0);
    std::istringstream input(archive);
    Base::ZipReader reader(input);
    EXPECT_EQ(read(reader, "empty"), "");
    EXPECT_EQ(read(reader, "data.txt"), "data");
}

TEST(GZipOutputStream, TestFlush)
{
    const std::string data = lines();
    auto out = std::ostringstream(std::ios::out | std::ios::binary);
    {
        Base::GZipOutputStream gzip(out);
        gzip << data << std::endl;
        EXPECT_TRUE(gzip.good());
    }
    EXPECT_LT(out.str().size(), data.size() / 4);
    EXPECT_EQ(gunzip(out.str()), data + "\n");
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
