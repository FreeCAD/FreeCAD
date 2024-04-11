// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"
#include <gmock/gmock.h>

#include <src/App/InitApplication.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include <App/ElementNamingUtils.h>
#include <Base/Interpreter.h>

using namespace App;
using namespace Data;

class DocumentObserverTest: public ::testing::Test
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

TEST_F(DocumentObserverTest, hasSubObject)
{
    // Arrange

    // An App::SubObjectT() object with a subname that doesn't contain a sub object name
    auto subObjTWithoutSubObj {SubObjectT()};
    // An App::SubObjectT() object with a subname that contains a sub object name
    auto subObjTWithSubObj {SubObjectT()};

    // A subname that doesn't contain a sub object name
    auto subObjTNameWithoutSubObj {"Line"};
    // A subname that contains a sub object name
    auto subObjTNameWithSubObj {std::string()};
    subObjTNameWithSubObj += subObjTNameWithoutSubObj;
    subObjTNameWithSubObj += ".Edge1";

    // A variable used to store the result of the call to the method SubObjectT::hasSubObject() by
    // the object subObjTWithoutSubObj
    auto hasNoSubObj {false};
    // A variable used to store the result of the call to the method SubObjectT::hasSubObject() by
    // the object subObjTWithSubObj
    auto hasSubObj {false};

    // Setting the subnames of the App::SubObjectT() objects defined previously
    subObjTWithoutSubObj.setSubName(subObjTNameWithoutSubObj);
    subObjTWithSubObj.setSubName(subObjTNameWithSubObj.c_str());

    // Act

    hasNoSubObj = subObjTWithoutSubObj.hasSubObject();
    hasSubObj = subObjTWithSubObj.hasSubObject();

    // Assert

    // The subname of subObjTWithoutSubObj doesn't contain the name of a sub object, therefor
    // hasNoSubObj should be false
    EXPECT_FALSE(hasNoSubObj);

    // The subname of subObjTWithSubObj doesn't contain the name of a sub object, therefor hasSubObj
    // should be true
    EXPECT_TRUE(hasSubObj);
}

TEST_F(DocumentObserverTest, hasSubElement)
{
    // Arrange

    // An App::SubObjectT() object with a subname that doesn't contain a sub element name
    auto subObjTWithoutSubEl {SubObjectT()};
    // An App::SubObjectT() object with a subname that contains a sub element name
    auto subObjTWithSubEl {SubObjectT()};

    // A subname that doesn't contain a sub element name
    auto subObjTNameWithoutSubEl {"Sketch."};
    // A subname that contains a sub element name
    auto subObjTNameWithSubEl {std::string()};
    subObjTNameWithSubEl += subObjTNameWithoutSubEl;
    subObjTNameWithSubEl += ELEMENT_MAP_PREFIX;
    subObjTNameWithSubEl += "e1.ExternalEdge1";

    // A variable used to store the result of the call to the method SubObjectT::hasSubElement() by
    // the object subObjTWithoutSubEl
    auto hasNoSubEl {false};
    // A variable used to store the result of the call to the method SubObjectT::hasSubElement() by
    // the object subObjTWithSubEl
    auto hasSubEl {false};

    // Setting the subnames of the App::SubObjectT() objects defined previously
    subObjTWithoutSubEl.setSubName(subObjTNameWithoutSubEl);
    subObjTWithSubEl.setSubName(subObjTNameWithSubEl.c_str());

    // Act

    hasNoSubEl = subObjTWithoutSubEl.hasSubElement();
    hasSubEl = subObjTWithSubEl.hasSubElement();

    // Assert

    // The subname of subObjTWithoutSubEl doesn't contain the name of a sub element, therefor
    // hasNoSubEl should be false
    EXPECT_FALSE(hasNoSubEl);

    // The subname of subObjTWithSubEl doesn't contain the name of a sub element, therefor hasSubEl
    // should be true
    EXPECT_TRUE(hasSubEl);
}

TEST_F(DocumentObserverTest, normalize)
{
    // Arrange

    // A Part::Box added to the document
    auto box {_doc->addObject("Part::Box")};
    // The name of the Part::Box added to the document
    auto boxName {box->getNameInDocument()};
    // The name of a Part::Cylinder added to the document
    auto cylName {_doc->addObject("Part::Cylinder")->getNameInDocument()};
    // An App::Part added to the document
    auto part {_doc->addObject("App::Part")};
    // The name of the App::Part added to the document
    auto partName {part->getNameInDocument()};
    // An App::LinkGroup added to the document
    auto lGrp {_doc->addObject("App::LinkGroup")};
    // The name of the App::LinkGroup added to the document
    auto lGrpName {lGrp->getNameInDocument()};
    // The name of the object used as argument for various calls of the constructors of
    // App::SubObjectT objects
    auto subName {std::string()};
    // An helper string used to compose the argument of some calls of
    // Base::Interpreter().runString()
    auto cmd {std::string()};

    // Performing a fusion to create an object that will be added to the App::Part object and
    // linked inside the App::LinkGroup object
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

    // Adding the fusion to the App::Part object to test the differences between calling
    // SubObjectT::normalize() with the options argument set to
    // SubObjectT::NormalizeOption::NoFlatten or set to other values
    cmd = "App.ActiveDocument.getObject(\"";
    cmd += partName;
    cmd += "\").addObject(App.ActiveDocument.getObject(\"";
    cmd += fuseName;
    cmd += "\"))";
    Base::Interpreter().runString(cmd.c_str());
    Base::Interpreter().runString("App.ActiveDocument.recompute()");

    // Adding the fusion to the App::LinkGroup object to test SubObjectT::normalize() with the
    // options argument set to SubObjectT::NormalizeOption::ConvertIndex
    cmd = "App.ActiveDocument.getObject(\"";
    cmd += lGrpName;
    cmd += "\").setLink(App.ActiveDocument.getObject(\"";
    cmd += partName;
    cmd += "\"))";
    Base::Interpreter().runString(cmd.c_str());
    Base::Interpreter().runString("App.ActiveDocument.recompute()");

    // Defining the name of the object that will be used as argument for various calls of the
    // constructors of App::SubObjectT objects
    subName = partName;
    subName += ".";
    subName += fuseName;
    subName += ".";
    subName += boxName;
    subName += ".Edge1";

    // An empty App::SubObjectT object
    auto subObjTEmpty {SubObjectT()};
    // An App::SubObjectT object without sub objects
    auto subObjTWithoutSubObj {SubObjectT(box, boxName)};
    // An App::SubObjectT object with sub objects
    auto subObjTWithSubObj {SubObjectT(lGrp, subName.c_str())};
    // An App::SubObjectT object used to test SubObjectT::normalize() with the option argument set
    // to SubObjectT::NormalizeOption::NoElement
    auto subObjTWithoutEl {SubObjectT(lGrp, subName.c_str())};
    // An App::SubObjectT object used to test SubObjectT::normalize() with the option argument set
    // to SubObjectT::NormalizeOption::NoFlatten
    auto subObjTNoFlatten {SubObjectT(lGrp, subName.c_str())};
    // An App::SubObjectT object used to test SubObjectT::normalize() with the option argument set
    // to SubObjectT::NormalizeOption::KeepSubName
    auto subObjTKeepSubName {SubObjectT(lGrp, subName.replace(0, strlen(partName), "0").c_str())};
    // An App::SubObjectT object used to test SubObjectT::normalize() with the option argument set
    // to SubObjectT::NormalizeOption::ConvertIndex
    auto subObjTConvertIndex {SubObjectT(lGrp, subName.c_str())};

    // A bool variable used to store the result of subObjTEmpty.normalize()
    auto normalizeEmpty {false};
    // A bool variable used to store the result of subObjTWithoutSubObj.normalize()
    auto normalizeWithoutSubObj {false};
    // A bool variable used to store the result of subObjTWithSubObj.normalize()
    auto normalizeWithSubObj {false};
    // A bool variable used to store the result of subObjTWithoutEl.normalize()
    auto normalizeWithoutEl {false};
    // A bool variable used to store the result of subObjTNoFlatten.normalize()
    auto normalizeNoFlatten {false};
    // A bool variable used to store the result of subObjTKeepSubName.normalize()
    auto normalizeKeepSubName {false};
    // A bool variable used to store the result of subObjTConvertIndex.normalize()
    auto normalizeConvertIndex {false};

    // Act

    normalizeEmpty = subObjTEmpty.normalize();
    normalizeWithoutSubObj = subObjTWithoutSubObj.normalize();
    normalizeWithSubObj = subObjTWithSubObj.normalize();
    normalizeWithoutEl = subObjTWithoutEl.normalize(SubObjectT::NormalizeOption::NoElement);
    normalizeNoFlatten = subObjTNoFlatten.normalize(SubObjectT::NormalizeOption::NoFlatten);
    normalizeKeepSubName = subObjTKeepSubName.normalize(SubObjectT::NormalizeOption::KeepSubName);
    normalizeConvertIndex =
        subObjTConvertIndex.normalize(SubObjectT::NormalizeOption::ConvertIndex);

    // Assert

    // In this case calling SubObjectT::normalize() doesn't have effect as subObjTEmpty has been
    // initialized with an empty constructor
    EXPECT_FALSE(normalizeEmpty);

    // In this case calling SubObjectT::normalize() doesn't have effect as subObjTWithoutSubObj
    // hasn't any sub objects
    EXPECT_FALSE(normalizeWithoutSubObj);

    // In this case calling SubObjectT::normalize() changes subObjTWithSubObj subname removing the
    // "Fusion." part of the of the original subname
    EXPECT_TRUE(normalizeWithSubObj);

    // In this case calling SubObjectT::normalize() changes subObjTWithoutEl subname removing the
    // "Fusion." and "Edge1" parts of the of the original subname
    EXPECT_TRUE(normalizeWithoutEl);

    // In this case calling SubObjectT::normalize() doesn't have effect as neither the
    // DocumentObject referenced nor the subname of subObjTNoFlatten are changed
    EXPECT_FALSE(normalizeNoFlatten);

    // In this case calling SubObjectT::normalize() changes subObjTKeepSubName subname removing the
    // "Fusion." part of the of the original subname
    EXPECT_TRUE(normalizeKeepSubName);

    // In this case calling SubObjectT::normalize() changes subObjTConvertIndex subname replacing
    // the "0" part of the of the original subname with "App__Part" as lGrp object at position 0 is
    // the DocumentObject with name "App__Part" and removing the "Fusion" part
    EXPECT_TRUE(normalizeConvertIndex);
}

TEST_F(DocumentObserverTest, normalized)
{
    // Arrange

    // A Part::Box added to the document
    auto box {_doc->addObject("Part::Box")};
    // The name of the Part::Box added to the document
    auto boxName {box->getNameInDocument()};
    // The name of a Part::Cylinder added to the document
    auto cylName {_doc->addObject("Part::Cylinder")->getNameInDocument()};
    // An App::Part added to the document
    auto part {_doc->addObject("App::Part")};
    // The name of the App::Part added to the document
    auto partName {part->getNameInDocument()};
    // An App::LinkGroup added to the document
    auto lGrp {_doc->addObject("App::LinkGroup")};
    // The name of the App::LinkGroup added to the document
    auto lGrpName {lGrp->getNameInDocument()};
    // The name of the object used as argument for various calls of the constructors of
    // App::SubObjectT objects
    auto subName {std::string()};
    // An helper string used to compose the argument of some calls of
    // Base::Interpreter().runString()
    auto cmd {std::string()};

    // Performing a fusion to create an object that will be added to the App::Part object and
    // linked inside the App::LinkGroup object
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

    // Adding the fusion to the App::Part object to test the differences between calling
    // SubObjectT::normalized() with the options argument set to
    // SubObjectT::NormalizeOption::NoFlatten or set to other values
    cmd = "App.ActiveDocument.getObject(\"";
    cmd += partName;
    cmd += "\").addObject(App.ActiveDocument.getObject(\"";
    cmd += fuseName;
    cmd += "\"))";
    Base::Interpreter().runString(cmd.c_str());
    Base::Interpreter().runString("App.ActiveDocument.recompute()");

    // Adding the fusion to the App::LinkGroup object to test SubObjectT::normalized() with the
    // options argument set to SubObjectT::NormalizeOption::ConvertIndex
    cmd = "App.ActiveDocument.getObject(\"";
    cmd += lGrpName;
    cmd += "\").setLink(App.ActiveDocument.getObject(\"";
    cmd += partName;
    cmd += "\"))";
    Base::Interpreter().runString(cmd.c_str());
    Base::Interpreter().runString("App.ActiveDocument.recompute()");

    // Defining the name of the object that will be used as argument for various calls of the
    // constructors of App::SubObjectT objects
    subName = partName;
    subName += ".";
    subName += fuseName;
    subName += ".";
    subName += boxName;
    subName += ".Edge1";

    // An empty App::SubObjectT object
    auto subObjTEmpty {SubObjectT()};
    // An App::SubObjectT object without sub objects
    auto subObjTWithoutSubObj {SubObjectT(box, boxName)};
    // An App::SubObjectT object with sub objects
    auto subObjTWithSubObj {SubObjectT(lGrp, subName.c_str())};
    // An App::SubObjectT object used to test SubObjectT::normalized() with the option argument set
    // to SubObjectT::NormalizeOption::NoElement
    auto subObjTWithoutEl {SubObjectT(lGrp, subName.c_str())};
    // An App::SubObjectT object used to test SubObjectT::normalized() with the option argument set
    // to SubObjectT::NormalizeOption::NoFlatten
    auto subObjTNoFlatten {SubObjectT(lGrp, subName.c_str())};
    // An App::SubObjectT object used to test SubObjectT::normalized() with the option argument set
    // to SubObjectT::NormalizeOption::KeepSubName
    auto subObjTKeepSubName {SubObjectT(lGrp, subName.replace(0, strlen(partName), "0").c_str())};
    // An App::SubObjectT object used to test SubObjectT::normalized() with the option argument set
    // to SubObjectT::NormalizeOption::ConvertIndex
    auto subObjTConvertIndex {SubObjectT(lGrp, subName.c_str())};

    // An App::SubObjectT object used to store the result of subObjTEmpty.normalized()
    auto subObjTEmptyNormalized {SubObjectT()};
    // An App::SubObjectT object used to store the result of subObjTWithoutSubObj.normalized()
    auto subObjTWithoutSubObjNormalized {SubObjectT()};
    // An App::SubObjectT object used to store the result of subObjTWithSubObj.normalized()
    auto subObjTWithSubObjNormalized {SubObjectT()};
    // An App::SubObjectT object used to store the result of subObjTWithoutEl.normalized()
    auto subObjTWithoutElNormalized {SubObjectT()};
    // An App::SubObjectT object used to store the result of subObjTNoFlatten.normalized()
    auto subObjTNoFlattenNormalized {SubObjectT()};
    // An App::SubObjectT object used to store the result of subObjTKeepSubName.normalized()
    auto subObjTKeepSubNameNormalized {SubObjectT()};
    // An App::SubObjectT object used to store the result of subObjTConvertIndex.normalized()
    auto subObjTConvertIndexNormalized {SubObjectT()};

    // Act

    subObjTEmptyNormalized = subObjTEmpty.normalized();
    subObjTWithoutSubObjNormalized = subObjTWithoutSubObj.normalized();
    subObjTWithSubObjNormalized = subObjTWithSubObj.normalized();
    subObjTWithoutElNormalized =
        subObjTWithoutEl.normalized(SubObjectT::NormalizeOption::NoElement);
    subObjTNoFlattenNormalized =
        subObjTNoFlatten.normalized(SubObjectT::NormalizeOption::NoFlatten);
    subObjTKeepSubNameNormalized =
        subObjTKeepSubName.normalized(SubObjectT::NormalizeOption::KeepSubName);
    subObjTConvertIndexNormalized =
        subObjTConvertIndex.normalized(SubObjectT::NormalizeOption::ConvertIndex);

    // Assert

    // In this case calling SubObjectT::normalized() doesn't have effect as subObjTEmpty has been
    // initialized with an empty constructor
    EXPECT_EQ(subObjTEmpty.getSubName(), subObjTEmptyNormalized.getSubName());

    // In this case calling SubObjectT::normalized() doesn't have effect as subObjTWithoutSubObj
    // hasn't any sub objects
    EXPECT_EQ(subObjTWithoutSubObj.getSubName(), subObjTWithoutSubObj.getSubName());

    // In this case calling SubObjectT::normalized() changes subObjTWithSubObj subname removing the
    // "Fusion." part of the of the original subname
    EXPECT_EQ(subObjTWithSubObjNormalized.getSubName().find(fuseName), std::string::npos);

    // In this case calling SubObjectT::normalized() changes subObjTWithoutEl subname removing the
    // "Fusion." and "Edge1" parts of the of the original subname
    EXPECT_EQ(subObjTWithoutElNormalized.getSubName().find(fuseName), std::string::npos);
    EXPECT_EQ(subObjTWithoutElNormalized.getSubName().find("Edge1"), std::string::npos);

    // In this case calling SubObjectT::normalized() doesn't have effect as neither the
    // DocumentObject referenced nor the subname of subObjTNoFlatten are changed
    EXPECT_EQ(subObjTNoFlatten.getSubName(), subObjTNoFlattenNormalized.getSubName());

    // In this case calling SubObjectT::normalized() changes subObjTKeepSubName subname removing the
    // "Fusion." part of the of the original subname
    EXPECT_EQ(subObjTKeepSubNameNormalized.getSubName().find(fuseName), std::string::npos);

    // In this case calling SubObjectT::normalized() changes subObjTConvertIndex subname replacing
    // the "0" part of the of the original subname with "App__Part" as lGrp object at position 0 is
    // the DocumentObject with name "App__Part" and removing the "Fusion" part
    EXPECT_EQ(subObjTConvertIndexNormalized.getSubName().find("0"), std::string::npos);
    EXPECT_EQ(subObjTConvertIndexNormalized.getSubName().find(fuseName), std::string::npos);
    EXPECT_NE(subObjTConvertIndexNormalized.getSubName().find(partName), std::string::npos);
}
