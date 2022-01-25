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


#include "PreCompiled.h"

#ifndef _PreComp_
#   include <cassert>
#   include <memory>
#   include <xercesc/util/PlatformUtils.hpp>
#   include <xercesc/util/XercesVersion.hpp>
#   include <xercesc/dom/DOM.hpp>
#   include <xercesc/dom/DOMImplementation.hpp>
#   include <xercesc/dom/DOMImplementationLS.hpp>
#   if (XERCES_VERSION_MAJOR == 2)
#   include <xercesc/dom/DOMWriter.hpp>
#   endif
#   include <xercesc/framework/StdOutFormatTarget.hpp>
#   include <xercesc/framework/LocalFileFormatTarget.hpp>
#   include <xercesc/framework/LocalFileInputSource.hpp>
#   include <xercesc/framework/MemBufFormatTarget.hpp>
#   include <xercesc/framework/MemBufInputSource.hpp>
#   include <xercesc/parsers/XercesDOMParser.hpp>
#   include <xercesc/util/XMLUni.hpp>
#   include <xercesc/util/XMLUniDefs.hpp>
#   include <xercesc/util/XMLString.hpp>
#   include <xercesc/sax/ErrorHandler.hpp>
#   include <xercesc/sax/SAXParseException.hpp>
#   include <fcntl.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   ifdef FC_OS_WIN32
#   include <io.h>
#   endif
#   include <sstream>
#   include <cstdio>
#endif


#include <fcntl.h>
#ifdef FC_OS_LINUX
#   include <unistd.h>
#endif

#include "Parameter.h"
#include "Parameter.inl"
#include "Exception.h"
#include "Console.h"


//#ifdef XERCES_HAS_CPP_NAMESPACE
//  using namespace xercesc;
//#endif

XERCES_CPP_NAMESPACE_USE
using namespace Base;


#include "XMLTools.h"

//**************************************************************************
//**************************************************************************
// private classes declaration:
// - DOMTreeErrorReporter
// - StrX
// - DOMPrintFilter
// - DOMPrintErrorHandler
// - XStr
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


class DOMTreeErrorReporter : public ErrorHandler
{
public:
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    DOMTreeErrorReporter() :
            fSawErrors(false) {
    }

    ~DOMTreeErrorReporter() {
    }


    // -----------------------------------------------------------------------
    //  Implementation of the error handler interface
    // -----------------------------------------------------------------------
    void warning(const SAXParseException& toCatch);
    void error(const SAXParseException& toCatch);
    void fatalError(const SAXParseException& toCatch);
    void resetErrors();

    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    bool getSawErrors() const;

    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fSawErrors
    //      This is set if we get any errors, and is queryable via a getter
    //      method. Its used by the main code to suppress output if there are
    //      errors.
    // -----------------------------------------------------------------------
    bool    fSawErrors;
};


#if (XERCES_VERSION_MAJOR == 2)
class DOMPrintFilter : public DOMWriterFilter
{
public:

    /** @name Constructors */
    DOMPrintFilter(unsigned long whatToShow = DOMNodeFilter::SHOW_ALL);
    //@{

    /** @name Destructors */
    ~DOMPrintFilter() {};
    //@{

    /** @ interface from DOMWriterFilter */
    virtual short acceptNode(const XERCES_CPP_NAMESPACE_QUALIFIER DOMNode*) const;
    //@{

    virtual unsigned long getWhatToShow() const {
        return fWhatToShow;
    };

    virtual void          setWhatToShow(unsigned long toShow) {
        fWhatToShow = toShow;
    };

private:
    // unimplemented copy ctor and assignment operator
    DOMPrintFilter(const DOMPrintFilter&);
    DOMPrintFilter & operator = (const DOMPrintFilter&);

    unsigned long fWhatToShow;

};
#else
class DOMPrintFilter : public DOMLSSerializerFilter
{
public:

    /** @name Constructors */
    DOMPrintFilter(ShowType whatToShow = DOMNodeFilter::SHOW_ALL);
    //@{

    /** @name Destructors */
    ~DOMPrintFilter() {}
    //@{

    /** @ interface from DOMWriterFilter */
    virtual FilterAction acceptNode(const XERCES_CPP_NAMESPACE_QUALIFIER DOMNode*) const;
    //@{

    virtual ShowType getWhatToShow() const {
        return fWhatToShow;
    }

private:
    // unimplemented copy ctor and assignment operator
    DOMPrintFilter(const DOMPrintFilter&);
    DOMPrintFilter & operator = (const DOMPrintFilter&);

   ShowType fWhatToShow;
};
#endif
class DOMPrintErrorHandler : public DOMErrorHandler
{
public:

    DOMPrintErrorHandler() {}
    ~DOMPrintErrorHandler() {}

    /** @name The error handler interface */
    bool handleError(const DOMError& domError);
    void resetErrors() {}

private :
    /* Unimplemented constructors and operators */
    DOMPrintErrorHandler(const DOMErrorHandler&);
    void operator=(const DOMErrorHandler&);

};


inline bool DOMTreeErrorReporter::getSawErrors() const
{
    return fSawErrors;
}


//**************************************************************************
//**************************************************************************
// ParameterManager
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//**************************************************************************
// Construction/Destruction


/** Default construction
  */
ParameterGrp::ParameterGrp(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *GroupNode,const char* sName)
        : Base::Handled(), Subject<const char*>(),_pGroupNode(GroupNode)
{
    if (sName) _cName=sName;
}


/** Destruction
  * complete destruction of the object
  */
ParameterGrp::~ParameterGrp()
{
}

//**************************************************************************
// Access methods

void ParameterGrp::copyTo(Base::Reference<ParameterGrp> Grp)
{
    // delete previous content
    Grp->Clear();

    // copy all
    insertTo(Grp);
}

void ParameterGrp::insertTo(Base::Reference<ParameterGrp> Grp)
{
    // copy group
    std::vector<Base::Reference<ParameterGrp> > Grps = GetGroups();
    std::vector<Base::Reference<ParameterGrp> >::iterator It1;
    for (It1 = Grps.begin();It1 != Grps.end();++It1)
        (*It1)->insertTo(Grp->GetGroup((*It1)->GetGroupName()));

    // copy strings
    std::vector<std::pair<std::string,std::string> > StringMap = GetASCIIMap();
    std::vector<std::pair<std::string,std::string> >::iterator It2;
    for (It2 = StringMap.begin();It2 != StringMap.end();++It2)
        Grp->SetASCII(It2->first.c_str(),It2->second.c_str());

    // copy bool
    std::vector<std::pair<std::string,bool> > BoolMap = GetBoolMap();
    std::vector<std::pair<std::string,bool> >::iterator It3;
    for (It3 = BoolMap.begin();It3 != BoolMap.end();++It3)
        Grp->SetBool(It3->first.c_str(),It3->second);

    // copy int
    std::vector<std::pair<std::string,long> > IntMap = GetIntMap();
    std::vector<std::pair<std::string,long> >::iterator It4;
    for (It4 = IntMap.begin();It4 != IntMap.end();++It4)
        Grp->SetInt(It4->first.c_str(),It4->second);

    // copy float
    std::vector<std::pair<std::string,double> > FloatMap = GetFloatMap();
    std::vector<std::pair<std::string,double> >::iterator It5;
    for (It5 = FloatMap.begin();It5 != FloatMap.end();++It5)
        Grp->SetFloat(It5->first.c_str(),It5->second);

    // copy uint
    std::vector<std::pair<std::string,unsigned long> > UIntMap = GetUnsignedMap();
    std::vector<std::pair<std::string,unsigned long> >::iterator It6;
    for (It6 = UIntMap.begin();It6 != UIntMap.end();++It6)
        Grp->SetUnsigned(It6->first.c_str(),It6->second);
}

void ParameterGrp::exportTo(const char* FileName)
{
    ParameterManager Mngr;

    Mngr.CreateDocument();

    // copy all into the new document
    insertTo(Mngr.GetGroup("BaseApp"));

    Mngr.SaveDocument(FileName);
}

void ParameterGrp::importFrom(const char* FileName)
{
    ParameterManager Mngr;

    if (Mngr.LoadDocument(FileName) != 1)
        throw FileException("ParameterGrp::import() cannot load document", FileName);

    Mngr.GetGroup("BaseApp")->copyTo(Base::Reference<ParameterGrp>(this));
}

void ParameterGrp::insert(const char* FileName)
{
    ParameterManager Mngr;

    if (Mngr.LoadDocument(FileName) != 1)
        throw FileException("ParameterGrp::import() cannot load document", FileName);

    Mngr.GetGroup("root")->insertTo(Base::Reference<ParameterGrp>(this));
}

Base::Reference<ParameterGrp> ParameterGrp::GetGroup(const char* Name)
{
    std::string cName = Name;

    std::string::size_type pos = cName.find('/');

    // is there a path separator ?
    if (pos == std::string::npos) {
        return _GetGroup(Name);
    }
    else if (pos == cName.size()) {
        // ending slash! cut it away
        cName.erase(pos);
        return _GetGroup(cName.c_str());
    }
    else if (pos == 0) {
        // a leading slash is not handled (root unknown)
        //throw FCException("ParameterGrp::GetGroup() leading slash not allowed");
        // remove leading slash
        cName.erase(0,1);
        // subsequent call
        return GetGroup(cName.c_str());
    }
    else {
        // path, split the first path
        std::string cTemp;
        // getting the first part
        cTemp.assign(cName,0,pos);
        // removing the first part from the original
        cName.erase(0,pos+1);
        //subsequent call
        return _GetGroup(cTemp.c_str())->GetGroup(cName.c_str());
    }
}

Base::Reference<ParameterGrp> ParameterGrp::_GetGroup(const char* Name)
{
    Base::Reference<ParameterGrp> rParamGrp;
    DOMElement *pcTemp;

    // already created?
    if ((rParamGrp=_GroupMap[Name]).isValid()) {
        // just return the already existing Group handle
        return rParamGrp;
    }

    // search if Group node already there
    pcTemp = FindOrCreateElement(_pGroupNode,"FCParamGroup",Name);

    // create and register handle
    rParamGrp = Base::Reference<ParameterGrp> (new ParameterGrp(pcTemp,Name));
    _GroupMap[Name] = rParamGrp;

    return rParamGrp;
}

std::vector<Base::Reference<ParameterGrp> > ParameterGrp::GetGroups(void)
{
    Base::Reference<ParameterGrp> rParamGrp;
    std::vector<Base::Reference<ParameterGrp> >  vrParamGrp;
    DOMElement *pcTemp; //= _pGroupNode->getFirstChild();
    std::string Name;

    pcTemp = FindElement(_pGroupNode,"FCParamGroup");

    while (pcTemp) {
        Name = StrX(static_cast<DOMElement*>(pcTemp)->getAttributes()->getNamedItem(XStr("Name").unicodeForm())->getNodeValue()).c_str();
        // already created?
        if (!(rParamGrp=_GroupMap[Name]).isValid()) {
            rParamGrp = Base::Reference<ParameterGrp> (new ParameterGrp(static_cast<DOMElement*>(pcTemp),Name.c_str()));
            _GroupMap[Name] = rParamGrp;
        }
        vrParamGrp.push_back( rParamGrp );
        // go to next
        pcTemp = FindNextElement(pcTemp,"FCParamGroup");
    }

    return vrParamGrp;
}

/// test if this group is empty
bool ParameterGrp::IsEmpty() const
{
    if ( _pGroupNode->getFirstChild() )
        return false;
    else
        return true;
}

/// test if a special sub group is in this group
bool ParameterGrp::HasGroup(const char* Name) const
{
    if ( _GroupMap.find(Name) != _GroupMap.end() )
        return true;

    if ( FindElement(_pGroupNode,"FCParamGroup",Name) != nullptr )
        return true;

    return false;
}

bool ParameterGrp::GetBool(const char* Name, bool bPreset) const
{
    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCBool",Name);
    // if not return preset
    if (!pcElem) return bPreset;
    // if yes check the value and return
    if (strcmp(StrX(pcElem->getAttribute(XStr("Value").unicodeForm())).c_str(),"1"))
        return false;
    else
        return true;
}

void  ParameterGrp::SetBool(const char* Name, bool bValue)
{
    // find or create the Element
    DOMElement *pcElem = FindOrCreateElement(_pGroupNode,"FCBool",Name);
    if (pcElem) {
        // and set the value
        pcElem->setAttribute(XStr("Value").unicodeForm(), XStr(bValue?"1":"0").unicodeForm());
        // trigger observer
        Notify(Name);
    }
}

std::vector<bool> ParameterGrp::GetBools(const char * sFilter) const
{
    std::vector<bool>  vrValues;
    DOMElement *pcTemp;// = _pGroupNode->getFirstChild();
    std::string Name;

    pcTemp = FindElement(_pGroupNode,"FCBool");
    while ( pcTemp) {
        Name = StrX(static_cast<DOMElement*>(pcTemp)->getAttributes()->getNamedItem(XStr("Name").unicodeForm())->getNodeValue()).c_str();
        // check on filter condition
        if (sFilter == nullptr || Name.find(sFilter)!= std::string::npos) {
            if (strcmp(StrX(static_cast<DOMElement*>(pcTemp)->getAttribute(XStr("Value").unicodeForm())).c_str(),"1"))
                vrValues.push_back(false);
            else
                vrValues.push_back(true);
        }
        pcTemp = FindNextElement(pcTemp,"FCBool");
    }

    return vrValues;
}

std::vector<std::pair<std::string,bool> > ParameterGrp::GetBoolMap(const char * sFilter) const
{
    std::vector<std::pair<std::string,bool> >  vrValues;
    DOMElement *pcTemp;// = _pGroupNode->getFirstChild();
    std::string Name;

    pcTemp = FindElement(_pGroupNode,"FCBool");
    while ( pcTemp) {
        Name = StrX(static_cast<DOMElement*>(pcTemp)->getAttributes()->getNamedItem(XStr("Name").unicodeForm())->getNodeValue()).c_str();
        // check on filter condition
        if (sFilter == nullptr || Name.find(sFilter)!= std::string::npos) {
            if (strcmp(StrX(static_cast<DOMElement*>(pcTemp)->getAttribute(XStr("Value").unicodeForm())).c_str(),"1"))
                vrValues.emplace_back(Name, false);
            else
                vrValues.emplace_back(Name, true);
        }
        pcTemp = FindNextElement(pcTemp,"FCBool");
    }

    return vrValues;
}

long ParameterGrp::GetInt(const char* Name, long lPreset) const
{
    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCInt",Name);
    // if not return preset
    if (!pcElem) return lPreset;
    // if yes check the value and return
    return atol (StrX(pcElem->getAttribute(XStr("Value").unicodeForm())).c_str());
}

void  ParameterGrp::SetInt(const char* Name, long lValue)
{
    char cBuf[256];
    // find or create the Element
    DOMElement *pcElem = FindOrCreateElement(_pGroupNode,"FCInt",Name);
    if (pcElem) {
        // and set the value
        sprintf(cBuf,"%li",lValue);
        pcElem->setAttribute(XStr("Value").unicodeForm(), XStr(cBuf).unicodeForm());
        // trigger observer
        Notify(Name);
    }
}

std::vector<long> ParameterGrp::GetInts(const char * sFilter) const
{
    std::vector<long>  vrValues;
    DOMNode *pcTemp;// = _pGroupNode->getFirstChild();
    std::string Name;

    pcTemp = FindElement(_pGroupNode,"FCInt") ;
    while ( pcTemp ) {
        Name = StrX(static_cast<DOMElement*>(pcTemp)->getAttributes()->getNamedItem(XStr("Name").unicodeForm())->getNodeValue()).c_str();
        // check on filter condition
        if (sFilter == nullptr || Name.find(sFilter)!= std::string::npos) {
            vrValues.push_back(atol(StrX(static_cast<DOMElement*>(pcTemp)->getAttribute(XStr("Value").unicodeForm())).c_str()) );
        }
        pcTemp = FindNextElement(pcTemp,"FCInt") ;
    }

    return vrValues;
}

std::vector<std::pair<std::string,long> > ParameterGrp::GetIntMap(const char * sFilter) const
{
    std::vector<std::pair<std::string,long> > vrValues;
    DOMNode *pcTemp;// = _pGroupNode->getFirstChild();
    std::string Name;

    pcTemp = FindElement(_pGroupNode,"FCInt") ;
    while ( pcTemp ) {
        Name = StrX(static_cast<DOMElement*>(pcTemp)->getAttributes()->getNamedItem(XStr("Name").unicodeForm())->getNodeValue()).c_str();
        // check on filter condition
        if (sFilter == nullptr || Name.find(sFilter)!= std::string::npos) {
            vrValues.emplace_back(Name,
                               ( atol (StrX(static_cast<DOMElement*>(pcTemp)->getAttribute(XStr("Value").unicodeForm())).c_str())));
        }
        pcTemp = FindNextElement(pcTemp,"FCInt") ;
    }

    return vrValues;
}

unsigned long ParameterGrp::GetUnsigned(const char* Name, unsigned long lPreset) const
{
    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCUInt",Name);
    // if not return preset
    if (!pcElem) return lPreset;
    // if yes check the value and return
    return strtoul (StrX(pcElem->getAttribute(XStr("Value").unicodeForm())).c_str(),nullptr,10);
}

void  ParameterGrp::SetUnsigned(const char* Name, unsigned long lValue)
{
    char cBuf[256];
    // find or create the Element
    DOMElement *pcElem = FindOrCreateElement(_pGroupNode,"FCUInt",Name);
    if (pcElem) {
        // and set the value
        sprintf(cBuf,"%lu",lValue);
        pcElem->setAttribute(XStr("Value").unicodeForm(), XStr(cBuf).unicodeForm());
        // trigger observer
        Notify(Name);
    }
}

std::vector<unsigned long> ParameterGrp::GetUnsigneds(const char * sFilter) const
{
    std::vector<unsigned long>  vrValues;
    DOMNode *pcTemp;// = _pGroupNode->getFirstChild();
    std::string Name;

    pcTemp = FindElement(_pGroupNode,"FCUInt");
    while ( pcTemp ) {
        Name = StrX(static_cast<DOMElement*>(pcTemp)->getAttributes()->getNamedItem(XStr("Name").unicodeForm())->getNodeValue()).c_str();
        // check on filter condition
        if (sFilter == nullptr || Name.find(sFilter)!= std::string::npos) {
            vrValues.push_back( strtoul (StrX(static_cast<DOMElement*>(pcTemp)->getAttribute(XStr("Value").unicodeForm())).c_str(),nullptr,10) );
        }
        pcTemp = FindNextElement(pcTemp,"FCUInt") ;
    }

    return vrValues;
}

std::vector<std::pair<std::string,unsigned long> > ParameterGrp::GetUnsignedMap(const char * sFilter) const
{
    std::vector<std::pair<std::string,unsigned long> > vrValues;
    DOMNode *pcTemp;// = _pGroupNode->getFirstChild();
    std::string Name;

    pcTemp = FindElement(_pGroupNode,"FCUInt");
    while ( pcTemp ) {
        Name = StrX(static_cast<DOMElement*>(pcTemp)->getAttributes()->getNamedItem(XStr("Name").unicodeForm())->getNodeValue()).c_str();
        // check on filter condition
        if (sFilter == NULL || Name.find(sFilter)!= std::string::npos) {
            vrValues.emplace_back(Name,
                               ( strtoul (StrX(static_cast<DOMElement*>(pcTemp)->getAttribute(XStr("Value").unicodeForm())).c_str(),nullptr,10) ));
        }
        pcTemp = FindNextElement(pcTemp,"FCUInt");
    }

    return vrValues;
}

double ParameterGrp::GetFloat(const char* Name, double dPreset) const
{
    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCFloat",Name);
    // if not return preset
    if (!pcElem) return dPreset;
    // if yes check the value and return
    return atof (StrX(pcElem->getAttribute(XStr("Value").unicodeForm())).c_str());
}

void  ParameterGrp::SetFloat(const char* Name, double dValue)
{
    char cBuf[256];
    // find or create the Element
    DOMElement *pcElem = FindOrCreateElement(_pGroupNode,"FCFloat",Name);
    if (pcElem) {
        // and set the value
        sprintf(cBuf,"%.12f",dValue); // use %.12f instead of %f to handle values < 1.0e-6
        pcElem->setAttribute(XStr("Value").unicodeForm(), XStr(cBuf).unicodeForm());
        // trigger observer
        Notify(Name);
    }
}

std::vector<double> ParameterGrp::GetFloats(const char * sFilter) const
{
    std::vector<double>  vrValues;
    DOMElement *pcTemp ;//= _pGroupNode->getFirstChild();
    std::string Name;

    pcTemp = FindElement(_pGroupNode,"FCFloat") ;
    while ( pcTemp ) {
        Name = StrX(static_cast<DOMElement*>(pcTemp)->getAttributes()->getNamedItem(XStr("Name").unicodeForm())->getNodeValue()).c_str();
        // check on filter condition
        if (sFilter == nullptr || Name.find(sFilter)!= std::string::npos) {
            vrValues.push_back( atof (StrX(static_cast<DOMElement*>(pcTemp)->getAttribute(XStr("Value").unicodeForm())).c_str()) );
        }
        pcTemp = FindNextElement(pcTemp,"FCFloat");
    }

    return vrValues;
}

std::vector<std::pair<std::string,double> > ParameterGrp::GetFloatMap(const char * sFilter) const
{
    std::vector<std::pair<std::string,double> > vrValues;
    DOMElement *pcTemp ;//= _pGroupNode->getFirstChild();
    std::string Name;

    pcTemp = FindElement(_pGroupNode,"FCFloat") ;
    while ( pcTemp ) {
        Name = StrX(static_cast<DOMElement*>(pcTemp)->getAttributes()->getNamedItem(XStr("Name").unicodeForm())->getNodeValue()).c_str();
        // check on filter condition
        if (sFilter == nullptr || Name.find(sFilter)!= std::string::npos) {
            vrValues.emplace_back(Name,
                               ( atof (StrX(static_cast<DOMElement*>(pcTemp)->getAttribute(XStr("Value").unicodeForm())).c_str())));
        }
        pcTemp = FindNextElement(pcTemp,"FCFloat");
    }

    return vrValues;
}



void  ParameterGrp::SetBlob(const char* /*Name*/, void* /*pValue*/, long /*lLength*/)
{
    // not implemented so far
    assert(0);
}

void ParameterGrp::GetBlob(const char* /*Name*/, void* /*pBuf*/, long /*lMaxLength*/, void* /*pPreset*/) const
{
    // not implemented so far
    assert(0);
}

void  ParameterGrp::SetASCII(const char* Name, const char *sValue)
{
    // find or create the Element
    DOMElement *pcElem = FindOrCreateElement(_pGroupNode,"FCText",Name);
    if (pcElem) {
        // and set the value
        DOMNode *pcElem2 = pcElem->getFirstChild();
        if (!pcElem2) {
            XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *pDocument = _pGroupNode->getOwnerDocument();
            DOMText *pText = pDocument->createTextNode(XUTF8Str(sValue).unicodeForm());
            pcElem->appendChild(pText);
        }
        else {
            pcElem2->setNodeValue(XUTF8Str(sValue).unicodeForm());
        }
        // trigger observer
        Notify(Name);
    }
}

std::string ParameterGrp::GetASCII(const char* Name, const char * pPreset) const
{
    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCText",Name);
    // if not return preset
    if (!pcElem) {
        if (pPreset==nullptr)
            return std::string("");
        else
            return std::string(pPreset);
    }
    // if yes check the value and return
    DOMNode *pcElem2 = pcElem->getFirstChild();
    if (pcElem2)
        return std::string(StrXUTF8(pcElem2->getNodeValue()).c_str());
    else
        return std::string("");
}

std::vector<std::string> ParameterGrp::GetASCIIs(const char * sFilter) const
{
    std::vector<std::string>  vrValues;
    DOMElement *pcTemp;// = _pGroupNode->getFirstChild();
    std::string Name;

    pcTemp = FindElement(_pGroupNode,"FCText");
    while ( pcTemp  ) {
        Name = StrXUTF8(static_cast<DOMElement*>(pcTemp)->getAttributes()->getNamedItem(XStr("Name").unicodeForm())->getNodeValue()).c_str();
        // check on filter condition
        if (sFilter == nullptr || Name.find(sFilter)!= std::string::npos) {
            // retrieve the text element
            DOMNode *pcElem2 = pcTemp->getFirstChild();
            if (pcElem2)
                vrValues.emplace_back(StrXUTF8(pcElem2->getNodeValue()).c_str() );
            else
                vrValues.emplace_back(""); // For a string, an empty value is possible and allowed
        }
        pcTemp = FindNextElement(pcTemp,"FCText");
    }

    return vrValues;
}

std::vector<std::pair<std::string,std::string> > ParameterGrp::GetASCIIMap(const char * sFilter) const
{
    std::vector<std::pair<std::string,std::string> >  vrValues;
    DOMElement *pcTemp;// = _pGroupNode->getFirstChild();
    std::string Name;

    pcTemp = FindElement(_pGroupNode,"FCText");
    while ( pcTemp) {
        Name = StrXUTF8(static_cast<DOMElement*>(pcTemp)->getAttributes()->getNamedItem(XStr("Name").unicodeForm())->getNodeValue()).c_str();
        // check on filter condition
        if (sFilter == nullptr || Name.find(sFilter)!= std::string::npos) {
            // retrieve the text element
            DOMNode *pcElem2 = pcTemp->getFirstChild();
            if (pcElem2)
                vrValues.emplace_back(Name, std::string(StrXUTF8(pcElem2->getNodeValue()).c_str()));
            else
                vrValues.emplace_back(Name, std::string()); // For a string, an empty value is possible and allowed
        }
        pcTemp = FindNextElement(pcTemp,"FCText");
    }

    return vrValues;
}

//**************************************************************************
// Access methods

void ParameterGrp::RemoveASCII(const char* Name)
{
    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCText",Name);
    // if not return
    if (!pcElem)
        return;

    DOMNode* node = _pGroupNode->removeChild(pcElem);
    node->release();

    // trigger observer
    Notify(Name);
}

void ParameterGrp::RemoveBool(const char* Name)
{
    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCBool",Name);
    // if not return
    if (!pcElem)
        return;

    DOMNode* node = _pGroupNode->removeChild(pcElem);
    node->release();

    // trigger observer
    Notify(Name);
}

void ParameterGrp::RemoveBlob(const char* /*Name*/)
{
    /* not implemented yet
    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCGrp",Name);
    // if not return
    if(!pcElem)
    	return;
    else
    	_pGroupNode->removeChild(pcElem);
    */
}

void ParameterGrp::RemoveFloat(const char* Name)
{
    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCFloat",Name);
    // if not return
    if (!pcElem)
        return;

    DOMNode* node = _pGroupNode->removeChild(pcElem);
    node->release();

    // trigger observer
    Notify(Name);
}

void ParameterGrp::RemoveInt(const char* Name)
{
    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCInt",Name);
    // if not return
    if (!pcElem)
        return;

    DOMNode* node = _pGroupNode->removeChild(pcElem);
    node->release();

    // trigger observer
    Notify(Name);
}

void ParameterGrp::RemoveUnsigned(const char* Name)
{
    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCUInt",Name);
    // if not return
    if (!pcElem)
        return;

    DOMNode* node = _pGroupNode->removeChild(pcElem);
    node->release();

    // trigger observer
    Notify(Name);
}

void ParameterGrp::RemoveGrp(const char* Name)
{
    auto it = _GroupMap.find(Name);
    if (it == _GroupMap.end())
        return;

    // if this or any of its children is referenced by an observer
    // it cannot be deleted
#if 1
    if (!it->second->ShouldRemove()) {
        it->second->Clear();
    }
    else {
#endif
        // check if Element in group
        DOMElement *pcElem = FindElement(_pGroupNode,"FCParamGroup",Name);
        // if not return
        if (!pcElem)
            return;

        // remove group handle
        _GroupMap.erase(Name);

        DOMNode* node = _pGroupNode->removeChild(pcElem);
        node->release();
#if 1
    }
#endif

    // trigger observer
    Notify(Name);
}

bool ParameterGrp::RenameGrp(const char* OldName, const char* NewName)
{
    auto it = _GroupMap.find(OldName);
    if (it == _GroupMap.end())
        return false;
    auto jt = _GroupMap.find(NewName);
    if (jt != _GroupMap.end())
        return false;

    // rename group handle
    _GroupMap[NewName] = _GroupMap[OldName];
    _GroupMap.erase(OldName);
    _GroupMap[NewName]->_cName = NewName;

    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode, "FCParamGroup", OldName);
    if (pcElem)
        pcElem-> setAttribute(XStr("Name").unicodeForm(), XStr(NewName).unicodeForm());

    return true;
}

void ParameterGrp::Clear(void)
{
    std::vector<DOMNode*> vecNodes;

    // checking on references
    std::vector<std::string> removeGrp;
    for (auto it = _GroupMap.begin();it!=_GroupMap.end();++it) {
        // If a group is referenced by some observer then do not remove it
        // but clear it
        if (!it->second->ShouldRemove()) {
            it->second->Clear();
        }
        else {
            removeGrp.push_back(it->first);
        }
    }

    // remove group handles
    for (auto it : removeGrp) {
        auto pos = _GroupMap.find(it);
        vecNodes.push_back(pos->second->_pGroupNode);
        _GroupMap.erase(pos->first);
    }

    // searching all non-group nodes
    for (DOMNode *child = _pGroupNode->getFirstChild(); child != 0;  child = child->getNextSibling()) {
        if (XMLString::compareString(child->getNodeName(), XStr("FCParamGroup").unicodeForm()) != 0)
            vecNodes.push_back(child);
    }

    // deleting the nodes
    for (auto it = vecNodes.begin(); it != vecNodes.end(); ++it) {
        DOMNode *child = _pGroupNode->removeChild(*it);
        child->release();
    }

    // trigger observer
    Notify("");
}

//**************************************************************************
// Access methods

bool ParameterGrp::ShouldRemove() const
{
    if (this->getRefCount() > 1)
        return false;
    for (auto it : _GroupMap) {
        bool ok = it.second->ShouldRemove();
        if (!ok)
            return false;
    }
    return true;
}

XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ParameterGrp::FindElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *Start, const char* Type, const char* Name) const
{
    if (XMLString::compareString(Start->getNodeName(), XStr("FCParamGroup").unicodeForm()) != 0 &&
        XMLString::compareString(Start->getNodeName(), XStr("FCParameters").unicodeForm()) != 0) {
        Base::Console().Warning("FindElement: %s cannot have the element %s of type %s\n", StrX(Start->getNodeName()).c_str(), Name, Type);
        return nullptr;
    }
    for (DOMNode *clChild = Start->getFirstChild(); clChild != nullptr;  clChild = clChild->getNextSibling()) {
        if (clChild->getNodeType() == DOMNode::ELEMENT_NODE) {
            // the right node Type
            if (!strcmp(Type,StrX(clChild->getNodeName()).c_str())) {
                if (clChild->getAttributes()->getLength() > 0) {
                    if (Name) {
                        if (!strcmp(Name,StrX(clChild->getAttributes()->getNamedItem(XStr("Name").unicodeForm())->getNodeValue()).c_str()))
                            return static_cast<DOMElement*>(clChild);
                    }
                    else
                        return static_cast<DOMElement*>(clChild);

                }
            }
        }
    }
    return nullptr;
}

XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ParameterGrp::FindNextElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode *Prev, const char* Type) const
{
    DOMNode *clChild = Prev;
    if (!clChild)
        return nullptr;

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

XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ParameterGrp::FindOrCreateElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *Start, const char* Type, const char* Name) const
{
    // first try to find it
    DOMElement *pcElem = FindElement(Start,Type,Name);
    if (pcElem)
        return pcElem;

    if (XMLString::compareString(Start->getNodeName(), XStr("FCParamGroup").unicodeForm()) != 0 &&
        XMLString::compareString(Start->getNodeName(), XStr("FCParameters").unicodeForm()) != 0) {
        Base::Console().Warning("FindOrCreateElement: %s cannot have the element %s of type %s\n", StrX(Start->getNodeName()).c_str(), Name, Type);
        return nullptr;
    }

    XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *pDocument = _pGroupNode->getOwnerDocument();

    pcElem = pDocument->createElement(XStr(Type).unicodeForm());
    pcElem-> setAttribute(XStr("Name").unicodeForm(), XStr(Name).unicodeForm());
    Start->appendChild(pcElem);

    return pcElem;
}

void ParameterGrp::NotifyAll()
{
    // get all ints and notify
    std::vector<std::pair<std::string,long> > IntMap = GetIntMap();
    for (std::vector<std::pair<std::string,long> >::iterator It1= IntMap.begin(); It1 != IntMap.end(); ++It1)
        Notify(It1->first.c_str());

    // get all booleans and notify
    std::vector<std::pair<std::string,bool> > BoolMap = GetBoolMap();
    for (std::vector<std::pair<std::string,bool> >::iterator It2= BoolMap.begin(); It2 != BoolMap.end(); ++It2)
        Notify(It2->first.c_str());

    // get all Floats and notify
    std::vector<std::pair<std::string,double> > FloatMap  = GetFloatMap();
    for (std::vector<std::pair<std::string,double> >::iterator It3= FloatMap.begin(); It3 != FloatMap.end(); ++It3)
        Notify(It3->first.c_str());

    // get all strings and notify
    std::vector<std::pair<std::string,std::string> > StringMap = GetASCIIMap();
    for (std::vector<std::pair<std::string,std::string> >::iterator It4= StringMap.begin(); It4 != StringMap.end(); ++It4)
        Notify(It4->first.c_str());

    // get all uints and notify
    std::vector<std::pair<std::string,unsigned long> > UIntMap = GetUnsignedMap();
    for (std::vector<std::pair<std::string,unsigned long> >::iterator It5= UIntMap.begin(); It5 != UIntMap.end(); ++It5)
        Notify(It5->first.c_str());
}

//**************************************************************************
//**************************************************************************
// ParameterSerializer
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
ParameterSerializer::ParameterSerializer(const std::string& fn)
  : filename(fn)
{
}

ParameterSerializer::~ParameterSerializer()
{
}

void ParameterSerializer::SaveDocument(const ParameterManager& mgr)
{
    mgr.SaveDocument(filename.c_str());
}

int ParameterSerializer::LoadDocument(ParameterManager& mgr)
{
    return mgr.LoadDocument(filename.c_str());
}

bool ParameterSerializer::LoadOrCreateDocument(ParameterManager& mgr)
{
    return mgr.LoadOrCreateDocument(filename.c_str());
}

//**************************************************************************
//**************************************************************************
// ParameterManager
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

static XercesDOMParser::ValSchemes    gValScheme       = XercesDOMParser::Val_Auto;

//**************************************************************************
// Construction/Destruction

/** Default construction
  */
ParameterManager::ParameterManager()
  : ParameterGrp(), _pDocument(nullptr), paramSerializer(nullptr)
{
    // initialize the XML system
    Init();

// ---------------------------------------------------------------------------
//  Local data
//
//  gXmlFile
//      The path to the file to parser. Set via command line.
//
//  gDoNamespaces
//      Indicates whether namespace processing should be done.
//
//  gDoSchema
//      Indicates whether schema processing should be done.
//
//  gSchemaFullChecking
//      Indicates whether full schema constraint checking should be done.
//
//  gDoCreate
//      Indicates whether entity reference nodes needs to be created or not.
//      Defaults to false.
//
//  gOutputEncoding
//      The encoding we are to output in. If not set on the command line,
//      then it is defaults to the encoding of the input XML file.
//
//  gMyEOLSequence
//      The end of line sequence we are to output.
//
//  gSplitCdataSections
//      Indicates whether split-cdata-sections is to be enabled or not.
//
//  gDiscardDefaultContent
//      Indicates whether default content is discarded or not.
//
//  gUseFilter
//      Indicates if user wants to plug in the DOMPrintFilter.
//
//  gValScheme
//      Indicates what validation scheme to use. It defaults to 'auto', but
//      can be set via the -v= command.
//
// ---------------------------------------------------------------------------

    gDoNamespaces          = false;
    gDoSchema              = false;
    gSchemaFullChecking    = false;
    gDoCreate              = true;

    gOutputEncoding        = nullptr;
    gMyEOLSequence         = nullptr;

    gSplitCdataSections    = true;
    gDiscardDefaultContent = true;
    gUseFilter             = true;
    gFormatPrettyPrint     = true;
}

/** Destruction
  * complete destruction of the object
  */
ParameterManager::~ParameterManager()
{
    delete _pDocument;
    delete paramSerializer;
}

void ParameterManager::Init()
{
    static bool Init = false;
    if (!Init) {
        try {
            XMLPlatformUtils::Initialize();
        }
        catch (const XMLException& toCatch) {
#if defined(FC_OS_LINUX) || defined(FC_OS_CYGWIN)
            std::ostringstream err;
#else
            std::stringstream err;
#endif
            char *pMsg = XMLString::transcode(toCatch.getMessage());
            err << "Error during Xerces-c Initialization.\n"
                << "  Exception message:"
                << pMsg;
            XMLString::release(&pMsg);
            throw XMLBaseException(err.str().c_str());
        }
        Init = true;
    }
}

void ParameterManager::Terminate()
{
    XMLTools::terminate();
    XMLPlatformUtils::Terminate();
}

//**************************************************************************
// Serializer handling

void ParameterManager::SetSerializer(ParameterSerializer* ps)
{
    if (paramSerializer != ps)
        delete paramSerializer;
    paramSerializer = ps;
}

bool ParameterManager::HasSerializer() const
{
    return (paramSerializer != nullptr);
}

int ParameterManager::LoadDocument()
{
    if (paramSerializer)
        return paramSerializer->LoadDocument(*this);
    else
        return -1;
}

bool ParameterManager::LoadOrCreateDocument()
{
    if (paramSerializer)
        return paramSerializer->LoadOrCreateDocument(*this);
    else
        return false;
}

void ParameterManager::SaveDocument() const
{
    if (paramSerializer)
        paramSerializer->SaveDocument(*this);
}

//**************************************************************************
// Document handling

bool ParameterManager::LoadOrCreateDocument(const char* sFileName)
{
    Base::FileInfo file(sFileName);
    if (file.exists()) {
        LoadDocument(sFileName);
        return false;
    }
    else {
        CreateDocument();
        return true;
    }
}

int  ParameterManager::LoadDocument(const char* sFileName)
{
    Base::FileInfo file(sFileName);

    try {
#if defined (FC_OS_WIN32)
        LocalFileInputSource inputSource(reinterpret_cast<const XMLCh*>(file.toStdWString().c_str()));
#else
        LocalFileInputSource inputSource(XStr(file.filePath().c_str()).unicodeForm());
#endif
        return LoadDocument(inputSource);
    }
    catch (const Base::Exception& e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
    catch (...) {
        std::cerr << "An error occurred during parsing\n " << std::endl;
        throw;
    }
}

int ParameterManager::LoadDocument(const XERCES_CPP_NAMESPACE_QUALIFIER InputSource& inputSource)
{
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

    _pDocument = parser->adoptDocument();
    delete parser;
    delete errReporter;

    if (!_pDocument)
        throw XMLBaseException("Malformed Parameter document: Invalid document");

    DOMElement* rootElem = _pDocument->getDocumentElement();
    if (!rootElem)
        throw XMLBaseException("Malformed Parameter document: Root group not found");

    _pGroupNode = FindElement(rootElem,"FCParamGroup","Root");

    if (!_pGroupNode)
        throw XMLBaseException("Malformed Parameter document: Root group not found");

    return 1;
}

void  ParameterManager::SaveDocument(const char* sFileName) const
{
    Base::FileInfo file(sFileName);

    try {
        //
        // Plug in a format target to receive the resultant
        // XML stream from the serializer.
        //
        // LocalFileFormatTarget prints the resultant XML stream
        // to a file once it receives any thing from the serializer.
        //
#if defined (FC_OS_WIN32)
        XMLFormatTarget *myFormTarget = new LocalFileFormatTarget (reinterpret_cast<const XMLCh*>(file.toStdWString().c_str()));
#else
        XMLFormatTarget *myFormTarget = new LocalFileFormatTarget (file.filePath().c_str());
#endif
        SaveDocument(myFormTarget);
        delete myFormTarget;
    }
    catch (XMLException& e) {
        std::cerr << "An error occurred during creation of output transcoder. Msg is:"
        << std::endl
        << StrX(e.getMessage()) << std::endl;
    }
}

void  ParameterManager::SaveDocument(XMLFormatTarget* pFormatTarget) const
{
#if (XERCES_VERSION_MAJOR == 2)
    DOMPrintFilter   *myFilter = 0;

    try {
        // get a serializer, an instance of DOMWriter
        XMLCh tempStr[100];
        XMLString::transcode("LS", tempStr, 99);
        DOMImplementation *impl          = DOMImplementationRegistry::getDOMImplementation(tempStr);
        DOMWriter         *theSerializer = ((DOMImplementationLS*)impl)->createDOMWriter();

        // set user specified end of line sequence and output encoding
        theSerializer->setNewLine(gMyEOLSequence);
        theSerializer->setEncoding(gOutputEncoding);

        // plug in user's own filter
        if (gUseFilter) {
            // even we say to show attribute, but the DOMWriter
            // will not show attribute nodes to the filter as
            // the specs explicitly says that DOMWriter shall
            // NOT show attributes to DOMWriterFilter.
            //
            // so DOMNodeFilter::SHOW_ATTRIBUTE has no effect.
            // same DOMNodeFilter::SHOW_DOCUMENT_TYPE, no effect.
            //
            myFilter = new DOMPrintFilter(DOMNodeFilter::SHOW_ELEMENT   |
                                          DOMNodeFilter::SHOW_ATTRIBUTE |
                                          DOMNodeFilter::SHOW_DOCUMENT_TYPE
                                         );
            theSerializer->setFilter(myFilter);
        }

        // plug in user's own error handler
        DOMErrorHandler *myErrorHandler = new DOMPrintErrorHandler();
        theSerializer->setErrorHandler(myErrorHandler);

        // set feature if the serializer supports the feature/mode
        if (theSerializer->canSetFeature(XMLUni::fgDOMWRTSplitCdataSections, gSplitCdataSections))
            theSerializer->setFeature(XMLUni::fgDOMWRTSplitCdataSections, gSplitCdataSections);

        if (theSerializer->canSetFeature(XMLUni::fgDOMWRTDiscardDefaultContent, gDiscardDefaultContent))
            theSerializer->setFeature(XMLUni::fgDOMWRTDiscardDefaultContent, gDiscardDefaultContent);

        if (theSerializer->canSetFeature(XMLUni::fgDOMWRTFormatPrettyPrint, gFormatPrettyPrint))
            theSerializer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, gFormatPrettyPrint);

        //
        // do the serialization through DOMWriter::writeNode();
        //
        theSerializer->writeNode(pFormatTarget, *_pDocument);

        delete theSerializer;

        //
        // Filter and error handler
        // are NOT owned by the serializer.
        //
        delete myErrorHandler;

        if (gUseFilter)
            delete myFilter;

    }
    catch (XMLException& e) {
        std::cerr << "An error occurred during creation of output transcoder. Msg is:"
        << std::endl
        << StrX(e.getMessage()) << std::endl;
    }
#else
    try {
        std::unique_ptr<DOMPrintFilter>   myFilter;
        std::unique_ptr<DOMErrorHandler>  myErrorHandler;

        // get a serializer, an instance of DOMWriter
        XMLCh tempStr[100];
        XMLString::transcode("LS", tempStr, 99);
        DOMImplementation *impl          = DOMImplementationRegistry::getDOMImplementation(tempStr);
        DOMLSSerializer   *theSerializer = static_cast<DOMImplementationLS*>(impl)->createLSSerializer();

        // set user specified end of line sequence and output encoding
        theSerializer->setNewLine(gMyEOLSequence);


        //
        // do the serialization through DOMWriter::writeNode();
        //
        if (_pDocument) {
            DOMLSOutput *theOutput = static_cast<DOMImplementationLS*>(impl)->createLSOutput();
            theOutput->setEncoding(gOutputEncoding);

            if (gUseFilter) {
                myFilter.reset(new DOMPrintFilter(DOMNodeFilter::SHOW_ELEMENT   |
                                                  DOMNodeFilter::SHOW_ATTRIBUTE |
                                                  DOMNodeFilter::SHOW_DOCUMENT_TYPE
                                                  ));
                theSerializer->setFilter(myFilter.get());
            }

            // plug in user's own error handler
            myErrorHandler.reset(new DOMPrintErrorHandler());
            DOMConfiguration* config = theSerializer->getDomConfig();
            config->setParameter(XMLUni::fgDOMErrorHandler, myErrorHandler.get());

            // set feature if the serializer supports the feature/mode
            if (config->canSetParameter(XMLUni::fgDOMWRTSplitCdataSections, gSplitCdataSections))
                config->setParameter(XMLUni::fgDOMWRTSplitCdataSections, gSplitCdataSections);

            if (config->canSetParameter(XMLUni::fgDOMWRTDiscardDefaultContent, gDiscardDefaultContent))
                config->setParameter(XMLUni::fgDOMWRTDiscardDefaultContent, gDiscardDefaultContent);

            if (config->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, gFormatPrettyPrint))
                config->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, gFormatPrettyPrint);

            theOutput->setByteStream(pFormatTarget);
            theSerializer->write(_pDocument, theOutput);

            theOutput->release();
        }

        theSerializer->release();
    }
    catch (XMLException& e) {
        std::cerr << "An error occurred during creation of output transcoder. Msg is:"
        << std::endl
        << StrX(e.getMessage()) << std::endl;
    }
#endif
}

void  ParameterManager::CreateDocument()
{
    // creating a document from screatch
    DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(XStr("Core").unicodeForm());
    delete _pDocument;
    _pDocument = impl->createDocument(
                     nullptr,                                    // root element namespace URI.
                     XStr("FCParameters").unicodeForm(),         // root element name
                     nullptr);                                   // document type object (DTD).

    // creating the node for the root group
    DOMElement* rootElem = _pDocument->getDocumentElement();
    _pGroupNode = _pDocument->createElement(XStr("FCParamGroup").unicodeForm());
    static_cast<DOMElement*>(_pGroupNode)->setAttribute(XStr("Name").unicodeForm(), XStr("Root").unicodeForm());
    rootElem->appendChild(_pGroupNode);
}

void  ParameterManager::CheckDocument() const
{
    if (!_pDocument)
        return;

    try {
        //
        // Plug in a format target to receive the resultant
        // XML stream from the serializer.
        //
        // LocalFileFormatTarget prints the resultant XML stream
        // to a file once it receives any thing from the serializer.
        //
        MemBufFormatTarget myFormTarget;
        SaveDocument(&myFormTarget);

        // Either use the file saved on disk or write the current XML into a buffer in memory
        // const char* xmlFile = "...";
        MemBufInputSource xmlFile(myFormTarget.getRawBuffer(), myFormTarget.getLen(), "(memory)");

        // Either load the XSD file from disk or use the built-in string
        // const char* xsdFile = "...";
        std::string xsdStr(xmlSchemeString);
        MemBufInputSource xsdFile(reinterpret_cast<const XMLByte*>(xsdStr.c_str()), xsdStr.size(), "Parameter.xsd");

        // See http://apache-xml-project.6118.n7.nabble.com/validating-xml-with-xsd-schema-td17515.html
        //
        XercesDOMParser parser;
        Grammar* grammar = parser.loadGrammar(xsdFile, Grammar::SchemaGrammarType, true);
        if (!grammar) {
            Base::Console().Error("Grammar file cannot be loaded.\n");
            return;
        }

        parser.setExternalNoNamespaceSchemaLocation("Parameter.xsd");
        //parser.setExitOnFirstFatalError(true);
        //parser.setValidationConstraintFatal(true);
        parser.cacheGrammarFromParse(true);
        parser.setValidationScheme(XercesDOMParser::Val_Auto);
        parser.setDoNamespaces(true);
        parser.setDoSchema(true);

        DOMTreeErrorReporter errHandler;
        parser.setErrorHandler(&errHandler);
        parser.parse(xmlFile);

        if (parser.getErrorCount() > 0) {
            Base::Console().Error("Unexpected XML structure detected: %zu errors\n", parser.getErrorCount());
        }
    }
    catch (XMLException& e) {
        std::cerr << "An error occurred while checking document. Msg is:"
        << std::endl
        << StrX(e.getMessage()) << std::endl;
    }
}


//**************************************************************************
//**************************************************************************
// DOMTreeErrorReporter
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void DOMTreeErrorReporter::warning(const SAXParseException&)
{
    //
    // Ignore all warnings.
    //
}

void DOMTreeErrorReporter::error(const SAXParseException& toCatch)
{
    fSawErrors = true;
    std::cerr << "Error at file \"" << StrX(toCatch.getSystemId())
    << "\", line " << toCatch.getLineNumber()
    << ", column " << toCatch.getColumnNumber()
    << "\n   Message: " << StrX(toCatch.getMessage()) << std::endl;
}

void DOMTreeErrorReporter::fatalError(const SAXParseException& toCatch)
{
    fSawErrors = true;
    std::cerr << "Fatal Error at file \"" << StrX(toCatch.getSystemId())
    << "\", line " << toCatch.getLineNumber()
    << ", column " << toCatch.getColumnNumber()
    << "\n   Message: " << StrX(toCatch.getMessage()) << std::endl;
}

void DOMTreeErrorReporter::resetErrors()
{
    // No-op in this case
}


//**************************************************************************
//**************************************************************************
// DOMPrintFilter
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#if (XERCES_VERSION_MAJOR == 2)
DOMPrintFilter::DOMPrintFilter(unsigned long whatToShow)
        :fWhatToShow(whatToShow)
{

}

short DOMPrintFilter::acceptNode(const DOMNode* node) const
{
    //
    // The DOMWriter shall call getWhatToShow() before calling
    // acceptNode(), to show nodes which are supposed to be
    // shown to this filter.
    // TODO:
    // REVISIT: In case the DOMWriter does not follow the protocol,
    //          Shall the filter honor, or NOT, what it claims
    //         (when it is constructed/setWhatToShow())
    //          it is interested in ?
    //
    // The DOMLS specs does not specify that acceptNode() shall do
    // this way, or not, so it is up the implementation,
    // to skip the code below for the sake of performance ...
    //
    if ((getWhatToShow() & (1 << (node->getNodeType() - 1))) == 0)
        return DOMNodeFilter::FILTER_ACCEPT;

    switch (node->getNodeType()) {
    case DOMNode::ELEMENT_NODE: {
        // for element whose name is "person", skip it
        //if (XMLString::compareString(node->getNodeName(), element_person)==0)
        //	return DOMNodeFilter::FILTER_SKIP;
        // for element whose name is "line", reject it
        //if (XMLString::compareString(node->getNodeName(), element_link)==0)
        //	return DOMNodeFilter::FILTER_REJECT;
        // for rest, accept it
        return DOMNodeFilter::FILTER_ACCEPT;

        break;
    }
    case DOMNode::COMMENT_NODE: {
        // the WhatToShow will make this no effect
        //return DOMNodeFilter::FILTER_REJECT;
        return DOMNodeFilter::FILTER_ACCEPT;
        break;
    }
    case DOMNode::TEXT_NODE: {
        // the WhatToShow will make this no effect
        //return DOMNodeFilter::FILTER_REJECT;
        return DOMNodeFilter::FILTER_ACCEPT;
        break;
    }
    case DOMNode::DOCUMENT_TYPE_NODE: {
        // even we say we are going to process document type,
        // we are not able be to see this node since
        // DOMWriterImpl (a XercesC's default implementation
        // of DOMWriter) will not pass DocumentType node to
        // this filter.
        //
        return DOMNodeFilter::FILTER_REJECT;  // no effect
        break;
    }
    case DOMNode::DOCUMENT_NODE: {
        // same as DOCUMENT_NODE
        return DOMNodeFilter::FILTER_REJECT;  // no effect
        break;
    }
    default : {
        return DOMNodeFilter::FILTER_ACCEPT;
        break;
    }
    }

    return DOMNodeFilter::FILTER_ACCEPT;
}
#else
DOMPrintFilter::DOMPrintFilter(ShowType whatToShow)
    : fWhatToShow(whatToShow)
{
}

DOMPrintFilter::FilterAction DOMPrintFilter::acceptNode(const DOMNode* node) const
{
    if (XMLString::compareString(node->getNodeName(), XStr("FCParameters").unicodeForm()) == 0) {
        // This node is supposed to have a single FCParamGroup and two text nodes.
        // Over time it can happen that the text nodes collect extra newlines.
        const DOMNodeList*  children =  node->getChildNodes();
        for (XMLSize_t i=0; i<children->getLength(); i++) {
            DOMNode* child = children->item(i);
            if (child->getNodeType() == DOMNode::TEXT_NODE) {
                child->setNodeValue(XStr("\n").unicodeForm());
            }
        }
    }

    switch (node->getNodeType()) {
    case DOMNode::ELEMENT_NODE: {
        return DOMNodeFilter::FILTER_ACCEPT;

        break;
    }
    case DOMNode::COMMENT_NODE: {
        return DOMNodeFilter::FILTER_ACCEPT;
        break;
    }
    case DOMNode::TEXT_NODE: {
        return DOMNodeFilter::FILTER_ACCEPT;
        break;
    }
    case DOMNode::DOCUMENT_TYPE_NODE: {
        return DOMNodeFilter::FILTER_REJECT;  // no effect
        break;
    }
    case DOMNode::DOCUMENT_NODE: {
        return DOMNodeFilter::FILTER_REJECT;  // no effect
        break;
    }
    default : {
        return DOMNodeFilter::FILTER_ACCEPT;
        break;
    }
    }

    return DOMNodeFilter::FILTER_ACCEPT;
}
#endif

//**************************************************************************
//**************************************************************************
// DOMPrintErrorHandler
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


bool DOMPrintErrorHandler::handleError(const DOMError &domError)
{
    // Display whatever error message passed from the serializer
    char *msg = XMLString::transcode(domError.getMessage());
    std::cout<<msg<<std::endl;
    XMLString::release(&msg);

    // Instructs the serializer to continue serialization if possible.
    return true;
}
