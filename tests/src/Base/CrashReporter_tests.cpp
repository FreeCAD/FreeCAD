// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD project association AISBL
// SPDX-FileNotice: Part of the FreeCAD project.

#include <gtest/gtest.h>

#include <Base/CrashReporter/Reader.h>
#include <src/TempDirectory.h>

#include <fstream>
#include <cstring>

class CrashReporterReaderTests: public ::testing::Test
{
public:
    std::vector<char> createGoodCrashReport();

protected:
    tests::TempDirectory tempDir{"crash_reader"};
};

uint32_t addStringToTable(std::vector<char> &stringTable, const std::string &string)
{
    uint32_t offset = stringTable.size();
    stringTable.resize(offset + sizeof(uint16_t) + string.length());
    uint16_t length {static_cast<uint16_t>(string.length())};
    std::memcpy(stringTable.data()+offset, &length, sizeof(uint16_t));
    std::memcpy(stringTable.data()+offset+sizeof(uint16_t), string.data(), length);
    return offset;
}

std::vector<char> CrashReporterReaderTests::createGoodCrashReport()
{
    std::vector<char> buffer;

    Base::CrashReporter::Header header;
    header.faultAddress = 0xDEADBEEFCAFEF00D;
    header.threadID = 0x1122334455667788;
    header.timestamp = 1700000000;
    header.processID = 0xABCD;
    header.code = 11; // SIGSEGV
    header.freecadVersionMajor = 5;
    header.freecadVersionMinor = 6;
    header.freecadVersionPatch = 7;
    header.freecadVersionRevision = 0x000DEAD;
    header.osID = Base::CrashReporter::OS::Windows;
    header.architectureID = Base::CrashReporter::Architecture::x64;

    std::vector<char> stringTable;
    header.buildIDStringOffset = addStringToTable(stringTable, "R43210");
    header.exceptionMessageStringOffset = addStringToTable(stringTable, "A bad thing happened");
    header.freecadVersionSuffixStringOffset = addStringToTable(stringTable, "dev");
    header.minidumpPathStringOffset = Base::CrashReporter::NoString;

    header.frameCount = 4;
    std::vector<Base::CrashReporter::Frame> frames (header.frameCount);
    frames[0].rawAddress = 0x0000000011111111;
    frames[0].moduleOffset = 0xA100;
    frames[0].moduleStringOffset = addStringToTable(stringTable, "Module1");
    frames[1].rawAddress = 0x0000000022222222;
    frames[1].moduleOffset = 0xB100;
    frames[1].moduleStringOffset = frames[0].moduleStringOffset; // Also Module1, deduped
    frames[2].rawAddress = 0x0000000033333333;
    frames[2].moduleOffset = 0xC100;
    frames[2].moduleStringOffset = addStringToTable(stringTable, "Module2");
    frames[3].rawAddress = 0x0000000044444444;
    frames[3].moduleOffset = 0xD100;
    frames[3].moduleStringOffset = Base::CrashReporter::NoString;

    Base::CrashReporter::Footer footer;

    buffer.resize(sizeof(Base::CrashReporter::Header) +
        header.frameCount * sizeof(Base::CrashReporter::Frame) +
        stringTable.size() +
        sizeof(footer));

    header.frameTableOffset = sizeof(Base::CrashReporter::Header);
    header.stringTableOffset = header.frameTableOffset + header.frameCount * sizeof(Base::CrashReporter::Frame);
    header.fileSize = static_cast<std::uint32_t>(buffer.size());

    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data()+header.frameTableOffset, frames.data(), header.frameCount * sizeof(Base::CrashReporter::Frame));
    std::memcpy(buffer.data()+header.stringTableOffset, stringTable.data(), stringTable.size());

    footer.checksum = Base::CrashReporter::crc32(std::span<const char>(buffer.data(), buffer.size()-sizeof(footer)));
    std::memcpy(buffer.data()+header.stringTableOffset+stringTable.size(), &footer, sizeof(footer));

    return buffer;
}

template<typename t>
void overwriteAtOffset(std::vector<char> &buffer, std::size_t offset, const t &value)
{
    std::memcpy(buffer.data() + offset, &value, sizeof(value));
}

TEST_F(CrashReporterReaderTests, TestMissingFileThrows)
{
    EXPECT_THROW (
    auto _ = Base::CrashReporter::parse("/no/such/file.fcrash"),
    Base::FileException
    );
}

TEST_F(CrashReporterReaderTests, TestFileTooSmall)
{
    auto fcrashPath = tempDir.path() / "too_small.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        ofs.write("Bad", 3);
    }
    EXPECT_THROW(
    auto _ = Base::CrashReporter::parse(fcrashPath.string()),
    Base::BadFormatError
    );
}

TEST_F(CrashReporterReaderTests, TestFileTooLarge)
{
    auto fcrashPath = tempDir.path() / "too_large.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        constexpr std::size_t bufferSize = Base::CrashReporter::MaxFileSize+1;
        std::vector<char> buffer(bufferSize, 'x');
        ofs.write(buffer.data(), bufferSize);
    }
    EXPECT_THROW(
    auto _ = Base::CrashReporter::parse(fcrashPath.string()),
    Base::BadFormatError
    );
}

TEST_F(CrashReporterReaderTests, TestGoodCrashReportLoads)
{
    auto fcrashPath = tempDir.path() / "good_report.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        ofs.write(buffer.data(), buffer.size());
    }
    auto report = Base::CrashReporter::parse(fcrashPath.string());
    EXPECT_EQ(report.faultAddress, 0xDEADBEEFCAFEF00D);
    EXPECT_EQ(report.threadID, 0x1122334455667788);
    EXPECT_EQ(report.osID, Base::CrashReporter::OS::Windows);
    EXPECT_EQ(report.architectureID, Base::CrashReporter::Architecture::x64);
    EXPECT_FALSE(report.partialWrite);
    EXPECT_EQ(report.buildID, "R43210");
    EXPECT_EQ(report.exceptionMessage, "A bad thing happened");
    EXPECT_TRUE(report.minidumpPath.empty());  // NoString -> empty
    ASSERT_EQ(report.stackFrames.size(), 4U);
    EXPECT_EQ(report.stackFrames[0].rawAddress, 0x11111111U);
    EXPECT_EQ(report.stackFrames[0].modulePath, "Module1");
    EXPECT_EQ(report.stackFrames[1].modulePath, "Module1");  // deduped offset
    EXPECT_EQ(report.stackFrames[2].modulePath, "Module2");
    EXPECT_TRUE(report.stackFrames[3].modulePath.empty());  // NoString frame
}

TEST_F(CrashReporterReaderTests, TestBadMagicNumber)
{
    auto fcrashPath = tempDir.path() / "bad_magic_report.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        constexpr uint32_t badMagic = 0xDEADBEEF;
        overwriteAtOffset(buffer, offsetof(Base::CrashReporter::Header, magic), badMagic);
        ofs.write(buffer.data(), buffer.size());
    }
    EXPECT_THROW(
    auto _ = Base::CrashReporter::parse(fcrashPath.string()),
    Base::BadFormatError
    );
}

TEST_F(CrashReporterReaderTests, TestBadVersionNumber)
{
    auto fcrashPath = tempDir.path() / "bad_version_report.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        constexpr uint32_t badVersion = 0xDEADBEEF;
        overwriteAtOffset(buffer, offsetof(Base::CrashReporter::Header, version), badVersion);
        ofs.write(buffer.data(), buffer.size());
    }
    EXPECT_THROW(
    auto _ = Base::CrashReporter::parse(fcrashPath.string()),
    Base::BadFormatError
    );
}


TEST_F(CrashReporterReaderTests, TestBadFileSize)
{
    auto fcrashPath = tempDir.path() / "bad_file_size_report.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        constexpr uint32_t badFileSize = 16;  // Too small, can't be real
        overwriteAtOffset(buffer, offsetof(Base::CrashReporter::Header, fileSize), badFileSize);
        ofs.write(buffer.data(), buffer.size());
    }
    EXPECT_THROW(
    auto _ = Base::CrashReporter::parse(fcrashPath.string()),
    Base::BadFormatError
    );
}

TEST_F(CrashReporterReaderTests, TestBadStringTableOffset)
{
    auto fcrashPath = tempDir.path() / "bad_string_table_offset.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        overwriteAtOffset(buffer, offsetof(Base::CrashReporter::Header, stringTableOffset), buffer.size());
        ofs.write(buffer.data(), buffer.size());
    }
    EXPECT_THROW(
    auto _ = Base::CrashReporter::parse(fcrashPath.string()),
    Base::BadFormatError
    );
}

TEST_F(CrashReporterReaderTests, TestBadFrameCount)
{
    auto fcrashPath = tempDir.path() / "bad_frame_count.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        overwriteAtOffset(buffer, offsetof(Base::CrashReporter::Header, stringTableOffset), Base::CrashReporter::MaxFrames+1);
        ofs.write(buffer.data(), buffer.size());
    }
    EXPECT_THROW(
    auto _ = Base::CrashReporter::parse(fcrashPath.string()),
    Base::BadFormatError
    );
}

TEST_F(CrashReporterReaderTests, TestBadFrameTableOffset)
{
    auto fcrashPath = tempDir.path() / "bad_frame_table_offset.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        overwriteAtOffset(buffer, offsetof(Base::CrashReporter::Header, frameTableOffset), buffer.size());
        ofs.write(buffer.data(), buffer.size());
    }
    EXPECT_THROW(
    auto _ = Base::CrashReporter::parse(fcrashPath.string()),
    Base::BadFormatError
    );
}


TEST_F(CrashReporterReaderTests, TestBadCRC)
{
    auto fcrashPath = tempDir.path() / "bad_crc.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        overwriteAtOffset(buffer, buffer.size() - sizeof(Base::CrashReporter::Footer), static_cast<std::uint32_t>(0));
        ofs.write(buffer.data(), buffer.size());
    }
    auto parsedReport = Base::CrashReporter::parse(fcrashPath.string());
    // A bad checksum means the write got interrupted: this is somewhat recoverable, so it doesn't
    // throw, instead it just sets the partialWrite flag.
    EXPECT_TRUE(parsedReport.partialWrite);
}