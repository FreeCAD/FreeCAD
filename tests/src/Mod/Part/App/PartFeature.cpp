// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "Mod/Part/App/FeaturePartCommon.h"
#include <src/App/InitApplication.h>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include "PartTestHelpers.h"
#include "App/MappedElement.h"

using namespace Part;
using namespace PartTestHelpers;

class FeaturePartTest: public ::testing::Test, public PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }


    void SetUp() override
    {
        createTestDoc();
        _common = dynamic_cast<Common*>(_doc->addObject("Part::Common"));
    }

    void TearDown() override
    {}

    Common* _common = nullptr;  // NOLINT Can't be private in a test framework
};

TEST_F(FeaturePartTest, testGetElementName)
{
    // Arrange
    _common->Base.setValue(_boxes[0]);
    _common->Tool.setValue(_boxes[1]);

    // Act
    _common->execute();
    const TopoShape& ts = _common->Shape.getShape();

    auto namePair = _common->getElementName("test");
    auto namePairExport = _common->getElementName("test", App::GeoFeature::Export);
    auto namePairSelf = _common->getElementName(nullptr);
    // Assert
    EXPECT_STREQ(namePair.first.c_str(), "");
    EXPECT_STREQ(namePair.second.c_str(), "test");
    EXPECT_STREQ(namePairExport.first.c_str(), "");
    EXPECT_STREQ(namePairExport.second.c_str(), "test");
    EXPECT_STREQ(namePairSelf.first.c_str(), "");
    EXPECT_STREQ(namePairSelf.second.c_str(), "");
#ifndef FC_USE_TNP_FIX
    EXPECT_EQ(ts.getElementMap().size(), 0);
#else
    EXPECT_EQ(ts.getElementMap().size(), 26);
#endif
    // TBD
}

TEST_F(FeaturePartTest, create)
{
    // Arrange

    // A shape that will be passed to the various calls of Feature::create
    auto shape {TopoShape(BRepBuilderAPI_MakeVertex(gp_Pnt(1.0, 1.0, 1.0)).Vertex(), 1)};

    auto otherDocName {App::GetApplication().getUniqueDocumentName("otherDoc")};
    // Another document where it will be created a shape
    auto otherDoc {App::GetApplication().newDocument(otherDocName.c_str(), "otherDocUser")};

    // _doc is populated by PartTestHelperClass::createTestDoc. Making it an empty document
    _doc->clearDocument();

    // Setting the active document back to _doc otherwise the first 3 calls to Feature::create will
    // act on otherDoc
    App::GetApplication().setActiveDocument(_doc);

    // Act

    // A feature with an empty TopoShape
    auto featureNoShape {Feature::create(TopoShape())};

    // A feature with a TopoShape
    auto featureNoName {Feature::create(shape)};

    // A feature with a TopoShape and a name
    auto featureNoDoc {Feature::create(shape, "Vertex")};

    // A feature with a TopoShape and a name in the document otherDoc
    auto feature {Feature::create(shape, "Vertex", otherDoc)};

    // Assert

    // Check that the shapes have been added. Only featureNoShape should return an empty shape, the
    // others should have it as TopoShape shape is passed as argument
    EXPECT_TRUE(featureNoShape->Shape.getValue().IsNull());
    EXPECT_FALSE(featureNoName->Shape.getValue().IsNull());
    EXPECT_FALSE(featureNoDoc->Shape.getValue().IsNull());
    EXPECT_FALSE(feature->Shape.getValue().IsNull());

    // Check the features names

    // Without a name the feature's name will be set to "Shape"
    EXPECT_STREQ(_doc->getObjectName(featureNoShape), "Shape");

    // In _doc there's already a shape with name "Shape" and, as there can't be duplicated names in
    // the same document, the other feature will get an unique name that will still contain "Shape"
    EXPECT_STREQ(_doc->getObjectName(featureNoName), "Shape001");

    // There aren't other features with name "Vertex" in _doc, therefor that name will be assigned
    // without modifications
    EXPECT_STREQ(_doc->getObjectName(featureNoDoc), "Vertex");

    // The feature is created in otherDoc, which doesn't have other features and thertherefor the
    // feature's name will be assigned without modifications
    EXPECT_STREQ(otherDoc->getObjectName(feature), "Vertex");

    // Check that the features have been created in the correct document

    // The first 3 calls to Feature::create acts on _doc, which is empty, and therefor the number of
    // features in that document is the same of the features created with Feature::create
    EXPECT_EQ(_doc->getObjects().size(), 3);

    // The last call to Feature::create acts on otherDoc, which is empty, and therefor that document
    // will have only 1 feature
    EXPECT_EQ(otherDoc->getObjects().size(), 1);
}

TEST_F(FeaturePartTest, getElementHistory)
{
    // Arrange
    const char* name = "Part__Box";
    const char* name2 = "Edge2";  // Edge, Vertex, or Face. will work here.
    // Act
    auto result = Feature::getElementHistory(_boxes[0], name2, true, false);
    Data::HistoryItem histItem = result.front();
    // Assert
    EXPECT_EQ(result.size(), 1);
    EXPECT_NE(histItem.tag, 0);  // Make sure we have one.  It will vary.
    EXPECT_EQ(histItem.index.getIndex(), 2);
    EXPECT_STREQ(histItem.index.getType(), "Edge");
    EXPECT_STREQ(histItem.element.toString().c_str(), name2);
    EXPECT_EQ(histItem.obj, _boxes[0]);
}

TEST_F(FeaturePartTest, getRelatedElements)
{
    // Arrange
    _common->Base.setValue(_boxes[0]);
    _common->Tool.setValue(_boxes[1]);
    // Act
    _common->execute();
    auto label1 = _common->Label.getValue();
    auto label2 = _boxes[1]->Label.getValue();
    const TopoShape& ts = _common->Shape.getShape();
    auto result = Feature::getRelatedElements(_doc->getObject(label1),
                                              "Edge2",
                                              HistoryTraceType::followTypeChange,
                                              true);
    auto result2 = Feature::getRelatedElements(_doc->getObject(label2),
                                               "Edge1",
                                               HistoryTraceType::followTypeChange,
                                               true);
    // Assert
#ifdef FC_USE_TNP_FIX
    EXPECT_EQ(result.size(), 1);   // Found the one.
    EXPECT_EQ(result2.size(), 0);  // No element map, so no related elements
    // The results are always going to vary, so we can't test for specific values:
    // EXPECT_STREQ(result.front().name.toString().c_str(),"Edge3;:M;CMN;:H38d:7,E");
#else
    EXPECT_EQ(result.size(), 0);
#endif
}

// Note that this test is pretty trivial and useless .. but the method in question is never
// called in the codebase.
TEST_F(FeaturePartTest, getElementFromSource)
{
    // Arrange
    _common->Base.setValue(_boxes[0]);
    _common->Tool.setValue(_boxes[1]);
    App::DocumentObject sourceObject;
    //    const char *sourceSubElement;
    // Act
    _common->execute();
    auto label1 = _common->Label.getValue();
    auto label2 = _boxes[1]->Label.getValue();
    const TopoShape& ts = _common->Shape.getShape();
    auto element = Feature::getElementFromSource(_common,
                                                 "Part__Box001",  // "Edge1",
                                                 _boxes[0],
                                                 "Face1",  // "Edge1",
                                                 false);
    // Assert
    EXPECT_EQ(element.size(), 0);
}

TEST_F(FeaturePartTest, getSubObject)
{
    // Arrange
    _common->Base.setValue(_boxes[0]);
    _common->Tool.setValue(_boxes[1]);
    App::DocumentObject sourceObject;
    const char* sourceSubElement;
    PyObject* pyObj;
    // Act
    _common->execute();
    auto result = _boxes[1]->getSubObject("Face5", &pyObj, nullptr, false, 10);
    // Assert
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result->getNameInDocument(), "Part__Box001");
}
