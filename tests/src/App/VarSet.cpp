/****************************************************************************
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <Base/Exception.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/VarSet.h>
#include <App/PropertyVarSet.h>
#include <App/Part.h>
#include <App/DocumentObjectGroup.h>

#include <src/App/InitApplication.h>


using ::testing::NotNull;

const char* const NAME_VARSET = "VarSet";
const double SOME_VALUE = 123.0;

const char* const NAME_PROPERTY_PARENT = "NamePropertyParent";
const char* const NAME_PROPERTY_EXPOSED = "Exposed";

// NOLINTBEGIN(readability-magic-numbers)

class VarSet: public ::testing::Test
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

    App::Document* doc()
    {
        return _doc;
    }

private:
    std::string _docName;
    App::Document* _doc {};
};


class VarSetEquivalence: public VarSet
{
protected:
    void SetUp() override
    {
        VarSet::SetUp();
        _varSet1 = dynamic_cast<App::VarSet*>(doc()->addObject("App::VarSet", NAME_VARSET));
        _varSet2 = dynamic_cast<App::VarSet*>(doc()->addObject("App::VarSet", NAME_VARSET));
    }

    App::VarSet* varSet1()
    {
        return _varSet1;
    }

    App::VarSet* varSet2()
    {
        return _varSet2;
    }

private:
    App::VarSet* _varSet1 {};
    App::VarSet* _varSet2 {};
};


class VarSetInPart: public VarSet
{
protected:
    void SetUp() override
    {
        VarSet::SetUp();
        _varSet = doc()->addObject("App::VarSet", NAME_VARSET);
        _part = dynamic_cast<App::Part*>(doc()->addObject("App::Part", "Part"));
        _part->addObject(_varSet);
    }

    App::DocumentObject* varSet()
    {
        return _varSet;
    }

    App::Part* part()
    {
        return _part;
    }

private:
    std::string _docName;
    App::Document* _doc {};
    App::DocumentObject* _varSet {};
    App::Part* _part {};
};


// Tests whether we can create a VarSet
TEST_F(VarSet, createVarSet)
{
    // Arrange

    // Act
    auto varSet = doc()->addObject("App::VarSet", NAME_VARSET);

    // Assert
    EXPECT_THAT(varSet, NotNull());
}

// Tests whether we can add a property to a VarSet
TEST_F(VarSet, addProperty)
{
    // Arrange
    auto varSet = doc()->addObject("App::VarSet", NAME_VARSET);

    // Act
    auto prop = dynamic_cast<App::PropertyFloat*>(
        varSet->addDynamicProperty("App::PropertyFloat", "Variable", "Variables"));
    prop->setValue(SOME_VALUE);

    // Assert
    EXPECT_EQ(prop->getValue(), SOME_VALUE);
}

void expectExposedDisabled(App::PropertyBool* prop)
{
    EXPECT_THAT(prop, NotNull());
    if (prop) {
        EXPECT_EQ(prop->getValue(), false);
        EXPECT_TRUE(prop->testStatus(App::Property::Immutable));
        EXPECT_TRUE(prop->testStatus(App::Property::ReadOnly));
    }
}

void expectExposedEnabled(App::PropertyBool* prop, bool exposed)
{
    EXPECT_THAT(prop, NotNull());
    if (prop) {
        EXPECT_EQ(prop->getValue(), exposed);
        EXPECT_FALSE(prop->testStatus(App::Property::Immutable));
        EXPECT_FALSE(prop->testStatus(App::Property::ReadOnly));
    }
}

// Tests whether there is a property Exposed and whether it is disabled
TEST_F(VarSet, exposeDisabled)
{
    // Arrange
    auto varSet = doc()->addObject("App::VarSet", NAME_VARSET);

    // Act
    auto propExposed =
        dynamic_cast<App::PropertyBool*>(varSet->getPropertyByName(NAME_PROPERTY_EXPOSED));

    // Assert
    expectExposedDisabled(propExposed);
}

// Tests whether the property NAME_PROPERTY_EXPOSED is disabled in objects
TEST_F(VarSet, exposeDisabledInObject)
{
    // Arrange
    auto varSet = doc()->addObject("App::VarSet", NAME_VARSET);
    auto group = dynamic_cast<App::DocumentObjectGroup*>(
        doc()->addObject("App::DocumentObjectGroup", "Group"));
    group->addObject(varSet);

    // Act
    auto propExposed =
        dynamic_cast<App::PropertyBool*>(varSet->getPropertyByName(NAME_PROPERTY_EXPOSED));

    // Assert
    expectExposedDisabled(propExposed);
}

// Tests whether two VarSets are equivalent
TEST_F(VarSetEquivalence, sameProps)
{
    // Arrange

    // Act
    varSet1()->addDynamicProperty("App::PropertyFloat", "A");
    varSet1()->addDynamicProperty("App::PropertyFloat", "B");

    varSet2()->addDynamicProperty("App::PropertyFloat", "A");
    varSet2()->addDynamicProperty("App::PropertyFloat", "B");

    // Assert
    EXPECT_TRUE(varSet1()->isEquivalent(varSet2()));
}

// Tests whether two VarSets are equivalent
TEST_F(VarSetEquivalence, samePropsDifferentOrder)
{
    // Arrange

    // Act
    varSet1()->addDynamicProperty("App::PropertyFloat", "B");
    varSet1()->addDynamicProperty("App::PropertyFloat", "A");

    varSet2()->addDynamicProperty("App::PropertyFloat", "A");
    varSet2()->addDynamicProperty("App::PropertyFloat", "B");

    // Assert
    EXPECT_TRUE(varSet1()->isEquivalent(varSet2()));
}

// Tests whether two VarSets are equivalent
TEST_F(VarSetEquivalence, differentNamesProps)
{
    // Arrange

    // Act
    varSet1()->addDynamicProperty("App::PropertyFloat", "A");
    varSet1()->addDynamicProperty("App::PropertyFloat", "B");

    varSet2()->addDynamicProperty("App::PropertyFloat", "A");
    varSet2()->addDynamicProperty("App::PropertyFloat", "C");

    // Assert
    EXPECT_FALSE(varSet1()->isEquivalent(varSet2()));
}

// Tests whether two VarSets are equivalent
TEST_F(VarSetEquivalence, differenTypesProps)
{
    // Arrange

    // Act
    varSet1()->addDynamicProperty("App::PropertyInteger", "A");
    varSet1()->addDynamicProperty("App::PropertyFloat", "B");

    varSet2()->addDynamicProperty("App::PropertyFloat", "A");
    varSet2()->addDynamicProperty("App::PropertyFloat", "B");

    // Assert
    EXPECT_FALSE(varSet1()->isEquivalent(varSet2()));
}


// Tests whether the property NAME_PROPERTY_EXPOSED is enabled in an App::Part
TEST_F(VarSetInPart, exposeEnabledInPart)
{
    // Arrange

    // Act
    auto propExposed =
        dynamic_cast<App::PropertyBool*>(varSet()->getPropertyByName(NAME_PROPERTY_EXPOSED));

    // Assert
    expectExposedEnabled(propExposed, false);  // Expose is false initially
}

// Tests whether nothing is exposed when set to false
TEST_F(VarSetInPart, notExposedInitially)
{
    // Arrange
    auto propExposed =
        dynamic_cast<App::PropertyBool*>(varSet()->getPropertyByName(NAME_PROPERTY_EXPOSED));
    auto propNameParent =
        dynamic_cast<App::PropertyString*>(varSet()->getPropertyByName(NAME_PROPERTY_PARENT));
    auto nameParent = propNameParent->getValue();

    // Act

    // Assert
    expectExposedEnabled(propExposed, false);  // Expose is false by default
    EXPECT_STREQ(nameParent, "");              // empty string if it hasn't been enabled yet
    EXPECT_EQ(part()->getPropertyByName(nameParent), nullptr);
}

// Tests whether the VarSet is exposed in a Part
TEST_F(VarSetInPart, exposedInitially)
{
    // Arrange
    auto propExposed =
        dynamic_cast<App::PropertyBool*>(varSet()->getPropertyByName(NAME_PROPERTY_EXPOSED));
    // the property that contains the name of the parent property
    auto propNameParent =
        dynamic_cast<App::PropertyString*>(varSet()->getPropertyByName(NAME_PROPERTY_PARENT));
    // the name of the property in the parent that points to a VarSet
    auto namePropParent = propNameParent->getValue();

    // Act
    propExposed->setValue(true);

    // Assert
    expectExposedEnabled(propExposed, true);  // expose should now be enabled
    EXPECT_STREQ(namePropParent,
                 NAME_VARSET);  // take the name of the label of the VarSet initially

    auto* propVarSet =
        dynamic_cast<App::PropertyVarSet*>(part()->getPropertyByName(namePropParent));
    App::VarSet* varSetParent = propVarSet->getValue();

    EXPECT_THAT(propVarSet, NotNull());
    EXPECT_EQ(varSetParent,
              varSet());  // the property should point to the exposed VarSet initially.
}

// Tests the behavior if a property is already set
TEST_F(VarSetInPart, exposedParentPropertyExists)
{
    // Arrange
    auto propExposed =
        dynamic_cast<App::PropertyBool*>(varSet()->getPropertyByName(NAME_PROPERTY_EXPOSED));
    // add a property that the expose is going to create
    part()->addDynamicProperty("App::PropertyString", NAME_VARSET);

    // Act / Assert
    EXPECT_THROW(propExposed->setValue(true), Base::NameError);
}

// Tests changing a parent property to an equivalent VarSet
TEST_F(VarSetInPart, setParentPropertyEquivalent)
{
    // Arrange
    auto propExposed =
        dynamic_cast<App::PropertyBool*>(varSet()->getPropertyByName(NAME_PROPERTY_EXPOSED));
    auto propNameParent =
        dynamic_cast<App::PropertyString*>(varSet()->getPropertyByName(NAME_PROPERTY_PARENT));
    // the name of the property in the parent that points to a VarSet
    auto namePropParent = propNameParent->getValue();

    varSet()->addDynamicProperty("App::PropertyFloat", "A");
    varSet()->addDynamicProperty("App::PropertyFloat", "B");

    App::DocumentObject* varSet2 = doc()->addObject("App::VarSet", "VarSet");
    varSet2->addDynamicProperty("App::PropertyFloat", "A");
    varSet2->addDynamicProperty("App::PropertyFloat", "B");

    // Act
    propExposed->setValue(true);
    auto* propVarSet =
        dynamic_cast<App::PropertyVarSet*>(part()->getPropertyByName(namePropParent));

    // Assert
    EXPECT_NO_THROW(propVarSet->setValue(varSet2));
}

// Tests changing a parent property to a non-equivalent VarSet
TEST_F(VarSetInPart, setParentPropertyNonEquivalent)
{
    // Arrange
    auto propExposed =
        dynamic_cast<App::PropertyBool*>(varSet()->getPropertyByName(NAME_PROPERTY_EXPOSED));
    auto propNameParent =
        dynamic_cast<App::PropertyString*>(varSet()->getPropertyByName(NAME_PROPERTY_PARENT));
    // the name of the property in the parent that points to a VarSet
    auto namePropParent = propNameParent->getValue();

    varSet()->addDynamicProperty("App::PropertyFloat", "A");
    varSet()->addDynamicProperty("App::PropertyFloat", "B");

    App::DocumentObject* varSet2 = doc()->addObject("App::VarSet", "VarSet");
    varSet2->addDynamicProperty("App::PropertyFloat", "c");
    varSet2->addDynamicProperty("App::PropertyFloat", "d");

    // Act
    propExposed->setValue(true);
    auto* propVarSet =
        dynamic_cast<App::PropertyVarSet*>(part()->getPropertyByName(namePropParent));

    // Assert
    EXPECT_THROW(propVarSet->setValue(varSet2), Base::ValueError);
}


// NOLINTEND(readability-magic-numbers)
