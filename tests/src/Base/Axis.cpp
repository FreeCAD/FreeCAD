#include "gtest/gtest.h"
#include <Base/Axis.h>

TEST(Axis, TestDefault)
{
    Base::Axis axis;
    EXPECT_EQ(axis.getBase(), Base::Vector3d());
    EXPECT_EQ(axis.getDirection(), Base::Vector3d());
}

TEST(Axis, TestCustom)
{
    Base::Axis axis(Base::Vector3d(0, 0, 1), Base::Vector3d(1, 1, 1));
    EXPECT_EQ(axis.getBase(), Base::Vector3d(0, 0, 1));
    EXPECT_EQ(axis.getDirection(), Base::Vector3d(1, 1, 1));
}

TEST(Axis, TestSetter)
{
    Base::Axis axis;
    axis.setBase(Base::Vector3d(0, 0, 1));
    axis.setDirection(Base::Vector3d(1, 1, 1));
    EXPECT_EQ(axis.getBase(), Base::Vector3d(0, 0, 1));
    EXPECT_EQ(axis.getDirection(), Base::Vector3d(1, 1, 1));
}

TEST(Axis, TestAssign)
{
    Base::Axis axis;
    Base::Axis move;
    axis.setBase(Base::Vector3d(0, 0, 1));
    axis.setDirection(Base::Vector3d(1, 1, 1));
    move = std::move(axis);  // NOLINT
    EXPECT_EQ(move.getBase(), Base::Vector3d(0, 0, 1));
    EXPECT_EQ(move.getDirection(), Base::Vector3d(1, 1, 1));
}

TEST(Axis, TestReverse)
{
    Base::Axis axis(Base::Vector3d(0, 0, 1), Base::Vector3d(1, 1, 1));
    axis.reverse();
    EXPECT_EQ(axis.getBase(), Base::Vector3d(0, 0, 1));
    EXPECT_EQ(axis.getDirection(), Base::Vector3d(-1, -1, -1));
}

TEST(Axis, TestReversed)
{
    Base::Axis axis(Base::Vector3d(0, 0, 1), Base::Vector3d(1, 1, 1));
    Base::Axis rev(axis.reversed());
    EXPECT_EQ(axis.getDirection(), Base::Vector3d(1, 1, 1));
    EXPECT_EQ(rev.getBase(), Base::Vector3d(0, 0, 1));
    EXPECT_EQ(rev.getDirection(), Base::Vector3d(-1, -1, -1));
}

TEST(Axis, TestMove)
{
    Base::Axis axis(Base::Vector3d(0, 0, 1), Base::Vector3d(1, 1, 1));
    axis.move(Base::Vector3d(1, 2, 3));
    EXPECT_EQ(axis.getBase(), Base::Vector3d(1, 2, 4));
    EXPECT_EQ(axis.getDirection(), Base::Vector3d(1, 1, 1));
}

TEST(Axis, TestMult)
{
    Base::Axis axis(Base::Vector3d(0, 0, 1), Base::Vector3d(0, 0, 1));
    axis *= Base::Placement(Base::Vector3d(1, 1, 1), Base::Rotation(1, 0, 0, 0));
    EXPECT_EQ(axis.getBase(), Base::Vector3d(1, 1, 0));
    EXPECT_EQ(axis.getDirection(), Base::Vector3d(0, 0, -1));
}
