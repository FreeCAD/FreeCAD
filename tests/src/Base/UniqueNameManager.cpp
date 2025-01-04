/***************************************************************************
 *   Copyright (c) 2024 Kevin Martin <kpmartin@papertrail.ca>              *
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
 ***************************************************************************/
#include <gtest/gtest.h>
#include <string>
#include <Base/UniqueNameManager.h>

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)
TEST(UniqueNameManager, TestUniqueName1)
{
    EXPECT_EQ(Base::UniqueNameManager().makeUniqueName("Body"), "Body");
}

TEST(UniqueNameManager, TestUniqueName2)
{
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    EXPECT_EQ(manager.makeUniqueName("Body", 1), "Body1");
}

TEST(UniqueNameManager, TestUniqueName3)
{
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    EXPECT_EQ(manager.makeUniqueName("Body", 3), "Body001");
}

TEST(UniqueNameManager, TestUniqueName4)
{
    Base::UniqueNameManager manager;
    manager.addExactName("Body001");
    EXPECT_EQ(manager.makeUniqueName("Body", 3), "Body002");
}

TEST(UniqueNameManager, TestUniqueName5)
{
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    manager.addExactName("Body001");
    EXPECT_EQ(manager.makeUniqueName("Body", 3), "Body002");
}

TEST(UniqueNameManager, TestUniqueName6)
{
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    manager.addExactName("Body001");
    EXPECT_EQ(manager.makeUniqueName("Body001", 3), "Body002");
}

TEST(UniqueNameManager, TestUniqueName7)
{
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    EXPECT_EQ(manager.makeUniqueName("Body001", 3), "Body001");
}

TEST(UniqueNameManager, TestUniqueName8)
{
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    EXPECT_EQ(manager.makeUniqueName("Body12345", 3), "Body001");
}

TEST(UniqueNameManager, Issue18504)
{
    std::string name;
    Base::UniqueNameManager manager;
    manager.addExactName("Origin007");
    manager.addExactName("Origin");
    manager.addExactName("Origin008");
    manager.addExactName("Origin010");
    manager.addExactName("Origin011");
    manager.addExactName("Origin013");
    manager.addExactName("Origin016");

    name = manager.makeUniqueName("Origin", 3);
    manager.addExactName(name);

    name = manager.makeUniqueName("Origin", 3);
    manager.addExactName(name);

    name = manager.makeUniqueName("Origin", 3);
    EXPECT_NE(name, "Origin010");
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
