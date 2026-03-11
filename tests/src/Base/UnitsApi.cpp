#include <gtest/gtest.h>
#include "Base/UnitsApi.h"

using Base::UnitsApi;

TEST(UnitsApi_toUnicodeSuperscript, no_caret_unchanged)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("mm"), "mm");
}

TEST(UnitsApi_toUnicodeSuperscript, empty_string)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript(""), "");
}

TEST(UnitsApi_toUnicodeSuperscript, digit_0)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("x^0"), "x\xe2\x81\xb0");  // ⁰
}

TEST(UnitsApi_toUnicodeSuperscript, digit_1)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("x^1"), "x\xc2\xb9");  // ¹
}

TEST(UnitsApi_toUnicodeSuperscript, digit_2)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("x^2"), "x\xc2\xb2");  // ²
}

TEST(UnitsApi_toUnicodeSuperscript, digit_3)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("x^3"), "x\xc2\xb3");  // ³
}

TEST(UnitsApi_toUnicodeSuperscript, digit_4)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("x^4"), "x\xe2\x81\xb4");  // ⁴
}

TEST(UnitsApi_toUnicodeSuperscript, digit_5)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("x^5"), "x\xe2\x81\xb5");  // ⁵
}

TEST(UnitsApi_toUnicodeSuperscript, digit_6)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("x^6"), "x\xe2\x81\xb6");  // ⁶
}

TEST(UnitsApi_toUnicodeSuperscript, digit_7)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("x^7"), "x\xe2\x81\xb7");  // ⁷
}

TEST(UnitsApi_toUnicodeSuperscript, digit_8)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("x^8"), "x\xe2\x81\xb8");  // ⁸
}

TEST(UnitsApi_toUnicodeSuperscript, digit_9)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("x^9"), "x\xe2\x81\xb9");  // ⁹
}

TEST(UnitsApi_toUnicodeSuperscript, negative_exponent_2)
{
    // ^-2  →  ⁻²
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("s^-2"), "s\xe2\x81\xbb\xc2\xb2");
}

TEST(UnitsApi_toUnicodeSuperscript, negative_exponent_3)
{
    // ^-3  →  ⁻³
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("s^-3"), "s\xe2\x81\xbb\xc2\xb3");
}

TEST(UnitsApi_toUnicodeSuperscript, area_unit_mm_squared)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("mm^2"), "mm\xc2\xb2");  // mm²
}

TEST(UnitsApi_toUnicodeSuperscript, multiple_exponents_in_one_string)
{
    // mm^2/s^3  →  mm²/s³
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("mm^2/s^3"),
              "mm\xc2\xb2/s\xc2\xb3");
}

TEST(UnitsApi_toUnicodeSuperscript, inverse_area_unit)
{
    // 1/mm^2  →  1/mm²
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("1/mm^2"), "1/mm\xc2\xb2");
}

TEST(UnitsApi_toUnicodeSuperscript, complex_unit_with_negative_exponent)
{
    // mm^2*kg/s^-2  →  mm²*kg/s⁻²
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("mm^2*kg/s^-2"),
              "mm\xc2\xb2*kg/s\xe2\x81\xbb\xc2\xb2");
}

TEST(UnitsApi_toUnicodeSuperscript, caret_at_end_of_string)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("mm^"), "mm^");
}

TEST(UnitsApi_toUnicodeSuperscript, caret_minus_at_end_of_string)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("mm^-"), "mm^-");
}

TEST(UnitsApi_toUnicodeSuperscript, caret_followed_by_non_digit)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("mm^a"), "mm^a");
}

TEST(UnitsApi_toUnicodeSuperscript, double_caret_before_digit)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("mm^^2"), "mm^\xc2\xb2"); // mm^²
}

TEST(UnitsApi_toUnicodeSuperscript, multi_digit)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("2^32"), "2\xc2\xb3\xc2\xb2");  // 2³²
}

TEST(UnitsApi_toUnicodeSuperscript, negative_multi_digit)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("2^-32"), "2\xe2\x81\xbb\xc2\xb3\xc2\xb2");  // 2⁻³²
}

TEST(UnitsApi_toUnicodeSuperscript, minus_inside_exponent)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("2^3-2"), "2\xc2\xb3-2");  // 2³-2
}

TEST(UnitsApi_toUnicodeSuperscript, minus_inside_negative_exponent)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("2^-3-2"), "2\xe2\x81\xbb\xc2\xb3-2");  // 2⁻³-2
}

TEST(UnitsApi_toUnicodeSuperscript, minus_inside_multi_digit_exponent)
{
    EXPECT_EQ(UnitsApi::toUnicodeSuperscript("2^-31-2"), "2\xe2\x81\xbb\xc2\xb3\xc2\xb9-2");  // 2⁻³¹-2
}
