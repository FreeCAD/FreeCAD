#include "gtest/gtest.h"
#include <Base/Placement.h>
#include <Base/ViewProj.h>

TEST(ViewProj, TestViewProjMatrix)
{
    Base::Matrix4D mat;
    Base::ViewProjMatrix proj(mat);

    Base::Vector3d vec(1, 2, 3);
    EXPECT_EQ(proj(vec), Base::Vector3d(1, 1.5, 2));
    EXPECT_EQ(proj.inverse(Base::Vector3d(1, 1.5, 2)), vec);
}

TEST(ViewProj, TestViewOrthoProjMatrix)
{
    Base::Matrix4D mat;
    Base::ViewOrthoProjMatrix proj(mat);

    Base::Vector3d vec(1, 2, 3);
    EXPECT_EQ(proj(vec), vec);
    EXPECT_EQ(proj.inverse(vec), vec);
}
