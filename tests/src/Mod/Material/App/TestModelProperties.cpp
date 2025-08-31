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
    auto prop = Materials::ModelProperty(QStringLiteral("1"),
                           QStringLiteral("2"),
                           QStringLiteral("3"),
                           QStringLiteral("4"),
                           QStringLiteral("5"),
                           QStringLiteral("6"));
    EXPECT_EQ(prop.getName(), QStringLiteral("1"));
    EXPECT_EQ(prop.getDisplayName(), QStringLiteral("2"));
    EXPECT_EQ(prop.getPropertyType(), QStringLiteral("3"));
    EXPECT_EQ(prop.getUnits(), QStringLiteral("4"));
    EXPECT_EQ(prop.getURL(), QStringLiteral("5"));
    EXPECT_EQ(prop.getDescription(), QStringLiteral("6"));
    EXPECT_TRUE(prop.getInheritance().isNull());
    EXPECT_FALSE(prop.isInherited());
    EXPECT_EQ(prop.columns(), 0);

    prop.setInheritance(QStringLiteral("12345"));
    EXPECT_EQ(prop.getInheritance(), QStringLiteral("12345"));
    EXPECT_TRUE(prop.isInherited());
}

TEST_F(TestModelProperties, TestAddColumns)
{
    auto prop = Materials::ModelProperty(QStringLiteral("1"),
                           QStringLiteral("2"),
                           QStringLiteral("3"),
                           QStringLiteral("4"),
                           QStringLiteral("5"),
                           QStringLiteral("6"));
    auto prop1 = Materials::ModelProperty(QStringLiteral("10"),
                           QStringLiteral("9"),
                           QStringLiteral("8"),
                           QStringLiteral("7"),
                           QStringLiteral("6"),
                           QStringLiteral("5"));
    auto prop2 = Materials::ModelProperty(QStringLiteral("a"),
                           QStringLiteral("b"),
                           QStringLiteral("c"),
                           QStringLiteral("d"),
                           QStringLiteral("e"),
                           QStringLiteral("f"));

    EXPECT_EQ(prop.columns(), 0);
    prop.addColumn(prop1);
    EXPECT_EQ(prop.columns(), 1);
    prop.addColumn(prop2);
    EXPECT_EQ(prop.columns(), 2);

    auto columns = prop.getColumns();
    auto entry1 = columns.at(0);
    EXPECT_EQ(entry1.getName(), QStringLiteral("10"));
    EXPECT_EQ(entry1.getDisplayName(), QStringLiteral("9"));
    EXPECT_EQ(entry1.getPropertyType(), QStringLiteral("8"));
    EXPECT_EQ(entry1.getUnits(), QStringLiteral("7"));
    EXPECT_EQ(entry1.getURL(), QStringLiteral("6"));
    EXPECT_EQ(entry1.getDescription(), QStringLiteral("5"));
    EXPECT_TRUE(entry1.getInheritance().isNull());
    EXPECT_FALSE(entry1.isInherited());
    EXPECT_EQ(entry1.columns(), 0);

    auto entry2 = columns.at(1);
    EXPECT_EQ(entry2.getName(), QStringLiteral("a"));
    EXPECT_EQ(entry2.getDisplayName(), QStringLiteral("b"));
    EXPECT_EQ(entry2.getPropertyType(), QStringLiteral("c"));
    EXPECT_EQ(entry2.getUnits(), QStringLiteral("d"));
    EXPECT_EQ(entry2.getURL(), QStringLiteral("e"));
    EXPECT_EQ(entry2.getDescription(), QStringLiteral("f"));
    EXPECT_TRUE(entry2.getInheritance().isNull());
    EXPECT_FALSE(entry2.isInherited());
    EXPECT_EQ(entry2.columns(), 0);

}

// clang-format on
