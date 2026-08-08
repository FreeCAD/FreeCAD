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

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <span>
#include <string_view>

namespace Base
{

/// Non-owning view over a byte sequence (may contain embedded NULs).
using BytesView = std::span<const std::byte>;

/// Mutable non-owning view over a byte sequence.
using MutableBytesView = std::span<std::byte>;

inline constexpr std::size_t bytesNpos = std::size_t(-1);

/// Construct a byte view over an untyped byte range.
inline BytesView asBytes(const void* data, std::size_t size) noexcept
{
    return {static_cast<const std::byte*>(data), size};
}

/// Construct a byte view over textual storage without taking ownership.
inline BytesView asBytes(std::string_view text) noexcept
{
    return asBytes(text.data(), text.size());
}

inline std::size_t findBytes(BytesView value, BytesView target, std::size_t start = 0) noexcept
{
    if (start > value.size()) {
        return bytesNpos;
    }
    if (target.empty()) {
        return start;
    }
    if (target.size() > value.size() - start) {
        return bytesNpos;
    }

    const auto last = value.size() - target.size();
    for (std::size_t pos = start; pos <= last; ++pos) {
        if (std::memcmp(value.data() + pos, target.data(), target.size()) == 0) {
            return pos;
        }
    }
    return bytesNpos;
}

inline bool equalBytes(BytesView first, BytesView second) noexcept
{
    return first.size() == second.size()
        && (first.empty() || std::memcmp(first.data(), second.data(), first.size()) == 0);
}

inline std::size_t rfindBytes(BytesView value, BytesView target, std::size_t start = bytesNpos) noexcept
{
    if (target.empty()) {
        return std::min(start, value.size());
    }
    if (target.size() > value.size()) {
        return bytesNpos;
    }

    std::size_t pos = std::min(start, value.size() - target.size());
    do {
        if (std::memcmp(value.data() + pos, target.data(), target.size()) == 0) {
            return pos;
        }
    } while (pos-- != 0U);
    return bytesNpos;
}

inline bool startsWithBytes(BytesView value, BytesView prefix) noexcept
{
    return prefix.size() <= value.size()
        && (prefix.empty() || std::memcmp(value.data(), prefix.data(), prefix.size()) == 0);
}

inline bool endsWithBytes(BytesView value, BytesView suffix) noexcept
{
    return suffix.size() <= value.size()
        && (suffix.empty()
            || std::memcmp(value.data() + value.size() - suffix.size(), suffix.data(), suffix.size())
                == 0);
}

}  // namespace Base
