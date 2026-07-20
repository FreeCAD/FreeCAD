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

#include "Sha1.h"

#include <cstddef>

namespace
{

std::uint32_t rotateLeft(std::uint32_t value, unsigned bits) noexcept
{
    return (value << bits) | (value >> (32U - bits));
}

std::uint32_t loadBe32(const std::uint8_t* p) noexcept
{
    return (static_cast<std::uint32_t>(p[0]) << 24U) | (static_cast<std::uint32_t>(p[1]) << 16U)
        | (static_cast<std::uint32_t>(p[2]) << 8U) | (static_cast<std::uint32_t>(p[3]) << 0U);
}

void processBlock(const std::uint8_t block[64], std::uint32_t state[5]) noexcept
{
    std::uint32_t w[80] {};
    for (int i = 0; i < 16; ++i) {
        w[i] = loadBe32(block + i * 4);
    }
    for (int i = 16; i < 80; ++i) {
        w[i] = rotateLeft(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    std::uint32_t a = state[0];
    std::uint32_t b = state[1];
    std::uint32_t c = state[2];
    std::uint32_t d = state[3];
    std::uint32_t e = state[4];

    for (int i = 0; i < 80; ++i) {
        std::uint32_t f = 0;
        std::uint32_t k = 0;
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999U;
        }
        else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1U;
        }
        else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCU;
        }
        else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6U;
        }

        const std::uint32_t temp = rotateLeft(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rotateLeft(b, 30);
        b = a;
        a = temp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

}  // namespace

std::array<std::uint8_t, 20> Base::sha1Digest(BytesView bytes) noexcept
{
    std::uint32_t state[5] {
        0x67452301U,
        0xEFCDAB89U,
        0x98BADCFEU,
        0x10325476U,
        0xC3D2E1F0U,
    };

    const auto* data = reinterpret_cast<const std::uint8_t*>(bytes.data());
    const std::size_t size = bytes.size();

    std::size_t offset = 0;
    while (offset + 64 <= size) {
        processBlock(data + offset, state);
        offset += 64;
    }

    const std::uint64_t bitLen = static_cast<std::uint64_t>(size) * 8U;

    const std::size_t rem = size - offset;
    std::array<std::uint8_t, 128> tail {};
    for (std::size_t i = 0; i < rem; ++i) {
        tail[i] = data[offset + i];
    }
    tail[rem] = 0x80U;

    const std::size_t total = rem + 1 + 8;
    const std::size_t paddedSize = (total <= 64) ? 64 : 128;

    const std::size_t lenOffset = paddedSize - 8;
    tail[lenOffset + 0] = static_cast<std::uint8_t>((bitLen >> 56U) & 0xFFU);
    tail[lenOffset + 1] = static_cast<std::uint8_t>((bitLen >> 48U) & 0xFFU);
    tail[lenOffset + 2] = static_cast<std::uint8_t>((bitLen >> 40U) & 0xFFU);
    tail[lenOffset + 3] = static_cast<std::uint8_t>((bitLen >> 32U) & 0xFFU);
    tail[lenOffset + 4] = static_cast<std::uint8_t>((bitLen >> 24U) & 0xFFU);
    tail[lenOffset + 5] = static_cast<std::uint8_t>((bitLen >> 16U) & 0xFFU);
    tail[lenOffset + 6] = static_cast<std::uint8_t>((bitLen >> 8U) & 0xFFU);
    tail[lenOffset + 7] = static_cast<std::uint8_t>((bitLen >> 0U) & 0xFFU);

    processBlock(tail.data(), state);
    if (paddedSize == 128) {
        processBlock(tail.data() + 64, state);
    }

    std::array<std::uint8_t, 20> out {};
    for (int i = 0; i < 5; ++i) {
        const std::uint32_t w = state[i];
        out[i * 4 + 0] = static_cast<std::uint8_t>((w >> 24U) & 0xFFU);
        out[i * 4 + 1] = static_cast<std::uint8_t>((w >> 16U) & 0xFFU);
        out[i * 4 + 2] = static_cast<std::uint8_t>((w >> 8U) & 0xFFU);
        out[i * 4 + 3] = static_cast<std::uint8_t>((w >> 0U) & 0xFFU);
    }

    return out;
}
