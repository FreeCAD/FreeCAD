#include "gtest/gtest.h"
#include <Base/Vector3D.h>

TEST(Vector, TestDefault)
{
    Base::Vector3d vec;
    EXPECT_EQ(vec.x, 0.0);
    EXPECT_EQ(vec.y, 0.0);
    EXPECT_EQ(vec.z, 0.0);
}

TEST(Vector, TestDefault2)
{
    Base::Vector3d vec(1.0, 2.0, 3.0);
    EXPECT_EQ(vec.x, 1.0);
    EXPECT_EQ(vec.y, 2.0);
    EXPECT_EQ(vec.z, 3.0);
}

TEST(Vector, TestCopy)
{
    Base::Vector3d vec(1.0, 2.0, 3.0);
    Base::Vector3d copy(vec);
    EXPECT_EQ(copy.x, 1.0);
    EXPECT_EQ(copy.y, 2.0);
    EXPECT_EQ(copy.z, 3.0);
}

TEST(Vector, TestMove)
{
    Base::Vector3d vec(1.0, 2.0, 3.0);
    Base::Vector3d copy(std::move(vec));
    EXPECT_EQ(copy.x, 1.0);
    EXPECT_EQ(copy.y, 2.0);
    EXPECT_EQ(copy.z, 3.0);
}

TEST(Vector, TestNull)
{
    Base::Vector3d vec(1.0, 2.0, 3.0);
    EXPECT_EQ(vec.IsNull(), false);
    EXPECT_EQ(Base::Vector3d().IsNull(), true);
}

TEST(Vector, TestEqual)
{
    Base::Vector3d vec(1.0, 2.0, 3.0);
    EXPECT_EQ(vec.IsEqual(vec, 0), true);
    EXPECT_EQ(Base::Vector3d().IsEqual(vec, 0), false);
}

TEST(Vector, TestIndex)
{
    Base::Vector3d vec(1.0, 2.0, 3.0);
    EXPECT_EQ(vec[0], 1.0);
    EXPECT_EQ(vec[1], 2.0);
    EXPECT_EQ(vec[2], 3.0);
    vec[0] = 4.0;
    EXPECT_EQ(vec[0], 4.0);
}

TEST(Vector, TestPlus)
{
    Base::Vector3d vec = Base::Vector3d(1, 2, 3) + Base::Vector3d(4, 3, 1);
    EXPECT_EQ(vec.x, 5.0);
    EXPECT_EQ(vec.y, 5.0);
    EXPECT_EQ(vec.z, 4.0);
}

TEST(Vector, TestPlusAssign)
{
    Base::Vector3d vec(1, 2, 3);
    vec += Base::Vector3d(4, 3, 1);
    EXPECT_EQ(vec.x, 5.0);
    EXPECT_EQ(vec.y, 5.0);
    EXPECT_EQ(vec.z, 4.0);
}

TEST(Vector, TestMinus)
{
    Base::Vector3d vec = Base::Vector3d(1, 2, 3) - Base::Vector3d(4, 3, 1);
    EXPECT_EQ(vec.x, -3.0);
    EXPECT_EQ(vec.y, -1.0);
    EXPECT_EQ(vec.z, 2.0);
}

TEST(Vector, TestMinusAssign)
{
    Base::Vector3d vec(1, 2, 3);
    vec -= Base::Vector3d(4, 3, 1);
    EXPECT_EQ(vec.x, -3.0);
    EXPECT_EQ(vec.y, -1.0);
    EXPECT_EQ(vec.z, 2.0);
}

TEST(Vector, TestNegative)
{
    Base::Vector3d vec = -Base::Vector3d(4, 3, 1);
    EXPECT_EQ(vec.x, -4.0);
    EXPECT_EQ(vec.y, -3.0);
    EXPECT_EQ(vec.z, -1.0);
}

TEST(Vector, TestScale)
{
    Base::Vector3d vec(1, 2, 3);
    vec = vec * 2.0;
    EXPECT_EQ(vec.x, 2.0);
    EXPECT_EQ(vec.y, 4.0);
    EXPECT_EQ(vec.z, 6.0);
}

TEST(Vector, TestScaleAssign)
{
    Base::Vector3d vec(1, 2, 3);
    vec *= 2.0;
    EXPECT_EQ(vec.x, 2.0);
    EXPECT_EQ(vec.y, 4.0);
    EXPECT_EQ(vec.z, 6.0);
}

TEST(Vector, TestDivScale)
{
    Base::Vector3d vec(1, 2, 3);
    vec = vec / 2.0;
    EXPECT_EQ(vec.x, 0.5);
    EXPECT_EQ(vec.y, 1.0);
    EXPECT_EQ(vec.z, 1.5);
}

TEST(Vector, TestDivScaleAssign)
{
    Base::Vector3d vec(1, 2, 3);
    vec /= 2.0;
    EXPECT_EQ(vec.x, 0.5);
    EXPECT_EQ(vec.y, 1.0);
    EXPECT_EQ(vec.z, 1.5);
}

TEST(Vector, TestScaleXYZ)
{
    Base::Vector3d vec(1, 2, 3);
    vec.Scale(1, 2, 3);
    EXPECT_EQ(vec.x, 1.0);
    EXPECT_EQ(vec.y, 4.0);
    EXPECT_EQ(vec.z, 9.0);
}

TEST(Vector, TestMoveXYZ)
{
    Base::Vector3d vec(1, 2, 3);
    vec.Move(1, 2, 3);
    EXPECT_EQ(vec.x, 2.0);
    EXPECT_EQ(vec.y, 4.0);
    EXPECT_EQ(vec.z, 6.0);
}

TEST(Vector, TestCopyAssign)
{
    Base::Vector3d vec;
    Base::Vector3d copy(1, 2, 3);
    vec = copy;
    EXPECT_EQ(vec.x, 1.0);
    EXPECT_EQ(vec.y, 2.0);
    EXPECT_EQ(vec.z, 3.0);
}

TEST(Vector, TestMoveAssign)
{
    Base::Vector3d vec;
    Base::Vector3d copy(1, 2, 3);
    vec = std::move(copy);
    EXPECT_EQ(vec.x, 1.0);
    EXPECT_EQ(vec.y, 2.0);
    EXPECT_EQ(vec.z, 3.0);
}

TEST(Vector, TestScalar)
{
    Base::Vector3d vec1(1, 2, 3);
    Base::Vector3d vec2(3, 2, 1);
    double dot1 = vec1 * vec2;
    double dot2 = vec2.Dot(vec1);
    EXPECT_EQ(dot1, 10.0);
    EXPECT_EQ(dot2, 10.0);
}

TEST(Vector, TestCross)
{
    Base::Vector3d vec1(1, 2, 3);
    Base::Vector3d vec2(3, 2, 1);
    Base::Vector3d cross1 = vec1 % vec2;
    Base::Vector3d cross2 = vec2.Cross(vec1);
    EXPECT_EQ(cross1.x, -4.0);
    EXPECT_EQ(cross1.y, 8.0);
    EXPECT_EQ(cross1.z, -4.0);
    EXPECT_EQ(cross2.x, 4.0);
    EXPECT_EQ(cross2.y, -8.0);
    EXPECT_EQ(cross2.z, 4.0);
}

TEST(Vector, TestCompare)
{
    Base::Vector3d vec1(1, 2, 3);
    Base::Vector3d vec2(1, 2, 1);
    EXPECT_EQ(vec1 == vec2, false);
    EXPECT_EQ(vec1 != vec2, true);
}

TEST(Vector, TestNormalize)
{
    Base::Vector3d vec(1, 2, 3);
    vec.Normalize();
    EXPECT_EQ(vec.Length(), 1.0);
}

TEST(Vector, TestCSTransform)
{
    Base::Vector3d vec(1, 2, 3);
    vec.TransformToCoordinateSystem(Base::Vector3d(1, 1, 1),
                                    Base::Vector3d(0, 1, 0),
                                    Base::Vector3d(1, 0, 0));
    EXPECT_EQ(vec.x, 1);
    EXPECT_EQ(vec.y, 0);
    EXPECT_EQ(vec.z, -2);
}

TEST(Vector, TestLineSegment)
{
    Base::Vector3d vec(1, 2, 3);
    EXPECT_EQ(vec.IsOnLineSegment(Base::Vector3d(), Base::Vector3d(2, 4, 6)), true);
}

TEST(Vector, TestDistanceLineSegment)
{
    Base::Vector3d vec(1, 2, 3);
    EXPECT_EQ(vec.DistanceToLineSegment(Base::Vector3d(), Base::Vector3d(2, 4, 6)).Length(), 0.0);
}

TEST(Vector, TestProjectToPlane)
{
    Base::Vector3d vec(1, 2, 3);
    Base::Vector3d proj;
    vec.ProjectToPlane(Base::Vector3d(1, 1, 1), Base::Vector3d(0, 0, 1), proj);
    EXPECT_EQ(proj.x, 1);
    EXPECT_EQ(proj.y, 2);
    EXPECT_EQ(proj.z, 1);
}

TEST(Vector, TestDistanceToPlane)
{
    Base::Vector3d vec(1, 2, 3);
    double dist = vec.DistanceToPlane(Base::Vector3d(1, 1, 1), Base::Vector3d(0, 0, 1));
    EXPECT_EQ(dist, 2);
}

TEST(Vector, TestAngle)
{
    Base::Vector3d vec1(0, 0, 0.000001);
    Base::Vector3d vec2(0, 0.000001, 0);
    double angle = vec1.GetAngle(vec2);
    EXPECT_EQ(angle, Base::float_traits<double>::pi() / 2);
}
