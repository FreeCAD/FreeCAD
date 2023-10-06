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
#include "DocumentReader.h"
#include "InputSource.h"
#include "XMLTools.h"

#include <Base/Reader.h>
#include <Base/Parameter.h>
#include "Persistence.h"
#include "Sequencer.h"


#ifdef _MSC_VER
#include <zipios++/zipios-config.h>
#endif
#include <zipios++/zipinputstream.h>

#ifndef _PreComp_
//#   include <xercesc/dom/DOM.hpp>
#   include <xercesc/parsers/XercesDOMParser.hpp>
#	include <xercesc/dom/DOMException.hpp>
#	include <xercesc/dom/DOMElement.hpp>
#endif

#ifdef _MSC_VER
# define strdup _strdup
#endif

XERCES_CPP_NAMESPACE_USE

//using namespace std;
using namespace Base;

// ---------------------------------------------------------------------------
//  DocumentReader: Constructors and Destructor
// ---------------------------------------------------------------------------
static XercesDOMParser::ValSchemes    gValScheme       = XercesDOMParser::Val_Auto;
DocumentReader::DocumentReader()
{
	gDoNamespaces          = false;
    gDoSchema              = false;
    gSchemaFullChecking    = false;
    gDoCreate              = true;
}

int DocumentReader::LoadDocument(Base::Reader& reader)
{
	FileInfo _File( reader.getFileName() );
	StdInputSource inputSource(reader, _File.filePath().c_str());
	
    //
    //  Create our parser, then attach an error handler to the parser.
    //  The parser will call back to methods of the ErrorHandler if it
    //  discovers errors during the course of parsing the XML document.
    //
    XercesDOMParser *parser = new XercesDOMParser;
    parser->setValidationScheme(gValScheme);
    parser->setDoNamespaces(gDoNamespaces);
    parser->setDoSchema(gDoSchema);
    parser->setValidationSchemaFullChecking(gSchemaFullChecking);
    parser->setCreateEntityReferenceNodes(gDoCreate);

    DOMTreeErrorReporter *errReporter = new DOMTreeErrorReporter();
    parser->setErrorHandler(errReporter);
    //
    //  Parse the XML file, catching any XML exceptions that might propagate
    //  out of it.
    //
    bool errorsOccured = false;
    try {
        parser->parse(inputSource);
    }
    catch (const XMLException& e) {
        std::cerr << "An error occurred during parsing\n   Message: "
        << StrX(e.getMessage()) << std::endl;
        errorsOccured = true;
    }
    catch (const DOMException& e) {
        std::cerr << "A DOM error occurred during parsing\n   DOMException code: "
        << e.code << std::endl;
        errorsOccured = true;
    }
    catch (...) {
        std::cerr << "An error occurred during parsing\n " << std::endl;
        errorsOccured = true;
    }

    if (errorsOccured) {
        delete parser;
        delete errReporter;
        return 0;
    }

    XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* _pDocument = parser->adoptDocument();
    delete parser;
    delete errReporter;

    if (!_pDocument)
        throw XMLBaseException("Malformed Parameter document: Invalid document");

    DOMElement* rootElem = _pDocument->getDocumentElement();
    if (!rootElem)
        throw XMLBaseException("Malformed Parameter document: Root group not found");
    
    _pGroupNode = rootElem;

    if (!_pGroupNode){
        throw XMLBaseException("Malformed document.");
	}

    return 1;
}

XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *DocumentReader::GetRootElement() const
{
	//if (!_pGroupNode)
        //return nullptr;
	return _pGroupNode;
}

XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *DocumentReader::FindElement(const char* Type) const
{
	if(!Type)
		return nullptr;
		
    for (DOMNode *clChild = _pGroupNode->getFirstChild(); clChild != nullptr;  clChild = clChild->getNextSibling()) {
        if (clChild->getNodeType() == DOMNode::ELEMENT_NODE) {
            if (!strcmp(Type,StrX(clChild->getNodeName()).c_str())) {
            	return static_cast<DOMElement*>(clChild);
            }
        }
    }
    return nullptr;
}

XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *DocumentReader::FindElementByField(
	XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* Start,
	const char* TypeEl,const char* field_name,const char* field_value) const
{
	if(!TypeEl || !field_name ||!field_value)
		return nullptr;
	for (DOMNode *clChild = Start; clChild != nullptr;  clChild = clChild->getNextSibling()) {
		//auto cast = static_cast<DOMElement*>(clChild);
    	const char* attr = GetAttribute( static_cast<DOMElement*>(clChild), field_name );
		if(attr){
			if( !strcmp( attr, field_value ) ){
				return static_cast<DOMElement*>(clChild);;
			}
		}
    }
    return nullptr;
}

XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *DocumentReader::FindElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* Start, const char* Type) const
{
	if(!Start || !Type)
		return nullptr;
    for (DOMNode *clChild = Start->getFirstChild(); clChild != nullptr;  clChild = clChild->getNextSibling()) {
        if (clChild->getNodeType() == DOMNode::ELEMENT_NODE) {
            if (!strcmp(Type,StrX(clChild->getNodeName()).c_str())) {
            	return static_cast<DOMElement*>(clChild);
            }
        }
    }
    return nullptr;
}

XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *DocumentReader::FindNextElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *Prev, const char* Type) const
{
	if (!Prev || !Type)
        return nullptr;
    DOMNode *clChild = Prev;
    while ((clChild = clChild->getNextSibling()) != nullptr) {
        if (clChild->getNodeType() == DOMNode::ELEMENT_NODE) {
            // the right node Type
            if (!strcmp(Type,StrX(clChild->getNodeName()).c_str())) {
                return static_cast<DOMElement*>(clChild);
            }
        }
    }
    return nullptr;
}

long DocumentReader::ContentToInt( const char* content )
{
	return atol( content );
}

unsigned long DocumentReader::ContentToUnsigned(const char* content)
{
	return strtoul(content,nullptr,10);
}

double DocumentReader::ContentToFloat(const char* content)
{
	return atof(content);
}

bool DocumentReader::ContentToBool(const char* content)
{
	if (strcmp(content,"1"))
        return false;
    else
        return true;
}

//ATTRIBUTE:
const char * DocumentReader::GetAttribute(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *DOMEl, const char* Attr) const
{
	if(!Attr)
		return nullptr;
    XStr xstr( Attr );
	bool hasAttr = DOMEl->hasAttribute(xstr.unicodeForm());
    if (!hasAttr){
        return nullptr;
    }
    const XMLCh * attr = DOMEl->getAttribute( xstr.unicodeForm() );
    return strdup( StrX( attr ).c_str() );
}

const char * DocumentReader::GetAttribute(const char* Attr) const
{
	if(!Attr)
		return nullptr;
	XStr xstr( Attr );
	bool hasAttr = _pGroupNode->hasAttribute(xstr.unicodeForm());
    if (!hasAttr){
        return nullptr;
    }
    const XMLCh * attr = _pGroupNode->getAttribute( xstr.unicodeForm() );
    return strdup( StrX( attr ).c_str() );//strdup is needed since pointer from strx only exists in context where StrX() is created.
}
//Status
void Base::DocumentReader::setPartialRestore(bool on)
{
    setStatus(PartialRestore, on);
    setStatus(PartialRestoreInDocumentObject, on);
    setStatus(PartialRestoreInProperty, on);
    setStatus(PartialRestoreInObject, on);
}

void Base::DocumentReader::clearPartialRestoreProperty()
{
    setStatus(PartialRestoreInProperty, false);
    setStatus(PartialRestoreInObject, false);
}

bool Base::DocumentReader::testStatus(ReaderStatus pos) const
{
    return StatusBits.test(static_cast<size_t>(pos));
}

void Base::DocumentReader::setStatus(ReaderStatus pos, bool on)
{
    StatusBits.set(static_cast<size_t>(pos), on);
}

const char *Base::DocumentReader::addFile(const char* Name, Base::Persistence *Object)
{
    FileEntry temp;
    temp.FileName = Name;
    temp.Object = Object;

    FileList.push_back(temp);
    FileNames.push_back( temp.FileName );

    return Name;
}

void Base::DocumentReader::readFiles(zipios::ZipInputStream &zipstream) const
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
        entry = zipstream.getNextEntry();
    }
    catch (const std::exception&) {
        // There is no further file at all. This can happen if the
        // project file was created without GUI
        return;
    }
    std::vector<FileEntry>::const_iterator it = FileList.begin();
    Base::SequencerLauncher seq("Importing project files...", FileList.size());
    while (entry->isValid() && it != FileList.end()) {
        std::vector<FileEntry>::const_iterator jt = it;
        // Check if the current entry is registered, otherwise check the next registered files as soon as
        // both file names match
        while (jt != FileList.end() && entry->getName() != jt->FileName)
            ++jt;
        // If this condition is true both file names match and we can read-in the data, otherwise
        // no file name for the current entry in the zip was registered.
        if (jt != FileList.end()) {
            try {
        		Base::Reader reader(zipstream, jt->FileName, FileVersion);
	            jt->Object->RestoreDocFile(reader);
                if (reader.getLocalReader())
                    reader.getLocalReader()->readFiles(zipstream);
                if (reader.getLocalDocReader())
                	reader.getLocalDocReader()->readFiles(zipstream);
                    
            }catch(...) {
                // For any exception we just continue with the next file.
                // It doesn't matter if the last reader has read more or
                // less data than the file size would allow.
                // All what we need to do is to notify the user about the
                // failure.
                Base::Console().Error("Reading failed from embedded file(DocumentReader): %s\n", entry->toString().c_str());
            }
            // Go to the next registered file name
            it = jt + 1;
        }

        seq.next();

        // In either case we must go to the next entry
        try {
            entry = zipstream.getNextEntry();
        }
        catch (const std::exception&) {
            // there is no further entry
            break;
        }
    }
}
