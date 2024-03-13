// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"
#include "src/App/InitApplication.h"
#include <Mod/Part/App/TopoShape.h>
#include "Mod/Part/App/TopoShapeMapper.h"
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "PartTestHelpers.h"

#include <boost/core/ignore_unused.hpp>
#include <BRepAdaptor_CompCurve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepFeat_SplitShape.hxx>
#include <BRepOffsetAPI_MakeEvolved.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_BSplineCurve.hxx>
#include <gp_Pln.hxx>
#include <ShapeFix_Wireframe.hxx>
#include <ShapeBuild_ReShape.hxx>
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
    TopoShape topoShape {1L};
    std::vector<TopoShape> shapes {TopoShape(edge1, 2L), TopoShape(edge2, 3L)};
    // Act
    topoShape.makeElementCompound(shapes);
    auto elements = elementMap((topoShape));
    Base::BoundBox3d bb = topoShape.getBoundBox();
    // Assert shape is correct
    EXPECT_FLOAT_EQ(getLength(topoShape.getShape()), 2);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, 0, 2, 0, 0)));
    // Assert map is correct
    EXPECT_FALSE(topoShape.getMappedChildElements().empty());
    EXPECT_EQ(elements.size(), 6);
    EXPECT_EQ(elements[IndexedName("Edge", 1)], MappedName("Edge1;:H2,E"));
    EXPECT_EQ(elements[IndexedName("Edge", 2)], MappedName("Edge1;:H3,E"));
    EXPECT_EQ(elements[IndexedName("Vertex", 1)], MappedName("Vertex1;:H2,V"));
    EXPECT_EQ(elements[IndexedName("Vertex", 2)], MappedName("Vertex2;:H2,V"));
    EXPECT_EQ(elements[IndexedName("Vertex", 3)], MappedName("Vertex1;:H3,V"));
    EXPECT_EQ(elements[IndexedName("Vertex", 4)], MappedName("Vertex2;:H3,V"));
}

TEST_F(TopoShapeExpansionTest, makeElementCompoundTwoCubes)
{
    auto [cube1TS, cube2TS] = CreateTwoTopoShapeCubes();
    // Act
    TopoShape topoShape {3L};
    topoShape.makeElementCompound({cube1TS, cube2TS});
    auto elementMap = cube1TS.getElementMap();
    Base::BoundBox3d bb = topoShape.getBoundBox();
    // Assert shape is correct
    EXPECT_EQ(22,
              topoShape.getMappedChildElements()
                  .size());  // Changed with PR#12471. Probably will change again after importing
                             // other TopoNaming logics
    EXPECT_FLOAT_EQ(getVolume(topoShape.getShape()), 2);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, 0, 2, 1, 1)));
    // Assert map is correct
    // Two cubes, each consisting of:
    // 8 Vertices
    // 12 Edges
    // 6 Faces
    // ----------
    // 26 subshapes each
    EXPECT_TRUE(allElementsMatch(
        topoShape,
        {
            "Vertex1;:H1,V;:H7:6,V", "Vertex2;:H1,V;:H7:6,V", "Vertex3;:H1,V;:H7:6,V",
            "Vertex4;:H1,V;:H7:6,V", "Vertex1;:H2,V;:H7:6,V", "Vertex2;:H2,V;:H7:6,V",
            "Vertex3;:H2,V;:H7:6,V", "Vertex4;:H2,V;:H7:6,V", "Face1;:H8,F;:He:6,F",
            "Face1;:H9,F;:He:6,F",   "Face1;:Ha,F;:He:6,F",   "Face1;:Hb,F;:He:6,F",
            "Face1;:Hc,F;:He:6,F",   "Face1;:Hd,F;:He:6,F",   "Edge1;:H8,E;:He:6,E",
            "Edge2;:H8,E;:He:6,E",   "Edge3;:H8,E;:He:6,E",   "Edge4;:H8,E;:He:6,E",
            "Edge1;:H9,E;:He:6,E",   "Edge2;:H9,E;:He:6,E",   "Edge3;:H9,E;:He:6,E",
            "Edge4;:H9,E;:He:6,E",   "Edge1;:Ha,E;:He:6,E",   "Edge2;:Ha,E;:He:6,E",
            "Edge3;:Ha,E;:He:6,E",   "Edge4;:Ha,E;:He:6,E",   "Vertex1;:H8,V;:He:6,V",
            "Vertex2;:H8,V;:He:6,V", "Vertex3;:H8,V;:He:6,V", "Vertex4;:H8,V;:He:6,V",
            "Vertex1;:H9,V;:He:6,V", "Vertex2;:H9,V;:He:6,V", "Vertex3;:H9,V;:He:6,V",
            "Vertex4;:H9,V;:He:6,V", "Edge1;:H1,E;:H7:6,E",   "Edge2;:H1,E;:H7:6,E",
            "Edge3;:H1,E;:H7:6,E",   "Edge4;:H1,E;:H7:6,E",   "Edge1;:H2,E;:H7:6,E",
            "Edge2;:H2,E;:H7:6,E",   "Edge3;:H2,E;:H7:6,E",   "Edge4;:H2,E;:H7:6,E",
            "Edge1;:H3,E;:H7:6,E",   "Edge2;:H3,E;:H7:6,E",   "Edge3;:H3,E;:H7:6,E",
            "Edge4;:H3,E;:H7:6,E",   "Face1;:H1,F;:H7:6,F",   "Face1;:H2,F;:H7:6,F",
            "Face1;:H3,F;:H7:6,F",   "Face1;:H4,F;:H7:6,F",   "Face1;:H5,F;:H7:6,F",
            "Face1;:H6,F;:H7:6,F",
        }));  // Changed with PR#12471. Probably will change again after importing
              // other TopoNaming logics
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
    const auto& face = faceMkr.Face();

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

TEST_F(TopoShapeExpansionTest, MapperHistoryModified)
{
    // Arrange
    // Creating a all the shapes needed for the operations that have a history
    auto vertex1 {BRepBuilderAPI_MakeVertex(gp_Pnt(-1.0, -1.0, 0.0)).Vertex()};
    auto vertex2 {BRepBuilderAPI_MakeVertex(gp_Pnt(1.0, 0.0, 0.0)).Vertex()};
    auto edge1 {BRepBuilderAPI_MakeEdge(vertex1, vertex2).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(-1.0, 0.0, 0.0), gp_Pnt(0.0, 1.0, 0.0)).Edge()};
    auto edge3 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 1.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge()};
    auto wire {BRepBuilderAPI_MakeWire(edge1, edge2, edge3).Wire()};

    // Definition of a MapperHistory made with ShapeBuild_ReShape and of all the objects needed
    // (https://dev.opencascade.org/doc/refman/html/class_shape_build___re_shape.html)
    // (https://dev.opencascade.org/doc/overview/html/occt_user_guides__shape_healing.html#occt_shg_5_1)
    Handle(ShapeBuild_ReShape) reshape {new ShapeBuild_ReShape()};
    // Recording all the shapes that will be modified
    vertex1 = reshape->CopyVertex(vertex1);
    vertex2 = reshape->CopyVertex(vertex2);
    reshape->Apply(edge1);
    auto reshapeMprHst {MapperHistory(reshape)};

    // Definition a MapperHistory made with ShapeFix_Wireframe and of all the objects needed
    // (https://dev.opencascade.org/doc/refman/html/class_shape_fix___wireframe.html)
    // (https://dev.opencascade.org/doc/overview/html/occt_user_guides__shape_healing.html#occt_shg_2_1)
    Handle(ShapeFix_Wireframe) fix {new ShapeFix_Wireframe()};
    fix->SetContext(reshape);
    fix->SetPrecision(0.0);
    auto fixMprHst {MapperHistory(*fix)};

    // Definition of a MapperHistory made with the BRepTools_History of reshape
    // (https://dev.opencascade.org/doc/refman/html/class_b_rep_tools___history.html)
    auto historyMprHst {MapperHistory(reshape->History())};

    // Act
    // Closing the wire
    fix->Load(wire);
    fix->FixWireGaps();

    // Replacing the edge with the new one made with the modified Vertexes
    reshape->Replace(edge1, BRepBuilderAPI_MakeEdge(vertex1, vertex2).Edge());
    reshape->Apply(edge1);

    // Assert
    // Check that all the shapes and operations have been performed
    EXPECT_TRUE(reshape->Status(ShapeExtend_DONE1));
    EXPECT_TRUE(reshape->Status(ShapeExtend_DONE3));
    EXPECT_TRUE(fix->StatusWireGaps(ShapeExtend_DONE));

    // Check the results of the operations after the ShapeFix_Wireframe.
    // The history is in common so all the MapperHistory object defined previously will return the
    // same values
    EXPECT_EQ(historyMprHst.modified(edge1).size(), 1);
    EXPECT_EQ(reshapeMprHst.modified(edge1).size(), 1);
    EXPECT_EQ(fixMprHst.modified(edge1).size(), 1);
}

TEST_F(TopoShapeExpansionTest, MapperHistoryGenerated)
{
    // Arrange
    // Creating a all the shapes needed for the operations that have a history
    auto edge1 {BRepBuilderAPI_MakeEdge(gp_Pnt(-1.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge()};
    auto edge2 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, -1.0, 0.0), gp_Pnt(0.0, 1.0, 0.0)).Edge()};
    auto edge3 {BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, -1.0), gp_Pnt(0.0, 0.0, 1.0)).Edge()};

    // Definition of a MapperHistory made with a BRepTools_History containing the Generated() shapes
    // of the Fuse operations, added manually to workaround a CI failure
    // (https://github.com/FreeCAD/FreeCAD/pull/12402#issuecomment-1946234571)
    auto fuse1Mkr {BRepAlgoAPI_Fuse(edge1, edge2)};
    Handle(BRepTools_History) fuse1Hst {new BRepTools_History()};
    auto fuse1MprHst {MapperHistory(fuse1Hst)};
    auto fuse2Mkr {BRepAlgoAPI_Fuse(edge1, edge3)};
    Handle(BRepTools_History) fuse2Hst {new BRepTools_History()};
    auto fuse2MprHst {MapperHistory(fuse2Hst)};

    // Act
    fuse1Mkr.Build();
    fuse1Hst->AddGenerated(edge1, fuse1Mkr);
    fuse1Hst->AddGenerated(edge2, fuse1Mkr);
    fuse2Mkr.Build();
    fuse2Hst->AddGenerated(edge1, fuse2Mkr);
    fuse2Hst->AddGenerated(edge3, fuse2Mkr);

    // Assert
    // Check that all the shapes and operations have been performed
    EXPECT_TRUE(fuse1Mkr.IsDone());
    EXPECT_TRUE(fuse2Mkr.IsDone());

    // Check the result of the operations
    EXPECT_EQ(fuse1MprHst.generated(edge1).size(), 1);  // fuse1 has a new vertex generated by edge1
    EXPECT_EQ(fuse1MprHst.generated(edge2).size(), 1);  // fuse1 has a new vertex generated by edge2
    EXPECT_EQ(fuse1MprHst.generated(edge3).size(),
              0);  // fuse1 doesn't have a new vertex generated by edge3

    EXPECT_EQ(fuse2MprHst.generated(edge1).size(), 1);  // fuse2 has a new vertex generated by edge1
    EXPECT_EQ(fuse2MprHst.generated(edge2).size(),
              0);  // fuse2 doesn't have a new vertex generated by edge2
    EXPECT_EQ(fuse2MprHst.generated(edge3).size(), 1);  // fuse2 has a new vertex generated by edge3
}

TEST_F(TopoShapeExpansionTest, resetElementMapTest)
{
    // Arrange
    // Creating various TopoShapes to check different conditions

    // A TopoShape without a map
    auto shapeWithoutMap {
        TopoShape(BRepBuilderAPI_MakeEdge(gp_Pnt(-1.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge(),
                  1)};

    // A TopoShape without a map that will be replaced by another map
    auto shapeWithoutMapAfterReset {TopoShape(shapeWithoutMap)};

    // A TopoShape with a map
    auto shapeWithMap {
        TopoShape(BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, -1.0, 0.0), gp_Pnt(0.0, 1.0, 0.0)).Edge(),
                  3)};
    shapeWithMap.makeShapeWithElementMap(shapeWithMap.getShape(),
                                         TopoShape::Mapper(),
                                         {shapeWithMap});

    // A TopoShape with a map that will be replaced by another map
    auto shapeWithMapAfterReset {TopoShape(shapeWithMap)};
    shapeWithMapAfterReset.makeShapeWithElementMap(shapeWithMapAfterReset.getShape(),
                                                   TopoShape::Mapper(),
                                                   {shapeWithMapAfterReset});

    // A TopoShape with a map that will be replaced by an empty map
    auto shapeWithMapAfterEmptyReset {TopoShape(shapeWithMap)};
    shapeWithMapAfterEmptyReset.makeShapeWithElementMap(shapeWithMapAfterEmptyReset.getShape(),
                                                        TopoShape::Mapper(),
                                                        {shapeWithMapAfterEmptyReset});

    // A new map
    auto newElementMapPtr {std::make_shared<Data::ElementMap>()};
    newElementMapPtr->setElementName(IndexedName("Edge", 2),
                                     MappedName("Edge2;:H,E"),
                                     3,
                                     nullptr,
                                     true);

    // Act
    shapeWithoutMapAfterReset.resetElementMap(newElementMapPtr);
    shapeWithMapAfterReset.resetElementMap(newElementMapPtr);
    shapeWithMapAfterEmptyReset.resetElementMap(nullptr);

    // Assert
    // Check that the original maps haven't been modified
    EXPECT_EQ(shapeWithoutMap.getElementMapSize(false), 0);
    EXPECT_EQ(shapeWithMap.getElementMapSize(false), 3);

    // Check that the two shapes have the same map
    EXPECT_EQ(shapeWithoutMapAfterReset.getElementMap(), shapeWithMapAfterReset.getElementMap());
    // Check that inside the shape's map there's the element of the new map (same result if
    // checking with the other shape)
    EXPECT_NE(shapeWithoutMapAfterReset.getElementMap()[0].name.find("Edge2"), -1);
    // Check that there aren't leftovers from the previous map
    EXPECT_EQ(shapeWithMapAfterReset.getElementMap()[0].name.find("Edge1"), -1);

    // Check that the map has been emptied
    EXPECT_EQ(shapeWithMapAfterEmptyReset.getElementMapSize(false), 0);
}

TEST_F(TopoShapeExpansionTest, flushElementMapTest)
{
    // Arrange
    // Creating various TopoShapes to check different conditions

    // A TopoShape with a map that won't be flushed
    auto shapeWithMapNotFlushed {
        TopoShape(BRepBuilderAPI_MakeEdge(gp_Pnt(-1.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge(),
                  1)};
    shapeWithMapNotFlushed.makeShapeWithElementMap(shapeWithMapNotFlushed.getShape(),
                                                   TopoShape::Mapper(),
                                                   {shapeWithMapNotFlushed});

    // A TopoShape with a map that will be reset and then flushed
    auto shapeWithMapFlushed {
        TopoShape(BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, -1.0, 0.0), gp_Pnt(0.0, 1.0, 0.0)).Edge(),
                  2)};
    shapeWithMapFlushed.makeShapeWithElementMap(shapeWithMapFlushed.getShape(),
                                                TopoShape::Mapper(),
                                                {shapeWithMapFlushed});

    // A child TopoShape that will be flushed
    auto childshapeWithMapFlushed {shapeWithMapFlushed.getSubTopoShape(TopAbs_VERTEX, 1)};
    childshapeWithMapFlushed.Tag = 3;

    // A new map
    auto newElementMapPtr {std::make_shared<Data::ElementMap>()};
    newElementMapPtr->setElementName(IndexedName("Edge", 2),
                                     MappedName("Edge2;:H,E"),
                                     3,
                                     nullptr,
                                     true);

    // Setting a different element map and then resetting otherwise flush won't have effect
    shapeWithMapFlushed.resetElementMap(newElementMapPtr);
    shapeWithMapFlushed.resetElementMap(nullptr);

    // Act
    shapeWithMapNotFlushed.flushElementMap();
    shapeWithMapFlushed.flushElementMap();
    childshapeWithMapFlushed.flushElementMap();

    // Assert
    // Check that the original map haven't been modified
    EXPECT_EQ(shapeWithMapNotFlushed.getElementMapSize(false), 3);

    // Check that the two maps have been flushed
    EXPECT_NE(shapeWithMapFlushed.getElementMap()[0].name.find("Edge2"), -1);
    EXPECT_NE(childshapeWithMapFlushed.getElementMap()[0].name.find("Vertex1"), -1);
}

TEST_F(TopoShapeExpansionTest, cacheRelatedElements)
{
    // Arrange
    TopoShape topoShape {3L};
    QVector<MappedElement> names {
        {MappedName {"Test1"}, IndexedName {"Test", 1}},
        {MappedName {"Test2"}, IndexedName {"Test", 2}},
        {MappedName {"OtherTest1"}, IndexedName {"OtherTest", 1}},
    };
    QVector<MappedElement> names2 {
        {MappedName {"Test3"}, IndexedName {"Test", 3}},
    };
    HistoryTraceType traceType = HistoryTraceType::followTypeChange;
    MappedName keyName {"Key1"};
    MappedName keyName2 {"Key2"};
    QVector<MappedElement> returnedNames;
    QVector<MappedElement> returnedNames2;
    QVector<MappedElement> returnedNames3;
    // Act
    topoShape.cacheRelatedElements(keyName, traceType, names);
    topoShape.cacheRelatedElements(keyName2, HistoryTraceType::stopOnTypeChange, names2);
    topoShape.getRelatedElementsCached(keyName, traceType, returnedNames);
    topoShape.getRelatedElementsCached(keyName, HistoryTraceType::stopOnTypeChange, returnedNames3);
    topoShape.getRelatedElementsCached(keyName2,
                                       HistoryTraceType::stopOnTypeChange,
                                       returnedNames2);
    // Assert
    EXPECT_EQ(returnedNames.size(), 3);
    EXPECT_STREQ(returnedNames[0].name.toString().c_str(), "Test1");
    EXPECT_EQ(returnedNames2.size(), 1);
    EXPECT_STREQ(returnedNames2[0].name.toString().c_str(), "Test3");
    EXPECT_EQ(returnedNames3.size(), 0);  // No entries of this type.
}

TEST_F(TopoShapeExpansionTest, makeElementWiresCombinesAdjacent)
{
    // Arrange
    auto edge1 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    auto edge2 = BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(2.0, 0.0, 0.0)).Edge();
    TopoShape topoShape {3L};
    std::vector<TopoShape> shapes {TopoShape(edge1, 1L), TopoShape(edge2, 2L)};
    //    std::vector<TopoShape> shapes {edge1, edge2};
    // Act
    topoShape.makeElementWires(shapes);
    auto elementMap = topoShape.getElementMap();
    // Assert
    EXPECT_EQ(6, elementMap.size());
}

TEST_F(TopoShapeExpansionTest, makeElementWiresCombinesWires)
{
    // Arrange
    auto edge1 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    auto edge2 = BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(2.0, 0.0, 0.0)).Edge();
    auto edge3 = BRepBuilderAPI_MakeEdge(gp_Pnt(3.0, 0.0, 0.0), gp_Pnt(2.0, 1.0, 0.0)).Edge();
    auto edge4 = BRepBuilderAPI_MakeEdge(gp_Pnt(2.0, 1.0, 0.0), gp_Pnt(2.0, 2.0, 0.0)).Edge();
    std::vector<TopoShape> shapes {TopoShape(edge1, 1L), TopoShape(edge2, 2L)};
    std::vector<TopoShape> shapes2 {TopoShape(edge3, 4L), TopoShape(edge4, 4L)};
    //    std::vector<TopoShape> shapes {edge1, edge2};
    // Act
    auto& wire1 = (new TopoShape {})->makeElementWires(shapes);
    auto& wire2 = (new TopoShape {})->makeElementWires(shapes2);
    auto& topoShape = (new TopoShape {})->makeElementWires({wire1, wire2});
    auto elements = elementMap((topoShape));
    Base::BoundBox3d bb = topoShape.getBoundBox();
    // Assert shape is correct
    EXPECT_FLOAT_EQ(getLength(topoShape.getShape()), 4.4142137);
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0, 0, 0, 3, 2, 0)));
    // Assert map is correct
    EXPECT_TRUE(allElementsMatch(topoShape,
                                 {
                                     "Edge1;:C1;:H4:4,E;WIR;:H4:4,E;WIR;:H4:4,E",
                                     "Edge1;:H1,E;WIR;:H1:4,E;WIR;:H1:4,E",
                                     "Edge1;:H2,E;WIR;:H2:4,E;WIR;:H2:4,E",
                                     "Edge1;:H4,E;WIR;:H4:4,E;WIR;:H4:4,E",
                                     "Vertex1;:H1,V;WIR;:H1:4,V;WIR;:H1:4,V",
                                     "Vertex1;:H4,V;WIR;:H4:4,V;WIR;:H4:4,V",
                                     "Vertex2;:C1;:H4:4,V;WIR;:H4:4,V;WIR;:H4:4,V",
                                     "Vertex2;:H1,V;WIR;:H1:4,V;WIR;:H1:4,V",
                                     "Vertex2;:H2,V;WIR;:H2:4,V;WIR;:H2:4,V",
                                     "Vertex2;:H4,V;WIR;:H4:4,V;WIR;:H4:4,V",
                                 }));  // Changed with PR#12471. Probably will change again after
                                       // importing other TopoNaming logics
}

TEST_F(TopoShapeExpansionTest, makeElementFaceNull)
{
    // Arrange
    const float Len = 3, Wid = 2, Rad = 1;
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
    TopoShape topoShape {1L};
    // Act
    Data::MappedName result = topoShape.setElementComboName(Data::IndexedName(), {});
    // ASSERT
    EXPECT_STREQ(result.toString().c_str(), "");
}


TEST_F(TopoShapeExpansionTest, setElementComboNameSimple)
{
    // Arrange
    auto edge1 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    TopoShape topoShape {edge1, 1L};
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
    TopoShape topoShape {2L};
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
    TopoShape topoShape {2L};
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
    EXPECT_STREQ(
        result.toString().c_str(),
        "Edge1;:H,E;CMN(Face7|Face8);Copy");  // Changed with PR#12471. Probably will change again
                                              // after importing other TopoNaming logics
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

TEST_F(TopoShapeExpansionTest, getSubShapes)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoTopoShapeCubes();
    // Act
    auto subShapes = cube1.getSubShapes();
    auto subShapes2 = cube1.getSubShapes(TopAbs_FACE);
    auto subShapes3 = cube1.getSubShapes(TopAbs_SHAPE, TopAbs_EDGE);
    // Assert
    EXPECT_EQ(subShapes.size(), 6);
    EXPECT_EQ(subShapes2.size(), 6);
    EXPECT_EQ(subShapes3.size(), 0);  // TODO:  Why doesn't this match the next test?
}

TEST_F(TopoShapeExpansionTest, getSubTopoShapes)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoTopoShapeCubes();
    // Act
    auto subShapes = cube1.getSubTopoShapes();
    auto subShapes2 = cube1.getSubTopoShapes(TopAbs_FACE);
    auto subShapes3 = cube1.getSubTopoShapes(TopAbs_SHAPE, TopAbs_EDGE);
    // Assert
    EXPECT_EQ(subShapes.size(), 6);
    EXPECT_EQ(subShapes2.size(), 6);
    EXPECT_EQ(subShapes3.size(), 6);
}

TEST_F(TopoShapeExpansionTest, getOrderedEdges)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoTopoShapeCubes();
    // Act
    auto subShapes = cube1.getOrderedEdges(MapElement::noMap);
    // Assert
    EXPECT_EQ(subShapes.size(), 24);
    //    EXPECT_THROW(cube1.getOrderedEdges(), NullShapeException);  // No Map
    EXPECT_EQ(subShapes.front().getElementMap().size(), 0);
    //    EXPECT_EQ(subShapes2.front().getElementMap().size(),2);
}

TEST_F(TopoShapeExpansionTest, getOrderedVertexes)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoTopoShapeCubes();
    // Act
    auto subShapes = cube1.getOrderedVertexes(MapElement::noMap);
    // Assert
    EXPECT_EQ(subShapes.size(), 24);
    //    EXPECT_THROW(cube1.getOrderedEdges(), NullShapeException);  // No Map
}

TEST_F(TopoShapeExpansionTest, getSubTopoShapeByEnum)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape cube1TS {cube1, 1L};

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
    Part::TopoShape cube1TS {cube1, 1L};
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
    TopoShape cube1TS {cube1, 1L};

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
    TopoShape cube1TS {cube1, 1L};
    TopoShape cube2TS {cube2, 2L};
    // Act
    std::vector<TopoShape> subShapes = cube1TS.getSubTopoShapes(TopAbs_FACE);
    TopoShape face1 = subShapes.front();
    face1.Tag = 3;
    cube1TS.mapSubElement(face1);
    cube2TS.mapSubElement(face1);
    // Assert
    EXPECT_EQ(cube1TS.getElementMap().size(), 9);  // Valid, the face is in Cube1
    EXPECT_EQ(cube2TS.getElementMap().size(), 0);  // Invalid, the face is not in Cube2
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

TEST_F(TopoShapeExpansionTest, findSubShapesWithSharedVertexEverything)
{
    // Arrange
    auto [box1, box2] = CreateTwoCubes();
    TopoShape box1TS {box1};
    std::vector<std::string> names;
    std::vector<std::string> names1;
    std::vector<std::string> names2;
    double tol {2};  // Silly big tolerance to get everything
    double atol {2};

    TopExp_Explorer exp(box1, TopAbs_FACE);
    auto face = exp.Current();
    exp.Init(box1, TopAbs_EDGE);
    auto edge = exp.Current();
    exp.Init(box1, TopAbs_VERTEX);
    auto vertex = exp.Current();
    // Act
    auto shapes =
        box1TS.findSubShapesWithSharedVertex(face, &names, CheckGeometry::checkGeometry, tol, atol);
    auto shapes1 = box1TS.findSubShapesWithSharedVertex(edge,
                                                        &names1,
                                                        CheckGeometry::checkGeometry,
                                                        tol,
                                                        atol);
    auto shapes2 = box1TS.findSubShapesWithSharedVertex(vertex,
                                                        &names2,
                                                        CheckGeometry::checkGeometry,
                                                        tol,
                                                        atol);
    //  Assert
    EXPECT_EQ(shapes.size(), 6);
    EXPECT_EQ(names.size(), 6);
    EXPECT_STREQ(names[0].c_str(), "Face1");
    EXPECT_STREQ(names[1].c_str(), "Face3");
    EXPECT_STREQ(names[2].c_str(), "Face6");
    EXPECT_STREQ(names[3].c_str(), "Face5");
    EXPECT_STREQ(names[4].c_str(), "Face4");
    EXPECT_STREQ(names[5].c_str(), "Face2");
    EXPECT_EQ(shapes1.size(), 12);
    EXPECT_EQ(names1.size(), 12);
    EXPECT_EQ(shapes2.size(), 8);
    EXPECT_EQ(names2.size(), 8);
}

TEST_F(TopoShapeExpansionTest, findSubShapesWithSharedVertexMid)
{
    // Arrange
    auto [box1, box2] = CreateTwoCubes();
    TopoShape box1TS {box1};
    std::vector<std::string> names;
    std::vector<std::string> names1;
    std::vector<std::string> names2;
    double tol {1e-0};
    double atol {1e-04};

    TopExp_Explorer exp(box1, TopAbs_FACE);
    auto face = exp.Current();
    exp.Init(box1, TopAbs_EDGE);
    auto edge = exp.Current();
    exp.Init(box1, TopAbs_VERTEX);
    auto vertex = exp.Current();
    // Act
    auto shapes =
        box1TS.findSubShapesWithSharedVertex(face, &names, CheckGeometry::checkGeometry, tol, atol);
    auto shapes1 = box1TS.findSubShapesWithSharedVertex(edge,
                                                        &names1,
                                                        CheckGeometry::checkGeometry,
                                                        tol,
                                                        atol);
    auto shapes2 = box1TS.findSubShapesWithSharedVertex(vertex,
                                                        &names2,
                                                        CheckGeometry::checkGeometry,
                                                        tol,
                                                        atol);
    //  Assert
    EXPECT_EQ(shapes.size(), 6);
    EXPECT_EQ(names.size(), 6);
    EXPECT_EQ(shapes1.size(), 7);
    EXPECT_EQ(names1.size(), 7);
    EXPECT_EQ(shapes2.size(), 4);
    EXPECT_EQ(names2.size(), 4);
}

TEST_F(TopoShapeExpansionTest, findSubShapesWithSharedVertexClose)
{
    // Arrange
    auto [box1, box2] = CreateTwoCubes();
    TopoShape box1TS {box1};
    std::vector<std::string> names;
    std::vector<std::string> names1;
    std::vector<std::string> names2;
    double tol {1e-02};
    double atol {1e-04};

    TopExp_Explorer exp(box1, TopAbs_FACE);
    auto face = exp.Current();
    exp.Init(box1, TopAbs_EDGE);
    auto edge = exp.Current();
    exp.Init(box1, TopAbs_VERTEX);
    auto vertex = exp.Current();
    // Act
    auto shapes =
        box1TS.findSubShapesWithSharedVertex(face, &names, CheckGeometry::checkGeometry, tol, atol);
    auto shapes1 = box1TS.findSubShapesWithSharedVertex(edge,
                                                        &names1,
                                                        CheckGeometry::checkGeometry,
                                                        tol,
                                                        atol);
    auto shapes2 = box1TS.findSubShapesWithSharedVertex(vertex,
                                                        &names2,
                                                        CheckGeometry::checkGeometry,
                                                        tol,
                                                        atol);
    //  Assert
    EXPECT_EQ(shapes.size(), 1);
    EXPECT_EQ(names.size(), 1);
    EXPECT_EQ(shapes1.size(), 1);
    EXPECT_EQ(names1.size(), 1);
    EXPECT_EQ(shapes2.size(), 1);
    EXPECT_EQ(names2.size(), 1);
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

TEST_F(TopoShapeExpansionTest, makeElementShellFromWires)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape topoShape {cube1};
    std::vector<TopoShape> shapes;
    for (const auto& face : topoShape.getSubShapes(TopAbs_WIRE)) {
        shapes.emplace_back(face);
    }
    // Act
    TopoShape topoShape1 {1L};
    topoShape1.makeElementCompound(shapes, "D");
    // Assert
    TopoShape result = topoShape1.makeElementShellFromWires(shapes);
#if OCC_VERSION_HEX >= 0x070400
    EXPECT_EQ(result.getShape().NbChildren(), 20);  // 6  TODO: VERSION DEPENDENT?
#endif
    EXPECT_EQ(result.countSubElements("Vertex"), 8);
    EXPECT_EQ(result.countSubElements("Edge"), 32);
    EXPECT_EQ(result.countSubElements("Face"), 20);
    EXPECT_STREQ(result.shapeName().c_str(), "Shell");
}

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
    // Assert shape is correct
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 0);
    // Assert elementMap is correct
    EXPECT_EQ(elements.size(), 0);
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
    // Assert shape is correct
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 0.25);
    // Assert elementMap is correct
    EXPECT_EQ(elements.size(), 26);
    EXPECT_EQ(elements.count(IndexedName("Face", 1)), 1);
    EXPECT_EQ(elements[IndexedName("Face", 1)], MappedName("Face3;:M;CMN;:H1:7,F"));
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
    // Assert shape is correct
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 0.75);
    // Assert elementMap is correct
    EXPECT_EQ(elements.size(), 38);
    EXPECT_EQ(elements.count(IndexedName("Face", 1)), 1);
    EXPECT_EQ(
        elements[IndexedName("Face", 1)],
        MappedName(
            "Face3;:M;CUT;:H1:7,F;:U;CUT;:H1:7,E;:L(Face5;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E|Face5;:M;"
            "CUT;:H1:7,F;:U2;CUT;:H1:8,E;:U;CUT;:H1:7,V;:L(Face6;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E;:U;"
            "CUT;:H1:7,V);CUT;:H1:3c,E|Face6;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E);CUT;:H1:cb,F"));
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
    // Assert shape is correct
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 1.75);
    // Assert element map is correct
    EXPECT_EQ(elements.size(), 66);
    EXPECT_EQ(elements.count(IndexedName("Face", 1)), 1);
    EXPECT_EQ(
        elements[IndexedName("Face", 1)],
        MappedName(
            "Face3;:M;FUS;:H1:7,F;:U;FUS;:H1:7,E;:L(Face5;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E|Face5;:M;"
            "FUS;:H1:7,F;:U2;FUS;:H1:8,E;:U;FUS;:H1:7,V;:L(Face6;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E;:U;"
            "FUS;:H1:7,V);FUS;:H1:3c,E|Face6;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E);FUS;:H1:cb,F"));
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
    EXPECT_NEAR(getVolume(result.getShape()), 4.3333333333, 1e-06);  // Truncated pyramid
    // Assert elementMap is correct
    EXPECT_EQ(elements.size(), 26);  // Cubes have 6 Faces, 12 Edges, 8 Vertexes
    EXPECT_EQ(elements[IndexedName("Edge", 1)], MappedName("Face1;:G;DFT;:H1:7,F;:U;DFT;:H1:7,E"));
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
    EXPECT_EQ(elements[IndexedName("Face", 1)], MappedName("Face1;:H8,F;:G;DFT;:He:7,F"));
    EXPECT_NEAR(getVolume(result.getShape()), 4.3333333333, 1e-06);  // Truncated pyramid
    EXPECT_EQ(result2.getElementMap().size(), 26);
    EXPECT_EQ(result3.getElementMap().size(), 26);
}

TEST_F(TopoShapeExpansionTest, linearizeEdge)
{
    // Arrange
    TColgp_Array1OfPnt points {1, 2};
    points.SetValue(1, gp_Pnt(0.0, 0.0, 0.0));
    points.SetValue(2, gp_Pnt(1.0, 0.0, 0.0));
    auto line1 = new Geom_BezierCurve(points);
    auto edge1 = BRepBuilderAPI_MakeEdge(line1).Edge();
    TopoShape topoShape1 {edge1, 1L};
    // Act
    auto edges = topoShape1.getSubTopoShapes(TopAbs_EDGE);
    BRepAdaptor_Curve curve(TopoDS::Edge(edges.front().getShape()));
    topoShape1.linearize(LinearizeFace::noFaces, LinearizeEdge::linearizeEdges);
    auto edges2 = topoShape1.getSubTopoShapes(TopAbs_EDGE);
    BRepAdaptor_Curve curve2(TopoDS::Edge(edges2.front().getShape()));
    // Assert
    EXPECT_EQ(curve.GetType(), GeomAbs_BezierCurve);
    EXPECT_EQ(curve2.GetType(), GeomAbs_Line);
}

TEST_F(TopoShapeExpansionTest, linearizeFace)
{
    TColgp_Array2OfPnt points2 {1, 2, 1, 2};
    points2.SetValue(1, 1, gp_Pnt(0.0, 0.0, 0.0));
    points2.SetValue(2, 1, gp_Pnt(1.0, 0.0, 0.0));
    points2.SetValue(1, 2, gp_Pnt(0.0, 1.0, 0.0));
    points2.SetValue(2, 2, gp_Pnt(1.0, 1.0, 0.0));
    auto face1 = new Geom_BezierSurface(points2);
    auto surf1 = BRepBuilderAPI_MakeFace(face1, 0.1).Face();
    TopoShape topoShape2 {surf1, 2L};
    // Act
    auto faces = topoShape2.getSubTopoShapes(TopAbs_FACE);
    BRepAdaptor_Surface surface(TopoDS::Face(faces.front().getShape()));
    topoShape2.linearize(LinearizeFace::linearizeFaces, LinearizeEdge::noEdges);
    auto faces2 = topoShape2.getSubTopoShapes(TopAbs_FACE);
    BRepAdaptor_Surface surface2(TopoDS::Face(faces.front().getShape()));
    // Assert
    EXPECT_EQ(surface.GetType(), GeomAbs_BezierSurface);
    EXPECT_EQ(surface2.GetType(), GeomAbs_Plane);
}

TEST_F(TopoShapeExpansionTest, makeElementRuledSurfaceEdges)
{
    // Arrange
    auto edge1 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(0.0, 0.0, 8.0)).Edge();
    auto edge2 = BRepBuilderAPI_MakeEdge(gp_Pnt(2.5, 0.0, 0.0), gp_Pnt(2.5, 0.0, 8.0)).Edge();
    TopoShape edge1ts {edge1, 2L};
    TopoShape edge2ts {edge2, 3L};
    TopoShape topoShape {1L};
    // Act
    topoShape.makeElementRuledSurface({edge1ts, edge2ts}, 0);  // TODO: orientation as enum?
    auto elements = elementMap(topoShape);
    // Assert shape is correct
    EXPECT_EQ(topoShape.countSubElements("Wire"), 1);
    EXPECT_FLOAT_EQ(getArea(topoShape.getShape()), 20);
    // Assert that we're creating a correct element map
    EXPECT_TRUE(topoShape.getMappedChildElements().empty());
    // TODO: Revisit these when resetElementMap() is fully worked through.  Suspect that last loop
    // of makeElementRuledSurface is dependent on this to create elementMaps.
    //    EXPECT_EQ(elements.size(), 24);
    //    EXPECT_EQ(elements.count(IndexedName("Edge", 1)), 1);
    //    EXPECT_EQ(elements[IndexedName("Edge", 1)], MappedName("Vertex1;:G;PSH;:H2:7,E"));
}

TEST_F(TopoShapeExpansionTest, makeElementRuledSurfaceWires)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape cube1TS {cube1, 1L};
    std::vector<TopoShape> subWires = cube1TS.getSubTopoShapes(TopAbs_WIRE);
    // Act
    cube1TS.makeElementRuledSurface({subWires[0], subWires[1]}, 0);  // TODO: orientation as enum?
    auto elements = elementMap(cube1TS);
    // Assert
    EXPECT_EQ(cube1TS.countSubElements("Wire"), 4);
    EXPECT_FLOAT_EQ(getArea(cube1TS.getShape()), 2.023056);
    // Assert that we're creating a correct element map
    EXPECT_TRUE(cube1TS.getMappedChildElements().empty());
    // TODO: Revisit these when resetElementMap() is fully worked through.  Suspect that last loop
    // of makeElementRuledSurface is dependent on this to create elementMaps.
    //    EXPECT_EQ(elements.size(), 24);
    //    EXPECT_EQ(elements.count(IndexedName("Edge", 1)), 1);
    //    EXPECT_EQ(elements[IndexedName("Edge", 1)], MappedName("Vertex1;:G;PSH;:H2:7,E"));
}

TEST_F(TopoShapeExpansionTest, makeElementLoft)
{
    // Loft must have either all open or all closed sections to work, we'll do two closed.
    // Arrange
    const float Len = 5;
    const float Wid = 5;
    auto [face1, wire1, edge1, edge2, edge3, edge4] = CreateRectFace(Len, Wid);
    auto transform {gp_Trsf()};
    transform.SetTranslation(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(0.0, 0.0, 10.0));
    auto wire2 = wire1;  // Shallow copy
    wire2.Move(TopLoc_Location(transform));
    TopoShape wire1ts {
        wire1,
        1L};  // One of these shapes should have a tag or else we won't get an Element Map
    TopoShape wire2ts {
        wire2,
        2L};  // If you change either tag or eliminate one it changes the resulting name.
    std::vector<TopoShape> shapes = {wire1ts, wire2ts};
    // Act
    auto& topoShape =
        (new TopoShape())->makeElementLoft(shapes, IsSolid::notSolid, IsRuled::notRuled);
    auto& topoShape2 =
        (new TopoShape())->makeElementLoft(shapes, IsSolid::solid, IsRuled::notRuled);
    auto& topoShape3 =
        (new TopoShape())->makeElementLoft(shapes, IsSolid::notSolid, IsRuled::ruled);
    auto& topoShape4 = (new TopoShape())->makeElementLoft(shapes, IsSolid::solid, IsRuled::ruled);
    auto& topoShape5 =
        (new TopoShape())
            ->makeElementLoft(shapes, IsSolid::notSolid, IsRuled::notRuled, IsClosed::closed);
    auto elements = elementMap((topoShape));
    // Assert that we haven't broken the basic Loft functionality
    EXPECT_EQ(topoShape.countSubElements("Wire"), 4);
    EXPECT_FLOAT_EQ(getArea(topoShape.getShape()), 200);
    EXPECT_FLOAT_EQ(getVolume(topoShape.getShape()), 166.66667);
    EXPECT_FLOAT_EQ(getVolume(topoShape2.getShape()), 250);
    EXPECT_FLOAT_EQ(getVolume(topoShape3.getShape()), 166.66667);
    EXPECT_FLOAT_EQ(getVolume(topoShape4.getShape()), 250);
    EXPECT_NEAR(getVolume(topoShape5.getShape()), 0, 1e-07);
    // Assert that we're creating a correct element map
    EXPECT_TRUE(topoShape.getMappedChildElements().empty());
    EXPECT_EQ(elements.size(), 24);
    EXPECT_EQ(elements.count(IndexedName("Edge", 1)), 1);
    EXPECT_EQ(elements[IndexedName("Edge", 1)], MappedName("Edge1;LFT;:H1:4,E"));
}

TEST_F(TopoShapeExpansionTest, makeElementPipeShell)
{
    // Arrange
    const float Len = 5;
    const float Wid = 5;
    auto [face1, wire1, edge1, edge2, edge3, edge4] = CreateRectFace(Len, Wid);
    auto edge5 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(0.0, 0.0, -8.0)).Edge();
    auto wire2 = BRepBuilderAPI_MakeWire({edge5}).Wire();
    TopoShape face1ts {face1, 1L};
    TopoShape edge5ts {edge5, 2L};
    std::vector<TopoShape> shapes = {face1ts, edge5ts};
    // Act
    auto& topoShape = (new TopoShape())->makeElementPipeShell(shapes, MakeSolid::noSolid, false);
    auto elements = elementMap((topoShape));
    // Assert that we haven't broken the basic Loft functionality
    EXPECT_EQ(topoShape.countSubElements("Wire"), 4);
    EXPECT_FLOAT_EQ(getArea(topoShape.getShape()), 160);
    EXPECT_FLOAT_EQ(getVolume(topoShape.getShape()), 133.33334);
    // Assert that we're creating a correct element map
    EXPECT_TRUE(topoShape.getMappedChildElements().empty());
    EXPECT_EQ(elements.size(), 24);
    EXPECT_EQ(elements.count(IndexedName("Edge", 1)), 1);
    EXPECT_EQ(elements[IndexedName("Edge", 1)], MappedName("Vertex1;:G;PSH;:H2:7,E"));
}

TEST_F(TopoShapeExpansionTest, makeElementThickSolid)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape cube1TS {cube1, 1L};
    std::vector<TopoShape> subFaces = cube1TS.getSubTopoShapes(TopAbs_FACE);
    subFaces[0].Tag = 2L;
    subFaces[1].Tag = 3L;
    std::vector<TopoShape> shapes = {subFaces[0], subFaces[1]};
    // Act
    cube1TS.makeElementThickSolid(cube1TS, shapes, 0.1, 1e-07);
    auto elements = elementMap(cube1TS);
    // Assert
    EXPECT_EQ(cube1TS.countSubElements("Wire"), 16);
    EXPECT_FLOAT_EQ(getArea(cube1TS.getShape()), 9.4911509);
    // Assert that we're creating a correct element map
    EXPECT_TRUE(cube1TS.getMappedChildElements().empty());
    EXPECT_EQ(elements.size(), 74);
    EXPECT_EQ(elements.count(IndexedName("Edge", 1)), 1);
    EXPECT_EQ(elements[IndexedName("Edge", 1)], MappedName("Edge11;THK;:H1:4,E"));
}

TEST_F(TopoShapeExpansionTest, makeElementGeneralFuse)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    auto tr {gp_Trsf()};
    tr.SetTranslation(gp_Vec(gp_XYZ(-0.5, -0.5, 0)));
    cube2.Move(TopLoc_Location(tr));
    TopoShape topoShape1 {cube1, 1L};
    TopoShape topoShape2 {cube2, 2L};
    // Act
    std::vector<std::vector<TopoShape>> modified {{}};
    TopoShape& result = topoShape1.makeElementGeneralFuse({topoShape1, topoShape2}, modified);

    auto elements = elementMap(result);
    // Assert shape is correct
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 1.75);
    // Assert elementMap is correct
    EXPECT_EQ(elements.size(), 72);
    EXPECT_EQ(elements.count(IndexedName("Face", 1)), 1);
    EXPECT_EQ(
        elements[IndexedName("Face", 1)],
        MappedName(
            "Face3;:M;GFS;:H1:7,F;:U;GFS;:H1:7,E;:L(Face5;:M;GFS;:H1:7,F;:U2;GFS;:H1:8,E|Face5;:M;"
            "GFS;:H1:7,F;:U2;GFS;:H1:8,E;:U;GFS;:H1:7,V;:L(Face6;:M;GFS;:H1:7,F;:U2;GFS;:H1:8,E;:U;"
            "GFS;:H1:7,V);GFS;:H1:3c,E|Face6;:M;GFS;:H1:7,F;:U2;GFS;:H1:8,E);GFS;:H1:cb,F"));
}

TEST_F(TopoShapeExpansionTest, makeElementFuse)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    auto tr {gp_Trsf()};
    tr.SetTranslation(gp_Vec(gp_XYZ(-0.5, -0.5, 0)));
    cube2.Move(TopLoc_Location(tr));
    TopoShape topoShape1 {cube1, 1L};
    TopoShape topoShape2 {cube2, 2L};
    // Act
    TopoShape& result = topoShape1.makeElementFuse({topoShape1, topoShape2});  // op, tolerance
    auto elements = elementMap(result);
    // Assert shape is correct
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 1.75);
    // Assert elementMap is correct
    EXPECT_EQ(elements.size(), 66);
    EXPECT_EQ(elements.count(IndexedName("Face", 1)), 1);
    EXPECT_EQ(
        elements[IndexedName("Face", 1)],
        MappedName(
            "Face3;:M;FUS;:H1:7,F;:U;FUS;:H1:7,E;:L(Face5;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E|Face5;:M;"
            "FUS;:H1:7,F;:U2;FUS;:H1:8,E;:U;FUS;:H1:7,V;:L(Face6;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E;:U;"
            "FUS;:H1:7,V);FUS;:H1:3c,E|Face6;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E);FUS;:H1:cb,F"));
}

TEST_F(TopoShapeExpansionTest, makeElementCut)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    auto tr {gp_Trsf()};
    tr.SetTranslation(gp_Vec(gp_XYZ(-0.5, -0.5, 0)));
    cube2.Move(TopLoc_Location(tr));
    TopoShape topoShape1 {cube1, 1L};
    TopoShape topoShape2 {cube2, 2L};
    // Act
    TopoShape& result = topoShape1.makeElementCut(
        {topoShape1, topoShape2});  //, const char* op = nullptr, double tol = 0);
    auto elements = elementMap(result);
    // Assert shape is correct
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 0.75);
    // Assert elementMap is correct
    EXPECT_EQ(elements.size(), 38);
    EXPECT_EQ(elements.count(IndexedName("Face", 1)), 1);
    EXPECT_EQ(
        elements[IndexedName("Face", 1)],
        MappedName(
            "Face3;:M;CUT;:H1:7,F;:U;CUT;:H1:7,E;:L(Face5;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E|Face5;:M;"
            "CUT;:H1:7,F;:U2;CUT;:H1:8,E;:U;CUT;:H1:7,V;:L(Face6;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E;:U;"
            "CUT;:H1:7,V);CUT;:H1:3c,E|Face6;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E);CUT;:H1:cb,F"));
}

TEST_F(TopoShapeExpansionTest, makeElementChamfer)
{
    // Arrange
    // Fillets / Chamfers do not work on compounds of faces, so use complete boxes ( Solids ) here.
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape cube1TS {cube1, 1L};
    auto edges = cube1TS.getSubTopoShapes(TopAbs_EDGE);
    // Act
    cube1TS.makeElementChamfer({cube1TS}, edges, .05, .05);
    auto elements = elementMap(cube1TS);
    // Assert shape is correct
    EXPECT_EQ(cube1TS.countSubElements("Wire"), 26);
    EXPECT_FLOAT_EQ(getArea(cube1TS.getShape()), 5.640996);
    // Assert that we're creating a correct element map
    EXPECT_TRUE(cube1TS.getMappedChildElements().empty());
    EXPECT_TRUE(allElementsMatch(cube1TS,
                                 {
                                     "Edge10;:G;CHF;:H1:7,F",
                                     "Edge10;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E",
                                     "Edge10;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E",
                                     "Edge10;:G;CHF;:H1:7,F;:U4;CHF;:H1:8,E",
                                     "Edge10;:G;CHF;:H1:7,F;:U;CHF;:H1:7,E",
                                     "Edge11;:G;CHF;:H1:7,F",
                                     "Edge11;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E",
                                     "Edge11;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E",
                                     "Edge11;:G;CHF;:H1:7,F;:U4;CHF;:H1:8,E",
                                     "Edge11;:G;CHF;:H1:7,F;:U;CHF;:H1:7,E",
                                     "Edge12;:G;CHF;:H1:7,F",
                                     "Edge12;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E",
                                     "Edge12;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E",
                                     "Edge12;:G;CHF;:H1:7,F;:U4;CHF;:H1:8,E",
                                     "Edge12;:G;CHF;:H1:7,F;:U;CHF;:H1:7,E",
                                     "Edge1;:G;CHF;:H1:7,F",
                                     "Edge1;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E",
                                     "Edge1;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U2;CHF;:H1:8,V",
                                     "Edge1;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U;CHF;:H1:7,V",
                                     "Edge1;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E",
                                     "Edge1;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E;:U2;CHF;:H1:8,V",
                                     "Edge1;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E;:U;CHF;:H1:7,V",
                                     "Edge1;:G;CHF;:H1:7,F;:U4;CHF;:H1:8,E",
                                     "Edge1;:G;CHF;:H1:7,F;:U;CHF;:H1:7,E",
                                     "Edge2;:G;CHF;:H1:7,F",
                                     "Edge2;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E",
                                     "Edge2;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U2;CHF;:H1:8,V",
                                     "Edge2;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U;CHF;:H1:7,V",
                                     "Edge2;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E",
                                     "Edge2;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E;:U2;CHF;:H1:8,V",
                                     "Edge2;:G;CHF;:H1:7,F;:U4;CHF;:H1:8,E",
                                     "Edge2;:G;CHF;:H1:7,F;:U;CHF;:H1:7,E",
                                     "Edge3;:G;CHF;:H1:7,F",
                                     "Edge3;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E",
                                     "Edge3;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U2;CHF;:H1:8,V",
                                     "Edge3;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U;CHF;:H1:7,V",
                                     "Edge3;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E",
                                     "Edge3;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E;:U;CHF;:H1:7,V",
                                     "Edge3;:G;CHF;:H1:7,F;:U4;CHF;:H1:8,E",
                                     "Edge3;:G;CHF;:H1:7,F;:U;CHF;:H1:7,E",
                                     "Edge4;:G;CHF;:H1:7,F",
                                     "Edge4;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E",
                                     "Edge4;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U2;CHF;:H1:8,V",
                                     "Edge4;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U;CHF;:H1:7,V",
                                     "Edge4;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E",
                                     "Edge4;:G;CHF;:H1:7,F;:U4;CHF;:H1:8,E",
                                     "Edge4;:G;CHF;:H1:7,F;:U;CHF;:H1:7,E",
                                     "Edge5;:G;CHF;:H1:7,F",
                                     "Edge5;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E",
                                     "Edge5;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U2;CHF;:H1:8,V",
                                     "Edge5;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U;CHF;:H1:7,V",
                                     "Edge5;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E",
                                     "Edge5;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E;:U2;CHF;:H1:8,V",
                                     "Edge5;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E;:U;CHF;:H1:7,V",
                                     "Edge5;:G;CHF;:H1:7,F;:U4;CHF;:H1:8,E",
                                     "Edge5;:G;CHF;:H1:7,F;:U;CHF;:H1:7,E",
                                     "Edge6;:G;CHF;:H1:7,F",
                                     "Edge6;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E",
                                     "Edge6;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U2;CHF;:H1:8,V",
                                     "Edge6;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U;CHF;:H1:7,V",
                                     "Edge6;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E",
                                     "Edge6;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E;:U2;CHF;:H1:8,V",
                                     "Edge6;:G;CHF;:H1:7,F;:U4;CHF;:H1:8,E",
                                     "Edge6;:G;CHF;:H1:7,F;:U;CHF;:H1:7,E",
                                     "Edge7;:G;CHF;:H1:7,F",
                                     "Edge7;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E",
                                     "Edge7;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U2;CHF;:H1:8,V",
                                     "Edge7;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U;CHF;:H1:7,V",
                                     "Edge7;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E",
                                     "Edge7;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E;:U;CHF;:H1:7,V",
                                     "Edge7;:G;CHF;:H1:7,F;:U4;CHF;:H1:8,E",
                                     "Edge7;:G;CHF;:H1:7,F;:U;CHF;:H1:7,E",
                                     "Edge8;:G;CHF;:H1:7,F",
                                     "Edge8;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E",
                                     "Edge8;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U2;CHF;:H1:8,V",
                                     "Edge8;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E;:U;CHF;:H1:7,V",
                                     "Edge8;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E",
                                     "Edge8;:G;CHF;:H1:7,F;:U4;CHF;:H1:8,E",
                                     "Edge8;:G;CHF;:H1:7,F;:U;CHF;:H1:7,E",
                                     "Edge9;:G;CHF;:H1:7,F",
                                     "Edge9;:G;CHF;:H1:7,F;:U2;CHF;:H1:8,E",
                                     "Edge9;:G;CHF;:H1:7,F;:U3;CHF;:H1:8,E",
                                     "Edge9;:G;CHF;:H1:7,F;:U4;CHF;:H1:8,E",
                                     "Edge9;:G;CHF;:H1:7,F;:U;CHF;:H1:7,E",
                                     "Face1;:M;CHF;:H1:7,F",
                                     "Face2;:M;CHF;:H1:7,F",
                                     "Face3;:M;CHF;:H1:7,F",
                                     "Face4;:M;CHF;:H1:7,F",
                                     "Face5;:M;CHF;:H1:7,F",
                                     "Face6;:M;CHF;:H1:7,F",
                                     "Vertex1;:G;CHF;:H1:7,F",
                                     "Vertex2;:G;CHF;:H1:7,F",
                                     "Vertex3;:G;CHF;:H1:7,F",
                                     "Vertex4;:G;CHF;:H1:7,F",
                                     "Vertex5;:G;CHF;:H1:7,F",
                                     "Vertex6;:G;CHF;:H1:7,F",
                                     "Vertex7;:G;CHF;:H1:7,F",
                                     "Vertex8;:G;CHF;:H1:7,F",
                                 }));
}

TEST_F(TopoShapeExpansionTest, makeElementFillet)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape cube1TS {cube1, 1L};
    auto edges = cube1TS.getSubTopoShapes(TopAbs_EDGE);
    // Act
    cube1TS.makeElementFillet({cube1TS}, edges, .05, .05);
    auto elements = elementMap(cube1TS);
    // Assert shape is correct
    EXPECT_EQ(cube1TS.countSubElements("Wire"), 26);
    EXPECT_FLOAT_EQ(getArea(cube1TS.getShape()), 5.739646);
    // Assert that we're creating a correct element map
    EXPECT_TRUE(cube1TS.getMappedChildElements().empty());
    EXPECT_TRUE(elementsMatch(cube1TS,
                              {
                                  "Edge10;:G;FLT;:H1:7,F",
                                  "Edge10;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Edge10;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E",
                                  "Edge10;:G;FLT;:H1:7,F;:U4;FLT;:H1:8,E",
                                  "Edge10;:G;FLT;:H1:7,F;:U;FLT;:H1:7,E",
                                  "Edge11;:G;FLT;:H1:7,F",
                                  "Edge11;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Edge11;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E",
                                  "Edge11;:G;FLT;:H1:7,F;:U4;FLT;:H1:8,E",
                                  "Edge11;:G;FLT;:H1:7,F;:U;FLT;:H1:7,E",
                                  "Edge12;:G;FLT;:H1:7,F",
                                  "Edge12;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Edge12;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E",
                                  "Edge12;:G;FLT;:H1:7,F;:U4;FLT;:H1:8,E",
                                  "Edge12;:G;FLT;:H1:7,F;:U;FLT;:H1:7,E",
                                  "Edge1;:G;FLT;:H1:7,F",
                                  "Edge1;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Edge1;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U2;FLT;:H1:8,V",
                                  "Edge1;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U;FLT;:H1:7,V",
                                  "Edge1;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E",
                                  "Edge1;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E;:U2;FLT;:H1:8,V",
                                  "Edge1;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E;:U;FLT;:H1:7,V",
                                  "Edge1;:G;FLT;:H1:7,F;:U4;FLT;:H1:8,E",
                                  "Edge1;:G;FLT;:H1:7,F;:U;FLT;:H1:7,E",
                                  "Edge2;:G;FLT;:H1:7,F",
                                  "Edge2;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Edge2;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U2;FLT;:H1:8,V",
                                  "Edge2;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U;FLT;:H1:7,V",
                                  "Edge2;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E",
                                  "Edge2;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E;:U2;FLT;:H1:8,V",
                                  "Edge2;:G;FLT;:H1:7,F;:U4;FLT;:H1:8,E",
                                  "Edge2;:G;FLT;:H1:7,F;:U;FLT;:H1:7,E",
                                  "Edge3;:G;FLT;:H1:7,F",
                                  "Edge3;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Edge3;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U2;FLT;:H1:8,V",
                                  "Edge3;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U;FLT;:H1:7,V",
                                  "Edge3;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E",
                                  "Edge3;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E;:U;FLT;:H1:7,V",
                                  "Edge3;:G;FLT;:H1:7,F;:U4;FLT;:H1:8,E",
                                  "Edge3;:G;FLT;:H1:7,F;:U;FLT;:H1:7,E",
                                  "Edge4;:G;FLT;:H1:7,F",
                                  "Edge4;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Edge4;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U2;FLT;:H1:8,V",
                                  "Edge4;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U;FLT;:H1:7,V",
                                  "Edge4;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E",
                                  "Edge4;:G;FLT;:H1:7,F;:U4;FLT;:H1:8,E",
                                  "Edge4;:G;FLT;:H1:7,F;:U;FLT;:H1:7,E",
                                  "Edge5;:G;FLT;:H1:7,F",
                                  "Edge5;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Edge5;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U2;FLT;:H1:8,V",
                                  "Edge5;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U;FLT;:H1:7,V",
                                  "Edge5;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E",
                                  "Edge5;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E;:U2;FLT;:H1:8,V",
                                  "Edge5;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E;:U;FLT;:H1:7,V",
                                  "Edge5;:G;FLT;:H1:7,F;:U4;FLT;:H1:8,E",
                                  "Edge5;:G;FLT;:H1:7,F;:U;FLT;:H1:7,E",
                                  "Edge6;:G;FLT;:H1:7,F",
                                  "Edge6;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Edge6;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U2;FLT;:H1:8,V",
                                  "Edge6;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U;FLT;:H1:7,V",
                                  "Edge6;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E",
                                  "Edge6;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E;:U2;FLT;:H1:8,V",
                                  "Edge6;:G;FLT;:H1:7,F;:U4;FLT;:H1:8,E",
                                  "Edge6;:G;FLT;:H1:7,F;:U;FLT;:H1:7,E",
                                  "Edge7;:G;FLT;:H1:7,F",
                                  "Edge7;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Edge7;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U2;FLT;:H1:8,V",
                                  "Edge7;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U;FLT;:H1:7,V",
                                  "Edge7;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E",
                                  "Edge7;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E;:U;FLT;:H1:7,V",
                                  "Edge7;:G;FLT;:H1:7,F;:U4;FLT;:H1:8,E",
                                  "Edge7;:G;FLT;:H1:7,F;:U;FLT;:H1:7,E",
                                  "Edge8;:G;FLT;:H1:7,F",
                                  "Edge8;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Edge8;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U2;FLT;:H1:8,V",
                                  "Edge8;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E;:U;FLT;:H1:7,V",
                                  "Edge8;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E",
                                  "Edge8;:G;FLT;:H1:7,F;:U4;FLT;:H1:8,E",
                                  "Edge8;:G;FLT;:H1:7,F;:U;FLT;:H1:7,E",
                                  "Edge9;:G;FLT;:H1:7,F",
                                  "Edge9;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Edge9;:G;FLT;:H1:7,F;:U3;FLT;:H1:8,E",
                                  "Edge9;:G;FLT;:H1:7,F;:U4;FLT;:H1:8,E",
                                  "Edge9;:G;FLT;:H1:7,F;:U;FLT;:H1:7,E",
                                  "Face1;:M;FLT;:H1:7,F",
                                  "Face2;:M;FLT;:H1:7,F",
                                  "Face3;:M;FLT;:H1:7,F",
                                  "Face4;:M;FLT;:H1:7,F",
                                  "Face5;:M;FLT;:H1:7,F",
                                  "Face6;:M;FLT;:H1:7,F",
                                  "Vertex1;:G;FLT;:H1:7,F",
                                  "Vertex1;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Vertex2;:G;FLT;:H1:7,F",
                                  "Vertex2;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Vertex3;:G;FLT;:H1:7,F",
                                  "Vertex3;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Vertex4;:G;FLT;:H1:7,F",
                                  "Vertex4;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Vertex5;:G;FLT;:H1:7,F",
                                  "Vertex5;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Vertex6;:G;FLT;:H1:7,F",
                                  "Vertex6;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Vertex7;:G;FLT;:H1:7,F",
                                  "Vertex7;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                                  "Vertex8;:G;FLT;:H1:7,F",
                                  "Vertex8;:G;FLT;:H1:7,F;:U2;FLT;:H1:8,E",
                              }));
}

TEST_F(TopoShapeExpansionTest, makeElementSlice)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();  // TopoShape version works too
    TopoShape cube1TS {cube1};               // Adding a tag here only adds text in each mapped name
    auto faces = cube1TS.getSubShapes(TopAbs_FACE);
    TopoShape slicer {faces[0]};
    Base::Vector3d direction {1.0, 0.0, 0.0};
    // Act
    auto& result = slicer.makeElementSlice(cube1TS, direction, 0.5);
    // Assert shape is correct
    EXPECT_FLOAT_EQ(getLength(result.getShape()), 4);
    EXPECT_EQ(TopAbs_ShapeEnum::TopAbs_WIRE, result.getShape().ShapeType());
    // Assert that we're creating a correct element map
    EXPECT_TRUE(result.getMappedChildElements().empty());
    EXPECT_TRUE(elementsMatch(
        result,
        {
            "Face1;SLC;:H1:4,F;:G2;SLC;:H1:8,V;SLC;:H1:4,V;MAK;:H1:4,V",
            "Face1;SLC;:H1:4,F;:G3;SLC;:H1:8,V;SLC;:H1:4,V;MAK;:H1:4,V",
            // MacOSX difference:
            // "Face1;SLC;:H1:4,F;:G4;SLC;:H1:8,V;D25fd;:H1:6,V;SLC;:H1:4,V;MAK;:H1:4,V",
            // "Face1;SLC;:H1:4,F;:G4;SLC;:H1:8,V;D1;:H1:3,V;SLC;:H1:4,V;MAK;:H1:4,V",
            "Face1;SLC;:H1:4,F;:G4;SLC;:H1:8,V;SLC;:H1:4,V;MAK;:H1:4,V",
            "Face1;SLC;:H1:4,F;:G5;SLC;:H1:8,E;SLC;:H1:4,E;MAK;:H1:4,E",
            "Face1;SLC;:H1:4,F;:G6;SLC;:H1:8,E;SLC;:H1:4,E;MAK;:H1:4,E",
            "Face1;SLC;:H1:4,F;:G7;SLC;:H1:8,E;SLC;:H1:4,E;MAK;:H1:4,E",
            "Face1;SLC;:H1:4,F;:G8;SLC;:H1:8,E;SLC;:H1:4,E;MAK;:H1:4,E",
        }));  // Changed with PR#12471. Probably will change again after
              //  importing other TopoNaming logics
}

TEST_F(TopoShapeExpansionTest, makeElementSlices)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape cube1TS {cube1, 1L};
    auto faces = cube1TS.getSubShapes(TopAbs_FACE);
    TopoShape slicer {faces[0]};
    Base::Vector3d direction {1.0, 0.0, 0.0};
    // Act
    auto& result = slicer.makeElementSlices(cube1TS, direction, {0.25, 0.5, 0.75});
    auto subTopoShapes = result.getSubTopoShapes(TopAbs_WIRE);
    // Assert shape is correct
    EXPECT_EQ(result.countSubElements("Wire"), 3);
    EXPECT_FLOAT_EQ(getLength(result.getShape()), 12);
    EXPECT_FLOAT_EQ(getLength(subTopoShapes[0].getShape()), 4);
    EXPECT_EQ(TopAbs_ShapeEnum::TopAbs_COMPOUND, result.getShape().ShapeType());
    EXPECT_EQ(TopAbs_ShapeEnum::TopAbs_WIRE, subTopoShapes[0].getShape().ShapeType());
    EXPECT_EQ(TopAbs_ShapeEnum::TopAbs_WIRE, subTopoShapes[1].getShape().ShapeType());
    EXPECT_EQ(TopAbs_ShapeEnum::TopAbs_WIRE, subTopoShapes[2].getShape().ShapeType());
    // Assert that we're creating a correct element map
    EXPECT_TRUE(result.getMappedChildElements().empty());
    EXPECT_TRUE(elementsMatch(
        result,
        {
            "Edge10;:G(Face1;SLC;:H1:4,F;K-2;:H1:4,F);SLC;:H1:26,V;SLC;:H1:4,V;MAK;:H1:4,V",
            "Edge10;:G(Face1;SLC_2;:H2:6,F;K-2;:H2:4,F);SLC_2;:H1:2a,V;SLC_2;:H1:6,V;MAK;:H1:4,V",
            "Edge10;:G(Face1;SLC_3;:H3:6,F;K-2;:H3:4,F);SLC_3;:H1:2a,V;SLC_3;:H1:6,V;MAK;:H1:4,V",
            "Edge11;:G(Face1;SLC;:H1:4,F;K-3;:H1:4,F);SLC;:H1:26,V;SLC;:H1:4,V;MAK;:H1:4,V",
            "Edge11;:G(Face1;SLC_2;:H2:6,F;K-3;:H2:4,F);SLC_2;:H1:2a,V;SLC_2;:H1:6,V;MAK;:H1:4,V",
            "Edge11;:G(Face1;SLC_3;:H3:6,F;K-3;:H3:4,F);SLC_3;:H1:2a,V;SLC_3;:H1:6,V;MAK;:H1:4,V",
            // TODO: Prove that this difference is not a problem.
            //  The next elements vary according to platform / OCCT version and thus can't be
            //  absolutely tested.
            //            "Edge12;:G(Face1;SLC;:H1:4,F;K-4;:H1:4,F);SLC;:H1:26,V;D1;:H1:3,V;SLC;:H1:4,V;MAK;:H1:"
            //            "4,V",
            //            "Edge12;:G(Face1;SLC;:H1:4,F;K-4;:H1:4,F);SLC;:H1:26,V;SLC;:H1:4,V;MAK;:H1:4,V",
            //            "Edge12;:G(Face1;SLC_2;:H2:6,F;K-4;:H2:4,F);SLC_2;:H1:2a,V;D1;:H1:3,V;SLC_2;:H1:6,V;"
            //            "MAK;:H1:4,V",
            //            "Edge12;:G(Face1;SLC_2;:H2:6,F;K-4;:H2:4,F);SLC_2;:H1:2a,V;SLC_2;:H1:6,V;MAK;:H1:4,V",
            //            "Edge12;:G(Face1;SLC_3;:H3:6,F;K-4;:H3:4,F);SLC_3;:H1:2a,V;D1;:H1:3,V;SLC_3;:H1:6,V;"
            //            "MAK;:H1:4,V",
            //            "Edge12;:G(Face1;SLC_3;:H3:6,F;K-4;:H3:4,F);SLC_3;:H1:2a,V;SLC_3;:H1:6,V;MAK;:H1:4,V",
            "Face1;SLC;:H1:4,F;:G5(Face3;K-1;:H1:4,F);SLC;:H1:1b,E;SLC;:H1:4,E;MAK;:H1:4,E",
            "Face1;SLC;:H1:4,F;:G6(Face4;K-1;:H1:4,F);SLC;:H1:1b,E;SLC;:H1:4,E;MAK;:H1:4,E",
            "Face1;SLC;:H1:4,F;:G7(Face5;K-1;:H1:4,F);SLC;:H1:1b,E;SLC;:H1:4,E;MAK;:H1:4,E",
            "Face1;SLC;:H1:4,F;:G8(Face6;K-1;:H1:4,F);SLC;:H1:1b,E;SLC;:H1:4,E;MAK;:H1:4,E",
            "Face3;:G(Face1;SLC_2;:H2:6,F;K-5;:H2:4,F);SLC_2;:H1:2a,E;SLC_2;:H1:6,E;MAK;:H1:4,E",
            "Face3;:G(Face1;SLC_3;:H3:6,F;K-5;:H3:4,F);SLC_3;:H1:2a,E;SLC_3;:H1:6,E;MAK;:H1:4,E",
            "Face4;:G(Face1;SLC_2;:H2:6,F;K-6;:H2:4,F);SLC_2;:H1:2a,E;SLC_2;:H1:6,E;MAK;:H1:4,E",
            "Face4;:G(Face1;SLC_3;:H3:6,F;K-6;:H3:4,F);SLC_3;:H1:2a,E;SLC_3;:H1:6,E;MAK;:H1:4,E",
            "Face5;:G(Face1;SLC_2;:H2:6,F;K-7;:H2:4,F);SLC_2;:H1:2a,E;SLC_2;:H1:6,E;MAK;:H1:4,E",
            "Face5;:G(Face1;SLC_3;:H3:6,F;K-7;:H3:4,F);SLC_3;:H1:2a,E;SLC_3;:H1:6,E;MAK;:H1:4,E",
            "Face6;:G(Face1;SLC_2;:H2:6,F;K-8;:H2:4,F);SLC_2;:H1:2a,E;SLC_2;:H1:6,E;MAK;:H1:4,E",
            "Face6;:G(Face1;SLC_3;:H3:6,F;K-8;:H3:4,F);SLC_3;:H1:2a,E;SLC_3;:H1:6,E;MAK;:H1:4,E",
        }));
    EXPECT_FALSE(
        subTopoShapes[0].getElementMap().empty());  // Changed with PR#12471. Probably will change
                                                    // again after importing other TopoNaming logics
}

TEST_F(TopoShapeExpansionTest, makeElementMirror)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape cube1TS {cube1, 1L};
    auto edges = cube1TS.getSubTopoShapes(TopAbs_EDGE);
    gp_Ax2 axis {gp_Pnt {0, 0, 0}, gp_Dir {1, 0, 0}};
    // Act
    auto& result = cube1TS.makeElementMirror(cube1TS, axis);
    auto elements = elementMap(cube1TS);
    Base::BoundBox3d bb = result.getBoundBox();
    // Assert shape is correct
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(-1, 0, 0, 0, 1, 1)));
    EXPECT_EQ(result.countSubElements("Wire"), 6);
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 1);
    EXPECT_EQ(TopAbs_ShapeEnum::TopAbs_SOLID, result.getShape().ShapeType());
    // Assert that we're creating a correct element map
    EXPECT_TRUE(result.getMappedChildElements().empty());
    EXPECT_TRUE(
        elementsMatch(result,
                      {"Edge10;:M;MIR;:H1:7,E",  "Edge11;:M;MIR;:H1:7,E",  "Edge12;:M;MIR;:H1:7,E",
                       "Edge1;:M;MIR;:H1:7,E",   "Edge2;:M;MIR;:H1:7,E",   "Edge3;:M;MIR;:H1:7,E",
                       "Edge4;:M;MIR;:H1:7,E",   "Edge5;:M;MIR;:H1:7,E",   "Edge6;:M;MIR;:H1:7,E",
                       "Edge7;:M;MIR;:H1:7,E",   "Edge8;:M;MIR;:H1:7,E",   "Edge9;:M;MIR;:H1:7,E",
                       "Face1;:M;MIR;:H1:7,F",   "Face2;:M;MIR;:H1:7,F",   "Face3;:M;MIR;:H1:7,F",
                       "Face4;:M;MIR;:H1:7,F",   "Face5;:M;MIR;:H1:7,F",   "Face6;:M;MIR;:H1:7,F",
                       "Vertex1;:M;MIR;:H1:7,V", "Vertex2;:M;MIR;:H1:7,V", "Vertex3;:M;MIR;:H1:7,V",
                       "Vertex4;:M;MIR;:H1:7,V", "Vertex5;:M;MIR;:H1:7,V", "Vertex6;:M;MIR;:H1:7,V",
                       "Vertex7;:M;MIR;:H1:7,V", "Vertex8;:M;MIR;:H1:7,V"}));
}

TEST_F(TopoShapeExpansionTest, makeElementTransformWithoutMap)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    auto tr {gp_Trsf()};
    tr.SetTranslation(gp_Vec(gp_XYZ(-0.5, -0.5, 0)));
    TopoShape topoShape1 {cube1, 1L};
    // Act
    TopoShape& result = topoShape1.makeElementTransform(topoShape1, tr);
    auto elements = elementMap(result);
    Base::BoundBox3d bb = result.getBoundBox();
    // Assert shape is correct
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(-0.5, -0.5, 0.0, 0.5, 0.5, 1.0)));
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 1);
    // Assert elementMap is correct
    EXPECT_EQ(elements.size(), 0);
}

TEST_F(TopoShapeExpansionTest, makeElementTransformWithMap)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    auto tr {gp_Trsf()};
    tr.SetTranslation(gp_Vec(gp_XYZ(-0.5, -0.5, 0)));
    cube2.Move(TopLoc_Location(tr));
    TopoShape topoShape1 {cube1, 1L};
    TopoShape topoShape2 {cube2, 2L};
    // Act
    TopoShape& result = topoShape1.makeElementFuse({topoShape1, topoShape2});  // op, tolerance
    topoShape1.makeElementTransform(result, tr);
    auto elements = elementMap(result);
    Base::BoundBox3d bb = result.getBoundBox();
    // Assert shape is correct
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(-0.5, -1.0, 0.0, 1.0, 0.5, 1.0)));
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 1.75);
    // Assert elementMap is correct
    EXPECT_EQ(elements.size(), 66);
    EXPECT_EQ(elements.count(IndexedName("Face", 1)), 1);
    EXPECT_EQ(
        elements[IndexedName("Face", 1)],
        MappedName(
            "Face3;:M;FUS;:H1:7,F;:U;FUS;:H1:7,E;:L(Face5;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E|Face5;:M;"
            "FUS;:H1:7,F;:U2;FUS;:H1:8,E;:U;FUS;:H1:7,V;:L(Face6;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E;:U;"
            "FUS;:H1:7,V);FUS;:H1:3c,E|Face6;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E);FUS;:H1:cb,F"));
}

TEST_F(TopoShapeExpansionTest, makeElementGTransformWithoutMap)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    auto tr {gp_Trsf()};
    tr.SetTranslation(gp_Vec(gp_XYZ(-0.5, -0.5, 0)));
    TopoShape topoShape1 {cube1, 1L};
    // Act
    TopoShape& result = topoShape1.makeElementGTransform(topoShape1, TopoShape::convert(tr));
    auto elements = elementMap(result);
    Base::BoundBox3d bb = result.getBoundBox();
    // Assert shape is correct
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(-0.5, -0.5, 0.0, 0.5, 0.5, 1.0)));
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 1);
    // Assert elementMap is correct
    EXPECT_EQ(elements.size(), 0);
}

TEST_F(TopoShapeExpansionTest, makeElementGTransformWithMap)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    auto tr {gp_Trsf()};
    tr.SetTranslation(gp_Vec(gp_XYZ(-0.5, -0.5, 0)));
    cube2.Move(TopLoc_Location(tr));
    TopoShape topoShape1 {cube1, 1L};
    TopoShape topoShape2 {cube2, 2L};
    // Act
    TopoShape& result = topoShape1.makeElementFuse({topoShape1, topoShape2});  // op, tolerance
    topoShape1.makeElementGTransform(result, TopoShape::convert(tr));
    auto elements = elementMap(result);
    Base::BoundBox3d bb = result.getBoundBox();
    // Assert shape is correct
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(-0.5, -1.0, 0.0, 1.0, 0.5, 1.0)));
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 1.75);
    // Assert elementMap is correct
    EXPECT_EQ(elements.size(), 66);
    EXPECT_EQ(elements.count(IndexedName("Face", 1)), 1);
    EXPECT_EQ(
        elements[IndexedName("Face", 1)],
        MappedName(
            "Face3;:M;FUS;:H1:7,F;:U;FUS;:H1:7,E;:L(Face5;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E|Face5;:M;"
            "FUS;:H1:7,F;:U2;FUS;:H1:8,E;:U;FUS;:H1:7,V;:L(Face6;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E;:U;"
            "FUS;:H1:7,V);FUS;:H1:3c,E|Face6;:M;FUS;:H1:7,F;:U2;FUS;:H1:8,E);FUS;:H1:cb,F"));
}

// Not testing _makeElementTransform as it is a thin wrapper that calls the same places as the four
// preceding tests.

TEST_F(TopoShapeExpansionTest, makeElementSolid)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    auto tr {gp_Trsf()};
    tr.SetTranslation(gp_Vec(gp_XYZ(-0.5, -0.5, 0)));
    cube2.Move(TopLoc_Location(tr));
    TopoShape topoShape1 {cube1, 1L};
    TopoShape topoShape2 {cube2, 2L};
    // Act
    TopExp_Explorer exp(topoShape1.getShape(), TopAbs_SHELL);
    auto shell1 = exp.Current();
    exp.Init(topoShape2.getShape(), TopAbs_SHELL);
    auto shell2 = exp.Current();
    TopoShape& topoShape3 = topoShape1.makeElementCompound({shell1, shell2});
    TopoShape& result = topoShape1.makeElementSolid(topoShape3);  // Need the single parm form
    auto elements = elementMap(result);
    Base::BoundBox3d bb = result.getBoundBox();
    // Assert shape is correct
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0.0, -0.5, 0.0, 1.5, 1.0, 1.0)));
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 2);
    // Assert elementMap is correct
    EXPECT_EQ(elements.size(), 52);
    EXPECT_EQ(elements.count(IndexedName("Face", 1)), 1);
    EXPECT_EQ(
        elements[IndexedName("Face", 1)],
        MappedName("Face1;:H,F;SLD;:H1:4,F"));  // Changed with PR#12471. Probably will change again
                                                // after importing other TopoNaming logics
}

TEST_F(TopoShapeExpansionTest, makeElementRevolve)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape topoShape1 {cube1, 1L};
    gp_Ax1 axis {gp_Pnt {0, 0, 0}, gp_Dir {0, 1, 0}};
    double angle = 45;
    auto subTopoFaces = topoShape1.getSubTopoShapes(TopAbs_FACE);
    subTopoFaces[0].Tag = 2L;
    // Act
    TopoShape result = subTopoFaces[0].makeElementRevolve(axis, angle);
    auto elements = elementMap(result);
    Base::BoundBox3d bb = result.getBoundBox();
    // Assert shape is correct
    EXPECT_TRUE(PartTestHelpers::boxesMatch(
        bb,
        Base::BoundBox3d(0.0, 0.0, 0.0, 0.85090352453411933, 1.0, 1.0)));
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 0.50885141);
    // Assert elementMap is correct
    EXPECT_TRUE(
        elementsMatch(result,
                      {
                          "Edge1;:G;RVL;:H2:7,F",
                          "Edge1;:G;RVL;:H2:7,F;:U;RVL;:H2:7,E",
                          "Edge1;:G;RVL;:H2:7,F;:U;RVL;:H2:7,E;:L(Edge2;:G;RVL;:H2:7,F;:U;RVL;:H2:"
                          "7,E|Edge3;:G;RVL;:H2:7,F;:U;RVL;:H2:7,E|Edge4;RVL;:H2:4,E);RVL;:H2:62,F",
                          "Edge1;:G;RVL;:H2:7,F;:U;RVL;:H2:7,E;:U;RVL;:H2:7,V",
                          "Edge1;RVL;:H2:4,E",
                          "Edge2;:G;RVL;:H2:7,F",
                          "Edge2;:G;RVL;:H2:7,F;:U;RVL;:H2:7,E",
                          "Edge2;:G;RVL;:H2:7,F;:U;RVL;:H2:7,E;:U;RVL;:H2:7,V",
                          "Edge2;RVL;:H2:4,E",
                          "Edge3;:G;RVL;:H2:7,F",
                          "Edge3;:G;RVL;:H2:7,F;:U;RVL;:H2:7,E",
                          "Edge3;RVL;:H2:4,E",
                          "Edge4;RVL;:H2:4,E",
                          "Face1;RVL;:H2:4,F",
                          "Vertex1;:G;RVL;:H2:7,E",
                          "Vertex1;RVL;:H2:4,V",
                          "Vertex2;RVL;:H2:4,V",
                          "Vertex3;:G;RVL;:H2:7,E",
                          "Vertex3;RVL;:H2:4,V",
                          "Vertex4;RVL;:H2:4,V",
                      }));
}

TEST_F(TopoShapeExpansionTest, makeElementPrism)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape topoShape1 {cube1, 1L};
    auto subTopoFaces = topoShape1.getSubTopoShapes(TopAbs_FACE);
    subTopoFaces[0].Tag = 2L;
    // Act
    TopoShape& result = topoShape1.makeElementPrism(subTopoFaces[0], {0.75, 0, 0});
    auto elements = elementMap(result);
    Base::BoundBox3d bb = result.getBoundBox();
    // Assert shape is correct
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0.0, 0.0, 0.0, 0.75, 1.0, 1.0)));
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 0.75);
    // Assert elementMap is correct
    EXPECT_TRUE(elementsMatch(
        result,
        {
            "Edge1;:G;XTR;:H2:7,F",
            "Edge1;:G;XTR;:H2:7,F;:U;XTR;:H2:7,E",
            "Edge1;:G;XTR;:H2:7,F;:U;XTR;:H2:7,E;:L(Edge2;:G;XTR;:H2:7,F;:U;XTR;:H2:7,E|Edge3;:G;"
            "XTR;:H2:7,F;:U;XTR;:H2:7,E|Edge4;:G;XTR;:H2:7,F;:U;XTR;:H2:7,E);XTR;:H2:74,F",
            "Edge1;:G;XTR;:H2:7,F;:U;XTR;:H2:7,E;:U2;XTR;:H2:8,V",
            "Edge1;:G;XTR;:H2:7,F;:U;XTR;:H2:7,E;:U;XTR;:H2:7,V",
            "Edge1;XTR;:H2:4,E",
            "Edge2;:G;XTR;:H2:7,F",
            "Edge2;:G;XTR;:H2:7,F;:U;XTR;:H2:7,E",
            "Edge2;:G;XTR;:H2:7,F;:U;XTR;:H2:7,E;:U;XTR;:H2:7,V",
            "Edge2;XTR;:H2:4,E",
            "Edge3;:G;XTR;:H2:7,F",
            "Edge3;:G;XTR;:H2:7,F;:U;XTR;:H2:7,E",
            "Edge3;:G;XTR;:H2:7,F;:U;XTR;:H2:7,E;:U2;XTR;:H2:8,V",
            "Edge3;XTR;:H2:4,E",
            "Edge4;:G;XTR;:H2:7,F",
            "Edge4;:G;XTR;:H2:7,F;:U;XTR;:H2:7,E",
            "Edge4;XTR;:H2:4,E",
            "Face1;XTR;:H2:4,F",
            "Vertex1;:G;XTR;:H2:7,E",
            "Vertex1;XTR;:H2:4,V",
            "Vertex2;:G;XTR;:H2:7,E",
            "Vertex2;XTR;:H2:4,V",
            "Vertex3;:G;XTR;:H2:7,E",
            "Vertex3;XTR;:H2:4,V",
            "Vertex4;:G;XTR;:H2:7,E",
            "Vertex4;XTR;:H2:4,V",
        })

    );
}

// TODO:  This code was written in Feb 2024 as part of the toponaming project, but appears to be
// unused.  It is potentially useful if debugged.
//
// TEST_F(TopoShapeExpansionTest, makeElementPrismUntil)
//{
//    // Arrange
//    auto [cube1, cube2] = CreateTwoCubes();
//    TopoShape cube1TS {cube1, 1L};
//    auto subFaces = cube1TS.getSubShapes(TopAbs_FACE);
//    auto subTopoFaces = cube1TS.getSubTopoShapes(TopAbs_FACE);
//    subTopoFaces[0].Tag = 2L;
//    subTopoFaces[1].Tag = 3L;
//    auto tr {gp_Trsf()};
//    auto direction = gp_Vec(gp_XYZ(0.0, 0.0, 0.25));
//    tr.SetTranslation(direction);
//    auto support = subFaces[0].Moved(TopLoc_Location(tr));
//    auto upto = support.Moved(TopLoc_Location(tr));
//    // Act
//    TopoShape result = cube1TS.makeElementPrismUntil(subTopoFaces[0],
//                                                     TopoShape(support, 4L),
//                                                     TopoShape(upto, 5L),
//                                                     direction,
//                                                     TopoShape::PrismMode::CutFromBase);
//    auto elements = elementMap(result);
//    Base::BoundBox3d bb = result.getBoundBox();
//    // Assert shape is correct
//    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0.0, -0.5, 0.0, 1.5, 1.0, 1.0)));
//    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 2);
//    // Assert elementMap is correct
//    EXPECT_TRUE(elementsMatch(result,
//                              {"Edge1;:G;XTR;:H2:7,F",}));
//}

TEST_F(TopoShapeExpansionTest, makeElementFilledFace)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    TopoShape topoShape1 {cube1, 1L};
    auto wires = topoShape1.getSubShapes(TopAbs_WIRE);
    TopoShape topoShape2 {wires[0], 2L};
    // Act
    auto params = TopoShape::BRepFillingParams();
    TopoShape& result = topoShape1.makeElementFilledFace({topoShape2}, params);
    auto elements = elementMap(result);
    Base::BoundBox3d bb = result.getBoundBox();
    // Assert shape is correct
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0.0, -0.6, -0.6, 0, 1.6, 1.6)));
    EXPECT_FLOAT_EQ(getArea(result.getShape()), 1);
    // Assert elementMap is correct
    EXPECT_TRUE(elementsMatch(result,
                              {
                                  "Edge1;:G;FFC;:H2:7,E",
                                  "Edge1;:G;FFC;:H2:7,E;:L(Edge2;:G;FFC;:H2:7,E|Edge3;:G;FFC;:"
                                  "H2:7,E|Edge4;:G;FFC;:H2:7,E);FFC;:H2:47,F",
                                  "Edge2;:G;FFC;:H2:7,E",
                                  "Edge3;:G;FFC;:H2:7,E",
                                  "Edge4;:G;FFC;:H2:7,E",
                                  // TODO: Prove that this difference is not a problem.
                                  //  The next elements vary according to platform / OCCT version
                                  //  and thus can't be absolutely tested.
                                  //                                     "Vertex1;:G;FFC;:H2:7,V",
                                  //                                     "Vertex2;:G;FFC;:H2:7,V",
                                  //                                     "Vertex3;:G;FFC;:H2:7,V",
                                  //                                     "Vertex4;:G;FFC;:H2:7,V",
                              }));
}

TEST_F(TopoShapeExpansionTest, makeElementBSplineFace)
{
    // Arrange
    TColgp_Array1OfPnt array1(1, 3);  // sizing array
    array1.SetValue(1, gp_Pnt(-4, 0, 2));
    array1.SetValue(2, gp_Pnt(-7, 2, 2));
    array1.SetValue(3, gp_Pnt(-10, 0, 2));
    Handle(Geom_BSplineCurve) curve1 = GeomAPI_PointsToBSpline(array1).Curve();

    TColgp_Array1OfPnt array2(1, 3);  // sizing array
    array2.SetValue(1, gp_Pnt(-4, 0, 2));
    array2.SetValue(2, gp_Pnt(-7, -2, 2));
    array2.SetValue(3, gp_Pnt(-9, 0, 2));
    Handle(Geom_BSplineCurve) curve2 = GeomAPI_PointsToBSpline(array2).Curve();

    auto edge = BRepBuilderAPI_MakeEdge(curve1);
    auto edge1 = BRepBuilderAPI_MakeEdge(curve2);
    TopoShape topoShape {1L};
    TopoShape topoShape2 {edge, 2L};
    TopoShape topoShape3 {edge1, 3L};
    // Act
    TopoShape& result = topoShape.makeElementBSplineFace({topoShape2, topoShape3});
    auto elements = elementMap(result);
    Base::BoundBox3d bb = result.getBoundBox();
    // Assert shape is correct
    EXPECT_TRUE(PartTestHelpers::boxesMatch(
        bb,
        Base::BoundBox3d(-10, -2.0597998470594132, 2, -4, 2.1254369627132599, 2)));
    EXPECT_FLOAT_EQ(getArea(result.getShape()), 14.677052);
    // Assert elementMap is correct
    EXPECT_TRUE(elementsMatch(result,
                              {
                                  "Edge1",
                                  "Edge1;BSF",
                                  "Edge1;D1",
                                  "Edge1;D2",
                                  "Edge1;D3",
                                  "Vertex1",
                                  "Vertex1;D1",
                                  "Vertex2",
                                  "Vertex2;D1",
                              }));
}

TEST_F(TopoShapeExpansionTest, replaceElementShape)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoTopoShapeCubes();
    // We can't use a compound in replaceElementShape, so we'll make a replacement wire and a shell
    auto wire {BRepBuilderAPI_MakeWire(
                   BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)),
                   BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(1.0, 1.0, 0.0)),
                   BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 1.0, 0.0), gp_Pnt(0.0, 0.0, 0.0)))
                   .Wire()};
    auto shell = cube1.makeElementShell();
    auto wires = shell.getSubTopoShapes(TopAbs_WIRE);
    // Act
    TopoShape& result = shell.replaceElementShape(shell, {{wires[0], wire}});
    Base::BoundBox3d bb = result.getBoundBox();
    // Assert shape is correct
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0.0, 0.0, 0.0, 1.0, 1.0, 1.0)));
    EXPECT_FLOAT_EQ(getArea(result.getShape()), 5);
    EXPECT_EQ(result.countSubElements("Wire"), 6);
    // Assert that we're creating a correct element map
    EXPECT_TRUE(result.getMappedChildElements().empty());
    EXPECT_TRUE(elementsMatch(
        result,
        {
            "Edge1;:H1,E",   "Edge1;:H2,E",   "Edge1;:H3,E",   "Edge2;:H1,E",   "Edge2;:H2,E",
            "Edge2;:H3,E",   "Edge3;:H1,E",   "Edge3;:H2,E",   "Edge3;:H3,E",   "Edge4;:H1,E",
            "Edge4;:H2,E",   "Edge4;:H3,E",   "Face1;:H2,F",   "Face1;:H3,F",   "Face1;:H4,F",
            "Face1;:H5,F",   "Face1;:H6,F",   "Vertex1;:H1,V", "Vertex1;:H2,V", "Vertex2;:H1,V",
            "Vertex2;:H2,V", "Vertex3;:H1,V", "Vertex3;:H2,V", "Vertex4;:H1,V", "Vertex4;:H2,V",
        }));
}

TEST_F(TopoShapeExpansionTest, removeElementShape)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoTopoShapeCubes();
    auto faces = cube1.getSubTopoShapes(TopAbs_FACE);
    // Act
    TopoShape result = cube1.removeElementShape({faces[0]});
    Base::BoundBox3d bb = result.getBoundBox();
    // Assert shape is correct
    EXPECT_TRUE(PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(0.0, 0.0, 0.0, 1.0, 1.0, 1.0)));
    EXPECT_FLOAT_EQ(getArea(result.getShape()), 5);
    EXPECT_EQ(result.countSubShapes("Compound"), 1);
    EXPECT_EQ(result.countSubShapes("Face"), 5);
    // Assert that we're creating a correct element map
    EXPECT_TRUE(result.getMappedChildElements().empty());
    EXPECT_TRUE(
        elementsMatch(result,
                      {
                          "Edge1;:H1,E;:H7,E",   "Edge1;:H2,E;:H7,E",   "Edge1;:H3,E;:H7,E",
                          "Edge2;:H1,E;:H7,E",   "Edge2;:H2,E;:H7,E",   "Edge2;:H3,E;:H7,E",
                          "Edge3;:H1,E;:H7,E",   "Edge3;:H2,E;:H7,E",   "Edge3;:H3,E;:H7,E",
                          "Edge4;:H1,E;:H7,E",   "Edge4;:H2,E;:H7,E",   "Edge4;:H3,E;:H7,E",
                          "Face1;:H2,F;:H7,F",   "Face1;:H3,F;:H7,F",   "Face1;:H4,F;:H7,F",
                          "Face1;:H5,F;:H7,F",   "Face1;:H6,F;:H7,F",   "Vertex1;:H1,V;:H7,V",
                          "Vertex1;:H2,V;:H7,V", "Vertex2;:H1,V;:H7,V", "Vertex2;:H2,V;:H7,V",
                          "Vertex3;:H1,V;:H7,V", "Vertex3;:H2,V;:H7,V", "Vertex4;:H1,V;:H7,V",
                          "Vertex4;:H2,V;:H7,V",
                      }));
}

TEST_F(TopoShapeExpansionTest, makeElementEvolve)
{
    BRepBuilderAPI_MakePolygon polygon(gp_Pnt(0.0, 0.0, 0.0),
                                       gp_Pnt(200.0, 0.0, 0.0),
                                       gp_Pnt(200.0, 200.0, 0.0),
                                       gp_Pnt(0.0, 200.0, 0.0));
    polygon.Close();
    TopoShape spine {polygon.Wire(), 1L};
    // Alternative:
    //    auto face {BRepBuilderAPI_MakeFace(polygon.Wire()).Face()};
    //    TopoShape spine {face, 11L};
    BRepBuilderAPI_MakePolygon polygon2(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(-60.0, -60.0, -200.0));
    TopoShape profile {polygon2.Wire(), 2L};
    // Alternative:
    //    TopoShape profile {
    //        BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(-60.0, -60.0, -200.0)).Edge(),
    //        10L};
    // Act
    TopoShape topoShape {3L};
    auto& result = topoShape.makeElementEvolve(spine, profile);
    Base::BoundBox3d bb = result.getBoundBox();
    // Assert shape is correct
    EXPECT_TRUE(
        PartTestHelpers::boxesMatch(bb, Base::BoundBox3d(-60.0, -60.0, -200.0, 260.0, 260.0, 0)));
    EXPECT_FLOAT_EQ(getVolume(result.getShape()), 8910324);
    // Assert elementMap is correct
    EXPECT_EQ(topoShape.getElementMap().size(), 0);
    // Neither the Spine nor the Profile have an elementMap, because they are simple wires or faces.
    // The resulting Evolved also does not populate the elementMap, but that might be a bug in
    // underutilized code.
    EXPECT_EQ(spine.getElementMap().size(), 0);
}

TEST_F(TopoShapeExpansionTest, traceElement)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    auto tr {gp_Trsf()};
    tr.SetTranslation(gp_Vec(gp_XYZ(-0.5, -0.5, 0)));
    cube2.Move(TopLoc_Location(tr));
    TopoShape topoShape1 {cube1, 1L};
    TopoShape topoShape2 {cube2, 2L};
    // Act
    TopoShape& result = topoShape1.makeElementCut(
        {topoShape1, topoShape2});  //, const char* op = nullptr, double tol = 0);
    std::string name {"Face2;:M;CUT;:H1:7,F"};
    //    auto faces = result.getSubTopoShapes(TopAbs_FACE);
    Data::MappedName mappedName(name);
    // Arrange
    Data::TraceCallback cb =
        [name](const Data::MappedName& elementname, int offset, long encodedTag, long tag) {
            boost::ignore_unused(offset);
            boost::ignore_unused(tag);
            // TODO:  This is likely a flawed way to address testing a callback.
            // Also, it isn't clear exactly what the correct results are, although as soon as
            // we start addressing History, we will quickly discover that, and likely the right
            // things to test.
            if (encodedTag == 1) {
                EXPECT_STREQ(elementname.toString().c_str(), name.c_str());
            }
            else {
                EXPECT_STREQ(elementname.toString().c_str(), name.substr(0, 5).c_str());
            }
            return false;
        };
    // Act
    result.traceElement(mappedName, cb);
    // Assert we have the element map we think we do.
    EXPECT_TRUE(allElementsMatch(
        result,
        {
            "Edge10;:G(Edge2;K-1;:H2:4,E);CUT;:H1:1a,V",
            "Edge10;:M;CUT;:H1:7,E",
            "Edge10;:M;CUT;:H1:7,E;:U;CUT;:H1:7,V",
            "Edge11;:M;CUT;:H2:7,E",
            "Edge12;:M;CUT;:H2:7,E",
            "Edge2;:M;CUT;:H2:7,E",
            "Edge2;:M;CUT;:H2:7,E;:U;CUT;:H2:7,V",
            "Edge4;:M;CUT;:H2:7,E",
            "Edge4;:M;CUT;:H2:7,E;:U;CUT;:H2:7,V",
            "Edge6;:G(Edge12;K-1;:H2:4,E);CUT;:H1:1b,V",
            "Edge6;:M;CUT;:H1:7,E",
            "Edge6;:M;CUT;:H1:7,E;:U;CUT;:H1:7,V",
            "Edge8;:G(Edge11;K-1;:H2:4,E);CUT;:H1:1b,V",
            "Edge8;:M;CUT;:H1:7,E",
            "Edge8;:M;CUT;:H1:7,E;:U;CUT;:H1:7,V",
            "Edge9;:G(Edge4;K-1;:H2:4,E);CUT;:H1:1a,V",
            "Edge9;:M;CUT;:H1:7,E",
            "Edge9;:M;CUT;:H1:7,E;:U;CUT;:H1:7,V",
            "Face1;:M;CUT;:H2:7,F",
            "Face1;:M;CUT;:H2:7,F;:U;CUT;:H2:7,E",
            "Face2;:G(Face4;K-1;:H2:4,F);CUT;:H1:1a,E",
            "Face2;:M;CUT;:H1:7,F",
            "Face2;:M;CUT;:H1:7,F;:U;CUT;:H1:7,E",
            "Face2;:M;CUT;:H1:7,F;:U;CUT;:H1:7,E;:L(Face5;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E;:U;CUT;:"
            "H1:7,V;:L(Face6;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E;:U;CUT;:H1:7,V);CUT;:H1:3c,E|Face5;:M;"
            "CUT;:H1:7,F;:U;CUT;:H1:7,E|Face6;:M;CUT;:H1:7,F;:U;CUT;:H1:7,E);CUT;:H1:c9,F",
            "Face3;:G(Face1;K-1;:H2:4,F);CUT;:H1:1a,E",
            "Face3;:M;CUT;:H1:7,F",
            "Face3;:M;CUT;:H1:7,F;:U;CUT;:H1:7,E",
            "Face3;:M;CUT;:H1:7,F;:U;CUT;:H1:7,E;:L(Face5;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E|Face5;:M;"
            "CUT;:H1:7,F;:U2;CUT;:H1:8,E;:U;CUT;:H1:7,V;:L(Face6;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E;:U;"
            "CUT;:H1:7,V);CUT;:H1:3c,E|Face6;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E);CUT;:H1:cb,F",
            "Face4;:M;CUT;:H2:7,F",
            "Face5;:M;CUT;:H1:7,F",
            "Face5;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E",
            "Face5;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E;:U;CUT;:H1:7,V",
            "Face5;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E;:U;CUT;:H1:7,V;:L(Face6;:M;CUT;:H1:7,F;:U2;CUT;:"
            "H1:8,E;:U;CUT;:H1:7,V);CUT;:H1:3c,E",
            "Face5;:M;CUT;:H1:7,F;:U;CUT;:H1:7,E",
            "Face6;:M;CUT;:H1:7,F",
            "Face6;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E",
            "Face6;:M;CUT;:H1:7,F;:U2;CUT;:H1:8,E;:U;CUT;:H1:7,V",
            "Face6;:M;CUT;:H1:7,F;:U;CUT;:H1:7,E",
        }));
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
