/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
 ***************************************************************************/


#ifndef BASE_XMLTOOLS_H
#define BASE_XMLTOOLS_H

#include <memory>
#include <iostream>
#include <xercesc/util/TransService.hpp>
#include <xercesc/util/XercesDefs.hpp>

#include <Base/Exception.h>
XERCES_CPP_NAMESPACE_USE
// Helper class
class BaseExport XMLTools
{
public:
    static std::string toStdString(const XMLCh* const toTranscode);
    static std::basic_string<XMLCh> toXMLString(const char* const fromTranscode);
    static void initialize();
    static void terminate();

private:
    static std::unique_ptr<XMLTranscoder> transcoder;
};

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
    fLocalForm = XMLString::transcode(toTranscode);
}

inline StrX::~StrX()
{
    XMLString::release(&fLocalForm);
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
};

inline std::ostream& operator<<(std::ostream& target, const StrXUTF8& toDump)
{
    target << toDump.c_str();
    return target;
}

inline StrXUTF8::StrXUTF8(const XMLCh* const toTranscode)
{
    str = XMLTools::toStdString(toTranscode);
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
    fUnicodeForm = XMLString::transcode(toTranscode);
}

inline XStr::~XStr()
{
    XMLString::release(&fUnicodeForm);
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

private :
    std::basic_string<XMLCh>  str;
};

inline XUTF8Str::XUTF8Str(const char* const fromTranscode)
{
    str = XMLTools::toXMLString(fromTranscode);
}

inline XUTF8Str::~XUTF8Str() = default;


// -----------------------------------------------------------------------
//  Getter methods
// -----------------------------------------------------------------------
inline const XMLCh* XUTF8Str::unicodeForm() const
{
    return str.c_str();
}

#endif // BASE_XMLTOOLS_H
