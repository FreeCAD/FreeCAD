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


#include "PreCompiled.h"

#include "XMLTools.h"

using namespace Base;
XERCES_CPP_NAMESPACE_USE

std::unique_ptr<XMLTranscoder> XMLTools::transcoder;

void XMLTools::initialize()
{
    if (!transcoder.get()) {
        XMLTransService::Codes res{};
        transcoder.reset(XMLPlatformUtils::fgTransService->makeNewTranscoderFor(XMLRecognizer::UTF_8, res, 4096, XMLPlatformUtils::fgMemoryManager));
        if (res != XMLTransService::Ok)
            throw Base::UnicodeError("Can\'t create transcoder");
    }
}

std::string XMLTools::toStdString(const XMLCh* const toTranscode)
{
    std::string str;

    initialize();

    //char outBuff[128];
    static XMLByte outBuff[128];
    XMLSize_t outputLength = 0;
    XMLSize_t eaten = 0;
    XMLSize_t offset = 0;
    XMLSize_t inputLength = XMLString::stringLen(toTranscode);

    while (inputLength)
    {
        outputLength = transcoder->transcodeTo(toTranscode + offset, inputLength, outBuff, 128, eaten, XMLTranscoder::UnRep_RepChar);
        str.append(reinterpret_cast<const char*>(outBuff), outputLength);
        offset += eaten;
        inputLength -= eaten;

        //  Bail out if nothing more was produced
        if (outputLength == 0)
            break;
    }

    return str;
}

std::basic_string<XMLCh> XMLTools::toXMLString(const char* const fromTranscode)
{
    std::basic_string<XMLCh> str;
    if (!fromTranscode)
        return str;
	
    initialize();

    static XMLCh outBuff[128];
    const XMLByte* xmlBytes = reinterpret_cast<const XMLByte*>(fromTranscode);
    XMLSize_t outputLength = 0;
    XMLSize_t eaten = 0;
    XMLSize_t offset = 0;
    XMLSize_t inputLength = std::string(fromTranscode).size();

    unsigned char* charSizes = new unsigned char[inputLength];
    while (inputLength)
    {
        outputLength = transcoder->transcodeFrom(xmlBytes + offset, inputLength, outBuff, 128, eaten, charSizes);
        str.append(outBuff, outputLength);
        offset += eaten;
        inputLength -= eaten;

        //  Bail out if nothing more was produced
        if (outputLength == 0)
            break;
    }

    delete[] charSizes;
    return str;
}

void XMLTools::terminate()
{
    transcoder.reset();
}
