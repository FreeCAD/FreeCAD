#include "gtest/gtest.h"
#include <Mod/Mesh/App/Mesh.h>

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)
TEST(MeshTest, TestDefault)
{
    MeshCore::MeshKernel kernel;
    Base::Vector3f p1 {
        0,
        0,
        0,
    };
    Base::Vector3f p2 {0, 0, 1};
    Base::Vector3f p3 {0, 1, 0};
    kernel.AddFacet(MeshCore::MeshGeomFacet(p1, p2, p3));

    EXPECT_EQ(kernel.CountPoints(), 3);
    EXPECT_EQ(kernel.CountEdges(), 3);
    EXPECT_EQ(kernel.CountFacets(), 1);
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
