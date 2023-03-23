#include "gtest/gtest.h"
#include <Base/Exception.h>
#include <Base/Matrix.h>
#include <Base/Rotation.h>


// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
TEST(Rotation, TestNonUniformScaleLeft)
{
    Base::Rotation rot;
    rot.setYawPitchRoll(45.0, 0.0, 0.0);

    Base::Matrix4D mat;
    rot.getValue(mat);

    Base::Matrix4D scale;
    scale.scale(1.0, std::sqrt(3.0), 1.0);

    Base::Rotation scaled_rot(scale * mat);

    rot.setYawPitchRoll(60.0, 0.0, 0.0);
    EXPECT_EQ(scaled_rot.isSame(rot, 1.0e-7), true);
    EXPECT_EQ(rot.isSame(scaled_rot, 1.0e-7), true);
}

TEST(Rotation, TestNonUniformScaleRight)
{
    Base::Rotation rot;
    rot.setYawPitchRoll(20.0, 0.0, 0.0);

    Base::Matrix4D mat;
    rot.getValue(mat);

    Base::Matrix4D scale;
    scale.scale(2.0, 3.0, 4.0);

    Base::Rotation scaled_rot(mat * scale);

    EXPECT_EQ(scaled_rot.isSame(rot, 1.0e-7), true);
    EXPECT_EQ(rot.isSame(scaled_rot, 1.0e-7), true);
}

TEST(Rotation, TestUniformScaleGT1)
{
    Base::Rotation rot;
    rot.setYawPitchRoll(20.0, 0.0, 0.0);

    Base::Matrix4D mat;
    rot.getValue(mat);

    Base::Matrix4D scale;
    scale.scale(3.0, 3.0, 3.0);

    Base::Rotation scaled_rot(mat * scale);

    EXPECT_EQ(scaled_rot.isSame(rot, 1.0e-7), true);
    EXPECT_EQ(rot.isSame(scaled_rot, 1.0e-7), true);
}

TEST(Rotation, TestUniformScaleLT1)
{
    Base::Matrix4D mat;
    mat.scale(0.5);

    Base::Rotation scaled_rot(mat);

    EXPECT_EQ(scaled_rot.isSame(scaled_rot, 1.0e-7), true);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
