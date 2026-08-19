// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD project association AISBL
// SPDX-FileNotice: Part of the FreeCAD project.

#include <gtest/gtest.h>

#include <Base/CrashReporter/FaultCodes.h>
#include <Base/CrashReporter/Reader.h>
#include <Base/CrashReporter/Writer.h>
#include <Base/CrashReporter/Manager.h>
#include <Build/Version.h>
#include <src/TempDirectory.h>

#include <algorithm>
#include <cstring>  // IWYU pragma: keep
#include <fstream>

#ifdef FC_OS_WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# include <windows.h>
# include <cstdlib>  // IWYU pragma: keep
# include <Base/CrashReporter/WindowsCrashReporter.h>
#else
# include <csignal>
# include <sys/resource.h>
#endif

/// For tests that need a fault that specifically contains an address
constexpr std::uint32_t syntheticFaultCodeWithAddress()
{
#ifdef FC_OS_WIN32
    return EXCEPTION_ACCESS_VIOLATION;
#else
    return SIGSEGV;
#endif
}

/// The reverse of the above: a code that does *not* contain an address
constexpr std::uint32_t syntheticFaultCodeWithoutAddress()
{
#ifdef FC_OS_WIN32
    return EXCEPTION_STACK_OVERFLOW;
#else
    return SIGABRT;
#endif
}

/// A check for the above: not general, specific to this test suite -- all tests below use this set
/// of three utility methods to manage this bit of machinery.
constexpr bool isAddressCarryingFaultCode(std::uint32_t code)
{
#ifdef FC_OS_WIN32
    return code == syntheticFaultCodeWithAddress();
#else
    return code == syntheticFaultCodeWithAddress() || code == SIGBUS;
#endif
}

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-vararg,hicpp-vararg,cppcoreguidelines-owning-memory,cppcoreguidelines-avoid-non-const-global-variables,cert-err58-cpp)

class CrashReporterTests: public testing::Test
{
public:
    static std::vector<char> createGoodCrashReport(
        std::uint32_t faultCode = syntheticFaultCodeWithAddress()
    );

    /// Sometimes we need a name that can actually be parsed (for example, to test `scan()`). This
    /// creates an on-disk file with a name that the code internals can parse.
    static std::string placeReport(
        const std::filesystem::path& dir,
        std::int64_t ts,
        int pid,
        const std::vector<char>& buffer
    );

    /// Create <dir>/archive/crash-<ts>-<pid>.fcrash (+ optional .dmp). Content irrelevant.
    static void placeArchivedFile(
        const std::filesystem::path& dir,
        std::int64_t ts,
        int pid,
        bool withDump = false
    );

    /// Check for the existence of archived fcrash with this timestamp and PID
    static bool archivedFcrashExists(const std::filesystem::path& dir, std::int64_t ts, int pid);

    /// Check for the existence of archived minidump with this timestamp and PID
    static bool archivedDumpExists(const std::filesystem::path& dir, std::int64_t ts, int pid);

protected:
    tests::TempDirectory tempDir {"crash_reader"};
};

uint32_t addStringToTable(std::vector<char>& stringTable, const std::string& string)
{
    const uint32_t offset = stringTable.size();
    stringTable.resize(offset + sizeof(uint16_t) + string.length());
    const uint16_t length {static_cast<uint16_t>(string.length())};
    std::memcpy(stringTable.data() + offset, &length, sizeof(uint16_t));
    std::memcpy(stringTable.data() + offset + sizeof(uint16_t), string.data(), length);
    return offset;
}

std::vector<char> CrashReporterTests::createGoodCrashReport(std::uint32_t faultCode)
{
    std::vector<char> buffer;

    Base::CrashReporter::Header header;

    // Always planted, whatever the fault code
    header.faultAddress = 0xDEADBEEFCAFEF00D;

    header.threadID = 0x1122334455667788;
    header.timestamp = 1700000000;
    header.processID = 0xABCD;
    header.code = faultCode;
    header.freecadVersionMajor = 5;
    header.freecadVersionMinor = 6;
    header.freecadVersionPatch = 7;
#ifdef FC_OS_MACOSX
    header.osID = Base::CrashReporter::OS::macOS;
#elif defined(FC_OS_WIN32)
    header.osID = Base::CrashReporter::OS::Windows;
#elif defined(FC_OS_LINUX)
    header.osID = Base::CrashReporter::OS::Linux;
#elif defined(FC_OS_BSD)
    header.osID = Base::CrashReporter::OS::BSDFamily;
#endif
    header.architectureID = Base::CrashReporter::Architecture::x64;

    std::vector<char> stringTable;
#ifdef FCRepositoryHash
    header.buildIDStringOffset = addStringToTable(stringTable, FCRepositoryHash);
#else
    header.buildIDStringOffset = addStringToTable(stringTable, "R43210");
#endif
    header.freecadVersionSuffixStringOffset = addStringToTable(stringTable, "dev");
    header.minidumpPathStringOffset = Base::CrashReporter::NoString;

    header.frameCount = 4;
    std::vector<Base::CrashReporter::Frame> frames(header.frameCount);
    frames.at(0).rawAddress = 0x0000000011111111;
    frames.at(0).moduleOffset = 0xA100;
    frames.at(0).moduleStringOffset = addStringToTable(stringTable, "Module1");
    frames.at(1).rawAddress = 0x0000000022222222;
    frames.at(1).moduleOffset = 0xB100;
    frames.at(1).moduleStringOffset = frames.at(0).moduleStringOffset;  // Also Module1, deduped
    frames.at(2).rawAddress = 0x0000000033333333;
    frames.at(2).moduleOffset = 0xC100;
    frames.at(2).moduleStringOffset = addStringToTable(stringTable, "Module2");
    frames.at(3).rawAddress = 0x0000000044444444;
    frames.at(3).moduleOffset = 0xD100;
    frames.at(3).moduleStringOffset = Base::CrashReporter::NoString;

    Base::CrashReporter::Footer footer;

    buffer.resize(
        sizeof(Base::CrashReporter::Header) + (header.frameCount * sizeof(Base::CrashReporter::Frame))
        + stringTable.size() + sizeof(footer)
    );

    header.frameTableOffset = sizeof(Base::CrashReporter::Header);
    header.stringTableOffset = header.frameTableOffset
        + (header.frameCount * sizeof(Base::CrashReporter::Frame));
    header.fileSize = static_cast<std::uint32_t>(buffer.size());

    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(
        buffer.data() + header.frameTableOffset,
        frames.data(),
        header.frameCount * sizeof(Base::CrashReporter::Frame)
    );
    std::memcpy(buffer.data() + header.stringTableOffset, stringTable.data(), stringTable.size());

    footer.checksum = Base::CrashReporter::crc32(
        std::span<const char>(buffer.data(), buffer.size() - sizeof(footer))
    );
    std::memcpy(buffer.data() + header.stringTableOffset + stringTable.size(), &footer, sizeof(footer));

    return buffer;
}

std::string CrashReporterTests::placeReport(
    const std::filesystem::path& dir,
    std::int64_t ts,
    int pid,
    const std::vector<char>& buffer
)
{
    const auto path = dir / ("crash-" + std::to_string(ts) + "-" + std::to_string(pid) + ".fcrash");
    std::ofstream ofs(path, std::ios::binary);
    ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    return Base::FileInfo::pathToString(path);
}

void CrashReporterTests::placeArchivedFile(
    const std::filesystem::path& dir,
    std::int64_t ts,
    int pid,
    bool withDump
)
{
    const auto archiveDir = dir / "archive";
    std::filesystem::create_directories(archiveDir);
    const auto path = archiveDir
        / ("crash-" + std::to_string(ts) + "-" + std::to_string(pid) + ".fcrash");
    std::ofstream fcrash(path, std::ios::binary);
    if (withDump) {
        const auto minidump = archiveDir
            / ("crash-" + std::to_string(ts) + "-" + std::to_string(pid) + ".dmp");
        std::ofstream dmp(minidump, std::ios::binary);
    }
}

bool CrashReporterTests::archivedFcrashExists(const std::filesystem::path& dir, std::int64_t ts, int pid)
{
    const auto path = dir / "archive"
        / ("crash-" + std::to_string(ts) + "-" + std::to_string(pid) + ".fcrash");
    return Base::FileInfo(Base::FileInfo::pathToString(path)).exists();
}

bool CrashReporterTests::archivedDumpExists(const std::filesystem::path& dir, std::int64_t ts, int pid)
{
    const auto path = dir / "archive"
        / ("crash-" + std::to_string(ts) + "-" + std::to_string(pid) + ".dmp");
    return Base::FileInfo(Base::FileInfo::pathToString(path)).exists();
}

template<typename t>
void overwriteAtOffset(std::vector<char>& buffer, std::size_t offset, const t& value)
{
    std::memcpy(buffer.data() + offset, &value, sizeof(value));
}

TEST_F(CrashReporterTests, parseMissingFileThrows)  // NOLINT
{
    // NOLINTNEXTLINE
    EXPECT_THROW(auto _ = Base::CrashReporter::parse("/no/such/file.fcrash"), Base::FileException);
}

TEST_F(CrashReporterTests, parseFileTooSmall)  // NOLINT
{
    const auto fcrashPath = tempDir.path() / "too_small.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        ofs.write("Bad", 3);
    }
    // NOLINTNEXTLINE
    EXPECT_THROW(auto _ = Base::CrashReporter::parse(fcrashPath.string()), Base::BadFormatError);
}

TEST_F(CrashReporterTests, parseFileTooLarge)  // NOLINT
{
    auto fcrashPath = tempDir.path() / "too_large.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        constexpr std::size_t bufferSize = Base::CrashReporter::MaxFileSize + 1;
        std::vector buffer(bufferSize, 'x');
        ofs.write(buffer.data(), bufferSize);
    }
    // NOLINTNEXTLINE
    EXPECT_THROW(auto _ = Base::CrashReporter::parse(fcrashPath.string()), Base::BadFormatError);
}

TEST_F(CrashReporterTests, parseGoodCrashReportLoads)  // NOLINT
{
    auto fcrashPath = tempDir.path() / "good_report.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }
    auto report = Base::CrashReporter::parse(fcrashPath.string());

    // On all platforms we've specifically chosen a fault code that has an address, so make sure
    // it's there:
    EXPECT_EQ(report.faultAddress, 0xDEADBEEFCAFEF00D);
    EXPECT_EQ(report.threadID, 0x1122334455667788);

#ifdef FC_OS_MACOSX
    EXPECT_EQ(report.osID, Base::CrashReporter::OS::macOS);
#elif defined(FC_OS_WIN32)
    EXPECT_EQ(report.osID, Base::CrashReporter::OS::Windows);
#elif defined(FC_OS_LINUX)
    EXPECT_EQ(report.osID, Base::CrashReporter::OS::Linux);
#elif defined(FC_OS_BSD)
    EXPECT_EQ(report.osID, Base::CrashReporter::OS::BSDFamily);
#endif

    EXPECT_EQ(report.architectureID, Base::CrashReporter::Architecture::x64);
    EXPECT_FALSE(report.partialWrite);
#ifdef FCRepositoryHash
    EXPECT_EQ(report.buildID, std::string(FCRepositoryHash));
#else
    EXPECT_EQ(report.buildID, "R43210");
#endif
    EXPECT_FALSE(report.minidumpPath.has_value());
    ASSERT_EQ(report.stackFrames.size(), 4U);
    EXPECT_EQ(report.stackFrames.at(0).rawAddress, 0x11111111U);
    EXPECT_EQ(report.stackFrames.at(0).modulePath, "Module1");
    EXPECT_EQ(report.stackFrames.at(1).modulePath, "Module1");  // deduped offset
    EXPECT_EQ(report.stackFrames.at(2).modulePath, "Module2");
    EXPECT_TRUE(report.stackFrames.at(3).modulePath.empty());  // NoString frame
}

TEST_F(CrashReporterTests, parseDropsFaultAddressForCodeThatHasNone)  // NOLINT
{
    auto fcrashPath = tempDir.path() / "no_fault_address.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport(syntheticFaultCodeWithoutAddress());
        ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }
    auto report = Base::CrashReporter::parse(fcrashPath.string());

    // The record carries 0xDEADBEEFCAFEF00D like every other fixture, but this fault code has no
    // faulting data address, so the Reader must drop it rather than pass it on.
    EXPECT_FALSE(report.faultAddress.has_value());
    EXPECT_EQ(report.code, syntheticFaultCodeWithoutAddress());

    // The record is well-formed: everything else must still parse.
    EXPECT_FALSE(report.partialWrite);
    EXPECT_EQ(report.threadID, 0x1122334455667788);
    EXPECT_EQ(report.processID, 0xABCDU);
    ASSERT_EQ(report.stackFrames.size(), 4U);
}

TEST_F(CrashReporterTests, parseNamesUnrecognizedFaultCodeWithItsHexValue)  // NOLINT
{
    // Not a valid fault code on any platform we support, so it cannot be in the lookup table.
    constexpr std::uint32_t unrecognizedCode = std::numeric_limits<std::uint32_t>::max();

    auto fcrashPath = tempDir.path() / "unrecognized_fault_code.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport(unrecognizedCode);
        ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }
    auto report = Base::CrashReporter::parse(fcrashPath.string());

    // The exact spelling is deliberately pinned: downstream consumers might be doing something with
    // this, don't change it without good cause. (The address doesn't matter, but the form does)
    EXPECT_EQ(report.faultName, "UNKNOWN(0xFFFFFFFF)");

    // Nothing is known about the code, so nothing can be claimed about its address either.
    EXPECT_FALSE(report.faultAddress.has_value());

    // The record itself is well-formed: only the code is unrecognized.
    EXPECT_FALSE(report.partialWrite);
    EXPECT_EQ(report.code, unrecognizedCode);
    ASSERT_EQ(report.stackFrames.size(), 4U);
}

TEST_F(CrashReporterTests, parseBadMagicNumber)  // NOLINT
{
    auto fcrashPath = tempDir.path() / "bad_magic_report.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        constexpr uint32_t badMagic = 0xDEADBEEF;
        overwriteAtOffset(buffer, offsetof(Base::CrashReporter::Header, magic), badMagic);
        ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }
    // NOLINTNEXTLINE
    EXPECT_THROW(auto _ = Base::CrashReporter::parse(fcrashPath.string()), Base::BadFormatError);
}

TEST_F(CrashReporterTests, parseBadVersionNumber)  // NOLINT
{
    auto fcrashPath = tempDir.path() / "bad_version_report.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        constexpr uint32_t badVersion = 0xDEADBEEF;
        overwriteAtOffset(buffer, offsetof(Base::CrashReporter::Header, version), badVersion);
        ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }
    // NOLINTNEXTLINE
    EXPECT_THROW(auto _ = Base::CrashReporter::parse(fcrashPath.string()), Base::BadFormatError);
}


TEST_F(CrashReporterTests, parseBadFileSize)  // NOLINT
{
    auto fcrashPath = tempDir.path() / "bad_file_size_report.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        constexpr uint32_t badFileSize = 16;  // Too small, can't be real
        overwriteAtOffset(buffer, offsetof(Base::CrashReporter::Header, fileSize), badFileSize);
        ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }
    // NOLINTNEXTLINE
    EXPECT_THROW(auto _ = Base::CrashReporter::parse(fcrashPath.string()), Base::BadFormatError);
}

TEST_F(CrashReporterTests, parseBadStringTableOffset)  // NOLINT
{
    auto fcrashPath = tempDir.path() / "bad_string_table_offset.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        overwriteAtOffset(
            buffer,
            offsetof(Base::CrashReporter::Header, stringTableOffset),
            static_cast<std::uint32_t>(buffer.size())
        );
        ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }
    // NOLINTNEXTLINE
    EXPECT_THROW(auto _ = Base::CrashReporter::parse(fcrashPath.string()), Base::BadFormatError);
}

TEST_F(CrashReporterTests, parseBadFrameCount)  // NOLINT
{
    auto fcrashPath = tempDir.path() / "bad_frame_count.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        overwriteAtOffset(
            buffer,
            offsetof(Base::CrashReporter::Header, frameCount),
            Base::CrashReporter::MaxFrames + 1
        );
        ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }
    // NOLINTNEXTLINE
    EXPECT_THROW(auto _ = Base::CrashReporter::parse(fcrashPath.string()), Base::BadFormatError);
}

TEST_F(CrashReporterTests, parseBadFrameTableOffset)  // NOLINT
{
    auto fcrashPath = tempDir.path() / "bad_frame_table_offset.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        overwriteAtOffset(
            buffer,
            offsetof(Base::CrashReporter::Header, frameTableOffset),
            static_cast<std::uint32_t>(buffer.size())
        );
        ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }
    // NOLINTNEXTLINE
    EXPECT_THROW(auto _ = Base::CrashReporter::parse(fcrashPath.string()), Base::BadFormatError);
}


TEST_F(CrashReporterTests, parseBadCRC)  // NOLINT
{
    const auto fcrashPath = tempDir.path() / "bad_crc.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        auto buffer = createGoodCrashReport();
        overwriteAtOffset(
            buffer,
            buffer.size() - sizeof(Base::CrashReporter::Footer),
            static_cast<std::uint32_t>(0)
        );
        ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }
    const auto parsedReport = Base::CrashReporter::parse(fcrashPath.string());
    // A bad checksum means the write got interrupted: this is somewhat recoverable, so it doesn't
    // throw, instead it just sets the partialWrite flag.
    EXPECT_TRUE(parsedReport.partialWrite);
}

TEST_F(CrashReporterTests, matchingHashAttemptsSymbolication)  // NOLINT
{
    const auto fcrashPath = tempDir.path() / "matching_hash.fcrash";
    {
        std::ofstream ofs(fcrashPath, std::ios::binary);
        const auto buffer = createGoodCrashReport();
        ofs.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }

#if defined(FCRepositoryHash) && defined(FC_HAVE_CPPTRACE)
    constexpr bool expectedResult = true;
#else
    constexpr bool expectedResult = false;
#endif
    const auto parsedReport = Base::CrashReporter::parse(fcrashPath.string());
    EXPECT_EQ(parsedReport.symbolicated, expectedResult);
}

/// Given a directory that contains a single *.fcrash file, return the complete path to that file
std::string findSoleFcrashIn(const std::string& path)
{
    if (const Base::FileInfo info(path); info.isDir()) {
        for (const auto content = info.getDirectoryContent(); const auto& item : content) {
            if (item.isFile() && item.extension() == "fcrash") {
                return item.filePath();
            }
        }
    }
    return {};
}

// A chunk of code that will crash reliably, and prevent any kind of inlining or other sort of
// optimization to ensure that the crash site is clearly recognizable in the backtrace.
#ifdef _MSC_VER
# define FC_NOINLINE __declspec(noinline)
#else
# define FC_NOINLINE __attribute__((noinline))
#endif
// The target is loaded through a volatile pointer so that the compiler cannot see a literal null.
// Given one, GCC at -O2 proves the store is unreachable UB and deletes the call to this function,
// leaving the release build with nothing to crash on.
static int* volatile deliberateNullTarget = nullptr;

extern "C" FC_NOINLINE void crashReporterFaultSite()
{
    volatile int* p = deliberateNullTarget;
    *p = 13;
    std::atomic_signal_fence(std::memory_order_seq_cst);  // defeat tail-call/reorder
}

static std::string resolveCrashDir(const tests::TempDirectory& tempDir)
{
    // getenv/setenv are the channel that carries the pinned crash dir across the death-test
    // process re-exec; this runs single-threaded in test setup, so the mt-unsafe race cannot occur.
    // NOLINTBEGIN(concurrency-mt-unsafe)
    if (const char* fromEnv = std::getenv("FC_CRASH_TEST_DIR")) {
        return fromEnv;
    }
    // NOLINTEND(concurrency-mt-unsafe)
    auto dir = (tempDir.path() / "CrashReports").string();
#ifdef _MSC_VER
    _putenv_s("FC_CRASH_TEST_DIR", dir.c_str());
#else
    setenv("FC_CRASH_TEST_DIR", dir.c_str(), 1);
#endif
    return dir;
}

static void installAndCrash(const std::string& crashDir)
{
#ifdef _MSC_VER
    SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS);
#else
    // These tests crash on purpose, several times per run on three platforms. Without this the
    // build directory (and /cores on macOS) collects dumps nobody asked for.
    const rlimit noCoreDumps {0, 0};
    setrlimit(RLIMIT_CORE, &noCoreDumps);
#endif
    Base::CrashReporter::Writer::prewarm();
    Base::CrashReporter::Writer::install(crashDir);
#ifdef _MSC_VER
    Base::CrashReporter::WindowsCrashReporter::install(crashDir);
#endif
    crashReporterFaultSite();
}

// Windows needs --gtest_catch_exceptions=0, because gtest's own SEH handler would otherwise eat
// the fault before SetUnhandledExceptionFilter sees it. That flag cannot be assumed in CI, so the
// assertion test runs automatically on POSIX only;
#ifdef _MSC_VER
# define FC_REAL_CAPTURE_TEST DISABLED_realCrashCapturesUsableFrames
#else
# define FC_REAL_CAPTURE_TEST realCrashCapturesUsableFrames
#endif

TEST_F(CrashReporterTests, FC_REAL_CAPTURE_TEST)  // NOLINT
{
    const std::string crashDir = resolveCrashDir(tempDir);
    EXPECT_DEATH(installAndCrash(crashDir), "");  // NOLINT

    const auto path = findSoleFcrashIn(crashDir);
    ASSERT_FALSE(path.empty());
    const auto report = Base::CrashReporter::parse(path);

    // The point of the whole exercise: a real crash has to produce a real stack.
    ASSERT_GT(report.stackFrames.size(), 0U);

    // Every frame must carry a genuine instruction address.
    for (const auto& frame : report.stackFrames) {
        EXPECT_NE(frame.rawAddress, 0U);
    }

    // At least one frame must be attributed to a module. Not all of them: an unattributable frame
    // is legitimate and is recorded with NoString
    EXPECT_TRUE(std::ranges::any_of(report.stackFrames, [](const auto& frame) {
        return !frame.modulePath.empty();
    }));

    // A symbol is a function name. When a frame resolves from a symbol table rather than from
    // debug information the symbolicator appends "+ <offset>", which would put a byte offset into
    // the key consumers group crashes by, so the Reader trims it.
    for (const auto& frame : report.stackFrames) {
        const auto plus = frame.symbol.rfind(" + ");
        const bool endsWithOffset = plus != std::string::npos
            && frame.symbol.find_first_not_of("0123456789", plus + 3) == std::string::npos;
        EXPECT_FALSE(endsWithOffset) << "symbol still carries an offset: " << frame.symbol;
    }

    // `file` is a source file. A frame with no debug information has none, and must not fall back
    // to naming the object it resolved against.
    for (const auto& frame : report.stackFrames) {
        if (!frame.file.empty()) {
            EXPECT_NE(frame.file, frame.modulePath);
        }
    }

    EXPECT_TRUE(isAddressCarryingFaultCode(report.code)) << "unexpected fault code: " << report.code;
    EXPECT_FALSE(report.partialWrite);
}

// This test is always disabled in CI runs: to run it, use a direct manual call:
// Base_tests_run.exe --gtest_also_run_disabled_tests \
//                    --gtest_catch_exceptions=0 \
//                    --gtest_filter=*DeliberateSegfaultRoundTrip*
TEST_F(CrashReporterTests, DISABLED_DeliberateSegfaultRoundTrip)  // NOLINT
{
    const std::string crashDir = resolveCrashDir(tempDir);
    EXPECT_DEATH(installAndCrash(crashDir), "");  // NOLINT

    const auto path = findSoleFcrashIn(crashDir);
    ASSERT_FALSE(path.empty());
    const auto report = Base::CrashReporter::parse(path);

    // CODE FOR STACK NAME DISCOVERY -- use to construct the "skip list" of calls we will
    // strip off the top of the stack before reporting the crash site. Anything above
    // `crashReporterFaultSite` should be added to the list of names to omit.
    //
    // Keep this code around: you can uncomment it to check out a new/different OS's call stack
    // to add those symbols to the skip-list
    for (std::size_t i = 0; i < report.stackFrames.size(); ++i) {
        std::printf("[%2zu] %s\n", i, report.stackFrames.at(i).symbol.value_or("<unresolved>").c_str());
    }
    std::fflush(stdout);
    // END DISCOVERY CODE
}


TEST_F(CrashReporterTests, trimLeadingPlumbingFramesLeavesNoPlumbingAlone)  // NOLINT
{
    const std::vector<Base::CrashReporter::ParsedFrame> frames = {
        {.symbol = "Frame1"},
        {.symbol = "Frame2"},
        {.symbol = "Frame3"},
    };
    const auto trimmed = Base::CrashReporter::trimLeadingPlumbingFrames(frames);
    EXPECT_EQ(frames.size(), trimmed.size());
}

TEST_F(CrashReporterTests, trimLeadingPlumbingFramesLeavesEmptyVectorAlone)  // NOLINT
{
    const std::vector<Base::CrashReporter::ParsedFrame> frames;
    const auto trimmed = Base::CrashReporter::trimLeadingPlumbingFrames(frames);
    EXPECT_TRUE(trimmed.empty());
}

TEST_F(CrashReporterTests, trimLeadingPlumbingFramesRemovesSingleExpectedSymbolFromTop)  // NOLINT
{
    const std::vector<Base::CrashReporter::ParsedFrame> frames = {
        {.symbol = "KiUserExceptionDispatcher"},
        {.symbol = "Frame2"},
        {.symbol = "Frame3"},
    };
    const auto trimmed = Base::CrashReporter::trimLeadingPlumbingFrames(frames);
    EXPECT_EQ(trimmed.size(), frames.size() - 1);
    EXPECT_EQ(trimmed.front().symbol, "Frame2");
}

TEST_F(CrashReporterTests, trimLeadingPlumbingFramesRemovesAnythingAboveFirstTrampoline)  // NOLINT
{
    const std::vector<Base::CrashReporter::ParsedFrame> frames = {
        {.symbol = "Something"},
        {.symbol = "SomethingElse"},
        {.symbol = "KiUserExceptionDispatcher"},
        {.symbol = "Frame2"},
        {.symbol = "Frame3"},
    };
    const auto trimmed = Base::CrashReporter::trimLeadingPlumbingFrames(frames);
    EXPECT_EQ(trimmed.size(), 2);  // Only keeps the last two frames
    EXPECT_EQ(trimmed.front().symbol, "Frame2");
}

TEST_F(CrashReporterTests, trimLeadingPlumbingFramesKeepsUnsymbolicatedTopFrame)  // NOLINT
{
    const std::vector<Base::CrashReporter::ParsedFrame> frames = {
        {.symbol = "__kernel_rt_sigreturn"},
        {.symbol = std::nullopt},  // unsymbolicated: treated as real
        {.symbol = "App::foo()"},
    };
    const auto trimmed = Base::CrashReporter::trimLeadingPlumbingFrames(frames);
    EXPECT_EQ(trimmed.size(), frames.size() - 1);
    EXPECT_FALSE(trimmed.front().symbol.has_value());  // Our unsymbolicated frame is still here
}


TEST_F(CrashReporterTests, trimLeadingPlumbingFramesKeepsAllWhenTrampolineIsDeepest)  // NOLINT
{
    const std::vector<Base::CrashReporter::ParsedFrame> frames = {
        {.symbol = "App::foo()"},
        {.symbol = "KiUserExceptionDispatcher"},  // anchor is last: nothing below to keep
    };
    const auto trimmed = Base::CrashReporter::trimLeadingPlumbingFrames(frames);
    EXPECT_EQ(trimmed.size(), frames.size());  // returned unchanged, NOT emptied
}


TEST_F(CrashReporterTests, scanDetectsAndArchivesGoodReport)  // NOLINT
{
    const auto buffer = createGoodCrashReport();
    const auto originalName = placeReport(tempDir.path(), 1700000000, 1234, buffer);
    Base::CrashReporter::Manager::scan(tempDir.string());

    const auto& reports = Base::CrashReporter::Manager::reports();
    ASSERT_EQ(reports.size(), 1U);

    EXPECT_FALSE(Base::FileInfo(originalName).exists());
    EXPECT_NE(reports.at(0).pathToRawReportFile.find("archive"), std::string::npos);
    EXPECT_TRUE(Base::FileInfo(reports.at(0).pathToRawReportFile).exists());
}

TEST_F(CrashReporterTests, scanDetectsAndRenamesBadReport)  // NOLINT
{
    auto buffer = createGoodCrashReport();
    constexpr uint32_t badMagic = 0xDEADBEEF;
    overwriteAtOffset(buffer, offsetof(Base::CrashReporter::Header, magic), badMagic);
    const auto originalName = placeReport(tempDir.path(), 1700000000, 1234, buffer);
    Base::CrashReporter::Manager::scan(tempDir.string());

    const auto& reports = Base::CrashReporter::Manager::reports();
    EXPECT_TRUE(reports.empty());
    EXPECT_FALSE(Base::FileInfo(originalName).exists());  // Should have been renamed *.corrupt
}

TEST_F(CrashReporterTests, scanCorrectlyHandlesMixOfGoodAndBadReports)  // NOLINT
{
    // Arrange
    auto buffer = createGoodCrashReport();
    const auto goodName = placeReport(tempDir.path(), 1700000000, 1234, buffer);
    constexpr uint32_t badMagic = 0xDEADBEEF;
    overwriteAtOffset(buffer, offsetof(Base::CrashReporter::Header, magic), badMagic);
    const auto badName = placeReport(tempDir.path(), 1700000001, 1235, buffer);

    // Act
    Base::CrashReporter::Manager::scan(tempDir.string());

    // Assert
    const auto& reports = Base::CrashReporter::Manager::reports();
    EXPECT_EQ(reports.size(), 1U);

    EXPECT_FALSE(Base::FileInfo(goodName).exists());  // Should have been archived
    EXPECT_FALSE(Base::FileInfo(badName).exists());   // Should have been renamed to *.corrupt
    EXPECT_NE(reports.at(0).pathToRawReportFile.find("archive"), std::string::npos);
    EXPECT_TRUE(Base::FileInfo(reports.at(0).pathToRawReportFile).exists());
}

TEST_F(CrashReporterTests, scanReportsIncompleteFiles)  // NOLINT
{
    // Arrange
    auto buffer = createGoodCrashReport();
    placeReport(tempDir.path(), 1700000000, 1234, buffer);
    overwriteAtOffset(
        buffer,
        buffer.size() - sizeof(Base::CrashReporter::Footer),
        static_cast<std::uint32_t>(0)
    );
    placeReport(tempDir.path(), 1700000001, 1235, buffer);

    // Act
    Base::CrashReporter::Manager::scan(tempDir.string());

    // Assert
    const auto& reports = Base::CrashReporter::Manager::reports();
    EXPECT_EQ(reports.size(), 2U);
    const auto partialWrites = std::ranges::count_if(reports, [](const auto& report) {
        return report.partialWrite;
    });
    EXPECT_EQ(partialWrites, 1U);
}

TEST_F(CrashReporterTests, clearRemovesEverything)  // NOLINT
{
    // Arrange
    const auto buffer = createGoodCrashReport();
    placeReport(tempDir.path(), 1700000000, 1234, buffer);
    placeReport(tempDir.path(), 1700000001, 1235, buffer);
    Base::CrashReporter::Manager::scan(tempDir.string());
    const auto& reports = Base::CrashReporter::Manager::reports();
    ASSERT_EQ(reports.size(), 2U);
    const auto filename1 = reports.at(0).pathToRawReportFile;  // Make sure to *copy* the name
    const auto filename2 = reports.at(1).pathToRawReportFile;

    // Act
    Base::CrashReporter::Manager::clear();

    // Assert
    EXPECT_FALSE(Base::FileInfo(filename1).exists());
    EXPECT_FALSE(Base::FileInfo(filename2).exists());
    EXPECT_TRUE(reports.empty());                           // This is a live reference
    EXPECT_TRUE(Base::FileInfo(tempDir.string()).isDir());  // Make sure it got recreated
}

TEST_F(CrashReporterTests, clearBeforeScanDoesNothing)  // NOLINT
{
    Base::CrashReporter::Manager::clear();  // Doesn't crash, throw, etc.
}

TEST_F(CrashReporterTests, scanSetsOSVersionWhenPresent)  // NOLINT
{
    // Arrange
    const auto buffer = createGoodCrashReport();
    placeReport(tempDir.path(), 1700000000, 1234, buffer);

    // Act
    Base::CrashReporter::Manager::scan(tempDir.string(), {}, "The best version");
    const auto& reports = Base::CrashReporter::Manager::reports();
    ASSERT_EQ(reports.size(), 1U);

    // Assert
    const auto osVersion = reports.at(0).osVersion;
    EXPECT_EQ(osVersion, "The best version");
}

TEST_F(CrashReporterTests, scanDoesNotSetOSVersionWhenNotPresent)  // NOLINT
{
    // Arrange
    const auto buffer = createGoodCrashReport();
    placeReport(tempDir.path(), 1700000000, 1234, buffer);

    // Act
    Base::CrashReporter::Manager::
        scan(tempDir.string(), {} /*, No version string set here: defaults to an empty optional*/);
    const auto& reports = Base::CrashReporter::Manager::reports();
    ASSERT_EQ(reports.size(), 1U);

    // Assert
    const auto osVersion = reports.at(0).osVersion;
    EXPECT_FALSE(osVersion.has_value());
}


namespace
{
constexpr std::int64_t secondsPerDay = 86'400;

std::int64_t nowTimestamp()
{
    namespace ch = std::chrono;
    return ch::duration_cast<ch::seconds>(ch::system_clock::now().time_since_epoch()).count();
}

}  // namespace

TEST_F(CrashReporterTests, retentionKeepsAllWhenUnderCountAndYoung)  // NOLINT
{
    // Arrange
    const auto now = nowTimestamp();
    placeArchivedFile(tempDir.path(), now, 1234);
    placeArchivedFile(tempDir.path(), now - (1 * secondsPerDay), 1235);
    placeArchivedFile(tempDir.path(), now - (2 * secondsPerDay), 1236);

    // Act (retention is part of the scan process)
    Base::CrashReporter::Manager::scan(
        tempDir.string(),
        Base::CrashReporter::RetentionPolicy {.maxReports = 3, .maxAge = std::chrono::days {30}}
    );

    // Assert (all still exist, nothing hits outside the retention window)
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now, 1234));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (1 * secondsPerDay), 1235));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (2 * secondsPerDay), 1236));
}

TEST_F(CrashReporterTests, retentionDeletesOldestBeyondCount)  // NOLINT
{
    // Arrange
    const auto now = nowTimestamp();
    placeArchivedFile(tempDir.path(), now - (100 * secondsPerDay), 1234);
    placeArchivedFile(tempDir.path(), now - (200 * secondsPerDay), 1235);
    placeArchivedFile(tempDir.path(), now - (300 * secondsPerDay), 1236);
    placeArchivedFile(tempDir.path(), now - (400 * secondsPerDay), 1237);
    placeArchivedFile(tempDir.path(), now - (500 * secondsPerDay), 1238);

    // Act (retention is part of the scan process)
    Base::CrashReporter::Manager::scan(
        tempDir.string(),
        Base::CrashReporter::RetentionPolicy {.maxReports = 3, .maxAge = std::chrono::days {30}}
    );

    // Assert (should delete the oldest, leaving the newest three)
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (100 * secondsPerDay), 1234));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (200 * secondsPerDay), 1235));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (300 * secondsPerDay), 1236));
    EXPECT_FALSE(archivedFcrashExists(tempDir.path(), now - (400 * secondsPerDay), 1237));
    EXPECT_FALSE(archivedFcrashExists(tempDir.path(), now - (500 * secondsPerDay), 1238));
}

TEST_F(CrashReporterTests, retentionKeepsYoungBeyondCount)  // NOLINT
{
    // Arrange
    const auto now = nowTimestamp();
    placeArchivedFile(tempDir.path(), now - (1 * secondsPerDay), 1234);
    placeArchivedFile(tempDir.path(), now - (2 * secondsPerDay), 1235);
    placeArchivedFile(tempDir.path(), now - (3 * secondsPerDay), 1236);
    placeArchivedFile(tempDir.path(), now - (4 * secondsPerDay), 1237);
    placeArchivedFile(tempDir.path(), now - (5 * secondsPerDay), 1238);

    // Act (retention is part of the scan process)
    Base::CrashReporter::Manager::scan(
        tempDir.string(),
        Base::CrashReporter::RetentionPolicy {.maxReports = 3, .maxAge = std::chrono::days {30}}
    );

    // Assert (should not delete anything, they are all young)
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (1 * secondsPerDay), 1234));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (2 * secondsPerDay), 1235));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (3 * secondsPerDay), 1236));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (4 * secondsPerDay), 1237));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (5 * secondsPerDay), 1238));
}

TEST_F(CrashReporterTests, retentionKeepsOldWithinCount)  // NOLINT
{
    // Arrange
    const auto now = nowTimestamp();
    placeArchivedFile(tempDir.path(), now - (100 * secondsPerDay), 1234);
    placeArchivedFile(tempDir.path(), now - (200 * secondsPerDay), 1235);
    placeArchivedFile(tempDir.path(), now - (300 * secondsPerDay), 1236);
    placeArchivedFile(tempDir.path(), now - (400 * secondsPerDay), 1237);
    placeArchivedFile(tempDir.path(), now - (500 * secondsPerDay), 1238);

    // Act (retention is part of the scan process)
    Base::CrashReporter::Manager::scan(
        tempDir.string(),
        Base::CrashReporter::RetentionPolicy {.maxReports = 5, .maxAge = std::chrono::days {30}}
    );

    // Assert (should not delete anything, they are all under the count)
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (100 * secondsPerDay), 1234));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (200 * secondsPerDay), 1235));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (300 * secondsPerDay), 1236));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (400 * secondsPerDay), 1237));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (500 * secondsPerDay), 1238));
}

TEST_F(CrashReporterTests, retentionDeletesOnlyOldAndBeyondCount)  // NOLINT
{
    // Arrange
    const auto now = nowTimestamp();
    placeArchivedFile(tempDir.path(), now - (1 * secondsPerDay), 1234);
    placeArchivedFile(tempDir.path(), now - (2 * secondsPerDay), 1235);
    placeArchivedFile(tempDir.path(), now - (3 * secondsPerDay), 1236);
    placeArchivedFile(tempDir.path(), now - (4 * secondsPerDay), 1237);
    placeArchivedFile(tempDir.path(), now - (500 * secondsPerDay), 1238);

    // Act (retention is part of the scan process)
    Base::CrashReporter::Manager::scan(
        tempDir.string(),
        Base::CrashReporter::RetentionPolicy {.maxReports = 3, .maxAge = std::chrono::days {30}}
    );

    // Assert (should only delete the one that is both old and outside the count)
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (1 * secondsPerDay), 1234));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (2 * secondsPerDay), 1235));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (3 * secondsPerDay), 1236));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (4 * secondsPerDay), 1237));
    EXPECT_FALSE(archivedFcrashExists(tempDir.path(), now - (500 * secondsPerDay), 1238));
}

TEST_F(CrashReporterTests, retentionDeletesCompanionDump)  // NOLINT
{
    // Arrange
    const auto now = nowTimestamp();
    placeArchivedFile(tempDir.path(), now, 1234);
    placeArchivedFile(tempDir.path(), now - (100 * secondsPerDay), 1235);
    placeArchivedFile(tempDir.path(), now - (200 * secondsPerDay), 1236, true);

    // Act (retention is part of the scan process)
    Base::CrashReporter::Manager::scan(
        tempDir.string(),
        Base::CrashReporter::RetentionPolicy {.maxReports = 2, .maxAge = std::chrono::days {30}}
    );

    // Assert (all still exist, nothing hits outside the retention window)
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now, 1234));
    EXPECT_TRUE(archivedFcrashExists(tempDir.path(), now - (100 * secondsPerDay), 1235));
    EXPECT_FALSE(archivedFcrashExists(tempDir.path(), now - (200 * secondsPerDay), 1236));
    EXPECT_FALSE(archivedDumpExists(tempDir.path(), now - (200 * secondsPerDay), 1236));
}

TEST_F(CrashReporterTests, faultAddressIsMeaningfulReturnsFalseForCodeWithoutAddress)  // NOLINT
{
    EXPECT_FALSE(Base::CrashReporter::faultAddressIsMeaningful(syntheticFaultCodeWithoutAddress()));
}

TEST_F(CrashReporterTests, faultAddressIsMeaningfulReturnsTrueForCodeWithAddress)  // NOLINT
{
    // Spot check (don't bother with an exhaustive test, it's tautological)
    EXPECT_TRUE(Base::CrashReporter::faultAddressIsMeaningful(syntheticFaultCodeWithAddress()));
}

TEST_F(CrashReporterTests, describeFaultCodeGivesValidResultForValidCode)  // NOLINT
{
    // Spot check (don't bother with an exhaustive test, it's tautological)
    auto description = Base::CrashReporter::describeFaultCode(syntheticFaultCodeWithAddress());
    ASSERT_NE(description, std::nullopt);
    EXPECT_FALSE(description->name.empty());
}

TEST_F(CrashReporterTests, describeFaultCodeGivesNulloptForBadCode)  // NOLINT
{
    auto description = Base::CrashReporter::describeFaultCode(
        std::numeric_limits<std::uint32_t>::max()
    );
    EXPECT_EQ(description, std::nullopt);
}


// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-vararg,hicpp-vararg,cppcoreguidelines-owning-memory,cppcoreguidelines-avoid-non-const-global-variables,cert-err58-cpp)
