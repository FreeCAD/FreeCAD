// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2025 The FreeCAD project association AISBL               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "InitApplication.h"

#include <App/BackupPolicy.h>

#include <filesystem>
#include <array>
#include <fstream>
#include <random>
#include <regex>
#include <string>

#if defined(__cpp_lib_chrono) && __cpp_lib_chrono >= 201907L && defined(_LIBCPP_VERSION)           \
    && _LIBCPP_VERSION >= 13000
// Apple's clang compiler did not support timezones fully until a quite recent version:
// before removing this preprocessor check, verify that it compiles on our oldest-supported
// macOS version.
#define CAN_USE_CHRONO_AND_FORMAT
#include <chrono>
#include <format>
#endif


class BackupPolicyTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _tempDir =
            std::filesystem::temp_directory_path() / ("fc_backup_policy-" + randomString(16));
        std::filesystem::create_directory(_tempDir);
    }

    void TearDown() override
    {
        std::filesystem::remove_all(_tempDir);
    }

    void apply(const std::string& sourcename, const std::string& targetname)
    {
        _policy.apply(sourcename, targetname);
    }

    void setPolicyTerms(App::BackupPolicy::Policy p, int count, bool useExt, const std::string& fmt)
    {
        _policy.setPolicy(p);
        _policy.setNumberOfFiles(count);
        _policy.useBackupExtension(useExt);
        _policy.setDateFormat(fmt);
    }

    // Create a named temporary file: returns the full path to the new file. Deleted by the TearDown
    // method at the end of the test.
    std::filesystem::path createTempFile(const std::string& filename)
    {
        std::filesystem::path p = _tempDir / filename;
        std::ofstream fileStream(p.string());
        fileStream << "Test data";
        fileStream.close();
        return p;
    }


protected:
    std::string randomString(size_t length)
    {
        static constexpr std::string_view chars = "0123456789"
                                                  "abcdefghijklmnopqrstuvwxyz"
                                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, static_cast<int>(chars.size()) - 1);

        std::string result;
        result.reserve(length);

        std::ranges::generate_n(std::back_inserter(result), length, [&]() {
            return chars[dis(gen)];
        });

        return result;
    }

    std::string filenameFromDateFormatString(const std::string& fmt)
    {
#if CAN_USE_CHRONO_AND_FORMAT
        std::chrono::zoned_time local_time {std::chrono::current_zone(),
                                            std::chrono::system_clock::now()};
        std::string fmt_str = "{:" + fmt + "}";
        std::string result = std::vformat(fmt_str, std::make_format_args(local_time));
#else
        std::time_t now = std::time(nullptr);
        std::tm local_tm {};
#if defined(_WIN32)
        localtime_s(&local_tm, &now);  // Windows
#else
        localtime_r(&now, &local_tm);  // POSIX
#endif
        constexpr size_t bufferLength = 128;
        std::array<char, bufferLength> buffer {};
        size_t bytes = std::strftime(buffer.data(), bufferLength, fmt.c_str(), &local_tm);
        if (bytes == 0) {
            throw std::runtime_error("failed to format time");
        }
        std::string result {buffer.data()};
#endif

        return result;
    }

    std::filesystem::path getTempPath()
    {
        return _tempDir;
    }

private:
    App::BackupPolicy _policy;
    std::filesystem::path _tempDir;
};

TEST_F(BackupPolicyTest, StandardSourceDoesNotExist)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::Standard, 1, false, "%Y-%m-%d_%H-%M-%S");

    // Act & Assert
    EXPECT_THROW(apply("nonexistent.fcstd", "backup.fcstd"), Base::FileException);
}

TEST_F(BackupPolicyTest, StandardWithZeroFilesDeletesExisting)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::Standard, 0, false, "%Y-%m-%d_%H-%M-%S");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");

    // Act
    apply(source.string(), target.string());

    // Assert
    GTEST_SKIP();  // Can't test on a real filesystem, too much caching for reliable results
    EXPECT_FALSE(std::filesystem::exists(target));
}

TEST_F(BackupPolicyTest, StandardWithOneFileNoPreviousBackups)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::Standard, 1, false, "%Y-%m-%d_%H-%M-%S");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");

    // Act
    apply(source.string(), target.string());

    // Assert
    EXPECT_TRUE(std::filesystem::exists(target.string() + "1"));
}

TEST_F(BackupPolicyTest, StandardWithOneFileOnePreviousBackup)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::Standard, 1, false, "%Y-%m-%d_%H-%M-%S");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");
    auto backup = createTempFile("target.fcstd1");

    // Act
    apply(source.string(), target.string());

    // Assert
    EXPECT_TRUE(std::filesystem::exists(backup));
    EXPECT_FALSE(std::filesystem::exists(target.string() + "2"));
}

TEST_F(BackupPolicyTest, StandardWithTwoFilesOnePreviousBackup)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::Standard, 2, false, "%Y-%m-%d_%H-%M-%S");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");
    auto backup = createTempFile("target.fcstd1");

    // Act
    apply(source.string(), target.string());

    // Assert
    EXPECT_TRUE(std::filesystem::exists(backup));
    EXPECT_TRUE(std::filesystem::exists(target.string() + "2"));
}

TEST_F(BackupPolicyTest, StandardWithTwoFilesOnePreviousBackupUnexpectedSuffix)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::Standard, 2, false, "%Y-%m-%d_%H-%M-%S");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");
    auto backup = createTempFile("target.fcstd1");
    auto weird = createTempFile("target.fcstd2a");

    // Act
    apply(source.string(), target.string());

    // Assert
    EXPECT_TRUE(std::filesystem::exists(backup));
    EXPECT_TRUE(std::filesystem::exists(target.string() + "2"));
    EXPECT_TRUE(std::filesystem::exists(weird));
}

TEST_F(BackupPolicyTest, StandardWithTwoFilesOnePreviousBackupOutOfSequenceNumber)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::Standard, 2, false, "%Y-%m-%d_%H-%M-%S");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");
    auto backup = createTempFile("target.fcstd1");
    auto weird = createTempFile("target.fcstd999");

    // Act
    apply(source.string(), target.string());

    // Assert
    EXPECT_TRUE(std::filesystem::exists(backup));
    bool check1 = std::filesystem::exists(target.string() + "2");
    bool check2 = std::filesystem::exists(weird);
    EXPECT_NE(check1, check2);  // Only one or the other can exist (we don't know which because it
                                // depends on file modification date)
}

TEST_F(BackupPolicyTest, StandardWithFCBakSet)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::Standard, 1, true, "%Y-%m-%d_%H-%M-%S");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");

    // Act
    apply(source.string(), target.string());

    // Assert
    EXPECT_TRUE(std::filesystem::exists(target.string() + "1"));  // No FCBak extension for Standard
}

TEST_F(BackupPolicyTest, TimestampSourceDoesNotExist)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 1, false, "%Y-%m-%d_%H-%M-%S");

    // Act & Assert
    EXPECT_THROW(apply("nonexistent.fcstd", "backup.fcstd"), Base::FileException);
}

TEST_F(BackupPolicyTest, TimestampNoSourceGiven)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 1, false, "%Y-%m-%d_%H-%M-%S");
    auto target = createTempFile("target.fcstd");

    // Act & Assert
    EXPECT_THROW(apply("nonexistent.fcstd", target.string()), Base::FileException);
}

TEST_F(BackupPolicyTest, TimestampNoTargetGiven)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 1, false, "%Y-%m-%d_%H-%M-%S");
    auto source = createTempFile("source.fcstd");

    // Act & Assert
    EXPECT_THROW(apply(source.string(), ""), Base::FileException);
}

TEST_F(BackupPolicyTest, TimestampWithZeroFilesDeletesExisting)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 0, false, "%Y-%m-%d_%H-%M-%S");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");

    // Act
    apply(source.string(), target.string());

    // Assert
    GTEST_SKIP();  // Can't test on a real filesystem, too much caching for reliable results
    EXPECT_FALSE(std::filesystem::exists(target));
}

TEST_F(BackupPolicyTest, TimestampWithOneFileAndNoneExistingNotFCBakCreatesNumberedFile)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 1, false, "%Y-%m-%d");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");

    // Act
    apply(source.string(), target.string());

    // Assert
    // Without the .FCBak extension, the date stuff is completely ignored, even if the policy is set
    // to "Timestamp"
    EXPECT_TRUE(std::filesystem::exists(target.string() + "1"));
}

TEST_F(BackupPolicyTest, TimestampSourceHasNoExtension)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 1, true, "%Y-%m-%d");
    auto source = createTempFile("source");
    auto target = createTempFile("target.fcstd");

    // Act
    apply(source.string(), target.string());

    // Assert
    auto expected = "target." + filenameFromDateFormatString("%Y-%m-%d") + ".FCBak";
    EXPECT_TRUE(std::filesystem::exists(getTempPath() / expected));
}

TEST_F(BackupPolicyTest, TimestampTargetHasNoExtension)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 1, true, "%Y-%m-%d");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target");

    // Act
    apply(source.string(), target.string());

    // Assert
    auto expected = "target." + filenameFromDateFormatString("%Y-%m-%d") + ".FCBak";
    EXPECT_TRUE(std::filesystem::exists(getTempPath() / expected));
}

TEST_F(BackupPolicyTest, TimestampWithOneFileAndNoneExistingFCBakCreatesDatedFile)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 1, true, "%Y-%m-%d");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");

    // Act
    apply(source.string(), target.string());

    // Assert
    auto expected = "target." + filenameFromDateFormatString("%Y-%m-%d") + ".FCBak";
    EXPECT_TRUE(std::filesystem::exists(getTempPath() / expected));
}

TEST_F(BackupPolicyTest, TimestampReplacesDotsWithDashes)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 1, true, "%Y.%m.%d");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");

    // Act
    apply(source.string(), target.string());

    // Assert
    auto expected = "target." + filenameFromDateFormatString("%Y-%m-%d") + ".FCBak";
    EXPECT_TRUE(std::filesystem::exists(getTempPath() / expected));
}

TEST_F(BackupPolicyTest, DISABLED_TimestampWithInvalidFormatStringThrows)
{
    // THIS TEST IS DISABLED BECAUSE THE CURRENT CODE DOES NOT CORRECTLY HANDLE INVALID FORMAT
    // OPERATIONS, AND CRASHES WHEN GIVEN ONE. FIXME.

    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 1, true, "%Q-%W-%E");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");

    // Act and Assert
    EXPECT_THROW(apply(source.string(), target.string()), Base::FileException);
}

TEST_F(BackupPolicyTest, DISABLED_TimestampWithAbsurdlyLongFormatStringThrows)
{
    // THIS TEST IS DISABLED BECAUSE THE CURRENT CODE DOES NOT CORRECTLY HANDLE OVER-LENGTH FORMAT
    // OPERATIONS, AND GENERATES AN INVALID FILENAME. FIXME.

    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp,
                   1,
                   true,
                   "%A, %B %d, %Y at %H:%M:%S %Z (Day %j of the year, Week %U/%W) — This is a "
                   "verbose date string for demonstration purposes.");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");

    // Act and Assert
    EXPECT_THROW(apply(source.string(), target.string()), Base::FileException);
}

TEST_F(BackupPolicyTest, TimestampDetectsOldBackupFormat)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 1, true, "%Y-%m-%d");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");
    auto backup = createTempFile("target.fcstd12345");

    // Act
    apply(source.string(), target.string());

    // Assert
    bool check1 = std::filesystem::exists(
        getTempPath() / ("target." + filenameFromDateFormatString("%Y-%m-%d") + ".FCBak"));
    bool check2 = std::filesystem::exists(backup);
    EXPECT_NE(check1, check2);
}

TEST_F(BackupPolicyTest, TimestampDetectsOldBackupFormatIgnoresOther)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 1, true, "%Y-%m-%d");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");
    auto backup = createTempFile("target.fcstd12345");
    auto weird = createTempFile("target.fcstd12345abc");

    // Act
    apply(source.string(), target.string());

    // Assert
    bool check1 = std::filesystem::exists(
        getTempPath() / ("target." + filenameFromDateFormatString("%Y-%m-%d") + ".FCBak"));
    bool check2 = std::filesystem::exists(backup);
    EXPECT_NE(check1, check2);
    EXPECT_TRUE(std::filesystem::exists(weird));
}

TEST_F(BackupPolicyTest, TimestampDetectsAndRetainsOldBackupWhenAllowed)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 2, true, "%Y-%m-%d");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");
    auto backup = createTempFile("target.fcstd12345");

    // Act
    apply(source.string(), target.string());

    // Assert
    EXPECT_TRUE(std::filesystem::exists(
        getTempPath() / ("target." + filenameFromDateFormatString("%Y-%m-%d") + ".FCBak")));
    EXPECT_TRUE(std::filesystem::exists(backup));
}

TEST_F(BackupPolicyTest, TimestampFormatStringEndsWithSpace)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 1, true, "%Y-%m-%d ");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");

    // Act
    apply(source.string(), target.string());

    // Assert (the space is stripped, and an index is added)
    EXPECT_TRUE(std::filesystem::exists(
        getTempPath() / ("target." + filenameFromDateFormatString("%Y-%m-%d") + "1.FCBak")));
}

TEST_F(BackupPolicyTest, TimestampFormatStringEndsWithDash)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 1, true, "%Y-%m-%d-");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");

    // Act
    apply(source.string(), target.string());

    // Assert (the dash is left, and an index is added)
    EXPECT_TRUE(std::filesystem::exists(
        getTempPath() / ("target." + filenameFromDateFormatString("%Y-%m-%d") + "-1.FCBak")));
}

TEST_F(BackupPolicyTest, TimestampFormatFileAlreadyExists)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 2, true, "%Y-%m-%d");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");
    auto backup = createTempFile("target." + filenameFromDateFormatString("%Y-%m-%d") + ".FCBak");

    // Act
    apply(source.string(), target.string());

    // Assert (An index is appended)
    EXPECT_TRUE(std::filesystem::exists(backup));
    EXPECT_TRUE(std::filesystem::exists(
        getTempPath() / ("target." + filenameFromDateFormatString("%Y-%m-%d") + "-1.FCBak")));
}

TEST_F(BackupPolicyTest, TimestampFormatFileAlreadyExistsMultipleTimes)
{
    // Arrange
    setPolicyTerms(App::BackupPolicy::Policy::TimeStamp, 5, true, "%Y-%m-%d");
    auto source = createTempFile("source.fcstd");
    auto target = createTempFile("target.fcstd");
    auto backup = createTempFile("target." + filenameFromDateFormatString("%Y-%m-%d") + ".FCBak");
    auto backup1 =
        createTempFile("target." + filenameFromDateFormatString("%Y-%m-%d") + "-1.FCBak");
    auto backup2 =
        createTempFile("target." + filenameFromDateFormatString("%Y-%m-%d") + "-2.FCBak");
    auto backup3 =
        createTempFile("target." + filenameFromDateFormatString("%Y-%m-%d") + "-3.FCBak");

    // Act
    apply(source.string(), target.string());

    // Assert (An index is appended)
    EXPECT_TRUE(std::filesystem::exists(backup));
    EXPECT_TRUE(std::filesystem::exists(backup1));
    EXPECT_TRUE(std::filesystem::exists(backup2));
    EXPECT_TRUE(std::filesystem::exists(backup3));
    EXPECT_TRUE(std::filesystem::exists(
        getTempPath() / ("target." + filenameFromDateFormatString("%Y-%m-%d") + "-4.FCBak")));
}
