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

#pragma once

#include <atomic>
#include <cstddef>
#include <cstring>
#include <utility>

#ifndef FC_GLOBAL_H
# include <FCGlobal.h>
#endif

#include "BytesView.h"

namespace Base
{

/// Small byte container that can either own its storage or explicitly borrow it.
///
/// Intended for performance-sensitive code that needs:
/// - raw bytes with embedded NULs
/// - cheap copies (shared backing store)
/// - the ability to create non-owning instances for temporary lookups/parsing
///
/// Owned bytes live in a single reference counted block. A buffer may view a
/// sub-range of its block, which lets assignment from a borrowed view of the
/// buffer's own storage keep that storage alive without copying.
///
/// This uses C++20 byte views and does not rely on heterogeneous lookup in unordered containers.
class BaseExport ByteBuffer
{
public:
    using size_type = std::size_t;

    ByteBuffer() noexcept = default;

    ByteBuffer(const ByteBuffer& other) noexcept
        : ptr(other.ptr)
        , len(other.len)
        , block(other.block)
    {
        if (block) {
            block->refs.fetch_add(1, std::memory_order_relaxed);
        }
    }

    ByteBuffer(ByteBuffer&& other) noexcept
        : ptr(std::exchange(other.ptr, nullptr))
        , len(std::exchange(other.len, 0U))
        , block(std::exchange(other.block, nullptr))
    {}

    ByteBuffer& operator=(const ByteBuffer& other) noexcept
    {
        ByteBuffer copy(other);
        return *this = std::move(copy);
    }

    ByteBuffer& operator=(ByteBuffer&& other) noexcept;

    ~ByteBuffer()
    {
        release(block);
    }

    static ByteBuffer copy(BytesView bytes);
    static ByteBuffer borrow(BytesView bytes) noexcept
    {
        ByteBuffer out;
        if (!bytes.empty()) {
            out.ptr = bytes.data();
            out.len = bytes.size();
        }
        return out;
    }

    BytesView view() const noexcept
    {
        return {data(), len};
    }
    const std::byte* data() const noexcept
    {
        return ptr ? ptr : emptyData();
    }
    size_type size() const noexcept
    {
        return len;
    }
    bool empty() const noexcept
    {
        return len == 0U;
    }
    bool isBorrowed() const noexcept
    {
        return len != 0U && !block;
    }

    /// Ensure this instance owns its data. Shared owned storage is kept shared.
    void makeOwning();
    void clear() noexcept;
    void reserve(size_type capacity);
    void resize(size_type newSize, std::byte fill = std::byte {0});
    void append(BytesView bytes);

    friend bool operator==(const ByteBuffer& a, const ByteBuffer& b) noexcept
    {
        return a.len == b.len && (a.len == 0U || std::memcmp(a.ptr, b.ptr, a.len) == 0);
    }
    friend bool operator!=(const ByteBuffer& a, const ByteBuffer& b) noexcept
    {
        return !(a == b);
    }

    int compare(const ByteBuffer& other) const noexcept
    {
        const size_type common = len < other.len ? len : other.len;
        if (common != 0U) {
            const int cmp = std::memcmp(ptr, other.ptr, common);
            if (cmp != 0) {
                return cmp;
            }
        }
        return len < other.len ? -1 : (len > other.len ? 1 : 0);
    }
    bool operator<(const ByteBuffer& other) const noexcept
    {
        return compare(other) < 0;
    }

private:
    struct Block
    {
        explicit Block(size_type capacity) noexcept
            : refs(1U)
            , capacity(capacity)
        {}

        std::atomic<size_type> refs;
        size_type capacity;

        std::byte* bytes() noexcept
        {
            return reinterpret_cast<std::byte*>(this + 1);
        }
    };

    static const std::byte* emptyData() noexcept;
    static Block* allocate(size_type capacity);
    static void destroy(Block* block) noexcept;
    static void release(Block* block) noexcept
    {
        if (block && block->refs.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            destroy(block);
        }
    }

    bool viewsOwnBlock(BytesView bytes) const noexcept;
    /// Make the bytes uniquely owned with room for at least @p minCapacity bytes
    /// past the start of the data.
    void ensureUnique(size_type minCapacity);

private:
    const std::byte* ptr {nullptr};
    size_type len {0U};
    Block* block {nullptr};
};

}  // namespace Base
