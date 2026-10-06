// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <algorithm>
#include <random>
#include <sstream>
#include <string>
#include <zipios++/zipoutputstream.h>

namespace
{

// Behaves like a file on a device that runs out of space after 'limit' bytes
class LimitedStringBuf: public std::stringbuf
{
public:
    explicit LimitedStringBuf(std::streamsize limit)
        : limit {limit}
    {}

protected:
    std::streamsize xsputn(const char* data, std::streamsize count) override
    {
        return std::stringbuf::xsputn(data, std::min(count, available()));
    }

    int_type overflow(int_type ch) override
    {
        return available() > 0 ? std::stringbuf::overflow(ch) : traits_type::eof();
    }

private:
    std::streamsize available() const
    {
        return std::max<std::streamsize>(limit - static_cast<std::streamsize>(view().size()), 0);
    }

    std::streamsize limit;
};

std::string randomData(std::size_t size)
{
    std::mt19937 engine {42};
    std::string data(size, '\0');
    std::ranges::generate(data, [&engine] { return static_cast<char>(engine()); });
    return data;
}

void writeArchive(zipios::ZipOutputStream& zip)
{
    zip.putNextEntry("Document.xml");
    zip << "<Document/>";
    zip.putNextEntry("Shape.brp");
    zip << randomData(20000);
    zip.close();
}

std::streamsize archiveSize()
{
    std::stringbuf buf;
    std::ostream os {&buf};
    zipios::ZipOutputStream zip {os};
    writeArchive(zip);
    return static_cast<std::streamsize>(buf.view().size());
}

bool writeFails(std::streamsize limit)
{
    LimitedStringBuf buf {limit};
    std::ostream os {&buf};
    zipios::ZipOutputStream zip {os};
    writeArchive(zip);
    return zip.fail();
}

}  // namespace

TEST(ZipOutputStream, CompleteArchiveIsGood)
{
    EXPECT_FALSE(writeFails(archiveSize()));
}

TEST(ZipOutputStream, TruncatedEntryDataFails)
{
    EXPECT_TRUE(writeFails(archiveSize() / 2));
}

TEST(ZipOutputStream, TruncatedLastEntryFails)
{
    // enough room for everything but the end of the last entry and the central directory
    EXPECT_TRUE(writeFails(archiveSize() - 500));
}

TEST(ZipOutputStream, TruncatedCentralDirectoryFails)
{
    EXPECT_TRUE(writeFails(archiveSize() - 1));
}

TEST(ZipOutputStream, FailureInFirstHeaderFails)
{
    EXPECT_TRUE(writeFails(0));
}
