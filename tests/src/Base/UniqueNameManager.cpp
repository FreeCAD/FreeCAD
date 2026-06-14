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
TEST(UniqueNameManager, NonconflictingBaseNameReturnsSameName)
{
    // Check that a model name that is a base name that does not conflict with
    // any existing name returns the base name
    EXPECT_EQ(Base::UniqueNameManager().makeUniqueName("Body"), "Body");
}

TEST(UniqueNameManager, NonconflictingUniqueNameReturnsBaseName)
{
    // Check that a model name that is a unique name whose base name does not conflict with
    // any existing name returns the base name
    EXPECT_EQ(Base::UniqueNameManager().makeUniqueName("Body123"), "Body");
}

TEST(UniqueNameManager, ManyDigitsInModelNameIgnored)
{
    // Check that the number of digits in the model name does not affect the number
    // of digits in the returned name.
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    EXPECT_EQ(manager.makeUniqueName("Body12345", 3), "Body001");
}

TEST(UniqueNameManager, ConflictingBaseNameReturnsUniqueName)
{
    // Check that a model name that conflicts with an existing name returns a unique form of
    // the base name
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    EXPECT_EQ(manager.makeUniqueName("Body", 1), "Body1");
}

TEST(UniqueNameManager, ConflictingUniqueNameReturnsNewUniqueName)
{
    // Check that a given unique name already in the collection causes the return of
    // a new unique name.
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    manager.addExactName("Body001");
    EXPECT_EQ(manager.makeUniqueName("Body001", 3), "Body002");
}

TEST(UniqueNameManager, VerifyUniqueDigitCount)
{
    // Check that when a unique name is generated it contains at least the specified number
    // of inserted digits
    Base::UniqueNameManager manager;
    manager.addExactName("Body");
    EXPECT_EQ(manager.makeUniqueName("Body", 3), "Body001");
}

TEST(UniqueNameManager, UniqueNameLargerThanAnyName)
{
    // Check that the name returned is larger (using digits) that any wxisting name
    // even if the given model name is not itself in the collection
    Base::UniqueNameManager manager;
    manager.addExactName("Body001");
    EXPECT_EQ(manager.makeUniqueName("Body", 3), "Body002");
}

TEST(UniqueNameManager, Issue18504)
{
    // Check that the management of spans of assigned unique digit values is
    // correct
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

TEST(UniqueNameManager, UniqueNameWithManyDigits)
{
    // Check that names with many digits (value larger than max unsigned long) work
    Base::UniqueNameManager manager;
    manager.addExactName("Compound006002002002002");
    EXPECT_EQ(manager.makeUniqueName("Compound", 3), "Compound006002002002003");
}
TEST(UniqueNameManager, UniqueNameWith9NDigits)
{
    // Check that names with a multiple of 9 digits work. The manager chunks nine digits at a time
    // so this boundary condition needs a test.
    Base::UniqueNameManager manager;
    manager.addExactName("Compound123456789");
    EXPECT_EQ(manager.makeUniqueName("Compound", 3), "Compound123456790");
}
// NOLINTEND(cppcoreguidelines-*,readability-*)
