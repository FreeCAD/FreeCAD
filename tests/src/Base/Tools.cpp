#include "gtest/gtest.h"
#include <Base/Tools.h>
#include <bitset>

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)
TEST(BaseToolsSuite, TestUniqueName1)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body", {}), "Body");
}

TEST(BaseToolsSuite, TestUniqueName2)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body", {"Body"}, 1), "Body1");
}

TEST(BaseToolsSuite, TestUniqueName3)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body", {"Body"}, 3), "Body001");
}

TEST(BaseToolsSuite, TestUniqueName4)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body", {"Body001"}, 3), "Body002");
}

TEST(BaseToolsSuite, TestUniqueName5)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body", {"Body", "Body001"}, 3), "Body002");
}

TEST(BaseToolsSuite, TestUniqueName6)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body001", {"Body", "Body001"}, 3), "Body002");
}

TEST(BaseToolsSuite, TestUniqueName7)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body001", {"Body"}, 3), "Body002");
}

TEST(BaseToolsSuite, TestUniqueName8)
{
    EXPECT_EQ(Base::Tools::getUniqueName("Body12345", {"Body"}, 3), "Body12346");
}

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
    EXPECT_DOUBLE_EQ(Base::toRadians<double>(180), M_PI);
    EXPECT_DOUBLE_EQ(Base::toRadians<double>(90.0), M_PI / 2.0);
    EXPECT_DOUBLE_EQ(Base::toRadians<double>(0.0), 0.0);
}

TEST(Tools, TestDegree)
{
    EXPECT_EQ(Base::toDegrees<int>(3), 171);
    EXPECT_DOUBLE_EQ(Base::toDegrees<double>(M_PI), 180.0);
    EXPECT_DOUBLE_EQ(Base::toDegrees<double>(M_PI / 2.0), 90.0);
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
// NOLINTEND(cppcoreguidelines-*,readability-*)
