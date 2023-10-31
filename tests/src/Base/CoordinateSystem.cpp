#include "gtest/gtest.h"
#include <Base/CoordinateSystem.h>
#include <Base/Exception.h>

// NOLINTBEGIN
TEST(CoordinateSystem, TestDefault)
{
    Base::CoordinateSystem cs;
    EXPECT_EQ(cs.getPosition(), Base::Vector3d(0, 0, 0));
    EXPECT_EQ(cs.getXDirection(), Base::Vector3d(1, 0, 0));
    EXPECT_EQ(cs.getYDirection(), Base::Vector3d(0, 1, 0));
    EXPECT_EQ(cs.getZDirection(), Base::Vector3d(0, 0, 1));
}

TEST(CoordinateSystem, TestSetAxisFailure)
{
    Base::CoordinateSystem cs;

    EXPECT_THROW(cs.setAxes(Base::Vector3d(0, 0, 1), Base::Vector3d(0, 0, 1)), Base::ValueError);
}

TEST(CoordinateSystem, TestSetAxis)
{
    Base::CoordinateSystem cs;
    Base::Axis axis;
    axis.setBase(Base::Vector3d(1, 2, 3));
    axis.setDirection(Base::Vector3d(1, 1, 1));
    cs.setAxes(axis, Base::Vector3d(0, 0, 1));

    EXPECT_EQ(cs.getPosition(), Base::Vector3d(1, 2, 3));
    EXPECT_EQ(cs.getXDirection(), Base::Vector3d(-1, -1, 2).Normalize());
    EXPECT_EQ(cs.getYDirection(), Base::Vector3d(1, -1, 0).Normalize());
    EXPECT_EQ(cs.getZDirection(), Base::Vector3d(1, 1, 1).Normalize());
}

TEST(CoordinateSystem, TestSetXDir)
{
    Base::CoordinateSystem cs;
    cs.setXDirection(Base::Vector3d(1, 1, 1));

    EXPECT_EQ(cs.getXDirection(), Base::Vector3d(1, 1, 0).Normalize());
    EXPECT_EQ(cs.getYDirection(), Base::Vector3d(-1, 1, 0).Normalize());
    EXPECT_EQ(cs.getZDirection(), Base::Vector3d(0, 0, 1).Normalize());
}

TEST(CoordinateSystem, TestSetYDir)
{
    Base::CoordinateSystem cs;
    cs.setYDirection(Base::Vector3d(1, 1, 1));

    EXPECT_EQ(cs.getXDirection(), Base::Vector3d(1, -1, 0).Normalize());
    EXPECT_EQ(cs.getYDirection(), Base::Vector3d(1, 1, 0).Normalize());
    EXPECT_EQ(cs.getZDirection(), Base::Vector3d(0, 0, 1).Normalize());
}

TEST(CoordinateSystem, TestSetZDir)
{
    Base::CoordinateSystem cs;
    cs.setZDirection(Base::Vector3d(1, 1, 1));

    EXPECT_EQ(cs.getXDirection(), Base::Vector3d(2, -1, -1).Normalize());
    EXPECT_EQ(cs.getYDirection(), Base::Vector3d(0, 1, -1).Normalize());
    EXPECT_EQ(cs.getZDirection(), Base::Vector3d(1, 1, 1).Normalize());
}

TEST(CoordinateSystem, TestTransformPlacement)
{
    Base::CoordinateSystem cs;
    Base::CoordinateSystem csT;
    Base::Placement plm;
    plm.setPosition(Base::Vector3d(1, 2, 3));
    plm.setRotation(Base::Rotation(1, 1, 2, 2));
    csT.transform(plm);

    Base::Placement dis = cs.displacement(csT);
    EXPECT_EQ(plm.isSame(dis, 1e-15), true);

    Base::Placement disT = csT.displacement(cs);
    EXPECT_EQ(plm.inverse().isSame(disT, 1e-15), true);
}

TEST(CoordinateSystem, TestMultTransformPlacement)
{
    Base::CoordinateSystem cs;
    Base::CoordinateSystem csT;
    Base::Placement plm;
    plm.setPosition(Base::Vector3d(1, 2, 3));
    plm.setRotation(Base::Rotation(1, 1, 2, 2));
    csT.transform(plm);
    csT.transform(plm);

    plm = plm * plm;

    Base::Placement dis = cs.displacement(csT);
    EXPECT_EQ(plm.isSame(dis, 0.001), true);

    Base::Placement disT = csT.displacement(cs);
    EXPECT_EQ(plm.inverse().isSame(disT, 0.001), true);
}

TEST(CoordinateSystem, TestTransformRotation)
{
    Base::CoordinateSystem cs;
    Base::CoordinateSystem csT;
    Base::Rotation rot(1, 1, 2, 2);
    csT.transform(rot);

    Base::Placement dis = cs.displacement(csT);
    EXPECT_EQ(rot.isSame(dis.getRotation(), 1e-15), true);

    Base::Placement disT = csT.displacement(cs);
    EXPECT_EQ(rot.inverse().isSame(disT.getRotation(), 1e-15), true);
}

TEST(CoordinateSystem, TestTransformPoint)
{
    Base::CoordinateSystem cs;
    Base::CoordinateSystem csT;
    Base::Placement plm;
    plm.setPosition(Base::Vector3d(1, 2, 3));
    plm.setRotation(Base::Rotation(1, 1, 2, 2));
    csT.transform(plm);

    Base::Vector3d src(-1, 5, 3), dst;
    plm.inverse().multVec(src, dst);
    csT.transformTo(src);

    EXPECT_EQ(src, dst);
}

TEST(CoordinateSystem, TestSetPlacement)
{
    Base::CoordinateSystem cs;
    Base::CoordinateSystem csT;
    Base::Placement plm;
    plm.setPosition(Base::Vector3d(1, 2, 3));
    plm.setRotation(Base::Rotation(1, 1, 2, 2));
    csT.transform(plm);
    csT.transform(plm);
    csT.transform(plm);
    csT.setPlacement(plm);

    Base::Placement dis = cs.displacement(csT);
    EXPECT_EQ(plm.isSame(dis, 1e-15), true);

    Base::Placement disT = csT.displacement(cs);
    EXPECT_EQ(plm.inverse().isSame(disT, 1e-15), true);
}

// NOLINTEND
