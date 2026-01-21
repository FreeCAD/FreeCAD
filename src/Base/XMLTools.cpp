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

#include <xercesc/framework/MemoryManager.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>

#include "Exception.h"
#include "XMLTools.h"

using namespace XERCES_CPP_NAMESPACE;

std::unique_ptr<XMLTranscoder> XMLTools::transcoder;  // NOLINT

void XMLTools::initialize()
{
    if (!transcoder) {
        XMLTransService::Codes res {};
        transcoder.reset(
            XMLPlatformUtils::fgTransService
                ->makeNewTranscoderFor(XMLRecognizer::UTF_8, res, 4096, XMLPlatformUtils::fgMemoryManager)
        );
        if (res != XMLTransService::Ok) {
            throw Base::UnicodeError("Can't create transcoder");
        }
    }
}

std::string XMLTools::toStdString(const XMLCh* const toTranscode)
{
    std::string str;

    initialize();

    // char outBuff[128];
    static XMLByte outBuff[128];
    XMLSize_t outputLength = 0;
    XMLSize_t eaten = 0;
    XMLSize_t offset = 0;
    XMLSize_t inputLength = XMLString::stringLen(toTranscode);

    while (inputLength) {
        outputLength = transcoder->transcodeTo(
            toTranscode + offset,
            inputLength,
            outBuff,
            128,
            eaten,
            XMLTranscoder::UnRep_RepChar
        );
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        str.append(reinterpret_cast<const char*>(outBuff), outputLength);
        offset += eaten;
        inputLength -= eaten;

        //  Bail out if nothing more was produced
        if (outputLength == 0) {
            break;
        }
    }

    return str;
}

std::basic_string<XMLCh> XMLTools::toXMLString(const char* const fromTranscode)
{
    std::basic_string<XMLCh> str;
    if (!fromTranscode) {
        return str;
    }

    initialize();

    static XMLCh outBuff[128];
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const XMLByte* xmlBytes = reinterpret_cast<const XMLByte*>(fromTranscode);
    XMLSize_t outputLength = 0;
    XMLSize_t eaten = 0;
    XMLSize_t offset = 0;
    XMLSize_t inputLength = std::string(fromTranscode).size();

    unsigned char* charSizes = new unsigned char[inputLength];
    while (inputLength) {
        outputLength
            = transcoder->transcodeFrom(xmlBytes + offset, inputLength, outBuff, 128, eaten, charSizes);
        str.append(outBuff, outputLength);
        offset += eaten;
        inputLength -= eaten;

        //  Bail out if nothing more was produced
        if (outputLength == 0) {
            break;
        }
    }

    delete[] charSizes;
    return str;
}

/*!
 * \brief Escape special XML characters in a string.
 *
 * Replaces XML special characters (&, <, >, ", ') with their entity equivalents
 * (&amp;, &lt;, &gt;, &quot;, &apos;).
 *
 * \param input The string to escape
 * \return The escaped string safe for use in XML content or attributes
 */
std::string XMLTools::escapeXml(const std::string& input)
{
    std::string output;
    output.reserve(input.size());
    for (char ch : input) {
        switch (ch) {
            case '&':
                output.append("&amp;");
                break;
            case '<':
                output.append("&lt;");
                break;
            case '>':
                output.append("&gt;");
                break;
            case '"':
                output.append("&quot;");
                break;
            case '\'':
                output.append("&apos;");
                break;
            default:
                output.push_back(ch);
                break;
        }
    }
    return output;
}

void XMLTools::terminate()
{
    transcoder.reset();
}

void* XStrMemoryManager::allocate(XMLSize_t size)
{
    auto ptr = ::operator new(
        static_cast<size_t>(size),
        static_cast<std::align_val_t>(alignof(XMLCh)),
        std::nothrow
    );
    if (ptr == nullptr && size != 0) {
        throw XERCES_CPP_NAMESPACE::OutOfMemoryException();
    }
    return ptr;
}

void XStrMemoryManager::deallocate(void* p)
{
    ::operator delete(p);
}
