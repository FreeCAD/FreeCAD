/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <assert.h>
# include <sstream>
#endif

#include <QFileInfo>
#include <QDateTime>
#include <QDir>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/bind.hpp>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <CXX/Objects.hxx>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#include "Application.h"
#include "DocumentObject.h"
#include "DocumentObjectPy.h"
#include "Document.h"
#include "GeoFeature.h"

#include "PropertyLinks.h"

FC_LOG_LEVEL_INIT("PropertyLinks",true,true);

using namespace App;
using namespace Base;
using namespace std;

//**************************************************************************
//**************************************************************************
// PropertyLinkBase
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyLinkBase , App::Property)

static std::unordered_map<std::string, std::set<PropertyLinkBase*> > _LabelMap;

static std::unordered_map<App::DocumentObject *, std::unordered_set<PropertyLinkBase*> > _ElementRefMap;

PropertyLinkBase::PropertyLinkBase()
{}

PropertyLinkBase::~PropertyLinkBase() {
    unregisterLabelReferences();
    unregisterElementReference();
}

void PropertyLinkBase::setAllowExternal(bool allow) {
    setFlag(LinkAllowExternal,allow);
}

void PropertyLinkBase::hasSetValue() {
    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    if(owner)
        owner->clearOutListCache();
    Property::hasSetValue();
}

void PropertyLinkBase::unregisterElementReference() {
    for(auto obj : _ElementRefs) {
        auto it = _ElementRefMap.find(obj);
        if(it != _ElementRefMap.end()) {
            it->second.erase(this);
            if(it->second.empty())
                _ElementRefMap.erase(it);
        }
    }
    _ElementRefs.clear();
}

void PropertyLinkBase::unregisterLabelReferences() 
{
    for(auto &label : _LabelRefs) {
        auto it = _LabelMap.find(label);
        if(it!=_LabelMap.end()) {
            it->second.erase(this);
            if(it->second.empty())
                _LabelMap.erase(it);
        }
    }
    _LabelRefs.clear();
}

void PropertyLinkBase::getLabelReferences(std::vector<std::string> &subs,const char *subname) {
    const char *dot;
    for(;(subname=strchr(subname,'$'))!=0; subname=dot+1) {
        ++subname;
        dot = strchr(subname,'.');
        if(!dot) break;
        subs.emplace_back(subname,dot-subname);
    }
}

void PropertyLinkBase::registerLabelReferences(std::vector<std::string> &&labels, bool reset) {
    if(reset)
        unregisterLabelReferences();
    for(auto &label : labels) {
        auto res = _LabelRefs.insert(std::move(label));
        if(res.second)
            _LabelMap[*res.first].insert(this);
    }
}

void PropertyLinkBase::checkLabelReferences(const std::vector<std::string> &subs, bool reset) {
    if(reset)
        unregisterLabelReferences();
    std::vector<std::string> labels;
    for(auto &sub : subs) {
        labels.clear();
        getLabelReferences(labels,sub.c_str());
        registerLabelReferences(std::move(labels),false);
    }
}

std::string PropertyLinkBase::updateLabelReference(const App::DocumentObject *parent, 
        const char *subname, App::DocumentObject *obj, const std::string &ref, const char *newLabel)
{
    if(!obj || !obj->getNameInDocument() || !parent || !parent->getNameInDocument())
        return std::string();

    // Because the label is allowed to be the same across different
    // hierarchy, we have to search for all occurance, and make sure the
    // referenced sub-object at the found hierarchy is actually the given
    // object.
    for(const char *pos=subname; ((pos=strstr(pos,ref.c_str()))!=0); pos+=ref.size()) {
        auto sub = std::string(subname,pos+ref.size()-subname);
        auto sobj = parent->getSubObject(sub.c_str());
        if(sobj == obj) {
            sub = subname;
            sub.replace(pos+1-subname,ref.size()-2,newLabel);
            return sub;
        }
    }
    return std::string();
}

std::vector<std::pair<Property*, std::unique_ptr<Property> > > 
PropertyLinkBase::updateLabelReferences(App::DocumentObject *obj, const char *newLabel) 
{
    std::vector<std::pair<Property*,std::unique_ptr<Property> > >  ret;
    if(!obj || !obj->getNameInDocument())
        return ret;
    auto it = _LabelMap.find(obj->Label.getStrValue());
    if(it == _LabelMap.end())
        return ret;
    std::string ref("$");
    ref += obj->Label.getValue();
    ref += '.';
    std::vector<PropertyLinkBase*> props;
    props.reserve(it->second.size());
    props.insert(props.end(),it->second.begin(),it->second.end());
    for(auto prop : props) {
        if(!prop->getContainer())
            continue;
        std::unique_ptr<Property> copy(prop->CopyOnLabelChange(obj,ref,newLabel));
        if(copy)
            ret.emplace_back(prop,std::move(copy));
    }
    return ret;
}

static std::string propertyName(Property *prop) {
    if(!prop)
        return std::string();
    auto obj = Base::freecad_dynamic_cast<DocumentObject>(prop->getContainer());
    std::string name;
    if(obj) 
        name = obj->getFullName() + ".";
    if(prop->getContainer()) {
        auto n = prop->getName();
        if(n) {
            name += n;
            return name;
        }
    }
    auto xlink = Base::freecad_dynamic_cast<PropertyXLink>(prop);
    if(xlink)
        return propertyName(xlink->parent());
    return name;
}

void PropertyLinkBase::updateElementReferences(DocumentObject *feature, bool reverse) {
    if(!feature || !feature->getNameInDocument())
        return;
    auto it = _ElementRefMap.find(feature);
    if(it == _ElementRefMap.end())
        return;
    std::vector<PropertyLinkBase*> props;
    props.reserve(it->second.size());
    props.insert(props.end(),it->second.begin(),it->second.end());
    for(auto prop : props) {
        if(prop->getContainer()) {
            try {
                prop->updateElementReference(feature,reverse,true);
            }catch(Base::Exception &e) {
                e.ReportException();
                FC_ERR("Failed to update element reference of " << propertyName(prop));
            }catch(std::exception &e) {
                FC_ERR("Failed to update element reference of " << propertyName(prop)
                        << ": " << e.what());
            }
        }
    }
}

void PropertyLinkBase::_registerElementReference(App::DocumentObject *obj, std::string &sub, ShadowSub &shadow)
{
    if(!obj || !obj->getNameInDocument() || sub.empty())
        return;
    if(shadow.first.empty()) {
        _updateElementReference(0,obj,sub,shadow,false);
        return;
    }
    const char *element=0;
    auto sobj = obj->resolve(sub.c_str(),0,0,&element);
    if(!sobj || !element || !element[0])
        return;

    sobj = sobj->getLinkedObject(true);
    if(sobj && sobj->isDerivedFrom(GeoFeature::getClassTypeId())) {
        if(_ElementRefs.insert(sobj).second)
            _ElementRefMap[sobj].insert(this);
    }
}

class StringGuard {
public:
    StringGuard(char *c)
        :c(c)
    {
        v1 = c[0];
        v2 = c[1];
        c[0] = '.';
        c[1] = 0;
    }
    ~StringGuard()
    {
        c[0] = v1;
        c[1] = v2;
    }

    char *c;
    char v1;
    char v2;
};

void PropertyLinkBase::restoreLabelReference(const DocumentObject *obj, 
        std::string &subname, ShadowSub *shadow)
{
    std::ostringstream ss;
    char *sub = &subname[0];
    char *next=sub;
    for(char *dot=strchr(next,'.');dot;next=dot+1,dot=strchr(next,'.')) {
        if(dot!=next && dot[-1]!='@')
            continue;
        DocumentObject *sobj;
        try {
            StringGuard guard(dot-1);
            sobj = obj->getSubObject(subname.c_str());
            if(!sobj) {
                FC_ERR("Failed to restore lable reference " << obj->getFullName() 
                        << '.' << ss.str());
                return;
            }
        }catch(...){
            throw;
        }
        ss.write(sub,next-sub);
        ss << '$' << sobj->Label.getStrValue() << '.';
        sub = dot+1;
    }
    if(sub == subname.c_str())
        return;

    size_t count = sub-subname.c_str();
    const auto &newSub = ss.str();
    if(shadow && shadow->second.size()>=count) 
        shadow->second = newSub + (shadow->second.c_str()+count);
    if(shadow && shadow->first.size()>=count) 
        shadow->first = newSub + (shadow->first.c_str()+count);
    subname = newSub + sub;
}

bool PropertyLinkBase::_updateElementReference(DocumentObject *feature, 
        App::DocumentObject *obj, std::string &sub, ShadowSub &shadow, 
        bool reverse, bool notify)
{
    if(!obj || !obj->getNameInDocument()) return false;
    ShadowSub elementName;
    const char *subname;
    if(shadow.first.size())
        subname = shadow.first.c_str();
    else if(shadow.second.size())
        subname = shadow.second.c_str();
    else
        subname = sub.c_str();
    GeoFeature *geo = 0;
    const char *element=0;
    auto ret = GeoFeature::resolveElement(obj,subname, elementName,true,
                GeoFeature::ElementNameType::Export,feature,&element,&geo);
    if(!ret || !geo || !element || !element[0]) {
        if(elementName.second.size())
            shadow.second.swap(elementName.second);
        return false;
    }

    if(_ElementRefs.insert(geo).second)
        _ElementRefMap[geo].insert(this);

    if(!reverse && elementName.first.empty()) {
        shadow.second.swap(elementName.second);
        return false;
    }
    if(shadow==elementName)
        return false;

    if(GeoFeature::hasMissingElement(elementName.second.c_str())) {
        if(reverse && shadow.first.size()) {
            // reverse means we are trying to either generate the element name for
            // the first time, or upgrade to a new map version. In case of
            // upgrading, we still consult the original mapped name in first
            // try. Here means the first try failed, so we try the non-mapped name
            ShadowSub shadowCopy;
            shadowCopy.second = shadow.second;
            bool res = _updateElementReference(feature,obj,sub,shadowCopy,reverse,notify);
            if(shadowCopy.first.size()) 
                shadow.swap(shadowCopy);
            else
                shadow.second.swap(shadowCopy.second);
            return res;
        }
        if(notify)
            aboutToSetValue();
        FC_WARN(propertyName(this) 
                << " missing element reference " << ret->getFullName() << " "
                << (elementName.first.size()?elementName.first:elementName.second));
        shadow.second.swap(elementName.second);
    }else{
        if(notify)
            aboutToSetValue();
        FC_LOG(propertyName(this) 
                << " element reference shadow update " << ret->getFullName() << " "
                << shadow.first << " -> " << elementName.first);
        shadow.swap(elementName);
    }
    if(reverse) {
        if(shadow.first.size() && Data::ComplexGeoData::hasMappedElementName(sub.c_str()))
            sub = shadow.first;
        else
            sub = shadow.second;
        return true;
    }
    auto pos2 = shadow.first.rfind('.');
    if(pos2 == std::string::npos)
        return true;
    ++pos2;
    auto pos = sub.rfind('.');
    if(pos == std::string::npos)
        pos = 0;
    else
        ++pos;
    if(pos==pos2) {
        if(sub.compare(pos,sub.size()-pos,&shadow.first[pos2])!=0) {
            FC_LOG("element reference update " << sub << " -> " << shadow.first);
            sub.replace(pos,sub.size()-pos,&shadow.first[pos2]);
        } 
    } else if(sub!=shadow.second) {
        FC_LOG("element reference update " << sub << " -> " << shadow.second);
        sub = shadow.second;
    }
    return true;
}


//**************************************************************************
//**************************************************************************
// PropertyLinkListBase
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyLinkListBase , App::PropertyLinkBase)

//**************************************************************************
//**************************************************************************
// PropertyLink
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLink, App::PropertyLinkBase)
TYPESYSTEM_SOURCE(App::PropertyLinkChild , App::PropertyLink)
TYPESYSTEM_SOURCE(App::PropertyLinkGlobal , App::PropertyLink)
TYPESYSTEM_SOURCE(App::PropertyLinkHidden , App::PropertyLink)

//**************************************************************************
// Construction/Destruction


PropertyLink::PropertyLink()
:_pcLink(0)
{

}


PropertyLink::~PropertyLink()
{
    resetLink();
}

//**************************************************************************
// Base class implementer

void PropertyLink::resetLink() {
    //in case this property gets dynamically removed
#ifndef USE_OLD_DAG
    // maintain the back link in the DocumentObject class if it is from a document object
    if (_pcScope!=LinkScope::Hidden && 
        _pcLink && 
        getContainer() && 
        getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) 
    {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            if (_pcLink)
                _pcLink->_removeBackLink(parent);
        }
    }
#endif
    _pcLink = 0;
}

void PropertyLink::setValue(App::DocumentObject * lValue)
{
    auto parent = dynamic_cast<App::DocumentObject*>(getContainer());
    if(!testFlag(LinkAllowExternal) && parent && lValue && parent->getDocument()!=lValue->getDocument())
        throw Base::ValueError("PropertyLink does not support external object");

    aboutToSetValue();
#ifndef USE_OLD_DAG
    // maintain the back link in the DocumentObject class if it is from a document object
    if (_pcScope!=LinkScope::Hidden && parent) {
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            if (_pcLink)
                _pcLink->_removeBackLink(parent);
            if (lValue)
                lValue->_addBackLink(parent);
        }
    }
#endif
    _pcLink=lValue;
    hasSetValue();
}

App::DocumentObject * PropertyLink::getValue(void) const
{
    return _pcLink;
}

App::DocumentObject * PropertyLink::getValue(Base::Type t) const
{
    return (_pcLink && _pcLink->getTypeId().isDerivedFrom(t)) ? _pcLink : 0;
}

PyObject *PropertyLink::getPyObject(void)
{
    if (_pcLink)
        return _pcLink->getPyObject();
    else
        Py_Return;
}

void PropertyLink::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(DocumentObjectPy::Type))) {
        DocumentObjectPy  *pcObject = (DocumentObjectPy*)value;
        setValue(pcObject->getDocumentObjectPtr());
    }
    else if (Py_None == value) {
        setValue(0);
    }
    else {
        std::string error = std::string("type must be 'DocumentObject' or 'NoneType', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyLink::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Link value=\"" <<  (_pcLink?_pcLink->getExportName():"") <<"\"/>" << std::endl;
}

void PropertyLink::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("Link");
    // get the value of my attribute
    std::string name = reader.getName(reader.getAttribute("value"));

    // Property not in a DocumentObject!
    assert(getContainer()->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId()) );

    if (name != "") {
        DocumentObject* parent = static_cast<DocumentObject*>(getContainer());

        App::Document* document = parent->getDocument();
        DocumentObject* object = document ? document->getObject(name.c_str()) : 0;
        if (!object) {
            if (reader.isVerbose()) {
                Base::Console().Warning("Lost link to '%s' while loading, maybe "
                                        "an object was not loaded correctly\n",name.c_str());
            }
        }
        else if (parent == object) {
            if (reader.isVerbose()) {
                Base::Console().Warning("Object '%s' links to itself, nullify it\n",name.c_str());
            }
            object = 0;
        }

        setValue(object);
    }
    else {
        setValue(0);
    }
}

Property *PropertyLink::Copy(void) const
{
    PropertyLink *p= new PropertyLink();
    p->_pcLink = _pcLink;
    return p;
}

void PropertyLink::Paste(const Property &from)
{
    if (!from.isDerivedFrom(PropertyLink::getClassTypeId()))
        throw Base::TypeError("Incompatible property to paste to");

    setValue(static_cast<const PropertyLink&>(from)._pcLink);
}

void PropertyLink::getLinks(std::vector<App::DocumentObject *> &objs, 
        bool all, std::vector<std::string> *subs, bool newStyle) const
{
    (void)newStyle;
    (void)subs;
    if((all||_pcScope!=LinkScope::Hidden) && _pcLink && _pcLink->getNameInDocument())
        objs.push_back(_pcLink);
}

void PropertyLink::breakLink(App::DocumentObject *obj, bool clear) {
    if(_pcLink == obj || (clear && getContainer()==obj))
        setValue(0);
}

static inline void adjustLinkError(bool relative, 
        App::Property *prop, App::DocumentObject *link, const char *subname="") 
{
    const char *msg = relative?"Failed to adjust relateive link":"Cyclic link";
    auto owner = dynamic_cast<DocumentObject*>(prop->getContainer());
    if(!owner)
        THROWM(Base::RuntimeError,msg);
    std::ostringstream ss;
    ss << msg << " to " << link->getFullName() << '.' << subname
        << " by " << owner->getFullName() << '.' << prop->getName();
    THROWM(Base::RuntimeError,ss.str());
}

bool PropertyLink::adjustLink(const std::set<App::DocumentObject*> &inList) {
    if (_pcScope==LinkScope::Hidden)
        return false;
    if(_pcLink && _pcLink->getNameInDocument() && inList.count(_pcLink))
        adjustLinkError(false,this,_pcLink);
    return false;
}

//**************************************************************************
// PropertyLinkList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLinkList, App::PropertyLinkListBase)
TYPESYSTEM_SOURCE(App::PropertyLinkListChild, App::PropertyLinkList)
TYPESYSTEM_SOURCE(App::PropertyLinkListGlobal, App::PropertyLinkList)
TYPESYSTEM_SOURCE(App::PropertyLinkListHidden, App::PropertyLinkList)

//**************************************************************************
// Construction/Destruction


PropertyLinkList::PropertyLinkList()
{

}

PropertyLinkList::~PropertyLinkList()
{
    //in case this property gety dynamically removed
#ifndef USE_OLD_DAG   
    //maintain the back link in the DocumentObject class
    if (_pcScope!=LinkScope::Hidden && 
        !_lValueList.empty() && 
        getContainer() && 
        getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) 
    {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            for(auto *obj : _lValueList)
                obj->_removeBackLink(parent);
        }
    }
#endif

}

void PropertyLinkList::setSize(int newSize)
{
    for(int i=newSize;i<(int)_lValueList.size();++i) {
        auto obj = _lValueList[i];
        if(!obj && !obj->getNameInDocument())
            continue;
        _nameMap.erase(obj->getNameInDocument());
#ifndef USE_OLD_DAG
        if (_pcScope!=LinkScope::Hidden)
            obj->_removeBackLink(static_cast<DocumentObject*>(getContainer()));
#endif
    }
    _lValueList.resize(newSize);
}

void PropertyLinkList::setSize(int newSize, const_reference def) {
    auto oldSize = getSize();
    setSize(newSize);
    for(auto i=oldSize;i<newSize;++i)
        _lValueList[i] = def;
}

void PropertyLinkList::set1Value(int idx, DocumentObject* const &value, bool touch) {
    DocumentObject *obj = 0;
    if(idx>=0 && idx<(int)_lValueList.size()) {
        obj = _lValueList[idx];
        if(obj == value) return;
    }

    if(!value || !value->getNameInDocument())
        throw Base::ValueError("invalid document object");

    _nameMap.clear();

#ifndef USE_OLD_DAG   
    if (getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope!=LinkScope::Hidden) {
            if(obj) 
                obj->_removeBackLink(static_cast<DocumentObject*>(getContainer()));
            if(value)
                value->_addBackLink(static_cast<DocumentObject*>(getContainer()));
        }
    }
#endif

    _set1Value(idx,value,touch);
}

void PropertyLinkList::setValues(const std::vector<DocumentObject*>& lValue) {
    if(lValue.size()==1 && !lValue[0]) {
        // one null element means clear, as backward compatibility for old code
        setValues(std::vector<DocumentObject*>());
        return;
    }

    auto parent = dynamic_cast<App::DocumentObject*>(getContainer());
    for(auto obj : lValue) {
        if(!obj || !obj->getNameInDocument())
            throw Base::ValueError("PropertyLinkList: invalid document object");
        if(!testFlag(LinkAllowExternal) && parent && parent->getDocument()!=obj->getDocument())
            throw Base::ValueError("PropertyLinkList does not support external object");
    }
    _nameMap.clear();

#ifndef USE_OLD_DAG
    //maintain the back link in the DocumentObject class
    if (parent) {
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope!=LinkScope::Hidden) {
            for(auto obj : _lValueList)
                if(obj) obj->_removeBackLink(static_cast<DocumentObject*>(getContainer()));
            for(auto obj : lValue)
                if(obj) obj->_addBackLink(static_cast<DocumentObject*>(getContainer()));
        }
    }
#endif
    inherited::setValues(lValue);
}

PyObject *PropertyLinkList::getPyObject(void)
{
    int count = getSize();
#if 0//FIXME: Should switch to tuple
    Py::Tuple sequence(count);
#else
    Py::List sequence(count);
#endif
    for (int i = 0; i<count; i++) {
        auto obj = _lValueList[i];
        if(obj && obj->getNameInDocument())
            sequence.setItem(i, Py::asObject(_lValueList[i]->getPyObject()));
        else
            sequence.setItem(i, Py::None());
    }

    return Py::new_reference_to(sequence);
}

DocumentObject *PropertyLinkList::getPyValue(PyObject *item) const {
    if (!PyObject_TypeCheck(item, &(DocumentObjectPy::Type))) {
        std::string error = std::string("type must be 'DocumentObject' or list of 'DocumentObject', not ");
        error += item->ob_type->tp_name;
        throw Base::TypeError(error);
    }
    return static_cast<DocumentObjectPy*>(item)->getDocumentObjectPtr();
}

void PropertyLinkList::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<LinkList count=\"" << getSize() << "\">" << endl;
    writer.incInd();
    for (int i = 0; i<getSize(); i++) {
        DocumentObject* obj = _lValueList[i];
        if (obj)
            writer.Stream() << writer.ind() << "<Link value=\"" << obj->getExportName() << "\"/>" << endl;
        else
            writer.Stream() << writer.ind() << "<Link value=\"\"/>" << endl;
    }

    writer.decInd();
    writer.Stream() << writer.ind() << "</LinkList>" << endl;
}

void PropertyLinkList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("LinkList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");
    App::PropertyContainer* container = getContainer();
    if (!container)
        throw Base::RuntimeError("Property is not part of a container");
    if (!container->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        std::stringstream str;
        str << "Container is not a document object ("
            << container->getTypeId().getName() << ")";
        throw Base::TypeError(str.str());
    }

    std::vector<DocumentObject*> values;
    values.reserve(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("Link");
        std::string name = reader.getName(reader.getAttribute("value"));
        // In order to do copy/paste it must be allowed to have defined some
        // referenced objects in XML which do not exist anymore in the new
        // document. Thus, we should silently ignore this.
        // Property not in an object!
        DocumentObject* father = static_cast<DocumentObject*>(getContainer());
        App::Document* document = father->getDocument();
        DocumentObject* child = document ? document->getObject(name.c_str()) : 0;
        if (child)
            values.push_back(child);
        else if (reader.isVerbose())
            FC_WARN("Lost link to " << (document?document->getName():"") << " " << name 
                    << " while loading, maybe an object was not loaded correctly");
    }

    reader.readEndElement("LinkList");

    // assignment
    setValues(values);
}

Property *PropertyLinkList::Copy(void) const
{
    PropertyLinkList *p = new PropertyLinkList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyLinkList::Paste(const Property &from)
{
    if(!from.isDerivedFrom(PropertyLinkList::getClassTypeId()))
        throw Base::TypeError("Incompatible property to paste to");

    setValues(static_cast<const PropertyLinkList&>(from)._lValueList);
}

unsigned int PropertyLinkList::getMemSize(void) const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(App::DocumentObject *));
}

DocumentObject *PropertyLinkList::find(const std::string &name, int *pindex) const {
    if(_nameMap.empty() || _nameMap.size()>_lValueList.size()) {
        _nameMap.clear();
        for(int i=0;i<(int)_lValueList.size();++i) {
            auto obj = _lValueList[i];
            if(obj && obj->getNameInDocument()) 
                _nameMap[obj->getNameInDocument()] = i;
        }
    }
    auto it = _nameMap.find(name);
    if(it == _nameMap.end())
        return 0;
    if(pindex) *pindex = it->second;
    return _lValueList[it->second];
}

void PropertyLinkList::getLinks(std::vector<App::DocumentObject *> &objs, 
        bool all, std::vector<std::string> *subs, bool newStyle) const
{
    (void)subs;
    (void)newStyle;
    if(all||_pcScope!=LinkScope::Hidden) {
        objs.reserve(objs.size()+_lValueList.size());
        for(auto obj : _lValueList) {
            if(obj && obj->getNameInDocument())
                objs.push_back(obj);
        }
    }
}

void PropertyLinkList::breakLink(App::DocumentObject *obj, bool clear) {
    if(clear && getContainer()==obj) {
        setValues({});
        return;
    }
    std::vector<App::DocumentObject*> values;
    values.reserve(_lValueList.size());
    for(auto o : _lValueList) {
        if(o != obj)
            values.push_back(o);
    }
    if(values.size()!=_lValueList.size())
        setValues(values);
}

bool PropertyLinkList::adjustLink(const std::set<App::DocumentObject*> &inList) {
    if (_pcScope==LinkScope::Hidden)
        return false;
    for(auto o : _lValueList) {
        if(o && o->getNameInDocument() && inList.count(o))
            adjustLinkError(false,this,o);
    }
    return false;
}


//**************************************************************************
// PropertyLinkSub
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLinkSub, App::PropertyLinkBase)
TYPESYSTEM_SOURCE(App::PropertyLinkSubChild, App::PropertyLinkSub)
TYPESYSTEM_SOURCE(App::PropertyLinkSubGlobal, App::PropertyLinkSub)
TYPESYSTEM_SOURCE(App::PropertyLinkSubHidden, App::PropertyLinkSub)

//**************************************************************************
// Construction/Destruction


PropertyLinkSub::PropertyLinkSub()
:_pcLinkSub(0)
{

}


PropertyLinkSub::~PropertyLinkSub()
{
    //in case this property is dynamically removed
#ifndef USE_OLD_DAG
    if (_pcLinkSub && getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope!=LinkScope::Hidden) {
            if (_pcLinkSub)
                _pcLinkSub->_removeBackLink(parent);
        }
    }
#endif
}

void PropertyLinkSub::setValue(App::DocumentObject * lValue, 
        const std::vector<std::string> &SubList, std::vector<ShadowSub> &&shadows)
{
    setValue(lValue,std::vector<std::string>(SubList),std::move(shadows));
}

void PropertyLinkSub::setValue(App::DocumentObject * lValue, 
        std::vector<std::string> &&subs, std::vector<ShadowSub> &&shadows)
{
    auto parent = dynamic_cast<App::DocumentObject*>(getContainer());
    if(lValue) {
        if(!lValue->getNameInDocument())
            throw Base::ValueError("PropertyLinkSub: invalid document object");
        if(!testFlag(LinkAllowExternal) && parent && parent->getDocument()!=lValue->getDocument())
            throw Base::ValueError("PropertyLinkSub does not support external object");
    }
    aboutToSetValue();
#ifndef USE_OLD_DAG
    if(parent) {
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope!=LinkScope::Hidden) {
            if (_pcLinkSub)
                _pcLinkSub->_removeBackLink(parent);
            if (lValue)
                lValue->_addBackLink(parent);
        }
    }
#endif
    _pcLinkSub=lValue;
    _cSubList=std::move(subs);
    if(shadows.size()==_cSubList.size())
        _ShadowSubList = std::move(shadows);
    else
        updateElementReference(0);
    checkLabelReferences(_cSubList);
    hasSetValue();
}

App::DocumentObject * PropertyLinkSub::getValue(void) const
{
    return _pcLinkSub;
}

const std::vector<std::string>& PropertyLinkSub::getSubValues(void) const
{
    return _cSubList;
}

static inline const std::string &getSubNameWithStyle(const std::string &subName,
        const PropertyLinkBase::ShadowSub &shadow, bool newStyle)
{
    if(!newStyle) {
        if(shadow.second.size()) 
            return shadow.second;
    }else if(shadow.first.size()) 
        return shadow.first;
    return subName;
}

std::vector<std::string> PropertyLinkSub::getSubValues(bool newStyle) const {
    assert(_cSubList.size() == _ShadowSubList.size());
    std::vector<std::string> ret;
    ret.reserve(_cSubList.size());
    for(size_t i=0;i<_ShadowSubList.size();++i)
        ret.push_back(getSubNameWithStyle(_cSubList[i],_ShadowSubList[i],newStyle));
    return ret;
}

std::vector<std::string> PropertyLinkSub::getSubValuesStartsWith(const char* starter, bool newStyle) const
{
    assert(_cSubList.size() == _ShadowSubList.size());
    std::vector<std::string> ret;
    for(size_t i=0;i<_ShadowSubList.size();++i) {
        const auto &sub = getSubNameWithStyle(_cSubList[i],_ShadowSubList[i],newStyle);
        auto element = Data::ComplexGeoData::findElementName(sub.c_str());
        if(element && boost::starts_with(element,starter))
            ret.emplace_back(element);
    }
    return ret;
}

App::DocumentObject * PropertyLinkSub::getValue(Base::Type t) const
{
    return (_pcLinkSub && _pcLinkSub->getTypeId().isDerivedFrom(t)) ? _pcLinkSub : 0;
}

PyObject *PropertyLinkSub::getPyObject(void)
{
    Py::Tuple tup(2);
    Py::List list(static_cast<int>(_cSubList.size()));
    if (_pcLinkSub) {
        _pcLinkSub->getPyObject();
        tup[0] = Py::asObject(_pcLinkSub->getPyObject());
        for(unsigned int i = 0;i<_cSubList.size(); i++)
            list[i] = Py::String(_cSubList[i]);
        tup[1] = list;
        return Py::new_reference_to(tup);
    }
    else {
        return Py::new_reference_to(Py::None());
    }
}

void PropertyLinkSub::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(DocumentObjectPy::Type))) {
        DocumentObjectPy  *pcObject = (DocumentObjectPy*)value;
        setValue(pcObject->getDocumentObjectPtr());
    }
    else if (PyTuple_Check(value) || PyList_Check(value)) {
        Py::Sequence seq(value);
        if(seq.size() == 0)
            setValue(NULL);
        else if(seq.size()!=2) 
            throw Base::ValueError("Expect input sequence of size 2");
        else if (PyObject_TypeCheck(seq[0].ptr(), &(DocumentObjectPy::Type))) {
            DocumentObjectPy  *pcObj = (DocumentObjectPy*)seq[0].ptr();
            if (seq[1].isString()) {
                std::vector<std::string> vals;
                vals.push_back((std::string)Py::String(seq[1]));
                setValue(pcObj->getDocumentObjectPtr(),std::move(vals));
            }
            else if (seq[1].isSequence()) {
                Py::Sequence list(seq[1]);
                std::vector<std::string> vals(list.size());
                unsigned int i=0;
                for (Py::Sequence::iterator it = list.begin();it!=list.end();++it,++i)
                    vals[i] = Py::String(*it);
                setValue(pcObj->getDocumentObjectPtr(),std::move(vals));
            }
            else {
                std::string error = std::string("type of second element in tuple must be str or sequence of str");
                throw Base::TypeError(error);
            }
        }
        else {
            std::string error = std::string("type of first element in tuple must be 'DocumentObject', not ");
            error += seq[0].ptr()->ob_type->tp_name;
            throw Base::TypeError(error);
        }
    }
    else if(Py_None == value) {
        setValue(0);
    }
    else {
        std::string error = std::string("type must be 'DocumentObject', 'NoneType' or ('DocumentObject',['String',]) not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

static bool updateLinkReference(App::PropertyLinkBase *prop, 
        App::DocumentObject *feature, bool reverse, bool notify,
        App::DocumentObject *link, std::vector<std::string> &subs, std::vector<int> &mapped,
        std::vector<PropertyLinkBase::ShadowSub> &shadows)
{
    if(!feature) {
        shadows.clear();
        prop->unregisterElementReference();
    }
    shadows.resize(subs.size());
    if(!link || !link->getNameInDocument())
        return false;
    auto owner = dynamic_cast<DocumentObject*>(prop->getContainer());
    if(owner && owner->isRestoring())
        return false;
    int i=0;
    bool touched = false;
    for(auto &sub : subs) {
        if(prop->_updateElementReference(
                    feature,link,sub,shadows[i++],reverse,notify&&!touched))
            touched = true;
    }
    if(!touched)
        return false;
    for(int idx : mapped) {
        if(idx<(int)subs.size() && shadows[idx].first.size())
            subs[idx] = shadows[idx].first;
    }
    mapped.clear();
    if(owner && feature) 
        owner->onUpdateElementReference(prop);
    return true;
}

void PropertyLinkSub::afterRestore() {
    _ShadowSubList.resize(_cSubList.size());
    if(!testFlag(LinkRestoreLabel) ||!_pcLinkSub || !_pcLinkSub->getNameInDocument())
        return;
    setFlag(LinkRestoreLabel,false);
    for(std::size_t i=0;i<_cSubList.size();++i)
        restoreLabelReference(_pcLinkSub,_cSubList[i],&_ShadowSubList[i]);
}

void PropertyLinkSub::onContainerRestored() {
    unregisterElementReference();
    if(!_pcLinkSub || !_pcLinkSub->getNameInDocument())
        return;
    for(std::size_t i=0;i<_cSubList.size();++i)
        _registerElementReference(_pcLinkSub,_cSubList[i],_ShadowSubList[i]);
}

void PropertyLinkSub::updateElementReference(DocumentObject *feature, bool reverse, bool notify) {
    if(!updateLinkReference(this,feature,reverse,notify,_pcLinkSub,_cSubList,_mapped,_ShadowSubList))
        return;
    if(notify)
        hasSetValue();
}

bool PropertyLinkSub::referenceChanged() const {
    return !_mapped.empty();
}

std::string PropertyLinkBase::importSubName(Base::XMLReader &reader, const char *sub, bool &restoreLabel) {
    if(!reader.doNameMapping())
        return sub;
    std::ostringstream str;
    for(const char *dot=strchr(sub,'.');dot;sub=dot+1,dot=strchr(sub,'.')) {
        size_t count = dot-sub;
        const char *tail = ".";
        if(count && dot[-1] == '@') {
            // tail=='@' means we are exporting a label reference. So retain
            // this marker so that the label can be restored in afterRestore().
            tail = "@.";
            --count;
            restoreLabel = true;
        }
        str << reader.getName(std::string(sub,count).c_str()) << tail;
    }
    str << sub;
    return str.str();
}

std::string PropertyLinkBase::exportSubName(
        const App::DocumentObject *obj, const char *sub, bool check) 
{
    if(!obj)
        return (!check&&sub)?std::string(sub):std::string();
    std::ostringstream str;
    for(const char *dot=strchr(sub,'.');dot;sub=dot+1,dot=strchr(sub,'.')) {
        // name with trailing '.'
        auto name = std::string(sub,dot-sub+1);
        obj = obj->getSubObject(name.c_str());
        if(!obj || !obj->getNameInDocument()) {
            FC_ERR("missing sub object '" << name << "' in '" << sub <<"'");
            break;
        }
        if(obj->isExporting()) {
            if(name[0] == '$') {
                if(name.compare(1,name.size()-2,obj->Label.getValue())!=0) {
                    str << obj->getExportName(true) << "@.";
                    check = false;
                    continue;
                }
            } else if(name.compare(0,name.size()-1,obj->getNameInDocument())==0) {
                str << obj->getExportName(true) << '.';
                check = false;
                continue;
            }
        }
        str << name;
    }
    if(check)
        return std::string();
    str << sub;
    return str.str();
}

App::DocumentObject *PropertyLinkBase::tryImport(const App::Document *doc,
            const App::DocumentObject *obj, const std::map<std::string,std::string> &nameMap)
{
    if(doc && obj && obj->getNameInDocument())  {
        auto it = nameMap.find(obj->getExportName(true));
        if(it!=nameMap.end()) {
            obj = doc->getObject(it->second.c_str());
            if(!obj)
                FC_THROWM(Base::RuntimeError,"Cannot find import object " << it->second);
        }
    }
    return const_cast<DocumentObject*>(obj);
}

std::string PropertyLinkBase::tryImportSubName(const App::DocumentObject *obj, const char *_subname, 
        const App::Document *doc, const std::map<std::string,std::string> &nameMap)
{
    if(!doc || !obj || !obj->getNameInDocument()) 
        return std::string();

    std::ostringstream ss;
    std::string subname(_subname);
    char *sub = &subname[0];
    char *next = sub;
    for(char *dot=strchr(next,'.');dot;next=dot+1,dot=strchr(next,'.')) {
        StringGuard guard(dot);
        auto sobj = obj->getSubObject(subname.c_str());
        if(!sobj) {
            FC_ERR("Failed to restore lable reference " << obj->getFullName() << '.' << subname);
            return std::string();
        }
        dot[0] = 0;
        if(next[0] == '$') {
            if(strcmp(next+1,sobj->Label.getValue())!=0)
                continue;
        } else if(strcmp(next,sobj->getNameInDocument())!=0) {
            continue;
        }
        auto it = nameMap.find(sobj->getExportName(true));
        if(it == nameMap.end())
            continue;
        auto imported = doc->getObject(it->second.c_str());
        if(!imported) 
            FC_THROWM(RuntimeError, "Failed to find imported object " << it->second);
        ss.write(sub,next-sub);
        if(next[0] == '$')
            ss << '$' << imported->Label.getStrValue() << '.';
        else
            ss << it->second << '.';
        sub = dot+1;
    }
    if(sub!=subname.c_str())
        return ss.str();
    return std::string();
}

#define ATTR_SHADOWED "shadowed"
#define ATTR_SHADOW "shadow"
#define ATTR_MAPPED "mapped"

void PropertyLinkSub::Save (Base::Writer &writer) const
{
    assert(_cSubList.size() == _ShadowSubList.size());

    std::string internal_name;
    // it can happen that the object is still alive but is not part of the document anymore and thus
    // returns 0
    if (_pcLinkSub && _pcLinkSub->getNameInDocument())
        internal_name = _pcLinkSub->getExportName();
    writer.Stream() << writer.ind() << "<LinkSub value=\"" 
        <<  internal_name <<"\" count=\"" <<  _cSubList.size();
    writer.Stream() << "\">" << std::endl;
    writer.incInd();
    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    bool exporting = owner && owner->isExporting();
    for(unsigned int i = 0;i<_cSubList.size(); i++) {
        const auto &shadow = _ShadowSubList[i];
        // shadow.second stores the old style element name. For backward
        // compatibility reason, we shall store the old name into attribute
        // 'value' whenver possible.
        const auto &sub = shadow.second.empty()?_cSubList[i]:shadow.second;
        writer.Stream() << writer.ind() << "<Sub value=\"";
        if(exporting) {
            writer.Stream() << exportSubName(_pcLinkSub,sub.c_str());
            if(shadow.second.size() && shadow.first == _cSubList[i])
                writer.Stream() << "\" " ATTR_MAPPED "=\"1";
        } else {
            writer.Stream() << sub;
            if(_cSubList[i].size()) {
                if(sub!=_cSubList[i]) {
                    // Stores the actual value that is shadowed. For new version FC,
                    // we will restore this shadowed value instead.
                    writer.Stream() << "\" " ATTR_SHADOWED "=\"" << _cSubList[i];
                }else if(shadow.first.size()){
                    // Here means the user set value is old style element name.
                    // We shall then store the shadow somewhere else.
                    writer.Stream() << "\" " ATTR_SHADOW "=\"" << shadow.first;
                }
            }
        }
        writer.Stream()<<"\"/>" << endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</LinkSub>" << endl ;
}

void PropertyLinkSub::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("LinkSub");
    // get the values of my attributes
    std::string name = reader.getName(reader.getAttribute("value"));
    int count = reader.getAttributeAsInteger("count");

    // Property not in a DocumentObject!
    assert(getContainer()->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId()) );
    App::Document* document = static_cast<DocumentObject*>(getContainer())->getDocument();

    DocumentObject *pcObject = 0;
    if (!name.empty()) {
        pcObject = document ? document->getObject(name.c_str()) : 0;
        if (!pcObject) {
            if (reader.isVerbose()) {
                FC_WARN("Lost link to " << name 
                        << " while loading, maybe an object was not loaded correctly");
            }
        }
    }

    std::vector<int> mapped;
    std::vector<std::string> values(count);
    std::vector<ShadowSub> shadows(count);
    bool restoreLabel=false;
    // Sub may store '.' separated object names, so be aware of the possible mapping when import
    for (int i = 0; i < count; i++) {
        reader.readElement("Sub");
        shadows[i].second = importSubName(reader,reader.getAttribute("value"),restoreLabel);
        if(reader.hasAttribute(ATTR_SHADOWED)) {
            values[i] = shadows[i].first = 
                importSubName(reader,reader.getAttribute(ATTR_SHADOWED),restoreLabel);
        } else {
            values[i] = shadows[i].second;
            if(reader.hasAttribute(ATTR_SHADOW))
                shadows[i].first = importSubName(reader,reader.getAttribute(ATTR_SHADOW),restoreLabel);
        }
        if(reader.hasAttribute(ATTR_MAPPED))
            mapped.push_back(i);
    }
    setFlag(LinkRestoreLabel,restoreLabel);

    reader.readEndElement("LinkSub");

    if(pcObject) {
        setValue(pcObject,std::move(values),std::move(shadows));
        _mapped = std::move(mapped);
    }
    else {
       setValue(0);
    }
}

template<class Func, class... Args >
std::vector<std::string> updateLinkSubs(const App::DocumentObject *obj,
        const std::vector<std::string> &subs, Func *f, Args&&... args )
{
    if(!obj || !obj->getNameInDocument())
        return {};

    std::vector<std::string> res;
    for(auto it=subs.begin();it!=subs.end();++it) {
        const auto &sub = *it;
        auto new_sub = (*f)(obj,sub.c_str(),std::forward<Args>(args)...);
        if(new_sub.size()) {
            if(res.empty()) {
                res.reserve(subs.size());
                res.insert(res.end(),subs.begin(),it);
            }
            res.push_back(std::move(new_sub));
        }else if(res.size())
            res.push_back(sub);
    }
    return res;
}

Property *PropertyLinkSub::CopyOnImportExternal(
        const std::map<std::string,std::string> &nameMap) const
{
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if(!owner || !owner->getDocument())
        return 0;
    if(!_pcLinkSub || !_pcLinkSub->getNameInDocument())
        return 0;

    auto subs = updateLinkSubs(_pcLinkSub,_cSubList,
            &tryImportSubName,owner->getDocument(),nameMap);
    auto linked = tryImport(owner->getDocument(),_pcLinkSub,nameMap);
    if(subs.empty() && linked==_pcLinkSub) 
        return 0;

    PropertyLinkSub *p= new PropertyLinkSub();
    p->_pcLinkSub = linked;
    if(subs.empty())
        p->_cSubList = _cSubList;
    else
        p->_cSubList = std::move(subs);
    return p;
}

Property *PropertyLinkSub::CopyOnLabelChange(App::DocumentObject *obj, 
        const std::string &ref, const char *newLabel) const 
{
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if(!owner || !owner->getDocument())
        return 0;
    if(!_pcLinkSub || !_pcLinkSub->getNameInDocument())
        return 0;

    auto subs = updateLinkSubs(_pcLinkSub,_cSubList,&updateLabelReference,obj,ref,newLabel);
    if(subs.empty()) 
        return 0;

    PropertyLinkSub *p= new PropertyLinkSub();
    p->_pcLinkSub = _pcLinkSub;
    p->_cSubList = std::move(subs);
    return p;
}

Property *PropertyLinkSub::Copy(void) const
{
    PropertyLinkSub *p= new PropertyLinkSub();
    p->_pcLinkSub = _pcLinkSub;
    p->_cSubList = _cSubList;
    return p;
}

void PropertyLinkSub::Paste(const Property &from)
{
    if(!from.isDerivedFrom(PropertyLinkSub::getClassTypeId()))
        throw Base::TypeError("Incompatible property to paste to");
    auto &link = static_cast<const PropertyLinkSub&>(from);
    setValue(link._pcLinkSub, link._cSubList);
}

void PropertyLinkSub::getLinks(std::vector<App::DocumentObject *> &objs, 
        bool all, std::vector<std::string> *subs, bool newStyle) const
{
    if(all||_pcScope!=LinkScope::Hidden) {
        if(_pcLinkSub && _pcLinkSub->getNameInDocument()) {
            objs.push_back(_pcLinkSub);
            if(subs)
                *subs = getSubValues(newStyle);
        }
    }
}

void PropertyLinkSub::breakLink(App::DocumentObject *obj, bool clear) {
    if(obj == _pcLinkSub || (clear && getContainer()==obj))
        setValue(0);
}

static App::DocumentObject *adjustLinkSubs(App::PropertyLinkBase *prop,
        const std::set<App::DocumentObject*> &inList,
        App::DocumentObject *link, std::vector<std::string> &subs,
        std::map<App::DocumentObject *, std::vector<std::string> > *links=0)
{
    App::DocumentObject *newLink = 0;
    for(auto &sub : subs) {
        size_t pos = sub.find('.');
        for(;pos!=std::string::npos;pos=sub.find('.',pos+1)) {
            auto sobj = link->getSubObject(sub.substr(0,pos+1).c_str());
            if(!sobj || 
               (!prop->testFlag(PropertyLinkBase::LinkAllowExternal) && 
                sobj->getDocument()!=link->getDocument())) 
            {
                pos = std::string::npos;
                break;
            }
            if(!newLink) {
                if(inList.count(sobj))
                    continue;
                newLink = sobj;
                if(links)
                    (*links)[sobj].push_back(sub.substr(pos+1));
                else
                    sub = sub.substr(pos+1);
            }else if(links) 
                (*links)[sobj].push_back(sub.substr(pos+1));
            else if(sobj == newLink)
                sub = sub.substr(pos+1);
            break;
        }
        if(pos == std::string::npos)
            adjustLinkError(true,prop,link,sub.c_str());
    }
    return newLink;
}

bool PropertyLinkSub::adjustLink(const std::set<App::DocumentObject*> &inList) {
    if (_pcScope==LinkScope::Hidden)
        return false;
    if(!_pcLinkSub || !_pcLinkSub->getNameInDocument() || !inList.count(_pcLinkSub))
        return false;
    auto subs = _cSubList;
    auto link = adjustLinkSubs(this,inList,_pcLinkSub,subs);
    setValue(link,std::move(subs));
    return true;
}

//**************************************************************************
// PropertyLinkSubList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLinkSubList, App::PropertyLinkBase)
TYPESYSTEM_SOURCE(App::PropertyLinkSubListChild, App::PropertyLinkSubList)
TYPESYSTEM_SOURCE(App::PropertyLinkSubListGlobal, App::PropertyLinkSubList)
TYPESYSTEM_SOURCE(App::PropertyLinkSubListHidden, App::PropertyLinkSubList)

//**************************************************************************
// Construction/Destruction


PropertyLinkSubList::PropertyLinkSubList()
{

}

PropertyLinkSubList::~PropertyLinkSubList()
{
    //in case this property is dynamically removed
#ifndef USE_OLD_DAG
    //maintain backlinks
    if (!_lValueList.empty() && getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope!=LinkScope::Hidden) {
            for(auto *obj : _lValueList)
                obj->_removeBackLink(parent);
        }
    }
#endif
}

void PropertyLinkSubList::setSize(int newSize)
{
    _lValueList.resize(newSize);
    _lSubList  .resize(newSize);
    _ShadowSubList.resize(newSize);
}

int PropertyLinkSubList::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}


void PropertyLinkSubList::setValue(DocumentObject* lValue,const char* SubName)
{
    auto parent = dynamic_cast<App::DocumentObject*>(getContainer());
    if(lValue) {
        if(!lValue->getNameInDocument())
            throw Base::ValueError("PropertyLinkSubList: invalid document object");
        if(!testFlag(LinkAllowExternal) && parent && parent->getDocument()!=lValue->getDocument())
            throw Base::ValueError("PropertyLinkSubList does not support external object");
    }
#ifndef USE_OLD_DAG
    //maintain backlinks
    if(parent) {
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope!=LinkScope::Hidden) {
            for(auto *obj : _lValueList)
                obj->_removeBackLink(parent);
            if (lValue)
                lValue->_addBackLink(parent);
        }
    }
#endif
    
    if (lValue) {
        aboutToSetValue();
        _lValueList.resize(1);
        _lValueList[0]=lValue;
        _lSubList.resize(1);
        _lSubList[0]=SubName;
    }
    else {
        aboutToSetValue();
        _lValueList.clear();
        _lSubList.clear();
    }
    updateElementReference(0);
    checkLabelReferences(_lSubList);
    hasSetValue();
}

void PropertyLinkSubList::setValues(const std::vector<DocumentObject*>& lValue, const std::vector<const char*>& lSubNames)
{
    auto parent = dynamic_cast<App::DocumentObject*>(getContainer());
    for(auto obj : lValue) {
        if(!obj || !obj->getNameInDocument())
            throw Base::ValueError("PropertyLinkSubList: invalid document object");
        if(!testFlag(LinkAllowExternal) && parent && parent->getDocument()!=obj->getDocument())
            throw Base::ValueError("PropertyLinkSubList does not support external object");
    }
    if (lValue.size() != lSubNames.size())
        throw Base::ValueError("PropertyLinkSubList::setValues: size of subelements list != size of objects list");

#ifndef USE_OLD_DAG
    //maintain backlinks. 
    if(parent) {
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope!=LinkScope::Hidden) {
            //_lValueList can contain items multiple times, but we trust the document
            //object to ensure that this works
            for(auto *obj : _lValueList)
                obj->_removeBackLink(parent);

            //maintain backlinks. lValue can contain items multiple times, but we trust the document
            //object to ensure that the backlink is only added once
            for(auto *obj : lValue)
                obj->_addBackLink(parent);
        }
    }
#endif

    aboutToSetValue();
    _lValueList = lValue;
    _lSubList.resize(lSubNames.size());
    int i = 0;
    for (std::vector<const char*>::const_iterator it = lSubNames.begin();it!=lSubNames.end();++it,++i) {
        if (*it != nullptr)
            _lSubList[i] = *it;
    }
    updateElementReference(0);
    checkLabelReferences(_lSubList);
    hasSetValue();
}

void PropertyLinkSubList::setValues(const std::vector<DocumentObject*>& lValue,
        const std::vector<std::string>& lSubNames, std::vector<ShadowSub> &&ShadowSubList)
{
    setValues(std::vector<DocumentObject*>(lValue),
            std::vector<std::string>(lSubNames),std::move(ShadowSubList));
}

void PropertyLinkSubList::setValues(std::vector<DocumentObject*>&& lValue,
        std::vector<std::string>&& lSubNames, std::vector<ShadowSub> &&ShadowSubList)
{
    auto parent = dynamic_cast<App::DocumentObject*>(getContainer());
    for(auto obj : lValue) {
        if(!obj || !obj->getNameInDocument())
            throw Base::ValueError("PropertyLinkSubList: invalid document object");
        if(!testFlag(LinkAllowExternal) && parent && parent->getDocument()!=obj->getDocument())
            throw Base::ValueError("PropertyLinkSubList does not support external object");
    }
    if (lValue.size() != lSubNames.size())
        throw Base::ValueError("PropertyLinkSubList::setValues: size of subelements list != size of objects list");
    
#ifndef USE_OLD_DAG
    //maintain backlinks. 
    if(parent) {
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope!=LinkScope::Hidden) {
            //_lValueList can contain items multiple times, but we trust the document
            //object to ensure that this works
            for(auto *obj : _lValueList)
                obj->_removeBackLink(parent);

            //maintain backlinks. lValue can contain items multiple times, but we trust the document
            //object to ensure that the backlink is only added once
            for(auto *obj : lValue)
                obj->_addBackLink(parent);
        }
    }
#endif

    aboutToSetValue();
    _lValueList = std::move(lValue);
    _lSubList = std::move(lSubNames);
    if(ShadowSubList.size()==_lSubList.size())
        _ShadowSubList = std::move(ShadowSubList);
    else
        updateElementReference(0);
    checkLabelReferences(_lSubList);
    hasSetValue();
}

void PropertyLinkSubList::setValue(DocumentObject* lValue, const std::vector<string> &SubList)
{
    auto parent = dynamic_cast<App::DocumentObject*>(getContainer());
    if(lValue) {
        if(!lValue->getNameInDocument())
            throw Base::ValueError("PropertyLinkSubList: invalid document object");
        if(!testFlag(LinkAllowExternal) && parent && parent->getDocument()!=lValue->getDocument())
            throw Base::ValueError("PropertyLinkSubList does not support external object");
    }
#ifndef USE_OLD_DAG   
    //maintain backlinks.
    if(parent) {
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope!=LinkScope::Hidden) {
            //_lValueList can contain items multiple times, but we trust the document
            //object to ensure that this works
            for(auto *obj : _lValueList)
                obj->_removeBackLink(parent);

            //maintain backlinks. lValue can contain items multiple times, but we trust the document
            //object to ensure that the backlink is only added once
            if (lValue)
                lValue->_addBackLink(parent);
        }
    }
#endif

    aboutToSetValue();
    std::size_t size = SubList.size();
    this->_lValueList.clear();
    this->_lSubList.clear();
    if (size == 0) {
        if (lValue) {
            this->_lValueList.push_back(lValue);
            this->_lSubList.push_back(std::string());
        }
    }
    else {
        this->_lSubList = SubList;
        this->_lValueList.insert(this->_lValueList.begin(), size, lValue);
    }
    updateElementReference(0);
    checkLabelReferences(_lSubList);
    hasSetValue();
}

const string PropertyLinkSubList::getPyReprString() const
{
    assert(this->_lValueList.size() == this->_lSubList.size());

    if (this->_lValueList.size() == 0)
        return std::string("None");

    std::stringstream strm;
    strm << "[";
    for (std::size_t i = 0; i < this->_lSubList.size(); i++) {
        if (i>0)
            strm << ",(";
        else
            strm << "(";
        App::DocumentObject* obj = this->_lValueList[i];
        if (obj) {
            strm << "App.getDocument('" << obj->getDocument()->getName() 
                 << "').getObject('" << obj->getNameInDocument() << "')";
        } else {
            strm << "None";
        }
        strm << ",";
        strm << "'" << this->_lSubList[i] << "'";
        strm << ")";
    }
    strm << "]";
    return strm.str();
}

DocumentObject *PropertyLinkSubList::getValue() const
{
    App::DocumentObject* ret = 0;
    //FIXME: cache this to avoid iterating each time, to improve speed
    for (std::size_t i = 0; i < this->_lValueList.size(); i++) {
        if (ret == 0)
            ret = this->_lValueList[i];
        if (ret != this->_lValueList[i])
            return 0;
    }
    return ret;
}

int PropertyLinkSubList::removeValue(App::DocumentObject *lValue)
{
    assert(this->_lValueList.size() == this->_lSubList.size());

    std::size_t num = std::count(this->_lValueList.begin(), this->_lValueList.end(), lValue);
    if (num == 0)
        return 0;

    std::vector<DocumentObject*> links;
    std::vector<std::string> subs;
    links.reserve(this->_lValueList.size() - num);
    subs.reserve(this->_lSubList.size() - num);

    for (std::size_t i=0; i<this->_lValueList.size(); ++i) {
        if (this->_lValueList[i] != lValue) {
            links.push_back(this->_lValueList[i]);
            subs.push_back(this->_lSubList[i]);
        }
    }

    setValues(links, subs);
    return static_cast<int>(num);
}

void PropertyLinkSubList::setSubListValues(const std::vector<PropertyLinkSubList::SubSet>& values)
{
    std::vector<DocumentObject*> links;
    std::vector<std::string> subs;

    for (std::vector<PropertyLinkSubList::SubSet>::const_iterator it = values.begin(); it != values.end(); ++it) {
        for (std::vector<std::string>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
            links.push_back(it->first);
            subs.push_back(*jt);
        }
    }

    setValues(links, subs);
}

std::vector<PropertyLinkSubList::SubSet> PropertyLinkSubList::getSubListValues(bool newStyle) const
{
    std::vector<PropertyLinkSubList::SubSet> values;
    if (_lValueList.size() != _lSubList.size())
        throw Base::ValueError("PropertyLinkSubList::getSubListValues: size of subelements list != size of objects list");

    assert(_ShadowSubList.size() == _lSubList.size());

    for (std::size_t i = 0; i < _lValueList.size(); i++) {
        App::DocumentObject* link = _lValueList[i];
        std::string sub;
        if(newStyle && _ShadowSubList[i].first.size())
            sub = _ShadowSubList[i].first;
        else if(!newStyle && _ShadowSubList[i].second.size())
            sub = _ShadowSubList[i].second;
        else
            sub = _lSubList[i];
        if (values.size() == 0 || values.back().first != link){
            //new object started, start a new subset.
            values.push_back(SubSet(link, std::vector<std::string>()));
        }
        values.back().second.push_back(sub);
    }
    return values;
}

PyObject *PropertyLinkSubList::getPyObject(void)
{
#if 1
    std::vector<SubSet> subLists = getSubListValues();
    std::size_t count = subLists.size();
#if 0//FIXME: Should switch to tuple
    Py::Tuple sequence(count);
#else
    Py::List sequence(count);
#endif
    for (std::size_t i = 0; i<count; i++) {
        Py::Tuple tup(2);
        tup[0] = Py::asObject(subLists[i].first->getPyObject());

        const std::vector<std::string>& sub = subLists[i].second;
        Py::Tuple items(sub.size());
        for (std::size_t j = 0; j < sub.size(); j++) {
            items[j] = Py::String(sub[j]);
        }

        tup[1] = items;
        sequence[i] = tup;
    }

    return Py::new_reference_to(sequence);
#else
    unsigned int count = getSize();
#if 0//FIXME: Should switch to tuple
    Py::Tuple sequence(count);
#else
    Py::List sequence(count);
#endif
    for (unsigned int i = 0; i<count; i++) {
        Py::Tuple tup(2);
        tup[0] = Py::Object(_lValueList[i]->getPyObject());
        std::string subItem;
        if (_lSubList.size() > i)
            subItem = _lSubList[i];
        tup[1] = Py::String(subItem);
        sequence[i] = tup;
    }
    return Py::new_reference_to(sequence);
#endif
}

void PropertyLinkSubList::setPyObject(PyObject *value)
{
    try { //try PropertyLinkSub syntax
        PropertyLinkSub dummy;
        dummy.setPyObject(value);
        this->setValue(dummy.getValue(), dummy.getSubValues());
        return;
    }
    catch (...) {}
    try {
        // try PropertyLinkList syntax
        PropertyLinkList dummy;
        dummy.setPyObject(value);
        const auto &values = dummy.getValues();
        std::vector<std::string> subs(values.size());
        this->setValues(values,subs);
        return;
    }catch(...) {}

#define SUBLIST_THROW \
    throw Base::TypeError(\
        "Expects sequence of items of type DocObj, (DocObj,SubName), or (DocObj, (SubName,...))")

    if (!PyTuple_Check(value) && !PyList_Check(value))
        SUBLIST_THROW;

    Py::Sequence list(value);
    Py::Sequence::size_type size = list.size();

    std::vector<DocumentObject*> values;
    values.reserve(size);
    std::vector<std::string>     SubNames;
    SubNames.reserve(size);
    for (Py::Sequence::size_type i=0; i<size; i++) {
        Py::Object item = list[i];
        if ((item.isTuple() || item.isSequence()) && PySequence_Size(*item)==2) {
            Py::Sequence seq(item);
            if (PyObject_TypeCheck(seq[0].ptr(), &(DocumentObjectPy::Type))){
                auto obj = static_cast<DocumentObjectPy*>(seq[0].ptr())->getDocumentObjectPtr();
                if (seq[1].isString()) {
                    values.push_back(obj);
                    SubNames.push_back(Py::String(seq[1]));
                } else if (seq[1].isSequence()) {
                    Py::Sequence list(seq[1]);
                    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                        values.push_back(obj);
                        SubNames.push_back(Py::String(*it));
                    }
                } else
                    SUBLIST_THROW;
            }
        } else if (PyObject_TypeCheck(*item, &(DocumentObjectPy::Type))) {
            DocumentObjectPy *pcObj;
            pcObj = static_cast<DocumentObjectPy*>(*item);
            values.push_back(pcObj->getDocumentObjectPtr());
            SubNames.emplace_back();
        } else
            SUBLIST_THROW;
    }
    setValues(values,SubNames);
}

void PropertyLinkSubList::afterRestore() {
    assert(_lSubList.size() == _ShadowSubList.size());
    if(!testFlag(LinkRestoreLabel))
        return;
    setFlag(LinkRestoreLabel,false);
    for(size_t i=0;i<_lSubList.size();++i)
        restoreLabelReference(_lValueList[i],_lSubList[i],&_ShadowSubList[i]);
}

void PropertyLinkSubList::onContainerRestored() {
    unregisterElementReference();
    for(size_t i=0;i<_lSubList.size();++i)
        _registerElementReference(_lValueList[i],_lSubList[i],_ShadowSubList[i]);
}

void PropertyLinkSubList::updateElementReference(DocumentObject *feature, bool reverse, bool notify) {
    if(!feature) {
        _ShadowSubList.clear();
        unregisterElementReference();
    }
    _ShadowSubList.resize(_lSubList.size());
    auto owner = freecad_dynamic_cast<DocumentObject>(getContainer());
    if(owner && owner->isRestoring())
        return;
    int i=0;
    bool touched = false;
    for(auto &sub : _lSubList) {
        auto obj = _lValueList[i];
        if(_updateElementReference(feature,obj,sub,_ShadowSubList[i++],reverse,notify&&!touched))
            touched = true;
    }
    if(!touched) 
        return;

    std::vector<int> mapped;
    mapped.reserve(_mapped.size());
    for(int idx : _mapped) {
        if(idx<(int)_lSubList.size()) {
            if(_ShadowSubList[idx].first.size())
                _lSubList[idx] = _ShadowSubList[idx].first;
            else
                mapped.push_back(idx);
        }
    }
    _mapped.swap(mapped);
    if(owner && feature) 
        owner->onUpdateElementReference(this);
    if(notify)
        hasSetValue();
}

bool PropertyLinkSubList::referenceChanged() const{
    return !_mapped.empty();
}

void PropertyLinkSubList::Save (Base::Writer &writer) const
{
    assert(_lSubList.size() == _ShadowSubList.size());

    int count = 0;
    for(auto obj : _lValueList) {
        if(obj && obj->getNameInDocument())
            ++count;
    }
    writer.Stream() << writer.ind() << "<LinkSubList count=\"" << count <<"\">" << endl;
    writer.incInd();
    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    bool exporting = owner && owner->isExporting();
    for (int i = 0; i < getSize(); i++) {
        auto obj = _lValueList[i];
        if(!obj || !obj->getNameInDocument())
            continue;
        const auto &shadow = _ShadowSubList[i];
        // shadow.second stores the old style element name. For backward
        // compatibility reason, we shall store the old name into attribute
        // 'value' whenver possible.
        const auto &sub = shadow.second.empty()?_lSubList[i]:shadow.second;

        writer.Stream() << writer.ind() << "<Link obj=\"" << obj->getExportName() << "\" sub=\"";
        if(exporting) {
            writer.Stream() << exportSubName(obj,sub.c_str());
            if(shadow.second.size() && _lSubList[i]==shadow.first)
                writer.Stream() << "\" " ATTR_MAPPED "=\"1";
        } else {
            writer.Stream() << sub;
            if(_lSubList[i].size()) {
                if(sub!=_lSubList[i]) {
                    // Stores the actual value that is shadowed. For new version FC,
                    // we will restore this shadowed value instead.
                    writer.Stream() << "\" " ATTR_SHADOWED "=\"" << _lSubList[i];
                }else if(shadow.first.size()) {
                    // Here means the user set value is old style element name.
                    // We shall then store the shadow somewhere else.
                    writer.Stream() << "\" " ATTR_SHADOW "=\"" << shadow.first;
                }
            }
        }
        writer.Stream() << "\"/>" << endl;
    }

    writer.decInd();
    writer.Stream() << writer.ind() << "</LinkSubList>" << endl ;
}

void PropertyLinkSubList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("LinkSubList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");

    std::vector<DocumentObject*> values;
    values.reserve(count);
    std::vector<std::string> SubNames;
    SubNames.reserve(count);
    std::vector<ShadowSub> shadows;
    shadows.reserve(count);
    DocumentObject* father = dynamic_cast<DocumentObject*>(getContainer());
    App::Document* document = father ? father->getDocument() : 0;
    std::vector<int> mapped;
    bool restoreLabel=false;
    for (int i = 0; i < count; i++) {
        reader.readElement("Link");
        std::string name = reader.getName(reader.getAttribute("obj"));
        // In order to do copy/paste it must be allowed to have defined some
        // referenced objects in XML which do not exist anymore in the new
        // document. Thus, we should silently ignore this.
        // Property not in an object!
        DocumentObject* child = document ? document->getObject(name.c_str()) : 0;
        if (child) {
            values.push_back(child);
            shadows.emplace_back();
            auto &shadow = shadows.back();
            shadow.second = importSubName(reader,reader.getAttribute("sub"),restoreLabel);
            if(reader.hasAttribute(ATTR_SHADOWED)) {
                shadow.first = importSubName(reader,reader.getAttribute(ATTR_SHADOWED),restoreLabel);
                SubNames.push_back(shadow.first);
            }else{
                SubNames.push_back(shadow.second);
                if(reader.hasAttribute(ATTR_SHADOW)) 
                    shadow.first = importSubName(reader,reader.getAttribute(ATTR_SHADOW),restoreLabel);
            }
            if(reader.hasAttribute(ATTR_MAPPED))
                mapped.push_back(i);
        } else if (reader.isVerbose())
            Base::Console().Warning("Lost link to '%s' while loading, maybe "
                                    "an object was not loaded correctly\n",name.c_str());
    }
    setFlag(LinkRestoreLabel,restoreLabel);

    reader.readEndElement("LinkSubList");

    // assignment
    setValues(values,std::move(SubNames),std::move(shadows));
    _mapped.swap(mapped);
}

Property *PropertyLinkSubList::CopyOnImportExternal(
        const std::map<std::string,std::string> &nameMap) const
{
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if(!owner || !owner->getDocument() || _lValueList.size()!=_lSubList.size())
        return 0;
    std::vector<App::DocumentObject *> values;
    std::vector<std::string> subs;
    auto itSub = _lSubList.begin();
    for(auto itValue=_lValueList.begin();itValue!=_lValueList.end();++itValue,++itSub) {
        auto value = *itValue;
        const auto &sub = *itSub;
        if(!value || !value->getNameInDocument()) {
            if(values.size()) {
                values.push_back(0);
                subs.emplace_back();
            }
            continue;
        }
        auto linked = tryImport(owner->getDocument(),value,nameMap);
        auto new_sub = tryImportSubName(value,sub.c_str(),owner->getDocument(),nameMap);
        if(linked!=value || new_sub.size()) {
            if(values.empty()) {
                values.reserve(_lValueList.size());
                values.insert(values.end(),_lValueList.begin(),itValue);
                subs.reserve(_lSubList.size());
                subs.insert(subs.end(),_lSubList.begin(),itSub);
            }
            values.push_back(linked);
            subs.push_back(std::move(new_sub));
        }else if(values.size()) {
            values.push_back(linked);
            subs.push_back(sub);
        }
    }
    if(values.empty()) 
        return 0;
    std::unique_ptr<PropertyLinkSubList> p(new PropertyLinkSubList);
    p->_lValueList = std::move(values);
    p->_lSubList = std::move(subs);
    return p.release();
}

Property *PropertyLinkSubList::CopyOnLabelChange(App::DocumentObject *obj, 
        const std::string &ref, const char *newLabel) const 
{
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if(!owner || !owner->getDocument())
        return 0;
    std::vector<App::DocumentObject *> values;
    std::vector<std::string> subs;
    auto itSub = _lSubList.begin();
    for(auto itValue=_lValueList.begin();itValue!=_lValueList.end();++itValue,++itSub) {
        auto value = *itValue;
        const auto &sub = *itSub;
        if(!value || !value->getNameInDocument()) {
            if(values.size()) {
                values.push_back(0);
                subs.emplace_back();
            }
            continue;
        }
        auto new_sub = updateLabelReference(value,sub.c_str(),obj,ref,newLabel);
        if(new_sub.size()) {
            if(values.empty()) {
                values.reserve(_lValueList.size());
                values.insert(values.end(),_lValueList.begin(),itValue);
                subs.reserve(_lSubList.size());
                subs.insert(subs.end(),_lSubList.begin(),itSub);
            }
            values.push_back(value);
            subs.push_back(std::move(new_sub));
        }else if(values.size()) {
            values.push_back(value);
            subs.push_back(sub);
        }
    }
    if(values.empty()) 
        return 0;
    std::unique_ptr<PropertyLinkSubList> p(new PropertyLinkSubList);
    p->_lValueList = std::move(values);
    p->_lSubList = std::move(subs);
    return p.release();
}

Property *PropertyLinkSubList::Copy(void) const
{
    PropertyLinkSubList *p = new PropertyLinkSubList();
    p->_lValueList = _lValueList;
    p->_lSubList   = _lSubList;
    return p;
}

void PropertyLinkSubList::Paste(const Property &from)
{
    if(!from.isDerivedFrom(PropertyLinkSubList::getClassTypeId()))
        throw Base::TypeError("Incompatible property to paste to");
    auto &link = static_cast<const PropertyLinkSubList&>(from);
    setValues(link._lValueList, link._lSubList);
}

unsigned int PropertyLinkSubList::getMemSize (void) const
{
   unsigned int size = static_cast<unsigned int>(_lValueList.size() * sizeof(App::DocumentObject *));
   for(int i = 0;i<getSize(); i++)
       size += _lSubList[i].size();
   return size;
}

std::vector<std::string> PropertyLinkSubList::getSubValues(bool newStyle) const {
    assert(_lSubList.size() == _ShadowSubList.size());
    std::vector<std::string> ret;
    ret.reserve(_ShadowSubList.size());
    for(size_t i=0;i<_ShadowSubList.size();++i)
        ret.push_back(getSubNameWithStyle(_lSubList[i],_ShadowSubList[i],newStyle));
    return ret;
}

void PropertyLinkSubList::getLinks(std::vector<App::DocumentObject *> &objs, 
        bool all, std::vector<std::string> *subs, bool newStyle) const
{
    if(all||_pcScope!=LinkScope::Hidden) {
        objs.reserve(objs.size()+_lValueList.size());
        for(auto obj : _lValueList) {
            if(obj && obj->getNameInDocument())
                objs.push_back(obj);
        }
        if(subs) {
            auto _subs = getSubValues(newStyle);
            subs->reserve(subs->size()+_subs.size());
            std::move(_subs.begin(),_subs.end(),std::back_inserter(*subs));
        }
    }
}

void PropertyLinkSubList::breakLink(App::DocumentObject *obj, bool clear) {
    std::vector<DocumentObject*> values;
    std::vector<std::string> subs;

    if(clear && getContainer()==obj) {
        setValues(values,subs);
        return;
    }
    assert(_lValueList.size()==_lSubList.size());

    values.reserve(_lValueList.size());
    subs.reserve(_lSubList.size());

    int i=-1;
    for(auto o : _lValueList) {
        ++i;
        if(o==obj) 
            continue;
        values.push_back(o);
        subs.push_back(_lSubList[i]);
    }
    if(values.size()!=_lValueList.size())
        setValues(values,subs);
}

bool PropertyLinkSubList::adjustLink(const std::set<App::DocumentObject*> &inList) {
    if (_pcScope==LinkScope::Hidden)
        return false;
    auto subs = _lSubList;
    auto links = _lValueList;
    int idx = -1;
    bool touched = false;
    for(std::string &sub : subs) {
        ++idx;
        auto &link = links[idx];
        if(!link || !link->getNameInDocument() || !inList.count(link))
            continue;
        touched = true;
        size_t pos = sub.find('.');
        for(;pos!=std::string::npos;pos=sub.find('.',pos+1)) {
            auto sobj = link->getSubObject(sub.substr(0,pos+1).c_str());
            if(!sobj || sobj->getDocument()!=link->getDocument()) {
                pos = std::string::npos;
                break;
            }
            if(!inList.count(sobj)) {
                link = sobj;
                sub = sub.substr(pos+1);
                break;
            }
        }
        if(pos == std::string::npos)
            adjustLinkError(true,this,link,sub.c_str());
    }
    if(touched)
        setValues(links,subs);
    return touched;
}

//**************************************************************************
// DocInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Key on aboslute path. 
// Becuase of possible symbolic links, multiple entry may refer to the same
// file. We use QFileInfo::canonicalPath to resolve that.
typedef std::map<QString,DocInfoPtr> DocInfoMap;
DocInfoMap _DocInfoMap;

class App::DocInfo : 
    public std::enable_shared_from_this<App::DocInfo> 
{
public:
    typedef boost::signals2::scoped_connection Connection;
    Connection connFinishRestoreDocument;
    Connection connDeleteDocument;
    Connection connSaveDocument;
    Connection connDeletedObject;

    DocInfoMap::iterator myPos;
    std::string myPath;
    App::Document *pcDoc;
    std::set<PropertyXLink*> links;

    static std::string getDocPath(
            const char *filename, App::Document *pDoc, bool relative, QString *fullPath = 0) 
    {
        bool absolute;
        // make sure the filename is aboluste path
        QString path = QDir::cleanPath(QString::fromUtf8(filename));
        if((absolute=QFileInfo(path).isAbsolute())) {
            if(fullPath)
                *fullPath = path;
            if(!relative)
                return std::string(path.toUtf8().constData());
        }

        const char *docPath = pDoc->FileName.getValue();
        if(!docPath || *docPath==0)
            throw Base::RuntimeError("Owner document not saved");
        
        QDir docDir(QFileInfo(QString::fromUtf8(docPath)).absoluteDir());
        if(!absolute) {
            path = QDir::cleanPath(docDir.absoluteFilePath(path));
            if(fullPath)
                *fullPath = path;
        }

        if(relative)
            return std::string(docDir.relativeFilePath(path).toUtf8().constData());
        else
            return std::string(path.toUtf8().constData());
    }

    static DocInfoPtr get(const char *filename, 
            App::Document *pDoc,PropertyXLink *l, const char *objName) 
    {
        QString path;
        l->filePath = getDocPath(filename,pDoc,true,&path);

        FC_LOG("finding doc " << filename);

        auto it = _DocInfoMap.find(path);
        DocInfoPtr info;
        if(it != _DocInfoMap.end()) {
            info = it->second;
            if(!info->pcDoc) {
                QString fullpath(info->getFullPath());
                if(fullpath.size() &&  
                   App::GetApplication().addPendingDocument(
                       fullpath.toUtf8().constData(),objName,
                       l->testFlag(PropertyLinkBase::LinkAllowPartial))==0) 
                {
                    for(App::Document *doc : App::GetApplication().getDocuments()) {
                        if(getFullPath(doc->FileName.getValue()) == fullpath) {
                            info->attach(doc);
                            break;
                        }
                    }
                }
            }
        } else {
            info = std::make_shared<DocInfo>();
            auto ret = _DocInfoMap.insert(std::make_pair(path,info));
            info->init(ret.first,objName,l);
        }
        info->links.insert(l);
        return info;
    }

    static QString getFullPath(const char *p) {
        if(!p) return QString();
        return QFileInfo(QString::fromUtf8(p)).canonicalFilePath();
    }

    QString getFullPath() const {
        return QFileInfo(myPos->first).canonicalFilePath();
    }

    const char *filePath() const {
        return myPath.c_str();
    }

    DocInfo()
        :pcDoc(0)
    {}

    ~DocInfo() {
    }

    void deinit() {
        FC_LOG("deinit " << (pcDoc?pcDoc->getName():filePath()));
        assert(links.empty());
        connFinishRestoreDocument.disconnect();
        connDeleteDocument.disconnect();
        connSaveDocument.disconnect();
        connDeletedObject.disconnect();

        auto me = shared_from_this();
        _DocInfoMap.erase(myPos);
        myPos = _DocInfoMap.end();
        myPath.clear();
        pcDoc = 0;
    }

    void init(DocInfoMap::iterator pos, const char *objName, PropertyXLink *l) {
        myPos = pos;
        myPath = myPos->first.toUtf8().constData();
        App::Application &app = App::GetApplication();
        connFinishRestoreDocument = app.signalFinishRestoreDocument.connect(
            boost::bind(&DocInfo::slotFinishRestoreDocument,this,_1));
        connDeleteDocument = app.signalDeleteDocument.connect(
            boost::bind(&DocInfo::slotDeleteDocument,this,_1));
        connSaveDocument = app.signalSaveDocument.connect(
            boost::bind(&DocInfo::slotSaveDocument,this,_1));

        QString fullpath(getFullPath());
        if(fullpath.isEmpty())
            FC_ERR("document not found " << filePath());
        else{
            for(App::Document *doc : App::GetApplication().getDocuments()) {
                if(getFullPath(doc->FileName.getValue()) == fullpath) {
                    attach(doc);
                    return;
                }
            }
            FC_LOG("document pending " << filePath());
            app.addPendingDocument(fullpath.toUtf8().constData(),objName,
                    l->testFlag(PropertyLinkBase::LinkAllowPartial));
        }
    }

    void attach(Document *doc) {
        assert(!pcDoc);
        pcDoc = doc;
        FC_LOG("attaching " << doc->getName() << ", " << doc->FileName.getValue());
        std::map<App::PropertyLinkBase*,std::vector<App::PropertyXLink*> > parentLinks;
        for(auto it=links.begin(),itNext=it;it!=links.end();it=itNext) {
            ++itNext;
            auto link = *it;
            if(link->_pcLink)
                continue;
            if(link->parentProp) {
                parentLinks[link->parentProp].push_back(link);
                continue;
            }
            auto obj = doc->getObject(link->objectName.c_str());
            if(!obj) 
                FC_WARN("object '" << link->objectName << "' not found in document '" 
                        << doc->getName() << "'");
            else
                link->restoreLink(obj);
        }
        for(auto &v : parentLinks) {
            v.first->setFlag(PropertyLinkBase::LinkRestoring);
            v.first->aboutToSetValue();
            for(auto link : v.second) {
                auto obj = doc->getObject(link->objectName.c_str());
                if(!obj) 
                    FC_WARN("object '" << link->objectName << "' not found in document '" 
                            << doc->getName() << "'");
                else
                    link->restoreLink(obj);
            }
            v.first->hasSetValue();
            v.first->setFlag(PropertyLinkBase::LinkRestoring,false);
        }
    }

    void remove(PropertyXLink *l) {
        auto it = links.find(l);
        if(it != links.end()) {
            links.erase(it);
            if(links.empty())
                deinit();
        }
    }

    void slotFinishRestoreDocument(const App::Document &doc) {
        if(pcDoc) return;
        QString fullpath(getFullPath());
        if(!fullpath.isEmpty() && getFullPath(doc.FileName.getValue())==fullpath)
            attach(const_cast<App::Document*>(&doc));
    }

    void slotSaveDocument(const App::Document &doc) {
        if(!pcDoc) {
            slotFinishRestoreDocument(doc);
            return;
        }
        if(&doc!=pcDoc) return;

        QFileInfo info(myPos->first);
        QString path(info.canonicalFilePath());
        const char *filename = doc.FileName.getValue();
        QString docPath(getFullPath(filename));

        if(path.isEmpty() || path!=docPath) {
            FC_LOG("document '" << doc.getName() << "' path changed");
            auto me = shared_from_this();
            auto ret = _DocInfoMap.insert(std::make_pair(path,me));
            if(!ret.second) {
                // is that even possible?
                FC_WARN("document '" << doc.getName() << "' path exists, detach");
                slotDeleteDocument(doc);
                return;
            }
            _DocInfoMap.erase(myPos);
            myPos = ret.first;

            std::set<PropertyXLink *> tmp;
            tmp.swap(links);
            for(auto link : tmp) {
                auto owner = static_cast<DocumentObject*>(link->getContainer());
                QString path = QString::fromUtf8(link->filePath.c_str());
                // adjust file path for each PropertyXLink
                DocInfo::get(filename,owner->getDocument(),link,link->objectName.c_str());
            }
        }

        // time stamp changed, touch the linking document. Unfortunately, there
        // is no way to setModfied() for an App::Document. We don't want to touch
        // all PropertyXLink for a document, because the linked object is
        // potentially unchanged. So we just touch at most one.
        std::set<Document*> docs;
        for(auto link : links) {
            auto doc = static_cast<DocumentObject*>(link->getContainer())->getDocument();
            auto ret = docs.insert(doc);
            if(ret.second && !doc->isTouched())
                link->touch();
        }
    }

    void slotDeleteDocument(const App::Document &doc) {
        for(auto it=links.begin(),itNext=it;it!=links.end();it=itNext) {
            ++itNext;
            auto link = *it;
            auto obj = dynamic_cast<DocumentObject*>(link->getContainer());
            if(obj && obj->getDocument() == &doc) {
                links.erase(it);
                // must call unlink here, so that PropertyLink::resetLink can
                // remove back link before the owner object is marked as being
                // destroyed
                link->unlink();
            }
        }
        if(links.empty()) {
            deinit();
            return;
        }
        if(pcDoc!=&doc) return;
        std::map<App::PropertyLinkBase*,std::vector<App::PropertyXLink*> > parentLinks;
        for(auto link : links) {
            link->setFlag(PropertyLinkBase::LinkDetached);
            if(link->parentProp)
                parentLinks[link->parentProp].push_back(link);
            else
                parentLinks[0].push_back(link);
        }
        for(auto &v : parentLinks) {
            if(v.first) {
                v.first->setFlag(PropertyLinkBase::LinkDetached);
                v.first->aboutToSetValue();
            }
            for(auto l : v.second)
                l->detach();
            if(v.first) {
                v.first->hasSetValue();
                v.first->setFlag(PropertyLinkBase::LinkDetached,false);
            }
        }
        pcDoc = 0;
    }

    bool hasXLink(const App::Document *doc) const{
        for(auto link : links) {
            auto obj = dynamic_cast<DocumentObject*>(link->getContainer());
            if(obj && obj->getDocument() == doc)
                return true;
        }
        return false;
    }

    static void breakLinks(App::DocumentObject *obj, bool clear) {
        auto doc = obj->getDocument();
        for(auto itD=_DocInfoMap.begin(),itDNext=itD;itD!=_DocInfoMap.end();itD=itDNext) {
            ++itDNext;
            auto docInfo = itD->second;
            if(docInfo->pcDoc != doc)
                continue;
            auto &links = docInfo->links;
            std::set<PropertyLinkBase*> parentLinks;
            for(auto it=links.begin(),itNext=it;it!=links.end();it=itNext) {
                ++itNext;
                auto link = *it;
                if(link->_pcLink!=obj && !(clear && link->getContainer()==obj))
                    continue;
                if(link->parentProp)
                    parentLinks.insert(link->parentProp);
                else
                    link->breakLink(obj,clear);
            }
            for(auto link : parentLinks) 
                link->breakLink(obj,clear);
        }
    }
};

void PropertyLinkBase::breakLinks(App::DocumentObject *link, 
        const std::vector<App::DocumentObject*> &objs, bool clear) 
{
    std::vector<Property*> props;
    for(auto obj : objs) {
        props.clear();
        obj->getPropertyList(props);
        for(auto prop : props) {
            auto linkProp = dynamic_cast<PropertyLinkBase*>(prop);
            if(linkProp)
                linkProp->breakLink(link,clear);
        }
    }
    DocInfo::breakLinks(link,clear);
}

//**************************************************************************
// PropertyXLink
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyXLink , App::PropertyLink)

PropertyXLink::PropertyXLink(bool _allowPartial, PropertyLinkBase *parent)
    :parentProp(parent)
{
    setAllowPartial(_allowPartial);
    setAllowExternal(true);
    if(parent)
        setContainer(parent->getContainer());
}

PropertyXLink::~PropertyXLink() {
    unlink();
}

void PropertyXLink::unlink() {
    if(docInfo) {
        docInfo->remove(this);
        docInfo.reset();
    }
    objectName.clear();
    resetLink();
}

void PropertyXLink::detach() {
    if(docInfo && _pcLink) {
        aboutToSetValue();
        resetLink();
        updateElementReference(0);
        hasSetValue();
    }
}

void PropertyXLink::aboutToSetValue() {
    if(parentProp)
        parentProp->aboutToSetChildValue(*this);
    else
        PropertyLinkBase::aboutToSetValue();
}

void PropertyXLink::hasSetValue() {
    if(parentProp)
        parentProp->hasSetChildValue(*this);
    else
        PropertyLinkBase::hasSetValue();
}

void PropertyXLink::setSubName(const char *subname) 
{
    std::vector<std::string> subs;
    if(subname && subname[0])
        subs.emplace_back(subname);
    aboutToSetValue();
    _setSubValues(std::move(subs));
    hasSetValue();
}

void PropertyXLink::_setSubValues(std::vector<std::string> &&subs, 
        std::vector<ShadowSub> &&shadows)
{
    _SubList = std::move(subs);
    _ShadowSubList.clear();
    if(shadows.size() == _SubList.size()) 
        _ShadowSubList = std::move(shadows);
    else
        updateElementReference(0);
    checkLabelReferences(_SubList);
}

void PropertyXLink::setValue(App::DocumentObject * lValue) {
    setValue(lValue,0);
}

void PropertyXLink::setValue(App::DocumentObject * lValue, const char *subname)
{
    std::vector<std::string> subs;
    if(subname && subname[0])
        subs.emplace_back(subname);
    _setValue(lValue,std::move(subs));
}

void PropertyXLink::restoreLink(App::DocumentObject *lValue) {
    assert(!_pcLink && lValue && docInfo);

    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    if(!owner || !owner->getNameInDocument()) 
        throw Base::RuntimeError("invalid container");

    bool touched = owner->isTouched();
    setFlag(LinkDetached,false);
    setFlag(LinkRestoring);
    aboutToSetValue();
#ifndef USE_OLD_DAG
    if (!owner->testStatus(ObjectStatus::Destroy) && _pcScope!=LinkScope::Hidden)
        lValue->_addBackLink(owner);
#endif
    _pcLink=lValue;
    updateElementReference(0);
    hasSetValue();
    setFlag(LinkRestoring,false);

    if(!touched && 
        owner->isTouched() && 
        docInfo && 
        docInfo->pcDoc && 
        stamp==docInfo->pcDoc->LastModifiedDate.getValue())
    {
        owner->purgeTouched();
    }
}

void PropertyXLink::_setValue(App::DocumentObject *lValue, 
        std::vector<std::string> &&subs, std::vector<ShadowSub> &&shadows)
{
    if(_pcLink==lValue && _SubList==subs)
        return;

    if(lValue && (!lValue->getNameInDocument() || !lValue->getDocument())) {
        throw Base::ValueError("Invalid object");
        return;
    }

    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    if(!owner || !owner->getNameInDocument()) 
        throw Base::RuntimeError("invalid container");

    if(lValue == owner)
        throw Base::ValueError("self linking");

    DocInfoPtr info;
    const char *name = "";
    if(lValue) {
        name = lValue->getNameInDocument();
        if(lValue->getDocument() != owner->getDocument()) {
            if(!docInfo || lValue->getDocument()!=docInfo->pcDoc)
            {
                const char *filename = lValue->getDocument()->FileName.getValue();
                if(!filename || *filename==0) 
                    throw Base::RuntimeError("Linked document not saved");
                FC_LOG("xlink set to new document " << lValue->getDocument()->getName());
                info = DocInfo::get(filename,owner->getDocument(),this,name);
                assert(info && info->pcDoc == lValue->getDocument());
            }else
                info = docInfo;
        }
    }

    setFlag(LinkDetached,false);
    aboutToSetValue();
#ifndef USE_OLD_DAG
    if (!owner->testStatus(ObjectStatus::Destroy) && _pcScope!=LinkScope::Hidden) {
        if(_pcLink)
            _pcLink->_removeBackLink(owner);
        if(lValue)
            lValue->_addBackLink(owner);
    }
#endif
    if(docInfo!=info) {
        unlink();
        docInfo = info;
    }
    _pcLink=lValue;
    if(docInfo && docInfo->pcDoc)
        stamp=docInfo->pcDoc->LastModifiedDate.getValue();
    objectName = std::move(name);
    _setSubValues(std::move(subs),std::move(shadows));
    hasSetValue();
}

void PropertyXLink::_setValue(std::string &&filename, std::string &&name, 
        std::vector<std::string> &&subs, std::vector<ShadowSub> &&shadows)
{
    if(name.empty()) {
        _setValue(0,std::move(subs),std::move(shadows));
        return;
    }
    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    if(!owner || !owner->getNameInDocument()) 
        throw Base::RuntimeError("invalid container");

    DocumentObject *pObject=0;
    DocInfoPtr info;
    if(filename.size()) {
        info = DocInfo::get(filename.c_str(),owner->getDocument(),this,name.c_str());
        if(info->pcDoc) 
            pObject = info->pcDoc->getObject(name.c_str());
    }else
        pObject = owner->getDocument()->getObject(name.c_str());

    if(pObject || !info) {
        _setValue(pObject,std::move(subs),std::move(shadows));
        return;
    }
    setFlag(LinkDetached,false);
    aboutToSetValue();
#ifndef USE_OLD_DAG
    if (_pcLink && !owner->testStatus(ObjectStatus::Destroy) && _pcScope!=LinkScope::Hidden) 
        _pcLink->_removeBackLink(owner);
#endif
    _pcLink = 0;
    if(docInfo!=info) {
        unlink();
        docInfo = info;
    }
    if(docInfo && docInfo->pcDoc)
        stamp=docInfo->pcDoc->LastModifiedDate.getValue();
    objectName = std::move(name);
    _setSubValues(std::move(subs),std::move(shadows));
    hasSetValue();
}

App::Document *PropertyXLink::getDocument() const {
    return docInfo?docInfo->pcDoc:0;
}

const char *PropertyXLink::getDocumentPath() const {
    return docInfo?docInfo->filePath():0;
}

const char *PropertyXLink::getObjectName() const {
    return objectName.c_str();
}

bool PropertyXLink::upgrade(Base::XMLReader &reader, const char *typeName) {
    if(strcmp(typeName,App::PropertyLinkGlobal::getClassTypeId().getName())==0 ||
       strcmp(typeName,App::PropertyLink::getClassTypeId().getName())==0 ||
       strcmp(typeName,App::PropertyLinkChild::getClassTypeId().getName())==0) 
    {
        PropertyLink::Restore(reader);
        return true;
    }
    FC_ERR("Cannot upgrade from " << typeName);
    return false;
}

int PropertyXLink::checkRestore() const {
    if(!docInfo) return 0;
    if(!_pcLink) {
        if(testFlag(LinkAllowPartial) && 
           (!docInfo->pcDoc || 
            docInfo->pcDoc->testStatus(App::Document::PartialDoc)))
        {
            return 0;
        }
        return 2;
    }
    if(docInfo->pcDoc && stamp==docInfo->pcDoc->LastModifiedDate.getValue())
        return 0;
    return 1;
}

void PropertyXLink::afterRestore() {
    assert(_SubList.size() == _ShadowSubList.size());
    if(!testFlag(LinkRestoreLabel) || !_pcLink || !_pcLink->getNameInDocument())
        return;
    setFlag(LinkRestoreLabel,false);
    for(size_t i=0;i<_SubList.size();++i)
        restoreLabelReference(_pcLink,_SubList[i],&_ShadowSubList[i]);
}

void PropertyXLink::onContainerRestored() {
    if(!_pcLink || !_pcLink->getNameInDocument())
        return;
    for(size_t i=0;i<_SubList.size();++i)
        _registerElementReference(_pcLink,_SubList[i],_ShadowSubList[i]);
}

void PropertyXLink::updateElementReference(DocumentObject *feature,bool reverse,bool notify) {
    if(!updateLinkReference(this,feature,reverse,notify,_pcLink,_SubList,_mapped,_ShadowSubList))
        return;
    if(notify)
        hasSetValue();
}

bool PropertyXLink::referenceChanged() const{
    return !_mapped.empty();
}

void PropertyXLink::Save (Base::Writer &writer) const {
    auto owner = dynamic_cast<const DocumentObject *>(getContainer());
    if(!owner || !owner->getDocument())
        return;

    assert(_SubList.size() == _ShadowSubList.size());

    auto exporting = owner->isExporting();
    if(_pcLink && exporting && _pcLink->isExporting()) {
        // this means, we are exporting the owner and the linked object together. 
        // Lets save the export name
        writer.Stream() << writer.ind() << "<XLink name=\"" << _pcLink->getExportName();
    }else {
        const char *path = filePath.c_str();
        std::string _path;
        if(exporting) {
            // Here means we are exporting the owner but not exporting the
            // linked object.  Try to use aboslute file path for easy transition
            // into document at different directory
            if(docInfo) 
                _path = docInfo->filePath();
            else {
                auto pDoc = owner->getDocument();
                const char *docPath = pDoc->FileName.getValue();
                if(docPath && docPath[0]) {
                    if(filePath.size())
                        _path = DocInfo::getDocPath(filePath.c_str(),pDoc,false);
                    else
                        _path = docPath;
                }else 
                    FC_ERR("External document link without absolute path");
            }
            if(_path.size())
                path = _path.c_str();
        }
        writer.Stream() << writer.ind() 
            << "<XLink file=\"" << encodeAttribute(path)
            << "\" stamp=\"" << (docInfo&&docInfo->pcDoc?docInfo->pcDoc->LastModifiedDate.getValue():"")
            << "\" name=\"" << objectName;
    }

    if(testFlag(LinkAllowPartial))
        writer.Stream() << "\" partial=\"1";

    if(_SubList.empty()) {
        writer.Stream() << "\"/>" << std::endl;
    } else if(_SubList.size() == 1) {
        const auto &subName = _SubList[0];
        const auto &shadowSub = _ShadowSubList[0];
        const auto &sub = shadowSub.second.empty()?subName:shadowSub.second;
        if(exporting) {
            writer.Stream() << "\" sub=\"" << exportSubName(_pcLink,sub.c_str());
            if(shadowSub.second.size() && shadowSub.first==subName)
                writer.Stream() << "\" " ATTR_MAPPED "=\"1";
        }else{
            writer.Stream() << "\" sub=\"" << sub;
            if(sub.size()) {
                if(sub!=subName)
                    writer.Stream() << "\" " ATTR_SHADOWED "=\"" << subName;
                else if(shadowSub.first.size())
                    writer.Stream() << "\" " ATTR_SHADOW "=\"" << shadowSub.first;
            }
        }
        writer.Stream() << "\"/>" << std::endl;
    }else {
        writer.Stream() <<"\" count=\"" << _SubList.size() << "\">" << std::endl;
        writer.incInd();
        for(unsigned int i = 0;i<_SubList.size(); i++) {
            const auto &shadow = _ShadowSubList[i];
            // shadow.second stores the old style element name. For backward
            // compatibility reason, we shall store the old name into attribute
            // 'value' whenver possible.
            const auto &sub = shadow.second.empty()?_SubList[i]:shadow.second;
            writer.Stream() << writer.ind() << "<Sub value=\"";
            if(exporting) {
                writer.Stream() << exportSubName(_pcLink,sub.c_str());
                if(shadow.second.size() && shadow.first == _SubList[i])
                    writer.Stream() << "\" " ATTR_MAPPED "=\"1";
            } else {
                writer.Stream() << sub;
                if(_SubList[i].size()) {
                    if(sub!=_SubList[i])
                        writer.Stream() << "\" " ATTR_SHADOWED "=\"" << _SubList[i];
                    else if(shadow.first.size())
                        writer.Stream() << "\" " ATTR_SHADOW "=\"" << shadow.first;
                }
            }
            writer.Stream()<<"\"/>" << endl;
        }
        writer.decInd();
        writer.Stream() << writer.ind() << "</XLink>" << endl ;
    }
}

void PropertyXLink::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("XLink");
    std::string stamp,file;
    if(reader.hasAttribute("stamp"))
        stamp = reader.getAttribute("stamp");
    if(reader.hasAttribute("file"))
        file = reader.getAttribute("file");
    setFlag(LinkAllowPartial, 
            reader.hasAttribute("partial") && 
            reader.getAttributeAsInteger("partial"));
    std::string name;
    if(file.empty()) 
        name = reader.getName(reader.getAttribute("name"));
    else
        name = reader.getAttribute("name");

    assert(getContainer()->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId()));
    DocumentObject *object = 0;
    if(name.size() && file.empty()) {
        DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        Document *document = parent->getDocument();
        object = document ? document->getObject(name.c_str()) : 0;
        if(!object) {
            if(reader.isVerbose()) {
                FC_WARN("Lost link to '" << name << "' while loading, maybe "
                        "an object was not loaded correctly");
            }
        }
    }

    std::vector<std::string> subs;
    std::vector<ShadowSub> shadows;
    std::vector<int> mapped;
    bool restoreLabel = false;
    if(reader.hasAttribute("sub")) {
        if(reader.hasAttribute(ATTR_MAPPED))
            mapped.push_back(0);
        subs.emplace_back();
        auto &subname = subs.back();
        shadows.emplace_back();
        auto &shadow = shadows.back();
        shadow.second = importSubName(reader,reader.getAttribute("sub"),restoreLabel);
        if(reader.hasAttribute(ATTR_SHADOWED))
            subname = shadow.first = importSubName(reader,reader.getAttribute(ATTR_SHADOWED),restoreLabel);
        else {
            subname = shadow.second;
            if(reader.hasAttribute(ATTR_SHADOW))
                shadow.first = importSubName(reader,reader.getAttribute(ATTR_SHADOW),restoreLabel);
        }
    }else if(reader.hasAttribute("count")) {
        int count = reader.getAttributeAsInteger("count");
        subs.resize(count);
        shadows.resize(count);
        for (int i = 0; i < count; i++) {
            reader.readElement("Sub");
            shadows[i].second = importSubName(reader,reader.getAttribute("value"),restoreLabel);
            if(reader.hasAttribute(ATTR_SHADOWED)) 
                subs[i] = shadows[i].first = 
                    importSubName(reader,reader.getAttribute(ATTR_SHADOWED),restoreLabel);
            else {
                subs[i] = shadows[i].second;
                if(reader.hasAttribute(ATTR_SHADOW))
                    shadows[i].first = importSubName(reader,reader.getAttribute(ATTR_SHADOW),restoreLabel);
            }
            if(reader.hasAttribute(ATTR_MAPPED))
                mapped.push_back(i);
        }
        reader.readEndElement("XLink");
    }
    setFlag(LinkRestoreLabel,restoreLabel);

    if (name.empty()) {
        setValue(0);
        return;
    }

    if(file.size()) {
        this->stamp = stamp;
        _setValue(std::move(file),std::move(name),std::move(subs),std::move(shadows));
    }else
        _setValue(object,std::move(subs),std::move(shadows));
    _mapped = std::move(mapped);
}

Property *PropertyXLink::CopyOnImportExternal(
        const std::map<std::string,std::string> &nameMap) const
{
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if(!owner || !owner->getDocument() || !_pcLink || !_pcLink->getNameInDocument())
        return 0;

    auto subs = updateLinkSubs(_pcLink,_SubList,
                    &tryImportSubName,owner->getDocument(),nameMap);
    auto linked = tryImport(owner->getDocument(),_pcLink,nameMap);
    if(subs.empty() && linked==_pcLink) 
        return 0;

    PropertyXLink *p= createInstance();
    copyTo(*p,linked,&subs);
    return p;
}

PropertyXLink *PropertyXLink::createInstance() const {
    return new PropertyXLink();
}

Property *PropertyXLink::CopyOnLabelChange(App::DocumentObject *obj, 
        const std::string &ref, const char *newLabel) const 
{
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if(!owner || !owner->getDocument() || !_pcLink || !_pcLink->getNameInDocument())
        return 0;
    auto subs = updateLinkSubs(_pcLink,_SubList,&updateLabelReference,obj,ref,newLabel);
    if(subs.empty()) 
        return 0;
    PropertyXLink *p= createInstance();
    copyTo(*p,_pcLink,&subs);
    return p;
}

void PropertyXLink::copyTo(PropertyXLink &other, 
        DocumentObject *linked, std::vector<std::string> *subs) const 
{
    if(!linked)
        linked = _pcLink;
    if(_pcLink && _pcLink->getNameInDocument()) {
        other.docName = _pcLink->getDocument()->getName();
        other.objectName = _pcLink->getNameInDocument();
    }else{
        other.objectName = objectName;
        other.docName.clear();
        other.docInfo = docInfo;
        other.filePath = filePath;
    }
    if(subs)
        other._SubList = std::move(*subs);
    else
        other._SubList = _SubList;
    other._Flags = _Flags;
}

Property *PropertyXLink::Copy(void) const
{
    PropertyXLink *p= createInstance();
    copyTo(*p);
    return p;
}

void PropertyXLink::Paste(const Property &from)
{
    if(!from.isDerivedFrom(PropertyXLink::getClassTypeId()))
        throw Base::TypeError("Incompatible proeprty to paste to");

    const auto &other = static_cast<const PropertyXLink&>(from);
    if(other.docName.size()) {
        auto doc = GetApplication().getDocument(other.docName.c_str());
        if(!doc) {
            FC_WARN("Document '" << other.docName << "' not found");
            return;
        }
        auto obj = doc->getObject(other.objectName.c_str());
        if(!obj) {
            FC_WARN("Object '" << other.docName << '#' << other.objectName << "' not found");
            return;
        }
        _setValue(obj,std::vector<std::string>(other._SubList));
    } else
        _setValue(std::string(other.filePath),std::string(other.objectName),
                std::vector<std::string>(other._SubList));
    setFlag(LinkAllowPartial,other.testFlag(LinkAllowPartial));
}

bool PropertyXLink::supportXLink(const App::Property *prop) {
    return prop->isDerivedFrom(PropertyXLink::getClassTypeId()) ||
        prop->isDerivedFrom(PropertyXLinkSubList::getClassTypeId()) ||
        prop->isDerivedFrom(PropertyXLinkContainer::getClassTypeId());
}

bool PropertyXLink::hasXLink(const App::Document *doc) {
    for(auto &v : _DocInfoMap) {
        if(v.second->hasXLink(doc))
            return true;
    }
    return false;
}

bool PropertyXLink::hasXLink(
        const std::vector<App::DocumentObject*> &objs, std::vector<App::Document*> *unsaved) 
{
    std::set<App::Document*> docs;
    bool ret = false;
    for(auto o : objs) {
        if(o && o->getNameInDocument() && docs.insert(o->getDocument()).second) {
            if(!hasXLink(o->getDocument()))
                continue;
            if(!unsaved)
                return true;
            ret = true;
            if(!o->getDocument()->isSaved())
                unsaved->push_back(o->getDocument());
        }
    }
    return ret;
}

std::map<App::Document*,std::set<App::Document*> > 
PropertyXLink::getDocumentOutList(App::Document *doc) {
    std::map<App::Document*,std::set<App::Document*> > ret;
    for(auto &v : _DocInfoMap) {
        for(auto link : v.second->links) {
            if(!v.second->pcDoc) continue;
            auto obj = dynamic_cast<App::DocumentObject*>(link->getContainer());
            if(!obj || !obj->getNameInDocument() || !obj->getDocument())
                continue;
            if(doc && obj->getDocument()!=doc)
                continue;
            ret[obj->getDocument()].insert(v.second->pcDoc);
        }
    }
    return ret;
}

std::map<App::Document*,std::set<App::Document*> > 
PropertyXLink::getDocumentInList(App::Document *doc) {
    std::map<App::Document*,std::set<App::Document*> > ret;
    for(auto &v : _DocInfoMap) {
        if(!v.second->pcDoc || (doc && doc!=v.second->pcDoc))
            continue;
        auto &docs = ret[v.second->pcDoc];
        for(auto link : v.second->links) {
            auto obj = dynamic_cast<App::DocumentObject*>(link->getContainer());
            if(obj && obj->getNameInDocument() && obj->getDocument())
                docs.insert(obj->getDocument());
        }
    }
    return ret;
}

PyObject *PropertyXLink::getPyObject(void)
{
    if(!_pcLink)
        Py_Return;
    if(_SubList.empty())
        return _pcLink->getPyObject();
    Py::Tuple ret(2);
    ret.setItem(0,Py::Object(_pcLink->getPyObject(),true));
    ret.setItem(1,Py::String(getSubName(true)));
    return Py::new_reference_to(ret);
}

void PropertyXLink::setPyObject(PyObject *value) {
    if(PySequence_Check(value)) {
        Py::Sequence seq(value);
        if(seq.size()!=2) 
            throw Base::ValueError("Expect input sequence of size 2");
        std::string subname;
        PyObject *pyObj = seq[0].ptr();
        PyObject *pySub = seq[1].ptr();
        if(pyObj == Py_None) {
            setValue(0);
            return;
        } else if(!PyObject_TypeCheck(pyObj, &DocumentObjectPy::Type))
            throw Base::TypeError("Expect the first element to be of 'DocumentObject'");
        if (PyUnicode_Check(pySub)) {
#if PY_MAJOR_VERSION >= 3
            subname = PyUnicode_AsUTF8(pySub);
#else
            PyObject* unicode = PyUnicode_AsUTF8String(pySub);
            subname = PyString_AsString(unicode);
            Py_DECREF(unicode);
        }else if (PyString_Check(pySub)) {
            subname = PyString_AsString(pySub);
#endif
        }else
            throw Base::TypeError("Expect the second element to be a string");
        setValue(static_cast<DocumentObjectPy*>(pyObj)->getDocumentObjectPtr(), subname.c_str());
    } else if(PyObject_TypeCheck(value, &(DocumentObjectPy::Type))) {
        setValue(static_cast<DocumentObjectPy*>(value)->getDocumentObjectPtr());
    } else if (Py_None == value) {
        setValue(0);
    } else {
        throw Base::TypeError("type must be 'DocumentObject', 'None', or '(DocumentObject, SubName)");
    }
}

const char *PropertyXLink::getSubName(bool newStyle) const {
    if(_SubList.empty() || _ShadowSubList.empty())
        return "";
    return getSubNameWithStyle(_SubList[0],_ShadowSubList[0],newStyle).c_str();
}

void PropertyXLink::getLinks(std::vector<App::DocumentObject *> &objs, 
        bool all, std::vector<std::string> *subs, bool newStyle) const
{
    if((all||_pcScope!=LinkScope::Hidden) && _pcLink && _pcLink->getNameInDocument()) {
        objs.push_back(_pcLink);
        if(subs)
        if(subs && _SubList.size()==_ShadowSubList.size())
            *subs = getSubValues(newStyle);
    }
}

bool PropertyXLink::adjustLink(const std::set<App::DocumentObject*> &inList) {
    if (_pcScope==LinkScope::Hidden)
        return false;
    if(!_pcLink || !_pcLink->getNameInDocument() || !inList.count(_pcLink))
        return false;
    auto subs = _SubList;
    auto link = adjustLinkSubs(this,inList,_pcLink,subs); 
    _setValue(link,std::move(subs));
    return true;
}

std::vector<std::string> PropertyXLink::getSubValues(bool newStyle) const {
    assert(_SubList.size() == _ShadowSubList.size());
    std::vector<std::string> ret;
    ret.reserve(_SubList.size());
    for(size_t i=0;i<_ShadowSubList.size();++i)
        ret.push_back(getSubNameWithStyle(_SubList[i],_ShadowSubList[i],newStyle));
    return ret;
}

std::vector<std::string> PropertyXLink::getSubValuesStartsWith(const char* starter, bool newStyle) const
{
    assert(_SubList.size() == _ShadowSubList.size());
    std::vector<std::string> ret;
    for(size_t i=0;i<_ShadowSubList.size();++i) {
        const auto &sub = getSubNameWithStyle(_SubList[i],_ShadowSubList[i],newStyle);
        auto element = Data::ComplexGeoData::findElementName(sub.c_str());
        if(element && boost::starts_with(element,starter))
            ret.emplace_back(element);
    }
    return ret;
}

void PropertyXLink::setAllowPartial(bool enable) {
    setFlag(LinkAllowPartial,enable);
    if(enable)
        return;
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if(!owner)
        return;
    if(!App::GetApplication().isRestoring() && 
       !owner->getDocument()->isPerformingTransaction() &&
       !_pcLink && docInfo && filePath.size() && objectName.size() &&
       (!docInfo->pcDoc || docInfo->pcDoc->testStatus(Document::PartialDoc)))
    {
        auto path = docInfo->getDocPath(filePath.c_str(),owner->getDocument(),false);
        if(path.size())
            App::GetApplication().openDocument(path.c_str());
    }
}

//**************************************************************************
// PropertyXLinkSub
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyXLinkSub , App::PropertyXLink)

PropertyXLinkSub::PropertyXLinkSub(bool allowPartial, PropertyLinkBase *parent)
    :PropertyXLink(allowPartial,parent)
{

}

PropertyXLinkSub::~PropertyXLinkSub() {
}

void PropertyXLinkSub::setValue(App::DocumentObject *link,
        const std::vector<std::string> &subs, std::vector<ShadowSub> &&shadows)
{
    _setValue(link,std::vector<std::string>(subs),std::move(shadows));
}

void PropertyXLinkSub::setValue(App::DocumentObject *link,
        std::vector<std::string> &&subs, std::vector<ShadowSub> &&shadows)
{
    _setValue(link,std::move(subs),std::move(shadows));
}

void PropertyXLinkSub::setSubValues(std::vector<std::string> &&subs, 
        std::vector<ShadowSub> &&shadows)
{
    aboutToSetValue();
    _setSubValues(std::move(subs),std::move(shadows));
    hasSetValue();
}

PropertyXLink *PropertyXLinkSub::createInstance() const{
    return new PropertyXLinkSub();
}

bool PropertyXLinkSub::upgrade(Base::XMLReader &reader, const char *typeName) {
    if(strcmp(typeName, PropertyLinkSubGlobal::getClassTypeId().getName())==0 ||
       strcmp(typeName, PropertyLinkSub::getClassTypeId().getName())==0 ||
       strcmp(typeName, PropertyLinkSubChild::getClassTypeId().getName())==0) 
    {
        App::PropertyLinkSub linkProp;
        linkProp.setContainer(getContainer());
        linkProp.Restore(reader);
        setValue(linkProp.getValue(),linkProp.getSubValues());
        return true;
    }
    return PropertyXLink::upgrade(reader,typeName);
}

PyObject *PropertyXLinkSub::getPyObject(void)
{
    Py::Tuple tup(2);
    Py::List list(static_cast<int>(_SubList.size()));
    if (_pcLink) 
        tup[0] = Py::asObject(_pcLink->getPyObject());
    else {
        tup[0] = Py::None();
        if(_SubList.empty())
            Py_Return;
    }
    for(unsigned int i = 0;i<_SubList.size(); i++)
        list[i] = Py::String(_SubList[i]);
    tup[1] = list;
    return Py::new_reference_to(tup);
}

void PropertyXLinkSub::setPyObject(PyObject *value) {
    if(PySequence_Check(value)) {
        Py::Sequence seq(value);
        if(seq.size()!=2) 
            throw Base::ValueError("Expect input sequence of size 2");
        std::vector<std::string> subs;
        Py::Object pyObj(seq[0].ptr());
        Py::Object pySub(seq[1].ptr());
        if(pyObj.isNone()) {
            setValue(0);
            return;
        } else if(!PyObject_TypeCheck(pyObj.ptr(), &DocumentObjectPy::Type))
            throw Base::TypeError("Expect the first element to be of 'DocumentObject'");
        if(pySub.isString()) 
            subs.push_back(pySub.as_string());
        else if(pySub.isSequence()) {
            Py::Sequence seq(pySub);
            subs.reserve(seq.size());
            for(size_t i=0;i<seq.size();++i) {
                Py::Object sub(seq[i]);
                if(!sub.isString())
                    throw Base::TypeError("Expect only string inside second argument");
                subs.push_back(sub.as_string());
            }
        }else
            throw Base::TypeError("Expect the second element to be a string or sequence of string");
        setValue(static_cast<DocumentObjectPy*>(pyObj.ptr())->getDocumentObjectPtr(), std::move(subs));
    } else if(PyObject_TypeCheck(value, &(DocumentObjectPy::Type))) {
        setValue(static_cast<DocumentObjectPy*>(value)->getDocumentObjectPtr());
    } else if (Py_None == value) {
        setValue(0);
    } else {
        throw Base::TypeError("type must be 'DocumentObject', 'None', or '(DocumentObject, SubName)' or "
                "'DocumentObject, [SubName..])");
    }
}

//**************************************************************************
// PropertyXLinkSubList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyXLinkSubList , App::PropertyLinkBase)

//**************************************************************************
// Construction/Destruction


PropertyXLinkSubList::PropertyXLinkSubList()
{
    _pcScope = LinkScope::Global;
}

PropertyXLinkSubList::~PropertyXLinkSubList()
{
}

int PropertyXLinkSubList::getSize(void) const
{
    return static_cast<int>(_Links.size());
}

void PropertyXLinkSubList::setValue(DocumentObject* lValue,const char* SubName)
{
    std::map<DocumentObject*,std::vector<std::string> > values;
    if(lValue) {
        auto &subs = values[lValue];
        if(SubName)
            subs.emplace_back(SubName);
    }
    setValues(std::move(values));
}

void PropertyXLinkSubList::setValues(
        const std::vector<DocumentObject*>& lValue, 
        const std::vector<const char*>& lSubNames)
{
#define CHECK_SUB_SIZE(_l,_r) do{\
        if(_l.size()!=_r.size())\
            FC_THROWM(Base::ValueError, "object and subname size mismatch");\
    }while(0)
    CHECK_SUB_SIZE(lValue,lSubNames);
    std::map<DocumentObject*,std::vector<std::string> > values;
    int i=0;
    for(auto &obj : lValue) {
        const char *sub = lSubNames[i++];
        if(sub)
            values[obj].emplace_back(sub);
    }
    setValues(std::move(values));
}

void PropertyXLinkSubList::setValues(const std::vector<DocumentObject*>& lValue,
                                     const std::vector<std::string>& lSubNames)
{
    CHECK_SUB_SIZE(lValue,lSubNames);
    std::map<DocumentObject*,std::vector<std::string> > values;
    int i=0;
    for(auto &obj : lValue) 
        values[obj].push_back(lSubNames[i++]);
    setValues(std::move(values));
}

void PropertyXLinkSubList::setSubListValues(const std::vector<PropertyLinkSubList::SubSet> &svalues) {
    std::map<DocumentObject*,std::vector<std::string> > values;
    for(auto &v : svalues) {
        auto &s = values[v.first];
        s.reserve(s.size()+v.second.size());
        s.insert(s.end(),v.second.begin(),v.second.end());
    }
    setValues(std::move(values));
}

void PropertyXLinkSubList::setValues(
        const std::map<App::DocumentObject*,std::vector<std::string> > &values) 
{
    setValues(std::map<App::DocumentObject*,std::vector<std::string> >(values));
}

void PropertyXLinkSubList::setValues(
        std::map<App::DocumentObject*,std::vector<std::string> > &&values) 
{
    for(auto &v : values) {
        if(!v.first || !v.first->getNameInDocument())
            FC_THROWM(Base::ValueError,"invalid document object");
    }
    
    aboutToSetValue();

    for(auto it=_Links.begin(),itNext=it;it!=_Links.end();it=itNext) {
        ++itNext;
        auto iter = values.find(it->getValue());
        if(iter == values.end()) {
            _Links.erase(it);
            continue;
        }
        it->setSubValues(std::move(iter->second));
        values.erase(iter);
    }

    for(auto &v : values) {
        _Links.emplace_back(testFlag(LinkAllowPartial),this);
        _Links.back().setValue(v.first,std::move(v.second));
    }
    hasSetValue();
}

void PropertyXLinkSubList::addValue(App::DocumentObject *obj, 
        const std::vector<std::string> &subs, bool reset) 
{
    addValue(obj,std::vector<std::string>(subs),reset);
}

void PropertyXLinkSubList::addValue(App::DocumentObject *obj, 
        std::vector<std::string> &&subs, bool reset) {

    if(!obj || !obj->getNameInDocument())
        FC_THROWM(Base::ValueError,"invalid document object");
    
    for(auto &l : _Links) {
        if(l.getValue() == obj) {
            auto s = l.getSubValues();
            if(s.empty() || reset)
                l.setSubValues(std::move(subs));
            else {
                s.reserve(s.size()+subs.size());
                std::move(subs.begin(),subs.end(),std::back_inserter(s));
                l.setSubValues(std::move(s));
            }
            return;
        }
    }
    aboutToSetValue();
    _Links.emplace_back(testFlag(LinkAllowPartial),this);
    _Links.back().setValue(obj,std::move(subs));
    hasSetValue();
}

void PropertyXLinkSubList::setValue(DocumentObject* lValue, const std::vector<string> &SubList)
{
    std::map<DocumentObject *, std::vector<std::string> > values;
    if(lValue)
        values[lValue] = SubList;
    setValues(std::move(values));
}

const string PropertyXLinkSubList::getPyReprString() const
{
    if (_Links.empty())
        return std::string("None");
    std::ostringstream ss;
    ss << '[';
    for(auto &link : _Links) {
        auto obj = link.getValue();
        if(!obj || !obj->getNameInDocument())
            continue;
        ss << "(App.getDocument('" << obj->getDocument()->getName() 
           << "').getObject('" << obj->getNameInDocument() << "'),  (";
        const auto &subs = link.getSubValues();
        if(subs.empty())
            ss << "''";
        else{
            for(auto &sub : subs)
                ss << "'" << sub << "',";
        }
        ss << ")), ";
    }
    ss << ']';
    return ss.str();
}

DocumentObject *PropertyXLinkSubList::getValue() const
{
    if(_Links.size())
        return _Links.begin()->getValue();
    return 0;
}

int PropertyXLinkSubList::removeValue(App::DocumentObject *lValue)
{
    int ret = 0;
    auto it = std::find_if(_Links.begin(),_Links.end(),
                [=](const PropertyXLinkSub &l){return l.getValue()==lValue;});
    if(it != _Links.end()) {
        ret = (int)it->getSubValues().size();
        if(!ret)
            ret = 1;
        _Links.erase(it);
    }
    return ret;
}

PyObject *PropertyXLinkSubList::getPyObject(void)
{
    Py::List list;
    for(auto &link : _Links) {
        auto obj = link.getValue();
        if(!obj || !obj->getNameInDocument())
            continue;
        Py::Tuple tup(2);
        tup[0] = Py::asObject(obj->getPyObject());

        const auto &subs = link.getSubValues();
        Py::Tuple items(subs.size());
        for (std::size_t j = 0; j < subs.size(); j++) {
            items[j] = Py::String(subs[j]);
        }
        tup[1] = items;
        list.append(tup);
    }
    return Py::new_reference_to(list);
}

void PropertyXLinkSubList::setPyObject(PyObject *value)
{
    try { //try PropertyLinkSub syntax
        PropertyLinkSub dummy;
        dummy.setAllowExternal(true);
        dummy.setPyObject(value);
        this->setValue(dummy.getValue(), dummy.getSubValues());
        return;
    }
    catch (Base::Exception&) {}

    if (!PyTuple_Check(value) && !PyList_Check(value))
        throw Base::TypeError("Invalid type. Accepts (DocumentObject, (subname...)) or sequence of such type.");
    Py::Sequence seq(value);
    std::map<DocumentObject*, std::vector<std::string> > values;
    try {
        for(size_t i=0;i<seq.size();++i) {
            PropertyLinkSub link;
            link.setAllowExternal(true);
            link.setPyObject(seq[i].ptr());
            const auto &subs = link.getSubValues();
            auto &s = values[link.getValue()];
            s.reserve(s.size()+subs.size());
            s.insert(s.end(),subs.begin(),subs.end());
        }
    }catch(Base::Exception &e){
        throw Base::TypeError("Invalid type inside sequence. Must be type of (DocumentObject, (subname...))");
    }
    setValues(std::move(values));
}

void PropertyXLinkSubList::afterRestore() {
    for(auto &l : _Links) 
        l.afterRestore();
}

void PropertyXLinkSubList::onContainerRestored() {
    for(auto &l : _Links) 
        l.onContainerRestored();
}

void PropertyXLinkSubList::updateElementReference(DocumentObject *feature, bool reverse,bool notify) {
    for(auto &l : _Links)
        l.updateElementReference(feature,reverse,notify);
}

bool PropertyXLinkSubList::referenceChanged() const{
    for(auto &l : _Links) {
        if(l.referenceChanged())
            return true;
    }
    return false;
}

void PropertyXLinkSubList::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<XLinkSubList count=\"" << _Links.size();
    if(testFlag(LinkAllowPartial))
        writer.Stream() << "\" partial=\"1";
    writer.Stream() <<"\">" << endl;
    writer.incInd();
    for(auto &l : _Links)
        l.Save(writer);
    writer.decInd();
    writer.Stream() << writer.ind() << "</XLinkSubList>" << endl ;
}

void PropertyXLinkSubList::Restore(Base::XMLReader &reader)
{
    reader.readElement("XLinkSubList");
    setFlag(LinkAllowPartial,
                reader.hasAttribute("partial") &&
                reader.getAttributeAsInteger("partial"));
    int count = reader.getAttributeAsInteger("count");
    _Links.clear();
    for(int i=0;i<count;++i) {
        _Links.emplace_back(false,this);
        _Links.back().Restore(reader);
    }
    reader.readEndElement("XLinkSubList");
}

Property *PropertyXLinkSubList::CopyOnImportExternal(
        const std::map<std::string,std::string> &nameMap) const
{
    std::unique_ptr<Property> copy;
    auto it = _Links.begin();
    for(;it!=_Links.end();++it) {
        copy.reset(it->CopyOnImportExternal(nameMap));
        if(copy) break;
    }
    if(!copy)
        return 0;
    std::unique_ptr<PropertyXLinkSubList> p(new PropertyXLinkSubList);
    for(auto iter=_Links.begin();iter!=it;++iter) {
        p->_Links.emplace_back();
        iter->copyTo(p->_Links.back());
    }
    p->_Links.emplace_back();
    static_cast<PropertyXLinkSub&>(*copy).copyTo(p->_Links.back());
    for(++it;it!=_Links.end();++it) {
        p->_Links.emplace_back();
        copy.reset(it->CopyOnImportExternal(nameMap));
        if(copy)
            static_cast<PropertyXLinkSub&>(*copy).copyTo(p->_Links.back());
        else
            it->copyTo(p->_Links.back());
    }
    return p.release();
}

Property *PropertyXLinkSubList::CopyOnLabelChange(App::DocumentObject *obj, 
        const std::string &ref, const char *newLabel) const 
{
    std::unique_ptr<Property> copy;
    auto it = _Links.begin();
    for(;it!=_Links.end();++it) {
        copy.reset(it->CopyOnLabelChange(obj,ref,newLabel));
        if(copy) break;
    }
    if(!copy)
        return 0;
    std::unique_ptr<PropertyXLinkSubList> p(new PropertyXLinkSubList);
    for(auto iter=_Links.begin();iter!=it;++iter) {
        p->_Links.emplace_back();
        iter->copyTo(p->_Links.back());
    }
    p->_Links.emplace_back();
    static_cast<PropertyXLinkSub&>(*copy).copyTo(p->_Links.back());
    for(++it;it!=_Links.end();++it) {
        p->_Links.emplace_back();
        copy.reset(it->CopyOnLabelChange(obj,ref,newLabel));
        if(copy)
            static_cast<PropertyXLinkSub&>(*copy).copyTo(p->_Links.back());
        else
            it->copyTo(p->_Links.back());
    }
    return p.release();
}

Property *PropertyXLinkSubList::Copy(void) const
{
    PropertyXLinkSubList *p = new PropertyXLinkSubList();
    for(auto &l : _Links) {
        p->_Links.emplace_back(testFlag(LinkAllowPartial),p);
        l.copyTo(p->_Links.back());
    }
    return p;
}

void PropertyXLinkSubList::Paste(const Property &from)
{
    if(!from.isDerivedFrom(PropertyXLinkSubList::getClassTypeId()))
        throw Base::TypeError("Incompatible proeprty to paste to");

    aboutToSetValue();
    _Links.clear();
    for(auto &l : static_cast<const PropertyXLinkSubList&>(from)._Links) {
        _Links.emplace_back(testFlag(LinkAllowPartial),this);
        _Links.back().Paste(l);
    }
    hasSetValue();
}

unsigned int PropertyXLinkSubList::getMemSize (void) const
{
    unsigned int size=0;
    for(auto &l : _Links)
        size += l.getMemSize();
   return size;
}

const std::vector<std::string> &PropertyXLinkSubList::getSubValues(App::DocumentObject *obj) const {
    for(auto &l : _Links) {
        if(l.getValue() == obj)
            return l.getSubValues();
    }
    FC_THROWM(Base::RuntimeError, "object not found");
}

std::vector<std::string> PropertyXLinkSubList::getSubValues(App::DocumentObject *obj, bool newStyle) const {
    for(auto &l : _Links) {
        if(l.getValue() == obj)
            return l.getSubValues(newStyle);
    }
    return {};
}

void PropertyXLinkSubList::getLinks(std::vector<App::DocumentObject *> &objs, 
        bool all, std::vector<std::string> *subs, bool newStyle) const
{
    if(all||_pcScope!=LinkScope::Hidden) {
        if(!subs) {
            objs.reserve(objs.size()+_Links.size());
            for(auto &l : _Links) {
                auto obj = l.getValue();
                if(obj && obj->getNameInDocument())
                    objs.push_back(obj);
            }
            return;
        }
        size_t count=0;
        for(auto &l : _Links) {
            auto obj = l.getValue();
            if(obj && obj->getNameInDocument())
                count += l.getSubValues().size();
        }
        objs.reserve(objs.size()+count);
        subs->reserve(subs->size()+count);
        for(auto &l : _Links) {
            auto obj = l.getValue();
            if(obj && obj->getNameInDocument()) {
                for(auto &sub : l.getSubValues(newStyle)) {
                    objs.push_back(obj);
                    subs->push_back(std::move(sub));
                }
            }
        }
    }
}

void PropertyXLinkSubList::breakLink(App::DocumentObject *obj, bool clear) {
    if(clear && getContainer()==obj) {
        setValue(0);
        return;
    }
    bool touched = false;
    for(auto &l : _Links) {
        if(l.getValue() == obj) {
            if(!touched) {
                touched = true;
                aboutToSetValue();
            }
            l.setValue(0);
        }
    }
    if(touched)
        hasSetValue();
}

bool PropertyXLinkSubList::adjustLink(const std::set<App::DocumentObject*> &inList) {
    if (_pcScope==LinkScope::Hidden)
        return false;
    std::map<App::DocumentObject*,std::vector<std::string> > values;
    bool touched = false;
    int count=0;
    for(auto &l : _Links) {
        auto obj = l.getValue();
        if(!obj || !obj->getNameInDocument()) {
            ++count;
            continue;
        }
        if(inList.count(obj) && adjustLinkSubs(this,inList,obj,l._SubList,&values))
            touched = true;
    }
    if(touched) {
        decltype(_Links) tmp;
        if(count) {
            // XLink allows detached state, i.e. with closed external document. So
            // we need to preserve empty link
            for(auto it=_Links.begin(),itNext=it;it!=_Links.end();it=itNext) {
                ++itNext;
                if(!it->getValue())
                    tmp.splice(tmp.end(),_Links,it);
            }
        }
        setValues(std::move(values));
        _Links.splice(_Links.end(),tmp);
    }
    return touched;
}

int PropertyXLinkSubList::checkRestore() const {
    for(auto &l : _Links) {
        int res;
        if((res = l.checkRestore()))
            return res;
    }
    return 0;
}

bool PropertyXLinkSubList::upgrade(Base::XMLReader &reader, const char *typeName) {
    if(strcmp(typeName, PropertyLinkSubListGlobal::getClassTypeId().getName())==0 ||
       strcmp(typeName, PropertyLinkSubList::getClassTypeId().getName())==0 ||
       strcmp(typeName, PropertyLinkSubListChild::getClassTypeId().getName())==0) 
    {
        PropertyLinkSubList linkProp;
        linkProp.setContainer(getContainer());
        linkProp.Restore(reader);
        std::map<DocumentObject *, std::vector<std::string> > values;
        const auto &objs = linkProp.getValues();
        const auto &subs = linkProp.getSubValues();
        assert(objs.size() == subs.size());
        for(size_t i=0;i<objs.size();++i)
            values[objs[i]].push_back(subs[i]);
        setValues(std::move(values));
        return true;
    }
    _Links.clear();
    _Links.emplace_back(testFlag(LinkAllowPartial),this);
    if(!_Links.back().upgrade(reader,typeName)) {
        _Links.clear();
        return false;
    }
    return true;
}

void PropertyXLinkSubList::setAllowPartial(bool enable) {
    setFlag(LinkAllowPartial,enable);
    for(auto &l : _Links)
        l.setAllowPartial(enable);
}

void PropertyXLinkSubList::hasSetChildValue(Property &) {
    hasSetValue();
}

void PropertyXLinkSubList::aboutToSetChildValue(Property &) {
    aboutToSetValue();
}

//**************************************************************************
// PropertyXLinkContainer
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyXLinkContainer , App::PropertyLinkBase)

PropertyXLinkContainer::PropertyXLinkContainer() {
    _pcScope = LinkScope::Global;
    _LinkRestored = false;
}

PropertyXLinkContainer::~PropertyXLinkContainer() {
}

void PropertyXLinkContainer::afterRestore() {
    _DocMap.clear();
    if(!_XLinkRestores)
        return;
    _Deps.clear();
    for(auto &info : *_XLinkRestores) {
        auto obj = info.xlink->getValue();
        if(!obj)
            continue;
        if(info.docName.size()) {
            if(info.docName != obj->getDocument()->getName())
                _DocMap[info.docName] = obj->getDocument()->getName();
            if(info.docLabel != obj->getDocument()->Label.getValue())
                _DocMap[App::quote(info.docLabel)] = obj->getDocument()->Label.getValue();
        }
        if(_Deps.insert(obj).second)
            _XLinks[obj->getFullName()] = std::move(info.xlink);
    }
    _XLinkRestores.reset();
}

void PropertyXLinkContainer::breakLink(App::DocumentObject *obj, bool clear) {
    if(!obj || !obj->getNameInDocument())
        return;
    auto owner = dynamic_cast<App::DocumentObject*>(getContainer());
    if(!owner || !owner->getNameInDocument())
        return;
    if(!clear || obj!=owner) {
        if(!_Deps.erase(obj))
            return;
        aboutToSetValue();
        onBreakLink(obj);
        if(obj->getDocument() == owner->getDocument())
            obj->_removeBackLink(owner);
        else
            _XLinks.erase(obj->getFullName());
        hasSetValue();
        return;
    }
    if(obj!=owner)
        return;
    for(auto obj : _Deps) {
        if(!obj || !obj->getNameInDocument())
            continue;
        onBreakLink(obj);
        if(obj->getDocument()==owner->getDocument())
            obj->_removeBackLink(owner);
    }
    _XLinks.clear();
}

int PropertyXLinkContainer::checkRestore() const {
    if(_LinkRestored)
        return 1;
    for(auto &v : _XLinks) {
        int res = v.second->checkRestore();
        if(res)
            return res;
    }
    return 0;
}

void PropertyXLinkContainer::Save (Base::Writer &writer) const {

    writer.Stream() << writer.ind() << "<XLinks count=\"" << _XLinks.size();

    std::map<App::Document*,int> docSet;
    auto owner = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    if(owner && !owner->isExporting()) {
        // Document name and label can change on restore, we shall record the
        // current document name and label and pair it with the associated
        // xlink, so that we can restore them correctly.
        int i=-1;
        for(auto &v : _XLinks) {
            ++i;
            auto obj = v.second->getValue();
            if(obj && obj->getDocument())
                docSet.insert(std::make_pair(obj->getDocument(),i));
        }

        if(docSet.size())
            writer.Stream() << "\" docs=\"" << docSet.size();
    }

    writer.Stream() << "\">" << std::endl;
    writer.incInd();

    for(auto &v : docSet) {
        writer.Stream() << writer.ind() << "<DocMap "
            << "name=\"" << v.first->getName() 
            << "\" label=\"" << encodeAttribute(v.first->Label.getValue()) 
            << "\" index=\"" << v.second << "\"/>" << std::endl;
    }

    for(auto &v : _XLinks) 
        v.second->Save(writer);
    writer.decInd();

    writer.Stream() << writer.ind() << "</XLinks>" << std::endl;
}

void PropertyXLinkContainer::Restore(Base::XMLReader &reader) {
    reader.readElement("XLinks");
    auto count = reader.getAttributeAsUnsigned("count");
    _XLinkRestores.reset(new std::vector<RestoreInfo>(count));

    if(reader.hasAttribute("docs")) {
        auto docCount = reader.getAttributeAsUnsigned("docs");
        _DocMap.clear();
        std::vector<std::string> docs(count*2);
        for(unsigned i=0;i<docCount;++i) {
            reader.readElement("DocMap");
            auto index = reader.getAttributeAsUnsigned("index");
            if(index>=count) {
                FC_ERR(propertyName(this) << " invalid document map entry");
                continue;
            }
            auto &info = _XLinkRestores->at(index);
            info.docName = reader.getAttribute("name");
            info.docLabel = reader.getAttribute("label");
        }
    }

    for(auto &info : *_XLinkRestores) {
        info.xlink.reset(createXLink());
        info.xlink->Restore(reader);
    }
    reader.readEndElement("XLinks");
}

void PropertyXLinkContainer::aboutToSetChildValue(App::Property &prop) {
    auto xlink = dynamic_cast<App::PropertyXLink*>(&prop);
    if(xlink && xlink->testFlag(LinkDetached)) {
        if(_Deps.erase(const_cast<App::DocumentObject*>(xlink->getValue())))
            onBreakLink(xlink->getValue());
    }
}

void PropertyXLinkContainer::onBreakLink(DocumentObject *) {
}

PropertyXLink *PropertyXLinkContainer::createXLink() {
    return new PropertyXLink(false,this);
}

bool PropertyXLinkContainer::isLinkedToDocument(const App::Document &doc) const {
    auto iter = _XLinks.lower_bound(doc.getName());
    if(iter != _XLinks.end()) {
        size_t len = strlen(doc.getName());
        return iter->first.size()>len 
            && iter->first[len] == '#'
            && boost::starts_with(iter->first,doc.getName());
    }
    return false;
}

void PropertyXLinkContainer::updateDeps(std::set<DocumentObject*> &&newDeps) {
    auto owner = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    if(!owner || !owner->getNameInDocument())
        return;
    newDeps.erase(owner);

    for(auto obj : newDeps) {
        if(obj && obj->getNameInDocument()) {
            auto it = _Deps.find(obj);
            if(it != _Deps.end()) {
                _Deps.erase(it);
                continue;
            }
            if(owner->getDocument()!=obj->getDocument()) {
                auto &xlink = _XLinks[obj->getFullName()];
                if(!xlink) {
                    xlink.reset(createXLink());
                    xlink->setValue(obj);
                }
            }
#ifndef USE_OLD_DAG
            else
                obj->_addBackLink(owner);
#endif
            onAddDep(obj);
        }
    }
    for(auto obj : _Deps) {
        if(!obj || !obj->getNameInDocument())
            continue;
        if(obj->getDocument()==owner->getDocument()) {
#ifndef USE_OLD_DAG
            obj->_removeBackLink(owner);
#endif
        }else
            _XLinks.erase(obj->getFullName());
        onRemoveDep(obj);
    }
    _Deps = std::move(newDeps);

    _LinkRestored = testFlag(LinkRestoring);

    if(!_LinkRestored && !testFlag(LinkDetached)) {
        for(auto it=_XLinks.begin(),itNext=it;it!=_XLinks.end();it=itNext) {
            ++itNext;
            if(!it->second->getValue())
                _XLinks.erase(it);
        }
    }
}

void PropertyXLinkContainer::clearDeps() {
    auto owner = dynamic_cast<App::DocumentObject*>(getContainer());
    if(!owner || !owner->getNameInDocument())
        return;
    for(auto obj : _Deps) {
        if(obj && obj->getNameInDocument() && obj->getDocument()==owner->getDocument())
            obj->_removeBackLink(owner);
    }
    _Deps.clear();
    _XLinks.clear();
    _LinkRestored = false;
}

void PropertyXLinkContainer::getLinks(std::vector<App::DocumentObject *> &objs, 
        bool, std::vector<std::string> * /*subs*/, bool /*newStyle*/) const
{
    objs.insert(objs.end(),_Deps.begin(),_Deps.end());
}

