// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"

#include <memory>

#include <Mod/Start/App/FileUtilities.h>


class FileUtilitiesTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {}

    void TearDown() override
    {}


private:
};

TEST_F(FileUtilitiesTest, humanReadableSizeZeroBytes)
{
    auto result = Start::humanReadableSize(0);
    EXPECT_EQ(result, "0 B");
}

TEST_F(FileUtilitiesTest, humanReadableSizeBytesHasNoDecimal)
{
    constexpr uint64_t smallNumberOfBytes {512};
    auto result = Start::humanReadableSize(smallNumberOfBytes);
    EXPECT_EQ(result.find('.'), std::string::npos);
}

TEST_F(FileUtilitiesTest, humanReadableSizeOthersHaveDecimal)
{
    constexpr std::array<uint64_t, 6> testSizes {1000,
                                                 123456,
                                                 123456789,
                                                 123456789013,
                                                 100000000000000,
                                                 std::numeric_limits<uint64_t>::max()};
    for (const auto size : testSizes) {
        auto result = Start::humanReadableSize(size);
        EXPECT_NE(result.find('.'), std::string::npos);
    }
}

TEST_F(FileUtilitiesTest, humanReadableSizeB)
{
    EXPECT_EQ("999 B", Start::humanReadableSize(999));
}

TEST_F(FileUtilitiesTest, humanReadableSizekB)
{
    EXPECT_EQ("1.0 kB", Start::humanReadableSize(1000));
}

TEST_F(FileUtilitiesTest, humanReadableSizekBSignificantDigits)
{
    EXPECT_EQ(Start::humanReadableSize(1000), Start::humanReadableSize(1001));
}

TEST_F(FileUtilitiesTest, humanReadableSizeMB)
{
    EXPECT_EQ("2.5 MB", Start::humanReadableSize(2.5e6));
}

TEST_F(FileUtilitiesTest, humanReadableSizeGB)
{
    EXPECT_EQ("3.7 GB", Start::humanReadableSize(3.7e9));
}

TEST_F(FileUtilitiesTest, humanReadableSizeTB)
{
    EXPECT_EQ("444.8 TB", Start::humanReadableSize(444.823e12));
}

TEST_F(FileUtilitiesTest, humanReadableSizePB)
{
    EXPECT_EQ("1.2 PB", Start::humanReadableSize(1.2e15));
}

TEST_F(FileUtilitiesTest, humanReadableSizeEB)
{
    EXPECT_EQ("7.3 EB", Start::humanReadableSize(7.3e18));
}
