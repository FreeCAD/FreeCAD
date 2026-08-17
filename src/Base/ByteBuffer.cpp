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
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Lesser General Public License for more details.                       *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "ByteBuffer.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace Base
{

namespace
{

const std::byte emptyByte {};

}  // namespace

ByteBuffer::ByteBuffer() = default;
ByteBuffer::ByteBuffer(const ByteBuffer&) = default;
ByteBuffer& ByteBuffer::operator=(const ByteBuffer&) = default;

ByteBuffer::ByteBuffer(ByteBuffer&& other) noexcept
    : ptr(other.ptr)
    , len(other.len)
    , owner(std::move(other.owner))
{
    if (owner) {
        syncFromOwner();
    }
    other.resetToEmpty();
}

ByteBuffer& ByteBuffer::operator=(ByteBuffer&& other)
{
    if (this == &other) {
        return *this;
    }

    // A borrowed view can point into this buffer's current owner. Make a
    // stable copy before releasing that owner during the assignment.
    if (!other.owner && owner && rangesOverlap(ptr, len, other.ptr, other.len)) {
        owner = std::make_shared<std::vector<std::byte>>();
        owner->assign(other.ptr, other.ptr + other.len);
        syncFromOwner();
        other.resetToEmpty();
        return *this;
    }

    owner = std::move(other.owner);
    ptr = other.ptr;
    len = other.len;
    if (owner) {
        syncFromOwner();
    }
    other.resetToEmpty();
    return *this;
}

ByteBuffer::~ByteBuffer() = default;

ByteBuffer ByteBuffer::copy(BytesView bytes)
{
    ByteBuffer out;
    if (!bytes.empty()) {
        out.owner = std::make_shared<std::vector<std::byte>>(bytes.begin(), bytes.end());
        out.ptr = out.owner->data();
        out.len = out.owner->size();
    }
    return out;
}

ByteBuffer ByteBuffer::borrow(BytesView bytes)
{
    ByteBuffer out;
    if (!bytes.empty()) {
        out.ptr = bytes.data();
        out.len = bytes.size();
    }
    return out;
}

BytesView ByteBuffer::view() const
{
    if (len == 0U) {
        return {};
    }
    return {ptr, len};
}

const std::byte* ByteBuffer::data() const
{
    return len == 0U ? &emptyByte : ptr;
}

ByteBuffer::size_type ByteBuffer::size() const
{
    return len;
}

bool ByteBuffer::empty() const
{
    return len == 0U;
}

bool ByteBuffer::isBorrowed() const
{
    return (len != 0U) && !owner;
}

void ByteBuffer::makeOwning()
{
    ensureOwningForMutation(0U);
}

void ByteBuffer::clear()
{
    if (owner && owner.use_count() == 1) {
        owner->clear();
        ptr = nullptr;
        len = 0U;
        return;
    }
    owner.reset();
    ptr = nullptr;
    len = 0U;
}

void ByteBuffer::reserve(size_type capacity)
{
    ensureOwningForMutation(capacity);
    owner->reserve(capacity);
    syncFromOwner();
}

void ByteBuffer::resize(size_type newSize, std::byte fill)
{
    ensureOwningForMutation(newSize);
    owner->resize(newSize, fill);
    syncFromOwner();
}

void ByteBuffer::append(BytesView bytes)
{
    if (bytes.empty()) {
        return;
    }

    if (bytes.size() > std::numeric_limits<size_type>::max() - len) {
        throw std::length_error("ByteBuffer append exceeds maximum size");
    }

    // A view may refer to this buffer. Keep the source alive if the
    // mutation below detaches or reallocates the backing vector.
    std::vector<std::byte> sourceCopy;
    const std::byte* source = bytes.data();
    if (rangesOverlap(ptr, len, source, bytes.size())) {
        sourceCopy.assign(source, source + bytes.size());
        source = sourceCopy.data();
    }

    ensureOwningForMutation(len + bytes.size());
    owner->insert(owner->end(), source, source + bytes.size());
    syncFromOwner();
}

bool operator==(const ByteBuffer& a, const ByteBuffer& b)
{
    if (a.len != b.len) {
        return false;
    }
    if (a.len == 0U) {
        return true;
    }
    return std::memcmp(a.ptr, b.ptr, a.len) == 0;
}

bool operator!=(const ByteBuffer& a, const ByteBuffer& b)
{
    return !(a == b);
}

int ByteBuffer::compare(const ByteBuffer& other) const
{
    const size_type common = std::min(len, other.len);
    if (common != 0U) {
        const int cmp = std::memcmp(ptr, other.ptr, common);
        if (cmp != 0) {
            return cmp;
        }
    }
    if (len < other.len) {
        return -1;
    }
    if (len > other.len) {
        return 1;
    }
    return 0;
}

bool ByteBuffer::operator<(const ByteBuffer& other) const
{
    return compare(other) < 0;
}

bool ByteBuffer::rangesOverlap(
    const std::byte* first,
    size_type firstSize,
    const std::byte* second,
    size_type secondSize
)
{
    if (firstSize == 0U || secondSize == 0U) {
        return false;
    }

    const auto firstAddress = reinterpret_cast<std::uintptr_t>(first);
    const auto secondAddress = reinterpret_cast<std::uintptr_t>(second);
    if (firstAddress <= secondAddress) {
        return secondAddress - firstAddress < firstSize;
    }
    return firstAddress - secondAddress < secondSize;
}

void ByteBuffer::syncFromOwner()
{
    if (!owner || owner->empty()) {
        ptr = nullptr;
        len = 0U;
        return;
    }
    ptr = owner->data();
    len = owner->size();
}

void ByteBuffer::resetToEmpty() noexcept
{
    ptr = nullptr;
    len = 0U;
    owner.reset();
}

void ByteBuffer::ensureOwningForMutation(size_type minCapacity)
{
    if (!owner) {
        // Borrowed (or empty): materialize an owning copy.
        owner = std::make_shared<std::vector<std::byte>>();
        if (len != 0U) {
            owner->assign(ptr, ptr + len);
        }
    }
    else if (owner.use_count() != 1) {
        // Detach (copy-on-write).
        owner = std::make_shared<std::vector<std::byte>>(*owner);
    }

    if (minCapacity > owner->capacity()) {
        owner->reserve(minCapacity);
    }
    syncFromOwner();
}

}  // namespace Base
