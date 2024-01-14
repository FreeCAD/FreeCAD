// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"
#include "src/App/InitApplication.h"
#include <Mod/Part/App/TopoShape.h>

#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <TopoDS_Edge.hxx>
#include <BRep_Builder.hxx>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

class TopoShapeExpansionTest: public ::testing::Test
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
        _sids = &_sid;
        _hasher = Base::Reference<App::StringHasher>(new App::StringHasher);
        ASSERT_EQ(_hasher.getRefCount(), 1);
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

private:
    std::string _docName;
    Data::ElementIDRefs _sid;
    QVector<App::StringIDRef>* _sids = nullptr;
    App::StringHasherRef _hasher;
};

TEST_F(TopoShapeExpansionTest, makeElementCompoundOneShapeReturnsShape)
{
    // Arrange
    auto edge = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    Part::TopoShape topoShape {edge};
    std::vector<Part::TopoShape> shapes {topoShape};

    // Act
    topoShape.makeElementCompound(shapes, "C", false /*Don't force the creation*/);

    // Assert
    EXPECT_EQ(edge.ShapeType(), topoShape.getShape().ShapeType());  // NOT a Compound
}

TEST_F(TopoShapeExpansionTest, makeElementCompoundOneShapeForceReturnsCompound)
{
    // Arrange
    auto edge = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    Part::TopoShape topoShape {edge};
    std::vector<Part::TopoShape> shapes {topoShape};

    // Act
    topoShape.makeElementCompound(shapes, "C", true /*Force the creation*/);

    // Assert
    EXPECT_NE(edge.ShapeType(), topoShape.getShape().ShapeType());  // No longer the same thing
}

TEST_F(TopoShapeExpansionTest, makeElementCompoundTwoShapesReturnsCompound)
{
    // Arrange
    auto edge1 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    auto edge2 = BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(2.0, 0.0, 0.0)).Edge();
    Part::TopoShape topoShape {edge1};
    std::vector<Part::TopoShape> shapes {edge1, edge2};

    // Act
    topoShape.makeElementCompound(shapes);

    // Assert
    EXPECT_EQ(TopAbs_ShapeEnum::TopAbs_COMPOUND, topoShape.getShape().ShapeType());
}

TEST_F(TopoShapeExpansionTest, makeElementCompoundEmptyShapesReturnsEmptyCompound)
{
    // Arrange
    auto edge = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    Part::TopoShape topoShape {edge};
    std::vector<Part::TopoShape> shapes;

    // Act
    topoShape.makeElementCompound(shapes);

    // Assert
    EXPECT_EQ(TopAbs_ShapeEnum::TopAbs_COMPOUND, topoShape.getShape().ShapeType());
    EXPECT_TRUE(topoShape.getMappedChildElements().empty());
#if OCC_VERSION_HEX >= 0x070400
    EXPECT_EQ(0, topoShape.getShape().TShape()->NbChildren());
#endif
}

TEST_F(TopoShapeExpansionTest, makeElementCompoundTwoShapesGeneratesMap)
{
    // Arrange
    auto edge1 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    auto edge2 = BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(2.0, 0.0, 0.0)).Edge();
    Part::TopoShape topoShape {edge1};
    std::vector<Part::TopoShape> shapes {edge1, edge2};

    // Act
    topoShape.makeElementCompound(shapes);

    // Assert
    EXPECT_EQ(4, topoShape.getMappedChildElements().size());  // two vertices and two edges
}

namespace
{

std::pair<TopoDS_Shape, TopoDS_Shape> CreateTwoCubes()
{
    auto boxMaker1 = BRepPrimAPI_MakeBox(1.0, 1.0, 1.0);
    boxMaker1.Build();
    auto box1 = boxMaker1.Shape();

    auto boxMaker2 = BRepPrimAPI_MakeBox(1.0, 1.0, 1.0);
    boxMaker2.Build();
    auto box2 = boxMaker2.Shape();
    auto transform = gp_Trsf();
    transform.SetTranslation(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0));
    box2.Location(TopLoc_Location(transform));

    return {box1, box2};
}
}  // namespace

TEST_F(TopoShapeExpansionTest, makeElementCompoundTwoCubes)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    Part::TopoShape cube1TS {cube1};
    cube1TS.Tag = 1;
    Part::TopoShape cube2TS {cube2};
    cube2TS.Tag = 2;

    // Act
    Part::TopoShape topoShape;
    topoShape.makeElementCompound({cube1TS, cube2TS});

    // Assert
    auto elementMap = topoShape.getElementMap();
    EXPECT_EQ(52, elementMap.size());
    // Two cubes, each consisting of:
    // 8 Vertices
    // 12 Edges
    // 6 Faces
    // ----------
    // 26 subshapes each
}


TEST_F(TopoShapeExpansionTest, mapSubElement)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    Part::TopoShape cube1TS {cube1};
    cube1TS.Tag = 1;
    Part::TopoShape cube2TS {cube2};
    cube2TS.Tag = 2;
    auto [cube3, cube4] = CreateTwoCubes();
    Part::TopoShape cube3TS {cube3};
    cube3TS.Tag = 3;
    Part::TopoShape cube4TS {cube4};
    cube4TS.Tag = 4;
    std::vector<Part::TopoShape> subShapes = cube1TS.getSubTopoShapes(TopAbs_FACE);
    Part::TopoShape face1 = subShapes.front();
    face1.Tag = 3;
    Part::TopoShape topoShape, topoShape1;

    // Act
    int fs1 = topoShape.findShape(cube1TS.getShape());
    topoShape.setShape(cube1TS);
    // cube1TS.mapSubElement(face1);  // This throws an exception.  Is that right?
    // The cache ancestry only works on TopAbs_SHAPE so this is likely an invalid call,
    // but do we defend against it or expect the exception?

    topoShape.mapSubElement(
        cube1TS);  // Once we do this map, it's in the ancestry cache forevermore
    int fs2 = topoShape.findShape(cube1TS.getShape());
    int fs3 = topoShape.findShape(face1.getShape());
    int size1 = topoShape.getElementMap().size();
    int size0 = cube1TS.getElementMap().size();

    // Assert
    EXPECT_EQ(fs1, 0);
    EXPECT_EQ(fs2, 1);
    EXPECT_EQ(fs3, 1);
    EXPECT_EQ(0, size0);
    EXPECT_EQ(26, size1);

    // Act
    topoShape.setShape(TopoDS_Shape());
    int fs4 = topoShape.findShape(cube1TS.getShape());
    topoShape.setShape(cube1, true);
    // No mapSubElement required, it keeps finding it now
    int fs5 = topoShape.findShape(cube1TS.getShape());
    topoShape.setShape(cube1, false);
    int fs6 = topoShape.findShape(cube1TS.getShape());
    // Assert
    EXPECT_EQ(fs4, 0);
    EXPECT_EQ(fs5, 1);
    EXPECT_EQ(fs6, 1);

    // Act
    Part::TopoShape topoShape2, topoShape3;
    topoShape2.setShape(cube2TS);
    topoShape2.mapSubElement(cube2TS, nullptr, true);
    int fs7 = topoShape2.findShape(cube2TS.getShape());
    int fs8 = topoShape2.findShape(face1.getShape());

    topoShape3.setShape(cube3TS);
    topoShape3.mapSubElement(cube3TS, "Sample", true);
    int fs9 = topoShape3.findShape(cube3TS.getShape());
    int fs10 = topoShape3.findShape(face1.getShape());

    topoShape.makeElementCompound({cube1TS, cube2TS});
    int fs11 = topoShape.findShape(cube2TS.getShape());
    int size2 = topoShape.getElementMap().size();
    // Assert
    EXPECT_EQ(fs7, 1);
    EXPECT_EQ(fs8, 0);
    EXPECT_EQ(fs9, 1);
    EXPECT_EQ(fs10, 0);
    EXPECT_EQ(fs11, 2);
    EXPECT_EQ(52, size2);

    // Act
    topoShape2.makeElementCompound({cube3TS, cube4TS});
    topoShape2.mapSubElement(cube3TS, nullptr, true);
    topoShape3.makeElementCompound({topoShape, topoShape2});
    int fs12 = topoShape3.findShape(cube4TS.getShape());
    int size4 = topoShape3.getElementMap().size();
    // Assert
    EXPECT_EQ(104, size4);
    EXPECT_EQ(fs12, 4);
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
