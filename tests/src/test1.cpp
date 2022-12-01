#include "gtest/gtest.h"

TEST(FirstSuite, FirstTest){
    EXPECT_EQ(1, 1) << "are in fact equal";
}

TEST(FirstSuite, SecondTest){
    EXPECT_NE(1, 2) << "not equal";
}

TEST(FirstSuite, ThirdTest){
    ASSERT_STREQ("A", "A") << "str not equal";
}

TEST(FirstSuite,FifthTest){
    ASSERT_STREQ("am", "am") << "str equal";
}

TEST(FirstSuite, FourthTest){
    ASSERT_STRNE("am", "A") << "str not equal";
}
