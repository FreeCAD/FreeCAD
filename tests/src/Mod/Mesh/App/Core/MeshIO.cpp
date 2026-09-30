// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <sstream>
#include <Mod/Mesh/App/Core/MeshIO.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>

TEST(MeshIO, LoadAsciiSTLWithoutTrailingNewline)
{
    // Regression test for Issue #32928 -- the solid's name has to be at least 11 chars to trigger
    std::istringstream input(
        "solid ASCII_STL_WITH_A_LONG_NAME\n"
        "facet normal 0 0 1\n"
        "outer loop\n"
        "vertex 0 0 0\n"
        "vertex 1 0 0\n"
        "vertex 0 1 0\n"
        "endloop\n"
        "endfacet\n"
        "endsolid ASCII_STL_WIth_A_LONG_NAME"
    );
    MeshCore::MeshKernel kernel;
    MeshCore::MeshInput reader(kernel);

    EXPECT_TRUE(reader.LoadAsciiSTL(input));
    EXPECT_EQ(kernel.CountFacets(), 1);
}
