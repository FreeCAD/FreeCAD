// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include <gtest/gtest.h>

#include <QMetaType>

#include <App/Application.h>
#include <Gui/MetaTypes.h>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Model.h>
#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/App/ModelUuids.h>

// clang-format off

class TestModelProperties : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
  }
};

TEST_F(TestModelProperties, TestEmpty)
{
    auto prop = Materials::ModelProperty();
    EXPECT_TRUE(prop.getName().empty());
    EXPECT_TRUE(prop.getPropertyType().empty());
    EXPECT_TRUE(prop.getUnits().empty());
    EXPECT_TRUE(prop.getURL().empty());
    EXPECT_TRUE(prop.getDescription().empty());
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);
}

TEST_F(TestModelProperties, TestBasic)
{
    auto prop = Materials::ModelProperty("1",
                           "2",
                           "3",
                           "4",
                           "5",
                           "6");
    EXPECT_EQ(prop.getName(), "1");
    EXPECT_EQ(prop.getDisplayName(), "2");
    EXPECT_EQ(prop.getPropertyType(), "3");
    EXPECT_EQ(prop.getUnits(), "4");
    EXPECT_EQ(prop.getURL(), "5");
    EXPECT_EQ(prop.getDescription(), "6");
    EXPECT_TRUE(prop.getInheritance().empty());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop.setInheritance("12345");
    EXPECT_EQ(prop.getInheritance(), "12345");
    EXPECT_TRUE(prop.isInherited());
}

TEST_F(TestModelProperties, TestAddColumns)
{
    auto prop = Materials::ModelProperty("1",
                           "2",
                           "3",
                           "4",
                           "5",
                           "6");
    auto prop1 = Materials::ModelProperty("10",
                           "9",
                           "8",
                           "7",
                           "6",
                           "5");
    auto prop2 = Materials::ModelProperty("a",
                           "b",
                           "c",
                           "d",
                           "e",
                           "f");

    EXPECT_EQ(prop.columns(), 0);
    prop.addColumn(prop1);
    EXPECT_EQ(prop.columns(), 1);
    prop.addColumn(prop2);
    EXPECT_EQ(prop.columns(), 2);

    auto columns = prop.getColumns();
    auto entry1 = columns.at(0);
    EXPECT_EQ(entry1.getName(), "10");
    EXPECT_EQ(entry1.getDisplayName(), "9");
    EXPECT_EQ(entry1.getPropertyType(), "8");
    EXPECT_EQ(entry1.getUnits(), "7");
    EXPECT_EQ(entry1.getURL(), "6");
    EXPECT_EQ(entry1.getDescription(), "5");
    EXPECT_TRUE(entry1.getInheritance().empty());
    EXPECT_FALSE(entry1.isInherited());
    EXPECT_EQ(entry1.columns(), 0);

    auto entry2 = columns.at(1);
    EXPECT_EQ(entry2.getName(), "a");
    EXPECT_EQ(entry2.getDisplayName(), "b");
    EXPECT_EQ(entry2.getPropertyType(), "c");
    EXPECT_EQ(entry2.getUnits(), "d");
    EXPECT_EQ(entry2.getURL(), "e");
    EXPECT_EQ(entry2.getDescription(), "f");
    EXPECT_TRUE(entry2.getInheritance().empty());
    EXPECT_FALSE(entry2.isInherited());
    EXPECT_EQ(entry2.columns(), 0);

}

// clang-format on
