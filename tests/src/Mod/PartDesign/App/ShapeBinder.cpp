// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"

#include <App/Application.h>
#include <App/Document.h>
#include "Mod/Part/App/FeaturePartBox.h"
#include "Mod/PartDesign/App/Body.h"
#include "Mod/PartDesign/App/ShapeBinder.h"

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

class ShapeBinderTest: public ::testing::Test
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
        _body = _doc->addObject<PartDesign::Body>();
        _box = _doc->addObject<Part::Box>();
        _box->Length.setValue(1);
        _box->Width.setValue(2);
        _box->Height.setValue(3);
        _box->Placement.setValue(
            Base::Placement(Base::Vector3d(), Base::Rotation(), Base::Vector3d()));  // NOLINT
        //        _body->addObject(_box); // Invalid, Part::Features can't go in a PartDesign::Body,
        //        but we can bind them.
        _binder = _doc->addObject<PartDesign::ShapeBinder>("ShapeBinderFoo");
        _subbinder = _doc->addObject<PartDesign::SubShapeBinder>("SubShapeBinderBar");
        _binder->Shape.setValue(_box->Shape.getShape());
        _subbinder->setLinks({{_box, {"Face1", "Face2"}}}, false);
        _body->addObject(_binder);
        _body->addObject(_subbinder);
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)

    App::Document* _doc = nullptr;
    std::string _docName = "";
    Part::Box* _box = nullptr;
    PartDesign::Body* _body = nullptr;
    PartDesign::ShapeBinder* _binder = nullptr;
    PartDesign::SubShapeBinder* _subbinder = nullptr;

    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
};

TEST_F(ShapeBinderTest, shapeBinderExists)
{
    // Arrange
    // Act
    auto binder = _doc->getObject("ShapeBinderFoo");
    // Assert the object is correct
    EXPECT_NE(binder, nullptr);
    // Assert the elementMap is correct
}

TEST_F(ShapeBinderTest, subShapeBinderExists)
{
    // Arrange
    // Act
    auto subbinder = _doc->getObject("SubShapeBinderBar");
    // Assert the object is correct
    EXPECT_NE(subbinder, nullptr);
    // Assert the elementMap is correct
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
