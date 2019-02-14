/***************************************************************************
 *   (c) Jürgen Riegel (juergen.riegel@web.de)                             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License (LGPL)   *
 *   as published by the Free Software Foundation; either version 2 of     *
 *   the License, or (at your option) any later version.                   *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with FreeCAD; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 *                                                                         *
 *   Juergen Riegel 2002                                                   *
 ***************************************************************************/


#ifndef BASE_XMLTOOLS_H
#define BASE_XMLTOOLS_H

// Std. configurations


// Include files
#include <memory>
#include <iostream>
#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/XercesVersion.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/TransService.hpp>

#include <Base/Exception.h>

XERCES_CPP_NAMESPACE_BEGIN
    class DOMNode;
    class DOMElement;
    class DOMDocument;
XERCES_CPP_NAMESPACE_END


//**************************************************************************
//**************************************************************************
// StrXLocal
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class StrX
{
public :
    StrX(const XMLCh* const toTranscode);
    ~StrX();

    /// Getter method
    const char* c_str() const;

private :
    //  This is the local code page form of the string.
    char*   fLocalForm;
};

inline std::ostream& operator<<(std::ostream& target, const StrX& toDump)
{
    target << toDump.c_str();
    return target;
}

inline StrX::StrX(const XMLCh* const toTranscode)
{
    // Call the private transcoding method
    fLocalForm = XERCES_CPP_NAMESPACE_QUALIFIER XMLString::transcode(toTranscode);
//#ifdef FC_OS_WIN32
//    assert(0)
//    WideCharToMultiByte(CP_UTF8,0,toTranscode,-1,fLocaleForm)
//#else
//    fUnicodeForm = XERCES_CPP_NAMESPACE_QUALIFIER XMLString::transcode(toTranscode);
//#endif 
}

inline StrX::~StrX()
{
    //delete [] fLocalForm; // don't work on VC7.1
    XERCES_CPP_NAMESPACE_QUALIFIER XMLString::release(&fLocalForm);
}


// -----------------------------------------------------------------------
//  Getter methods
// -----------------------------------------------------------------------
inline const char* StrX::c_str() const
{
    return fLocalForm;
}

//**************************************************************************
//**************************************************************************
// StrXUTF-8
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class StrXUTF8
{
public :
    StrXUTF8(const XMLCh* const toTranscode);

    /// Getter method
    const char* c_str() const;
    /// string which holds the UTF-8 form
    std::string  str;

    static void terminate();

private :
    static std::unique_ptr<XERCES_CPP_NAMESPACE::XMLTranscoder> transcoder;
    //  This is the local code page form of the string.
};

inline std::ostream& operator<<(std::ostream& target, const StrXUTF8& toDump)
{
    target << toDump.c_str();
    return target;
}

inline StrXUTF8::StrXUTF8(const XMLCh* const toTranscode)
{
    XERCES_CPP_NAMESPACE_USE;
    if(!transcoder.get()){
        XMLTransService::Codes  res;
        transcoder.reset(XERCES_CPP_NAMESPACE_QUALIFIER XMLPlatformUtils::fgTransService->makeNewTranscoderFor(XERCES_CPP_NAMESPACE_QUALIFIER XMLRecognizer::UTF_8, res, 4096, XERCES_CPP_NAMESPACE_QUALIFIER XMLPlatformUtils::fgMemoryManager));
        if (res != XMLTransService::Ok)
            throw Base::UnicodeError("Can\'t create UTF-8 encoder in StrXUTF8::StrXUTF8()");
    }

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
        str.append((const char*)outBuff, outputLength);
        offset += eaten;
        inputLength -= eaten;

        //  Bail out if nothing more was produced
        if (outputLength == 0)
            break;
    }
}

// -----------------------------------------------------------------------
//  Getter methods
// -----------------------------------------------------------------------
inline const char* StrXUTF8::c_str() const
{
    return str.c_str();
}


//**************************************************************************
//**************************************************************************
// XStr
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


class XStr
{
public :
    ///  Constructors and Destructor
    XStr(const char* const toTranscode);
    /// 
    ~XStr();


    ///  Getter method
    const XMLCh* unicodeForm() const;

private :
    /// This is the Unicode XMLCh format of the string.
    XMLCh*   fUnicodeForm;
};


inline XStr::XStr(const char* const toTranscode)
{
    // Call the private transcoding method
//#ifdef FC_OS_WIN32
//    assert(0)
//    WideCharToMultiByte()
//#else
    fUnicodeForm = XERCES_CPP_NAMESPACE_QUALIFIER XMLString::transcode(toTranscode);
//#endif 
}

inline XStr::~XStr()
{
    //delete [] fUnicodeForm;
    XERCES_CPP_NAMESPACE_QUALIFIER XMLString::release(&fUnicodeForm);
}


// -----------------------------------------------------------------------
//  Getter methods
// -----------------------------------------------------------------------
inline const XMLCh* XStr::unicodeForm() const
{
    return fUnicodeForm;
}

//**************************************************************************
//**************************************************************************
// XUTF-8Str
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class XUTF8Str
{
public :
    XUTF8Str(const char* const fromTranscode);
    ~XUTF8Str();

    /// Getter method
    const XMLCh* unicodeForm() const;

    static void terminate();

private :
    std::basic_string<XMLCh>  str;
    static std::unique_ptr<XERCES_CPP_NAMESPACE::XMLTranscoder> transcoder;
};

inline XUTF8Str::XUTF8Str(const char* const fromTranscode)
{
    if (!fromTranscode)
        return;

    XERCES_CPP_NAMESPACE_USE;
    if(!transcoder.get()){
        XMLTransService::Codes  res;
        transcoder.reset(XERCES_CPP_NAMESPACE_QUALIFIER XMLPlatformUtils::fgTransService->makeNewTranscoderFor(XERCES_CPP_NAMESPACE_QUALIFIER XMLRecognizer::UTF_8, res, 4096, XERCES_CPP_NAMESPACE_QUALIFIER XMLPlatformUtils::fgMemoryManager));
        if (res != XMLTransService::Ok)
            throw Base::UnicodeError("Can\'t create UTF-8 decoder in XUTF8Str::XUTF8Str()");
    }

    static XMLCh outBuff[128];
    XMLByte* xmlBytes = (XMLByte*)fromTranscode;
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
}

inline XUTF8Str::~XUTF8Str()
{
}


// -----------------------------------------------------------------------
//  Getter methods
// -----------------------------------------------------------------------
inline const XMLCh* XUTF8Str::unicodeForm() const
{
    return str.c_str();
}

#endif // BASE_XMLTOOLS_H
