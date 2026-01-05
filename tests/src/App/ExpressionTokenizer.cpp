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

#include <App/ExpressionTokenizer.h>

TEST(ExpressionTokenizer, ReturnsFullPrefixAtEnd)
{
    App::ExpressionTokenizer tokenizer;
    const std::string prefix = "Box.Length";
    const std::string completion = tokenizer.perform(prefix, /*posBytes*/ 10);

    EXPECT_EQ(completion, "Box.Length");

    int start = -1;
    int end = -1;
    tokenizer.getPrefixRange(start, end);
    EXPECT_EQ(start, 0);
    EXPECT_EQ(end, 10);
}

TEST(ExpressionTokenizer, TruncatesToSeparatorWhenCursorAtTokenStart)
{
    App::ExpressionTokenizer tokenizer;
    const std::string prefix = "Box.Length";
    const std::string completion = tokenizer.perform(prefix, /*posBytes*/ 4);  // "Box.|Length"

    EXPECT_EQ(completion, "Box.");

    int start = -1;
    int end = -1;
    tokenizer.getPrefixRange(start, end);
    EXPECT_EQ(start, 0);
    EXPECT_EQ(end, 4);
}

TEST(ExpressionTokenizer, HandlesUtf8AtEndUsingByteCursorPositions)
{
    App::ExpressionTokenizer tokenizer;
    // Use UTF-8 byte sequence directly (C++20 `u8""` is `char8_t[]`).
    const std::string prefix = "\xCE\xB1.\xCE\xB2";
    const std::string completion = tokenizer.perform(prefix, /*posBytes*/ prefix.size());

    EXPECT_EQ(completion, prefix);

    int start = -1;
    int end = -1;
    tokenizer.getPrefixRange(start, end);
    EXPECT_EQ(start, 0);
    EXPECT_EQ(end, static_cast<int>(prefix.size()));
}

TEST(ExpressionTokenizer, ReturnsEmptyOnTrailingSpace)
{
    App::ExpressionTokenizer tokenizer;
    const std::string prefix = "Box. ";
    const std::string completion = tokenizer.perform(prefix, /*posBytes*/ 5);
    EXPECT_TRUE(completion.empty());
}
