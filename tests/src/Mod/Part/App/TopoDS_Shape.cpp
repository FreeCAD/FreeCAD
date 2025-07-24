// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Standard_TypeMismatch.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>

#include <src/App/InitApplication.h>

class TestTopoDS_Shape: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};

// NOLINTBEGIN
// clang-format off
TEST_F(TestTopoDS_Shape, TestCastEdgeToVertex)
{
    BRepBuilderAPI_MakeEdge mkEdge(gp_Pnt(0, 0, 0), gp_Pnt(10, 0, 0));
    TopoDS_Edge edge = mkEdge.Edge();
    TopoDS_Vertex vertex;
    EXPECT_THROW(vertex = TopoDS::Vertex(edge), Standard_TypeMismatch);
    EXPECT_TRUE(vertex.IsNull());
}

TEST_F(TestTopoDS_Shape, TestCastNullVertex)
{
    TopoDS_Vertex vertex1;
    TopoDS_Vertex vertex2;
    EXPECT_NO_THROW(vertex2 = TopoDS::Vertex(vertex1));
    EXPECT_TRUE(vertex2.IsNull());
}

TEST_F(TestTopoDS_Shape, TestCastNullEdge)
{
    TopoDS_Edge edge;
    TopoDS_Vertex vertex;
    EXPECT_NO_THROW(vertex = TopoDS::Vertex(edge));
    EXPECT_TRUE(vertex.IsNull());
}

TEST_F(TestTopoDS_Shape, TestExploreNullShape)
{
    TopoDS_Face face;
    TopExp_Explorer xp(face, TopAbs_FACE);
    EXPECT_FALSE(xp.More());
}

// clang-format on
// NOLINTEND
