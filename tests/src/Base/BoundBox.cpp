#include "gtest/gtest.h"
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif
#include <Base/BoundBox.h>
#include <boost/beast/core/span.hpp>

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)
TEST(BoundBox, TestDefault)
{
    Base::BoundBox3d box;
    EXPECT_EQ(box.IsValid(), false);
}

TEST(BoundBox, TestVoid)
{
    Base::BoundBox3d box;
    EXPECT_EQ(box.IsValid(), false);
    box.Add(Base::Vector3d(0, 0, 0));
    EXPECT_EQ(box.IsValid(), true);
    box.SetVoid();
    EXPECT_EQ(box.IsValid(), false);
}

TEST(BoundBox, TestCopy)
{
    Base::BoundBox3d box;
    box.Add(Base::Vector3d(0, 0, 0));
    box.Add(Base::Vector3d(1, 1, 1));
    Base::BoundBox3d copy(box);
    EXPECT_EQ(copy.IsValid(), true);
    EXPECT_EQ(copy.MinX, 0);
    EXPECT_EQ(copy.MinY, 0);
    EXPECT_EQ(copy.MinZ, 0);
    EXPECT_EQ(copy.MaxX, 1);
    EXPECT_EQ(copy.MaxY, 1);
    EXPECT_EQ(copy.MaxZ, 1);
}

TEST(BoundBox, TestAssign)
{
    Base::BoundBox3d box;
    box.Add(Base::Vector3d(0, 0, 0));
    box.Add(Base::Vector3d(1, 1, 1));
    Base::BoundBox3d copy;
    copy = box;
    EXPECT_EQ(copy.IsValid(), true);
    EXPECT_EQ(copy.MinX, 0);
    EXPECT_EQ(copy.MinY, 0);
    EXPECT_EQ(copy.MinZ, 0);
    EXPECT_EQ(copy.MaxX, 1);
    EXPECT_EQ(copy.MaxY, 1);
    EXPECT_EQ(copy.MaxZ, 1);
}

TEST(BoundBox, TestPoints)
{
    std::array<Base::Vector3d, 1> pts;
    Base::BoundBox3d box(pts.data(), pts.size());
    EXPECT_EQ(box.IsValid(), true);
    EXPECT_EQ(box.MinX, 0);
    EXPECT_EQ(box.MinY, 0);
    EXPECT_EQ(box.MinZ, 0);
    EXPECT_EQ(box.MaxX, 0);
    EXPECT_EQ(box.MaxY, 0);
    EXPECT_EQ(box.MaxZ, 0);
}

TEST(BoundBox, TestDistance)
{
    Base::BoundBox3d box(Base::Vector3d(0, 0, 0), 0.5);
    EXPECT_EQ(box.IsValid(), true);
    EXPECT_EQ(box.MinX, -0.5);
    EXPECT_EQ(box.MinY, -0.5);
    EXPECT_EQ(box.MinZ, -0.5);
    EXPECT_EQ(box.MaxX, 0.5);
    EXPECT_EQ(box.MaxY, 0.5);
    EXPECT_EQ(box.MaxZ, 0.5);
}

Base::BoundBox3d createBox(boost::beast::span<Base::Vector3d> span)
{
    return {span.data(), span.size()};
}

TEST(BoundBox, TestEmptySpan)
{
    Base::BoundBox3d box = createBox({});
    EXPECT_EQ(box.IsValid(), false);
}

TEST(BoundBox, TestNonEmptySpan)
{
    std::vector<Base::Vector3d> points = {Base::Vector3d {1.0, 0.0, 0.0},
                                          Base::Vector3d {0.0, 1.0, 0.0},
                                          Base::Vector3d {0.0, 0.0, 1.0}};
    Base::BoundBox3d box = createBox({points.data(), 2});
    EXPECT_EQ(box.IsValid(), true);
    EXPECT_EQ(box.MinX, 0);
    EXPECT_EQ(box.MinY, 0);
    EXPECT_EQ(box.MinZ, 0);
    EXPECT_EQ(box.MaxX, 1);
    EXPECT_EQ(box.MaxY, 1);
    EXPECT_EQ(box.MaxZ, 0);
}

TEST(BoundBox, TestArray)
{
    std::vector<Base::Vector3d> points = {Base::Vector3d {1.0, 0.0, 0.0},
                                          Base::Vector3d {0.0, 1.0, 0.0},
                                          Base::Vector3d {0.0, 0.0, 1.0}};
    Base::BoundBox3d box(points.data(), points.size());
    EXPECT_EQ(box.IsValid(), true);
    EXPECT_EQ(box.MinX, 0);
    EXPECT_EQ(box.MinY, 0);
    EXPECT_EQ(box.MinZ, 0);
    EXPECT_EQ(box.MaxX, 1);
    EXPECT_EQ(box.MaxY, 1);
    EXPECT_EQ(box.MaxZ, 1);
}

TEST(BoundBox, TestIntersect)
{
    Base::BoundBox3d box1;
    Base::BoundBox3d box2;
    Base::BoundBox3d box3;
    EXPECT_EQ(box1.Intersect(box1), false);
    box1.Add(Base::Vector3d(0, 0, 0));
    EXPECT_EQ(box1.Intersect(box1), true);
    EXPECT_EQ(box1.Intersect(box2), false);
    EXPECT_EQ(box2.Intersect(box1), false);

    box1.Add(Base::Vector3d(1, 1, 1));
    box2.Add(Base::Vector3d(0.5, 0.5, 0.5));
    box2.Add(Base::Vector3d(1.5, 1.5, 1.5));
    EXPECT_EQ(box1.Intersect(box2), true);
    EXPECT_EQ(box2.Intersect(box1), true);
    EXPECT_EQ(box1 && box2, true);
    EXPECT_EQ(box2 && box1, true);

    box3 = box1.Intersected(box2);
    EXPECT_EQ(box3.IsValid(), true);
}

TEST(BoundBox, TestIntersect2D)
{
    Base::BoundBox3d box1;
    box1.Add(Base::Vector3d(0, 0, 0));
    box1.Add(Base::Vector3d(1, 1, 1));

    Base::BoundBox2d box2;
    box2.Add(Base::Vector2d(0.5, 0.5));
    box2.Add(Base::Vector2d(1.5, 1.5));
    EXPECT_EQ(box1.Intersect(box2), true);
    EXPECT_EQ(box1 && box2, true);
}

TEST(BoundBox, TestUnite)
{
    Base::BoundBox3d box1;
    Base::BoundBox3d box2;
    Base::BoundBox3d box3;
    box1.Add(Base::Vector3d(0, 0, 0));
    box2.Add(Base::Vector3d(1, 1, 1));
    box3 = box1.United(box2);

    EXPECT_EQ(box3.IsValid(), true);
    EXPECT_EQ(box3.MinX, 0);
    EXPECT_EQ(box3.MinY, 0);
    EXPECT_EQ(box3.MinZ, 0);
    EXPECT_EQ(box3.MaxX, 1);
    EXPECT_EQ(box3.MaxY, 1);
    EXPECT_EQ(box3.MaxZ, 1);

    box1.Add(box2);
    EXPECT_EQ(box1.IsValid(), true);
    EXPECT_EQ(box1.MinX, 0);
    EXPECT_EQ(box1.MinY, 0);
    EXPECT_EQ(box1.MinZ, 0);
    EXPECT_EQ(box1.MaxX, 1);
    EXPECT_EQ(box1.MaxY, 1);
    EXPECT_EQ(box1.MaxZ, 1);
}

TEST(BoundBox, TestAdd)
{
    Base::BoundBox3d box1;
    Base::BoundBox3d box2;
    box1.Add(Base::Vector3d(0, 0, 0));
    box2.Add(Base::Vector3d(1, 1, 1));

    box1.Add(box2);
    EXPECT_EQ(box1.IsInBox(box2), true);
    EXPECT_EQ(box1.IsInBox(Base::Vector3d(0, 0, 0)), true);
    EXPECT_EQ(box1.IsInBox(Base::Vector3d(1, 1, 1)), true);
}

TEST(BoundBox, TestCenter)
{
    Base::BoundBox3d box;
    box.Add(Base::Vector3d(0, 0, 0));
    box.Add(Base::Vector3d(1, 1, 1));

    EXPECT_EQ(box.GetCenter(), Base::Vector3d(0.5, 0.5, 0.5));
}

TEST(BoundBox, TestMinimum)
{
    Base::BoundBox3d box;
    box.Add(Base::Vector3d(0, 0, 0));
    box.Add(Base::Vector3d(1, 1, 1));

    EXPECT_EQ(box.GetMinimum(), Base::Vector3d(0, 0, 0));
}

TEST(BoundBox, TestMaximum)
{
    Base::BoundBox3d box;
    box.Add(Base::Vector3d(0, 0, 0));
    box.Add(Base::Vector3d(1, 1, 1));

    EXPECT_EQ(box.GetMaximum(), Base::Vector3d(1, 1, 1));
}

TEST(BoundBox, TestDiagonalLength)
{
    Base::BoundBox3d box;
    box.Add(Base::Vector3d(0, 0, 0));
    box.Add(Base::Vector3d(1, 1, 1));

    EXPECT_GT(box.CalcDiagonalLength(), 1.7);
    EXPECT_LT(box.CalcDiagonalLength(), 1.8);
}

TEST(BoundBox, TestVolume)
{
    Base::BoundBox3d box;
    EXPECT_LT(box.Volume(), 0.0);
    box.Add(Base::Vector3d(0, 0, 0));
    EXPECT_EQ(box.Volume(), 0.0);
    box.Add(Base::Vector3d(1, 1, 1));
    EXPECT_GT(box.Volume(), 0.0);
}

TEST(BoundBox, TestDiagonalEnlarge)
{
    Base::BoundBox3d box;
    box.Add(Base::Vector3d(1, 1, 1));
    box.Enlarge(0.5);

    EXPECT_EQ(box.LengthX(), 1.0);
    EXPECT_EQ(box.LengthY(), 1.0);
    EXPECT_EQ(box.LengthZ(), 1.0);
}

TEST(BoundBox, TestDiagonalShrink)
{
    Base::BoundBox3d box;
    box.Add(Base::Vector3d(0, 0, 0));
    box.Add(Base::Vector3d(4, 6, 8));
    box.Shrink(0.5);

    EXPECT_EQ(box.LengthX(), 3.0);
    EXPECT_EQ(box.LengthY(), 5.0);
    EXPECT_EQ(box.LengthZ(), 7.0);
}

TEST(BoundBox, TestDiagonalMove)
{
    Base::BoundBox3d box;
    box.Add(Base::Vector3d(0, 0, 0));
    box.MoveX(1.0);
    box.MoveY(2.0);
    box.MoveZ(3.0);

    EXPECT_EQ(box.MinX, 1);
    EXPECT_EQ(box.MinY, 2);
    EXPECT_EQ(box.MinZ, 3);
    EXPECT_EQ(box.MaxX, 1);
    EXPECT_EQ(box.MaxY, 2);
    EXPECT_EQ(box.MaxZ, 3);
}

TEST(BoundBox, TestDiagonalScale)
{
    Base::BoundBox3d box;
    box.Add(Base::Vector3d(0, 0, 0));
    box.Add(Base::Vector3d(1, 2, 3));
    box.ScaleX(0.5);
    box.ScaleY(1.0);
    box.ScaleZ(2.0);

    EXPECT_EQ(box.MinX, 0);
    EXPECT_EQ(box.MinY, 0);
    EXPECT_EQ(box.MinZ, 0);
    EXPECT_EQ(box.MaxX, 0.5);
    EXPECT_EQ(box.MaxY, 2.0);
    EXPECT_EQ(box.MaxZ, 6.0);
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
