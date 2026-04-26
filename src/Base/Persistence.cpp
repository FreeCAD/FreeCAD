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

#include <algorithm>
#include <array>
#include <cassert>
#include <codecvt>
#include <locale>
#include <ranges>

#include <zipios++/zipinputstream.h>

#include <unicode/utf8.h>

#include "Exception.h"
#include "Reader.h"
#include "Writer.h"

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Persistence.h"


using namespace Base;

TYPESYSTEM_SOURCE_ABSTRACT(Base::Persistence, Base::BaseClass)


//**************************************************************************
// Construction/Destruction


//**************************************************************************
// separator for other implementation aspects

unsigned int Persistence::getMemSize() const
{
    // you have to implement this method in all descending classes!
    assert(0);
    return 0;
}

void Persistence::Save(Writer& /*writer*/) const
{
    // you have to implement this method in all descending classes!
    assert(0);
}

void Persistence::Restore(XMLReader& /*reader*/)
{
    // you have to implement this method in all descending classes!
    assert(0);
}

void Persistence::SaveDocFile(Writer& /*writer*/) const
{}

void Persistence::RestoreDocFile(Reader& /*reader*/)
{}

std::string Persistence::encodeAttribute(const std::string& str)
{
    std::string tmp;
    for (char it : str) {
        switch (it) {
            case '<':
                tmp += "&lt;";
                break;
            case '\"':
                tmp += "&quot;";
                break;
            case '\'':
                tmp += "&apos;";
                break;
            case '&':
                tmp += "&amp;";
                break;
            case '>':
                tmp += "&gt;";
                break;
            case '\r':
                tmp += "&#13;";
                break;
            case '\n':
                tmp += "&#10;";
                break;
            case '\t':
                tmp += "&#9;";
                break;
            default:
                tmp += it;
                break;
        }
    }

    return tmp;
}

// clang-format off
// https://www.w3.org/TR/xml/#charsets
// Nominally allowed ranges
static constexpr std::array<std::pair<char32_t, char32_t>, 6> validRanges {{
    {0x9, 0x9}, // TAB -- explicitly allowed
    {0xA, 0xA}, // LF -- explicitly allowed
    {0xD, 0xD}, // CR -- explicitly allowed
    {0x20, 0xD7FF},
    {0xE000, 0xFFFD},
    {0x10000, 0x10FFFF}
}};
static constexpr std::array<std::pair<char32_t, char32_t>, 19> discouragedRanges {{
    {0x7F, 0x84},
    {0x86, 0x9F},
    {0xFDD0, 0xFDEF},
    {0x1FFFE, 0x1FFFF},
    {0x2FFFE, 0x2FFFF},
    {0x3FFFE, 0x3FFFF},
    {0x4FFFE, 0x4FFFF},
    {0x5FFFE, 0x5FFFF},
    {0x6FFFE, 0x6FFFF},
    {0x7FFFE, 0x7FFFF},
    {0x8FFFE, 0x8FFFF},
    {0x9FFFE, 0x9FFFF},
    {0xAFFFE, 0xAFFFF},
    {0xBFFFE, 0xBFFFF},
    {0xCFFFE, 0xCFFFF},
    {0xDFFFE, 0xDFFFF},
    {0xEFFFE, 0xEFFFF},
    {0xFFFFE, 0xFFFFF},
    {0x10FFFE, 0x10FFFF}
}};
// clang-format on

/*!
 * In XML not all valid Unicode characters are allowed. Replace all
 * disallowed characters with '_'
 */
std::string Persistence::validateXMLString(const std::string& str)
{
    // Decode UTF-8 into code points and filter for XML 1.0 validity, replacing invalid or
    // discouraged code points with '_'.
    std::string out;
    out.reserve(str.size());

    const auto* data = reinterpret_cast<const uint8_t*>(str.data());  // NOLINT
    const int32_t len = static_cast<int32_t>(str.size());

    for (int32_t i = 0; i < len;) {
        UChar32 cp = 0;
        U8_NEXT(data, i, len, cp);
        if (cp < 0) {
            out.push_back('_');
            continue;
        }

        const char32_t c32 = static_cast<char32_t>(cp);
        const bool ok = std::ranges::any_of(validRanges, [c32](const auto& r) {
            return c32 >= r.first && c32 <= r.second;
        });
        const bool discouraged = std::ranges::any_of(discouragedRanges, [c32](const auto& r) {
            return c32 >= r.first && c32 <= r.second;
        });

        const char32_t emit = (ok && !discouraged) ? c32 : U'_';
        uint8_t buf[8] {};
        int32_t outLen = 0;
        UBool isError = false;
        U8_APPEND(buf, outLen, static_cast<int32_t>(sizeof(buf)), static_cast<UChar32>(emit), isError);
        if (isError) {
            out.push_back('_');
        }
        else {
            out.append(reinterpret_cast<const char*>(buf), static_cast<std::size_t>(outLen));  // NOLINT
        }
    }

    return out;
}

void Persistence::dumpToStream(std::ostream& stream, int compression)
{
    // we need to close the zipstream to get a good result, the only way to do this is to delete the
    // ZipWriter. Hence the scope...
    {
        // create the writer
        Base::ZipWriter writer(stream);
        writer.setLevel(compression);
        writer.putNextEntry("Persistence.xml");
        writer.setMode("BinaryBrep");

        // save the content (we need to encapsulate it with xml tags to be able to read single
        // element xmls like happen for properties)
        writer.Stream() << "<Content>" << std::endl;
        Save(writer);
        writer.Stream() << "</Content>";
        writer.writeFiles();
    }
}

void Persistence::restoreFromStream(std::istream& stream)
{
    zipios::ZipInputStream zipstream(stream);
    Base::XMLReader reader("", zipstream);

    if (!reader.isValid()) {
        throw Base::ValueError("Unable to construct reader");
    }

    reader.readElement("Content");
    Restore(reader);
    reader.readFiles(zipstream);
    restoreFinished();
}
