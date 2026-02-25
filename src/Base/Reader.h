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

#pragma once

#include <bitset>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <xercesc/framework/XMLPScanToken.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>

#include <boost/iostreams/categories.hpp>

#include "FileInfo.h"


namespace zipios
{
class ZipInputStream;
}

namespace XERCES_CPP_NAMESPACE
{
class Attributes;
class DefaultHandler;
class SAX2XMLReader;
}  // namespace XERCES_CPP_NAMESPACE

namespace Base
{
class Persistence;

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
    _cValue = reader.getAttribute<const char*>("value");
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
        str << ind(indent+1) << "<Property name=\"" << it->first << "\" type=\"" <<
it->second->getTypeId().getName() << "\">" ; it->second->Save(indent+2,str); str << "</Property>" <<
endl;
    }
    str << ind(indent) << "</Properties>" << endl;
}

void PropertyContainer::Restore(Base::Reader &reader)
{
    reader.readElement("Properties");
    int Cnt = reader.getAttribute<long>("Count");

    for(int i=0 ;i<Cnt ;i++)
    {
        reader.readElement("Property");
        string PropName = reader.getAttribute<const char*>("name");
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
class BaseExport XMLReader: public XERCES_CPP_NAMESPACE::DefaultHandler
{
public:
    enum ReaderStatus
    {
        PartialRestore = 0,  // This bit indicates that a partial restore took place somewhere in
                             // this Document
        PartialRestoreInDocumentObject = 1,  // This bit is local to the DocumentObject being read
                                             // indicating a partial restore therein
        PartialRestoreInProperty = 2,        // Local to the Property
        PartialRestoreInObject = 3           // Local to the object partially restored itself
    };
    /// open the file and read the first element
    XMLReader(const char* FileName, std::istream&);
    ~XMLReader() override;

    /** @name boost iostream device interface */
    //@{
    using category = boost::iostreams::source_tag;
    using char_type = char;
    std::streamsize read(char_type* s, std::streamsize n);
    //@}

    bool isValid() const
    {
        return _valid;
    }
    bool isVerbose() const
    {
        return _verbose;
    }
    void setVerbose(bool on)
    {
        _verbose = on;
    }

    /** @name Parser handling */
    //@{
    /// get the local name of the current Element
    const char* localName() const;
    /// get the current element level
    int level() const;

    /// return true if the end of an element is reached, false otherwise
    bool isEndOfElement() const;

    /// return true if the on the start of the document, false otherwise
    bool isStartOfDocument() const;

    /// return true if the end of the document is reached, false otherwise
    bool isEndOfDocument() const;

    /// read until a start element is found (\<name\>) or start-end element (\<name/\>) (with
    /// special name if given)
    void readElement(const char* ElementName = nullptr);

    /// Read in the next element. Return true if it succeeded and false otherwise
    bool readNextElement();

    /** read until an end element is found
     *
     * @param ElementName: optional end element name to look for. If given, then
     * the parser will read until this name is found.
     *
     * @param level: optional level to look for. If given, then the parser will
     * read until this level. Note that the parse only increase the level when
     * finding a start element, not start-end element, and decrease the level
     * after finding an end element. So, if you obtain the parser level after
     * calling readElement(), you should specify a level minus one when calling
     * this function. This \c level parameter is only useful if you know the
     * child element may have the same name as its parent, otherwise, using \c
     * ElementName is enough.
     */
    void readEndElement(const char* ElementName = nullptr, int level = -1);
    /// read until characters are found
    void readCharacters(const char* filename, CharStreamFormat format = CharStreamFormat::Raw);

    /** Obtain an input stream for reading characters
     *
     *  @return Return a input stream for reading characters. The stream will be
     *  auto destroyed when you call with readElement() or readEndElement(), or
     *  you can end it explicitly with endCharStream().
     */
    std::istream& beginCharStream(CharStreamFormat format = CharStreamFormat::Raw);
    /// Manually end the current character stream
    void endCharStream();
    /// Obtain the current character stream
    std::istream& charStream();
    //@}

    /// read binary file
    void readBinFile(const char*);
    //@}

    /** @name Attribute handling */
    //@{
    /// get the number of attributes of the current element
    unsigned int getAttributeCount() const;
    /// check if the read element has a special attribute
    bool hasAttribute(const char* AttrName) const;

private:
    // all explicit template instantiations - this is for getting
    // a compile error, rather than linker error.
    template<typename T>
    static constexpr bool instantiated = std::is_same_v<T, bool> || std::is_same_v<T, const char*>
        || std::is_same_v<T, double> || std::is_same_v<T, int> || std::is_same_v<T, long>
        || std::is_same_v<T, unsigned long>;

public:
    /// return the named attribute as T (does type checking); if missing return defaultValue.
    /// If defaultValue is not set, it will default to the default initialization of the
    /// corresponding type; bool: false, int: 0, ... as if one had used defaultValue=bool{}
    /// or defaultValue=int{}
    // General template, mark delete as it's not implemented, and should not be used!
    template<typename T>
        requires Base::XMLReader::instantiated<T>
    T getAttribute(const char* AttrName, T defaultValue) const;

    /// No default? Will throw exception if not found!
    template<typename T>
        requires Base::XMLReader::instantiated<T>
    T getAttribute(const char* AttrName) const;

    /// E.g. std::string, QString
    template<typename T>
    T getAttribute(const char* AttrName) const
    {
        return T(getAttribute<const char*>(AttrName));
    }
    /// E.g. std::string, QString
    template<typename T>
    T getAttribute(const char* AttrName, T defaultValue) const
    {
        return T(getAttribute<const char*>(AttrName, defaultValue));
    }

    /// Enum classes
    template<typename T>
        requires std::is_enum_v<T>
    T getAttribute(const char* AttrName, T defaultValue) const
    {
        return static_cast<T>(
            getAttribute<unsigned long>(AttrName, static_cast<unsigned long>(defaultValue))
        );
    }
    /// Enum classes
    template<typename T>
        requires std::is_enum_v<T>
    T getAttribute(const char* AttrName) const
    {
        return static_cast<T>(getAttribute<unsigned long>(AttrName));
    }

    /** @name additional file reading */
    //@{
    /// add a read request of a persistent object
    const char* addFile(const char* Name, Base::Persistence* Object);
    /// process the requested file writes
    void readFiles(zipios::ZipInputStream& zipstream) const;
    /// Returns whether reader has any registered filenames
    bool hasFilenames() const;
    /// returns true if reading the file \a filename has failed
    bool hasReadFailed(const std::string& filename) const;
    bool isRegistered(Base::Persistence* Object) const;
    virtual void addName(const char*, const char*);
    virtual const char* getName(const char*) const;
    virtual bool doNameMapping() const;
    //@}

    /// Schema Version of the document
    int DocumentSchema {0};
    /// Version of FreeCAD that wrote this document
    std::string ProgramVersion;
    /// Version of the file format
    int FileVersion {0};

    /// sets simultaneously the global and local PartialRestore bits
    void setPartialRestore(bool on);

    void clearPartialRestoreDocumentObject();
    void clearPartialRestoreProperty();
    void clearPartialRestoreObject();

    /// return the status bits
    bool testStatus(ReaderStatus pos) const;
    /// set the status bits
    void setStatus(ReaderStatus pos, bool on);

protected:
    /// read the next element
    bool read();

    // -----------------------------------------------------------------------
    //  Handlers for the SAX ContentHandler interface
    // -----------------------------------------------------------------------
    /** @name Content handler */
    //@{
    void startDocument() override;
    void endDocument() override;
    void startElement(
        const XMLCh* const uri,
        const XMLCh* const localname,
        const XMLCh* const qname,
        const XERCES_CPP_NAMESPACE::Attributes& attrs
    ) override;
    void endElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname) override;
    void characters(const XMLCh* const chars, const XMLSize_t length) override;
    void ignorableWhitespace(const XMLCh* const chars, const XMLSize_t length) override;
    //@}

    /** @name Lexical handler */
    //@{
    void startCDATA() override;
    void endCDATA() override;
    //@}

    /** @name Document handler */
    //@{
    void resetDocument() override;
    //@}


    // -----------------------------------------------------------------------
    //  Handlers for the SAX ErrorHandler interface
    // -----------------------------------------------------------------------
    /** @name Error handler */
    //@{
    void warning(const XERCES_CPP_NAMESPACE::SAXParseException& exc) override;
    void error(const XERCES_CPP_NAMESPACE::SAXParseException& exc) override;
    void fatalError(const XERCES_CPP_NAMESPACE::SAXParseException& exc) override;
    void resetErrors() override;
    //@}

private:
    int Level {0};
    std::string LocalName;
    std::string Characters;
    unsigned int CharacterCount {0};
    std::streamsize CharacterOffset {-1};

    std::map<std::string, std::string> AttrMap;
    using AttrMapType = std::map<std::string, std::string>;

    enum
    {
        None = 0,
        Chars,
        StartDocument,
        EndDocument,
        StartElement,
        StartEndElement,
        EndElement,
        StartCDATA,
        EndCDATA
    } ReadType {None};


    FileInfo _File;
    XERCES_CPP_NAMESPACE::SAX2XMLReader* parser;
    XERCES_CPP_NAMESPACE::XMLPScanToken token;
    bool _valid {false};
    bool _verbose {true};

public:
    struct FileEntry
    {
        std::string FileName;
        Base::Persistence* Object;
    };
    std::vector<FileEntry> FileList;

private:
    mutable std::vector<std::string> FailedFiles;

    std::bitset<32> StatusBits;

    std::unique_ptr<std::istream> CharStream;
};

class BaseExport Reader: public std::istream
{
public:
    Reader(std::istream&, const std::string&, int version);
    std::istream& getStream();
    std::string getFileName() const;
    int getFileVersion() const;
    void initLocalReader(std::shared_ptr<Base::XMLReader>);
    std::shared_ptr<Base::XMLReader> getLocalReader() const;

private:
    std::istream& _str;
    std::string _name;
    int fileVersion;
    std::shared_ptr<Base::XMLReader> localreader;
};

}  // namespace Base
