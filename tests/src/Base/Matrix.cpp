#include <gtest/gtest.h>
#include <Base/Matrix.h>
#include <Base/Rotation.h>
#include <Base/Tools.h>

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

    Base::Vector3f vec3{1.0F,1.0F,3.0F};
    vec3 = mat * vec3;
    EXPECT_EQ(vec3, Base::Vector3f(12.0F, 9.0F, 12.0F));

    Base::Vector3f vec4 {1.0F, 1.0F, 1.0F};
    mat.multVec(vec4, vec4);
    EXPECT_EQ(vec4, Base::Vector3f(6.0F, 7.0F, 8.0F));

    Base::Vector3f vec5 {1.0F, 1.0F, 1.0F};
    vec5 *= mat;
    EXPECT_EQ(vec5, Base::Vector3f(6.0F, 7.0F, 8.0F));
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
    EXPECT_NE(mat3, Base::Matrix4D());
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

TEST(Matrix, TestHatOperatorFloat)
{
    Base::Vector3f vec{1.0, 2.0, 3.0};

    Base::Matrix4D mat1;
    mat1.Hat(vec);

    Base::Matrix4D mat2{0.0F, -vec.z, vec.y, 0.0F,
                        vec.z, 0.0F, -vec.x, 0.0F,
                        -vec.y, vec.x, 0.0F, 0.0F,
                        0.0F, 0.0F, 0.0F, 1.0F};

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

TEST(Matrix, TestDyadicFloat)
{
    Base::Vector3f vec{1.0F, 2.0F, 3.0F};

    Base::Matrix4D mat1;
    mat1.Outer(vec, vec);

    Base::Matrix4D mat2{1.0, 2.0, 3.0, 0.0,
                        2.0, 4.0, 6.0, 0.0,
                        3.0, 6.0, 9.0, 0.0,
                        0.0, 0.0, 0.0, 1.0};

    EXPECT_EQ(mat1, mat2);
}

TEST(Matrix, TestDecomposeScale)
{
    Base::Matrix4D mat;
    mat.scale(1.0, 2.0, 3.0);
    auto res = mat.decompose();

    EXPECT_TRUE(res[0].isUnity());
    EXPECT_EQ(res[1], mat);
    EXPECT_TRUE(res[2].isUnity());
    EXPECT_TRUE(res[3].isUnity());
}

TEST(Matrix, TestDecomposeRotation)
{
    Base::Matrix4D mat;
    mat.rotX(1.0);
    mat.rotY(1.0);
    mat.rotZ(1.0);
    auto res = mat.decompose();

    EXPECT_TRUE(res[0].isUnity(1e-12));
    EXPECT_TRUE(res[1].isUnity(1e-12));
    EXPECT_EQ(res[2], mat);
    EXPECT_TRUE(res[3].isUnity(1e-12));
}

TEST(Matrix, TestDecomposeMove)
{
    Base::Matrix4D mat;
    mat.move(1.0, 2.0, 3.0);
    auto res = mat.decompose();

    EXPECT_TRUE(res[0].isUnity());
    EXPECT_TRUE(res[1].isUnity());
    EXPECT_TRUE(res[2].isUnity());
    EXPECT_EQ(res[3], mat);
}

TEST(Matrix, TestDecomposeMoveFloat)
{
    Base::Matrix4D mat;
    mat.move(Base::Vector3f(1.0F, 2.0F, 3.0F));
    auto res = mat.decompose();

    EXPECT_TRUE(res[0].isUnity());
    EXPECT_TRUE(res[1].isUnity());
    EXPECT_TRUE(res[2].isUnity());
    EXPECT_EQ(res[3], mat);
}

TEST(Matrix, TestDecompose)
{
    Base::Matrix4D mat;
    mat[0][0] = 1.0;
    mat[0][3] = 1.0;
    mat[1][3] = 2.0;
    mat[2][3] = 3.0;
    auto res = mat.decompose();

    Base::Matrix4D mul = res[3] * res[2] * res[1] * res[0];
    EXPECT_EQ(mul, mat);

    // shearing
    EXPECT_DOUBLE_EQ(res[0].determinant3(), 1.0);
    // rotation
    EXPECT_DOUBLE_EQ(res[2].determinant3(), 1.0);
    // translation
    EXPECT_DOUBLE_EQ(res[2].determinant3(), 1.0);
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

TEST(Matrix, TestTransform)
{
    Base::Matrix4D mat;
    mat.rotZ(Base::toRadians(90.0));

    Base::Matrix4D unity;
    unity.transform(Base::Vector3d(10.0, 0.0, 0.0), mat);

    Base::Matrix4D mov{0.0, -1.0, 0.0, 10.0,
                       1.0, 0.0, 0.0, -10.0,
                       0.0, 0.0, 1.0, 0.0,
                       0.0, 0.0, 0.0, 1.0};
    EXPECT_EQ(unity, mov);
}

TEST(Matrix, TestTransformFloat)
{
    Base::Matrix4D mat;
    mat.rotZ(Base::toRadians(90.0));

    Base::Matrix4D mat2;
    mat2.transform(Base::Vector3f(10.0F, 0.0F, 0.0F), mat);

    Base::Matrix4D mov{0.0, -1.0, 0.0, 10.0,
                       1.0, 0.0, 0.0, -10.0,
                       0.0, 0.0, 1.0, 0.0,
                       0.0, 0.0, 0.0, 1.0};
    EXPECT_EQ(mat2, mov);
}

TEST(Matrix, TestInverseOrthogonal)
{
    Base::Matrix4D mat;
    mat.rotZ(Base::toRadians(90.0));

    Base::Matrix4D mat2;
    mat2.transform(Base::Vector3d(10.0, 0.0, 0.0), mat);
    mat2.inverseOrthogonal();

    Base::Matrix4D mov{0.0, 1.0, 0.0, 10.0,
                       -1.0, 0.0, 0.0, 10.0,
                       0.0, 0.0, 1.0, 0.0,
                       0.0, 0.0, 0.0, 1.0};
    EXPECT_EQ(mat2, mov);
}

TEST(Matrix, TestTranspose)
{
    Base::Matrix4D mat{1.0, 2.0, 3.0, 4.0,
                       5.0, 6.0, 7.0, 8.0,
                       9.0, 1.0, 2.0, 3.0,
                       4.0, 5.0, 6.0, 7.0};

    Base::Matrix4D trp{1.0, 5.0, 9.0, 4.0,
                       2.0, 6.0, 1.0, 5.0,
                       3.0, 7.0, 2.0, 6.0,
                       4.0, 8.0, 3.0, 7.0};

    mat.transpose();
    EXPECT_EQ(mat, trp);
}

TEST(Matrix, TestTrace)
{
    Base::Matrix4D mat{1.0, 2.0, 3.0, 4.0,
                       5.0, 6.0, 7.0, 8.0,
                       9.0, 1.0, 2.0, 3.0,
                       4.0, 5.0, 6.0, 7.0};
    EXPECT_DOUBLE_EQ(mat.trace(), 16.0);
    EXPECT_DOUBLE_EQ(mat.trace3(), 9.0);
}

TEST(Matrix, TestSetAndGetMatrix)
{
    Base::Matrix4D mat{1.0, 2.0, 3.0, 0.0,
                       2.0, 4.0, 6.0, 0.0,
                       3.0, 6.0, 9.0, 0.0,
                       0.0, 0.0, 0.0, 1.0};

    std::array<double, 16> values;
    mat.getMatrix(values.data());
    Base::Matrix4D inp;
    inp.setMatrix(values.data());

    EXPECT_EQ(mat, inp);
}

TEST(Matrix, TestSetAndGetGLMatrix)
{
    Base::Matrix4D mat{1.0, 2.0, 3.0, 0.0,
                       2.0, 4.0, 6.0, 0.0,
                       3.0, 6.0, 9.0, 0.0,
                       0.0, 0.0, 0.0, 1.0};

    std::array<double, 16> values;
    mat.getGLMatrix(values.data());
    Base::Matrix4D inp;
    inp.setGLMatrix(values.data());

    EXPECT_EQ(mat, inp);
}

TEST(Matrix, TestToAndFromString)
{
    Base::Matrix4D mat{1.0, 2.0, 3.0, 0.0,
                       2.0, 4.0, 6.0, 0.0,
                       3.0, 6.0, 9.0, 0.0,
                       0.0, 0.0, 0.0, 1.0};

    std::string str = mat.toString();
    Base::Matrix4D inp;
    inp.fromString(str);

    EXPECT_EQ(mat, inp);
}
// clang-format on
// NOLINTEND(cppcoreguidelines-*,readability-magic-numbers)
