// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"
#include <gmock/gmock.h>

#include <src/App/InitApplication.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeatureGroupExtension.h>
#include <Base/Interpreter.h>

using namespace App;

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

class DocumentObjectTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    std::string _docName {};
    App::Document* _doc {};
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
};


TEST_F(DocumentObjectTest, getSubObjectList)
{
    // Arrange

    // The name of a Part::Box added to the document
    auto boxName {_doc->addObject("Part::Box")->getNameInDocument()};
    // The name of a Part::Cylinder added to the document
    auto cylName {_doc->addObject("Part::Cylinder")->getNameInDocument()};
    // An App::Part object added to the document
    auto part {_doc->addObject("App::Part")};
    // The name of the App::Part added to the document
    auto partName {part->getNameInDocument()};
    // The name of the object used as argument for the calls of DocumentObject::getSubObjectList()
    auto subName {std::string()};

    // A vector of int used as argument for the call of DocumentObject::getSubObjectList() with
    // argument flatten not set (or set to false)
    auto sizesNoFlatten {std::vector<int>()};
    // A vector of int used as argument for the call of DocumentObject::getSubObjectList() with
    // argument flatten set to true
    auto sizesFlatten {std::vector<int>()};
    // A helper string used to compose the argument of some calls of
    // Base::Interpreter().runString()
    auto cmd {std::string()};

    // Performing a fusion to create an object that will be searched with the calls of
    // DocumentObject::getSubObjectList()
    Base::Interpreter().runString("from BOPTools import BOPFeatures");
    Base::Interpreter().runString("bp = BOPFeatures.BOPFeatures(App.activeDocument())");
    cmd = "bp.make_multi_fuse([\"";
    cmd += boxName;
    cmd += "\", \"";
    cmd += cylName;
    cmd += "\", ])";
    Base::Interpreter().runString(cmd.c_str());
    Base::Interpreter().runString("App.ActiveDocument.recompute()");
    // The name of the fusion object
    auto fuseName {_doc->getObject("Fusion")->getNameInDocument()};

    // Defining the name of the object that will be searched with the calls of
    // DocumentObject::getSubObjectList()
    subName = fuseName;
    subName += ".";
    subName += boxName;
    subName += ".Edge1";

    // Adding the fusion to the App::Part object to test the differences between calling
    // DocumentObject::getSubObjectList() with the flatten argument set to false or set to true
    cmd = "App.ActiveDocument.getObject(\"";
    cmd += partName;
    cmd += "\").addObject(App.ActiveDocument.getObject(\"";
    cmd += fuseName;
    cmd += "\"))";
    Base::Interpreter().runString(cmd.c_str());
    Base::Interpreter().runString("App.ActiveDocument.recompute()");

    // A vector of DocumentObjects used to store the result of the call to
    // DocumentObject::getSubObjectList() without the subname parameter
    auto docSubObjsNoSubName {std::vector<DocumentObject*>()};
    // A vector of DocumentObjects used to store the result of the call to
    // DocumentObject::getSubObjectList() with only the subname parameter
    auto docSubObjsWithSubName {std::vector<DocumentObject*>()};
    // A vector of DocumentObjects used to store the result of the call to
    // DocumentObject::getSubObjectList() with the parameters subname and subsizes
    auto docSubObjsWithSubSizes {std::vector<DocumentObject*>()};
    // A vector of DocumentObjects used to store the result of the call to
    // DocumentObject::getSubObjectList() with the flatten parameter set to true
    auto docSubObjsFlatten {std::vector<DocumentObject*>()};

    // Act
    docSubObjsNoSubName = part->getSubObjectList(nullptr);
    docSubObjsWithSubName = part->getSubObjectList(subName.c_str());
    docSubObjsWithSubSizes = part->getSubObjectList(subName.c_str(), &sizesNoFlatten);
    docSubObjsFlatten = part->getSubObjectList(subName.c_str(), &sizesFlatten, true);

    // Assert

    // If DocumentObject::getSubObjectList() is called without giving the subname argument it
    // returns a vector with only one entry, corresponding to the object that called the method
    EXPECT_EQ(docSubObjsNoSubName.size(), 1);
    EXPECT_EQ(docSubObjsNoSubName[0]->getID(), _doc->getObject(partName)->getID());

    // If DocumentObject::getSubObjectList() is called with the subname argument it returns a vector
    // with one entry for each sub object in the subname plus one entry with the object that called
    // the method
    EXPECT_EQ(docSubObjsWithSubName.size(), 3);
    EXPECT_EQ(docSubObjsWithSubName[0]->getID(), _doc->getObject(partName)->getID());
    EXPECT_EQ(docSubObjsWithSubName[1]->getID(), _doc->getObject(fuseName)->getID());
    EXPECT_EQ(docSubObjsWithSubName[2]->getID(), _doc->getObject(boxName)->getID());

    // If DocumentObject::getSubObjectList() is called with the subsizes argument it returns the
    // same vector of the previous case and the subsizes argument stores the positions in the
    // subname string corresponding to start of the sub objects names.
    // The position takes into account also the '.' in the subname
    EXPECT_EQ(docSubObjsWithSubSizes.size(), 3);
    EXPECT_EQ(docSubObjsWithSubSizes[0]->getID(), _doc->getObject(partName)->getID());
    EXPECT_EQ(docSubObjsWithSubSizes[1]->getID(), _doc->getObject(fuseName)->getID());
    EXPECT_EQ(docSubObjsWithSubSizes[2]->getID(), _doc->getObject(boxName)->getID());
    EXPECT_EQ(sizesNoFlatten.size(), 3);
    EXPECT_EQ(sizesNoFlatten[0], 0);
    EXPECT_EQ(sizesNoFlatten[1], strlen(fuseName) + 1);
    EXPECT_EQ(sizesNoFlatten[2], strlen(fuseName) + strlen(boxName) + 2);

    // If DocumentObject::getSubObjectList() is called with the flattened argument set to true, it
    // returns a vector with all the sub objects in the subname that don't belong to the same
    // App::GeoFeatureGroupExtension object, plus one entry with the object that called the method
    EXPECT_EQ(docSubObjsFlatten.size(), 2);
    EXPECT_EQ(docSubObjsFlatten[0]->getID(), _doc->getObject(partName)->getID());
    EXPECT_EQ(docSubObjsFlatten[1]->getID(), _doc->getObject(boxName)->getID());
    EXPECT_EQ(sizesFlatten.size(), 2);
    EXPECT_EQ(sizesFlatten[0], 0);
    EXPECT_EQ(sizesFlatten[1], strlen(fuseName) + strlen(boxName) + 2);
}

// NOLINTEND(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)
