// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <memory>
#include <ostream>
#include <xercesc/util/TransService.hpp>
#include <xercesc/framework/MemoryManager.hpp>

#include <FCGlobal.h>

namespace XERCES_CPP_NAMESPACE
{
class DOMNode;
class DOMElement;
class DOMDocument;
}  // namespace XERCES_CPP_NAMESPACE

// Helper class
class BaseExport XMLTools
{
public:
    static std::string toStdString(const XMLCh* const toTranscode);
    static std::basic_string<XMLCh> toXMLString(const char* const fromTranscode);
    static std::string escapeXml(const std::string& input);
    static void initialize();
    static void terminate();

private:
    static std::unique_ptr<XERCES_CPP_NAMESPACE::XMLTranscoder> transcoder;  // NOLINT
};

// Helper class for XStrLiteral macro
// This implementation is almost same as Xerces default memory manager.
class BaseExport XStrMemoryManager final: public XERCES_CPP_NAMESPACE::MemoryManager
{
public:
    XStrMemoryManager() = default;
    ~XStrMemoryManager() = default;

    MemoryManager* getExceptionMemoryManager() override
    {
        return this;
    }

    void* allocate(XMLSize_t size) override;
    void deallocate(void* p) override;
};

//**************************************************************************
//**************************************************************************
// StrXLocal
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class StrX
{
public:
    explicit StrX(const XMLCh* const toTranscode);
    ~StrX();

    /// Getter method
    const char* c_str() const;
    FC_DISABLE_COPY_MOVE(StrX)

private:
    //  This is the local code page form of the string.
    char* fLocalForm;
};

inline std::ostream& operator<<(std::ostream& target, const StrX& toDump)
{
    target << toDump.c_str();
    return target;
}

inline StrX::StrX(const XMLCh* const toTranscode)
    : fLocalForm(XERCES_CPP_NAMESPACE::XMLString::transcode(toTranscode))
{}

inline StrX::~StrX()
{
    XERCES_CPP_NAMESPACE::XMLString::release(&fLocalForm);
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
public:
    explicit StrXUTF8(const XMLCh* const toTranscode);

    /// Getter method
    const char* c_str() const;
    /// string which holds the UTF-8 form
    std::string str;
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
public:
    ///  Constructors and Destructor
    explicit XStr(const char* const toTranscode);
    explicit XStr(const char* const toTranscode, XERCES_CPP_NAMESPACE::MemoryManager* memMgr);
    ~XStr();


    ///  Getter method
    const XMLCh* unicodeForm() const;
    FC_DISABLE_COPY_MOVE(XStr)

private:
    /// This is the Unicode XMLCh format of the string.
    XMLCh* fUnicodeForm;
    XERCES_CPP_NAMESPACE::MemoryManager* memMgr;
};


inline XStr::XStr(const char* const toTranscode)
    : XStr(toTranscode, XERCES_CPP_NAMESPACE::XMLPlatformUtils::fgMemoryManager)
{}

inline XStr::XStr(const char* const toTranscode, XERCES_CPP_NAMESPACE_QUALIFIER MemoryManager* memMgr)
    : fUnicodeForm(XERCES_CPP_NAMESPACE::XMLString::transcode(toTranscode, memMgr))
    , memMgr(memMgr)
{}

inline XStr::~XStr()
{
    XERCES_CPP_NAMESPACE::XMLString::release(&fUnicodeForm, memMgr);
}

// Uses the compiler to create a cache of transcoded string literals so that each subsequent call
// can reuse the data from the lambda's initial creation. Permits the same usage as
// XStr("literal").unicodeForm()
// XStrLiteral macro use local memory manager instance to prevent segfault on releasing cached
// string because xerces default memory manager is already deleted when destructing local static
// variable.
#define XStrLiteral(literal) \
    ([]() -> const XStr& { \
        static XStrMemoryManager memMgr; \
        static const XStr str {literal, &memMgr}; \
        return str; \
    }())


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
public:
    explicit XUTF8Str(const char* const fromTranscode);
    ~XUTF8Str();

    /// Getter method
    const XMLCh* unicodeForm() const;

    FC_DISABLE_COPY_MOVE(XUTF8Str)

private:
    std::basic_string<XMLCh> str;
};

inline XUTF8Str::XUTF8Str(const char* const fromTranscode)
{
    str = XMLTools::toXMLString(fromTranscode);
}

inline XUTF8Str::~XUTF8Str() = default;

// Uses the compiler to create a cache of transcoded string literals so that each subsequent call
// can reuse the data from the lambda's initial creation. Permits the same usage as
// XStr("literal").unicodeForm()
#define XUTF8StrLiteral(literal) \
    ([]() -> const XUTF8Str& { \
        static const XUTF8Str str {literal}; \
        return str; \
    }())

// -----------------------------------------------------------------------
//  Getter methods
// -----------------------------------------------------------------------
inline const XMLCh* XUTF8Str::unicodeForm() const
{
    return str.c_str();
}
