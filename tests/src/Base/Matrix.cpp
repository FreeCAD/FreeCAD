#include "gtest/gtest.h"
#include <Base/Matrix.h>
#include <Base/Rotation.h>

// NOLINTBEGIN(cppcoreguidelines-*,readability-magic-numbers)
// clang-format off
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

TEST(Matrix, TestDiagonal)
{
    Base::Matrix4D mat;
    mat.scale(2.0, 2.0, 2.0);
    Base::Vector3d diag = mat.diagonal();
    EXPECT_EQ(diag.x + diag.y + diag.z, 6.0);
}

TEST(Matrix, TestColRow)
{
    Base::Matrix4D mat;
    EXPECT_EQ(mat.getCol(0), Base::Vector3d(1, 0, 0));
    EXPECT_EQ(mat.getCol(1), Base::Vector3d(0, 1, 0));
    EXPECT_EQ(mat.getCol(2), Base::Vector3d(0, 0, 1));
    EXPECT_EQ(mat.getRow(0), Base::Vector3d(1, 0, 0));
    EXPECT_EQ(mat.getRow(1), Base::Vector3d(0, 1, 0));
    EXPECT_EQ(mat.getRow(2), Base::Vector3d(0, 0, 1));
}

TEST(Matrix, TestColRowMisc)
{
    Base::Matrix4D mat{1.0, 2.0, 3.0, 0.0,
                       3.0, 1.0, 2.0, 2.0,
                       2.0, 3.0, 1.0, 1.0,
                       0.0, 0.0, 0.0, 1.0};
    EXPECT_EQ(mat.getCol(0), Base::Vector3d(1.0, 3.0, 2.0));
    EXPECT_EQ(mat.getCol(1), Base::Vector3d(2.0, 1.0, 3.0));
    EXPECT_EQ(mat.getCol(2), Base::Vector3d(3.0, 2.0, 1.0));
    EXPECT_EQ(mat.getRow(0), Base::Vector3d(1.0, 2.0, 3.0));
    EXPECT_EQ(mat.getRow(1), Base::Vector3d(3.0, 1.0, 2.0));
    EXPECT_EQ(mat.getRow(2), Base::Vector3d(2.0, 3.0, 1.0));
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

TEST(Matrix, TestMultVec)
{
    Base::Matrix4D mat{1.0, 2.0, 3.0, 0.0,
                       2.0, 3.0, 1.0, 1.0,
                       3.0, 1.0, 2.0, 2.0,
                       0.0, 0.0, 0.0, 1.0};
    Base::Vector3d vec1{1,1,3};
    vec1 = mat * vec1;
    EXPECT_EQ(vec1, Base::Vector3d(12.0, 9.0, 12.0));

    Base::Vector3d vec2 {1, 1, 1};
    mat.multVec(vec2, vec2);
    EXPECT_EQ(vec2, Base::Vector3d(6.0, 7.0, 8.0));
}

TEST(Matrix, TestMult)
{
    Base::Matrix4D mat1{1.0, 2.0, 3.0, 0.0,
                        2.0, 3.0, 1.0, 1.0,
                        3.0, 1.0, 2.0, 2.0,
                        0.0, 0.0, 0.0, 1.0};
    Base::Matrix4D mat2{1.0, 2.0, 3.0, 0.0,
                        3.0, 1.0, 2.0, 1.0,
                        2.0, 3.0, 1.0, 2.0,
                        0.0, 0.0, 0.0, 1.0};
    Base::Matrix4D mat3 = mat1 * mat2;
    Base::Matrix4D mat4{13.0, 13.0, 10.0, 8.0,
                        13.0, 10.0, 13.0, 6.0,
                        10.0, 13.0, 13.0, 7.0,
                        0.0, 0.0, 0.0, 1.0};
    EXPECT_EQ(mat3, mat4);
}

TEST(Matrix, TestMultAssign)
{
    Base::Matrix4D mat1{1.0, 2.0, 3.0, 0.0,
                        2.0, 3.0, 1.0, 1.0,
                        3.0, 1.0, 2.0, 2.0,
                        0.0, 0.0, 0.0, 1.0};
    Base::Matrix4D mat2{1.0, 2.0, 3.0, 0.0,
                        3.0, 1.0, 2.0, 1.0,
                        2.0, 3.0, 1.0, 2.0,
                        0.0, 0.0, 0.0, 1.0};
    mat1 *= mat2;
    Base::Matrix4D mat4{13.0, 13.0, 10.0, 8.0,
                        13.0, 10.0, 13.0, 6.0,
                        10.0, 13.0, 13.0, 7.0,
                        0.0, 0.0, 0.0, 1.0};
    EXPECT_EQ(mat1, mat4);
}

TEST(Matrix, TestAdd)
{
    Base::Matrix4D mat1{1.0, 2.0, 3.0, 0.0,
                        2.0, 3.0, 1.0, 1.0,
                        3.0, 1.0, 2.0, 2.0,
                        0.0, 0.0, 0.0, 1.0};
    Base::Matrix4D mat2{1.0, 2.0, 3.0, 0.0,
                        3.0, 1.0, 2.0, 1.0,
                        2.0, 3.0, 1.0, 2.0,
                        0.0, 0.0, 0.0, 1.0};
    Base::Matrix4D mat3 = mat1 + mat2;
    Base::Matrix4D mat4{2.0, 4.0, 6.0, 0.0,
                        5.0, 4.0, 3.0, 2.0,
                        5.0, 4.0, 3.0, 4.0,
                        0.0, 0.0, 0.0, 2.0};
    EXPECT_EQ(mat3, mat4);
}

TEST(Matrix, TestAddAssign)
{
    Base::Matrix4D mat1{1.0, 2.0, 3.0, 0.0,
                        2.0, 3.0, 1.0, 1.0,
                        3.0, 1.0, 2.0, 2.0,
                        0.0, 0.0, 0.0, 1.0};
    Base::Matrix4D mat2{1.0, 2.0, 3.0, 0.0,
                        3.0, 1.0, 2.0, 1.0,
                        2.0, 3.0, 1.0, 2.0,
                        0.0, 0.0, 0.0, 1.0};
    mat1 += mat2;
    Base::Matrix4D mat4{2.0, 4.0, 6.0, 0.0,
                        5.0, 4.0, 3.0, 2.0,
                        5.0, 4.0, 3.0, 4.0,
                        0.0, 0.0, 0.0, 2.0};
    EXPECT_EQ(mat1, mat4);
}

TEST(Matrix, TestSub)
{
    Base::Matrix4D mat1{1.0, 2.0, 3.0, 0.0,
                        2.0, 3.0, 1.0, 1.0,
                        3.0, 1.0, 2.0, 2.0,
                        0.0, 0.0, 0.0, 1.0};
    Base::Matrix4D mat2{1.0, 2.0, 3.0, 0.0,
                        3.0, 1.0, 2.0, 1.0,
                        2.0, 3.0, 1.0, 2.0,
                        0.0, 0.0, 0.0, 1.0};
    Base::Matrix4D mat3 = mat1 - mat2;
    Base::Matrix4D mat4{0.0, 0.0, 0.0, 0.0,
                        -1.0, 2.0, -1.0, 0.0,
                        1.0, -2.0, 1.0, 0.0,
                        0.0, 0.0, 0.0, 0.0};
    EXPECT_EQ(mat3, mat4);
}

TEST(Matrix, TestSubAssign)
{
    Base::Matrix4D mat1{1.0, 2.0, 3.0, 0.0,
                        2.0, 3.0, 1.0, 1.0,
                        3.0, 1.0, 2.0, 2.0,
                        0.0, 0.0, 0.0, 1.0};
    Base::Matrix4D mat2{1.0, 2.0, 3.0, 0.0,
                        3.0, 1.0, 2.0, 1.0,
                        2.0, 3.0, 1.0, 2.0,
                        0.0, 0.0, 0.0, 1.0};
    mat1 -= mat2;
    Base::Matrix4D mat4{0.0, 0.0, 0.0, 0.0,
                        -1.0, 2.0, -1.0, 0.0,
                        1.0, -2.0, 1.0, 0.0,
                        0.0, 0.0, 0.0, 0.0};
    EXPECT_EQ(mat1, mat4);
}

TEST(Matrix, TestHatOperator)
{
    Base::Vector3d vec{1.0, 2.0, 3.0};

    Base::Matrix4D mat1;
    mat1.Hat(vec);

    Base::Matrix4D mat2{0.0, -vec.z, vec.y, 0.0,
                        vec.z, 0.0, -vec.x, 0.0,
                        -vec.y, vec.x, 0.0, 0.0,
                        0.0, 0.0, 0.0, 1.0};

    EXPECT_EQ(mat1, mat2);
}

TEST(Matrix, TestDyadic)
{
    Base::Vector3d vec{1.0, 2.0, 3.0};

    Base::Matrix4D mat1;
    mat1.Outer(vec, vec);

    Base::Matrix4D mat2{1.0, 2.0, 3.0, 0.0,
                        2.0, 4.0, 6.0, 0.0,
                        3.0, 6.0, 9.0, 0.0,
                        0.0, 0.0, 0.0, 1.0};

    EXPECT_EQ(mat1, mat2);
}

TEST(Matrix, TestRotLine)
{
    Base::Vector3d axis{1.0, 2.0, 3.0};
    double angle = 1.2345;

    Base::Matrix4D mat1;
    Base::Matrix4D mat2;
    mat1.rotLine(axis, angle);

    Base::Rotation rot(axis, angle);
    rot.getValue(mat2);

    EXPECT_EQ(mat1, mat2);
}

TEST(Matrix, TestRotAxisFormula) //NOLINT
{
    // R = I + sin(alpha)*P + (1-cos(alpha))*P^2
    // with P = hat operator of a vector
    Base::Vector3d axis{1.0, 2.0, 3.0};
    double angle = 1.2345;

    Base::Matrix4D mat1;
    Base::Matrix4D mat2;
    mat1.rotLine(axis, angle);

    double fsin = std::sin(angle);
    double fcos = std::cos(angle);
    Base::Matrix4D unit;
    Base::Matrix4D hat;
    Base::Matrix4D hat2;

    axis.Normalize();
    hat.Hat(axis);

    hat2 = hat * hat;
    mat2 = unit + hat * fsin + hat2 * (1 - fcos);

    EXPECT_DOUBLE_EQ(mat1[0][0], mat2[0][0]);
    EXPECT_DOUBLE_EQ(mat1[0][1], mat2[0][1]);
    EXPECT_DOUBLE_EQ(mat1[0][2], mat2[0][2]);
    EXPECT_DOUBLE_EQ(mat1[1][0], mat2[1][0]);
    EXPECT_DOUBLE_EQ(mat1[1][1], mat2[1][1]);
    EXPECT_DOUBLE_EQ(mat1[1][2], mat2[1][2]);
    EXPECT_DOUBLE_EQ(mat1[2][0], mat2[2][0]);
    EXPECT_DOUBLE_EQ(mat1[2][1], mat2[2][1]);
    EXPECT_DOUBLE_EQ(mat1[2][2], mat2[2][2]);
}
// clang-format on
// NOLINTEND(cppcoreguidelines-*,readability-magic-numbers)
