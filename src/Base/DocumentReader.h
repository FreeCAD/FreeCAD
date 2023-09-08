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

#ifndef BASE_DOCUMENTREADER_H
#define BASE_DOCUMENTREADER_H

#include <bitset>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <FCGlobal.h>

#include <xercesc/util/XercesDefs.hpp>

namespace zipios {
class ZipInputStream;
}

XERCES_CPP_NAMESPACE_BEGIN
	class DOMNode;
	class DOMElement;
XERCES_CPP_NAMESPACE_END
namespace Base
{
class Reader;
class Persistence;

class BaseExport DocumentReader
{
public:
	enum ReaderStatus {
        PartialRestore = 0,                     // This bit indicates that a partial restore took place somewhere in this Document
        PartialRestoreInDocumentObject = 1,     // This bit is local to the DocumentObject being read indicating a partial restore therein
        PartialRestoreInProperty = 2,           // Local to the Property
        PartialRestoreInObject = 3              // Local to the object partially restored itself
    };
    DocumentReader();
    int   LoadDocument(Base::Reader& reader);
    
    XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *GetRootElement() const;
    XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *FindElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* Start, const char* Type) const;
    XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *FindElement(const char* Type) const;
    
    XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *FindElementByField(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* Start,
    const char* TypeEl,const char* field_name, const char* field_value) const;
    
    XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *FindNextElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *Prev, const char* Type) const;
    
    static long ContentToASCII(const char* Content);
    
    static long ContentToInt(const char* Content);
    static unsigned long ContentToUnsigned(const char* Content);
    static double ContentToFloat(const char* Content);
    static bool ContentToBool(const char* Content);
    
    const char* GetAttribute(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* DOMEl, const char* Attr) const;
    const char* GetAttribute(const char* Attr) const;
    
    /// return the status bits
    bool testStatus(ReaderStatus pos) const;
    /// set the status bits
    void setStatus(ReaderStatus pos, bool on);
    /// sets simultaneously the global and local PartialRestore bits
    void setPartialRestore(bool on);
    
    void clearPartialRestoreProperty();
    
    /// add a read request of a persistent object
    const char *addFile(const char* Name, Base::Persistence *Object);
    
    void readFiles(zipios::ZipInputStream &zipstream) const;
    
    std::shared_ptr<Base::DocumentReader> getLocalReader() const;
    
    struct FileEntry {
        std::string FileName;
        Base::Persistence *Object;
    };
    std::vector<FileEntry> FileList;
    
    /// Version of the file format
    int FileVersion;
    
    
    
protected:
	XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *_pGroupNode;
	bool          gDoNamespaces         ;
    bool          gDoSchema             ;
    bool          gSchemaFullChecking   ;
    bool          gDoCreate             ;
    
    std::vector<std::string> FileNames;
    
    std::bitset<32> StatusBits;
    
};

}

#endif
