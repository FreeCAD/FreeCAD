// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for the makeShapeWithElementMap method, extracted from the main set of tests for TopoShape
// due to length and complexity.

#include "gtest/gtest.h"
#include "src/App/InitApplication.h"
#include "TopoShapeExpansionHelpers.h"
#include <Mod/Part/App/TopoShape.h>
// #include <MappedName.h>
#include <TopoDS_Vertex.hxx>

class TopoShapeMakeShapeWithElementMapTests: public ::testing::Test
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


TEST_F(TopoShapeMakeShapeWithElementMapTests, nullShapeThrows)
{
    // Arrange
    auto [cube1, cube2] = TopoShapeExpansionHelpers::CreateTwoCubes();
    std::vector<Part::TopoShape> sources {cube1, cube2};
    TopoDS_Vertex nullShape;

    // Act and assert
    EXPECT_THROW(Shape()->makeShapeWithElementMap(nullShape, *Mapper(), sources),
                 Part::NullShapeException);
}

using Data::IndexedName, Data::MappedName;
using Part::TopoShape;

// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapVertex)
// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapEdge)
// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapWire)
// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapFace)
// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapShell)
// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapSolid)

TEST_F(TopoShapeMakeShapeWithElementMapTests, mapCompound)
{
    // Arrange
    auto [cube1, cube2] = TopoShapeExpansionHelpers::CreateTwoCubes();
    auto transform {gp_Trsf()};
    transform.SetTranslation(gp_Vec(gp_XYZ(-0.5, -0.5, 0)));
    cube2.Move(TopLoc_Location(transform));
    std::vector<TopoShape> sources {cube1, cube2};
    sources[0].Tag = 1;
    sources[1].Tag = 2;
    // Map only one of the two sources to test different names.
    sources[0].makeShapeWithElementMap(sources[0].getShape(), *Mapper(), {sources[0]});
    TopoShape compound;
    compound.makeElementCompound(sources);
    auto preElements = compound.getElementMap();  // Map before mapping.
    std::map<IndexedName, MappedName> preMap;
    for (auto name : preElements) {
        preMap.emplace(name.index, name.name);
    }
    // Act
    compound.makeShapeWithElementMap(compound.getShape(), *Mapper(), sources);
    auto elements = compound.getElementMap();  // Map after mapping
    std::map<IndexedName, MappedName> map;
    for (auto name : elements) {
        map.emplace(name.index, name.name);
    }
    // Assert
    EXPECT_EQ(preElements.size(), 52);  // Check the before map.
    EXPECT_EQ(preMap[IndexedName("Edge", 1)], MappedName("Edge1;MAK;:H:4,E;:H1:b,E"));
    EXPECT_EQ(preMap[IndexedName("Edge", 13)], MappedName("Edge1;:H2,E"));

    EXPECT_EQ(elements.size(), 52);  // 12 Edges, 8 Vertexes, 6 Faces per each Cube
    EXPECT_EQ(map[IndexedName("Edge", 1)], MappedName("Edge1;MAK;:H:4,E;MAK;:H1:f,E"));
    EXPECT_EQ(map[IndexedName("Edge", 13)], MappedName("Edge1;MAK;:H2:4,E"));
    EXPECT_EQ(map.count(IndexedName("Edge", 24)), 1);
    EXPECT_EQ(map.count(IndexedName("Edge", 25)), 0);
    EXPECT_EQ(map.count(IndexedName("Vertex", 16)), 1);
    EXPECT_EQ(map.count(IndexedName("Vertex", 17)), 0);
    EXPECT_EQ(map.count(IndexedName("Face", 12)), 1);
    EXPECT_EQ(map.count(IndexedName("Face", 13)), 0);
    EXPECT_STREQ(sources[0].shapeName().c_str(), "Solid");
    EXPECT_STREQ(sources[1].shapeName().c_str(), "Solid");
    EXPECT_STREQ(compound.shapeName().c_str(), "Compound");
}
// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapCompSolid)
// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapOffsetCubes)
// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapIntersectingCubes)
