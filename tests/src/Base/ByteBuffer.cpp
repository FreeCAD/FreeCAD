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

TEST(ByteBuffer, moveAssignmentFromBorrowedAliasKeepsStorage)
{
    Base::ByteBuffer destination = Base::ByteBuffer::copy(Base::asBytes("abcdef"));
    const std::byte* storage = destination.data();
    Base::ByteBuffer source = Base::ByteBuffer::borrow(destination.view().subspan(1, 3));

    destination = std::move(source);

    EXPECT_FALSE(destination.isBorrowed());
    EXPECT_EQ(destination.data(), storage + 1);
    EXPECT_EQ(toString(destination), "bcd");

    destination.append(Base::asBytes("X"));
    EXPECT_EQ(toString(destination), "bcdX");
}

TEST(ByteBuffer, copyAssignmentFromBorrowedAlias)
{
    Base::ByteBuffer destination = Base::ByteBuffer::copy(Base::asBytes("abcdef"));
    const Base::ByteBuffer source = Base::ByteBuffer::borrow(destination.view().subspan(2, 2));

    destination = source;

    EXPECT_EQ(toString(destination), "cd");
}

TEST(ByteBuffer, makeOwningKeepsSharedStorage)
{
    Base::ByteBuffer a = Base::ByteBuffer::copy(Base::asBytes("abc"));
    Base::ByteBuffer b = a;

    b.makeOwning();

    EXPECT_EQ(a.data(), b.data());
}

TEST(ByteBuffer, appendGrowsGeometrically)
{
    Base::ByteBuffer buf;
    const std::byte* storage = nullptr;
    int reallocations = 0;
    for (int i = 0; i < 4096; ++i) {
        buf.append(Base::asBytes("x"));
        if (buf.data() != storage) {
            storage = buf.data();
            ++reallocations;
        }
    }

    EXPECT_EQ(buf.size(), 4096U);
    EXPECT_LE(reallocations, 13);
}

TEST(ByteBuffer, appendAfterSharingDetaches)
{
    Base::ByteBuffer a = Base::ByteBuffer::copy(Base::asBytes("abc"));
    a.reserve(16);
    Base::ByteBuffer b = a;

    a.append(Base::asBytes("d"));
    b.append(Base::asBytes("e"));

    EXPECT_EQ(toString(a), "abcd");
    EXPECT_EQ(toString(b), "abce");
}

TEST(ByteBuffer, clearAndResize)
{
    Base::ByteBuffer buf = Base::ByteBuffer::copy(Base::asBytes("abc"));
    buf.clear();
    EXPECT_TRUE(buf.empty());

    buf.resize(3, std::byte {'z'});
    EXPECT_EQ(toString(buf), "zzz");

    buf.resize(1);
    EXPECT_EQ(toString(buf), "z");
}
