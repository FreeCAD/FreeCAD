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

#include <Base/ByteBuffer.h>

#include <cstring>
#include <string>
#include <utility>

namespace
{

std::string toString(const Base::ByteBuffer& buffer)
{
    return {reinterpret_cast<const char*>(buffer.data()), buffer.size()};
}

}  // namespace

TEST(ByteBuffer, defaultConstruction)
{
    Base::ByteBuffer buf;
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.size(), 0U);
    EXPECT_NE(buf.data(), nullptr);
    EXPECT_FALSE(buf.isBorrowed());
}

TEST(ByteBuffer, borrowIsNonOwningUntilMakeOwning)
{
    const std::string backing = std::string("abc\0def", 7);
    Base::ByteBuffer buf = Base::ByteBuffer::borrow(Base::asBytes(backing.data(), backing.size()));

    EXPECT_EQ(buf.size(), 7U);
    EXPECT_TRUE(buf.isBorrowed());
    EXPECT_EQ(std::memcmp(buf.data(), backing.data(), backing.size()), 0);

    buf.makeOwning();
    EXPECT_FALSE(buf.isBorrowed());
    EXPECT_EQ(buf.size(), 7U);
    EXPECT_EQ(std::memcmp(buf.data(), backing.data(), backing.size()), 0);
}

TEST(ByteBuffer, copyOwnsAndPreservesEmbeddedNul)
{
    const std::string backing = std::string("abc\0def", 7);
    Base::ByteBuffer buf = Base::ByteBuffer::copy(Base::asBytes(backing.data(), backing.size()));

    EXPECT_FALSE(buf.isBorrowed());
    EXPECT_EQ(buf.size(), 7U);
    EXPECT_EQ(std::memcmp(buf.data(), backing.data(), backing.size()), 0);
}

TEST(ByteBuffer, copyOnWriteDetachOnMutation)
{
    Base::ByteBuffer a = Base::ByteBuffer::copy(Base::asBytes("abc"));
    Base::ByteBuffer b = a;  // shares backing storage

    EXPECT_EQ(a, b);

    b.append(Base::asBytes("d"));
    EXPECT_NE(a, b);

    EXPECT_EQ(a.size(), 3U);
    EXPECT_EQ(toString(a), "abc");
    EXPECT_EQ(toString(b), "abcd");
}

TEST(ByteBuffer, moveLeavesSourceEmpty)
{
    Base::ByteBuffer source = Base::ByteBuffer::copy(Base::asBytes("abc"));
    Base::ByteBuffer moved = std::move(source);

    EXPECT_EQ(toString(moved), "abc");
    EXPECT_TRUE(source.empty());
    EXPECT_FALSE(source.isBorrowed());

    Base::ByteBuffer assigned;
    assigned = std::move(moved);

    EXPECT_EQ(toString(assigned), "abc");
    EXPECT_TRUE(moved.empty());
    EXPECT_FALSE(moved.isBorrowed());
}

TEST(ByteBuffer, movingBorrowedBufferPreservesViewAndClearsSource)
{
    const std::string backing = "abc";
    Base::ByteBuffer source = Base::ByteBuffer::borrow(Base::asBytes(backing));
    Base::ByteBuffer moved = std::move(source);

    EXPECT_TRUE(moved.isBorrowed());
    EXPECT_EQ(toString(moved), backing);
    EXPECT_TRUE(source.empty());
    EXPECT_FALSE(source.isBorrowed());
}

TEST(ByteBuffer, moveAssignmentHandlesBorrowedAlias)
{
    Base::ByteBuffer destination = Base::ByteBuffer::copy(Base::asBytes("abcdef"));
    Base::ByteBuffer source = Base::ByteBuffer::borrow(destination.view().subspan(1, 3));

    destination = std::move(source);

    EXPECT_EQ(toString(destination), "bcd");
    EXPECT_TRUE(source.empty());
}

TEST(ByteBuffer, appendOwnView)
{
    const std::string backing(4096, 'x');
    Base::ByteBuffer buf = Base::ByteBuffer::copy(Base::asBytes(backing.data(), backing.size()));

    buf.append(buf.view());

    EXPECT_EQ(buf.size(), backing.size() * 2U);
    EXPECT_EQ(toString(buf).substr(0, backing.size()), backing);
    EXPECT_EQ(toString(buf).substr(backing.size(), backing.size()), backing);
}
