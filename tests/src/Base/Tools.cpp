// SPDX-FileNotice: Part of the FreeCAD project.

#include <gtest/gtest.h>
#include <Base/Tools.h>
#include <bitset>
#include <vector>

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)

TEST(Tools, TestIota)
{
    Base::iotaGen<int> iota(1);
    std::vector<int> vec(5);
    std::generate(vec.begin(), vec.end(), iota);
    std::vector<int> out = {1, 2, 3, 4, 5};
    EXPECT_EQ(vec, out);
}

TEST(Tools, TestClamp)
{
    EXPECT_EQ(Base::clamp<int>(2, 1, 3), 2);
    EXPECT_EQ(Base::clamp<int>(4, 1, 3), 3);
    EXPECT_EQ(Base::clamp<int>(0, 1, 3), 1);

    EXPECT_DOUBLE_EQ(Base::clamp<double>(2, 1.5, 3.1), 2.0);
    EXPECT_DOUBLE_EQ(Base::clamp<double>(4, 1.5, 3.1), 3.1);
    EXPECT_DOUBLE_EQ(Base::clamp<double>(0, 1.5, 3.1), 1.5);
}

TEST(Tools, TestSignum)
{
    EXPECT_EQ(Base::sgn<int>(0), 0);
    EXPECT_EQ(Base::sgn<int>(2), 1);
    EXPECT_EQ(Base::sgn<int>(-2), -1);
    EXPECT_DOUBLE_EQ(Base::sgn<double>(0.1), 1.0);
    EXPECT_DOUBLE_EQ(Base::sgn<double>(2.0), 1.0);
    EXPECT_DOUBLE_EQ(Base::sgn<double>(-2.0), -1.0);
    EXPECT_DOUBLE_EQ(Base::sgn<double>(0.0), 0.0);
}

TEST(Tools, TestRadian)
{
    EXPECT_EQ(Base::toRadians<int>(90), 1);
    EXPECT_DOUBLE_EQ(Base::toRadians<double>(180), std::numbers::pi);
    EXPECT_DOUBLE_EQ(Base::toRadians<double>(90.0), std::numbers::pi / 2.0);
    EXPECT_DOUBLE_EQ(Base::toRadians<double>(0.0), 0.0);
}

TEST(Tools, TestDegree)
{
    EXPECT_EQ(Base::toDegrees<int>(3), 171);
    EXPECT_DOUBLE_EQ(Base::toDegrees<double>(std::numbers::pi), 180.0);
    EXPECT_DOUBLE_EQ(Base::toDegrees<double>(std::numbers::pi / 2.0), 90.0);
    EXPECT_DOUBLE_EQ(Base::toDegrees<double>(0.0), 0.0);
}

TEST(Tools, TestToggle)
{
    bool value = true;
    {
        Base::FlagToggler<bool> toggle(value);
        EXPECT_EQ(value, false);
    }
    EXPECT_EQ(value, true);
}

TEST(Tools, TestStateLocker)
{
    bool value = true;
    {
        Base::StateLocker lock(value);
        EXPECT_EQ(value, true);
    }
    EXPECT_EQ(value, true);
    {
        Base::StateLocker lock(value, false);
        EXPECT_EQ(value, false);
    }
    EXPECT_EQ(value, true);
}

TEST(Tools, TestBitsetLocker)
{
    std::bitset<8> value;
    {
        Base::BitsetLocker<decltype(value)> lock(value, 1);
        EXPECT_EQ(value.test(1), true);
    }
    EXPECT_EQ(value.test(1), false);
    {
        Base::BitsetLocker<decltype(value)> lock(value, 2, false);
        EXPECT_EQ(value.test(2), false);
    }
    EXPECT_EQ(value.test(2), false);

    value.set(3, true);
    {
        Base::BitsetLocker<decltype(value)> lock(value, 3, false);
        EXPECT_EQ(value.test(3), false);
    }
    EXPECT_EQ(value.test(3), true);
}
TEST(BaseToolsSuite, TestQuote)
{
    EXPECT_EQ(Base::Tools::quoted("Test"), "\"Test\"");
}

TEST(BaseToolsSuite, TestJoinList)
{
    EXPECT_EQ(Base::Tools::joinList({"AB", "CD"}), "AB, CD, ");
}
TEST(BaseToolsSuite, TestEscapeQuotesFromString)
{
    EXPECT_EQ(Base::Tools::escapeQuotesFromString("\'"), "\\\'");
    EXPECT_EQ(Base::Tools::escapeQuotesFromString("\""), "\\\"");
    EXPECT_EQ(Base::Tools::escapeQuotesFromString("\\"), "\\");
}
TEST(BaseToolsSuite, TestGetIdentifier)
{
    // ASCII and edge cases
    EXPECT_EQ(Base::Tools::getIdentifier("valid"), "valid");
    EXPECT_EQ(Base::Tools::getIdentifier("_valid"), "_valid");
    EXPECT_EQ(Base::Tools::getIdentifier("1invalid"), "_1invalid");
    EXPECT_EQ(Base::Tools::getIdentifier(""), "_");

    // Unicode letters (valid start and continue)
    EXPECT_EQ(Base::Tools::getIdentifier("πValue"), "πValue");  // Greek lowercase
    EXPECT_EQ(Base::Tools::getIdentifier("Δx"), "Δx");          // Greek uppercase
    EXPECT_EQ(Base::Tools::getIdentifier("ǅz"), "ǅz");          // Titlecase letter
    EXPECT_EQ(Base::Tools::getIdentifier("ʰindex"), "ʰindex");  // Modifier letter
    EXPECT_EQ(Base::Tools::getIdentifier("名字"), "名字");      // CJK characters (Lo)
    EXPECT_EQ(Base::Tools::getIdentifier("ⅨCount"), "ⅨCount");  // Letter number (Nl)

    // Digits not valid as first char
    EXPECT_EQ(Base::Tools::getIdentifier("٢ndPlace"), "_٢ndPlace");  // Arabic-Indic digit (Nd)

    // Connector punctuation
    EXPECT_EQ(Base::Tools::getIdentifier("valid_name"), "valid_name");
    EXPECT_EQ(Base::Tools::getIdentifier("valid‿name"), "valid‿name");
    EXPECT_EQ(Base::Tools::getIdentifier("valid﹍name"), "valid﹍name");

    // Combining marks (Mn, Mc)
    EXPECT_EQ(Base::Tools::getIdentifier("éclair"), "éclair");  // 'e' + combining acute accent (Mn)
    EXPECT_EQ(Base::Tools::getIdentifier("devा"), "devा");      // Devanagari vowel sign (Mc)

    // Invalid symbols
    EXPECT_EQ(Base::Tools::getIdentifier("hello!"), "hello_");
    EXPECT_EQ(Base::Tools::getIdentifier("foo-bar"), "foo_bar");
    EXPECT_EQ(Base::Tools::getIdentifier("a🙂b"), "a_b");  // Emoji replaced
    EXPECT_EQ(Base::Tools::getIdentifier("a*b&c"), "a_b_c");

    // Edge: starts with underscore, includes mixed types
    EXPECT_EQ(Base::Tools::getIdentifier("_नमस्ते123"), "_नमस्ते123");

    // Starts with invalid character
    EXPECT_EQ(Base::Tools::getIdentifier("💡idea"), "_idea");

    // Full-width digit (U+FF11, looks like '1')
    EXPECT_EQ(Base::Tools::getIdentifier("１start"), "_１start");
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
