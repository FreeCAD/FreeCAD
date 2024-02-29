// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for the makeShape methods, extracted from the main set of tests for TopoShape
// due to length and complexity.

#include "gtest/gtest.h"
#include "src/App/InitApplication.h"
#include "PartTestHelpers.h"
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

using namespace Data;
using namespace Part;
using namespace PartTestHelpers;

class TopoShapeMakeShapeTests: public ::testing::Test
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
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    Part::TopoShape* Shape()
    {
        return &_shape;
    }

    Part::TopoShape::Mapper* Mapper()
    {
        return &_mapper;
    }

private:
    std::string _docName;
    Data::ElementIDRefs _sid;
    QVector<App::StringIDRef>* _sids = nullptr;
    Part::TopoShape _shape;
    Part::TopoShape::Mapper _mapper;
};

TEST_F(TopoShapeMakeShapeTests, nullShapeThrows)
{
    // Arrange
    auto [cube1, cube2] = CreateTwoCubes();
    std::vector<Part::TopoShape> sources {cube1, cube2};
    TopoDS_Vertex nullShape;

    // Act and assert
    EXPECT_THROW(Shape()->makeShapeWithElementMap(nullShape, *Mapper(), sources),
                 Part::NullShapeException);
}

TEST_F(TopoShapeMakeShapeTests, shapeVertex)
{
    // Arrange
    BRepBuilderAPI_MakeVertex vertexMaker = BRepBuilderAPI_MakeVertex(gp_Pnt(10, 10, 10));
    TopoShape topoShape(vertexMaker.Vertex(), 1L);
    // Act
    TopoShape& result = topoShape.makeElementShape(vertexMaker, topoShape);
    // Assert
    EXPECT_EQ(result.getElementMap().size(), 1);  // Changed with PR#12471. Probably will change
                                                  // again after importing other TopoNaming logics
    EXPECT_EQ(result.countSubElements("Vertex"), 1);
    EXPECT_EQ(result.countSubShapes("Vertex"), 1);
}

TEST_F(TopoShapeMakeShapeTests, thruSections)
{
    // Arrange
    auto [face1, wire1, edge1, edge2, edge3, edge4] = CreateRectFace();
    TopoDS_Wire wire2 = wire1;
    auto transform {gp_Trsf()};
    transform.SetTranslation(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(0.0, 0.5, 1.0));
    wire2.Move(TopLoc_Location(transform));
    TopoShape wire1ts {wire1, 1L};
    TopoShape wire2ts {wire2, 2L};
    BRepOffsetAPI_ThruSections thruMaker;
    thruMaker.AddWire(wire1);
    thruMaker.AddWire(wire2);
    TopoShape topoShape {};
    // Act
    TopoShape& result =
        topoShape.makeElementShape(thruMaker, {wire1ts, wire2ts}, OpCodes::ThruSections);
    auto elements = elementMap(result);
    // Assert
    EXPECT_EQ(elements.size(), 24);
    EXPECT_EQ(elements.count(IndexedName("Vertex", 1)), 1);
    EXPECT_EQ(elements[IndexedName("Vertex", 1)], MappedName("Vertex1;TRU;:H1:4,V"));  // NOLINT
    EXPECT_EQ(getVolume(result.getShape()), 4);
}

TEST_F(TopoShapeMakeShapeTests, sewing)
{
    // Arrange
    auto [face1, wire1, edge1, edge2, edge3, edge4] = CreateRectFace();
    auto face2 = face1;
    auto transform {gp_Trsf()};
    transform.SetTranslation(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(0.5, 0.5, 0.0));
    face2.Move(TopLoc_Location(transform));
    BRepBuilderAPI_Sewing sewer;
    sewer.Add(face1);
    sewer.Add(face2);
    sewer.Perform();
    std::vector<TopoShape> sources {{face1, 1L}, {face2, 2L}};
    TopoShape topoShape {};
    // Act
    TopoShape& result = topoShape.makeShapeWithElementMap(sewer.SewedShape(),
                                                          MapperSewing(sewer),
                                                          sources,
                                                          OpCodes::Sewing);

    auto elements = elementMap(result);
    // Assert
    EXPECT_EQ(&result, &topoShape);
    EXPECT_EQ(elements.size(), 18);  // Now a single cube
    EXPECT_EQ(elements.count(IndexedName("Vertex", 1)), 1);
    EXPECT_EQ(elements[IndexedName("Vertex", 1)], MappedName("Vertex1;SEW;:H1:4,V"));
    EXPECT_EQ(getArea(result.getShape()), 12);
}
