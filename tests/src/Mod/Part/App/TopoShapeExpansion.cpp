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


TEST_F(TopoShapeExpansionTest, mapSubElementInvalidParm)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    Part::TopoShape cube1TS {cube1};
    cube1TS.Tag = 1;

    // Act
    std::vector<Part::TopoShape> subShapes = cube1TS.getSubTopoShapes(TopAbs_FACE);
    Part::TopoShape face1 = subShapes.front();
    face1.Tag = 2;

    // Assert
    EXPECT_THROW(cube1TS.mapSubElement(face1), Part::NullShapeException);  // No subshapes
}

TEST_F(TopoShapeExpansionTest, mapSubElementFindShapeByNames)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    Part::TopoShape cube1TS {cube1};
    Part::TopoShape cube2TS {cube2};
    cube1TS.Tag = 1;
    cube2TS.Tag = 2;
    Part::TopoShape topoShape, topoShape1;

    // Act
    int fs1 = topoShape1.findShape(cube1);
    topoShape.setShape(cube2TS);
    topoShape1.makeElementCompound({cube1TS, cube2TS});
    int fs2 = topoShape1.findShape(cube1);

    TopoDS_Shape tds1 = topoShape.findShape("SubShape1");
    TopoDS_Shape tds2 = topoShape.findShape("SubShape2");  // Nonexistent
    TopoDS_Shape tds3 = topoShape1.findShape("SubShape1");
    TopoDS_Shape tds4 = topoShape1.findShape("SubShape2");
    TopoDS_Shape tds5 = topoShape1.findShape("NonExistentName");  // Invalid Name

    // Assert
    EXPECT_EQ(fs1, 0);
    EXPECT_EQ(fs2, 1);
    EXPECT_FALSE(tds1.IsNull());
    EXPECT_TRUE(tds2.IsNull());
    EXPECT_FALSE(tds3.IsNull());
    EXPECT_FALSE(tds4.IsNull());
    EXPECT_TRUE(tds5.IsNull());
}

TEST_F(TopoShapeExpansionTest, mapSubElementFindShapeByType)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    Part::TopoShape cube1TS {cube1};
    Part::TopoShape cube2TS {cube2};
    cube1TS.Tag = 1;
    cube2TS.Tag = 2;
    Part::TopoShape topoShape;
    topoShape.makeElementCompound({cube1TS, cube2TS});
    topoShape.mapSubElement(cube2TS, "Name", false);

    // Act, Assert
    for (int i = 1; i <= 12; i++) {
        TopoDS_Shape dshape1 = topoShape.findShape(TopAbs_FACE, i);
        EXPECT_FALSE(dshape1.IsNull()) << "Face num " << i;
    }
    TopoDS_Shape dshape1 = topoShape.findShape(TopAbs_FACE, 13);
    EXPECT_TRUE(dshape1.IsNull());
}


TEST_F(TopoShapeExpansionTest, mapSubElementFindAncestor)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    Part::TopoShape cube1TS {cube1};
    Part::TopoShape cube2TS {cube2};
    cube1TS.Tag = 1;
    cube2TS.Tag = 2;
    Part::TopoShape topoShape;
    topoShape.makeElementCompound({cube1TS, cube2TS});
    topoShape.mapSubElement(cube2TS, "Name", false);

    // Act
    int fa1 = topoShape.findAncestor(cube2, TopAbs_COMPOUND);
    TopoDS_Shape tds1 = topoShape.findAncestorShape(cube1, TopAbs_COMPOUND);

    // Assert
    EXPECT_EQ(fa1, 1);
    EXPECT_TRUE(tds1.IsEqual(topoShape.getShape()));
}


TEST_F(TopoShapeExpansionTest, mapSubElementFindAncestors)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    auto [cube3, cube4] = CreateTwoCubes();
    auto tr {gp_Trsf()};
    tr.SetTranslation(gp_Vec(gp_XYZ(0, 1, 0)));
    cube3.Move(TopLoc_Location(tr));
    cube4.Move(TopLoc_Location(tr));
    Part::TopoShape cube1TS {cube1};
    Part::TopoShape cube2TS {cube2};
    Part::TopoShape cube3TS {cube3};
    Part::TopoShape cube4TS {cube4};
    cube1TS.Tag = 1;
    cube2TS.Tag = 2;
    cube3TS.Tag = 3;
    cube4TS.Tag = 4;
    Part::TopoShape topoShape, topoShape1, topoShape2;
    Part::TopoShape topoShape3, topoShape4, topoShape5, topoShape6;
    topoShape.makeElementCompound({cube1TS, cube2TS});
    topoShape1.makeElementCompound({cube3TS, cube4TS});
    topoShape2.makeElementCompound({cube1TS, cube3TS});
    topoShape3.makeElementCompound({cube2TS, cube4TS});
    topoShape4.makeElementCompound({topoShape, topoShape1});
    topoShape5.makeElementCompound({topoShape2, topoShape3});
    topoShape6.makeElementCompound({topoShape4, topoShape5});
    topoShape6.mapSubElement(cube2TS, nullptr, false);

    // Act
    auto ancestorList = topoShape6.findAncestors(cube3, TopAbs_COMPOUND);
    auto ancestorShapeList = topoShape6.findAncestorsShapes(cube3, TopAbs_COMPOUND);

    // FIXME:  It seems very strange that both of these ancestors calls return lists of two items
    // that contain the same thing twice.  What I expect is that the ancestors of cube3 would be
    // topoShape6 topoShape5, topoShape3, topoShape2, and topoShape1.
    //
    // This is a very convoluted hierarchy, and the only way I could get more than one result from
    // findAncestors.  I guess it's possible that it's only intended to return a single result in
    // almost all cases; that would mean that what it returns is the shape at the top of the tree.
    // But that's exactly the shape we use to call it in the first place, so we already have it.
    //
    // Note that in the RT branch, findAncestorsShapes is called by GenericShapeMapper::init,
    // TopoShape::makEChamfer and MapperPrism
    //  findAncestors is used in a dozen places.
    //

    // Assert
    EXPECT_EQ(ancestorList.size(), 2);
    EXPECT_EQ(ancestorList.front(), 1);
    EXPECT_EQ(ancestorList.back(), 1);
    EXPECT_EQ(ancestorShapeList.size(), 2);
    EXPECT_TRUE(ancestorShapeList.front().IsEqual(topoShape6.getShape()));
    EXPECT_TRUE(ancestorShapeList.back().IsEqual(topoShape6.getShape()));
}


// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
