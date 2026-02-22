// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <QChar>
#include <QVector>
#include <QByteArray>


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
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    // In newer Qt we cannot use QString::toUcs4, so we have to do it the long way...

    const QString input = QString::fromUtf8(str);
    QString output;
    output.reserve(input.size());

    for (auto it = input.cbegin(); it != input.cend();) {
        const auto cp = static_cast<char32_t>(it->unicode());
        const uint ch = it->unicode();
        ++it;

        if (QChar::isHighSurrogate(ch) && it != input.cend()
            && QChar::isLowSurrogate(static_cast<char32_t>(it->unicode()))) {
            // We are outside the "Basic Multilingual Plane (BMP)" (directly storable in a UTF-16
            // char, which is what QString uses internally). So now we have to use *two* chars,
            // combine them into one for our check, and then run the validity check for XML output.
            const uint low = it->unicode();
            ++it;
            const char32_t full = QChar::surrogateToUcs4(ch, low);
            const bool valid = std::ranges::any_of(validRanges, [full](const auto& r) {
                return full >= r.first && full <= r.second;
            });
            const bool discouraged = std::ranges::any_of(discouragedRanges, [full](const auto& r) {
                return full >= r.first && full <= r.second;
            });
            output.append((valid && !discouraged) ? QChar::fromUcs4(full) : QChar::fromUcs4('_'));
        }
        else {
            // The character fits into 16 bytes, it can be checked directly
            const bool valid = std::ranges::any_of(validRanges, [cp](const auto& r) {
                return cp >= r.first && cp <= r.second;
            });
            const bool discouraged = std::ranges::any_of(discouragedRanges, [cp](const auto& r) {
                return cp >= r.first && cp <= r.second;
            });
            output.append((valid && !discouraged) ? QChar(ch) : QChar::fromUcs2('_'));
        }
    }

    const QByteArray utf8 = output.toUtf8();
    return {utf8.constData(), static_cast<size_t>(utf8.size())};
#else
    // In older Qt we can directly use QString::toUcs4, which makes for a bit simpler code
    const QString input = QString::fromStdString(str);
    const QVector<uint> ucs4 = input.toUcs4();

    QVector<uint> filtered;
    filtered.reserve(ucs4.size());

    for (uint cp : ucs4) {
        const char32_t c32 = static_cast<char32_t>(cp);
        const bool ok = std::ranges::any_of(validRanges, [c32](const auto& r) {
            return c32 >= r.first && c32 <= r.second;
        });
        const bool discouraged = std::ranges::any_of(discouragedRanges, [c32](const auto& r) {
            return c32 >= r.first && c32 <= r.second;
        });
        filtered.push_back((ok && !discouraged) ? cp : static_cast<uint>(U'_'));
    }

    const QString output = QString::fromUcs4(filtered.constData(), filtered.size());
    const QByteArray utf8 = output.toUtf8();
    return {utf8.constData(), static_cast<size_t>(utf8.size())};
#endif
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
