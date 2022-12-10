#include "gtest/gtest.h"

#include "fmt/core.h"
#include "fmt/ranges.h"
#include "fmt/format.h"

TEST(Format, string)
{
    std::string str1 {fmt::format("--{}--", "abcd")};
    std::string str2 {"--abcd--"};
    EXPECT_EQ(str1,str2);
}

TEST(Format, float1){
    EXPECT_EQ(fmt::format("-{}-", 1.2345) , "-1.2345-");
}

TEST(Format, float2){
    EXPECT_EQ(fmt::format("{}", 1.2345) , "1.2345");
}

TEST(Format, range1){
    std::vector vec {1, 2, 3, 4};
    EXPECT_EQ(fmt::format("{::}", vec), "[1, 2, 3, 4]");
}

TEST(Format, range2){
    std::vector vec {1.1, 2.2, 3.3, 4.4};
    EXPECT_EQ(fmt::format("--{::}--", vec), "--[1.1, 2.2, 3.3, 4.4]--");
}
