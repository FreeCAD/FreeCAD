// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"
#include "src/App/InitApplication.h"
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "PartTestHelpers.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepFeat_SplitShape.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <gp_Pln.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Edge.hxx>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

using namespace Part;
using namespace PartTestHelpers;

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
    App::StringHasherRef _hasher;
};

TEST_F(TopoShapeExpansionTest, makeElementCompoundOneShapeReturnsShape)
{
    // Arrange
    auto edge = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    TopoShape topoShape {edge};
    std::vector<TopoShape> shapes {topoShape};

    // Act
    topoShape.makeElementCompound(shapes,
                                  "C",
                                  TopoShape::SingleShapeCompoundCreationPolicy::returnShape);

    // Assert
    EXPECT_EQ(edge.ShapeType(), topoShape.getShape().ShapeType());  // NOT a Compound
}

TEST_F(TopoShapeExpansionTest, makeElementCompoundOneShapeForceReturnsCompound)
{
    // Arrange
    auto edge = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    TopoShape topoShape {edge};
    std::vector<TopoShape> shapes {topoShape};

    // Act
    topoShape.makeElementCompound(shapes,
                                  "C",
                                  TopoShape::SingleShapeCompoundCreationPolicy::forceCompound);

    // Assert
    EXPECT_NE(edge.ShapeType(), topoShape.getShape().ShapeType());  // No longer the same thing
}

TEST_F(TopoShapeExpansionTest, makeElementCompoundTwoShapesReturnsCompound)
{
    // Arrange
    auto edge1 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    auto edge2 = BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(2.0, 0.0, 0.0)).Edge();
    TopoShape topoShape {edge1};
    std::vector<TopoShape> shapes {edge1, edge2};

    // Act
    topoShape.makeElementCompound(shapes);

    // Assert
    EXPECT_EQ(TopAbs_ShapeEnum::TopAbs_COMPOUND, topoShape.getShape().ShapeType());
}

TEST_F(TopoShapeExpansionTest, makeElementCompoundEmptyShapesReturnsEmptyCompound)
{
    // Arrange
    auto edge = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    TopoShape topoShape {edge};
    std::vector<TopoShape> shapes;

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
    TopoShape topoShape {edge1};
    std::vector<TopoShape> shapes {edge1, edge2};

    // Act
    topoShape.makeElementCompound(shapes);

    // Assert
    EXPECT_EQ(4, topoShape.getMappedChildElements().size());  // two vertices and two edges
}

TEST_F(TopoShapeExpansionTest, makeElementCompoundTwoCubes)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape cube1TS {cube1};
    cube1TS.Tag = 1;
    TopoShape cube2TS {cube2};
    cube2TS.Tag = 2;

    // Act
    TopoShape topoShape;
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

TEST_F(TopoShapeExpansionTest, MapperMakerModified)
{
    // Arrange
    // Definition of all the objects needed for a Transformation
    // (https://dev.opencascade.org/doc/refman/html/class_b_rep_builder_a_p_i___transform.html)
    auto translation {gp_Trsf()};
    auto transform {BRepBuilderAPI_Transform(translation)};
    auto transformMprMkr {MapperMaker(transform)};

    // Definition of all the objects needed for a Shape Splitting
    // (https://dev.opencascade.org/doc/refman/html/class_b_rep_feat___split_shape.html)
    auto splitMkr {BRepFeat_SplitShape()};
    auto splitMprMkr {MapperMaker(splitMkr)};

    // Creating a Wire, used later to create a Face
    auto wireMkr {BRepBuilderAPI_MakeWire(
        BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)),
        BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(1.0, 1.0, 0.0)),
        BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 1.0, 0.0), gp_Pnt(0.0, 0.0, 0.0)))};
    auto wire = wireMkr.Wire();

    // Creating a Face using the Wire created before
    auto faceMkr {BRepBuilderAPI_MakeFace(wire)};
    auto face = faceMkr.Face();

    // Creating an Edge to split the Face and the Wire
    auto edgeMkr {BRepBuilderAPI_MakeEdge(gp_Pnt(0.5, 1.0, 0.0), gp_Pnt(0.5, -1.0, 0.0))};
    auto edge = edgeMkr.Edge();

    // Act
    // Performing the Transformation
    translation.SetTranslation(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0));
    transform.Perform(wire);

    // Initializing and performing the Split
    splitMkr.Init(face);
    splitMkr.Add(edge, face);
    splitMkr.Build();

    // Assert
    // Check that all the shapes and operations have been performed
    EXPECT_TRUE(wireMkr.IsDone());
    EXPECT_TRUE(faceMkr.IsDone());
    EXPECT_TRUE(edgeMkr.IsDone());
    EXPECT_TRUE(splitMkr.IsDone());
    EXPECT_TRUE(transform.IsDone());

    // Check the result of the operations
    EXPECT_EQ(transformMprMkr.modified(wire).size(), 1);  // The Transformation acts on the Wire...
    EXPECT_EQ(transformMprMkr.modified(face).size(), 1);  // ... and therefor on the created Face...
    EXPECT_EQ(transformMprMkr.modified(edge).size(), 1);  // ... and on the Edge added to the Face

    EXPECT_EQ(splitMprMkr.modified(edge).size(), 0);  // The Split doesn't modify the Edge
    EXPECT_EQ(splitMprMkr.modified(wire).size(), 2);  // The Split modifies the Wire into 2 Wires
    EXPECT_EQ(splitMprMkr.modified(face).size(), 2);  // The Split modifies the Face into 2 Faces
}

TEST_F(TopoShapeExpansionTest, MapperMakerGenerated)
{
    // Arrange
    // Creating tree Edges, used later in the Fuse operations
    auto edge1 {BRepBuilderAPI_MakeEdge(gp_Pnt(-1.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, -1.0, 0.0), gp_Pnt(0.0, 1.0, 0.0)).Edge()};
    auto edge3 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, -1.0), gp_Pnt(0.0, 0.0, 1.0)).Edge()};

    // Definition of all the objects needed for the Fuses
    // (https://dev.opencascade.org/doc/refman/html/class_b_rep_algo_a_p_i___fuse.html)
    // The Fuse operation, like other Boolean operations derived from BRepAlgoAPI_BuilderAlgo,
    // supports the generation history
    // (https://dev.opencascade.org/doc/refman/html/class_b_rep_algo_a_p_i___builder_algo.html)
    auto fuse1Mkr {BRepAlgoAPI_Fuse(edge1, edge2)};
    auto fuse1MprMkr {MapperMaker(fuse1Mkr)};
    auto fuse2Mkr {BRepAlgoAPI_Fuse(edge1, edge3)};
    auto fuse2MprMkr {MapperMaker(fuse2Mkr)};

    // Act
    fuse1Mkr.Build();
    fuse2Mkr.Build();

    // Assert
    // Check that all the shapes and operations have been performed
    EXPECT_TRUE(fuse1Mkr.IsDone());
    EXPECT_TRUE(fuse2Mkr.IsDone());

    // Check the result of the operations
    EXPECT_EQ(fuse1MprMkr.generated(edge1).size(), 1);  // fuse1 has a new vertex generated by edge1
    EXPECT_EQ(fuse1MprMkr.generated(edge2).size(), 1);  // fuse1 has a new vertex generated by edge2
    EXPECT_EQ(fuse1MprMkr.generated(edge3).size(),
              0);  // fuse1 doesn't have a new vertex generated by edge3

    EXPECT_EQ(fuse2MprMkr.generated(edge1).size(), 1);  // fuse2 has a new vertex generated by edge1
    EXPECT_EQ(fuse2MprMkr.generated(edge2).size(),
              0);  // fuse2 doesn't have a new vertex generated by edge2
    EXPECT_EQ(fuse2MprMkr.generated(edge3).size(), 1);  // fuse2 has a new vertex generated by edge3
}

// ================================================================================================
//  The following test has been disabled to avoid the CI failing
//  will be enabled again in following PRs
// ================================================================================================

// TEST_F(TopoShapeExpansionTest, makeElementWiresCombinesAdjacent)
// {
//     // Arrange
//     auto edge1 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
//     auto edge2 = BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(2.0, 0.0, 0.0)).Edge();
//     TopoShape topoShape;
//     std::vector<TopoShape> shapes {edge1, edge2};

//     // Act
//     topoShape.makeElementWires(shapes);

//     // Assert
//     auto elementMap = topoShape.getElementMap();
//     EXPECT_EQ(6, elementMap.size());
// }

// ================================================================================================

TEST_F(TopoShapeExpansionTest, makeElementFaceNull)
{
    // Arrange
    const double Len = 3, Wid = 2, Rad = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(Len, Wid, Rad);
    TopoShape topoShape {face1};
    double area = getArea(face1);
    double area1 = getArea(topoShape.getShape());
    // Act
    TopoShape newFace = topoShape.makeElementFace(nullptr);
    double area2 = getArea(newFace.getShape());
    double area3 = getArea(topoShape.getShape());
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
    const float Len = 3;
    const float Wid = 2;
    const float Rad = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(Len, Wid, Rad);
    TopoShape topoShape {face1};
    double area = getArea(face1);
    double area1 = getArea(topoShape.getShape());
    // Act
    TopoShape newFace = topoShape.makeElementFace(wire1);
    double area2 = getArea(newFace.getShape());
    double area3 = getArea(topoShape.getShape());
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
    const float Len = 3;
    const float Wid = 2;
    const float Rad = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(Len, Wid, Rad);
    TopoShape topoShape {face1, 1L};
    double area = getArea(face1);
    double area1 = getArea(topoShape.getShape());
    // Act
    TopoShape newFace = topoShape.makeElementFace(wire1, "Cut", "Part::FaceMakerBullseye", nullptr);
    double area2 = getArea(newFace.getShape());
    double area3 = getArea(topoShape.getShape());
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
    const float Len = 3;
    const float Wid = 2;
    const float Rad = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(Len, Wid, Rad);
    TopoShape topoShape {face1, 1L};
    double area = getArea(face1);
    double area1 = getArea(topoShape.getShape());
    // Act
    TopoShape newFace = topoShape.makeElementFace(face1, "Cut", "Part::FaceMakerBullseye", nullptr);
    double area2 = getArea(newFace.getShape());
    double area3 = getArea(topoShape.getShape());
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
    const float Len = 3;
    const float Wid = 2;
    const float Rad = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(Len, Wid, Rad);
    TopoShape topoShape {wire1, 1L};
    double area = getArea(face1);
    double area1 = getArea(topoShape.getShape());
    // Act
    TopoShape newFace = topoShape.makeElementFace(wire1, "Cut", nullptr, nullptr);
    double area2 = getArea(newFace.getShape());
    double area3 = getArea(topoShape.getShape());
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
    const float Len = 3;
    const float Wid = 2;
    const float Rad = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(Len, Wid, Rad);
    TopoShape topoShape {wire2, 1L};
    double area = getArea(face1);
    double area1 = getArea(topoShape.getShape());
    // Act
    TopoShape newFace = topoShape.makeElementFace(wire2, "Cut", "Part::FaceMakerBullseye", nullptr);
    double area2 = getArea(newFace.getShape());
    double area3 = getArea(topoShape.getShape());
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
    TopoShape topoShape(1L);
    // Act
    Data::MappedName result = topoShape.setElementComboName(Data::IndexedName(), {});
    // ASSERT
    EXPECT_STREQ(result.toString().c_str(), "");
}


TEST_F(TopoShapeExpansionTest, setElementComboNameSimple)
{
    // Arrange
    auto edge1 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    TopoShape topoShape(edge1, 1L);
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
    TopoShape topoShape(2L);
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
                                                            OpCodes::Common,
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
    TopoShape topoShape(2L);
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
                                                            OpCodes::Common,
                                                            op);
    // ASSERT
    EXPECT_STREQ(result.toString().c_str(), "Edge1;:H,E;CMN(Face7|Face8);Copy");
    // The detailed forms of names are covered in encodeElementName tests
}

TEST_F(TopoShapeExpansionTest, splitWires)
{
    // Arrange
    const float Len = 3;
    const float Wid = 2;
    const float Rad = 1;
    auto [face1, wire1, wire2] = CreateFaceWithRoundHole(Len, Wid, Rad);
    TopoShape topoShape {face1, 1L};
    std::vector<TopoShape> inner;
    // Act
    EXPECT_EQ(topoShape.getShape().Orientation(), TopAbs_FORWARD);
    TopoShape wire = topoShape.splitWires(&inner, TopoShape::SplitWireReorient::ReorientReversed);
    // Assert
    EXPECT_EQ(inner.size(), 1);
    EXPECT_FLOAT_EQ(getLength(wire.getShape()), 2 + 2 + 3 + 3);
    EXPECT_FLOAT_EQ(getLength(inner.front().getShape()), M_PI * Rad * 2);
    EXPECT_EQ(wire.getShape().Orientation(), TopAbs_REVERSED);
    for (TopoShape& shape : inner) {
        EXPECT_EQ(shape.getShape().Orientation(), TopAbs_FORWARD);
    }
}

// Possible future tests:
// splitWires without inner Wires
// splitWires with all four reorientation values NoReorient, ReOrient, ReorientForward,
// ReorientReversed

TEST_F(TopoShapeExpansionTest, getSubTopoShapeByEnum)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape cube1TS {cube1};
    cube1TS.Tag = 1L;

    // Act
    auto subShape = cube1TS.getSubTopoShape(TopAbs_FACE, 1);
    auto subShape2 = cube1TS.getSubTopoShape(TopAbs_FACE, 2);
    auto subShape3 = cube1TS.getSubTopoShape(TopAbs_FACE, 6);
    auto noshape1 = cube1TS.getSubTopoShape(TopAbs_FACE, 7, true);
    // Assert
    EXPECT_EQ(subShape.getShape().ShapeType(), TopAbs_FACE);
    EXPECT_EQ(subShape2.getShape().ShapeType(), TopAbs_FACE);
    EXPECT_EQ(subShape2.getShape().ShapeType(), TopAbs_FACE);
    EXPECT_TRUE(noshape1.isNull());
    EXPECT_THROW(cube1TS.getSubTopoShape(TopAbs_FACE, 7), Base::IndexError);  // Out of range
}

TEST_F(TopoShapeExpansionTest, getSubTopoShapeByStringDefaults)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    Part::TopoShape cube1TS {cube1};
    cube1TS.Tag = 1L;
    const float Len = 3;
    const float Wid = 2;
    auto [face1, wire1, edge1, edge2, edge3, edge4] = CreateRectFace(Len, Wid);
    TopoDS_Compound compound1;
    TopoDS_Builder builder {};
    builder.MakeCompound(compound1);
    builder.Add(compound1, face1);
    TopoShape topoShape {compound1, 2L};
    // Act
    auto subShape = cube1TS.getSubTopoShape(nullptr);
    auto subShape1 = cube1TS.getSubTopoShape("");
    auto subShape2 = topoShape.getSubTopoShape(nullptr);
    // Assert
    EXPECT_TRUE(subShape.getShape().IsEqual(cube1TS.getShape()));
    EXPECT_EQ(subShape.getShape().ShapeType(), TopAbs_SOLID);
    EXPECT_TRUE(subShape1.getShape().IsEqual(cube1TS.getShape()));
    EXPECT_EQ(subShape1.getShape().ShapeType(), TopAbs_SOLID);
    EXPECT_TRUE(subShape2.getShape().IsEqual(face1));
    EXPECT_EQ(subShape2.getShape().ShapeType(), TopAbs_FACE);
}

TEST_F(TopoShapeExpansionTest, getSubTopoShapeByStringNames)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape cube1TS {cube1};
    cube1TS.Tag = 1;

    // Act
    auto subShape = cube1TS.getSubTopoShape("Face1");
    auto subShape2 = cube1TS.getSubTopoShape("Face2");
    auto subShape3 = cube1TS.getSubTopoShape("Face3");
    auto noshape1 = cube1TS.getSubTopoShape("Face7", true);  // Out of range
    // Assert
    EXPECT_EQ(subShape.getShape().ShapeType(), TopAbs_FACE);
    EXPECT_EQ(subShape2.getShape().ShapeType(), TopAbs_FACE);
    EXPECT_EQ(subShape3.getShape().ShapeType(), TopAbs_FACE);
    EXPECT_TRUE(noshape1.isNull());
    EXPECT_THROW(cube1TS.getSubTopoShape("Face7"), Base::IndexError);          // Out of range
    EXPECT_THROW(cube1TS.getSubTopoShape("WOOHOO", false), Base::ValueError);  // Invalid
}

TEST_F(TopoShapeExpansionTest, mapSubElementInvalidParm)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape cube1TS {cube1};
    cube1TS.Tag = 1;

    // Act
    std::vector<TopoShape> subShapes = cube1TS.getSubTopoShapes(TopAbs_FACE);
    TopoShape face1 = subShapes.front();
    face1.Tag = 2;

    // Assert
    EXPECT_THROW(cube1TS.mapSubElement(face1), NullShapeException);  // No subshapes
}

TEST_F(TopoShapeExpansionTest, mapSubElementFindShapeByNames)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape cube1TS {cube1};
    TopoShape cube2TS {cube2};
    cube1TS.Tag = 1;
    cube2TS.Tag = 2;
    TopoShape topoShape;
    TopoShape topoShape1;

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
    TopoShape cube1TS {cube1};
    TopoShape cube2TS {cube2};
    cube1TS.Tag = 1;
    cube2TS.Tag = 2;
    TopoShape topoShape;
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
    TopoShape cube1TS {cube1};
    TopoShape cube2TS {cube2};
    cube1TS.Tag = 1;
    cube2TS.Tag = 2;
    TopoShape topoShape;
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
    TopoShape cube1TS {cube1};
    TopoShape cube2TS {cube2};
    TopoShape cube3TS {cube3};
    TopoShape cube4TS {cube4};
    cube1TS.Tag = 1;
    cube2TS.Tag = 2;
    cube3TS.Tag = 3;
    cube4TS.Tag = 4;
    TopoShape topoShape;
    TopoShape topoShape1;
    TopoShape topoShape2;
    TopoShape topoShape3;
    TopoShape topoShape4;
    TopoShape topoShape5;
    TopoShape topoShape6;
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

TEST_F(TopoShapeExpansionTest, makeElementShellInvalid)
{
    // Arrange
    TopoShape topoShape {1L};
    // Act / Assert
    EXPECT_THROW(topoShape.makeElementShell(false, nullptr), Base::CADKernelError);
}

TEST_F(TopoShapeExpansionTest, makeElementShellSingle)
{
    // Arrange
    const float Len = 3;
    const float Wid = 2;
    auto [face1, wire1, edge1, edge2, edge3, _] = CreateRectFace(Len, Wid);
    TopoShape topoShape {face1, 1L};
    // Act
    TopoShape result = topoShape.makeElementShell(false, nullptr);
    // Assert
#if OCC_VERSION_HEX >= 0x070400
    EXPECT_EQ(result.getShape().NbChildren(), 1);
#endif
    EXPECT_EQ(result.countSubElements("Vertex"), 4);
    EXPECT_EQ(result.countSubElements("Edge"), 4);
    EXPECT_EQ(result.countSubElements("Face"), 1);
    EXPECT_STREQ(result.shapeName().c_str(), "Shell");
}

TEST_F(TopoShapeExpansionTest, makeElementShellOpen)
{
    // Arrange
    const float Len = 3;
    const float Wid = 2;
    auto [face1, wire1, edge1, edge2, edge3, edge4] = CreateRectFace(Len, Wid);
    auto transform {gp_Trsf()};
    transform.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0)), M_PI / 2);
    auto face2 = face1;  // Shallow copy
    face2.Move(TopLoc_Location(transform));
    TopoDS_Compound compound1;
    TopoDS_Builder builder {};
    builder.MakeCompound(compound1);
    builder.Add(compound1, face1);
    builder.Add(compound1, face2);
    TopoShape topoShape {compound1, 1L};
    // Act
    TopoShape result = topoShape.makeElementShell(true, nullptr);
    // Assert
#if OCC_VERSION_HEX >= 0x070400
    EXPECT_EQ(result.getShape().NbChildren(), 2);
#endif
    EXPECT_EQ(result.countSubElements("Vertex"), 6);
    EXPECT_EQ(result.countSubElements("Edge"), 7);
    EXPECT_EQ(result.countSubElements("Face"), 2);
    EXPECT_STREQ(result.shapeName().c_str(), "Shell");
}

TEST_F(TopoShapeExpansionTest, makeElementShellClosed)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape topoShape {cube1};
    std::vector<TopoShape> shapes;
    for (const auto& face : topoShape.getSubShapes(TopAbs_FACE)) {
        shapes.emplace_back(face);
    }
    // Act
    TopoShape topoShape1 {1L};
    topoShape1.makeElementCompound(shapes, "D");
    // Assert
    TopoShape result = topoShape1.makeElementShell(false, "SH1");
#if OCC_VERSION_HEX >= 0x070400
    EXPECT_EQ(result.getShape().NbChildren(), 6);
#endif
    EXPECT_EQ(result.countSubElements("Vertex"), 8);
    EXPECT_EQ(result.countSubElements("Edge"), 12);
    EXPECT_EQ(result.countSubElements("Face"), 6);
    EXPECT_STREQ(result.shapeName().c_str(), "Shell");
}

TEST_F(TopoShapeExpansionTest, makeElementShellIntersecting)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    auto transform {gp_Trsf()};
    transform.SetTranslation(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(0.5, 0.5, 0.0));
    cube2.Move(TopLoc_Location(transform));
    TopoShape topoShape {cube1};
    std::vector<TopoShape> shapes;
    for (const auto& face : topoShape.getSubShapes(TopAbs_FACE)) {
        shapes.emplace_back(face);
    }
    topoShape.setShape(cube2);
    for (const auto& face : topoShape.getSubShapes(TopAbs_FACE)) {
        shapes.emplace_back(face);
    }
    // Act
    TopoShape topoShape1 {1L};
    topoShape1.makeElementCompound(shapes, "D");
    // Assert
    EXPECT_THROW(topoShape1.makeElementShell(false, nullptr), Base::CADKernelError);
}

// TEST_F(TopoShapeExpansionTest, makeElementShellFromWires)
// {
//     // Arrange
// }
TEST_F(TopoShapeExpansionTest, makeElementBooleanImpossibleCommon)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape topoShape1 {cube1, 1L};
    TopoShape topoShape2 {cube2, 2L};
    // Act
    TopoShape& result =
        topoShape1.makeElementBoolean(Part::OpCodes::Common, {topoShape1, topoShape2});
    auto elements = elementMap(result);
    // Assert
    EXPECT_EQ(elements.size(), 0);
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 0);
}

TEST_F(TopoShapeExpansionTest, makeElementBooleanCommon)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    auto tr {gp_Trsf()};
    tr.SetTranslation(gp_Vec(gp_XYZ(-0.5, -0.5, 0)));
    cube2.Move(TopLoc_Location(tr));
    TopoShape topoShape1 {cube1, 1L};
    TopoShape topoShape2 {cube2, 2L};
    // Act
    TopoShape& result =
        topoShape1.makeElementBoolean(Part::OpCodes::Common, {topoShape1, topoShape2});
    auto elements = elementMap(result);
    // Assert
    EXPECT_EQ(elements.size(), 26);
    EXPECT_EQ(elements.count(IndexedName("Face", 1)), 1);
    EXPECT_EQ(elements[IndexedName("Face", 1)], MappedName("Face3;:M;CMN;:H1:7,F"));
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 0.25);
}

TEST_F(TopoShapeExpansionTest, makeElementBooleanCut)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    auto tr {gp_Trsf()};
    tr.SetTranslation(gp_Vec(gp_XYZ(-0.5, -0.5, 0)));
    cube2.Move(TopLoc_Location(tr));
    TopoShape topoShape1 {cube1, 1L};
    TopoShape topoShape2 {cube2, 2L};
    // Act
    TopoShape& result = topoShape1.makeElementBoolean(Part::OpCodes::Cut, {topoShape1, topoShape2});
    auto elements = elementMap(result);
    // Assert
    EXPECT_EQ(elements.size(), 38);
    EXPECT_EQ(elements.count(IndexedName("Face", 1)), 1);
    EXPECT_EQ(
        elements[IndexedName("Face", 1)],
        MappedName(
            "Face3;:M;CUT;:H1:7,F;:U;CUT;:H1:7,E;:L(Face5;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E|Face5;:M;"
            "CUT;:H1:7,F;:U2;CUT;:H1:8,E;:U;CUT;:H1:7,V;:L(Face6;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E;:U;"
            "CUT;:H1:7,V);CUT;:H1:3c,E|Face6;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E);CUT;:H1:cb,F"));
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 0.75);
}

TEST_F(TopoShapeExpansionTest, makeElementBooleanFuse)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    auto tr {gp_Trsf()};
    tr.SetTranslation(gp_Vec(gp_XYZ(-0.5, -0.5, 0)));
    cube2.Move(TopLoc_Location(tr));
    TopoShape topoShape1 {cube1, 1L};
    TopoShape topoShape2 {cube2, 2L};
    // Act
    TopoShape& result =
        topoShape1.makeElementBoolean(Part::OpCodes::Fuse, {topoShape1, topoShape2});
    auto elements = elementMap(result);
    // Assert
    EXPECT_EQ(elements.size(), 66);
    EXPECT_EQ(elements.count(IndexedName("Face", 1)), 1);
    EXPECT_EQ(
        elements[IndexedName("Face", 1)],
        MappedName(
            "Face3;:M;FUS;:H1:7,F;:U;FUS;:H1:7,E;:L(Face5;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E|Face5;:M;"
            "FUS;:H1:7,F;:U2;FUS;:H1:8,E;:U;FUS;:H1:7,V;:L(Face6;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E;:U;"
            "FUS;:H1:7,V);FUS;:H1:3c,E|Face6;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E);FUS;:H1:cb,F"));
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 1.75);
}

TEST_F(TopoShapeExpansionTest, makeElementDraft)
{  // Draft as in Draft Angle or sloped sides for removing shapes from a mold.
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape cube1TS {cube1, 1L};
    std::vector<TopoShape> subShapes = cube1TS.getSubTopoShapes(TopAbs_FACE);
    std::vector<TopoShape> faces {subShapes[0], subShapes[1], subShapes[2], subShapes[3]};
    const gp_Dir pullDirection {0, 0, 1};
    double angle {M_PI * 10
                  / 8};  // Angle should be between Pi and Pi * 1.5 ( 180 and 270 degrees )
    const gp_Pln plane {};
    // Act
    TopoShape& result = cube1TS.makeElementDraft(cube1TS, faces, pullDirection, angle, plane);
    auto elements = elementMap(result);
    // Assert
    EXPECT_EQ(elements.size(), 26);  // Cubes have 6 Faces, 12 Edges, 8 Vertexes
    EXPECT_NEAR(getVolume(result.getShape()), 4.3333333333, 1e-06);  // Truncated pyramid
}

TEST_F(TopoShapeExpansionTest, makeElementDraftTopoShapes)
{
    // Arrange
    auto [cube1TS, cube2TS] = CreateTwoTopoShapeCubes();
    const gp_Dir pullDirection {0, 0, 1};
    double angle {M_PI * 10
                  / 8};  // Angle should be between Pi and Pi * 1.5 ( 180 and 270 degrees )
    const gp_Pln plane {};
    // Act
    TopoShape result3 = cube1TS.makeElementDraft(cube1TS.getSubTopoShapes(TopAbs_FACE),
                                                 pullDirection,
                                                 angle,
                                                 plane);  // Non Reference call type
    TopoShape result2 = cube1TS.makeElementDraft(cube1TS,
                                                 cube1TS.getSubTopoShapes(TopAbs_FACE),
                                                 pullDirection,
                                                 angle,
                                                 plane);  // Bad use of Reference call
    TopoShape& result = cube1TS.makeElementDraft(cube2TS,
                                                 cube2TS.getSubTopoShapes(TopAbs_FACE),
                                                 pullDirection,
                                                 angle,
                                                 plane);  // Correct usage
    auto elements = elementMap((result));
    // Assert
    EXPECT_TRUE(result.getMappedChildElements().empty());
    EXPECT_EQ(elements.size(), 26);
    EXPECT_EQ(elements.count(IndexedName("Face", 1)), 1);
    EXPECT_EQ(elements[IndexedName("Face", 1)], MappedName("Face1;:G;DFT;:He:7,F"));
    EXPECT_NEAR(getVolume(result.getShape()), 4.3333333333, 1e-06);  // Truncated pyramid
    EXPECT_EQ(result2.getElementMap().size(), 0);  // No element map in non reference call.
    EXPECT_EQ(result3.getElementMap().size(), 0);  // No element map in non reference call.
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
