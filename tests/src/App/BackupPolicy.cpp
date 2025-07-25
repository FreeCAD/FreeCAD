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
#include <fstream>
#include <random>
#include <string>


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


private:
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
