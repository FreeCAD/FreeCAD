// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"
#include "src/App/InitApplication.h"
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "PartTestHelpers.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <GC_MakeCircle.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>

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

std::tuple<TopoDS_Face, TopoDS_Wire, TopoDS_Edge, TopoDS_Edge, TopoDS_Edge, TopoDS_Edge>
CreateRectFace(float len = 2.0, float wid = 3.0)
{
    auto edge1 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(len, 0.0, 0.0)).Edge();
    auto edge2 = BRepBuilderAPI_MakeEdge(gp_Pnt(len, 0.0, 0.0), gp_Pnt(len, wid, 0.0)).Edge();
    auto edge3 = BRepBuilderAPI_MakeEdge(gp_Pnt(len, wid, 0.0), gp_Pnt(0.0, wid, 0.0)).Edge();
    auto edge4 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, wid, 0.0), gp_Pnt(0.0, 0.0, 0.0)).Edge();
    auto wire1 = BRepBuilderAPI_MakeWire({edge1, edge2, edge3, edge4}).Wire();
    auto face1 = BRepBuilderAPI_MakeFace(wire1).Face();
    return {face1, wire1, edge1, edge2, edge3, edge4};
}

std::tuple<TopoDS_Face, TopoDS_Wire, TopoDS_Wire>
CreateFaceWithRoundHole(float len = 2.0, float wid = 3.0, float radius = 1.0)
{
    auto [face1, wire1, edge1, edge2, edge3, edge4] = CreateRectFace(len, wid);
    auto circ1 =
        GC_MakeCircle(gp_Pnt(len / 2.0, wid / 2.0, 0), gp_Dir(0.0, 0.0, 1.0), radius).Value();
    auto edge5 = BRepBuilderAPI_MakeEdge(circ1).Edge();
    auto wire2 = BRepBuilderAPI_MakeWire(edge5).Wire();
    auto face2 = BRepBuilderAPI_MakeFace(face1, wire2).Face();
    return {face2, wire1, wire2};
}

TEST_F(TopoShapeExpansionTest, makeElementFaceNull)
{
    // Arrange
    const double Len = 3, Wid = 2, Rad = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(Len, Wid, Rad);
    Part::TopoShape topoShape {face1};
    double area = PartTestHelpers::getArea(face1);
    double area1 = PartTestHelpers::getArea(topoShape.getShape());
    // Act
    Part::TopoShape newFace = topoShape.makeElementFace(nullptr);
    double area2 = PartTestHelpers::getArea(newFace.getShape());
    double area3 = PartTestHelpers::getArea(topoShape.getShape());
    // Assert
    EXPECT_FALSE(face1.IsEqual(newFace.getShape()));
    EXPECT_FLOAT_EQ(area, Len * Wid + M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area1, Len * Wid + M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area2, Len * Wid - M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area3, Len * Wid + M_PI * Rad * Rad);
    EXPECT_STREQ(newFace.shapeName().c_str(), "Face");
}

TEST_F(TopoShapeExpansionTest, makeElementFaceSimple)
{
    // Arrange
    const double Len = 3, Wid = 2, Rad = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(Len, Wid, Rad);
    Part::TopoShape topoShape {face1};
    double area = PartTestHelpers::getArea(face1);
    double area1 = PartTestHelpers::getArea(topoShape.getShape());
    // Act
    Part::TopoShape newFace = topoShape.makeElementFace(wire1);
    double area2 = PartTestHelpers::getArea(newFace.getShape());
    double area3 = PartTestHelpers::getArea(topoShape.getShape());
    // Assert
    EXPECT_TRUE(newFace.getShape().IsEqual(topoShape.getShape()));  // topoShape was altered
    EXPECT_FALSE(face1.IsEqual(newFace.getShape()));
    EXPECT_FLOAT_EQ(area, Len * Wid + M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area1, Len * Wid + M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area2, Len * Wid);
    EXPECT_FLOAT_EQ(area3, Len * Wid);
    EXPECT_STREQ(newFace.shapeName().c_str(), "Face");
}

TEST_F(TopoShapeExpansionTest, makeElementFaceParams)
{
    // Arrange
    const double Len = 3, Wid = 2, Rad = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(Len, Wid, Rad);
    Part::TopoShape topoShape {face1, 1L};
    double area = PartTestHelpers::getArea(face1);
    double area1 = PartTestHelpers::getArea(topoShape.getShape());
    // Act
    Part::TopoShape newFace =
        topoShape.makeElementFace(wire1, "Cut", "Part::FaceMakerBullseye", nullptr);
    double area2 = PartTestHelpers::getArea(newFace.getShape());
    double area3 = PartTestHelpers::getArea(topoShape.getShape());
    // Assert
    EXPECT_TRUE(newFace.getShape().IsEqual(topoShape.getShape()));  // topoShape was altered
    EXPECT_FALSE(face1.IsEqual(newFace.getShape()));
    EXPECT_FLOAT_EQ(area, Len * Wid + M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area1, Len * Wid + M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area2, Len * Wid);
    EXPECT_FLOAT_EQ(area3, Len * Wid);
    EXPECT_STREQ(newFace.shapeName().c_str(), "Face");
}

TEST_F(TopoShapeExpansionTest, makeElementFaceFromFace)
{
    // Arrange
    const double Len = 3, Wid = 2, Rad = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(Len, Wid, Rad);
    Part::TopoShape topoShape {face1, 1L};
    double area = PartTestHelpers::getArea(face1);
    double area1 = PartTestHelpers::getArea(topoShape.getShape());
    // Act
    Part::TopoShape newFace =
        topoShape.makeElementFace(face1, "Cut", "Part::FaceMakerBullseye", nullptr);
    double area2 = PartTestHelpers::getArea(newFace.getShape());
    double area3 = PartTestHelpers::getArea(topoShape.getShape());
    // Assert
    EXPECT_TRUE(newFace.getShape().IsEqual(topoShape.getShape()));  // topoShape was altered
    EXPECT_FALSE(face1.IsEqual(newFace.getShape()));
    EXPECT_FLOAT_EQ(area, Len * Wid + M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area1, Len * Wid + M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area2, Len * Wid - M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area3, Len * Wid - M_PI * Rad * Rad);
    EXPECT_STREQ(newFace.shapeName().c_str(), "Face");
}


TEST_F(TopoShapeExpansionTest, makeElementFaceOpenWire)
{
    // Arrange
    const double Len = 3, Wid = 2, Rad = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(Len, Wid, Rad);
    Part::TopoShape topoShape {wire1, 1L};
    double area = PartTestHelpers::getArea(face1);
    double area1 = PartTestHelpers::getArea(topoShape.getShape());
    // Act
    Part::TopoShape newFace = topoShape.makeElementFace(wire1, "Cut", nullptr, nullptr);
    double area2 = PartTestHelpers::getArea(newFace.getShape());
    double area3 = PartTestHelpers::getArea(topoShape.getShape());
    // Assert
    EXPECT_TRUE(newFace.getShape().IsEqual(topoShape.getShape()));  // topoShape was altered
    EXPECT_FALSE(face1.IsEqual(newFace.getShape()));
    EXPECT_FLOAT_EQ(area, Len * Wid + M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area1, 0);  // Len * Wid - M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area2, Len * Wid);
    EXPECT_FLOAT_EQ(area3, Len * Wid);
    EXPECT_STREQ(newFace.shapeName().c_str(), "Face");
}


TEST_F(TopoShapeExpansionTest, makeElementFaceClosedWire)
{
    // Arrange
    const double Len = 3, Wid = 2, Rad = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(Len, Wid, Rad);
    Part::TopoShape topoShape {wire2, 1L};
    double area = PartTestHelpers::getArea(face1);
    double area1 = PartTestHelpers::getArea(topoShape.getShape());
    // Act
    Part::TopoShape newFace =
        topoShape.makeElementFace(wire2, "Cut", "Part::FaceMakerBullseye", nullptr);
    double area2 = PartTestHelpers::getArea(newFace.getShape());
    double area3 = PartTestHelpers::getArea(topoShape.getShape());
    // Assert
    EXPECT_TRUE(newFace.getShape().IsEqual(topoShape.getShape()));  // topoShape was altered
    EXPECT_FALSE(face1.IsEqual(newFace.getShape()));
    EXPECT_FLOAT_EQ(area, Len * Wid + M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area1, 0);  // Len * Wid - M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area2, M_PI * Rad * Rad);
    EXPECT_FLOAT_EQ(area3, M_PI * Rad * Rad);
    EXPECT_STREQ(newFace.shapeName().c_str(), "Face");
}

// Possible future makeElementFace tests:
// Overlapping wire
// Compound of wires
// Compound of faces
// Compound of other shape types


TEST_F(TopoShapeExpansionTest, setElementComboNameNothing)
{
    // Arrange
    Part::TopoShape topoShape(1L);
    // Act
    Data::MappedName result = topoShape.setElementComboName(Data::IndexedName(), {});
    // ASSERT
    EXPECT_STREQ(result.toString().c_str(), "");
}


TEST_F(TopoShapeExpansionTest, setElementComboNameSimple)
{
    // Arrange
    auto edge1 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    Part::TopoShape topoShape(edge1, 1L);
    topoShape.setElementMap({});  // Initialize the map to avoid a segfault.
    // Also, maybe the end of TopoShape::mapSubElementTypeForShape should enforce that elementMap()
    // isn't nullptr to eliminate the segfault.
    Data::MappedName edgeName("testname");
    // Act
    Data::MappedName result =
        topoShape.setElementComboName(Data::IndexedName::fromConst("Edge", 1), {edgeName});
    // Assert
    EXPECT_STREQ(result.toString().c_str(), "testname;");
}


TEST_F(TopoShapeExpansionTest, setElementComboName)
{
    // Arrange
    Part::TopoShape topoShape(2L);
    topoShape.setElementMap({});
    Data::MappedName edgeName =
        topoShape.getMappedName(Data::IndexedName::fromConst("Edge", 1), true);
    Data::MappedName faceName =
        topoShape.getMappedName(Data::IndexedName::fromConst("Face", 7), true);
    Data::MappedName faceName2 =
        topoShape.getMappedName(Data::IndexedName::fromConst("Face", 8), true);
    char* op = "Copy";
    // Act
    Data::MappedName result = topoShape.setElementComboName(Data::IndexedName::fromConst("Edge", 1),
                                                            {edgeName, faceName, faceName2},
                                                            Part::OpCodes::Common,
                                                            op);
    // Assert
    EXPECT_STREQ(result.toString().c_str(), "Edge1;CMN(Face7|Face8);Copy");
    // The detailed forms of names are covered in encodeElementName tests
}

TEST_F(TopoShapeExpansionTest, setElementComboNameCompound)
{
    // Arrange
    auto edge1 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    auto wire1 = BRepBuilderAPI_MakeWire({edge1}).Wire();
    auto wire2 = BRepBuilderAPI_MakeWire({edge1}).Wire();
    Part::TopoShape topoShape(2L);
    topoShape.makeElementCompound({wire1, wire2});  // Quality of shape doesn't matter
    Data::MappedName edgeName =
        topoShape.getMappedName(Data::IndexedName::fromConst("Edge", 1), true);
    Data::MappedName faceName =
        topoShape.getMappedName(Data::IndexedName::fromConst("Face", 7), true);
    Data::MappedName faceName2 =
        topoShape.getMappedName(Data::IndexedName::fromConst("Face", 8), true);
    char* op = "Copy";
    // Act
    Data::MappedName result = topoShape.setElementComboName(Data::IndexedName::fromConst("Edge", 1),
                                                            {edgeName, faceName, faceName2},
                                                            Part::OpCodes::Common,
                                                            op);
    // ASSERT
    EXPECT_STREQ(result.toString().c_str(), "Edge1;:H,E;CMN(Face7|Face8);Copy");
    // The detailed forms of names are covered in encodeElementName tests
}

TEST_F(TopoShapeExpansionTest, splitWires)
{
    // Arrange
    const double Len = 3, Wid = 2, Rad = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(Len, Wid, Rad);
    Part::TopoShape topoShape {face1, 1L};
    std::vector<Part::TopoShape> inner;
    // Act
    EXPECT_EQ(topoShape.getShape().Orientation(), TopAbs_FORWARD);
    Part::TopoShape wire =
        topoShape.splitWires(&inner, Part::TopoShape::SplitWireReorient::ReorientReversed);
    // Assert
    EXPECT_EQ(inner.size(), 1);
    EXPECT_FLOAT_EQ(PartTestHelpers::getLength(wire.getShape()), 2 + 2 + 3 + 3);
    EXPECT_FLOAT_EQ(PartTestHelpers::getLength(inner.front().getShape()), M_PI * Rad * 2);
    EXPECT_EQ(wire.getShape().Orientation(), TopAbs_REVERSED);
    for (Part::TopoShape& shape : inner) {
        EXPECT_EQ(shape.getShape().Orientation(), TopAbs_FORWARD);
    }
}

// Possible future tests:
// splitWires without inner Wires
// splitWires with allfour reorientation values NoReorient, ReOrient, ReorientForward,
// ReorientRevesed

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
