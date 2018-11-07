/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de)          *
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

#ifndef BASE_READER_H
#define BASE_READER_H


#include <string>
#include <map>
#include <bitset>

#include <xercesc/framework/XMLPScanToken.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>

#include "FileInfo.h"
#include "Writer.h"

namespace zipios {
class ZipInputStream;
}

XERCES_CPP_NAMESPACE_BEGIN
    class DefaultHandler;
    class SAX2XMLReader;
XERCES_CPP_NAMESPACE_END

namespace Base
{


/** The XML reader class
 * This is an important helper class for the store and retrieval system
 * of objects in FreeCAD. These classes mainly inherit the App::Persitance
 * base class and implement the Restore() method.
 *  \par
 * The reader gets mainly initialized by the App::Document on retrieving a
 * document out of a file. From there subsequently the Restore() method will
 * by called on all object stored.
 *  \par
 * A simple example is the Restore of App::PropertyString:
 *  \code
void PropertyString::Save (short indent,std::ostream &str)
{
    str << "<String value=\"" <<  _cValue.c_str() <<"\"/>" ;
}

void PropertyString::Restore(Base::Reader &reader)
{
    // read my Element
    reader.readElement("String");
    // get the value of my Attribute
    _cValue = reader.getAttribute("value");
}

 *  \endcode
 *  \par
 *  An more complicated example is the retrieval of the App::PropertyContainer:
 *  \code
void PropertyContainer::Save (short indent,std::ostream &str)
{
    std::map<std::string,Property*> Map;
    getPropertyMap(Map);

    str << ind(indent) << "<Properties Count=\"" << Map.size() << "\">" << endl;
    std::map<std::string,Property*>::iterator it;
    for(it = Map.begin(); it != Map.end(); ++it)
    {
        str << ind(indent+1) << "<Property name=\"" << it->first << "\" type=\"" << it->second->getTypeId().getName() << "\">" ;
        it->second->Save(indent+2,str);
        str << "</Property>" << endl;
    }
    str << ind(indent) << "</Properties>" << endl;
}

void PropertyContainer::Restore(Base::Reader &reader)
{
    reader.readElement("Properties");
    int Cnt = reader.getAttributeAsInteger("Count");

    for(int i=0 ;i<Cnt ;i++)
    {
        reader.readElement("Property");
        string PropName = reader.getAttribute("name");
        Property* prop = getPropertyByName(PropName.c_str());
        if(prop)
            prop->Restore(reader);

        reader.readEndElement("Property");
    }
    reader.readEndElement("Properties");
}
 *  \endcode
 * \see Base::Persistence
 * \author Juergen Riegel
 */
class BaseExport XMLReader : public XERCES_CPP_NAMESPACE_QUALIFIER DefaultHandler
{
public:
    enum ReaderStatus {
        PartialRestore = 0,                     // This bit indicates that a partial restore took place somewhere in this Document
        PartialRestoreInDocumentObject = 1,     // This bit is local to the DocumentObject being read indicating a partial restore therein
        PartialRestoreInProperty = 2,           // Local to the Property
        PartialRestoreInObject = 3              // Local to the object partially restored itself
    };
    /// open the file and read the first element
    XMLReader(const char* FileName, std::istream&);
    ~XMLReader();

    bool isValid() const { return _valid; }
    bool isVerbose() const { return _verbose; }
    void setVerbose(bool on) { _verbose = on; }

    /** @name Parser handling */
    //@{
    /// get the local name of the current Element
    const char* localName(void) const;
    /// read until a start element is found (\<name\>) or start-end element (\<name/\>) (with special name if given)
    void readElement   (const char* ElementName=0);
    /// read until an end element is found (with special name if given)
    void readEndElement(const char* ElementName=0);
    /// read until characters are found
    void readCharacters(void);
    /// read binary file
    void readBinFile(const char*);
    //@}

    /** @name Attribute handling */
    //@{
    /// get the number of attributes of the current element
    unsigned int getAttributeCount(void) const;
    /// check if the read element has a special attribute
    bool hasAttribute(const char* AttrName) const;
    /// return the named attribute as an interer (does type checking)
    long getAttributeAsInteger(const char* AttrName) const;
    unsigned long getAttributeAsUnsigned(const char* AttrName) const;
    /// return the named attribute as a double floating point (does type checking)
    double getAttributeAsFloat(const char* AttrName) const;
    /// return the named attribute as a double floating point (does type checking)
    const char* getAttribute(const char* AttrName) const;
    //@}

    /** @name additional file reading */
    //@{
    /// add a read request of a persistent object
    const char *addFile(const char* Name, Base::Persistence *Object);
    /// process the requested file writes
    void readFiles(zipios::ZipInputStream &zipstream) const;
    /// get all registered file names
    const std::vector<std::string>& getFilenames() const;
    bool isRegistered(Base::Persistence *Object) const;
    virtual void addName(const char*, const char*);
    virtual const char* getName(const char*) const;
    virtual bool doNameMapping() const;
    //@}

    /// Schema Version of the document
    int DocumentSchema;
    /// Version of FreeCAD that wrote this document
    std::string ProgramVersion;
    /// Version of the file format
    int FileVersion;

    /// sets simultaneously the global and local PartialRestore bits
    void setPartialRestore(bool on);

    void clearPartialRestoreDocumentObject(void);
    void clearPartialRestoreProperty(void);
    void clearPartialRestoreObject(void);

    /// return the status bits
    bool testStatus(ReaderStatus pos) const;
    /// set the status bits
    void setStatus(ReaderStatus pos, bool on);

protected:
    /// read the next element
    bool read(void);

    // -----------------------------------------------------------------------
    //  Handlers for the SAX ContentHandler interface
    // -----------------------------------------------------------------------
    /** @name Content handler */
    //@{
    virtual void startDocument();
    virtual void endDocument();
    virtual void startElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname, const XERCES_CPP_NAMESPACE_QUALIFIER Attributes& attrs);
    virtual void endElement  (const XMLCh* const uri, const XMLCh *const localname, const XMLCh *const qname);
#if (XERCES_VERSION_MAJOR == 2)
    virtual void characters         (const XMLCh* const chars, const unsigned int length);
    virtual void ignorableWhitespace(const XMLCh* const chars, const unsigned int length);
#else
    virtual void characters         (const XMLCh* const chars, const XMLSize_t length);
    virtual void ignorableWhitespace(const XMLCh* const chars, const XMLSize_t length);
#endif
    //@}

    /** @name Lexical handler */
    //@{
    virtual void startCDATA  ();
    virtual void endCDATA    ();
    //@}

    /** @name Document handler */
    //@{
    virtual void resetDocument();
    //@}


    // -----------------------------------------------------------------------
    //  Handlers for the SAX ErrorHandler interface
    // -----------------------------------------------------------------------
    /** @name Error handler */
    //@{
    void warning(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& exc);
    void error(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& exc);
    void fatalError(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& exc);
    void resetErrors();
    //@}


    int Level;
    std::string LocalName;
    std::string Characters;
    unsigned int CharacterCount;

    std::map<std::string,std::string> AttrMap;
    typedef std::map<std::string,std::string> AttrMapType;

    enum {
        None = 0,
        Chars,
        StartDocument,
        EndDocument,
        StartElement,
        StartEndElement,
        EndElement,
        StartCDATA,
        EndCDATA
    }   ReadType;


    FileInfo _File;
    XERCES_CPP_NAMESPACE_QUALIFIER SAX2XMLReader* parser;
    XERCES_CPP_NAMESPACE_QUALIFIER XMLPScanToken token;
    bool _valid;
    bool _verbose;

    struct FileEntry {
        std::string FileName;
        Base::Persistence *Object;
    };
    std::vector<FileEntry> FileList;
    std::vector<std::string> FileNames;

    std::bitset<32> StatusBits;
};

class BaseExport Reader : public std::istream
{
public:
    Reader(std::istream&, const std::string&, int version);
    std::istream& getStream();
    std::string getFileName() const;
    int getFileVersion() const;

private:
    std::istream& _str;
    std::string _name;
    int fileVersion;
};

}


#endif
