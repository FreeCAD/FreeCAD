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

#include <Base/HashUtils.h>

TEST(HashUtils, fnv1a64KnownValue)
{
    EXPECT_EQ(Base::fnv1a64(""), static_cast<std::size_t>(14695981039346656037ULL));
    EXPECT_EQ(Base::fnv1a64("hello"), static_cast<std::size_t>(0xa430d84680aabd0bULL));
}

TEST(HashUtils, hashCombineUsesBoostCompatibleFormula)
{
    constexpr std::size_t seed = 0x12345678U;
    constexpr std::size_t value = 0xabcdef01U;
    constexpr std::size_t expected = seed
        ^ (value + static_cast<std::size_t>(0x9e3779b97f4a7c15ULL) + (seed << 6U) + (seed >> 2U));

    EXPECT_EQ(Base::hashCombine(seed, value), expected);
}
