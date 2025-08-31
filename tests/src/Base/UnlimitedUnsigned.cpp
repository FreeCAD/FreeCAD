/***************************************************************************
 *   Copyright (c) 2025 Kevin Martin <kpmartin@papertrail.ca>              *
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
#include <Base/UnlimitedUnsigned.h>

// NOLINTBEGIN(cppcoreguidelines-*,readability-*)
TEST(UnlimitedUnsigned, Basics)
{
    // Check simple addition with carry and conversion from string
    Base::UnlimitedUnsigned one(1);
    auto nines = Base::UnlimitedUnsigned::fromString("999999999");
    EXPECT_EQ(nines + one, Base::UnlimitedUnsigned::fromString("1000000000"));
}
TEST(UnlimitedUnsigned, ToString)
{
    // Check toString on simple addition result
    Base::UnlimitedUnsigned one(1);
    auto nines = Base::UnlimitedUnsigned::fromString("999999999");
    EXPECT_EQ((nines + one).toString(), "1000000000");
}
TEST(UnlimitedUnsigned, TestSubtraction1)
{
    // Check subtraction and comparison with byte-sized number
    EXPECT_EQ(Base::UnlimitedUnsigned::fromString("6842357951")
                  - Base::UnlimitedUnsigned::fromString("6842357948"),
              3);
}
TEST(UnlimitedUnsigned, TestSubtraction2)
{
    // Check subtraction and comparison
    EXPECT_EQ(Base::UnlimitedUnsigned::fromString("6842357951")
                  - Base::UnlimitedUnsigned::fromString("6000000000"),
              Base::UnlimitedUnsigned::fromString("842357951"));
}

// NOLINTEND(cppcoreguidelines-*,readability-*)
