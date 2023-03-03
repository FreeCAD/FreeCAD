#include "gtest/gtest.h"
#include "fmt/format.h"

TEST(fmt, fail){
    EXPECT_NE("abc", fmt::format("{}{}","a","b"));
}
TEST(fmt, pass){
    EXPECT_EQ("ab", fmt::format("{}{}","a","b"));
}
