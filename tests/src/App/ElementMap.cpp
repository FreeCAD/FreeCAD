// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "App/ElementMap.h"
#include <App/Application.h>

// NOLINTBEGIN(readability-magic-numbers)


// this is a "holder" class used for simpler testing of ElementMap in the context of a class
class LessComplexPart
{
public:
    LessComplexPart(long tag, const std::string& nameStr)
        : elementMapPtr(std::make_shared<Data::ElementMap>())
        , Tag(tag)
        , name(nameStr)
    {
        // object also have Vertexes etc and the face count varies; but that is not important
        // here since we are not testing a real model
        // the "MappedName" is left blank for now
        auto face1 = Data::IndexedName("Face", 1);
        auto face2 = Data::IndexedName("Face", 2);
        auto face3 = Data::IndexedName("Face", 3);
        auto face4 = Data::IndexedName("Face", 4);
        auto face5 = Data::IndexedName("Face", 5);
        auto face6 = Data::IndexedName("Face", 6);
        elementMapPtr->setElementName(face1, Data::MappedName(face1), Tag);
        elementMapPtr->setElementName(face2, Data::MappedName(face2), Tag);
        elementMapPtr->setElementName(face3, Data::MappedName(face3), Tag);
        elementMapPtr->setElementName(face4, Data::MappedName(face4), Tag);
        elementMapPtr->setElementName(face5, Data::MappedName(face5), Tag);
        elementMapPtr->setElementName(face6, Data::MappedName(face6), Tag);
    }

    Data::ElementMapPtr elementMapPtr;
    mutable long Tag;
    Data::MappedName name;
};

class ElementMapTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        int argc = 1;
        char* argv[] = {"FreeCAD"};
        App::Application::Config()["ExeName"] = "FreeCAD";
        App::Application::init(argc, argv);
    }

    void SetUp() override
    {
        App::GetApplication().newDocument("test", "testUser");
        sids = &_sid;
    }

    // void TearDown() override {}

    Data::ElementIDRefs _sid;
    QVector<App::StringIDRef>* sids;
};

TEST_F(ElementMapTest, defaultConstruction)
{
    // Act
    Data::ElementMap elementMap = Data::ElementMap();

    // Assert
    EXPECT_EQ(elementMap.size(), 0);
}

TEST_F(ElementMapTest, setElementNameDefaults)
{
    // Arrange
    Data::ElementMap elementMap;
    Data::IndexedName element("Edge", 1);
    Data::MappedName mappedName("TEST");

    // Act
    auto resultName = elementMap.setElementName(element, mappedName, 0, sids);
    auto mappedToElement = elementMap.find(element);

    // Assert
    EXPECT_EQ(resultName, mappedName);
    EXPECT_EQ(mappedToElement, mappedName);
}

TEST_F(ElementMapTest, setElementNameWithHashing)
{
    // Arrange
    Data::ElementMap elementMap;
    std::ostringstream ss;
    Data::IndexedName element("Edge", 1);
    Data::MappedName elementNameHolder(element);// Will get modified by the encoder
    const Data::MappedName expectedName(element);

    // Act
    elementMap.encodeElementName(
        element.getType()[0], elementNameHolder, ss, nullptr, 0, nullptr, 0);
    auto resultName = elementMap.setElementName(element, elementNameHolder, 0, sids);
    auto mappedToElement = elementMap.find(element);

    // Assert
    EXPECT_EQ(resultName, expectedName);
    EXPECT_EQ(mappedToElement, expectedName);
}

TEST_F(ElementMapTest, mimicOnePart)
{
    // Arrange
    //   pattern: new doc, create Cube
    //   for a single part, there is no "naming algo" to speak of
    std::ostringstream ss;
    auto docName = "Unnamed";
    LessComplexPart cube(1L, "Box");

    // Act
    auto children = cube.elementMapPtr->getAll();
    ss << docName << "#" << cube.name << "."
       << cube.elementMapPtr->find(Data::IndexedName("Face", 6));

    // Assert
    EXPECT_EQ(children.size(), 6);
    EXPECT_EQ(children[0].index.toString(), "Face1");
    EXPECT_EQ(children[0].name.toString(), "Face1");
    EXPECT_EQ(children[1].index.toString(), "Face2");
    EXPECT_EQ(children[1].name.toString(), "Face2");
    EXPECT_EQ(children[2].index.toString(), "Face3");
    EXPECT_EQ(children[2].name.toString(), "Face3");
    EXPECT_EQ(children[3].index.toString(), "Face4");
    EXPECT_EQ(children[3].name.toString(), "Face4");
    EXPECT_EQ(children[4].index.toString(), "Face5");
    EXPECT_EQ(children[4].name.toString(), "Face5");
    EXPECT_EQ(children[5].index.toString(), "Face6");
    EXPECT_EQ(children[5].name.toString(), "Face6");
    EXPECT_EQ(ss.str(), "Unnamed#Box.Face6");
}

TEST_F(ElementMapTest, mimicSimpleUnion)
{
    // Arrange
    //   pattern: new doc, create Cube, create Cylinder, Union of both (Cube first)
    std::ostringstream ss;
    std::ostringstream finalSs;
    char* docName = "Unnamed";

    LessComplexPart cube(1L, "Box");
    LessComplexPart cylinder(2L, "Cylinder");
    LessComplexPart unionPart(3L, "Fusion");// Union (Fusion) operation via the Part Workbench

    // we are only going to simulate one face for testing purpose
    Data::IndexedName uface3("Face", 3);
    auto PartOp = "FUS";// Part::OpCodes::Fuse;

    // Act
    //   act: simulate a union/fuse operation
    auto parent = cube.elementMapPtr->getAll()[5];
    Data::MappedName postfixHolder(std::string(Data::POSTFIX_MOD) + "2");
    unionPart.elementMapPtr->encodeElementName(
        postfixHolder[0], postfixHolder, ss, nullptr, unionPart.Tag, nullptr, unionPart.Tag);
    auto postfixStr = postfixHolder.toString() + Data::ELEMENT_MAP_PREFIX + PartOp;

    //   act: with the fuse op, name against the cube's Face6
    Data::MappedName uface3Holder(parent.index);
    // we will invoke the encoder for face 3
    unionPart.elementMapPtr->encodeElementName(
        uface3Holder[0], uface3Holder, ss, nullptr, unionPart.Tag, postfixStr.c_str(), cube.Tag);
    unionPart.elementMapPtr->setElementName(uface3, uface3Holder, unionPart.Tag, nullptr, true);

    // act: generate a full toponame string for testing  purposes
    finalSs << docName << "#" << unionPart.name;
    finalSs << ".";
    finalSs << Data::ELEMENT_MAP_PREFIX + unionPart.elementMapPtr->find(uface3).toString();
    finalSs << ".";
    finalSs << uface3;

    // Assert
    EXPECT_EQ(postfixStr, ":M2;FUS");
    EXPECT_EQ(unionPart.elementMapPtr->find(uface3).toString(), "Face6;:M2;FUS;:H1:8,F");
    EXPECT_EQ(finalSs.str(), "Unnamed#Fusion.;Face6;:M2;FUS;:H1:8,F.Face3");

    // explanation of "Fusion.;Face6;:M2;FUS;:H2:3,F.Face3" toponame
    // Note: every postfix is prefixed by semicolon
    // Note: the start/middle/end are separated by periods
    //
    // "Fusion" means that we are on the "Fusion" object.
    // "." we are done with the first part
    // ";Face6" means default inheritance comes from face 6 of the parent (which is a cube)
    // ";:M2" means that a Workbench op has happened. "M" is the "Mod" directory in the source tree?
    // ";FUS" means that a Fusion operation has happened. Notice the lack of a colon.
    // ";:H2" means the subtending object (cylinder) has a tag of 2
    // ":3" means the writing position is 3; literally how far into the current postfix we are
    // ",F" means are of type "F" which is short for "Face" of Face3 of Fusion.
    // "." we are done with the second part
    // "Face3" is the localized name
}

// NOLINTEND(readability-magic-numbers)
