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
//#include "PreCompiled.h"

//#include <locale>

#include "DocumentReader.h"

//#include "Reader.h"
//#include "Base64.h"
//#include "Console.h"
#include "InputSource.h"
//#include "Persistence.h"
//#include "Sequencer.h"
//#include "Stream.h"
#include "XMLTools.h"
//#ifdef _MSC_VER
//#include <zipios++/zipios-config.h>
//#endif
//#include <zipios++/zipinputstream.h>

#include <Base/Reader.h>
#include <Base/Parameter.h>
#ifndef _PreComp_
//#   include <cassert>
//#   include <memory>
#   include <xercesc/dom/DOM.hpp>
//#   include <xercesc/framework/LocalFileFormatTarget.hpp>
//#   include <xercesc/framework/LocalFileInputSource.hpp>
//#   include <xercesc/framework/MemBufFormatTarget.hpp>
//#   include <xercesc/framework/MemBufInputSource.hpp>
#   include <xercesc/parsers/XercesDOMParser.hpp>
//#   include <xercesc/sax/ErrorHandler.hpp>
//#   include <xercesc/sax/SAXParseException.hpp>
//#   include <sstream>
//#   include <string>
//#   include <utility>
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

	/*
    gOutputEncoding        = nullptr;
    gMyEOLSequence         = nullptr;

    gSplitCdataSections    = true;
    gDiscardDefaultContent = true;
    gUseFilter             = true;
    gFormatPrettyPrint     = true;
    */

}

//DocumentReader::~DocumentReader()
//{
    //delete parser;
//}
//int DocumentReader::LoadDocument(const XERCES_CPP_NAMESPACE_QUALIFIER InputSource& inputSource)
//int DocumentReader::LoadDocument(std::istream& Stream,std::string filename)
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

    XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument*   _pDocument = parser->adoptDocument();
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
                if (clChild->getAttributes()->getLength() > 0) {
					return static_cast<DOMElement*>(clChild);
                }
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
                //if (clChild->getAttributes()->getLength() > 0) {
					//return static_cast<DOMElement*>(clChild);
                //}
            }
        }
    }
    return nullptr;
}

XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *DocumentReader::FindNextElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode *Prev, const char* Type) const
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

/*
//CONTENT:
const char * DocumentReader::GetContent(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *DOMEl) const
{
    if (!DOMEl)
        return nullptr;
    return StrX(DOMEl->getAttribute(XStr("Value").unicodeForm())).c_str();
}

std::string DocumentReader::GetContentASCII(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *DOMEl) const
{
	//return std::string(StrXUTF8(pcElem->getNodeValue()).c_str() );
	//maybe its better to use getNodeValue()
    if (!DOMEl)
        return nullptr;
    return std::string( StrXUTF8(DOMEl->getAttribute(XStr("Value").unicodeForm())).c_str() );
}
*/

long DocumentReader::ContentToInt( const char* content ) const
{
	return atol( content );
}

unsigned long DocumentReader::ContentToUnsigned(const char* content) const
{
	return strtoul(content,nullptr,10);
}

double DocumentReader::ContentToFloat(const char* content) const
{
	return atof(content);
}

bool DocumentReader::ContentToBool(const char* content) const
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
    //stringLen
    return strdup( StrX( attr ).c_str() );//strdup is needed since pointer from strx only exists in context where StrX() is created.
}

/*
unsigned int Base::XMLReader::getAttributeCount() const
{
    return static_cast<unsigned int>(AttrMap.size());
}
*/

//Status

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


