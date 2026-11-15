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
#include <memory>
#include <vector>

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
/// This uses C++20 byte views and does not rely on heterogeneous lookup in unordered containers.
class BaseExport ByteBuffer
{
public:
    using size_type = std::size_t;

    ByteBuffer();
    ByteBuffer(const ByteBuffer&);
    ByteBuffer& operator=(const ByteBuffer&);
    ByteBuffer(ByteBuffer&& other) noexcept;
    ByteBuffer& operator=(ByteBuffer&& other);
    ~ByteBuffer();

    static ByteBuffer copy(BytesView bytes);
    static ByteBuffer borrow(BytesView bytes);

    BytesView view() const;
    const std::byte* data() const;
    size_type size() const;
    bool empty() const;
    bool isBorrowed() const;

    /// Ensure this instance owns its data. If already owning but shared, detaches (COW).
    void makeOwning();
    void clear();
    void reserve(size_type capacity);
    void resize(size_type newSize, std::byte fill = std::byte {0});
    void append(BytesView bytes);

    friend bool operator==(const ByteBuffer& a, const ByteBuffer& b);
    friend bool operator!=(const ByteBuffer& a, const ByteBuffer& b);

    int compare(const ByteBuffer& other) const;
    bool operator<(const ByteBuffer& other) const;

private:
    static bool rangesOverlap(
        const std::byte* first,
        size_type firstSize,
        const std::byte* second,
        size_type secondSize
    );
    void syncFromOwner();
    void resetToEmpty() noexcept;
    void ensureOwningForMutation(size_type minCapacity);

private:
    const std::byte* ptr {nullptr};
    size_type len {0U};
    std::shared_ptr<std::vector<std::byte>> owner;
};

BaseExport bool operator==(const ByteBuffer& a, const ByteBuffer& b);
BaseExport bool operator!=(const ByteBuffer& a, const ByteBuffer& b);

}  // namespace Base
