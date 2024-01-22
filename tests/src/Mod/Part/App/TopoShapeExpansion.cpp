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
    const double L = 3, W = 2, R = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(L, W, R);
    Part::TopoShape topoShape {face1};
    double area = PartTestHelpers::getArea(face1);
    double area1 = PartTestHelpers::getArea(topoShape.getShape());
    // Act
    Part::TopoShape newFace = topoShape.makeElementFace(nullptr);
    double area2 = PartTestHelpers::getArea(newFace.getShape());
    double area3 = PartTestHelpers::getArea(topoShape.getShape());
    // Assert
    EXPECT_FALSE(face1.IsEqual(newFace.getShape()));
    EXPECT_FLOAT_EQ(area, L * W + M_PI * R * R);
    EXPECT_FLOAT_EQ(area1, L * W + M_PI * R * R);
    EXPECT_FLOAT_EQ(area2, L * W - M_PI * R * R);
    EXPECT_FLOAT_EQ(area3, L * W + M_PI * R * R);
    EXPECT_STREQ(newFace.shapeName().c_str(), "Face");
}

TEST_F(TopoShapeExpansionTest, makeElementFaceSimple)
{
    // Arrange
    const double L = 3, W = 2, R = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(L, W, R);
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
    EXPECT_FLOAT_EQ(area, L * W + M_PI * R * R);
    EXPECT_FLOAT_EQ(area1, L * W + M_PI * R * R);
    EXPECT_FLOAT_EQ(area2, L * W);
    EXPECT_FLOAT_EQ(area3, L * W);
    EXPECT_STREQ(newFace.shapeName().c_str(), "Face");
}

TEST_F(TopoShapeExpansionTest, makeElementFaceParams)
{
    // Arrange
    const double L = 3, W = 2, R = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(L, W, R);
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
    EXPECT_FLOAT_EQ(area, L * W + M_PI * R * R);
    EXPECT_FLOAT_EQ(area1, L * W + M_PI * R * R);
    EXPECT_FLOAT_EQ(area2, L * W);
    EXPECT_FLOAT_EQ(area3, L * W);
    EXPECT_STREQ(newFace.shapeName().c_str(), "Face");
}

TEST_F(TopoShapeExpansionTest, makeElementFaceFromFace)
{
    // Arrange
    const double L = 3, W = 2, R = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(L, W, R);
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
    EXPECT_FLOAT_EQ(area, L * W + M_PI * R * R);
    EXPECT_FLOAT_EQ(area1, L * W + M_PI * R * R);
    EXPECT_FLOAT_EQ(area2, L * W - M_PI * R * R);
    EXPECT_FLOAT_EQ(area3, L * W - M_PI * R * R);
    EXPECT_STREQ(newFace.shapeName().c_str(), "Face");
}


TEST_F(TopoShapeExpansionTest, makeElementFaceOpenWire)
{
    // Arrange
    const double L = 3, W = 2, R = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(L, W, R);
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
    EXPECT_FLOAT_EQ(area, L * W + M_PI * R * R);
    EXPECT_FLOAT_EQ(area1, 0);  // L * W - M_PI * R * R);
    EXPECT_FLOAT_EQ(area2, L * W);
    EXPECT_FLOAT_EQ(area3, L * W);
    EXPECT_STREQ(newFace.shapeName().c_str(), "Face");
}


TEST_F(TopoShapeExpansionTest, makeElementFaceClosedWire)
{
    // Arrange
    const double L = 3, W = 2, R = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(L, W, R);
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
    EXPECT_FLOAT_EQ(area, L * W + M_PI * R * R);
    EXPECT_FLOAT_EQ(area1, 0);  // L * W - M_PI * R * R);
    EXPECT_FLOAT_EQ(area2, M_PI * R * R);
    EXPECT_FLOAT_EQ(area3, M_PI * R * R);
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
    const double L = 3, W = 2, R = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(L, W, R);
    Part::TopoShape topoShape {face1, 1L};
    std::vector<Part::TopoShape> inner;
    // Act
    EXPECT_EQ(topoShape.getShape().Orientation(), TopAbs_FORWARD);
    Part::TopoShape wire =
        topoShape.splitWires(&inner, Part::TopoShape::SplitWireReorient::ReorientReversed);
    // Assert
    EXPECT_EQ(inner.size(), 1);
    EXPECT_FLOAT_EQ(PartTestHelpers::getLength(wire.getShape()), 2 + 2 + 3 + 3);
    EXPECT_FLOAT_EQ(PartTestHelpers::getLength(inner.front().getShape()), M_PI * R * 2);
    EXPECT_EQ(wire.getShape().Orientation(), TopAbs_REVERSED);
    for (Part::TopoShape& shape : inner) {
        EXPECT_EQ(shape.getShape().Orientation(), TopAbs_FORWARD);
    }
}

// Possible future tests:
// splitWires without inner Wires
// splitWires with allfour reorientation values NoReorient, ReOrient, ReorientForward,
// ReorientRevesed

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
