#include <gtest/gtest.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Base/TimeInfo.h>
#include <src/TempDirectory.h>

class FileInfoTest: public ::testing::Test
{
protected:
    FileInfoTest()
    {
        tmp.setFile(tempDir.string());

        file.setFile(tmp.filePath() + "/test.txt");
        dir.setFile(tmp.filePath() + "/subdir");
    }

    void SetUp() override
    {
        Base::ofstream str(file, std::ios::out);
        str << "Test\n";
        str.close();
        dir.createDirectory();
    }

    void TearDown() override
    {
        EXPECT_TRUE(file.deleteFile());
        EXPECT_TRUE(dir.deleteDirectory());
    }

protected:
    tests::TempDirectory tempDir {"fctest"};
    Base::FileInfo tmp;
    Base::FileInfo file;
    Base::FileInfo dir;
};

TEST_F(FileInfoTest, TestDirectoryPath)
{
    Base::FileInfo relfile("nofile.txt");
    relfile.setFile(relfile.dirPath());
    EXPECT_TRUE(relfile.exists());
}

TEST_F(FileInfoTest, TestExistsDirectory)
{
    EXPECT_TRUE(tmp.exists());
}

TEST_F(FileInfoTest, TestCreateDirectory)
{
    Base::FileInfo path;
    path.setFile(tmp.filePath() + "/subdir1/subdir2");
    EXPECT_FALSE(path.createDirectory());
}

TEST_F(FileInfoTest, TestDirectoryContent)
{
    auto content = tmp.getDirectoryContent();
    EXPECT_EQ(content.size(), 2);
    EXPECT_TRUE(content[0].exists());
    EXPECT_TRUE(content[1].exists());
}

TEST_F(FileInfoTest, TestCheckPermission)
{
    EXPECT_TRUE(file.isReadable());
    EXPECT_TRUE(file.isWritable());
}

TEST_F(FileInfoTest, TestCheckNoPermission)
{
    std::string path = tmp.filePath();
    Base::FileInfo nofile(path + "/nofile");
    EXPECT_FALSE(nofile.isReadable());
    EXPECT_FALSE(nofile.isWritable());
}

TEST_F(FileInfoTest, TestSetPermission)
{
    file.setPermissions(Base::FileInfo::ReadOnly);
    EXPECT_TRUE(file.isReadable());
    EXPECT_FALSE(file.isWritable());

#ifndef _WIN32
    // Windows ACLs do not support write-only files: removing read permission has no effect.
    file.setPermissions(Base::FileInfo::WriteOnly);
    EXPECT_FALSE(file.isReadable());
    EXPECT_TRUE(file.isWritable());
#endif

    file.setPermissions(Base::FileInfo::ReadWrite);
    EXPECT_TRUE(file.isReadable());
    EXPECT_TRUE(file.isWritable());
}

TEST_F(FileInfoTest, TestCheckFile)
{
    EXPECT_TRUE(file.isFile());
    EXPECT_FALSE(dir.isFile());

    std::string path = tmp.filePath();
    Base::FileInfo file2(path + "/file2");
    EXPECT_TRUE(file2.isFile());
}

TEST_F(FileInfoTest, TestCheckDirectory)
{
    EXPECT_FALSE(file.isDir());
    EXPECT_TRUE(dir.isDir());

    std::string path = tmp.filePath();
    Base::FileInfo file2(path + "/file2");
    EXPECT_FALSE(file2.isDir());
}

TEST_F(FileInfoTest, TestSize)
{
#ifdef _WIN32
    // Text mode writes \r\n on Windows, so "Test\n" becomes 6 bytes.
    EXPECT_EQ(file.size(), 6);
#else
    EXPECT_EQ(file.size(), 5);
#endif
}

TEST_F(FileInfoTest, TestLastModified)
{
    EXPECT_FALSE(file.lastModified().isNull());

    std::string path = tmp.filePath();
    Base::FileInfo nofile(path + "/nofile.txt");
    EXPECT_TRUE(nofile.lastModified().isNull());
}

TEST_F(FileInfoTest, TestDeleteFile)
{
    std::string path = tmp.filePath();
    Base::FileInfo file2(path + "/nofile.txt");
    EXPECT_FALSE(file2.deleteFile());
}

TEST_F(FileInfoTest, TestRenameFile)
{
    std::string path = tmp.filePath();
    Base::FileInfo file2(path + "/file2");
    EXPECT_FALSE(file2.renameFile((path + "/file3").c_str()));
    EXPECT_TRUE(file.renameFile((path + "/file2").c_str()));
}

TEST_F(FileInfoTest, TestCopyFile)
{
    std::string path = tmp.filePath();
    Base::FileInfo copy(path + "/copy.txt");
    EXPECT_TRUE(file.copyTo(copy.filePath().c_str()));
    EXPECT_TRUE(copy.deleteFile());
}

// Tests for pathToString / stringToPath UTF-8 round-trip (PR #28222)

class FileInfoPathConversionTest: public ::testing::Test
{
};

TEST_F(FileInfoPathConversionTest, RoundTripAsciiPath)
{
    std::string utf8 = "/some/simple/path";
    auto fsPath = Base::FileInfo::stringToPath(utf8);
    std::string result = Base::FileInfo::pathToString(fsPath);
    EXPECT_EQ(result, utf8);
}

TEST_F(FileInfoPathConversionTest, RoundTripNonAsciiPath)
{
    // German umlaut, common in Windows usernames (the exact bug scenario)
    std::string utf8 = "/home/m\xc3\xbcller/Documents";  // müller in UTF-8
    auto fsPath = Base::FileInfo::stringToPath(utf8);
    std::string result = Base::FileInfo::pathToString(fsPath);
    EXPECT_EQ(result, utf8);
}

TEST_F(FileInfoPathConversionTest, RoundTripChineseCharacters)
{
    // CJK characters: 用户 (user) in UTF-8
    std::string utf8 = "/home/\xe7\x94\xa8\xe6\x88\xb7/data";
    auto fsPath = Base::FileInfo::stringToPath(utf8);
    std::string result = Base::FileInfo::pathToString(fsPath);
    EXPECT_EQ(result, utf8);
}

TEST_F(FileInfoPathConversionTest, RoundTripAccentedCharacters)
{
    // French accented characters: café in UTF-8
    std::string utf8 = "/tmp/caf\xc3\xa9/file.txt";
    auto fsPath = Base::FileInfo::stringToPath(utf8);
    std::string result = Base::FileInfo::pathToString(fsPath);
    EXPECT_EQ(result, utf8);
}

TEST_F(FileInfoPathConversionTest, RoundTripEmptyString)
{
    std::string utf8;
    auto fsPath = Base::FileInfo::stringToPath(utf8);
    std::string result = Base::FileInfo::pathToString(fsPath);
    EXPECT_EQ(result, utf8);
}

TEST_F(FileInfoPathConversionTest, PathToStringPreservesUtf8)
{
    // Construct a path from a wide string directly and verify pathToString produces valid UTF-8
    std::filesystem::path p = Base::FileInfo::stringToPath("/tmp/\xc3\xa4\xc3\xb6\xc3\xbc");  // äöü
    std::string result = Base::FileInfo::pathToString(p);
    // Verify the UTF-8 bytes are preserved
    EXPECT_NE(result.find("\xc3\xa4"), std::string::npos);  // ä
    EXPECT_NE(result.find("\xc3\xb6"), std::string::npos);  // ö
    EXPECT_NE(result.find("\xc3\xbc"), std::string::npos);  // ü
}

TEST_F(FileInfoPathConversionTest, StringToPathProducesValidPath)
{
    // Verify that stringToPath produces a path that can be appended to
    std::string utf8 = "/home/\xc3\xbc\x73\x65r";  // üser
    auto fsPath = Base::FileInfo::stringToPath(utf8);
    auto child = fsPath / "subdir";
    std::string childStr = Base::FileInfo::pathToString(child);
    // The child path should contain both the parent with non-ASCII and the appended segment
    EXPECT_NE(childStr.find("\xc3\xbc"), std::string::npos);
    EXPECT_NE(childStr.find("subdir"), std::string::npos);
}

#ifdef _WIN32  // NOTE FC_OS_WIN32 is not available in the test code
TEST_F(FileInfoPathConversionTest, WidePathToUtf8)
{
    // Simulate a path obtained from the Windows OS (e.g. GetModuleFileNameW), which arrives as a
    // UTF-16 wide string. Verify pathToString encodes it as UTF-8.
    // L"C:\\Users\\müller" -- ü is U+00FC
    std::filesystem::path widePath(L"C:\\Users\\m\u00FCller\\Documents");
    std::string result = Base::FileInfo::pathToString(widePath);
    // Must contain the UTF-8 encoding of ü (0xC3 0xBC), not the ANSI mangled version
    EXPECT_NE(result.find("\xc3\xbc"), std::string::npos);
    EXPECT_NE(result.find("Documents"), std::string::npos);
}

TEST_F(FileInfoPathConversionTest, NaivePathStringLosesNonAscii)
{
    // Demonstrate the actual bug: on Windows, fs::path::string() converts to the ANSI codepage,
    // which mangles non-ASCII characters. This is what the old code did (before PR #28222) and why
    // pathToString is needed.
    std::filesystem::path widePath(L"C:\\Users\\m\u00FCller");
    std::string naive = widePath.string();                      // ANSI codepage on Windows
    std::string safe = Base::FileInfo::pathToString(widePath);  // UTF-8
    // The naive .string() result will differ from the correct UTF-8 encoding
    EXPECT_NE(naive, safe);
}
#endif

// Regression tests for GHSA-9vjf-h8f4-c229: a crafted archive entry name must not be able to
// escape the directory it is being extracted into.

TEST(FileInfoSafeArchiveEntryPathTest, AcceptsNamesWrittenByFreeCAD)
{
    // PropertyPostDataObject::SaveDocFile creates its names by removing the extraction directory
    // name from the front of an absolute path, so *every* name it writes starts with a separator.
    // Make sure we didn't break those, and they normalize to the expected separator-less name.
    EXPECT_EQ(Base::FileInfo::safeArchiveEntryPath("/datafile.vtm"), "datafile.vtm");
    EXPECT_EQ(
        Base::FileInfo::safeArchiveEntryPath("/datafile/datafile_0_0.vtu"),
        "datafile/datafile_0_0.vtu"
    );
    EXPECT_EQ(Base::FileInfo::safeArchiveEntryPath("dummy"), "dummy");
}

TEST(FileInfoSafeArchiveEntryPathTest, RejectsParentDirectoryTraversal)
{
    EXPECT_FALSE(Base::FileInfo::safeArchiveEntryPath("/../../../etc/passwd").has_value());
    EXPECT_FALSE(Base::FileInfo::safeArchiveEntryPath("../evil").has_value());
    EXPECT_FALSE(Base::FileInfo::safeArchiveEntryPath("datafile/../../evil").has_value());
    EXPECT_FALSE(Base::FileInfo::safeArchiveEntryPath("..").has_value());
    // Tricksy: starting with a separator shouldn't hide the traversal
    EXPECT_FALSE(Base::FileInfo::safeArchiveEntryPath("//..//evil").has_value());
}

TEST(FileInfoSafeArchiveEntryPathTest, LeadingDoubleSeparatorDoesNotEatAComponent)
{
    // On Windows a name starting with two separators parses as a filesystem root name, so naive
    // decomposition would drop "server" here (and, above, the ".." in "//..//evil"). The result
    // has to be identical on every platform.
    EXPECT_EQ(Base::FileInfo::safeArchiveEntryPath("//server/share/evil"), "server/share/evil");
    EXPECT_EQ(Base::FileInfo::safeArchiveEntryPath("\\\\server\\share\\evil"), "server/share/evil");
}

TEST(FileInfoSafeArchiveEntryPathTest, RejectsBackslashTraversal)
{
    // A backslash is an ordinary filename character on Linux and macOS, so these names look
    // harmless there. They are *not*: every path ultimately goes through FileInfo, and
    // setFile() rewrites '\' to '/'. So they *would* have been harmless, but are now real path seps
    // when the file is opened. So on Windows this test isn't that useful, but on Linux and macOS it
    // ensures that even though that happens, we still handle it as expected.
    EXPECT_FALSE(Base::FileInfo::safeArchiveEntryPath("..\\..\\evil").has_value());
    EXPECT_FALSE(Base::FileInfo::safeArchiveEntryPath("datafile\\..\\..\\evil").has_value());
    EXPECT_EQ(Base::FileInfo::safeArchiveEntryPath("datafile\\sub.vtu"), "datafile/sub.vtu");
}

TEST(FileInfoSafeArchiveEntryPathTest, RejectsColonNames)
{
    // Drive-relative paths and NTFS alternate data streams both escape via a colon, so the
    // sanitizer rejects colons wholesale -- any colon anywhere is treated as an invalid path.
    EXPECT_FALSE(Base::FileInfo::safeArchiveEntryPath("C:/Windows/System32/evil.dll").has_value());
    EXPECT_FALSE(Base::FileInfo::safeArchiveEntryPath("C:evil").has_value());
    EXPECT_FALSE(Base::FileInfo::safeArchiveEntryPath("datafile.vtu:stream").has_value());
}

TEST(FileInfoSafeArchiveEntryPathTest, RejectsNamesThatNormalizeToNothing)
{
    EXPECT_FALSE(Base::FileInfo::safeArchiveEntryPath("").has_value());
    EXPECT_FALSE(Base::FileInfo::safeArchiveEntryPath("/").has_value());
    EXPECT_FALSE(Base::FileInfo::safeArchiveEntryPath(".").has_value());
    EXPECT_FALSE(Base::FileInfo::safeArchiveEntryPath("./").has_value());
}

TEST(FileInfoSafeArchiveEntryPathTest, NormalizesRedundantComponents)
{
    // Not really security, but the method does do this cleanup
    EXPECT_EQ(Base::FileInfo::safeArchiveEntryPath("./datafile.vtm"), "datafile.vtm");
    EXPECT_EQ(Base::FileInfo::safeArchiveEntryPath("datafile//./sub.vtu"), "datafile/sub.vtu");
}

TEST(FileInfoSafeArchiveEntryPathTest, ContainedDotsAreAllowed)
{
    // A name that just *contains* dots is allowed, and must not get eaten
    EXPECT_EQ(Base::FileInfo::safeArchiveEntryPath("/datafile..vtu"), "datafile..vtu");
    EXPECT_EQ(Base::FileInfo::safeArchiveEntryPath("/a..b/c.vtu"), "a..b/c.vtu");
}

TEST(FileInfoSafeArchiveEntryPathTest, HostileNamesCannotEscapeWhenJoined)
{
    // Integration test for the above: **whatever** an archive holds, the path finally opened
    // is inside the extraction directory.
    const std::string extractionDir = "/tmp/FreeCAD_xxx/vtk_extract_datadir";
    const std::filesystem::path extractionPath
        = std::filesystem::path(extractionDir).lexically_normal();

    std::vector<std::string> hostile = {
        // Plain parent directory steps
        "/../evil",
        "../evil",
        "../../../etc/passwd",
        "a/../../evil",
        "a/b/../../../../evil",
        // Steps hidden behind redundant or empty components
        "..//evil",
        "//../evil",
        "/./../evil",
        ".././evil",
        "a/./../../evil",
        // Steps written with the separator FileInfo::setFile rewrites, including mixed forms
        "..\\evil",
        "a\\..\\..\\evil",
        "\\..\\..\\evil",
        "/..\\../evil",
        // Names that are nothing but a step
        "..",
        "../",
        "/..",
        "..\\",
        "./..",
        // Absolute and rooted names that try to ignore the extraction directory entirely
        "/etc/passwd",
        "//server/share/evil",
        "\\\\server\\share\\evil",
        "C:/Windows/System32/evil.dll",
        "C:evil",
        "evil.vtu:stream",
    };
    // A run long enough to climb past the filesystem root, which clamps rather than wrapping
    hostile.push_back("/" + [] {
        std::string steps;
        constexpr int soManySubdirs = 40;
        for (int i = 0; i < soManySubdirs; ++i) {
            steps += "../";
        }
        return steps;
    }() + "evil");

    std::size_t rejected = 0;
    std::size_t contained = 0;
    for (const auto& name : hostile) {
        auto safe = Base::FileInfo::safeArchiveEntryPath(name);
        if (!safe) {
            ++rejected;
            continue;
        }

        std::filesystem::path joined
            = std::filesystem::path(extractionDir + "/" + *safe).lexically_normal();
        std::filesystem::path relative = joined.lexically_relative(extractionPath);

        // An empty relative path means the two are unrelated, and a leading ".." means the
        // result climbed out. Either way the name escaped.
        EXPECT_FALSE(relative.empty()) << name << " -> " << joined.string();
        if (!relative.empty()) {
            EXPECT_NE(*relative.begin(), std::filesystem::path(".."))
                << name << " -> " << joined.string();
        }
        ++contained;
    }

    // Every single one of these should have tripped one test or the other, and both paths must
    // have been taken: a sanitizer that gave up and rejected everything would otherwise satisfy
    // the loop above without a single containment check ever running.
    EXPECT_EQ(rejected + contained, hostile.size());
    EXPECT_GT(contained, 0U);
}
