// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"
#include <Mod/Part/App/TopoShape.h>
#include "Mod/Part/App/TopoShapeMapper.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeShape.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools_History.hxx>
#include <BRepTools_ReShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS_Edge.hxx>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

class TopoShapeMapperTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        App::GetApplication().newDocument(_docName.c_str(), "testUser");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

private:
    std::string _docName;
};

TEST_F(TopoShapeMapperTest, shapeHasherSingle)
{
    // Arrange
    auto edge = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    Part::TopoShape topoShape {edge};
    auto edge2 = BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(1.0, 1.0, 0.0)).Edge();
    Part::TopoShape topoShape2 {edge2};
    struct Part::ShapeHasher hasher;

    // Act
    size_t hash1 = hasher(topoShape);
    size_t hash2 = hasher(topoShape2);
    size_t hash3 = hasher(edge);
    size_t hash4 = hasher(edge2);

    // Assert
    EXPECT_EQ(hash1, hash3);
    EXPECT_EQ(hash2, hash4);
    EXPECT_NE(hash1, hash2);
}

TEST_F(TopoShapeMapperTest, shapeHasherDual)
{
    // Arrange
    auto edge = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    Part::TopoShape topoShape {edge};
    auto edge2 = BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(1.0, 1.0, 0.0)).Edge();
    Part::TopoShape topoShape2 {edge2};
    struct Part::ShapeHasher hasher;

    // Act
    size_t hash5 = hasher(topoShape, topoShape);
    size_t hash6 = hasher(topoShape, topoShape2);
    size_t hash7 = hasher(edge, edge);
    size_t hash8 = hasher(edge, edge2);

    // Assert
    EXPECT_TRUE(hash5);
    EXPECT_FALSE(hash6);
    EXPECT_TRUE(hash7);
    EXPECT_FALSE(hash8);
}

TEST_F(TopoShapeMapperTest, shapeHasherPair)
{
    // Arrange
    auto edge = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    Part::TopoShape topoShape {edge};
    auto edge2 = BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(1.0, 1.0, 0.0)).Edge();
    Part::TopoShape topoShape2 {edge2};
    std::pair<Part::TopoShape, Part::TopoShape> pair1(topoShape, topoShape2);
    std::pair<Part::TopoShape, Part::TopoShape> pair2(topoShape, topoShape);
    std::pair<TopoDS_Shape, TopoDS_Shape> pair3(edge, edge2);
    std::pair<TopoDS_Shape, TopoDS_Shape> pair4(edge, edge);
    struct Part::ShapeHasher hasher;

    // Act
    size_t hash9 = hasher(pair1);
    size_t hash10 = hasher(pair2);
    size_t hash11 = hasher(pair3);
    size_t hash12 = hasher(pair4);

    // Assert
    EXPECT_EQ(hash9, hash11);
    EXPECT_EQ(hash10, hash12);
    EXPECT_NE(hash9, hash10);
}

TEST_F(TopoShapeMapperTest, shapeHasherPairs)
{
    // Arrange
    auto edge = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    Part::TopoShape topoShape {edge};
    auto edge2 = BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(1.0, 1.0, 0.0)).Edge();
    Part::TopoShape topoShape2 {edge2};
    std::pair<Part::TopoShape, Part::TopoShape> pair1(topoShape, topoShape2);
    std::pair<Part::TopoShape, Part::TopoShape> pair2(topoShape, topoShape);
    std::pair<TopoDS_Shape, TopoDS_Shape> pair3(edge, edge2);
    std::pair<TopoDS_Shape, TopoDS_Shape> pair4(edge, edge);
    struct Part::ShapeHasher hasher;

    // Act
    size_t hash13 = hasher(pair1, pair1);
    size_t hash14 = hasher(pair1, pair2);
    size_t hash15 = hasher(pair3, pair3);
    size_t hash16 = hasher(pair3, pair4);

    // Assert
    EXPECT_TRUE(hash13);
    EXPECT_FALSE(hash14);
    EXPECT_TRUE(hash15);
    EXPECT_FALSE(hash16);
}


TEST_F(TopoShapeMapperTest, shapeMapperTests)
{
    // Arrange
    auto mapper = Part::ShapeMapper();
    auto boxMaker1 = BRepPrimAPI_MakeBox(1.0, 1.0, 1.0);
    boxMaker1.Build();
    auto box1 = boxMaker1.Shape();
    Part::TopoShape topoShape1 {box1};

    // Act
    auto e = topoShape1.getSubTopoShapes(TopAbs_EDGE);
    mapper.populate(Part::MappingStatus::Modified, box1, {e[0], e[1], e[2], e[3]});
    mapper.populate(Part::MappingStatus::Generated, box1, {e[4], e[5], e[6]});
    std::vector<TopoDS_Shape> vec1 = mapper.modified(box1);
    std::vector<TopoDS_Shape> vec2 = mapper.generated(box1);

    // Assert
    EXPECT_EQ(vec1.size(), 4);
    EXPECT_EQ(vec2.size(), 3);
}


// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
