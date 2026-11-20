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
#include <cstdint>

#include <Base/BytesView.h>

namespace Base
{

/// Compute the deterministic, non-cryptographic 64-bit FNV-1a hash of bytes.
/// This hash is suitable for general-purpose hashing, such as hash tables,
/// but must not be used for security-sensitive purposes.
inline std::size_t fnv1a64(BytesView bytes) noexcept
{
    constexpr std::uint64_t offsetBasis = 14695981039346656037ULL;
    constexpr std::uint64_t prime = 1099511628211ULL;

    std::uint64_t hash = offsetBasis;
    for (const std::byte byte : bytes) {
        hash ^= std::to_integer<unsigned char>(byte);
        hash *= prime;
    }
    return static_cast<std::size_t>(hash);
}

/// Combine two hash values into a deterministic hash value.
inline std::size_t hashCombine(std::size_t seed, std::size_t value) noexcept
{
    return seed
        ^ (value + static_cast<std::size_t>(0x9e3779b97f4a7c15ULL) + (seed << 6U) + (seed >> 2U));
}

}  // namespace Base
