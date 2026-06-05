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

#include <cstdint>
#include <type_traits>
#include <bit>

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

namespace Base::CrashReporter {


enum class Flags : std::uint32_t {
    None = 0,
    HasMiniDump = 1U << 0,
    PartialWrite = 1U << 1,
    CaptureWasSignalSafe = 1U << 2
};

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

static constexpr std::uint32_t NoString = 0xFFFFFFFFu;


static constexpr std::size_t HeaderSize = 128;
struct Header {
    std::uint32_t magic = 0x52434346;  // hedxump gives 'FCCR' (little endian!)
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
    std::uint32_t freecadVersionRevision = 0;

    OS osID = OS::None;
    Architecture architectureID = Architecture::None;
    std::uint8_t freecadVersionMajor = 0;
    std::uint8_t freecadVersionMinor = 0;
    std::uint8_t freecadVersionPatch = 0;

    // The real data above takes up 85 bytes: pad it out to the HeaderSize
    std::uint8_t padding[HeaderSize-85] = {0};
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




struct Footer
{
    std::uint32_t checksum = 0;  // crc32
};

static_assert(sizeof(Footer) == 4);
static_assert(std::is_standard_layout_v<Footer>);
static_assert(std::is_trivially_copyable_v<Footer>);

}  // namespace Base::CrashReporter

