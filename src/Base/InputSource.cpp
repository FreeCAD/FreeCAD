// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <istream>
#include <cstddef>
#include <cstdint>

#include "InputSource.h"
#include "XMLTools.h"

using namespace Base;
using namespace std;


// ---------------------------------------------------------------------------
//  StdInputStream: Constructors and Destructor
// ---------------------------------------------------------------------------

namespace
{

// Replaces embedded NUL bytes and invalid UTF-8 byte sequences with '?' to avoid Xerces treating
// NUL as a string terminator and to keep input reasonably UTF-8-like.
void sanitizeUtf8Bytes(XMLByte* const toFill, const std::size_t len)
{
    auto* data = reinterpret_cast<std::uint8_t*>(
        toFill
    );  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

    auto markInvalidByte = [&](std::size_t pos) {
        data[pos] = static_cast<std::uint8_t>('?');
    };

    auto isCont = [&](std::uint8_t b) {
        return (b & 0xC0) == 0x80;
    };

    std::size_t i = 0;
    while (i < len) {
        const std::uint8_t b0 = data[i];

        if (b0 == 0) {
            markInvalidByte(i);
            ++i;
            continue;
        }

        if (b0 < 0x80) {
            ++i;
            continue;
        }

        // Reject stray continuation bytes
        if (isCont(b0)) {
            markInvalidByte(i);
            ++i;
            continue;
        }

        // 2-byte sequence: C2..DF 80..BF
        if (b0 >= 0xC2 && b0 <= 0xDF) {
            if (i + 1 >= len || !isCont(data[i + 1])) {
                markInvalidByte(i);
                ++i;
                continue;
            }
            i += 2;
            continue;
        }

        // 3-byte sequences
        if (b0 >= 0xE0 && b0 <= 0xEF) {
            if (i + 2 >= len) {
                markInvalidByte(i);
                ++i;
                continue;
            }

            const std::uint8_t b1 = data[i + 1];
            const std::uint8_t b2 = data[i + 2];
            if (!isCont(b1) || !isCont(b2)) {
                markInvalidByte(i);
                ++i;
                continue;
            }

            // Overlong / surrogate checks
            if (b0 == 0xE0 && b1 < 0xA0) {
                markInvalidByte(i);
                ++i;
                continue;
            }
            if (b0 == 0xED && b1 >= 0xA0) {
                // UTF-16 surrogate range U+D800..U+DFFF
                markInvalidByte(i);
                ++i;
                continue;
            }

            i += 3;
            continue;
        }

        // 4-byte sequences
        if (b0 >= 0xF0 && b0 <= 0xF4) {
            if (i + 3 >= len) {
                markInvalidByte(i);
                ++i;
                continue;
            }

            const std::uint8_t b1 = data[i + 1];
            const std::uint8_t b2 = data[i + 2];
            const std::uint8_t b3 = data[i + 3];
            if (!isCont(b1) || !isCont(b2) || !isCont(b3)) {
                markInvalidByte(i);
                ++i;
                continue;
            }

            // Overlong / > U+10FFFF checks
            if (b0 == 0xF0 && b1 < 0x90) {
                markInvalidByte(i);
                ++i;
                continue;
            }
            if (b0 == 0xF4 && b1 > 0x8F) {
                markInvalidByte(i);
                ++i;
                continue;
            }

            i += 4;
            continue;
        }

        // Invalid lead byte (C0/C1/F5..FF)
        markInvalidByte(i);
        ++i;
    }
}

}  // namespace

struct StdInputStream::TextCodec
{
    void validateBytes(XMLByte* const toFill, std::streamsize len)
    {
        if (len <= 0) {
            return;
        }
        sanitizeUtf8Bytes(toFill, static_cast<std::size_t>(len));
    }
};

StdInputStream::StdInputStream(
    std::istream& Stream,
    XERCES_CPP_NAMESPACE_QUALIFIER MemoryManager* const manager
)
    : stream(Stream)
    , codec(new TextCodec)
{
    (void)manager;
}


StdInputStream::~StdInputStream() = default;


// ---------------------------------------------------------------------------
//  StdInputStream: Implementation of the input stream interface
// ---------------------------------------------------------------------------
XMLFilePos StdInputStream::curPos() const
{
    return static_cast<XMLFilePos>(stream.tellg());
}

XMLSize_t StdInputStream::readBytes(XMLByte* const toFill, const XMLSize_t maxToRead)
{
    //
    //  Read up to the maximum bytes requested. We return the number
    //  actually read.
    //
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    stream.read(reinterpret_cast<char*>(toFill), static_cast<std::streamsize>(maxToRead));
    std::streamsize len = stream.gcount();

    codec->validateBytes(toFill, len);

    return static_cast<XMLSize_t>(len);
}


// ---------------------------------------------------------------------------
//  StdInputSource: Constructors and Destructor
// ---------------------------------------------------------------------------
StdInputSource::StdInputSource(
    std::istream& Stream,
    const char* filePath,
    XERCES_CPP_NAMESPACE::MemoryManager* const manager
)
    : InputSource(manager)
    , stream(Stream)
{
    // we have to set the file name in case an error occurs
    XStr tmpBuf(filePath);
    setSystemId(tmpBuf.unicodeForm());
}


StdInputSource::~StdInputSource() = default;


// ---------------------------------------------------------------------------
//  StdInputSource: InputSource interface implementation
// ---------------------------------------------------------------------------
XERCES_CPP_NAMESPACE::BinInputStream* StdInputSource::makeStream() const
{
    StdInputStream* retStrm = new StdInputStream(stream /*, getMemoryManager()*/);
    return retStrm;
}
