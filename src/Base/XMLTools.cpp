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

#ifndef _PreComp_
# include <cassert>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "XMLTools.h"

using namespace Base;

std::unique_ptr<XERCES_CPP_NAMESPACE::XMLTranscoder> XMLTools::transcoder;

void XMLTools::initialize()
{
    XERCES_CPP_NAMESPACE_USE;
    if (!transcoder.get()) {
        XMLTransService::Codes  res;
        transcoder.reset(XERCES_CPP_NAMESPACE_QUALIFIER XMLPlatformUtils::fgTransService->makeNewTranscoderFor(XERCES_CPP_NAMESPACE_QUALIFIER XMLRecognizer::UTF_8, res, 4096, XERCES_CPP_NAMESPACE_QUALIFIER XMLPlatformUtils::fgMemoryManager));
        if (res != XMLTransService::Ok)
            throw Base::UnicodeError("Can\'t create transcoder");
    }
}

std::string XMLTools::toStdString(const XMLCh* const toTranscode)
{
    std::string str;

    XERCES_CPP_NAMESPACE_USE;
    initialize();

    //char outBuff[128];
    static XMLByte outBuff[128];
#if (XERCES_VERSION_MAJOR == 2)
    unsigned int outputLength;
    unsigned int eaten = 0;
    unsigned int offset = 0;
    unsigned int inputLength = XMLString::stringLen(toTranscode);
#else
    XMLSize_t outputLength;
    XMLSize_t eaten = 0;
    XMLSize_t offset = 0;
    XMLSize_t inputLength = XMLString::stringLen(toTranscode);
#endif

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

    XERCES_CPP_NAMESPACE_USE;
    initialize();

    static XMLCh outBuff[128];
    const XMLByte* xmlBytes = reinterpret_cast<const XMLByte*>(fromTranscode);
#if (XERCES_VERSION_MAJOR == 2)
    unsigned int outputLength;
    unsigned int eaten = 0;
    unsigned int offset = 0;
    unsigned int inputLength = std::string(fromTranscode).size();
#else
    XMLSize_t outputLength;
    XMLSize_t eaten = 0;
    XMLSize_t offset = 0;
    XMLSize_t inputLength = std::string(fromTranscode).size();
#endif

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
