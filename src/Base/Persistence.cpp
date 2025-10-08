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

#include <zipios++/zipinputstream.h>

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
static constexpr std::array<std::pair<char32_t, char32_t>, 6> validRanges {{
    {0x9, 0x9},
    {0xA, 0xA},
    {0xD, 0xD},
    {0x20, 0xD7FF},
    {0xE000, 0xFFFD},
    {0x10000, 0x10FFFF},
}};
// clang-format on

/*!
 * In XML not all valid Unicode characters are allowed. Replace all
 * disallowed characters with '_'
 */
std::string Persistence::validateXMLString(const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;
    std::u32string cp_in = cvt.from_bytes(str);
    std::u32string cp_out;
    cp_out.reserve(cp_in.size());
    for (auto cp : cp_in) {
        if (std::any_of(validRanges.begin(), validRanges.end(), [cp](const auto& range) {
                return cp >= range.first && cp <= range.second;
            })) {
            cp_out += cp;
        }
        else {
            cp_out += '_';
        }
    }
    return cvt.to_bytes(cp_out);
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
