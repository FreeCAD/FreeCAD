// SPDX-License-Identifier: LGPL-2.1-or-later

// Tests for the makeShapeWithElementMap method, extracted from the main set of tests for TopoShape
// due to length and complexity.

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"
#include "PartTestHelpers.h"
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_CompSolid.hxx>
#include <TopoDS_Compound.hxx>

using namespace Part;
using namespace Data;

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

    void testFindSourceSubShapesInElementMapForSource(const std::vector<TopoShape>& sources,
                                                      const TopoShape& source);

private:
    std::string _docName;
    Data::ElementIDRefs _sid;
    Part::TopoShape _shape;
    Part::TopoShape::Mapper _mapper;
};

TEST_F(TopoShapeMakeShapeWithElementMapTests, nullShapeThrows)
{
    // Arrange
    auto [cube1, cube2] = PartTestHelpers::CreateTwoCubes();
    std::vector<Part::TopoShape> sources {cube1, cube2};
    TopoDS_Vertex nullVertex;
    TopoDS_Edge nullEdge;
    TopoDS_Wire nullWire;
    TopoDS_Face nullFace;
    TopoDS_Shell nullShell;
    TopoDS_Solid nullSolid;
    TopoDS_CompSolid nullCompSolid;
    TopoDS_Compound nullCompound;

    // Act and assert
    EXPECT_THROW(Shape()->makeShapeWithElementMap(nullVertex, *Mapper(), sources),
                 Part::NullShapeException);
    EXPECT_THROW(Shape()->makeShapeWithElementMap(nullEdge, *Mapper(), sources),
                 Part::NullShapeException);
    EXPECT_THROW(Shape()->makeShapeWithElementMap(nullWire, *Mapper(), sources),
                 Part::NullShapeException);
    EXPECT_THROW(Shape()->makeShapeWithElementMap(nullFace, *Mapper(), sources),
                 Part::NullShapeException);
    EXPECT_THROW(Shape()->makeShapeWithElementMap(nullShell, *Mapper(), sources),
                 Part::NullShapeException);
    EXPECT_THROW(Shape()->makeShapeWithElementMap(nullSolid, *Mapper(), sources),
                 Part::NullShapeException);
    EXPECT_THROW(Shape()->makeShapeWithElementMap(nullCompSolid, *Mapper(), sources),
                 Part::NullShapeException);
    EXPECT_THROW(Shape()->makeShapeWithElementMap(nullCompound, *Mapper(), sources),
                 Part::NullShapeException);
}

std::map<IndexedName, MappedName> elementMap(const TopoShape& shape)
{
    std::map<IndexedName, MappedName> result {};
    auto elements = shape.getElementMap();
    for (auto const& entry : elements) {
        result[entry.index] = entry.name;
    }
    return result;
}

// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapVertex)
// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapEdge)
// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapWire)
// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapFace)
// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapShell)
// TEST_F(TopoShapeMakeShapeWithElementMapTests, mapSolid)

TEST_F(TopoShapeMakeShapeWithElementMapTests, mapCompoundCount)
{
    // Arrange
    auto [cube1TS, cube2TS] = PartTestHelpers::CreateTwoTopoShapeCubes();
    std::vector<TopoShape> sources {cube1TS, cube2TS};
    TopoShape compound {3L};
    compound.makeElementCompound(sources);
    auto preElements = elementMap(compound);  // Map before mapping.
    // Act
    compound.makeShapeWithElementMap(compound.getShape(), *Mapper(), sources);
    auto postElements = elementMap(compound);  // Map after mapping
    // Assert
    EXPECT_EQ(preElements.size(), 52);   // Check the before map.
    EXPECT_EQ(postElements.size(), 52);  // 12 Edges, 8 Vertexes, 6 Faces per each Cube
    EXPECT_EQ(postElements.count(IndexedName("Edge", 24)), 1);
    EXPECT_EQ(postElements.count(IndexedName("Edge", 25)), 0);
    EXPECT_EQ(postElements.count(IndexedName("Vertex", 16)), 1);
    EXPECT_EQ(postElements.count(IndexedName("Vertex", 17)), 0);
    EXPECT_EQ(postElements.count(IndexedName("Face", 12)), 1);
    EXPECT_EQ(postElements.count(IndexedName("Face", 13)), 0);
    EXPECT_STREQ(sources[0].shapeName().c_str(), "Compound");
    EXPECT_STREQ(sources[1].shapeName().c_str(), "Compound");
    EXPECT_STREQ(compound.shapeName().c_str(), "Compound");
    EXPECT_EQ(
        22,
        compound.getMappedChildElements().size());  // Changed with PR#12471. Probably will change
                                                    // again after importing other TopoNaming logics
}

TEST_F(TopoShapeMakeShapeWithElementMapTests, emptySourceShapes)
{
    // Arrange
    auto [cube1, cube2] = PartTestHelpers::CreateTwoCubes();
    std::vector<Part::TopoShape> emptySources;
    std::vector<Part::TopoShape> nonEmptySources {cube1, cube2};

    // Act and assert
    for (auto& source : nonEmptySources) {
        Part::TopoShape& modifiedShape = source;
        Part::TopoShape& originalShape = source;

        EXPECT_EQ(
            &originalShape,
            &modifiedShape.makeShapeWithElementMap(source.getShape(), *Mapper(), emptySources));
    }
}

TEST_F(TopoShapeMakeShapeWithElementMapTests, nonMappableSources)
{
    // Arrange
    auto [cube1, cube2] = PartTestHelpers::CreateTwoCubes();
    std::vector<Part::TopoShape> sources {cube1, cube2};

    // Act and assert
    for (auto& source : sources) {
        size_t canMap = 0;
        for (const auto& mappableSource : sources) {
            if (source.canMapElement(mappableSource)) {
                ++canMap;
            }
        }

        if (canMap == 0U) {
            EXPECT_EQ(&source,
                      &source.makeShapeWithElementMap(source.getShape(), *Mapper(), sources));
        }
    }
}

void testFindSourceShapesInSingleShape(const Part::TopoShape& cmpdShape,
                                       const Part::TopoShape& source,
                                       const std::vector<Part::TopoShape>& sources,
                                       const TopoShape::Mapper& mapper)
{
    std::vector<Part::TopoShape> tmpSources {source};
    for (const auto& subSource : sources) {
        Part::TopoShape tmpShape {source.getShape()};
        tmpShape.makeShapeWithElementMap(source.getShape(), mapper, tmpSources);
        if (&source == &subSource) {
            EXPECT_NE(tmpShape.findShape(subSource.getShape()),
                      0);  // if tmpShape uses, for example, cube1 and we search for cube1 than
                           // we should find it
        }
        else {
            EXPECT_EQ(tmpShape.findShape(subSource.getShape()),
                      0);  // if tmpShape uses, for example, cube1 and we search for cube2 than
                           // we shouldn't find it
        }
    }
    EXPECT_NE(cmpdShape.findShape(source.getShape()),
              0);  // as cmpdShape is made with cube1 and cube2 we should find both of them
}

TEST_F(TopoShapeMakeShapeWithElementMapTests, findSourceShapesInShape)
{
    // Arrange
    auto [cube1, cube2] = PartTestHelpers::CreateTwoCubes();
    std::vector<Part::TopoShape> sources {cube1, cube2};
    sources[0].Tag = 1;  // setting Tag explicitly otherwise it is likely that this test will be
                         // more or less the same of nonMappableSources
    sources[1].Tag = 2;  // setting Tag explicitly otherwise it is likely that this test will be
                         // more or less the same of nonMappableSources
    Part::TopoShape cmpdShape;
    cmpdShape.makeElementCompound(sources);

    // Act and assert
    for (const auto& source : sources) {
        testFindSourceShapesInSingleShape(cmpdShape, source, sources, *Mapper());
    }
}

void testFindSubShapesForSourceWithTypeAndIndex(const std::string& shapeTypeStr,
                                                std::map<IndexedName, MappedName>& elementStdMap,
                                                unsigned long shapeIndex)
{
    std::string shapeIndexStr = std::to_string(shapeIndex);
    std::string shapeName {shapeTypeStr + shapeIndexStr};

    IndexedName indexedName {shapeTypeStr.c_str(), (int)shapeIndex};
    MappedName mappedName {elementStdMap[indexedName]};
    const char shapeTypePrefix {indexedName.toString()[0]};

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_MSVC(4834)  // Discarding a [[nodiscard]], which we are about to do...
    // We check that the IndexedName is one of the keys...
    EXPECT_NO_THROW(elementStdMap.at(indexedName));
    // ... that the element name is in the MappedName...
    EXPECT_NE(mappedName.find(shapeName.c_str()), -1);
    EXPECT_EQ(mappedName.toString().back(), shapeTypePrefix);
    QT_WARNING_POP
}

void testFindSubShapesForSourceWithType(const TopoShape& source,
                                        const char* shapeType,
                                        std::map<IndexedName, MappedName>& elementStdMap)
{
    std::string shapeTypeStr {shapeType};

    // ... and all the elements of the various types in the source TopoShape ...
    for (unsigned long shapeIndex = 1U; shapeIndex <= source.countSubElements(shapeType);
         shapeIndex++) {
        testFindSubShapesForSourceWithTypeAndIndex(shapeTypeStr, elementStdMap, shapeIndex);
    }

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_MSVC(4834)  // Discarding a [[nodiscard]], which we are about to do...
    // ... we also check that we don't find shapes that don't exist and therefore don't
    // have either an IndexedName or a MappedName
    IndexedName fakeIndexedName {shapeTypeStr.c_str(), (int)source.countSubElements(shapeType) + 1};
    EXPECT_THROW(elementStdMap.at(fakeIndexedName), std::out_of_range);
    QT_WARNING_POP
}

void TopoShapeMakeShapeWithElementMapTests::testFindSourceSubShapesInElementMapForSource(
    const std::vector<TopoShape>& sources,
    const TopoShape& source)
{
    TopoShape tmpShape {source.getShape()};
    tmpShape.makeShapeWithElementMap(source.getShape(), *Mapper(), sources);

    // First we create a map with the IndexedNames and MappedNames
    std::map<IndexedName, MappedName> elementStdMap;
    for (const auto& mappedElement : tmpShape.getElementMap()) {
        elementStdMap.emplace(mappedElement.index, mappedElement.name);
    }

    // Then for all the elements types (Vertex, Edge, Face) ...
    for (const auto& shapeType : source.getElementTypes()) {
        testFindSubShapesForSourceWithType(source, shapeType, elementStdMap);
    }
}

TEST_F(TopoShapeMakeShapeWithElementMapTests, findSourceSubShapesInElementMap)
{
    // Arrange
    auto [cube1, cube2] = PartTestHelpers::CreateTwoCubes();
    std::vector<TopoShape> sources {cube1, cube2};
    sources[0].Tag = 1;  // setting Tag explicitly otherwise it is likely that this test will be
                         // more or less the same of nonMappableSources
    sources[1].Tag = 2;  // setting Tag explicitly otherwise it is likely that this test will be
                         // more or less the same of nonMappableSources

    // Act and assert
    // Testing with all the source TopoShapes
    for (const auto& source : sources) {
        testFindSourceSubShapesInElementMapForSource(sources, source);
    }
}

TEST_F(TopoShapeMakeShapeWithElementMapTests, findMakerOpInElementMap)
{
    // Arrange
    auto [cube1, cube2] = PartTestHelpers::CreateTwoCubes();
    std::vector<TopoShape> sources {cube1, cube2};
    sources[0].Tag = 1;  // setting Tag explicitly otherwise it is likely that this test will be
                         // more or less the same of nonMappableSources
    sources[1].Tag = 2;  // setting Tag explicitly otherwise it is likely that this test will be
                         // more or less the same of nonMappableSources

    // Act and assert
    // Testing with all the source TopoShapes
    for (const auto& source : sources) {
        TopoShape tmpShape {source.getShape()};
        tmpShape.makeShapeWithElementMap(source.getShape(), *Mapper(), sources);
        EXPECT_EQ(tmpShape.getElementMapSize(), 26);

        // For all the mappedElements ...
        // for (const auto& mappedElement : tmpShape.getElementMap()) {
        // TODO:  This no longer works, it needs a different check.  We don't set MAK
        //            EXPECT_NE(mappedElement.name.find(OpCodes::Maker),
        //                      -1);  // ... we check that there's the "MAK" OpCode
        //}
    }
}

std::string composeTagInfo(const MappedElement& element, const TopoShape& shape)
{
    std::string elementNameStr {element.name.constPostfix()};
    std::string tagInfo = POSTFIX_TAG + std::to_string(shape.Tag);
    tagInfo +=
        ":" + std::to_string(elementNameStr.substr(0, elementNameStr.find(tagInfo)).length());

    return tagInfo;
}
