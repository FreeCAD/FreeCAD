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

#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
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
/// This is C++17-friendly and does not rely on heterogeneous lookup in unordered containers.
class BaseExport ByteBuffer
{
public:
    using size_type = std::size_t;

    ByteBuffer() = default;

    static ByteBuffer copy(BytesView bytes)
    {
        ByteBuffer out;
        if (!bytes.empty()) {
            out.owner = std::make_shared<std::string>(bytes.data(), bytes.size());
            out.ptr = out.owner->data();
            out.len = out.owner->size();
        }
        return out;
    }

    static ByteBuffer borrow(BytesView bytes)
    {
        ByteBuffer out;
        if (!bytes.empty()) {
            out.ptr = bytes.data();
            out.len = bytes.size();
        }
        return out;
    }

    BytesView view() const
    {
        if (len == 0U) {
            return {};
        }
        return {ptr, len};
    }

    const char* data() const
    {
        return len == 0U ? "" : ptr;
    }

    size_type size() const
    {
        return len;
    }

    bool empty() const
    {
        return len == 0U;
    }

    bool isBorrowed() const
    {
        return (len != 0U) && !owner;
    }

    /// Ensure this instance owns its data. If already owning but shared, detaches (COW).
    void makeOwning()
    {
        ensureOwningForMutation(0U);
    }

    void clear()
    {
        if (owner && owner.use_count() == 1) {
            owner->clear();
            ptr = "";
            len = 0U;
            return;
        }
        owner.reset();
        ptr = "";
        len = 0U;
    }

    void reserve(size_type capacity)
    {
        ensureOwningForMutation(capacity);
        owner->reserve(capacity);
        syncFromOwner();
    }

    void resize(size_type newSize, char fill = '\0')
    {
        ensureOwningForMutation(newSize);
        owner->resize(newSize, fill);
        syncFromOwner();
    }

    void append(BytesView bytes)
    {
        if (bytes.empty()) {
            return;
        }
        ensureOwningForMutation(len + bytes.size());
        owner->append(bytes.data(), bytes.size());
        syncFromOwner();
    }

    friend bool operator==(const ByteBuffer& a, const ByteBuffer& b)
    {
        if (a.len != b.len) {
            return false;
        }
        if (a.len == 0U) {
            return true;
        }
        return std::memcmp(a.ptr, b.ptr, a.len) == 0;
    }

    friend bool operator!=(const ByteBuffer& a, const ByteBuffer& b)
    {
        return !(a == b);
    }

    int compare(const ByteBuffer& other) const
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

    bool operator<(const ByteBuffer& other) const
    {
        return compare(other) < 0;
    }

private:
    void syncFromOwner()
    {
        if (!owner || owner->empty()) {
            ptr = "";
            len = 0U;
            if (owner && owner->empty()) {
                // keep owner as-is (capacity may be reused)
            }
            return;
        }
        ptr = owner->data();
        len = owner->size();
    }

    void ensureOwningForMutation(size_type minCapacity)
    {
        if (!owner) {
            // Borrowed (or empty): materialize an owning copy.
            owner = std::make_shared<std::string>(data(), len);
        }
        else if (owner.use_count() != 1) {
            // Detach (copy-on-write).
            owner = std::make_shared<std::string>(*owner);
        }

        if (minCapacity > owner->capacity()) {
            owner->reserve(minCapacity);
        }
        syncFromOwner();
    }

private:
    const char* ptr {""};
    size_type len {0U};
    std::shared_ptr<std::string> owner;
};

}  // namespace Base
