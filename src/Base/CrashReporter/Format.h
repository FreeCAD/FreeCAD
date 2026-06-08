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


/*******************************************************************************
 *******************************************************************************
 **                                                                           **
 **  WARNING: THIS FILE IS INCLUDED BY CODE THAT IS EXECUTED INSIDE SIGNAL    **
 **           HANDLERS AND MUST BE ASYNC-SIGNAL-SAFE. IT MUST NOT CONTAIN     **
 **           ANYTHING THAT WILL ALLOCATE ON THE HEAP DURING CONSTRUCTION     **
 **           (E.G. STL CONTAINERS).                                          **
 **                                                                           **
 *******************************************************************************
 *******************************************************************************/

#pragma once

#include "Base/Bitmask.h"

#include <array>
#include <bit>
#include <span>
#include <type_traits>

// This file defines the fcrash file format: it's very low-level because the file is written inside
// signal handlers and structured event handlers and has to be async-signal-safe, which rules out
// most "normal" C++ code.
//
// The file is a simple binary file (little-endian by definition) that has a fixed header, then
// some number of stack frame entries, then an arbitrary-sized string storage block, and finally
// a CRC32 checksum of all previous bytes.

// The struct below explicitly requires a little-endian system, which at the time of this writing
// was all FreeCAD supported anyway. Make sure if that ever changes this dies and someone comes
// looking.
static_assert(std::endian::native == std::endian::little);

// This code hand-packs a fixed binary buffer from an async-signal-safe context: C arrays,
// computed indices, and raw permission/mask literals are intentional and required here.
// NOLINTBEGIN(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays,cppcoreguidelines-pro-bounds-constant-array-index,readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers,clang-diagnostic-unsafe-buffer-usage)

namespace Base::CrashReporter
{
enum class Flags : std::uint32_t {
    None = 0,
    HasMiniDump = 1U << 0,
    PartialWrite = 1U << 1,
    CaptureWasSignalSafe = 1U << 2
};
}
ENABLE_BITMASK_OPERATORS(Base::CrashReporter::Flags);

namespace Base::CrashReporter
{
[[nodiscard]] constexpr bool hasFlag(Flags flags, Flags flag) noexcept
{
    return (flags & flag) != Flags::None;
}

enum class OS : std::uint8_t {
    None,
    Linux,
    macOS,
    Windows
};

enum class Architecture : std::uint8_t {
    None,
    x64,
    aarch64
};

static constexpr std::uint32_t NoString = 0xFFFFFFFFU;


static constexpr std::size_t HeaderSize = 128;
static constexpr std::uint32_t MagicNumber = 0x52434346; // hedxump gives 'FCCR' (little endian!)
struct Header {
    std::uint32_t magic = MagicNumber;
    std::uint32_t version = 1;  // The fcrash format version

    std::uint64_t faultAddress = 0;
    std::uint64_t threadID = 0;
    std::int64_t timestamp = 0;  // time(2) is async-signal-safe, and gives and int64

    std::uint32_t processID = 0;
    std::uint32_t code = 0;  // Signal number (POSIX) or SEH ExceptionCode (Windows)
    std::uint32_t frameCount = 0;
    std::uint32_t fileSize = 0;
    Flags flags = Flags::None;  // See CrashReportFlags enum above

    std::uint32_t frameTableOffset = 0;
    std::uint32_t stringTableOffset = 0;
    
    // Strings, stored as offsets into the string table
    std::uint32_t buildIDStringOffset = NoString;
    std::uint32_t freecadVersionSuffixStringOffset = NoString;
    std::uint32_t minidumpPathStringOffset = NoString;
    std::uint32_t exceptionMessageStringOffset = NoString;

    OS osID = OS::None;
    Architecture architectureID = Architecture::None;
    std::uint8_t freecadVersionMajor = 0;
    std::uint8_t freecadVersionMinor = 0;
    std::uint8_t freecadVersionPatch = 0;

    // The real data above takes up 81 bytes: pad it out to the HeaderSize
    std::array<std::uint8_t, HeaderSize-81> padding = {};
};

static_assert(sizeof(Header) == HeaderSize);
static_assert(std::is_standard_layout_v<Header>);
static_assert(std::is_trivially_copyable_v<Header>);


constexpr std::uint32_t MaxFrames = 128;
struct Frame
{
    std::uint64_t rawAddress = 0;
    std::uint64_t moduleOffset = 0;
    std::uint32_t moduleStringOffset = NoString;
    std::uint32_t padding = 0;  // Keep aligned on the eights
};

static_assert(sizeof(Frame) == 24);
static_assert(std::is_standard_layout_v<Frame>);
static_assert(std::is_trivially_copyable_v<Frame>);

constexpr std::uint32_t MaxStringLength = 4096;
struct StringRecord
{
    uint16_t stringLength = 0;
    // NOTE: The string itself is variable-size, so isn't a member of the struct. The read and write
    // code will simply inject stringLength bytes of string data after this record entry, and the
    // next string will immediately follow.
};

struct Footer
{
    std::uint32_t checksum = 0;  // crc32
};

static_assert(sizeof(Footer) == 4);
static_assert(std::is_standard_layout_v<Footer>);
static_assert(std::is_trivially_copyable_v<Footer>);

// That footer is a CRC32 checksum of all of the files' contents (except the footer itself, of
// course). To guarantee that our CRC32 code is async-signal-safe, don't use an external library.
// This is a well-known algorithm from RFC 1952 Appendix C (gzip spec, Peter Deutsch, 1996). Public
// domain. Code adapted for use here.
[[nodiscard]] constexpr std::uint32_t crc32(std::span<const char> data) noexcept
{
    constexpr auto table = [] {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c & 1U) ? (0xEDB88320U ^ (c >> 1)) : (c >> 1);
            }
            t[i] = c;
        }
        return t;
    }();

    std::uint32_t crc = 0xFFFFFFFFU;
    for (const char b : data) {
        crc = table[(crc ^ static_cast<unsigned char>(b)) & 0xFFU] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFU;
}



// The actual maximum file size is:
// sizeof(Header)                                                    = 128     +
// MaxFrames * sizeof(Frame) = 128 * 74                              = 3072    +
// (MaxFrames + 4) * (sizeof(uint16_t) + MaxStringLen) = 132 * 4,098 = 540,936 +
// sizeof(Footer)                                                    = 4
//                                                                   = 544,140 bytes
// So give us a nice round cap to check against -- very small in the grand scheme of things, a
// corrupted or malicious file this size will not cause any memory pressure.
constexpr std::uint64_t MaxFileSize = 1U << 20;  // 1 MiB

// NOLINTEND(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays,cppcoreguidelines-pro-bounds-constant-array-index,readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers,clang-diagnostic-unsafe-buffer-usage)


}  // namespace Base::CrashReporter

