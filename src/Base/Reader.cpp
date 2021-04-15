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
# include <xercesc/sax2/XMLReaderFactory.hpp>
#endif

#include <locale>

#include <boost/ref.hpp>
#include <boost/algorithm/string/predicate.hpp>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Reader.h"
#include "Base64.h"
#include "Console.h"
#include "InputSource.h"
#include "Persistence.h"
#include "Sequencer.h"
#include "Stream.h"
#include "XMLTools.h"

#ifdef _MSC_VER
#include <zipios++/zipios-config.h>
#endif
#include <zipios++/zipinputstream.h>


FC_LOG_LEVEL_INIT("Base",true,true);

XERCES_CPP_NAMESPACE_USE

using namespace std;
using namespace Base;

namespace bio = boost::iostreams;

static std::string _ReaderContext;

#define _FC_READER_THROW(_excp,_msg) \
    FC_THROWM(_excp, _msg << "\nIn context: " << _ReaderContext);

#define FC_READER_THROW(_msg) _FC_READER_THROW(Base::XMLParseException, _msg)

ReaderContext::ReaderContext(const char *name)
{
    init(name);
}

ReaderContext::ReaderContext(const std::string &name)
{
    init(name.c_str());
}

void ReaderContext::init(const char *name)
{
    size = std::string::npos;
    if(name && name[0]) {
        size = _ReaderContext.size();
        _ReaderContext += "|";
        _ReaderContext += name;
    }
}

ReaderContext::~ReaderContext()
{
    if(size != std::string::npos)
        _ReaderContext.resize(size);
}

// ---------------------------------------------------------------------------
//  Base::XMLReader: Constructors and Destructor
// ---------------------------------------------------------------------------

Base::XMLReader::XMLReader(Base::Reader &reader, std::size_t bufsize)
  : DocumentSchema(0), ProgramVersion(""), FileVersion(reader.getFileVersion()), Level(0),
    CharacterOffset(-1), ReadType(None), _File(reader.getFileName()), _valid(false),
    _verbose(true), _reader(&reader), _ownReader(false)
{
    init(bufsize);
}

Base::XMLReader::XMLReader(const char *name, std::istream &str, std::size_t bufsize)
  : DocumentSchema(0), ProgramVersion(""), FileVersion(0), Level(0),
    CharacterOffset(-1), ReadType(None), _File(name), _valid(false),
    _verbose(true),_reader(new Base::Reader(str,name)), _ownReader(true)
{
    init(bufsize);
}

void Base::XMLReader::init(std::size_t bufsize) {
#ifdef _MSC_VER
    _reader->imbue(std::locale::empty());
#else
    _reader->imbue(std::locale::classic());
#endif

    // create the parser
    parser = XMLReaderFactory::createXMLReader();
    //parser->setFeature(XMLUni::fgSAX2CoreNameSpaces, false);
    //parser->setFeature(XMLUni::fgXercesSchema, false);
    //parser->setFeature(XMLUni::fgXercesSchemaFullChecking, false);
    //parser->setFeature(XMLUni::fgXercesIdentityConstraintChecking, false);
    //parser->setFeature(XMLUni::fgSAX2CoreNameSpacePrefixes, false);
    //parser->setFeature(XMLUni::fgSAX2CoreValidation, true);
    //parser->setFeature(XMLUni::fgXercesDynamic, true);

    parser->setInputBufferSize(bufsize);
    parser->setContentHandler(this);

    // We don't seem to need this?
    // parser->setLexicalHandler(this);

    parser->setErrorHandler(this);

    try {
        StdInputSource file(*_reader, _File.filePath().c_str());
        _valid = parser->parseFirst(file, token);
    }
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cerr << "Exception message is: \n"
             << message << "\n";
        XMLString::release(&message);
    }
    catch (const SAXParseException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        cerr << "Exception message is: \n"
             << message << "\n";
        XMLString::release(&message);
    }
#ifndef FC_DEBUG
    catch (...) {
        cerr << "Unexpected Exception \n";
    }
#endif
}

Base::XMLReader::~XMLReader()
{
    //  Delete the parser itself.  Must be done prior to calling Terminate, below.
    delete parser;
    if(_ownReader)
        delete _reader;
}

const char* Base::XMLReader::localName() const
{
    return LocalName.c_str();
}

unsigned int Base::XMLReader::getAttributeCount() const
{
    return static_cast<unsigned int>(AttrMap.size());
}

long Base::XMLReader::getAttributeAsInteger(const char* AttrName, const char *def) const
{
    return atol(getAttribute(AttrName,def));
}

unsigned long Base::XMLReader::getAttributeAsUnsigned(const char* AttrName, const char *def) const
{
    return strtoul(getAttribute(AttrName,def),0,10);
}

double Base::XMLReader::getAttributeAsFloat  (const char* AttrName, const char *def) const
{
    return atof(getAttribute(AttrName,def));
}

const char*  Base::XMLReader::getAttribute (const char* AttrName, const char *def) const
{
    AttrMapType::const_iterator pos = AttrMap.find(AttrName);

    if (pos != AttrMap.end()) {
        return pos->second.c_str();
    }
    else if(def) 
        return def;
    else {
        _FC_READER_THROW(Base::XMLAttributeError, "XML Attribute: '" << AttrName << "' not found");
    }
}

bool Base::XMLReader::hasAttribute (const char* AttrName) const
{
    return AttrMap.find(AttrName) != AttrMap.end();
}

bool Base::XMLReader::read()
{
    if(ReadType == EndDocument)
        FC_READER_THROW("End of document reached");

    ReadType = None;

    try {
        parser->parseNext(token);
    }
    catch (const XMLException& toCatch) {
#if 0
        char* message = XMLString::transcode(toCatch.getMessage());
        cerr << "Exception message is: \n"
             << message << "\n";
        XMLString::release(&message);
        return false;
#else
        char* message = XMLString::transcode(toCatch.getMessage());
        std::string what = message;
        XMLString::release(&message);
        _FC_READER_THROW(Base::XMLBaseException, what);
#endif
    }
    catch (const SAXParseException& toCatch) {
#if 0
        char* message = XMLString::transcode(toCatch.getMessage());
        cerr << "Exception message is: \n"
             << message << "\n";
        XMLString::release(&message);
        return false;
#else
        char* message = XMLString::transcode(toCatch.getMessage());
        std::string what = message;
        XMLString::release(&message);
        FC_READER_THROW(what);
#endif
    }
    catch (...) {
#if 0
        cerr << "Unexpected Exception \n" ;
        return false;
#else
        _FC_READER_THROW(Base::XMLBaseException, "Unexpected XML exception");
#endif
    }

    return true;
}

void Base::XMLReader::readElement(const char* ElementName, int *guard)
{
    endCharStream();

    AttrMap.clear();

    int currentLevel = Level;
    std::string currentName = LocalName;
    do {
        read();
        if (ReadType == EndElement && currentName == LocalName && currentLevel >= Level) {
            // we have reached the end of the element when calling this method
            // thus we must stop reading on.
            if(ElementName && currentName!=ElementName) {
                if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                // Missing element. Consider this as non-fatal
                    FC_WARN("Document XML element '" << (ElementName?ElementName:"") << "' not found\n"
                            << "In context: " << _ReaderContext);
                }
            }
            break;
        }
        else if (ReadType == EndDocument) {
            // the end of the document has been reached but we still try to continue on reading
            FC_READER_THROW("End of document reached");
        }

        if(Guards.size() && Level < *Guards.back())
            FC_READER_THROW("Document parsing error");

    } while ((ReadType != StartElement && ReadType != StartEndElement) ||
             (ElementName && LocalName != ElementName));

    if(guard) {
        if(ReadType == StartEndElement)
            *guard = Level;
        else
            *guard = Level-1;
        Guards.push_back(guard);
    }
}

int Base::XMLReader::level() const {
    return Level;
}

void Base::XMLReader::readEndElement(const char* ElementName, int *guard)
{
    endCharStream();

    int level = -1;
    if(guard) {
        level = *guard;
        if(Guards.empty() || Guards.back()!=guard || Level < level)
            FC_READER_THROW("Parsing error while reading end element '"
                    << (ElementName?ElementName:"?") << "'");
    }

    // if we are already at the end of the current element
    if ((ReadType == EndElement || ReadType == StartEndElement)
            && ElementName
            && LocalName == ElementName
            && (level<0 || level==Level))
    {
        if(guard)
            Guards.pop_back();
        return;
    }
    else if (ReadType == EndDocument) {
        // the end of the document has been reached but we still try to continue on reading
        FC_READER_THROW("End of document reached while reading end element '"
                    << (ElementName?ElementName:"?") << "'");
    }

    do {
        read();
        if (ReadType == EndDocument)
            break;
    } while (ReadType != EndElement
                || (ElementName
                    && (LocalName != ElementName
                        || (level>=0 && level!=Level))));

    if(guard)
        Guards.pop_back();
}

std::streamsize Base::XMLReader::read(char_type* s, std::streamsize n) {

    char_type *buf = s;
    if(CharacterOffset<0)
        return -1;

    for(;;) {
        std::streamsize copy_size = Characters.size()-CharacterOffset;
        if(n<copy_size)
            copy_size = n;
        std::memcpy(s,Characters.c_str()+CharacterOffset,copy_size);
        n -= copy_size;
        s += copy_size;
        CharacterOffset += copy_size;

        if(!n) 
            break;

        if(ReadType==Chars)
            read();
        else {
            CharacterOffset = -1;
            break;
        }
    }

    return s - buf;
}

void Base::XMLReader::endCharStream() {
    CharacterOffset = -1;
    CharStream.reset();
}

std::istream &Base::XMLReader::charStream() {
    if(!CharStream) 
        FC_READER_THROW("no current character stream");
    return *CharStream;
}

std::istream &Base::XMLReader::beginCharStream(bool base64) {
    if(CharStream) 
        FC_READER_THROW("recursive character stream");

    // TODO: An XML element can actually contain a mix of child elements and
    // characters. So we should not actually demand 'StartElement' here. But
    // with the current implementation of character stream, we cannot track
    // child elements and character content at the same time.
    if (ReadType == StartElement) {
        CharacterOffset = 0;
        read();
    } else if (ReadType == StartEndElement) {
        // If we are current at a self closing element, just leave the offset
        // as negative and do not read any characters. This will result in an
        // empty input stream for the caller.
        CharacterOffset = -1;
    } else
        FC_READER_THROW("invalid state while reading character stream");

    CharStream.reset(new bio::filtering_istream);
    auto f = static_cast<bio::filtering_istream*>(CharStream.get());
    if(base64)
        f->push(base64_decoder(80,false));
    f->push(boost::ref(*this));
    return *CharStream;
}

void Base::XMLReader::readCharacters(const char* filename, bool base64)
{
    Base::FileInfo fi(filename);
    Base::ofstream to(fi, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!to)
        _FC_READER_THROW(Base::FileException, "failed to open binary file " << filename);

    beginCharStream(base64) >> to.rdbuf();
    to.close();
    endCharStream();
}

std::string Base::XMLReader::readCharacters()
{
    std::stringstream ss;
    beginCharStream(false) >> ss.rdbuf();
    endCharStream();
    return ss.str();
}

void Base::XMLReader::readFiles()
{
    if(FileList.size()) {
        assert(!_reader->getParent());
        _reader->readFiles(*this);
    }
}

const char *Base::XMLReader::addFile(const char* Name, Base::Persistence *Object)
{
    if(_reader->getParent())
        return _reader->getParent()->addFile(Name,Object);

    FileEntry temp;
    temp.FileName = Name;
    temp.Object = Object;

    FileList.push_back(temp);
    FileNames.push_back( temp.FileName );

    return Name;
}

const std::vector<std::string>& Base::XMLReader::getFilenames() const
{
    if(_reader->getParent())
        return _reader->getParent()->getFilenames();

    return FileNames;
}

const std::vector<Base::XMLReader::FileEntry> &Base::XMLReader::getFileList() const {
    return FileList;
}

bool Base::XMLReader::isRegistered(Base::Persistence *Object) const
{
    if(_reader->getParent())
        return _reader->getParent()->isRegistered(Object);

    if (Object) {
        for (std::vector<FileEntry>::const_iterator it = FileList.begin(); it != FileList.end(); ++it) {
            if (it->Object == Object)
                return true;
        }
    }

    return false;
}

void Base::XMLReader::addName(const char *key, const char *value)
{
    if(_reader->getParent())
        _reader->getParent()->addName(key,value);
}

const char* Base::XMLReader::getName(const char* name) const
{
    if(_reader->getParent())
        return _reader->getParent()->getName(name);
    else
        return name;
}

bool Base::XMLReader::doNameMapping() const
{
    if(_reader->getParent())
        return _reader->getParent()->doNameMapping();
    else
        return false;
}

// ---------------------------------------------------------------------------
//  Base::XMLReader: Implementation of the SAX DocumentHandler interface
// ---------------------------------------------------------------------------
void Base::XMLReader::startDocument()
{
    ReadType = StartDocument;
}

void Base::XMLReader::endDocument()
{
    ReadType = EndDocument;
}

void Base::XMLReader::startElement(const XMLCh* const /*uri*/, const XMLCh* const localname, const XMLCh* const /*qname*/, const XERCES_CPP_NAMESPACE_QUALIFIER Attributes& attrs)
{
    Level++; // new scope
    LocalName = StrX(localname).c_str();

    // saving attributes of the current scope, delete all previously stored ones
    AttrMap.clear();
    for (unsigned int i = 0; i < attrs.getLength(); i++) {
        AttrMap[StrX(attrs.getQName(i)).c_str()] = StrXUTF8(attrs.getValue(i)).c_str();
    }

    ReadType = StartElement;
}

void Base::XMLReader::endElement  (const XMLCh* const /*uri*/, const XMLCh *const localname, const XMLCh *const /*qname*/)
{
    Level--; // end of scope
    LocalName = StrX(localname).c_str();

    if (ReadType == StartElement)
        ReadType = StartEndElement;
    else
        ReadType = EndElement;

    if(Guards.size() && Level<*Guards.back())
        *Guards.back() = INT_MAX;
}

void Base::XMLReader::startCDATA ()
{
    ReadType = StartCDATA;
}

void Base::XMLReader::endCDATA ()
{
    ReadType = EndCDATA;
}

#if (XERCES_VERSION_MAJOR == 2)
void Base::XMLReader::characters(const   XMLCh* const chars, const unsigned int len)
#else
void Base::XMLReader::characters(const   XMLCh* const chars, const XMLSize_t len)
#endif
{
    (void)len;
    ReadType = Chars;

    // We only capture characters when some one wants it
    if(CharacterOffset>=0) {
        Characters = StrXUTF8(chars).c_str();
        CharacterOffset = 0;
    }
}

#if (XERCES_VERSION_MAJOR == 2)
void Base::XMLReader::ignorableWhitespace( const   XMLCh* const /*chars*/, const unsigned int /*length*/)
#else
void Base::XMLReader::ignorableWhitespace( const   XMLCh* const /*chars*/, const XMLSize_t /*length*/)
#endif
{
    //fSpaceCount += length;
}

void Base::XMLReader::resetDocument()
{
    //fAttrCount = 0;
    //fCharacterCount = 0;
    //fElementCount = 0;
    //fSpaceCount = 0;
}


// ---------------------------------------------------------------------------
//  Base::XMLReader: Overrides of the SAX ErrorHandler interface
// ---------------------------------------------------------------------------
void Base::XMLReader::error(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e)
{
    // print some details to error output and throw an
    // exception to abort the parsing
    cerr << "Error at file " << StrX(e.getSystemId())
         << ", line " << e.getLineNumber()
         << ", char " << e.getColumnNumber() << endl;
    throw e;
}

void Base::XMLReader::fatalError(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e)
{
    // print some details to error output and throw an
    // exception to abort the parsing
    cerr << "Fatal Error at file " << StrX(e.getSystemId())
         << ", line " << e.getLineNumber()
         << ", char " << e.getColumnNumber() << endl;
    throw e;
}

void Base::XMLReader::warning(const XERCES_CPP_NAMESPACE_QUALIFIER SAXParseException& e)
{
    // print some details to error output and throw an
    // exception to abort the parsing
   cerr << "Warning at file " << StrX(e.getSystemId())
        << ", line " << e.getLineNumber()
        << ", char " << e.getColumnNumber() << endl;
   throw e;
}

void Base::XMLReader::resetErrors()
{
}

bool Base::XMLReader::testStatus(ReaderStatus pos) const
{
    return StatusBits.test(static_cast<size_t>(pos));
}

void Base::XMLReader::setStatus(ReaderStatus pos, bool on)
{
    if(testStatus(pos)!=on) {
        if(_reader->getParent() && pos == PartialRestore)
            _reader->getParent()->setStatus(pos,on);
        StatusBits.set((size_t)pos, on);
    }
}

void Base::XMLReader::setPartialRestore(bool on)
{
    setStatus(PartialRestore, on);
    setStatus(PartialRestoreInDocumentObject, on);
    setStatus(PartialRestoreInProperty, on);
    setStatus(PartialRestoreInObject, on);
}

void Base::XMLReader::clearPartialRestoreDocumentObject()
{
    setStatus(PartialRestoreInDocumentObject, false);
    setStatus(PartialRestoreInProperty, false);
    setStatus(PartialRestoreInObject, false);
}

void Base::XMLReader::clearPartialRestoreProperty()
{
    setStatus(PartialRestoreInProperty, false);
    setStatus(PartialRestoreInObject, false);
}

void Base::XMLReader::clearPartialRestoreObject()
{
    setStatus(PartialRestoreInObject, false);
}

// ----------------------------------------------------------

Base::Reader::Reader(std::istream &str, const std::string& name, Base::XMLReader *parent)
  : std::istream(str.rdbuf()), _name(name), _parent(parent)
{
}

Base::Reader::Reader(const std::string& name, Base::XMLReader *parent)
  : std::istream(nullptr), _name(name), _parent(parent)
{
}

const std::string &Base::Reader::getFileName() const
{
    return this->_name;
}

int Base::Reader::getFileVersion() const
{
    return _parent?_parent->FileVersion:0;
}

int Base::Reader::getDocumentSchema() const
{
    return _parent?_parent->DocumentSchema:0;
}

Base::XMLReader *Base::Reader::getParent() const {
    return _parent;
}

// ----------------------------------------------------------

Base::ZipReader::ZipReader(zipios::ZipInputStream &str, const std::string &name, Base::XMLReader *parent)
    :Base::Reader(str,name,parent),_stream(str)
{
}

void Base::ZipReader::readFiles(XMLReader &xmlReader)
{
    // It's possible that not all objects inside the document could be created, e.g. if a module
    // is missing that would know these object types. So, there may be data files inside the zip
    // file that cannot be read. We simply ignore these files.
    // On the other hand, however, it could happen that a file should be read that is not part of
    // the zip file. This happens e.g. if a document is written without GUI up but is read with GUI
    // up. In this case the associated GUI document asks for its file which is not part of the ZIP
    // file, then.
    // In either case it's guaranteed that the order of the files is kept.
    zipios::ConstEntryPointer entry;
    try {
        entry = _stream.getNextEntry();
    }
    catch (const std::exception&) {
        // There is no further file at all. This can happen if the
        // project file was created without GUI
        return;
    }
    const auto &FileList = xmlReader.getFileList();
    std::size_t it = 0;
    Base::SequencerLauncher seq("Importing project files...", FileList.size());
    while (entry->isValid() && it < FileList.size()) {
        auto jt = it;
        // Check if the current entry is registered, otherwise check the next registered files as soon as
        // both file names match
        while (jt < FileList.size() && entry->getName() != FileList[jt].FileName)
            ++jt;
        // If this condition is true both file names match and we can read-in the data, otherwise
        // no file name for the current entry in the zip was registered.
        if (jt < FileList.size()) {
            try {
                Base::ZipReader zipreader(_stream, FileList[jt].FileName, &xmlReader);
                FileList[jt].Object->RestoreDocFile(zipreader);
            } catch(Base::AbortException &e) {
                e.ReportException();
                FC_ERR("User abort when reading embedded file: " << FileList[jt].FileName);
                throw;
            } catch(Base::Exception &e) {
                e.ReportException();
                FC_ERR("Reading failed from embedded file: " << FileList[jt].FileName);
            } catch(...) {
                // For any exception we just continue with the next file.
                // It doesn't matter if the last reader has read more or
                // less data than the file size would allow.
                // All what we need to do is to notify the user about the
                // failure.
                FC_ERR("Reading failed from embedded file: " << FileList[jt].FileName);
            }
            // Go to the next registered file name
            it = jt + 1;
        }

        seq.next();

        // In either case we must go to the next entry
        try {
            entry = _stream.getNextEntry();
        }
        catch (const std::exception&) {
            // there is no further entry
            break;
        }
    }
}


// ----------------------------------------------------------

Base::FileReader::FileReader(const Base::FileInfo &fi, 
        const std::string &name, Base::XMLReader *parent)
    :Base::Reader(name.size()?name:fi.fileName(),parent)
    ,_dir(fi.dirPath())
    ,_stream(fi,std::ios::in|std::ios::binary)
{
    this->rdbuf(_stream.rdbuf());
}

void Base::FileReader::readFiles(Base::XMLReader &xmlReader) {
    const auto &FileList = xmlReader.getFileList();
    Base::SequencerLauncher seq("Importing project files...", FileList.size());
    std::string dirname = Base::FileInfo(_dir).fileName();
    for(size_t i=0; i<FileList.size(); ++i) {
        const auto &entry = FileList[i];
        Base::FileInfo fi(_dir+'/'+entry.FileName);
        try {
            Base::FileReader freader(fi, dirname+'/'+entry.FileName, &xmlReader);
            if(!freader._stream.is_open()) {
                std::string msg("Failed to open ");
                msg += fi.filePath();
                FC_ERR(msg);
                entry.Object->SetRestoreError(msg.c_str());
            } else
                entry.Object->RestoreDocFile(freader);
        } catch(Base::AbortException &e) {
            e.ReportException();
            FC_ERR("User abort when reading: " << fi.filePath());
            throw;
        } catch(Base::Exception &e) {
            e.ReportException();
            FC_ERR("Reading failed: " << fi.filePath());
        }
        catch(...) {
            // For any exception we just continue with the next file.
            // It doesn't matter if the last reader has read more or
            // less data than the file size would allow.
            // All what we need to do is to notify the user about the
            // failure.
            FC_ERR("Reading failed: " << fi.filePath());
        }
        seq.next();
    }
}
