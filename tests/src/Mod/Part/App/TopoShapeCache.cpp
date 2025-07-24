// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeCache.h>

#include <src/App/InitApplication.h>
#include <gp_Quaternion.hxx>
#include <TopoDS_TVertex.hxx>
#include <BRep_TVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <TopoDS_Edge.hxx>

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

class ShapeRelationKey: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};

TEST_F(ShapeRelationKey, HistoryTraceTypeComparison)
{
    // Arrange
    Data::MappedName mappedName {"mappedName"};
    Part::HistoryTraceType htt1 {Part::HistoryTraceType::stopOnTypeChange};
    Part::HistoryTraceType htt2 {Part::HistoryTraceType::followTypeChange};
    Part::ShapeRelationKey key1 {mappedName, htt1};
    Part::ShapeRelationKey key2 {mappedName, htt2};

    // Act
    bool key1LessThanKey2 = key1 < key2;

    // Assert
    ASSERT_TRUE(key1LessThanKey2);
}

class TopoShapeCacheTest: public ::testing::Test
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

TEST_F(TopoShapeCacheTest, ConstructionFromTopoDS_Shape)
{
    // Arrange - create a TopoDS shape with some location transformation applied
    TopoDS_Vertex vertex;
    gp_Quaternion quaternion(1.0, 2.0, 3.0, 4.0);
    gp_Trsf transform;
    transform.SetRotation(quaternion);
    auto location = TopLoc_Location(transform);
    vertex.Location(location);

    // Act
    auto cache = Part::TopoShapeCache(vertex);

    // Assert - ensure the location of the cached shape was zeroed out
    EXPECT_NE(cache.shape.Location(), vertex.Location());
}

TEST_F(TopoShapeCacheTest, InsertRelationIntoEmptyTableCompacts)
{
    // Arrange
    Data::IndexedName indexedName {"EDGE1"};
    auto mappedName =
        Data::MappedName::fromRawData("#94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F");
    ASSERT_TRUE(mappedName.isRaw());
    Data::MappedElement mappedElement1 {indexedName, mappedName};
    QVector<Data::MappedElement> vectorOfElements {mappedElement1};
    TopoDS_Vertex vertex;
    Part::TopoShapeCache cache(vertex);
    Part::ShapeRelationKey key {mappedName, Part::HistoryTraceType::followTypeChange};

    // Act
    cache.insertRelation(key, vectorOfElements);

    // Assert
    auto foundIterator = cache.relations.find(key);
    EXPECT_NE(foundIterator, cache.relations.end());
    EXPECT_FALSE(foundIterator->first.name.isRaw());  // compact() was called
}

TEST_F(TopoShapeCacheTest, InsertAlreadyExistsUpdatesExisting)
{
    // Arrange
    Data::IndexedName indexedName {"EDGE1"};
    Data::MappedName mappedName("#94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F");
    Data::MappedElement mappedElement1 {indexedName, mappedName};
    QVector<Data::MappedElement> vectorOfElements {mappedElement1};
    TopoDS_Vertex vertex;
    Part::TopoShapeCache cache(vertex);
    Part::ShapeRelationKey key {mappedName, Part::HistoryTraceType::followTypeChange};

    // Act
    cache.insertRelation(key, vectorOfElements);
    QVector<Data::MappedElement> emptyVector;
    cache.insertRelation(key, emptyVector);

    // Assert
    EXPECT_TRUE(cache.relations.find(key)->second.empty());
}

TEST_F(TopoShapeCacheTest, IsTouchedNotPartners)
{
    // Arrange
    BRep_TVertex* vertex1 = new BRep_TVertex;
    vertex1->Pnt(gp_Pnt(1.0, 1.0, 1.0));
    BRep_TVertex* vertex2 = new BRep_TVertex;
    vertex2->Pnt(gp_Pnt(2.0, 2.0, 2.0));
    opencascade::handle<TopoDS_TShape> handle1(vertex1);
    opencascade::handle<TopoDS_TShape> handle2(vertex2);
    TopoDS_Vertex tds1;
    TopoDS_Vertex tds2;
    tds1.TShape(handle1);
    tds2.TShape(handle2);
    ASSERT_FALSE(tds1.IsPartner(tds2));
    Part::TopoShapeCache cache(tds1);

    // Act & Assert
    EXPECT_TRUE(cache.isTouched(tds2));
}

TEST_F(TopoShapeCacheTest, IsTouchedArePartners)
{
    // Arrange
    BRep_TVertex* vertex1 = new BRep_TVertex;
    vertex1->Pnt(gp_Pnt(1.0, 1.0, 1.0));
    opencascade::handle<TopoDS_TShape> handle1(vertex1);
    TopoDS_Vertex tds1;
    TopoDS_Vertex tds2;
    tds1.TShape(handle1);
    tds2.TShape(handle1);
    ASSERT_TRUE(tds1.IsPartner(tds2));
    Part::TopoShapeCache cache(tds1);

    // Act & Assert
    EXPECT_FALSE(cache.isTouched(tds2));
}

std::tuple<TopoDS_Shape, std::pair<TopoDS_Shape, TopoDS_Shape>> CreateShapeWithSubshapes()
{
    auto edge1 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0)).Edge();
    auto edge2 = BRepBuilderAPI_MakeEdge(gp_Pnt(1.0, 0.0, 0.0), gp_Pnt(2.0, 0.0, 0.0)).Edge();
    auto fuse = BRepAlgoAPI_Fuse(edge1, edge2);
    fuse.Build();
    return {fuse.Shape(), {edge1, edge2}};
}

TEST_F(TopoShapeCacheTest, GetAncestrySHAPE)
{
    // Arrange
    auto shape = std::get<0>(CreateShapeWithSubshapes());
    Part::TopoShapeCache cache(shape);

    // Act
    auto ancestry = cache.getAncestry(TopAbs_SHAPE);

    // Assert
    EXPECT_EQ(2, ancestry.count());
}

TEST_F(TopoShapeCacheTest, GetAncestryEDGE)
{
    // Arrange
    auto shape = std::get<0>(CreateShapeWithSubshapes());
    Part::TopoShapeCache cache(shape);

    // Act
    auto ancestry = cache.getAncestry(TopAbs_EDGE);

    // Assert
    EXPECT_EQ(2, ancestry.count());
}

TEST_F(TopoShapeCacheTest, GetAncestryFACE)
{
    // Arrange
    auto shape = std::get<0>(CreateShapeWithSubshapes());
    Part::TopoShapeCache cache(shape);

    // Act
    auto ancestry = cache.getAncestry(TopAbs_FACE);

    // Assert
    EXPECT_EQ(0, ancestry.count());
}

TEST_F(TopoShapeCacheTest, CountShape)
{
    // Arrange
    auto shape = std::get<0>(CreateShapeWithSubshapes());
    Part::TopoShapeCache cache(shape);

    // Act
    int countOfEdges = cache.countShape(TopAbs_EDGE);
    int countOfFaces = cache.countShape(TopAbs_FACE);
    int countOfShapes = cache.countShape(TopAbs_SHAPE);

    // Assert
    EXPECT_EQ(2, countOfEdges);
    EXPECT_EQ(0, countOfFaces);
    EXPECT_EQ(2, countOfShapes);
}

TEST_F(TopoShapeCacheTest, FindShapeGivenSubshape)
{
    // Arrange
    const auto [shape, ancestors] = CreateShapeWithSubshapes();
    Part::TopoShapeCache cache(shape);

    // Act
    auto shapeResult1 = cache.findShape(ancestors.first, shape);
    auto shapeResult2 = cache.findShape(ancestors.second, shape);

    // Assert
    EXPECT_NE(0, shapeResult1);
    EXPECT_NE(0, shapeResult2);
}

TEST_F(TopoShapeCacheTest, FindShapeGivenTypeAndIndex)
{
    // Arrange
    const auto [shape, ancestors] = CreateShapeWithSubshapes();
    Part::TopoShapeCache cache(shape);

    // Act
    auto shapeResult = cache.findShape(ancestors.first, TopAbs_EDGE, 1);  // NOT zero-indexed!

    // Assert
    EXPECT_FALSE(shapeResult.IsNull());
}

std::tuple<TopoDS_Shape, std::pair<TopoDS_Shape, TopoDS_Shape>> CreateFusedCubes()
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

    auto fuse = BRepAlgoAPI_Fuse(box1, box2);
    fuse.Build();

    return {fuse, {box1, box2}};
}

TEST_F(TopoShapeCacheTest, FindAncestor)
{
    // Arrange
    const auto [shape, ancestors] = CreateFusedCubes();
    Part::TopoShapeCache cache(shape);

    // Act
    auto ancestorResultCompound = cache.findAncestor(ancestors.first, shape, TopAbs_COMPOUND);

    // Assert
    EXPECT_FALSE(ancestorResultCompound.IsNull());
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
