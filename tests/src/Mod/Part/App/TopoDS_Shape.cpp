// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Standard_TypeMismatch.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

// NOLINTBEGIN
// clang-format off
TEST(TopoDS_Shape, TestCastEdgeToVertex)
{
    BRepBuilderAPI_MakeEdge mkEdge(gp_Pnt(0, 0, 0), gp_Pnt(10, 0, 0));
    TopoDS_Edge edge = mkEdge.Edge();
    TopoDS_Vertex vertex;
    EXPECT_THROW(vertex = TopoDS::Vertex(edge), Standard_TypeMismatch);
    EXPECT_TRUE(vertex.IsNull());
}

TEST(TopoDS_Shape, TestCastNullVertex)
{
    TopoDS_Vertex vertex1;
    TopoDS_Vertex vertex2;
    EXPECT_NO_THROW(vertex2 = TopoDS::Vertex(vertex1));
    EXPECT_TRUE(vertex2.IsNull());
}

TEST(TopoDS_Shape, TestCastNullEdge)
{
    TopoDS_Edge edge;
    TopoDS_Vertex vertex;
    EXPECT_NO_THROW(vertex = TopoDS::Vertex(edge));
    EXPECT_TRUE(vertex.IsNull());
}

// clang-format on
// NOLINTEND
