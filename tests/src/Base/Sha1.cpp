// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD project association AISBL
// SPDX-FileNotice: Part of the FreeCAD project.
/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <gtest/gtest.h>

#include <Base/Sha1.h>

#include <cstddef>
#include <string>
#include <string_view>

TEST(Sha1, EmptyString)
{
    EXPECT_EQ(
        Base::sha1HexDigest(Base::asBytes(std::string_view {})),
        "da39a3ee5e6b4b0d3255bfef95601890afd80709"
    );
}

TEST(Sha1, Abc)
{
    EXPECT_EQ(
        Base::sha1HexDigest(Base::asBytes(std::string_view {"abc"})),
        "a9993e364706816aba3e25717850c26c9cd0d89d"
    );
}

TEST(Sha1, PaddingBoundaries)
{
    struct TestCase
    {
        std::size_t size;
        const char* expected;
    };

    constexpr TestCase cases[] = {
        {55, "c1c8bbdc22796e28c0e15163d20899b65621d65a"},
        {56, "c2db330f6083854c99d4b5bfb6e8f29f201be699"},
        {64, "0098ba824b5c16427bd7a1122a5a442a25ec644d"},
        {65, "11655326c708d70319be2610e8a57d9a5b959d3b"},
    };

    for (const auto& testCase : cases) {
        const std::string input(testCase.size, 'a');
        EXPECT_EQ(Base::sha1HexDigest(Base::asBytes(input.data(), input.size())), testCase.expected);
    }
}
