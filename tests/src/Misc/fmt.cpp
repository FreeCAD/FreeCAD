#include "fmt/format.h"
#include "fmt/printf.h"
#include "gtest/gtest.h"
#include <stdexcept>

TEST(fmt, fail)
{
    EXPECT_NE("abc", fmt::format("{}{}", "a", "b"));
}

TEST(fmt, pass)
{
    EXPECT_EQ("ab", fmt::format("{}{}", "a", "b"));
}

TEST(fmt, print_pass)
{
    EXPECT_EQ("12", fmt::sprintf("%s%d", "1", 2));
    EXPECT_EQ("x", fmt::sprintf("%c", 'x'));
    EXPECT_EQ("1.23 2", fmt::sprintf("%.2f %d", 1.23456, 2));
}

TEST(fmt, print_fail)
{
    EXPECT_THROW(fmt::printf("%s%d", 1, 2), std::exception);
}
