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

#include <gtest/gtest.h>
#include "gmock/gmock.h"

#include <App/Application.h>
#include "App/Document.h"
#include <App/VarSet.h>
#include <src/App/InitApplication.h>

using ::testing::NotNull;

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

// Tests whether we can create a VarSet
TEST_F(VarSet, createVarSet)
{
    // Arrange
    const char* nameVarSet = "VarSet";

    // Act
    doc()->addObject("App::VarSet", nameVarSet);
    auto varSet = dynamic_cast<App::VarSet*>(doc()->getObject(nameVarSet));

    // Assert
    EXPECT_THAT(varSet, NotNull());
}

// Tests whether we can add a property to a VarSet
TEST_F(VarSet, addProperty)
{
    // Arrange
    const char* nameVarSet = "VarSet001";
    const long VALUE = 123;

    doc()->addObject("App::VarSet", nameVarSet);
    auto varSet = dynamic_cast<App::VarSet*>(doc()->getObject(nameVarSet));

    // Act
    auto prop = dynamic_cast<App::PropertyInteger*>(
        varSet->addDynamicProperty("App::PropertyInteger", "Variable", "Variables"));
    prop->setValue(VALUE);

    // Assert
    EXPECT_EQ(prop->getValue(), VALUE);
}

// NOLINTEND(readability-magic-numbers)
