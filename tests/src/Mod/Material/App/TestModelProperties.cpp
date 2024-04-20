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

#include "gtest/gtest.h"

#include <Mod/Material/App/PreCompiled.h>
#ifndef _PreComp_
#endif

#include <QMetaType>
#include <QString>

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
    EXPECT_TRUE(prop.getName().isNull());
    EXPECT_TRUE(prop.getPropertyType().isNull());
    EXPECT_TRUE(prop.getUnits().isNull());
    EXPECT_TRUE(prop.getURL().isNull());
    EXPECT_TRUE(prop.getDescription().isNull());
    EXPECT_TRUE(prop.getInheritance().isNull());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);
}

TEST_F(TestModelProperties, TestBasic)
{
    auto prop = Materials::ModelProperty(QString::fromStdString("1"),
                           QString::fromStdString("2"),
                           QString::fromStdString("3"),
                           QString::fromStdString("4"),
                           QString::fromStdString("5"),
                           QString::fromStdString("6"));
    EXPECT_EQ(prop.getName(), QString::fromStdString("1"));
    EXPECT_EQ(prop.getDisplayName(), QString::fromStdString("2"));
    EXPECT_EQ(prop.getPropertyType(), QString::fromStdString("3"));
    EXPECT_EQ(prop.getUnits(), QString::fromStdString("4"));
    EXPECT_EQ(prop.getURL(), QString::fromStdString("5"));
    EXPECT_EQ(prop.getDescription(), QString::fromStdString("6"));
    EXPECT_TRUE(prop.getInheritance().isNull());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop.setInheritance(QString::fromStdString("12345"));
    EXPECT_EQ(prop.getInheritance(), QString::fromStdString("12345"));
    EXPECT_TRUE(prop.isInherited());
}

TEST_F(TestModelProperties, TestAddColumns)
{
    auto prop = Materials::ModelProperty(QString::fromStdString("1"),
                           QString::fromStdString("2"),
                           QString::fromStdString("3"),
                           QString::fromStdString("4"),
                           QString::fromStdString("5"),
                           QString::fromStdString("6"));
    auto prop1 = Materials::ModelProperty(QString::fromStdString("10"),
                           QString::fromStdString("9"),
                           QString::fromStdString("8"),
                           QString::fromStdString("7"),
                           QString::fromStdString("6"),
                           QString::fromStdString("5"));
    auto prop2 = Materials::ModelProperty(QString::fromStdString("a"),
                           QString::fromStdString("b"),
                           QString::fromStdString("c"),
                           QString::fromStdString("d"),
                           QString::fromStdString("e"),
                           QString::fromStdString("f"));

    EXPECT_EQ(prop.columns(), 0);
    prop.addColumn(prop1);
    EXPECT_EQ(prop.columns(), 1);
    prop.addColumn(prop2);
    EXPECT_EQ(prop.columns(), 2);

    auto columns = prop.getColumns();
    auto entry1 = columns.at(0);
    EXPECT_EQ(entry1.getName(), QString::fromStdString("10"));
    EXPECT_EQ(entry1.getDisplayName(), QString::fromStdString("9"));
    EXPECT_EQ(entry1.getPropertyType(), QString::fromStdString("8"));
    EXPECT_EQ(entry1.getUnits(), QString::fromStdString("7"));
    EXPECT_EQ(entry1.getURL(), QString::fromStdString("6"));
    EXPECT_EQ(entry1.getDescription(), QString::fromStdString("5"));
    EXPECT_TRUE(entry1.getInheritance().isNull());
    EXPECT_FALSE(entry1.isInherited());
    EXPECT_EQ(entry1.columns(), 0);

    auto entry2 = columns.at(1);
    EXPECT_EQ(entry2.getName(), QString::fromStdString("a"));
    EXPECT_EQ(entry2.getDisplayName(), QString::fromStdString("b"));
    EXPECT_EQ(entry2.getPropertyType(), QString::fromStdString("c"));
    EXPECT_EQ(entry2.getUnits(), QString::fromStdString("d"));
    EXPECT_EQ(entry2.getURL(), QString::fromStdString("e"));
    EXPECT_EQ(entry2.getDescription(), QString::fromStdString("f"));
    EXPECT_TRUE(entry2.getInheritance().isNull());
    EXPECT_FALSE(entry2.isInherited());
    EXPECT_EQ(entry2.columns(), 0);

}

// clang-format on
