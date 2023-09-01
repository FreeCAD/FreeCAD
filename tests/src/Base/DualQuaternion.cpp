#include "gtest/gtest.h"
#include <Base/DualQuaternion.h>
#include <Base/Tools.h>

TEST(DualQuaternion, TestDefault)
{
    Base::DualQuat qq;
    EXPECT_EQ(qq.x.re, 0.0);
    EXPECT_EQ(qq.x.du, 0.0);
    EXPECT_EQ(qq.y.re, 0.0);
    EXPECT_EQ(qq.y.du, 0.0);
    EXPECT_EQ(qq.z.re, 0.0);
    EXPECT_EQ(qq.z.du, 0.0);
    EXPECT_EQ(qq.w.re, 0.0);
    EXPECT_EQ(qq.w.du, 0.0);
}

TEST(DualQuaternion, TestFromDouble)
{
    Base::DualQuat qq(1.0, 2.0, 3.0, 4.0);
    EXPECT_EQ(qq.x.re, 1.0);
    EXPECT_EQ(qq.x.du, 0.0);
    EXPECT_EQ(qq.y.re, 2.0);
    EXPECT_EQ(qq.y.du, 0.0);
    EXPECT_EQ(qq.z.re, 3.0);
    EXPECT_EQ(qq.z.du, 0.0);
    EXPECT_EQ(qq.w.re, 4.0);
    EXPECT_EQ(qq.w.du, 0.0);
}

TEST(DualQuaternion, TestFromDoubles)
{
    Base::DualQuat qq(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
    EXPECT_EQ(qq.x.re, 1.0);
    EXPECT_EQ(qq.x.du, 5.0);
    EXPECT_EQ(qq.y.re, 2.0);
    EXPECT_EQ(qq.y.du, 6.0);
    EXPECT_EQ(qq.z.re, 3.0);
    EXPECT_EQ(qq.z.du, 7.0);
    EXPECT_EQ(qq.w.re, 4.0);
    EXPECT_EQ(qq.w.du, 8.0);
}

TEST(DualQuaternion, TestFromDual)
{
    Base::DualNumber dn(1.0, 2.0);
    Base::DualQuat qq(dn, dn, dn, dn);
    EXPECT_EQ(qq.x.re, 1.0);
    EXPECT_EQ(qq.x.du, 2.0);
    EXPECT_EQ(qq.y.re, 1.0);
    EXPECT_EQ(qq.y.du, 2.0);
    EXPECT_EQ(qq.z.re, 1.0);
    EXPECT_EQ(qq.z.du, 2.0);
    EXPECT_EQ(qq.w.re, 1.0);
    EXPECT_EQ(qq.w.du, 2.0);
}

TEST(DualQuaternion, TestIdentity)
{
    Base::DualQuat qq = Base::DualQuat::identity();
    EXPECT_EQ(qq.x.re, 0.0);
    EXPECT_EQ(qq.x.du, 0.0);
    EXPECT_EQ(qq.y.re, 0.0);
    EXPECT_EQ(qq.y.du, 0.0);
    EXPECT_EQ(qq.z.re, 0.0);
    EXPECT_EQ(qq.z.du, 0.0);
    EXPECT_EQ(qq.w.re, 1.0);
    EXPECT_EQ(qq.w.du, 0.0);
}

TEST(DualQuaternion, TestReal)
{
    Base::DualQuat qq = Base::DualQuat(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0).real();
    EXPECT_EQ(qq.x.re, 1.0);
    EXPECT_EQ(qq.x.du, 0.0);
    EXPECT_EQ(qq.y.re, 2.0);
    EXPECT_EQ(qq.y.du, 0.0);
    EXPECT_EQ(qq.z.re, 3.0);
    EXPECT_EQ(qq.z.du, 0.0);
    EXPECT_EQ(qq.w.re, 4.0);
    EXPECT_EQ(qq.w.du, 0.0);
}

TEST(DualQuaternion, TestDual)
{
    Base::DualQuat qq = Base::DualQuat(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0).dual();
    EXPECT_EQ(qq.x.re, 5.0);
    EXPECT_EQ(qq.x.du, 0.0);
    EXPECT_EQ(qq.y.re, 6.0);
    EXPECT_EQ(qq.y.du, 0.0);
    EXPECT_EQ(qq.z.re, 7.0);
    EXPECT_EQ(qq.z.du, 0.0);
    EXPECT_EQ(qq.w.re, 8.0);
    EXPECT_EQ(qq.w.du, 0.0);
}

TEST(DualQuaternion, TestConjugate)
{
    Base::DualQuat qq = Base::DualQuat(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0).conj();
    EXPECT_EQ(qq.x.re, -1.0);
    EXPECT_EQ(qq.x.du, -5.0);
    EXPECT_EQ(qq.y.re, -2.0);
    EXPECT_EQ(qq.y.du, -6.0);
    EXPECT_EQ(qq.z.re, -3.0);
    EXPECT_EQ(qq.z.du, -7.0);
    EXPECT_EQ(qq.w.re, 4.0);
    EXPECT_EQ(qq.w.du, 8.0);
}

TEST(DualQuaternion, TestVec)
{
    Base::DualQuat qq = Base::DualQuat(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0).vec();
    EXPECT_EQ(qq.x.re, 1.0);
    EXPECT_EQ(qq.x.du, 5.0);
    EXPECT_EQ(qq.y.re, 2.0);
    EXPECT_EQ(qq.y.du, 6.0);
    EXPECT_EQ(qq.z.re, 3.0);
    EXPECT_EQ(qq.z.du, 7.0);
    EXPECT_EQ(qq.w.re, 0.0);
    EXPECT_EQ(qq.w.du, 0.0);
}

TEST(DualQuaternion, TestLength)
{
    Base::DualQuat qq = Base::DualQuat(0.0, 0.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
    EXPECT_DOUBLE_EQ(qq.length(), 5.0);
}

TEST(DualQuaternion, TestTheta)
{
    Base::DualQuat qq = Base::DualQuat(0.0, 0.0, 3.0, 3.0, 5.0, 6.0, 7.0, 8.0);
    EXPECT_DOUBLE_EQ(qq.theta(), Base::toRadians(90.0));
}

TEST(DualQuaternion, TestDot)
{
    Base::DualQuat qq1 = Base::DualQuat(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
    Base::DualQuat qq2 = Base::DualQuat(1.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0, 2.0);
    double dot = Base::DualQuat::dot(qq1, qq2);
    EXPECT_DOUBLE_EQ(dot, 10.0);
}

TEST(DualQuaternion, TestNegate)
{
    Base::DualQuat qq = Base::DualQuat(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
    Base::DualQuat dn(-qq);
    EXPECT_EQ(dn.x.re, -1.0);
    EXPECT_EQ(dn.x.du, -5.0);
    EXPECT_EQ(dn.y.re, -2.0);
    EXPECT_EQ(dn.y.du, -6.0);
    EXPECT_EQ(dn.z.re, -3.0);
    EXPECT_EQ(dn.z.du, -7.0);
    EXPECT_EQ(dn.w.re, -4.0);
    EXPECT_EQ(dn.w.du, -8.0);
}

TEST(DualQuaternion, TestPlus)
{
    Base::DualQuat qq1 = Base::DualQuat(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
    Base::DualQuat qq2 = Base::DualQuat(1.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0, 2.0);
    Base::DualQuat qq3 = qq1 + qq2;
    EXPECT_EQ(qq3.x.re, 2.0);
    EXPECT_EQ(qq3.x.du, 7.0);
    EXPECT_EQ(qq3.y.re, 3.0);
    EXPECT_EQ(qq3.y.du, 8.0);
    EXPECT_EQ(qq3.z.re, 4.0);
    EXPECT_EQ(qq3.z.du, 9.0);
    EXPECT_EQ(qq3.w.re, 5.0);
    EXPECT_EQ(qq3.w.du, 10.0);
}

TEST(DualQuaternion, TestMinus)
{
    Base::DualQuat qq1 = Base::DualQuat(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
    Base::DualQuat qq2 = Base::DualQuat(1.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0, 2.0);
    Base::DualQuat qq3 = qq1 - qq2;
    EXPECT_EQ(qq3.x.re, 0.0);
    EXPECT_EQ(qq3.x.du, 3.0);
    EXPECT_EQ(qq3.y.re, 1.0);
    EXPECT_EQ(qq3.y.du, 4.0);
    EXPECT_EQ(qq3.z.re, 2.0);
    EXPECT_EQ(qq3.z.du, 5.0);
    EXPECT_EQ(qq3.w.re, 3.0);
    EXPECT_EQ(qq3.w.du, 6.0);
}

TEST(DualQuaternion, TestMultiplyQuat)
{
    Base::DualQuat qq1 = Base::DualQuat(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
    Base::DualQuat qq2 = Base::DualQuat(1.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0, 2.0);
    Base::DualQuat qq3 = qq1 * qq2;
    EXPECT_EQ(qq3.x.re, 4.0);
    EXPECT_EQ(qq3.x.du, 20.0);
    EXPECT_EQ(qq3.y.re, 8.0);
    EXPECT_EQ(qq3.y.du, 32.0);
    EXPECT_EQ(qq3.z.re, 6.0);
    EXPECT_EQ(qq3.z.du, 26.0);
    EXPECT_EQ(qq3.w.re, -2.0);
    EXPECT_EQ(qq3.w.du, -14.0);
}

TEST(DualQuaternion, TestMultiplyNumber)
{
    Base::DualQuat qq1 = Base::DualQuat(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);
    Base::DualNumber dn1(1.0, 2.0);

    Base::DualQuat qq4 = qq1 * dn1;
    EXPECT_EQ(qq4.x.re, 1.0);
    EXPECT_EQ(qq4.x.du, 7.0);
    EXPECT_EQ(qq4.y.re, 2.0);
    EXPECT_EQ(qq4.y.du, 10.0);
    EXPECT_EQ(qq4.z.re, 3.0);
    EXPECT_EQ(qq4.z.du, 13.0);
    EXPECT_EQ(qq4.w.re, 4.0);
    EXPECT_EQ(qq4.w.du, 16.0);

    Base::DualQuat qq5 = dn1 * qq1;
    EXPECT_EQ(qq5.x.re, 1.0);
    EXPECT_EQ(qq5.x.du, 7.0);
    EXPECT_EQ(qq5.y.re, 2.0);
    EXPECT_EQ(qq5.y.du, 10.0);
    EXPECT_EQ(qq5.z.re, 3.0);
    EXPECT_EQ(qq5.z.du, 13.0);
    EXPECT_EQ(qq5.w.re, 4.0);
    EXPECT_EQ(qq5.w.du, 16.0);
}

TEST(DualQuaternion, TestMultiplyScalar)
{
    Base::DualQuat qq1 = Base::DualQuat(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);

    Base::DualQuat qq4 = qq1 * 2.0;
    EXPECT_EQ(qq4.x.re, 2.0);
    EXPECT_EQ(qq4.x.du, 10.0);
    EXPECT_EQ(qq4.y.re, 4.0);
    EXPECT_EQ(qq4.y.du, 12.0);
    EXPECT_EQ(qq4.z.re, 6.0);
    EXPECT_EQ(qq4.z.du, 14.0);
    EXPECT_EQ(qq4.w.re, 8.0);
    EXPECT_EQ(qq4.w.du, 16.0);

    Base::DualQuat qq5 = 2.0 * qq1;
    EXPECT_EQ(qq5.x.re, 2.0);
    EXPECT_EQ(qq5.x.du, 10.0);
    EXPECT_EQ(qq5.y.re, 4.0);
    EXPECT_EQ(qq5.y.du, 12.0);
    EXPECT_EQ(qq5.z.re, 6.0);
    EXPECT_EQ(qq5.z.du, 14.0);
    EXPECT_EQ(qq5.w.re, 8.0);
    EXPECT_EQ(qq5.w.du, 16.0);
}

TEST(DualQuaternion, TestPow)
{
    const double epsilon = 1e-12;
    Base::DualQuat qq1 = Base::DualQuat(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0);
    Base::DualQuat qq2 = qq1.pow(2.0);
    EXPECT_NEAR(qq2.x.re, 0.0, epsilon);
    EXPECT_NEAR(qq2.x.du, 0.0, epsilon);
    EXPECT_NEAR(qq2.y.re, 0.0, epsilon);
    EXPECT_NEAR(qq2.y.du, 0.0, epsilon);
    EXPECT_NEAR(qq2.z.re, 0.0, epsilon);
    EXPECT_NEAR(qq2.z.du, 0.0, epsilon);
    EXPECT_NEAR(qq2.w.re, -1.0, epsilon);
    EXPECT_NEAR(qq2.w.du, 0.0, epsilon);
}
