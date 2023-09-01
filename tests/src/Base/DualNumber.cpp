#include "gtest/gtest.h"
#include <Base/DualNumber.h>

TEST(DualNumber, TestDefault)
{
    Base::DualNumber dn;
    EXPECT_EQ(dn.re, 0.0);
    EXPECT_EQ(dn.du, 0.0);
}

TEST(DualNumber, TestNonDefault)
{
    Base::DualNumber dn(1.0);
    EXPECT_EQ(dn.re, 1.0);
    EXPECT_EQ(dn.du, 0.0);

    Base::DualNumber dn2(1.0, 2.0);
    EXPECT_EQ(dn2.re, 1.0);
    EXPECT_EQ(dn2.du, 2.0);
}

TEST(DualNumber, TestNegate)
{
    Base::DualNumber dn(-Base::DualNumber(1.0, 2.0));
    EXPECT_EQ(dn.re, -1.0);
    EXPECT_EQ(dn.du, -2.0);
}

TEST(DualNumber, TestPlus)
{
    Base::DualNumber dn1(1.0, 2.0);
    Base::DualNumber dn2(2.0, -1.0);

    Base::DualNumber dn3 = dn1 + dn2;
    EXPECT_EQ(dn3.re, 3.0);
    EXPECT_EQ(dn3.du, 1.0);

    Base::DualNumber dn4 = dn1 + 2.0;
    EXPECT_EQ(dn4.re, 3.0);
    EXPECT_EQ(dn4.du, 2.0);

    Base::DualNumber dn5 = 2.0 + dn1;
    EXPECT_EQ(dn5.re, 3.0);
    EXPECT_EQ(dn5.du, 2.0);
}

TEST(DualNumber, TestMinus)
{
    Base::DualNumber dn1(1.0, 2.0);
    Base::DualNumber dn2(2.0, -1.0);

    Base::DualNumber dn3 = dn1 - dn2;
    EXPECT_EQ(dn3.re, -1.0);
    EXPECT_EQ(dn3.du, 3.0);

    Base::DualNumber dn4 = dn1 - 2.0;
    EXPECT_EQ(dn4.re, -1.0);
    EXPECT_EQ(dn4.du, 2.0);

    Base::DualNumber dn5 = 2.0 - dn1;
    EXPECT_EQ(dn5.re, 1.0);
    EXPECT_EQ(dn5.du, -2.0);
}

TEST(DualNumber, TestMultiply)
{
    Base::DualNumber dn1(1.0, 2.0);
    Base::DualNumber dn2(2.0, -1.0);

    Base::DualNumber dn3 = dn1 * dn2;
    EXPECT_EQ(dn3.re, 2.0);
    EXPECT_EQ(dn3.du, 3.0);

    Base::DualNumber dn4 = dn1 * 2.0;
    EXPECT_EQ(dn4.re, 2.0);
    EXPECT_EQ(dn4.du, 4.0);

    Base::DualNumber dn5 = 2.0 * dn1;
    EXPECT_EQ(dn5.re, 2.0);
    EXPECT_EQ(dn5.du, 4.0);
}

TEST(DualNumber, TestDivide)
{
    Base::DualNumber dn1(1.0, 2.0);
    Base::DualNumber dn2(2.0, -1.0);

    Base::DualNumber dn3 = dn1 / dn2;
    EXPECT_EQ(dn3.re, 0.5);
    EXPECT_EQ(dn3.du, 1.25);

    Base::DualNumber dn4 = dn1 / 2.0;
    EXPECT_EQ(dn4.re, 0.5);
    EXPECT_EQ(dn4.du, 1.0);
}

TEST(DualNumber, TestPow)
{
    Base::DualNumber dn1(3.0, 2.0);
    Base::DualNumber dn2 = Base::pow(dn1, 2.0);
    EXPECT_EQ(dn2.re, 9.0);
    EXPECT_EQ(dn2.du, 12.0);
}
