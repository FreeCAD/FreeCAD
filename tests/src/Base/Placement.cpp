#include "gtest/gtest.h"
#include <Base/DualQuaternion.h>
#include <Base/Matrix.h>
#include <Base/Placement.h>
#include <Base/Tools.h>

static const double epsilon = 1e-9;

TEST(Placement, TestDefault)
{
    Base::Placement plm;
    EXPECT_EQ(plm.getPosition().IsNull(), true);
    EXPECT_EQ(plm.getRotation().isNull(), false);
    EXPECT_EQ(plm.getRotation().isIdentity(), true);
}

TEST(Placement, TestPosRot)
{
    Base::Vector3d pos(1, 2, 3);
    Base::Rotation rot(1, 1, 2, 2);
    Base::Placement plm(pos, rot);
    EXPECT_EQ(plm.getPosition(), pos);
    EXPECT_EQ(plm.getRotation(), rot);
}

TEST(Placement, TestPosRotCnt)
{
    Base::Vector3d pos(1, 2, 3);
    Base::Rotation rot(1, 0, 0, 0);
    Base::Vector3d cnt(4, 5, 6);
    Base::Placement plm(pos, rot, cnt);

    EXPECT_EQ(plm.getPosition(), Base::Vector3d(1, 12, 15));
    EXPECT_EQ(plm.getRotation(), rot);
}

TEST(Placement, TestMatrix)
{
    Base::Matrix4D mat;
    mat.rotX(Base::toRadians(90.0));
    mat.rotY(Base::toRadians(90.0));
    mat.rotZ(Base::toRadians(90.0));
    mat.setCol(3, Base::Vector3d(1, 2, 3));
    Base::Placement plm(mat);
    EXPECT_EQ(plm.getPosition(), Base::Vector3d(1, 2, 3));
    Base::Vector3d axis;
    double angle {};
    plm.getRotation().getValue(axis, angle);
    EXPECT_EQ(angle, Base::toRadians(90.0));
    EXPECT_EQ(axis.IsEqual(Base::Vector3d(0, 1, 0), 0.001), true);
}

TEST(Placement, TestIdentity)
{
    Base::Vector3d pos(1, 2, 3);
    Base::Rotation rot(1, 0, 0, 0);
    Base::Vector3d cnt(4, 5, 6);
    Base::Placement plm(pos, rot, cnt);
    Base::Placement mult = plm * plm.inverse();
    EXPECT_EQ(mult.isIdentity(), true);
}

TEST(Placement, TestInvert)
{
    Base::Vector3d pos(1, 2, 3);
    Base::Rotation rot(1, 0, 0, 0);
    Base::Vector3d cnt(4, 5, 6);
    Base::Placement plm(pos, rot, cnt);
    plm.invert();
    EXPECT_EQ(plm.getPosition(), Base::Vector3d(-1, 12, 15));
    EXPECT_EQ(plm.getRotation().isIdentity(), false);
    EXPECT_EQ(plm.getRotation(), rot);
}

TEST(Placement, TestMove)
{
    Base::Vector3d pos(1, 2, 3);
    Base::Rotation rot(1, 0, 0, 0);
    Base::Vector3d cnt(4, 5, 6);
    Base::Placement plm(pos, rot, cnt);
    plm.move(Base::Vector3d(-1, -12, -15));
    EXPECT_EQ(plm.getPosition().IsNull(), true);
}

TEST(Placement, TestSame)
{
    Base::Vector3d pos(1, 2, 3);
    Base::Rotation rot(1, 0, 0, 0);
    Base::Vector3d cnt(4, 5, 6);
    Base::Placement plm(pos, rot, cnt);
    EXPECT_EQ(plm.isSame(plm), true);
    EXPECT_EQ(plm.isSame(plm, 0.001), true);
    EXPECT_EQ(plm == plm, true);

    Base::Placement plm2(plm * plm);
    EXPECT_EQ(plm2.isSame(plm), false);
    EXPECT_EQ(plm2.isSame(plm, 0.001), false);
    EXPECT_EQ(plm2 == plm, false);
}

TEST(Placement, TestMultiply)
{
    Base::Vector3d pos(1, 2, 3);
    Base::Rotation rot(1, 0, 0, 0);
    Base::Vector3d cnt(4, 5, 6);
    Base::Placement plm(pos, rot, cnt);

    Base::Placement plm2(plm * plm);
    EXPECT_EQ(plm2.getRotation().isIdentity(), true);
}

TEST(Placement, TestMultRight)
{
    Base::Vector3d pos1(1, 2, 3);
    Base::Rotation rot1(1, 0, 0, 0);
    Base::Placement plm1(pos1, rot1);

    Base::Vector3d pos2(3, 2, 1);
    Base::Rotation rot2(0, 1, 0, 0);
    Base::Placement plm2(pos2, rot2);

    plm1.multRight(plm2);
    EXPECT_EQ(plm1.getRotation(), Base::Rotation(0, 0, 1, 0));
    EXPECT_EQ(plm1.getPosition(), Base::Vector3d(4, 0, 2));
}

TEST(Placement, TestMultLeft)
{
    Base::Vector3d pos1(1, 2, 3);
    Base::Rotation rot1(1, 0, 0, 0);
    Base::Placement plm1(pos1, rot1);

    Base::Vector3d pos2(3, 2, 1);
    Base::Rotation rot2(0, 1, 0, 0);
    Base::Placement plm2(pos2, rot2);

    plm1.multLeft(plm2);
    EXPECT_EQ(plm1.getRotation(), Base::Rotation(0, 0, 1, 0));
    EXPECT_EQ(plm1.getPosition(), Base::Vector3d(2, 4, -2));
}

TEST(Placement, TestMultVec)
{
    Base::Vector3d pos(1, 2, 3);
    Base::Rotation rot(1, 0, 0, 0);
    Base::Placement plm(pos, rot);

    Base::Vector3d vec {1, 1, 1};
    plm.multVec(vec, vec);
    EXPECT_EQ(vec, Base::Vector3d(2, 1, 2));
}

TEST(Placement, TestDualQuat)
{
    Base::Vector3d pos(1, 2, 3);
    Base::Rotation rot(1, 0, 0, 0);
    Base::Placement plm(pos, rot);
    Base::DualQuat qq = plm.toDualQuaternion();
    Base::Placement plm2 = Base::Placement::fromDualQuaternion(qq);
    EXPECT_EQ(plm.isSame(plm2), true);
}

TEST(Placement, TestPow)
{
    Base::Vector3d axis1, axis2;
    double angle1 {}, angle2 {};

    Base::Vector3d pos(1, 4, 6);
    Base::Rotation rot(1, 2, 3, 4);
    Base::Placement plm(pos, rot);
    rot.getValue(axis1, angle1);

    Base::Placement plm2 = plm.pow(1.5);
    plm2.getRotation().getValue(axis2, angle2);
    EXPECT_DOUBLE_EQ(angle2, 1.5 * angle1);
    EXPECT_EQ(axis1.IsEqual(axis2, 0.0001), true);
}

TEST(Placement, TestSlerp)
{
    Base::Vector3d pos(1, 4, 6);
    Base::Rotation rot1(1, 0, 0, 0);
    Base::Rotation rot2(0, 1, 0, 0);
    Base::Placement plm1(pos, rot1);
    Base::Placement plm2(pos, rot2);

    Base::Placement plm3 = Base::Placement::slerp(plm1, plm2, 0.0);
    EXPECT_EQ(plm3.isSame(plm1), true);
    Base::Placement plm4 = Base::Placement::slerp(plm1, plm2, 1.0);
    EXPECT_EQ(plm4.isSame(plm2), true);
    Base::Placement plm5 = Base::Placement::slerp(plm1, plm1, 0.5);
    EXPECT_EQ(plm5.isSame(plm1), true);
    Base::Placement plm6 = Base::Placement::slerp(plm1, plm2, 0.5);
    EXPECT_EQ(plm6.getRotation().isSame(Base::Rotation(1, 1, 0, 0), epsilon), true);
    EXPECT_EQ(plm6.getPosition().IsEqual(pos, epsilon), true);
}

TEST(Placement, TestSclerp)
{
    Base::Vector3d pos(1, 4, 6);
    Base::Rotation rot1(1, 0, 0, 0);
    Base::Rotation rot2(0, 1, 0, 0);
    Base::Placement plm1(pos, rot1);
    Base::Placement plm2(pos, rot2);

    Base::Placement plm3 = Base::Placement::sclerp(plm1, plm2, 0.0);
    EXPECT_EQ(plm3.isSame(plm1), true);
    Base::Placement plm4 = Base::Placement::sclerp(plm1, plm2, 1.0);
    EXPECT_EQ(plm4.isSame(plm2, epsilon), true);
    Base::Placement plm5 = Base::Placement::sclerp(plm1, plm1, 0.5);
    EXPECT_EQ(plm5.isSame(plm1), true);
    Base::Placement plm6 = Base::Placement::sclerp(plm1, plm2, 0.5);
    EXPECT_EQ(plm6.getRotation().isSame(Base::Rotation(1, 1, 0, 0), epsilon), true);
    EXPECT_EQ(plm6.getPosition().IsEqual(pos, epsilon), true);
}
