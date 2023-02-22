#include "gtest/gtest.h"
#include <Base/Matrix.h>


TEST(Matrix, TestShearing)
{
    Base::Matrix4D mat;
    mat.setRow(1, Base::Vector3d(0, 1, 1));
    EXPECT_EQ(mat.hasScale(), Base::ScaleType::Other);
}

TEST(Matrix, TestMultLeftRight)
{
    Base::Matrix4D mat1;
    mat1.rotX(1.0);
    Base::Matrix4D mat2;
    mat2.scale(1.0, 2.0, 3.0);
    EXPECT_EQ((mat1 * mat2).hasScale(), Base::ScaleType::NonUniformRight);
    EXPECT_EQ((mat2 * mat1).hasScale(), Base::ScaleType::NonUniformLeft);
}

TEST(Matrix, TestNoScale)
{
    Base::Matrix4D mat;
    mat.rotX(1.0);
    mat.scale(1.0);
    EXPECT_EQ(mat.hasScale(), Base::ScaleType::NoScaling);
}

TEST(Matrix, TestUniformScalePlusTwo)
{
    Base::Matrix4D mat;
    mat.rotX(1.0);
    mat.scale(2.0);
    EXPECT_EQ(mat.hasScale(), Base::ScaleType::Uniform);
}

TEST(Matrix, TestUniformScaleRight)
{
    Base::Matrix4D mat;
    mat.rotX(1.0);
    Base::Matrix4D scale;
    scale.scale(2.0);
    mat = mat * scale;
    EXPECT_EQ(mat.hasScale(), Base::ScaleType::Uniform);
}

TEST(Matrix, TestNonUniformScaleRight)
{
    Base::Matrix4D mat;
    mat.rotX(1.0);
    Base::Matrix4D scale;
    scale.scale(2.0, 1.0, 2.0);
    mat = mat * scale;
    EXPECT_EQ(mat.hasScale(), Base::ScaleType::NonUniformRight);
}

TEST(Matrix, TestUniformScaleMinusOne)
{
    Base::Matrix4D mat;
    mat.scale(-1.0);
    EXPECT_EQ(mat.hasScale(), Base::ScaleType::Uniform);
}

TEST(Matrix, TestUniformScaleMinusTwo)
{
    Base::Matrix4D mat;
    mat.scale(-2.0);
    EXPECT_EQ(mat.hasScale(), Base::ScaleType::Uniform);
}

TEST(Matrix, TestNonUniformScaleLeftOne)
{
    Base::Matrix4D mat;
    mat.rotX(1.0);
    mat.scale(1.0, -1.0, 1.0);
    // A scale(1,-1,1) is like scale(-1,-1,-1) * rotation
    EXPECT_EQ(mat.hasScale(), Base::ScaleType::Uniform);
}

TEST(Matrix, TestNonUniformScaleLeftTwo)
{
    Base::Matrix4D mat;
    mat.rotX(1.0);
    mat.scale(2.0, -2.0, 2.0);
    EXPECT_EQ(mat.hasScale(), Base::ScaleType::Uniform);
}

TEST(Matrix, TestTrace)
{
    Base::Matrix4D mat;
    mat.scale(2.0, 2.0, 2.0);
    Base::Vector3d trace = mat.trace();
    EXPECT_EQ(trace.x + trace.y + trace.z, 6.0);
}

TEST(Matrix, TestColRow)
{
    Base::Matrix4D mat;
    EXPECT_EQ(mat.getCol(1), Base::Vector3d(0, 1, 0));
    EXPECT_EQ(mat.getCol(2), Base::Vector3d(0, 0, 1));
}

TEST(Matrix, TestUnity)
{
    Base::Matrix4D mat;
    EXPECT_EQ(mat.isUnity(), true);

    mat.nullify();
    EXPECT_EQ(mat.isUnity(), false);

    mat.setToUnity();
    EXPECT_EQ(mat.isUnity(), true);
}

TEST(Matrix, TestNull)
{
    Base::Matrix4D mat;
    EXPECT_EQ(mat.isNull(), false);

    mat.nullify();
    EXPECT_EQ(mat.isNull(), true);
}
