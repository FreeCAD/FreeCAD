#include <fmt/format.h>
#include <fmt/printf.h>
#include <gtest/gtest.h>
#include <stdexcept>

// NOLINTBEGIN
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
    EXPECT_EQ("0.123456789012", fmt::sprintf("%.12f", 0.123456789012));
    EXPECT_EQ("-1234", fmt::sprintf("%li", -1234L));
    EXPECT_EQ("-1234567890", fmt::sprintf("%li", -1234567890L));
    EXPECT_EQ("1234567890", fmt::sprintf("%li", 1234567890UL));
}

TEST(fmt, print_fail)
{
    EXPECT_THROW(fmt::printf("%s%d", 1, 2), std::exception);
}
// NOLINTEND
