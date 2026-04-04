// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <random>
#include <string_view>
#include <stdexcept>

#include <fmt/format.h>

#if defined(_WIN32)
# include <objbase.h>
#endif

#include "Uuid.h"


using namespace Base;

namespace
{

constexpr std::size_t uuidLen = 36;
constexpr std::size_t uuidLenWithBraces = 38;

int hexValue(const char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return 10 + (ch - 'a');
    }
    if (ch >= 'A' && ch <= 'F') {
        return 10 + (ch - 'A');
    }
    return -1;
}

std::string formatUuid(const std::array<std::uint8_t, 16>& bytes)
{
    return fmt::format(
        "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:"
        "02x}{:02x}",
        bytes[0],
        bytes[1],
        bytes[2],
        bytes[3],
        bytes[4],
        bytes[5],
        bytes[6],
        bytes[7],
        bytes[8],
        bytes[9],
        bytes[10],
        bytes[11],
        bytes[12],
        bytes[13],
        bytes[14],
        bytes[15]
    );
}

bool parseUuid(std::string_view text, std::array<std::uint8_t, 16>& out)
{
    if (text.size() == uuidLenWithBraces && text.front() == '{' && text.back() == '}') {
        text.remove_prefix(1);
        text.remove_suffix(1);
    }

    if (text.size() != uuidLen) {
        return false;
    }

    // Canonical RFC4122 format: 8-4-4-4-12
    if (text[8] != '-' || text[13] != '-' || text[18] != '-' || text[23] != '-') {
        return false;
    }

    auto readByte = [&](const std::size_t pos, std::uint8_t& byte) -> bool {
        const int hi = hexValue(text[pos]);
        const int lo = hexValue(text[pos + 1]);
        if (hi < 0 || lo < 0) {
            return false;
        }
        byte = static_cast<std::uint8_t>((hi << 4) | lo);
        return true;
    };

    // Map positions skipping dashes.
    static constexpr std::array<std::size_t, 16> positions = {
        0,
        2,
        4,
        6,  // 8
        9,
        11,  // 4
        14,
        16,  // 4
        19,
        21,  // 4
        24,
        26,
        28,
        30,
        32,
        34  // 12
    };

    for (std::size_t i = 0; i < out.size(); ++i) {
        if (!readByte(positions[i], out[i])) {
            return false;
        }
    }

    // Validate RFC4122 variant (10xxxxxx) and version (4).
    const std::uint8_t version = static_cast<std::uint8_t>((out[6] >> 4) & 0x0F);
    const std::uint8_t variant = static_cast<std::uint8_t>((out[8] >> 6) & 0x03);
    if (version == 0) {
        // Reject "nil"/unspecified version UUIDs as invalid input in this API.
        return false;
    }
    // Accept RFC4122 variant only (binary 10).
    if (variant != 0b10) {
        return false;
    }

    return true;
}

std::array<std::uint8_t, 16> randomUuidV4Bytes()
{
    std::array<std::uint8_t, 16> bytes {};

#if defined(_WIN32)
    // CoCreateGuid does not require COM initialization.
    GUID guid;
    if (CoCreateGuid(&guid) != S_OK) {
        throw std::runtime_error("failed to create uuid");
    }
    static_assert(sizeof(guid) == 16);
    std::memcpy(bytes.data(), &guid, 16);
#else
    std::random_device rd;
    for (auto& b : bytes) {
        b = static_cast<std::uint8_t>(rd());
    }
#endif

    // Force RFC4122 version 4 and variant 1 bits.
    bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0F) | 0x40);
    bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3F) | 0x80);

    return bytes;
}

}  // namespace


//**************************************************************************
// Construction/Destruction

/**
 * A constructor.
 * A more elaborate description of the constructor.
 */
Uuid::Uuid()
{
    _uuid = createUuid();
}

/**
 * A destructor.
 * A more elaborate description of the destructor.
 */
Uuid::~Uuid() = default;

//**************************************************************************
//**************************************************************************
// Get the UUID
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::string Uuid::createUuid()
{
    return formatUuid(randomUuidV4Bytes());
}

void Uuid::setValue(const char* sString)
{
    if (sString) {
        std::array<std::uint8_t, 16> bytes {};
        if (!parseUuid(sString, bytes)) {
            throw std::runtime_error("invalid uuid");
        }
        _uuid = formatUuid(bytes);
    }
}

void Uuid::setValue(const std::string& sString)
{
    setValue(sString.c_str());
}

const std::string& Uuid::getValue() const
{
    return _uuid;
}
