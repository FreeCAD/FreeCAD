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

#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>

namespace Base
{

const std::byte* ByteBuffer::emptyData() noexcept
{
    static const std::byte emptyByte {};
    return &emptyByte;
}

ByteBuffer::Block* ByteBuffer::allocate(size_type capacity)
{
    if (capacity > std::numeric_limits<size_type>::max() - sizeof(Block)) {
        throw std::length_error("ByteBuffer exceeds maximum size");
    }
    return new (::operator new(sizeof(Block) + capacity)) Block(capacity);
}

void ByteBuffer::destroy(Block* block) noexcept
{
    block->~Block();
    ::operator delete(block);
}

ByteBuffer& ByteBuffer::operator=(ByteBuffer&& other) noexcept
{
    if (this == &other) {
        return *this;
    }

    // A borrowed view can point into this buffer's own block. Keep the block
    // and view that range instead of releasing the storage it points to.
    if (!other.block && viewsOwnBlock(other.view())) {
        ptr = std::exchange(other.ptr, nullptr);
        len = std::exchange(other.len, 0U);
        return *this;
    }

    Block* old = block;
    ptr = std::exchange(other.ptr, nullptr);
    len = std::exchange(other.len, 0U);
    block = std::exchange(other.block, nullptr);
    release(old);
    return *this;
}

ByteBuffer ByteBuffer::copy(BytesView bytes)
{
    ByteBuffer out;
    if (!bytes.empty()) {
        out.block = allocate(bytes.size());
        std::memcpy(out.block->bytes(), bytes.data(), bytes.size());
        out.ptr = out.block->bytes();
        out.len = bytes.size();
    }
    return out;
}

void ByteBuffer::makeOwning()
{
    if (isBorrowed()) {
        ensureUnique(len);
    }
}

void ByteBuffer::clear() noexcept
{
    if (block && block->refs.load(std::memory_order_acquire) == 1U) {
        ptr = block->bytes();
    }
    else {
        release(block);
        block = nullptr;
        ptr = nullptr;
    }
    len = 0U;
}

void ByteBuffer::reserve(size_type capacity)
{
    ensureUnique(capacity);
}

void ByteBuffer::resize(size_type newSize, std::byte fill)
{
    ensureUnique(newSize);
    if (newSize > len) {
        std::memset(const_cast<std::byte*>(ptr) + len, static_cast<int>(fill), newSize - len);
    }
    len = newSize;
}

void ByteBuffer::append(BytesView bytes)
{
    if (bytes.empty()) {
        return;
    }

    if (bytes.size() > std::numeric_limits<size_type>::max() - len) {
        throw std::length_error("ByteBuffer append exceeds maximum size");
    }

    const size_type needed = len + bytes.size();
    if (block && block->refs.load(std::memory_order_acquire) == 1U
        && block->capacity - static_cast<size_type>(ptr - block->bytes()) >= needed) {
        // The source may lie inside this block.
        std::memmove(const_cast<std::byte*>(ptr) + len, bytes.data(), bytes.size());
        len = needed;
        return;
    }

    // Grow geometrically so that repeated appends stay linear. The old storage
    // is released only after copying, as the source may point into it.
    const size_type capacity = len > needed / 2U && len <= std::numeric_limits<size_type>::max() / 2U
        ? 2U * len
        : needed;
    Block* grown = allocate(capacity);
    if (len != 0U) {
        std::memcpy(grown->bytes(), ptr, len);
    }
    std::memcpy(grown->bytes() + len, bytes.data(), bytes.size());
    release(block);
    block = grown;
    ptr = grown->bytes();
    len = needed;
}

bool ByteBuffer::viewsOwnBlock(BytesView bytes) const noexcept
{
    if (!block || bytes.empty()) {
        return false;
    }

    const auto begin = reinterpret_cast<std::uintptr_t>(block->bytes());
    const auto address = reinterpret_cast<std::uintptr_t>(bytes.data());
    return address >= begin && address - begin <= block->capacity
        && bytes.size() <= block->capacity - (address - begin);
}

void ByteBuffer::ensureUnique(size_type minCapacity)
{
    if (block && block->refs.load(std::memory_order_acquire) == 1U
        && block->capacity - static_cast<size_type>(ptr - block->bytes()) >= minCapacity) {
        return;
    }

    const size_type capacity = minCapacity > len ? minCapacity : len;
    if (capacity == 0U) {
        release(block);
        block = nullptr;
        ptr = nullptr;
        return;
    }

    Block* unique = allocate(capacity);
    if (len != 0U) {
        std::memcpy(unique->bytes(), ptr, len);
    }
    release(block);
    block = unique;
    ptr = unique->bytes();
}

}  // namespace Base
