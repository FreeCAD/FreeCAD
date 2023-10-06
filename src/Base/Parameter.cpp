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
#   include <xercesc/dom/DOM.hpp>
#   include <xercesc/framework/LocalFileFormatTarget.hpp>
#   include <xercesc/framework/LocalFileInputSource.hpp>
#   include <xercesc/framework/MemBufFormatTarget.hpp>
#   include <xercesc/framework/MemBufInputSource.hpp>
#   include <xercesc/parsers/XercesDOMParser.hpp>
#   include <xercesc/sax/SAXParseException.hpp>
#   include <sstream>
#   include <string>
#   include <utility>
#endif

#ifdef FC_OS_LINUX
#   include <unistd.h>
#endif

#include <boost/algorithm/string.hpp>

#include "Parameter.h"
#include "Parameter.inl"
#include "Console.h"
#include "Exception.h"
#include "Tools.h"

FC_LOG_LEVEL_INIT("Parameter", true, true)


XERCES_CPP_NAMESPACE_USE
using namespace Base;


#include "XMLTools.h"

//**************************************************************************
//**************************************************************************
// private classes declaration:
// - StrX
// - DOMPrintFilter
// - DOMPrintErrorHandler
// - XStr
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
DOMTreeErrorReporter::DOMTreeErrorReporter():
	fSawErrors(false) {
}

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

inline bool DOMTreeErrorReporter::getSawErrors() const
{
    return fSawErrors;
}

class DOMPrintFilter : public DOMLSSerializerFilter
{
public:

    /** @name Constructors */
    explicit DOMPrintFilter(ShowType whatToShow = DOMNodeFilter::SHOW_ALL);
    //@{

    /** @name Destructors */
    ~DOMPrintFilter() override = default;
    //@{

    /** @ interface from DOMWriterFilter */
    FilterAction acceptNode(const XERCES_CPP_NAMESPACE_QUALIFIER DOMNode*) const override;
    //@{

    ShowType getWhatToShow() const override {
        return fWhatToShow;
    }

    // unimplemented copy ctor and assignment operator
    DOMPrintFilter(const DOMPrintFilter&) = delete;
    DOMPrintFilter & operator = (const DOMPrintFilter&) = delete;

   ShowType fWhatToShow;
};
class DOMPrintErrorHandler : public DOMErrorHandler
{
public:

    DOMPrintErrorHandler() = default;
    ~DOMPrintErrorHandler() override = default;

    /** @name The error handler interface */
    bool handleError(const DOMError& domError) override;
    void resetErrors() {}

    /* Unimplemented constructors and operators */
    explicit DOMPrintErrorHandler(const DOMErrorHandler&) = delete;
    void operator=(const DOMErrorHandler&) = delete;

};
//**************************************************************************
//**************************************************************************
// ParameterManager
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//**************************************************************************
// Construction/Destruction


/** Default construction
  */
ParameterGrp::ParameterGrp(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *GroupNode,
                           const char* sName,
                           ParameterGrp *Parent)
        : Base::Handled(), Subject<const char*>()
        , _pGroupNode(GroupNode)
        , _Parent(Parent)
{
    if (sName) _cName=sName;
    if (_Parent) _Manager = _Parent->_Manager;
}


/** Destruction
  * complete destruction of the object
  */
ParameterGrp::~ParameterGrp()
{
    for (auto &v : _GroupMap) {
        v.second->_Parent = nullptr;
        v.second->_Manager = nullptr;
    }
    if (_Detached && _pGroupNode)
        _pGroupNode->release();
}

//**************************************************************************
// Access methods

void ParameterGrp::copyTo(Base::Reference<ParameterGrp> Grp)
{
    if (Grp == this)
        return;

    // delete previous content
    Grp->Clear(true);

    // copy all
    insertTo(Grp);
}

void ParameterGrp::insertTo(Base::Reference<ParameterGrp> Grp)
{
    if (Grp == this)
        return;

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
    auto Mngr = ParameterManager::Create();

    Mngr->CreateDocument();

    // copy all into the new document
    insertTo(Base::Reference<ParameterGrp>(Mngr));

    Mngr->SaveDocument(FileName);
}

void ParameterGrp::importFrom(const char* FileName)
{
    auto Mngr = ParameterManager::Create();

    if (Mngr->LoadDocument(FileName) != 1)
        throw FileException("ParameterGrp::import() cannot load document", FileName);

    Mngr->copyTo(Base::Reference<ParameterGrp>(this));
}

void ParameterGrp::insert(const char* FileName)
{
    auto Mngr = ParameterManager::Create();

    if (Mngr->LoadDocument(FileName) != 1)
        throw FileException("ParameterGrp::import() cannot load document", FileName);

    Mngr->insertTo(Base::Reference<ParameterGrp>(this));
}

void ParameterGrp::revert(const char* FileName)
{
    auto Mngr = ParameterManager::Create();

    if (Mngr->LoadDocument(FileName) != 1)
        throw FileException("ParameterGrp::revert() cannot load document", FileName);

    revert(Base::Reference<ParameterGrp>(Mngr));
}

void ParameterGrp::revert(Base::Reference<ParameterGrp> Grp)
{
    if (Grp == this)
        return;

    for (auto &grp : Grp->GetGroups()) {
        if (HasGroup(grp->GetGroupName()))
            GetGroup(grp->GetGroupName())->revert(grp);
    }

    for (const auto &v : Grp->GetASCIIMap()) {
        if (GetASCII(v.first.c_str(), v.second.c_str()) == v.second)
            RemoveASCII(v.first.c_str());
    }

    for (const auto &v : Grp->GetBoolMap()) {
        if (GetBool(v.first.c_str(), v.second) == v.second)
            RemoveBool(v.first.c_str());
    }

    for (const auto &v : Grp->GetIntMap()) {
        if (GetInt(v.first.c_str(), v.second) == v.second)
            RemoveInt(v.first.c_str());
    }

    for (const auto &v : Grp->GetUnsignedMap()) {
        if (GetUnsigned(v.first.c_str(), v.second) == v.second)
            RemoveUnsigned(v.first.c_str());
    }

    for (const auto &v : Grp->GetFloatMap()) {
        if (GetFloat(v.first.c_str(), v.second) == v.second)
            RemoveFloat(v.first.c_str());
    }
}

XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *
ParameterGrp::CreateElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *Start, const char* Type, const char* Name)
{
    if (XMLString::compareString(Start->getNodeName(), XStr("FCParamGroup").unicodeForm()) != 0 &&
        XMLString::compareString(Start->getNodeName(), XStr("FCParameters").unicodeForm()) != 0) {
        Base::Console().Warning("CreateElement: %s cannot have the element %s of type %s\n", StrX(Start->getNodeName()).c_str(), Name, Type);
        return nullptr;
    }

    if (_Detached && _Parent) {
        // re-attach the group
        _Parent->_GetGroup(_cName.c_str());
    }

    XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *pDocument = Start->getOwnerDocument();

    auto pcElem = pDocument->createElement(XStr(Type).unicodeForm());
    pcElem-> setAttribute(XStr("Name").unicodeForm(), XStr(Name).unicodeForm());
    Start->appendChild(pcElem);

    return pcElem;
}

Base::Reference<ParameterGrp> ParameterGrp::GetGroup(const char* Name)
{
    if (!Name)
        throw Base::ValueError("Empty group name");

    Base::Reference<ParameterGrp> hGrp = this;
    std::vector<std::string> tokens;
    boost::split(tokens, Name, boost::is_any_of("/"));
    for (auto &token : tokens) {
        boost::trim(token);
        if (token.empty())
            continue;
        hGrp = hGrp->_GetGroup(token.c_str());
        if (!hGrp) {
            // The group is clearing. Return some dummy group to avoid caller
            // crashing for backward compatibility.
            hGrp = new ParameterGrp();
            hGrp->_cName = Name;
            break;
        }
    }
    if (hGrp == this)
        throw Base::ValueError("Empty group name");
    return hGrp;
}

Base::Reference<ParameterGrp> ParameterGrp::_GetGroup(const char* Name)
{
    Base::Reference<ParameterGrp> rParamGrp;
    if (!_pGroupNode) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
            FC_WARN("Adding group " << Name << " in an orphan group " << _cName);
        return rParamGrp;
    }
    if (_Clearing) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
            FC_WARN("Adding group " << Name << " while clearing " << GetPath());
        return rParamGrp;
    }

    DOMElement *pcTemp{};

    // search if Group node already there
    pcTemp = FindElement(_pGroupNode,"FCParamGroup",Name);

    // already created?
    if (!(rParamGrp=_GroupMap[Name]).isValid()) {
        if (!pcTemp)
            pcTemp = CreateElement(_pGroupNode,"FCParamGroup",Name);
        // create and register handle
        rParamGrp = Base::Reference<ParameterGrp> (new ParameterGrp(pcTemp,Name,this));
        _GroupMap[Name] = rParamGrp;
    } else if (!pcTemp) {
        _pGroupNode->appendChild(rParamGrp->_pGroupNode);
        rParamGrp->_Detached = false;
        if (this->_Detached && this->_Parent) {
            // Re-attach the group. Note that this may fail if the parent is
            // clearing. That's why we check this->_Detached below.
            this->_Parent->_GetGroup(_cName.c_str());
        }
    }

    if (!pcTemp && !this->_Detached)
        _Notify(ParamType::FCGroup, Name, Name);

    return rParamGrp;
}

std::string ParameterGrp::GetPath() const
{
    std::string path;
    if (_Parent && _Parent != _Manager)
        path = _Parent->GetPath();
    if (path.size() && _cName.size())
        path += "/";
    path += _cName;
    return path;
}

std::vector<Base::Reference<ParameterGrp> > ParameterGrp::GetGroups()
{
    Base::Reference<ParameterGrp> rParamGrp;
    std::vector<Base::Reference<ParameterGrp> >  vrParamGrp;

    if (!_pGroupNode)
        return vrParamGrp;

    std::string Name;

    DOMElement *pcTemp = FindElement(_pGroupNode,"FCParamGroup");
    while (pcTemp) {
        Name = StrX(pcTemp->getAttributes()->getNamedItem(XStr("Name").unicodeForm())->getNodeValue()).c_str();
        // already created?
        if (!(rParamGrp=_GroupMap[Name]).isValid()) {
            rParamGrp = Base::Reference<ParameterGrp> (new ParameterGrp(pcTemp,Name.c_str(),this));
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
    if ( _pGroupNode && _pGroupNode->getFirstChild() )
        return false;
    else
        return true;
}

/// test if a special sub group is in this group
bool ParameterGrp::HasGroup(const char* Name) const
{
    if (_GroupMap.find(Name) != _GroupMap.end())
        return true;

    if (_pGroupNode && FindElement(_pGroupNode,"FCParamGroup",Name) != nullptr)
        return true;

    return false;
}

const char *ParameterGrp::TypeName(ParamType Type)
{
    switch(Type) {
    case ParamType::FCBool:
        return "FCBool";
    case ParamType::FCInt:
        return "FCInt";
    case ParamType::FCUInt:
        return "FCUInt";
    case ParamType::FCText:
        return "FCText";
    case ParamType::FCFloat:
        return "FCFloat";
    case ParamType::FCGroup:
        return "FCParamGroup";
    default:
        return nullptr;
    }
}

ParameterGrp::ParamType ParameterGrp::TypeValue(const char *Name)
{
    if (Name) {
        if (boost::equals(Name, "FCBool"))
            return ParamType::FCBool;
        if (boost::equals(Name, "FCInt"))
            return ParamType::FCInt;
        if (boost::equals(Name, "FCUInt"))
            return ParamType::FCUInt;
        if (boost::equals(Name, "FCText"))
            return ParamType::FCText;
        if (boost::equals(Name, "FCFloat"))
            return ParamType::FCFloat;
        if (boost::equals(Name, "FCParamGroup"))
            return ParamType::FCGroup;
    }
    return ParamType::FCInvalid;
}

void ParameterGrp::SetAttribute(ParamType Type, const char *Name, const char *Value)
{
    switch(Type) {
    case ParamType::FCBool:
    case ParamType::FCInt:
    case ParamType::FCUInt:
    case ParamType::FCFloat:
        return _SetAttribute(Type, Name, Value);
    case ParamType::FCText:
        return SetASCII(Name, Value);
    case ParamType::FCGroup:
        RenameGrp(Name, Value);
        break;
    default:
        break;
    }
}

void ParameterGrp::RemoveAttribute(ParamType Type, const char *Name)
{
    switch(Type) {
    case ParamType::FCBool:
        return RemoveBool(Name);
    case ParamType::FCInt:
        return RemoveInt(Name);
    case ParamType::FCUInt:
        return RemoveUnsigned(Name);
    case ParamType::FCText:
        return RemoveASCII(Name);
    case ParamType::FCFloat:
        return RemoveFloat(Name);
    case ParamType::FCGroup:
        return RemoveGrp(Name);
    default:
        break;
    }
}

const char *
ParameterGrp::GetAttribute(ParamType Type,
                           const char *Name,
                           std::string &Value,
                           const char *Default) const
{
    if (!_pGroupNode)
        return Default;

    const char *T = TypeName(Type);
    if (!T)
        return Default;

    DOMElement *pcElem = FindElement(_pGroupNode,T,Name);
    if (!pcElem)
        return Default;

    if (Type == ParamType::FCText)
        Value = GetASCII(Name, Default);
    else if (Type != ParamType::FCGroup)
        Value = StrX(pcElem->getAttribute(XStr("Value").unicodeForm())).c_str();
    return Value.c_str();
}

std::vector<std::pair<std::string, std::string>>
ParameterGrp::GetAttributeMap(ParamType Type, const char *sFilter) const
{
    std::vector<std::pair<std::string, std::string>> res;
    if (!_pGroupNode)
        return res;

    const char *T = TypeName(Type);
    if (!T)
        return res;

    std::string Name;

    DOMElement *pcTemp = FindElement(_pGroupNode, T);
    while ( pcTemp) {
        Name = StrX(static_cast<DOMElement*>(
                    pcTemp)->getAttributes()->getNamedItem(
                        XStr("Name").unicodeForm())->getNodeValue()).c_str();
        // check on filter condition
        if (!sFilter || Name.find(sFilter)!= std::string::npos) {
            if (Type == ParamType::FCGroup)
                res.emplace_back(Name, std::string());
            else
                res.emplace_back(Name, StrX(static_cast<DOMElement*>(pcTemp)->getAttribute(
                            XStr("Value").unicodeForm())).c_str());
        }
        pcTemp = FindNextElement(pcTemp,T);
    }
    return res;
}

void ParameterGrp::_Notify(ParamType Type, const char *Name, const char *Value)
{
    if (_Manager)
        _Manager->signalParamChanged(this, Type, Name, Value);
}

void ParameterGrp::_SetAttribute(ParamType T, const char *Name, const char *Value)
{ 
    const char *Type = TypeName(T);
    if (!Type)
        return;
    if (!_pGroupNode) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
            FC_WARN("Setting attribute " << Type << ":"
                    << Name << " in an orphan group " << _cName);
        return;
    }
    if (_Clearing) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
            FC_WARN("Adding attribute " << Type << ":"
                    << Name << " while clearing " << GetPath());
        return;
    }

    // find or create the Element
    DOMElement *pcElem = FindOrCreateElement(_pGroupNode,Type,Name);
    if (pcElem) {
        XStr attr("Value");
        // set the value only if different
        if (strcmp(StrX(pcElem->getAttribute(attr.unicodeForm())).c_str(),Value)!=0) {
            pcElem->setAttribute(attr.unicodeForm(), XStr(Value).unicodeForm());
            // trigger observer
            _Notify(T, Name, Value);
        }
        // For backward compatibility, old observer gets notified regardless of
        // value changes or not.
        Notify(Name);
    }
}

bool ParameterGrp::GetBool(const char* Name, bool bPreset) const
{
    if (!_pGroupNode)
        return bPreset;

    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCBool",Name);
    // if not return preset
    if (!pcElem)
        return bPreset;
    // if yes check the value and return
    if (strcmp(StrX(pcElem->getAttribute(XStr("Value").unicodeForm())).c_str(),"1"))
        return false;
    else
        return true;
}

void  ParameterGrp::SetBool(const char* Name, bool bValue)
{
    _SetAttribute(ParamType::FCBool, Name, bValue?"1":"0");
}

std::vector<bool> ParameterGrp::GetBools(const char * sFilter) const
{
    std::vector<bool>  vrValues;
    if (!_pGroupNode)
        return vrValues;

    std::string Name;

    DOMElement *pcTemp = FindElement(_pGroupNode,"FCBool");
    while ( pcTemp) {
        Name = StrX(pcTemp->getAttribute(XStr("Name").unicodeForm())).c_str();
        // check on filter condition
        if (!sFilter || Name.find(sFilter)!= std::string::npos) {
            if (strcmp(StrX(pcTemp->getAttribute(XStr("Value").unicodeForm())).c_str(),"1"))
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
    if (!_pGroupNode)
        return vrValues;

    std::string Name;

    DOMElement *pcTemp = FindElement(_pGroupNode,"FCBool");
    while ( pcTemp) {
        Name = StrX(pcTemp->getAttribute(XStr("Name").unicodeForm())).c_str();
        // check on filter condition
        if (!sFilter || Name.find(sFilter) != std::string::npos) {
            if (strcmp(StrX(pcTemp->getAttribute(XStr("Value").unicodeForm())).c_str(),"1"))
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
    if (!_pGroupNode)
        return lPreset;

    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCInt",Name);
    // if not return preset
    if (!pcElem)
        return lPreset;
    // if yes check the value and return
    return atol (StrX(pcElem->getAttribute(XStr("Value").unicodeForm())).c_str());
}

void  ParameterGrp::SetInt(const char* Name, long lValue)
{
    char cBuf[256];
    sprintf(cBuf,"%li",lValue);
    _SetAttribute(ParamType::FCInt, Name, cBuf);
}

std::vector<long> ParameterGrp::GetInts(const char * sFilter) const
{
    std::vector<long>  vrValues;
    if (!_pGroupNode)
        return vrValues;

    std::string Name;

    DOMElement *pcTemp = FindElement(_pGroupNode,"FCInt") ;
    while ( pcTemp ) {
        Name = StrX(pcTemp->getAttribute(XStr("Name").unicodeForm())).c_str();
        // check on filter condition
        if (!sFilter || Name.find(sFilter) != std::string::npos) {
            vrValues.push_back(atol(StrX(pcTemp->getAttribute(XStr("Value").unicodeForm())).c_str()) );
        }
        pcTemp = FindNextElement(pcTemp,"FCInt") ;
    }

    return vrValues;
}

std::vector<std::pair<std::string,long> > ParameterGrp::GetIntMap(const char * sFilter) const
{
    std::vector<std::pair<std::string,long> > vrValues;
    if (!_pGroupNode)
        return vrValues;

    std::string Name;

    DOMElement *pcTemp = FindElement(_pGroupNode,"FCInt") ;
    while ( pcTemp ) {
        Name = StrX(pcTemp->getAttribute(XStr("Name").unicodeForm())).c_str();
        // check on filter condition
        if (!sFilter || Name.find(sFilter) != std::string::npos) {
            vrValues.emplace_back(Name,
                               ( atol (StrX(pcTemp->getAttribute(XStr("Value").unicodeForm())).c_str())));
        }
        pcTemp = FindNextElement(pcTemp,"FCInt") ;
    }

    return vrValues;
}

unsigned long ParameterGrp::GetUnsigned(const char* Name, unsigned long lPreset) const
{
    if (!_pGroupNode)
        return lPreset;

    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCUInt",Name);
    // if not return preset
    if (!pcElem)
        return lPreset;
    // if yes check the value and return
    return strtoul (StrX(pcElem->getAttribute(XStr("Value").unicodeForm())).c_str(),nullptr,10);
}

void  ParameterGrp::SetUnsigned(const char* Name, unsigned long lValue)
{
    char cBuf[256];
    sprintf(cBuf,"%lu",lValue);
    _SetAttribute(ParamType::FCUInt, Name, cBuf);
}

std::vector<unsigned long> ParameterGrp::GetUnsigneds(const char * sFilter) const
{
    std::vector<unsigned long>  vrValues;
    if (!_pGroupNode)
        return vrValues;

    std::string Name;

    DOMElement *pcTemp = FindElement(_pGroupNode,"FCUInt");
    while ( pcTemp ) {
        Name = StrX(pcTemp->getAttribute(XStr("Name").unicodeForm())).c_str();
        // check on filter condition
        if (!sFilter || Name.find(sFilter) != std::string::npos) {
            vrValues.push_back( strtoul (StrX(pcTemp->getAttribute(XStr("Value").unicodeForm())).c_str(),nullptr,10) );
        }
        pcTemp = FindNextElement(pcTemp,"FCUInt") ;
    }

    return vrValues;
}

std::vector<std::pair<std::string,unsigned long> > ParameterGrp::GetUnsignedMap(const char * sFilter) const
{
    std::vector<std::pair<std::string,unsigned long> > vrValues;
    if (!_pGroupNode)
        return vrValues;

    std::string Name;

    DOMElement *pcTemp = FindElement(_pGroupNode,"FCUInt");
    while ( pcTemp ) {
        Name = StrX(pcTemp->getAttribute(XStr("Name").unicodeForm())).c_str();
        // check on filter condition
        if (!sFilter || Name.find(sFilter) != std::string::npos) {
            vrValues.emplace_back(Name,
                               ( strtoul (StrX(pcTemp->getAttribute(XStr("Value").unicodeForm())).c_str(),nullptr,10) ));
        }
        pcTemp = FindNextElement(pcTemp,"FCUInt");
    }

    return vrValues;
}

double ParameterGrp::GetFloat(const char* Name, double dPreset) const
{
    if (!_pGroupNode)
        return dPreset;

    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCFloat",Name);
    // if not return preset
    if (!pcElem)
        return dPreset;
    // if yes check the value and return
    return atof (StrX(pcElem->getAttribute(XStr("Value").unicodeForm())).c_str());
}

void  ParameterGrp::SetFloat(const char* Name, double dValue)
{
    char cBuf[256];
    sprintf(cBuf,"%.12f",dValue); // use %.12f instead of %f to handle values < 1.0e-6
    _SetAttribute(ParamType::FCFloat, Name, cBuf);
}

std::vector<double> ParameterGrp::GetFloats(const char * sFilter) const
{
    std::vector<double>  vrValues;
    if (!_pGroupNode)
        return vrValues;

    std::string Name;

    DOMElement *pcTemp = FindElement(_pGroupNode,"FCFloat") ;
    while ( pcTemp ) {
        Name = StrX(pcTemp->getAttribute(XStr("Name").unicodeForm())).c_str();
        // check on filter condition
        if (!sFilter || Name.find(sFilter) != std::string::npos) {
            vrValues.push_back( atof (StrX(pcTemp->getAttribute(XStr("Value").unicodeForm())).c_str()) );
        }
        pcTemp = FindNextElement(pcTemp,"FCFloat");
    }

    return vrValues;
}

std::vector<std::pair<std::string,double> > ParameterGrp::GetFloatMap(const char * sFilter) const
{
    std::vector<std::pair<std::string,double> > vrValues;
    if (!_pGroupNode)
        return vrValues;

    std::string Name;

    DOMElement *pcTemp = FindElement(_pGroupNode,"FCFloat") ;
    while ( pcTemp ) {
        Name = StrX(pcTemp->getAttribute(XStr("Name").unicodeForm())).c_str();
        // check on filter condition
        if (!sFilter || Name.find(sFilter) != std::string::npos) {
            vrValues.emplace_back(Name,
                               ( atof (StrX(pcTemp->getAttribute(XStr("Value").unicodeForm())).c_str())));
        }
        pcTemp = FindNextElement(pcTemp,"FCFloat");
    }

    return vrValues;
}


void  ParameterGrp::SetASCII(const char* Name, const char *sValue)
{
    if (!_pGroupNode) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
            FC_WARN("Setting attribute " << "FCText:"
                    << Name << " in an orphan group " << _cName);
        return;
    }
    if (_Clearing) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
            FC_WARN("Adding attribute " << "FCText:"
                    << Name << " while clearing " << GetPath());
        return;
    }

    bool isNew = false;
    DOMElement *pcElem = FindElement(_pGroupNode,"FCText",Name);
    if (!pcElem) {
        pcElem = CreateElement(_pGroupNode,"FCText",Name);
        isNew = true;
    }
    if (pcElem) {
        // and set the value
        DOMNode *pcElem2 = pcElem->getFirstChild();
        if (!pcElem2) {
            XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument *pDocument = _pGroupNode->getOwnerDocument();
            DOMText *pText = pDocument->createTextNode(XUTF8Str(sValue).unicodeForm());
            pcElem->appendChild(pText);
            if (isNew  || sValue[0]!=0)
                _Notify(ParamType::FCText, Name, sValue);
        }
        else if (strcmp(StrXUTF8(pcElem2->getNodeValue()).c_str(), sValue)!=0) {
            pcElem2->setNodeValue(XUTF8Str(sValue).unicodeForm());
            _Notify(ParamType::FCText, Name, sValue);
        }
        // trigger observer
        Notify(Name);
    }
}

std::string ParameterGrp::GetASCII(const char* Name, const char * pPreset) const
{
    if (!_pGroupNode)
        return pPreset ? pPreset : "";

    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCText",Name);
    // if not return preset
    if (!pcElem) {
        if (!pPreset)
            return {};
        return {pPreset};
    }
    // if yes check the value and return
    DOMNode *pcElem2 = pcElem->getFirstChild();
    if (pcElem2)
        return {StrXUTF8(pcElem2->getNodeValue()).c_str()};
    return {};
}

std::vector<std::string> ParameterGrp::GetASCIIs(const char * sFilter) const
{
    std::vector<std::string>  vrValues;
    if (!_pGroupNode)
        return vrValues;

    std::string Name;

    DOMElement *pcTemp = FindElement(_pGroupNode,"FCText");
    while ( pcTemp  ) {
        Name = StrXUTF8(pcTemp->getAttribute(XStr("Name").unicodeForm())).c_str();
        // check on filter condition
        if (!sFilter || Name.find(sFilter) != std::string::npos) {
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
    if (!_pGroupNode)
        return vrValues;

    std::string Name;

    DOMElement *pcTemp = FindElement(_pGroupNode,"FCText");
    while ( pcTemp) {
        Name = StrXUTF8(pcTemp->getAttribute(XStr("Name").unicodeForm())).c_str();
        // check on filter condition
        if (!sFilter || Name.find(sFilter) != std::string::npos) {
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
    if (!_pGroupNode)
        return;

    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCText",Name);
    // if not return
    if (!pcElem)
        return;

    DOMNode* node = _pGroupNode->removeChild(pcElem);
    node->release();

    // trigger observer
    _Notify(ParamType::FCText, Name, nullptr);
    Notify(Name);
}

void ParameterGrp::RemoveBool(const char* Name)
{
    if (!_pGroupNode)
        return;

    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCBool",Name);
    // if not return
    if (!pcElem)
        return;

    DOMNode* node = _pGroupNode->removeChild(pcElem);
    node->release();

    // trigger observer
    _Notify(ParamType::FCBool, Name, nullptr);
    Notify(Name);
}


void ParameterGrp::RemoveFloat(const char* Name)
{
    if (!_pGroupNode)
        return;

    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCFloat",Name);
    // if not return
    if (!pcElem)
        return;

    DOMNode* node = _pGroupNode->removeChild(pcElem);
    node->release();

    // trigger observer
    _Notify(ParamType::FCFloat,Name, nullptr);
    Notify(Name);
}

void ParameterGrp::RemoveInt(const char* Name)
{
    if (!_pGroupNode)
        return;

    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCInt",Name);
    // if not return
    if (!pcElem)
        return;

    DOMNode* node = _pGroupNode->removeChild(pcElem);
    node->release();

    // trigger observer
    _Notify(ParamType::FCInt, Name, nullptr);
    Notify(Name);
}

void ParameterGrp::RemoveUnsigned(const char* Name)
{
    if (!_pGroupNode)
        return;

    // check if Element in group
    DOMElement *pcElem = FindElement(_pGroupNode,"FCUInt",Name);
    // if not return
    if (!pcElem)
        return;

    DOMNode* node = _pGroupNode->removeChild(pcElem);
    node->release();

    // trigger observer
    _Notify(ParamType::FCUInt, Name, nullptr);
    Notify(Name);
}

void ParameterGrp::RemoveGrp(const char* Name)
{
    if (!_pGroupNode)
        return;

    auto it = _GroupMap.find(Name);
    if (it == _GroupMap.end())
        return;

    // If this or any of its children is referenced by an observer we do not
    // delete the handle, just in case the group is later added again, or else
    // those existing observer won't get any notification. BUT, we DO delete
    // the underlying xml elements, so that we don't save the empty group
    // later.
    it->second->Clear(false);
    if (!it->second->_Detached) {
        it->second->_Detached = true;
        _pGroupNode->removeChild(it->second->_pGroupNode);
    }
    if (it->second->ShouldRemove()) {
        it->second->_Parent = nullptr;
        it->second->_Manager = nullptr;
        _GroupMap.erase(it);
    }

    // trigger observer
    Notify(Name);
}

bool ParameterGrp::RenameGrp(const char* OldName, const char* NewName)
{
    if (!_pGroupNode)
        return false;

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

    _Notify(ParamType::FCGroup, NewName, OldName);
    return true;
}

void ParameterGrp::Clear(bool notify)
{
    if (!_pGroupNode)
        return;

    Base::StateLocker guard(_Clearing);

    // early trigger notification of group removal when all its children
    // hierarchies are intact.
    _Notify(ParamType::FCGroup, nullptr, nullptr);

    // checking on references
    for (auto it = _GroupMap.begin();it!=_GroupMap.end();) {
        // If a group handle is referenced by some observer, then do not remove
        // it but clear it, so that any existing observer can still get
        // notification if the group is later on add back. We do remove the
        // underlying xml element from its parent so that we won't save this
        // empty group.
        it->second->Clear(notify);
        if (!it->second->_Detached) {
            it->second->_Detached = true;
            _pGroupNode->removeChild(it->second->_pGroupNode);
        }
        if (!it->second->ShouldRemove())
            ++it;
        else {
            it->second->_Parent = nullptr;
            it->second->_Manager = nullptr;
            it = _GroupMap.erase(it);
        }
    }

    // Remove the rest of non-group nodes;
    std::vector<std::pair<ParamType, std::string>> params;
    for (DOMNode *child = _pGroupNode->getFirstChild(), *next = child; child != nullptr;  child = next) {
        next = next->getNextSibling();
        ParamType type = TypeValue(StrX(child->getNodeName()).c_str());
        if (type != ParamType::FCInvalid && type != ParamType::FCGroup)
            params.emplace_back(type, StrX(child->getAttributes()->getNamedItem(
                            XStr("Name").unicodeForm())->getNodeValue()).c_str());
        DOMNode *node = _pGroupNode->removeChild(child);
        node->release();
    }

    for (auto &v : params) {
        _Notify(v.first, v.second.c_str(), nullptr);
        if (notify)
            Notify(v.second.c_str());
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
    for (const auto& it : _GroupMap) {
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
                        DOMNode* attr = FindAttribute(clChild, "Name");
                        if (attr && !strcmp(Name,StrX(attr->getNodeValue()).c_str()))
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

XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ParameterGrp::FindOrCreateElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *Start, const char* Type, const char* Name)
{
    // first try to find it
    DOMElement *pcElem = FindElement(Start,Type,Name);
    if (pcElem)
        return pcElem;

    return CreateElement(Start,Type,Name);
}

XERCES_CPP_NAMESPACE_QUALIFIER DOMNode *ParameterGrp::FindAttribute(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode *Node, const char* Name) const
{
    DOMNamedNodeMap* attr = Node->getAttributes();
    if (attr) {
        return attr->getNamedItem(XStr(Name).unicodeForm());
    }
    return nullptr;
}

std::vector<std::pair<ParameterGrp::ParamType,std::string> >
ParameterGrp::GetParameterNames(const char * sFilter) const
{
    std::vector<std::pair<ParameterGrp::ParamType,std::string> > res;
    if (!_pGroupNode)
        return res;

    std::string Name;

    for (DOMNode *clChild = _pGroupNode->getFirstChild();
            clChild != nullptr;  clChild = clChild->getNextSibling()) {
        if (clChild->getNodeType() == DOMNode::ELEMENT_NODE) {
            StrX type(clChild->getNodeName());
            ParamType Type = TypeValue(type.c_str());
            if (Type != ParamType::FCInvalid && Type != ParamType::FCGroup) {
                if (clChild->getAttributes()->getLength() > 0) {
                    StrX name(clChild->getAttributes()->getNamedItem(
                                XStr("Name").unicodeForm())->getNodeValue());
                    if (!sFilter || strstr(name.c_str(), sFilter))
                        res.emplace_back(Type, name.c_str());
                }
            }
        }
    }
    return res;
}

void ParameterGrp::NotifyAll()
{
    // get all ints and notify
    std::vector<std::pair<std::string,long> > IntMap = GetIntMap();
    for (const auto & it : IntMap)
        Notify(it.first.c_str());

    // get all booleans and notify
    std::vector<std::pair<std::string,bool> > BoolMap = GetBoolMap();
    for (const auto & it : BoolMap)
        Notify(it.first.c_str());

    // get all Floats and notify
    std::vector<std::pair<std::string,double> > FloatMap  = GetFloatMap();
    for (const auto & it : FloatMap)
        Notify(it.first.c_str());

    // get all strings and notify
    std::vector<std::pair<std::string,std::string> > StringMap = GetASCIIMap();
    for (const auto & it : StringMap)
        Notify(it.first.c_str());

    // get all uints and notify
    std::vector<std::pair<std::string,unsigned long> > UIntMap = GetUnsignedMap();
    for (const auto & it : UIntMap)
        Notify(it.first.c_str());
}

void ParameterGrp::_Reset()
{
    _pGroupNode = nullptr;
    for (auto &v : _GroupMap)
        v.second->_Reset();
}

//**************************************************************************
//**************************************************************************
// ParameterSerializer
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
ParameterSerializer::ParameterSerializer(const std::string& fn)
  : filename(fn)
{
}

ParameterSerializer::~ParameterSerializer() = default;

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
{
    _Manager = this;

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
    _Reset();
    delete _pDocument;
    delete paramSerializer;
}

Base::Reference<ParameterManager> ParameterManager::Create()
{
    return {new ParameterManager()};
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

const std::string & ParameterManager::GetSerializeFileName() const
{
    static const std::string _dummy;
    return paramSerializer ? paramSerializer->GetFileName() : _dummy;
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
                myFilter = std::make_unique<DOMPrintFilter>(
                            DOMNodeFilter::SHOW_ELEMENT   |
                            DOMNodeFilter::SHOW_ATTRIBUTE |
                            DOMNodeFilter::SHOW_DOCUMENT_TYPE |
                            DOMNodeFilter::SHOW_TEXT
                           );
                theSerializer->setFilter(myFilter.get());
            }

            // plug in user's own error handler
            myErrorHandler = std::make_unique<DOMPrintErrorHandler>();
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
    _pGroupNode->setAttribute(XStr("Name").unicodeForm(), XStr("Root").unicodeForm());
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
// DOMPrintFilter
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
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
        case DOMNode::TEXT_NODE: {
            // Filter out text element if it is under a group node. Note text xml
            // element is plain text in between tags, and we do not store any text
            // there.
            auto parent = node->getParentNode();
            if (parent
                && XMLString::compareString(parent->getNodeName(),
                                            XStr("FCParamGroup").unicodeForm()) == 0)
                return DOMNodeFilter::FILTER_REJECT;
            return DOMNodeFilter::FILTER_ACCEPT;
        }
        case DOMNode::DOCUMENT_TYPE_NODE:
        case DOMNode::DOCUMENT_NODE: {
            return DOMNodeFilter::FILTER_REJECT;// no effect
        }
        default: {
            return DOMNodeFilter::FILTER_ACCEPT;
        }
    }
}

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
