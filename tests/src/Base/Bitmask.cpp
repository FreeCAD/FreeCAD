// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include <Base/Bitmask.h>

enum class TestFlagEnum
{
    Flag1,
    Flag2,
    Flag3
};

class BitmaskTest: public ::testing::Test
{
protected:
    // void SetUp() override {};
    // void TearDown() override {};
};

TEST_F(BitmaskTest, toUnderlyingType)
{
    // Arrange
    Base::Flags<TestFlagEnum> flag1 {TestFlagEnum::Flag1};

    // Act
    auto result = flag1.toUnderlyingType();

    // Assert
    EXPECT_EQ(typeid(result), typeid(int));
}
