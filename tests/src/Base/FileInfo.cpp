#include <gtest/gtest.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Base/TimeInfo.h>
#include <filesystem>

class FileInfoTest: public ::testing::Test
{
protected:
    FileInfoTest()
    {
        tmp.setFile(Base::FileInfo::getTempPath() + "fctest");
        tmp.createDirectory();

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

    file.setPermissions(Base::FileInfo::WriteOnly);
    EXPECT_FALSE(file.isReadable());
    EXPECT_TRUE(file.isWritable());

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
    EXPECT_EQ(file.size(), 5);
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
