/****************************************************************************
 *   Copyright (c) 2017 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost/preprocessor/stringize.hpp>
#include "Application.h"
#include "Document.h"
#include "GroupExtension.h"
#include "Link.h"
#include "LinkBaseExtensionPy.h"
#include <Base/Console.h>
#include "ComplexGeoData.h"
#include "ComplexGeoDataPy.h"

//FIXME: ISO C++11 requires at least one argument for the "..." in a variadic macro
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

FC_LOG_LEVEL_INIT("App::Link", true,true)

using namespace App;
using namespace Base;

EXTENSION_PROPERTY_SOURCE(App::LinkBaseExtension, App::DocumentObjectExtension)

LinkBaseExtension::LinkBaseExtension(void)
    :myOwner(0),enableLabelCache(false),hasOldSubElement(false)
{
    initExtensionType(LinkBaseExtension::getExtensionClassTypeId());
    EXTENSION_ADD_PROPERTY_TYPE(_LinkTouched, (false), " Link", 
            PropertyType(Prop_Hidden|Prop_NoPersist),0);
    EXTENSION_ADD_PROPERTY_TYPE(_ChildCache, (), " Link", 
            PropertyType(Prop_Hidden|Prop_NoPersist|Prop_ReadOnly),0);
    _ChildCache.setScope(LinkScope::Global);
    props.resize(PropMax,0);
}

LinkBaseExtension::~LinkBaseExtension()
{
}

PyObject* LinkBaseExtension::getExtensionPyObject(void) {
    if (ExtensionPythonObject.is(Py::_None())){
        // ref counter is set to 1
        ExtensionPythonObject = Py::Object(new LinkBaseExtensionPy(this),true);
    }
    return Py::new_reference_to(ExtensionPythonObject);
}

const std::vector<LinkBaseExtension::PropInfo> &LinkBaseExtension::getPropertyInfo() const {
    static std::vector<LinkBaseExtension::PropInfo> PropsInfo;
    if(PropsInfo.empty()) {
        BOOST_PP_SEQ_FOR_EACH(LINK_PROP_INFO,PropsInfo,LINK_PARAMS);
    }
    return PropsInfo;
}

const LinkBaseExtension::PropInfoMap &LinkBaseExtension::getPropertyInfoMap() const {
    static PropInfoMap PropsMap;
    if(PropsMap.empty()) {
        const auto &infos = getPropertyInfo();
        for(const auto &info : infos) 
            PropsMap[info.name] = info;
    }
    return PropsMap;
}

Property *LinkBaseExtension::getProperty(int idx) {
    if(idx>=0 && idx<(int)props.size())
        return props[idx];
    return 0;
}

Property *LinkBaseExtension::getProperty(const char *name) {
    const auto &info = getPropertyInfoMap();
    auto it = info.find(name);
    if(it == info.end())
        return 0;
    return getProperty(it->second.index);
}

void LinkBaseExtension::setProperty(int idx, Property *prop) {
    const auto &infos = getPropertyInfo();
    if(idx<0 || idx>=(int)infos.size())
        LINK_THROW(Base::RuntimeError,"App::LinkBaseExtension: property index out of range");

    if(props[idx]) {
        props[idx]->setStatus(Property::LockDynamic,false);
        props[idx] = 0;
    }
    if(!prop) 
        return;
    if(!prop->isDerivedFrom(infos[idx].type)) {
        std::ostringstream str;
        str << "App::LinkBaseExtension: expected property type '" << 
            infos[idx].type.getName() << "', instead of '" << 
            prop->getClassTypeId().getName() << "'";
        LINK_THROW(Base::TypeError,str.str().c_str());
    }

    props[idx] = prop;
    props[idx]->setStatus(Property::LockDynamic,true);

    switch(idx) {
    case PropLinkMode: {
        static const char *linkModeEnums[] = {"None","Auto Delete","Auto Link","Auto Unlink",0};
        auto propLinkMode = freecad_dynamic_cast<PropertyEnumeration>(prop);
        if(!propLinkMode->getEnums())
            propLinkMode->setEnums(linkModeEnums);
        break;
    } case PropLinkTransform:
    case PropLinkPlacement:
    case PropPlacement:
        if(getLinkTransformProperty() &&
           getLinkPlacementProperty() &&
           getPlacementProperty())
        {
            bool transform = getLinkTransformValue();
            getPlacementProperty()->setStatus(Property::Hidden,transform);
            getLinkPlacementProperty()->setStatus(Property::Hidden,!transform);
        }
        break;
    case PropElementList:
        getElementListProperty()->setStatus(Property::Hidden,true);
        // fall through
    case PropLinkedObject:
        // Make ElementList as read-only if we are not a group (i.e. having
        // LinkedObject property), because it is for holding array elements.
        if(getElementListProperty())
            getElementListProperty()->setStatus(
                    Property::Immutable,getLinkedObjectProperty()!=0);
        break;
    case PropVisibilityList:
        getVisibilityListProperty()->setStatus(Property::Immutable,true);
        getVisibilityListProperty()->setStatus(Property::Hidden,true);
        break;
    }

    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_TRACE)) {
        const char *propName;
        if(!prop) 
            propName = "<null>";
        else if(prop->getContainer())
            propName = prop->getName();
        else 
            propName = extensionGetPropertyName(prop);
        if(!propName) 
            propName = "?";
        FC_TRACE("set property " << infos[idx].name << ": " << propName);
    }
}

App::DocumentObjectExecReturn *LinkBaseExtension::extensionExecute(void) {
    // The actual value of LinkRecompouted is not important, just to notify view
    // provider that the link (in fact, its dependents, i.e. linked ones) have
    // recomputed.
    _LinkTouched.touch();

    if(getLinkedObjectProperty() && !getTrueLinkedObject(true))
        return new App::DocumentObjectExecReturn("Link broken");
    return inherited::extensionExecute();
}

short LinkBaseExtension::extensionMustExecute(void) {
    auto link = getLink();
    if(!link) return 0;
    return link->mustExecute();
}

App::GroupExtension *LinkBaseExtension::linkedPlainGroup() const {
    if(mySubElements.size() && mySubElements[0].size())
        return 0;
    auto linked = getTrueLinkedObject(false);
    if(!linked)
        return 0;
    return linked->getExtensionByType<GroupExtension>(true,false);
}

App::PropertyLinkList *LinkBaseExtension::_getElementListProperty() const {
    auto group = linkedPlainGroup();
    if(group)
        return &group->Group;
    return const_cast<PropertyLinkList*>(getElementListProperty());
}

const std::vector<App::DocumentObject*> &LinkBaseExtension::_getElementListValue() const {
    if(_ChildCache.getSize())
        return _ChildCache.getValues();
    if(getElementListProperty())
        return getElementListProperty()->getValues();
    static const std::vector<DocumentObject*> empty;
    return empty;
}

App::PropertyBool *LinkBaseExtension::_getShowElementProperty() const {
    auto prop = getShowElementProperty();
    if(prop && !linkedPlainGroup())
        return const_cast<App::PropertyBool*>(prop);
    return 0;
}

bool LinkBaseExtension::_getShowElementValue() const {
    auto prop = _getShowElementProperty();
    if(prop)
        return prop->getValue();
    return true;
}

App::PropertyInteger *LinkBaseExtension::_getElementCountProperty() const {
    auto prop = getElementCountProperty();
    if(prop && !linkedPlainGroup())
        return const_cast<App::PropertyInteger*>(prop);
    return 0;
}

int LinkBaseExtension::_getElementCountValue() const {
    auto prop = _getElementCountProperty();
    if(prop)
        return prop->getValue();
    return 0;
}

bool LinkBaseExtension::extensionHasChildElement() const {
    if(_getElementListProperty() || _getElementCountValue())
        return true;
    DocumentObject *linked = getTrueLinkedObject(false);
    if(linked) {
        if(linked->hasChildElement())
            return true;
    }
    return false;
}

int LinkBaseExtension::extensionSetElementVisible(const char *element, bool visible) {
    int index = _getShowElementValue()?getElementIndex(element):getArrayIndex(element);
    if(index>=0) {
        auto propElementVis = getVisibilityListProperty();
        if(!propElementVis || !element || !element[0]) 
            return -1;
        if(propElementVis->getSize()<=index) {
            if(visible) return 1;
            propElementVis->setSize(index+1, true);
        }
        propElementVis->setStatus(Property::User3,true);
        propElementVis->set1Value(index,visible);
        propElementVis->setStatus(Property::User3,false);
        const auto &elements = _getElementListValue();
        if(index<(int)elements.size()) {
            if(!visible)
                myHiddenElements.insert(elements[index]);
            else
                myHiddenElements.erase(elements[index]);
        }
        return 1;
    }
    DocumentObject *linked = getTrueLinkedObject(false);
    if(linked)
        return linked->setElementVisible(element,visible);
    return -1;
}

int LinkBaseExtension::extensionIsElementVisible(const char *element) {
    int index = _getShowElementValue()?getElementIndex(element):getArrayIndex(element);
    if(index>=0) {
        auto propElementVis = getVisibilityListProperty();
        if(propElementVis) {
            if(propElementVis->getSize()<=index || propElementVis->getValues()[index])
                return 1;
            return 0;
        }
        return -1;
    }
    DocumentObject *linked = getTrueLinkedObject(false);
    if(linked)
        return linked->isElementVisible(element);
    return -1;
}

const DocumentObject *LinkBaseExtension::getContainer() const {
    auto ext = getExtendedContainer();
    if(!ext || !ext->isDerivedFrom(DocumentObject::getClassTypeId()))
        LINK_THROW(Base::RuntimeError,"Link: container not derived from document object");
    return static_cast<const DocumentObject *>(ext);
}

DocumentObject *LinkBaseExtension::getContainer(){
    auto ext = getExtendedContainer();
    if(!ext || !ext->isDerivedFrom(DocumentObject::getClassTypeId()))
        LINK_THROW(Base::RuntimeError,"Link: container not derived from document object");
    return static_cast<DocumentObject *>(ext);
}

DocumentObject *LinkBaseExtension::getLink(int depth) const{
    GetApplication().checkLinkDepth(depth,false);
    if(getLinkedObjectProperty())
        return getLinkedObjectValue();
    return 0;
}

int LinkBaseExtension::getArrayIndex(const char *subname, const char **psubname) {
    if(!subname || Data::ComplexGeoData::isMappedElement(subname)) 
        return -1;
    const char *dot = strchr(subname,'.');
    if(!dot) dot= subname+strlen(subname);
    if(dot == subname) return -1;
    int idx = 0;
    for(const char *c=subname;c!=dot;++c) {
        if(!isdigit(*c)) return -1;
        idx = idx*10 + *c -'0';
    }
    if(psubname) {
        if(*dot)
            *psubname = dot+1;
        else
            *psubname = dot;
    }
    return idx;
}

int LinkBaseExtension::getElementIndex(const char *subname, const char **psubname) const {
    if(!subname || Data::ComplexGeoData::isMappedElement(subname)) 
        return -1;
    int idx = -1;
    const char *dot = strchr(subname,'.');
    if(!dot) dot= subname+strlen(subname);

    if(isdigit(subname[0])) {
        // If the name start with digits, treat as index reference
        idx = getArrayIndex(subname,0);
        if(idx<0) return -1;
        if(_getElementCountProperty()) {
            if(idx>=_getElementCountValue())
                return -1;
        }else if(idx>=(int)_getElementListValue().size()) 
            return -1;
    }else if(!_getShowElementValue() && _getElementCountValue()) {
        // If elements are collapsed, we check first for LinkElement naming
        // pattern, which is the owner object name + "_i" + index
        const char *name = subname[0]=='$'?subname+1:subname;
        auto owner = getContainer();
        if(owner && owner->getNameInDocument()) {
            std::string ownerName(owner->getNameInDocument());
            ownerName += '_';
            if(boost::algorithm::starts_with(name,ownerName.c_str())) {
                for(const char *txt=dot-1;txt>=name+ownerName.size();--txt) {
                    if(*txt == 'i') {
                        idx = getArrayIndex(txt+1,0);
                        if(idx<0 || idx>=_getElementCountValue())
                            idx = -1;
                        break;
                    }
                    if(!isdigit(*txt))
                        break;
                }
            }
        }
        if(idx<0) {
            // Then check for the actual linked object's name or label, and
            // redirect that reference to the first array element
            auto linked = getTrueLinkedObject(false);
            if(!linked || !linked->getNameInDocument()) return -1;
            std::string sub(subname,dot-subname);
            if(subname[0]=='$') {
                if(strcmp(sub.c_str()+1,linked->Label.getValue())==0)
                    idx = 0;
            }else if(sub==linked->getNameInDocument())
                idx = 0;
            if(idx<0) {
                // Lastly, try to get sub object directly from the linked object
                auto sobj = linked->getSubObject(sub.c_str());
                if(!sobj)
                    return -1;
                if(psubname)
                    *psubname = subname;
                return 0;
            }
        }
    }else if(subname[0]!='$') {
        // Try search by element objects' name
        std::string name(subname,dot);
        if(_ChildCache.getSize()) {
            auto obj=_ChildCache.find(name,&idx);
            if(obj) {
                auto group = obj->getExtensionByType<GroupExtension>(true,false);
                if(group) {
                    int nidx = getElementIndex(dot+1,psubname);
                    if(nidx >= 0)
                        return nidx;
                }
            }
        } else if(getElementListProperty())
            getElementListProperty()->find(name.c_str(),&idx);
        if(idx<0)
            return -1;
    }else {
        // Try search by label if the reference name start with '$'
        ++subname;
        std::string name(subname,dot-subname);
        const auto &elements = _getElementListValue();
        if(enableLabelCache) {
            if(myLabelCache.empty())
                cacheChildLabel(1);
            auto it = myLabelCache.find(name);
            if(it == myLabelCache.end())
                return -1;
            idx = it->second;
        }else{
            idx = 0;
            for(auto element : elements) {
                if(element->Label.getStrValue() == name)
                    break;
                ++idx;
            }
        }
        if(idx<0 || idx>=(int)elements.size())
            return -1;
        auto obj = elements[idx];
        if(obj && _ChildCache.getSize()) {
            auto group = obj->getExtensionByType<GroupExtension>(true,false);
            if(group) {
                int nidx = getElementIndex(dot+1,psubname);
                if(nidx >= 0)
                    return nidx;
            }
        }
    }
    if(psubname)
        *psubname = dot[0]?dot+1:dot;
    return idx;
}

void LinkBaseExtension::elementNameFromIndex(int idx, std::ostream &ss) const {
    const auto &elements = _getElementListValue();
    if(idx < 0 || idx >= (int)elements.size())
        return;

    auto obj = elements[idx];
    if(_ChildCache.getSize()) {
        auto group = GroupExtension::getGroupOfObject(obj);
        if(group && _ChildCache.find(group->getNameInDocument(),&idx))
            elementNameFromIndex(idx,ss);
    }
    ss << obj->getNameInDocument() << '.';
}

Base::Vector3d LinkBaseExtension::getScaleVector() const {
    if(getScaleVectorProperty())
        return getScaleVectorValue();
    double s = getScaleValue();
    return Base::Vector3d(s,s,s);
}

Base::Matrix4D LinkBaseExtension::getTransform(bool transform) const {
    Base::Matrix4D mat;
    if(transform) {
        if(getLinkPlacementProperty())
            mat = getLinkPlacementValue().toMatrix();
        else if(getPlacementProperty())
            mat = getPlacementValue().toMatrix();
    }
    if(getScaleProperty() || getScaleVectorProperty()) {
        Base::Matrix4D s;
        s.scale(getScaleVector());
        mat *= s;
    }
    return mat;
}

bool LinkBaseExtension::extensionGetSubObjects(std::vector<std::string> &ret, int reason) const {
    if(!getLinkedObjectProperty() && getElementListProperty()) {
        for(auto obj : getElementListProperty()->getValues()) {
            if(obj && obj->getNameInDocument()) {
                std::string name(obj->getNameInDocument());
                name+='.';
                ret.push_back(name);
            }
        }
        return true;
    }
    if(mySubElements.empty() || mySubElements[0].empty()) {
        DocumentObject *linked = getTrueLinkedObject(true);
        if(linked) {
            if(!_getElementCountValue())
                ret = linked->getSubObjects(reason);
            else{
                char index[30];
                for(int i=0,count=_getElementCountValue();i<count;++i) {
                    snprintf(index,sizeof(index),"%d.",i);
                    ret.push_back(index);
                }
            }
        }
    } else if(mySubElements.size()>1) {
        ret = mySubElements;
    }
    return true;
}

bool LinkBaseExtension::extensionGetSubObject(DocumentObject *&ret, const char *subname, 
        PyObject **pyObj, Base::Matrix4D *mat, bool transform, int depth) const 
{
    ret = 0;
    auto obj = getContainer();
    if(!subname || !subname[0]) {
        ret = const_cast<DocumentObject*>(obj);
        Base::Matrix4D _mat;
        if(mat) {
            // 'mat' here is used as an output to return the accumulated
            // transformation up until this object. Since 'subname' is empty
            // here, it means the we are at the end of the hierarchy. We shall
            // not include scale in the output transformation.
            //
            // Think of it this way, the transformation along object hierarchy
            // is public, while transformation through linkage is private to
            // link itself.
            if(transform) {
                if(getLinkPlacementProperty())
                    *mat *= getLinkPlacementValue().toMatrix();
                else if(getPlacementProperty())
                    *mat *= getPlacementValue().toMatrix();
            }
            _mat = *mat;
        }

        if(pyObj && !_getElementCountValue() 
                && _getElementListValue().empty() && mySubElements.size()<=1) 
        {
            // Scale will be included here
            if(getScaleProperty() || getScaleVectorProperty()) {
                Base::Matrix4D s;
                s.scale(getScaleVector());
                _mat *= s;
            }
            auto linked = getTrueLinkedObject(false,&_mat,depth);
            if(linked && linked!=obj) {
                linked->getSubObject(mySubElements.empty()?0:mySubElements.front().c_str(),
                                     pyObj,&_mat,false,depth+1);
                checkGeoElementMap(obj,linked,pyObj,0);
            }
        }
        return true;
    }

    if(mat) *mat *= getTransform(transform);

    DocumentObject *element = 0;
    bool isElement = false;
    int idx = getElementIndex(subname,&subname);
    if(idx>=0) {
        const auto &elements = _getElementListValue();
        if(elements.size()) {
            if(idx>=(int)elements.size() || !elements[idx] || !elements[idx]->getNameInDocument())
                return true;
            ret = elements[idx]->getSubObject(subname,pyObj,mat,true,depth+1);
            // do not resolve the link if this element is the last referenced object
            if(!subname || Data::ComplexGeoData::isMappedElement(subname) || !strchr(subname,'.'))
                ret = elements[idx];
            return true;
        }

        int elementCount = _getElementCountValue();
        if(idx>=elementCount)
            return true;
        isElement = true;
        if(mat) {
            auto placementList = getPlacementListProperty();
            if(placementList && placementList->getSize()>idx)
                *mat *= (*placementList)[idx].toMatrix();
            auto scaleList = getScaleListProperty();
            if(scaleList && scaleList->getSize()>idx) {
                Base::Matrix4D s;
                s.scale((*scaleList)[idx]);
                *mat *= s;
            }
        }
    }

    auto linked = getTrueLinkedObject(false,mat,depth);
    if(!linked || linked==obj) 
        return true;

    Base::Matrix4D matNext;
    if(mat) matNext = *mat;
    ret = linked->getSubObject(subname,pyObj,mat?&matNext:0,false,depth+1);
    std::string postfix;
    if(ret) {
        // do not resolve the link if we are the last referenced object
        if(subname && !Data::ComplexGeoData::isMappedElement(subname) && strchr(subname,'.')) {
            if(mat)
                *mat = matNext;
        }else if(element)
            ret = element;
        else if(!isElement) {
            ret = const_cast<DocumentObject*>(obj);
        } else {
            if(idx) {
                postfix = Data::ComplexGeoData::indexPostfix();
                postfix += std::to_string(idx);
            }
            if(mat)
                *mat = matNext;
        }
    }
    checkGeoElementMap(obj,linked,pyObj,postfix.size()?postfix.c_str():0);
    return true;
}

void LinkBaseExtension::checkGeoElementMap(const App::DocumentObject *obj, 
        const App::DocumentObject *linked, PyObject **pyObj, const char *postfix) const
{
    if(!pyObj || !*pyObj || (!postfix && obj->getDocument()==linked->getDocument()) ||
       !PyObject_TypeCheck(*pyObj, &Data::ComplexGeoDataPy::Type))
        return;
       
    // auto geoData = static_cast<Data::ComplexGeoDataPy*>(*pyObj)->getComplexGeoDataPtr();
    // geoData->reTagElementMap(obj->getID(),obj->getDocument()->Hasher,postfix);
}

void LinkBaseExtension::onExtendedUnsetupObject() {
    if(!getElementListProperty())
        return;
    auto objs = getElementListValue();
    getElementListProperty()->setValue();
    for(auto obj : objs) 
        detachElement(obj);
}

DocumentObject *LinkBaseExtension::getTrueLinkedObject(
        bool recurse, Base::Matrix4D *mat, int depth, bool noElement) const
{
    if(noElement && extensionIsDerivedFrom(LinkElement::getExtensionClassTypeId()) 
            && !static_cast<const LinkElement*>(this)->canDelete())
    {
        return 0;
    }

    auto ret = getLink(depth);
    if(!ret) return 0;
    bool transform = linkTransform();
    const char *subname = getSubName();
    if(subname) {
        ret = ret->getSubObject(subname,0,mat,transform,depth+1);
        transform = false;
    }
    if(ret && recurse)
        ret = ret->getLinkedObject(recurse,mat,transform,depth+1);
    if(ret && !ret->getNameInDocument())
        return 0;
    return ret;
}

bool LinkBaseExtension::extensionGetLinkedObject(DocumentObject *&ret, 
        bool recurse, Base::Matrix4D *mat, bool transform, int depth) const
{
    if(mat) 
        *mat *= getTransform(transform);
    ret = 0;
    if(!_getElementCountValue())
        ret = getTrueLinkedObject(recurse,mat,depth);
    if(!ret)
        ret = const_cast<DocumentObject*>(getContainer());
    // always return true to indicate we've handled getLinkObject() call
    return true;
}

void LinkBaseExtension::extensionOnChanged(const Property *prop) {
    auto parent = getContainer();
    if(parent && !parent->isRestoring() && prop && !prop->testStatus(Property::User3))
        update(parent,prop);
    inherited::extensionOnChanged(prop);
}

void LinkBaseExtension::parseSubName() const {
    // If user has ever linked to some sub-element, the Link shall always accept
    // sub-element linking in the future, which affects how ViewProviderLink
    // dropObjectEx() behave. So we will push an empty string later even if no
    // sub-element linking this time.
    bool hasSubElement = !mySubElements.empty();
    mySubElements.clear();
    mySubName.clear();
    auto xlink = freecad_dynamic_cast<const PropertyXLink>(getLinkedObjectProperty());
    if(!xlink || xlink->getSubValues().empty()) {
        if(hasSubElement)
            mySubElements.emplace_back("");
        return;
    }
    const auto &subs = xlink->getSubValues();
    auto subname = subs.front().c_str();
    auto element = Data::ComplexGeoData::findElementName(subname);
    if(!element || !element[0]) {
        mySubName = subs[0];
        if(hasSubElement)
            mySubElements.emplace_back("");
        return;
    }
    mySubElements.push_back(element);
    mySubName = std::string(subname,element-subname);
    for(std::size_t i=1;i<subs.size();++i) {
        auto &sub = subs[i];
        element = Data::ComplexGeoData::findElementName(sub.c_str());
        if(element && element[0] && boost::starts_with(sub,mySubName))
            mySubElements.push_back(element);
    }
}

void LinkBaseExtension::slotChangedPlainGroup(const App::DocumentObject &obj, const App::Property &prop) {
    auto group = obj.getExtensionByType<GroupExtension>(true,false);
    if(group && &prop == &group->Group)
        updateGroup();
}

void LinkBaseExtension::updateGroup() {
    std::vector<GroupExtension*> groups;
    std::unordered_set<const App::DocumentObject*> groupSet;
    auto group = linkedPlainGroup();
    if(group) {
        groups.push_back(group);
        groupSet.insert(group->getExtendedObject());
    }else{
        for(auto o : getElementListProperty()->getValues()) {
            if(!o || !o->getNameInDocument())
                continue;
            auto ext = o->getExtensionByType<GroupExtension>(true,false);
            if(ext) {
                groups.push_back(ext);
                groupSet.insert(o);
            }
        }
    }
    std::vector<App::DocumentObject*> children;
    if(groups.size()) {
        children = getElementListValue();
        std::set<DocumentObject*> childSet(children.begin(),children.end());
        for(auto ext : groups) {
            auto group = ext->getExtendedObject();
            auto &conn = plainGroupConns[group];
            if(!conn.connected()) {
                FC_LOG("new group connection " << getExtendedObject()->getFullName() 
                        << " -> " << group->getFullName());
                conn = group->signalChanged.connect(
                        boost::bind(&LinkBaseExtension::slotChangedPlainGroup,this,_1,_2));
            }
            std::size_t count = children.size();
            ext->getAllChildren(children,childSet);
            for(;count<children.size();++count) {
                auto child = children[count];
                if(!child->getExtensionByType<GroupExtension>(true,false))
                    continue;
                groupSet.insert(child);
                auto &conn = plainGroupConns[child];
                if(!conn.connected()) {
                    FC_LOG("new group connection " << getExtendedObject()->getFullName() 
                            << " -> " << child->getFullName());
                    conn = child->signalChanged.connect(
                            boost::bind(&LinkBaseExtension::slotChangedPlainGroup,this,_1,_2));
                }
            }
        }
    }
    for(auto it=plainGroupConns.begin();it!=plainGroupConns.end();) {
        if(!groupSet.count(it->first))
            it = plainGroupConns.erase(it);
        else
            ++it;
    }
    if(children != _ChildCache.getValues())
        _ChildCache.setValue(children);
}

void LinkBaseExtension::update(App::DocumentObject *parent, const Property *prop) {
    if(!prop) return;

    if(prop == getLinkPlacementProperty() || prop == getPlacementProperty()) {
        auto src = getLinkPlacementProperty();
        auto dst = getPlacementProperty();
        if(src!=prop) std::swap(src,dst);
        if(src && dst) {
            dst->setStatus(Property::User3,true);
            dst->setValue(src->getValue());
            dst->setStatus(Property::User3,false);
        }
    }else if(prop == getScaleProperty()) {
        if(!prop->testStatus(Property::User3) && getScaleVectorProperty()) {
            auto s = getScaleValue();
            auto p = getScaleVectorProperty();
            p->setStatus(Property::User3,true);
            p->setValue(s,s,s);
            p->setStatus(Property::User3,false);
        }
    }else if(prop == getScaleVectorProperty()) {
        if(!prop->testStatus(Property::User3) && getScaleProperty()) {
            const auto &v = getScaleVectorValue();
            if(v.x == v.y && v.x == v.z) {
                auto p = getScaleProperty();
                p->setStatus(Property::User3,true);
                p->setValue(v.x);
                p->setStatus(Property::User3,false);
            }
        }
    }else if(prop == _getShowElementProperty()) {
        if(_getShowElementValue()) 
            update(parent,_getElementCountProperty());
        else {
            auto objs = getElementListValue();

            // preserve element properties in ourself
            std::vector<Base::Placement> placements;
            placements.reserve(objs.size());
            std::vector<Base::Vector3d> scales;
            scales.reserve(objs.size());
            for(size_t i=0;i<objs.size();++i) {
                auto element = freecad_dynamic_cast<LinkElement>(objs[i]);
                if(element) {
                    placements.push_back(element->Placement.getValue());
                    scales.push_back(element->getScaleVector());
                }else{
                    placements.emplace_back();
                    scales.emplace_back(1,1,1);
                }
            }
            // touch the property again to make sure view provider has been
            // signaled before clearing the elements
            getShowElementProperty()->setStatus(App::Property::User3,true);
            getShowElementProperty()->touch();
            getShowElementProperty()->setStatus(App::Property::User3,false);

            getElementListProperty()->setValues(std::vector<App::DocumentObject*>());

            if(getPlacementListProperty()) {
                getPlacementListProperty()->setStatus(Property::User3,getScaleListProperty()!=0);
                getPlacementListProperty()->setValue(placements);
                getPlacementListProperty()->setStatus(Property::User3,false);
            }
            if(getScaleListProperty())
                getScaleListProperty()->setValue(scales);

            for(auto obj : objs) {
                if(obj && obj->getNameInDocument())
                    obj->getDocument()->removeObject(obj->getNameInDocument());
            }
        }
    }else if(prop == _getElementCountProperty()) {
        size_t elementCount = getElementCountValue()<0?0:(size_t)getElementCountValue();

        auto propVis = getVisibilityListProperty();
        if(propVis) {
            if(propVis->getSize()>(int)elementCount)
                propVis->setSize(getElementCountValue(),true);
        }

        if(!_getShowElementValue()) {
            if(getScaleListProperty()) {
                auto scales = getScaleListValue();
                scales.resize(elementCount,Base::Vector3d(1,1,1));
                getScaleListProperty()->setStatus(Property::User3,true);
                getScaleListProperty()->setValue(scales);
                getScaleListProperty()->setStatus(Property::User3,false);
            }
            if(getPlacementListProperty()) {
                auto placements = getPlacementListValue();
                if(placements.size()<elementCount) {
                    for(size_t i=placements.size();i<elementCount;++i)
                        placements.emplace_back(Base::Vector3d(i%10,(i/10)%10,i/100),Base::Rotation());
                }else
                    placements.resize(elementCount);
                getPlacementListProperty()->setStatus(Property::User3,true);
                getPlacementListProperty()->setValue(placements);
                getPlacementListProperty()->setStatus(Property::User3,false);
            }
        }else if(getElementListProperty()) {
            auto objs = getElementListValue();
            if(elementCount>objs.size()) {
                std::string name = parent->getNameInDocument();
                auto doc = parent->getDocument();
                name += "_i";
                name = doc->getUniqueObjectName(name.c_str());
                if(name[name.size()-1] != 'i')
                    name += "_i";
                auto offset = name.size();
                auto placementProp = getPlacementListProperty();
                auto scaleProp = getScaleListProperty();
                const auto &vis = getVisibilityListValue();
                auto proxy = freecad_dynamic_cast<PropertyPythonObject>(parent->getPropertyByName("Proxy"));
                Py::Callable method;
                Py::Tuple args(3);
                if(proxy) {
                    Py::Object proxyValue = proxy->getValue();
                    const char *fname = "onCreateLinkElement";
                    if (proxyValue.hasAttr(fname)) {
                        method = proxyValue.getAttr(fname);
                        args.setItem(0,Py::Object(parent->getPyObject(),true));
                    }
                }

                auto owner = getContainer();
                long ownerID = owner?owner->getID():0;

                for(size_t i=objs.size();i<elementCount;++i) {
                    name.resize(offset);
                    name += std::to_string(i);

                    // It is possible to have orphan LinkElement here due to,
                    // for example, undo and redo. So we try to re-claim the
                    // children element first.
                    auto obj = freecad_dynamic_cast<LinkElement>(doc->getObject(name.c_str()));
                    if(obj && (!obj->myOwner || obj->myOwner==ownerID))
                        obj->Visibility.setValue(false);
                    else {
                        if(!method.isNone()) {
                            obj = new LinkElementPython;
                            args.setItem(1,Py::Object(obj->getPyObject(),true));
                            args.setItem(2,Py::Int((int)i));
                            method.apply(args);
                        } else
                            obj = new LinkElement;
                        parent->getDocument()->addObject(obj,name.c_str());
                    }

                    if(vis.size()>i && !vis[i])
                        myHiddenElements.insert(obj);

                    if(placementProp && placementProp->getSize()>(int)i)
                        obj->Placement.setValue(placementProp->getValues()[i]);
                    else{
                        Base::Placement pla(Base::Vector3d(i%10,(i/10)%10,i/100),Base::Rotation());
                        obj->Placement.setValue(pla);
                    }
                    if(scaleProp && scaleProp->getSize()>(int)i)
                        obj->Scale.setValue(scaleProp->getValues()[i].x);
                    else
                        obj->Scale.setValue(1);
                    objs.push_back(obj);
                }
                if(getPlacementListProperty()) 
                    getPlacementListProperty()->setSize(0);
                if(getScaleListProperty())
                    getScaleListProperty()->setSize(0);

                getElementListProperty()->setValue(objs);

            }else if(elementCount<objs.size()){
                std::vector<App::DocumentObject*> tmpObjs;
                auto owner = getContainer();
                long ownerID = owner?owner->getID():0;
                while(objs.size()>elementCount) {
                    auto element = freecad_dynamic_cast<LinkElement>(objs.back());
                    if(element && element->myOwner==ownerID)
                        tmpObjs.push_back(objs.back());
                    objs.pop_back();
                }
                getElementListProperty()->setValue(objs);
                for(auto obj : tmpObjs) {
                    if(obj && obj->getNameInDocument())
                        obj->getDocument()->removeObject(obj->getNameInDocument());
                }
            }
        }
    }else if(prop == getVisibilityListProperty()) {
        if(_getShowElementValue()) {
            const auto &elements = _getElementListValue();
            const auto &vis = getVisibilityListValue();
            myHiddenElements.clear();
            for(size_t i=0;i<vis.size();++i) {
                if(i>=elements.size())
                    break;
                if(!vis[i])
                    myHiddenElements.insert(elements[i]);
            }
        }
    }else if(prop == getElementListProperty() || prop == &_ChildCache) {

        if(prop == getElementListProperty()) {
            _ChildCache.setStatus(Property::User3,true);
            updateGroup();
            _ChildCache.setStatus(Property::User3,false);
        }

        const auto &elements = _getElementListValue();

        if(enableLabelCache)
            myLabelCache.clear();

        // Element list changed, we need to sychrnoize VisibilityList.
        if(_getShowElementValue() && getVisibilityListProperty()) {
            if(parent->getDocument()->isPerformingTransaction()) {
                update(parent,getVisibilityListProperty());
            }else{
                boost::dynamic_bitset<> vis;
                vis.resize(elements.size(),true);
                std::unordered_set<const App::DocumentObject *> hiddenElements;
                for(size_t i=0;i<elements.size();++i) {
                    if(myHiddenElements.find(elements[i])!=myHiddenElements.end()) {
                        hiddenElements.insert(elements[i]);
                        vis[i] = false;
                    }
                }
                myHiddenElements.swap(hiddenElements);
                if(vis != getVisibilityListValue()) {
                    auto propVis = getVisibilityListProperty();
                    propVis->setStatus(Property::User3,true);
                    propVis->setValue(vis);
                    propVis->setStatus(Property::User3,false);
                }
            }
        }
        syncElementList();
        if(_getShowElementValue() 
                && _getElementCountProperty() 
                && getElementListProperty() 
                && getElementCountValue()!=getElementListProperty()->getSize())
        {
            getElementCountProperty()->setValue(
                    getElementListProperty()->getSize());
        }
    }else if(prop == getLinkedObjectProperty()) {
        auto group = linkedPlainGroup();
        if(getShowElementProperty())
            getShowElementProperty()->setStatus(Property::Hidden, !!group);
        if(getElementCountProperty())
            getElementCountProperty()->setStatus(Property::Hidden, !!group);
        if(group)
            updateGroup();
        else if(_ChildCache.getSize())
            _ChildCache.setValue();
        parseSubName();
        syncElementList();
    }else if(prop == getLinkTransformProperty()) {
        auto linkPlacement = getLinkPlacementProperty();
        auto placement = getPlacementProperty();
        if(linkPlacement && placement) {
            bool transform = getLinkTransformValue();
            placement->setStatus(Property::Hidden,transform);
            linkPlacement->setStatus(Property::Hidden,!transform);
        }
        syncElementList();
    }
}

void LinkBaseExtension::cacheChildLabel(int enable) const {
    enableLabelCache = enable?true:false;
    myLabelCache.clear();
    if(enable<=0)
        return;

    int idx = 0;
    for(auto child : _getElementListValue()) {
        if(child && child->getNameInDocument())
            myLabelCache[child->Label.getStrValue()] = idx;
        ++idx;
    }
}

bool LinkBaseExtension::linkTransform() const {
    if(!getLinkTransformProperty() && 
       !getLinkPlacementProperty() && 
       !getPlacementProperty())
        return true;
    return getLinkTransformValue();
}

void LinkBaseExtension::syncElementList() {
    auto transform = getLinkTransformProperty();
    auto link = getLinkedObjectProperty();
    auto xlink = freecad_dynamic_cast<const PropertyXLink>(link);

    auto owner = getContainer();
    auto ownerID = owner?owner->getID():0;
    auto elements = getElementListValue();
    for(size_t i=0;i<elements.size();++i) {
        auto element = freecad_dynamic_cast<LinkElement>(elements[i]);
        if(!element || (element->myOwner && element->myOwner!=ownerID)) 
            continue;

        element->myOwner = ownerID;

        element->LinkTransform.setStatus(Property::Hidden,transform!=0);
        element->LinkTransform.setStatus(Property::Immutable,transform!=0);
        if(transform && element->LinkTransform.getValue()!=transform->getValue())
            element->LinkTransform.setValue(transform->getValue());

        element->LinkedObject.setStatus(Property::Hidden,link!=0);
        element->LinkedObject.setStatus(Property::Immutable,link!=0);
        if(xlink) {
            if(element->LinkedObject.getValue()!=xlink->getValue() ||
               element->LinkedObject.getSubValues()!=xlink->getSubValues())
            {
                element->LinkedObject.setValue(xlink->getValue(), xlink->getSubValues());
            }
        } else if(element->LinkedObject.getValue()!=link->getValue() ||
                  element->LinkedObject.getSubValues().size())
        {
            element->setLink(-1,link->getValue());
        }
    }
}

void LinkBaseExtension::onExtendedDocumentRestored() {
    inherited::onExtendedDocumentRestored();
    myHiddenElements.clear();
    auto parent = getContainer();
    if(!parent)
        return;
    if(hasOldSubElement) {
        hasOldSubElement = false;
        // SubElements was stored as a PropertyStringList. It is now migrated to be
        // stored inside PropertyXLink.
        auto xlink = freecad_dynamic_cast<PropertyXLink>(getLinkedObjectProperty());
        if(!xlink) 
            FC_ERR("Failed to restore SubElements for " << parent->getFullName());
        else if(!xlink->getValue())
            FC_ERR("Discard SubElements of " << parent->getFullName() << " due to null link");
        else if(xlink->getSubValues().size() > 1)
            FC_ERR("Failed to restore SubElements for " << parent->getFullName() 
                    << " due to conflict subnames");
        else if(xlink->getSubValues().empty()) {
            auto subs = xlink->getSubValues();
            xlink->setSubValues(std::move(subs));
        } else {
            std::set<std::string> subset(mySubElements.begin(),mySubElements.end());
            auto sub = xlink->getSubValues().front();
            auto element = Data::ComplexGeoData::findElementName(sub.c_str());
            if(element && element[0]) {
                subset.insert(element);
                sub.resize(element - sub.c_str());
            }
            std::vector<std::string> subs;
            for(auto &s : subset)
                subs.push_back(sub + s);
            xlink->setSubValues(std::move(subs));
        }
    }
    if(getScaleVectorProperty() && getScaleProperty()) {
        // Scale vector is added later. The code here is for migration.
        const auto &v = getScaleVectorValue();
        double s = getScaleValue();
        if(v.x == v.y && v.x == v.z && v.x != s)
            getScaleVectorProperty()->setValue(s,s,s);
    }
    update(parent,getVisibilityListProperty());
    update(parent,getLinkedObjectProperty());
    update(parent,getElementListProperty());
}

void LinkBaseExtension::_handleChangedPropertyName(
        Base::XMLReader &reader, const char * TypeName, const char *PropName)
{
    if(strcmp(PropName,"SubElements")==0
        && strcmp(TypeName,PropertyStringList::getClassTypeId().getName())==0)
    {
        PropertyStringList prop;
        prop.setContainer(getContainer());
        prop.Restore(reader);
        if(prop.getSize()) {
            mySubElements = prop.getValues();
            hasOldSubElement = true;
        }
    }
}

void LinkBaseExtension::setLink(int index, DocumentObject *obj, 
    const char *subname, const std::vector<std::string> &subElements) 
{
    auto parent = getContainer();
    if(!parent)
        LINK_THROW(Base::RuntimeError,"No parent container");

    if(obj && !App::Document::isAnyRestoring()) {
        auto inSet = parent->getInListEx(true);
        inSet.insert(parent);
        if(inSet.find(obj)!=inSet.end())
            LINK_THROW(Base::RuntimeError,"Cyclic dependency");
    }

    auto linkProp = getLinkedObjectProperty();

    // If we are a group (i.e. no LinkObject property), and the index is
    // negative with a non-zero 'obj' assignment, we treat this as group
    // expansion by changing the index to one pass the existing group size
    if(index<0 && obj && !linkProp && getElementListProperty())
        index = getElementListProperty()->getSize();

    if(index>=0) {
        // LinkGroup assignment

        if(linkProp || !getElementListProperty())
            LINK_THROW(Base::RuntimeError,"Cannot set link element");

        DocumentObject *old = 0;
        const auto &elements = getElementListProperty()->getValues();
        if(!obj) {
            if(index>=(int)elements.size())
                LINK_THROW(Base::ValueError,"Link element index out of bound");
            std::vector<DocumentObject*> objs;
            old = elements[index];
            for(int i=0;i<(int)elements.size();++i) {
                if(i!=index)
                    objs.push_back(elements[i]);
            }
            getElementListProperty()->setValue(objs);
        }else if(!obj->getNameInDocument())
            LINK_THROW(Base::ValueError,"Invalid object");
        else{
            if(index>(int)elements.size())
                LINK_THROW(Base::ValueError,"Link element index out of bound");

            if(index < (int)elements.size())
                old = elements[index];

            int idx = -1;
            if(getLinkModeValue()>=LinkModeAutoLink ||
               (subname && subname[0]) ||
               subElements.size() ||
               obj->getDocument()!=parent->getDocument() ||
               (getElementListProperty()->find(obj->getNameInDocument(),&idx) && idx!=index))
            {
                std::string name = parent->getDocument()->getUniqueObjectName("Link");
                auto link = new Link;
                link->myOwner = parent->getID();
                parent->getDocument()->addObject(link,name.c_str());
                link->setLink(-1,obj,subname,subElements);
                auto linked = link->getTrueLinkedObject(true);
                if(linked)
                    link->Label.setValue(linked->Label.getValue());
                auto pla = freecad_dynamic_cast<PropertyPlacement>(obj->getPropertyByName("Placement"));
                if(pla)
                    link->Placement.setValue(pla->getValue());
                link->Visibility.setValue(false);
                obj = link;
            }

            if(old == obj)
                return;

            getElementListProperty()->set1Value(index,obj);
        }
        detachElement(old);
        return;
    }

    if(!linkProp) {
        // Reaching here means, we are group (i.e. no LinkedObject), and
        // index<0, and 'obj' is zero. We shall clear the whole group

        if(obj || !getElementListProperty())
            LINK_THROW(Base::RuntimeError,"No PropertyLink or PropertyLinkList configured");

        auto objs = getElementListValue();
        getElementListProperty()->setValue();
        for(auto obj : objs) 
            detachElement(obj);
        return;
    }

    // Here means we are assigning a Link

    auto xlink = freecad_dynamic_cast<PropertyXLink>(linkProp);
    if(obj) {
        if(!obj->getNameInDocument())
            LINK_THROW(Base::ValueError,"Invalid document object");
        if(!xlink) {
            if(parent && obj->getDocument()!=parent->getDocument())
                LINK_THROW(Base::ValueError,"Cannot link to external object without PropertyXLink");
        }
    }

    if(!xlink) {
        if(subElements.size() || (subname && subname[0]))
            LINK_THROW(Base::RuntimeError,"SubName/SubElement link requires PropertyXLink");
        linkProp->setValue(obj);
        return;
    }

    std::vector<std::string> subs;
    if(subElements.size()) {
        subs.reserve(subElements.size());
        for(const auto &s : subElements) {
            subs.emplace_back(subname?subname:"");
            subs.back() += s;
        }
    } else if(subname && subname[0])
        subs.emplace_back(subname);
    xlink->setValue(obj,std::move(subs));
}

void LinkBaseExtension::detachElement(DocumentObject *obj) {
    if(!obj || !obj->getNameInDocument() || obj->isRemoving())
        return;
    auto ext = obj->getExtensionByType<LinkBaseExtension>(true);
    auto owner = getContainer();
    long ownerID = owner?owner->getID():0;
    if(getLinkModeValue()==LinkModeAutoUnlink) {
        if(!ext || ext->myOwner!=ownerID)
            return;
    }else if(getLinkModeValue()!=LinkModeAutoDelete) {
        if(ext && ext->myOwner==ownerID)
            ext->myOwner = 0;
        return;
    }
    obj->getDocument()->removeObject(obj->getNameInDocument());
}

std::vector<App::DocumentObject*> LinkBaseExtension::getLinkedChildren(bool filter) const{
    if(!filter)
        return _getElementListValue();
    std::vector<App::DocumentObject*> ret;
    for(auto o : _getElementListValue()) {
        if(!o->hasExtension(GroupExtension::getExtensionClassTypeId(),false))
            ret.push_back(o);
    }
    return ret;
}

const char *LinkBaseExtension::flattenSubname(const char *subname) const {
    if(subname && _ChildCache.getSize()) {
        const char *sub = subname;
        std::string s;
        for(const char* dot=strchr(sub,'.');dot;sub=dot+1,dot=strchr(sub,'.')) {
            DocumentObject *obj = 0;
            s.clear();
            s.append(sub,dot+1);
            extensionGetSubObject(obj,s.c_str());
            if(!obj)
                break;
            if(!obj->hasExtension(GroupExtension::getExtensionClassTypeId(),false))
                return sub;
        }
    }
    return subname;
}

void LinkBaseExtension::expandSubname(std::string &subname) const {
    if(!_ChildCache.getSize())
        return;

    const char *pos = 0;
    int index = getElementIndex(subname.c_str(),&pos);
    if(index<0)
        return;
    std::ostringstream ss;
    elementNameFromIndex(index,ss);
    ss << pos;
    subname = ss.str();
}

static bool isExcludedProperties(const char *name) {
#define CHECK_EXCLUDE_PROP(_name) if(strcmp(name,#_name)==0) return true;
    CHECK_EXCLUDE_PROP(Shape);
    CHECK_EXCLUDE_PROP(Proxy);
    CHECK_EXCLUDE_PROP(Placement);
    return false;
}

Property *LinkBaseExtension::extensionGetPropertyByName(const char* name) const {
    auto prop = inherited::extensionGetPropertyByName(name);
    if(prop || isExcludedProperties(name))
        return prop;
    auto owner = getContainer();
    if(owner && owner->canLinkProperties()) {
        auto linked = getTrueLinkedObject(true);
        if(linked)
            return linked->getPropertyByName(name);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////

namespace App {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(App::LinkBaseExtensionPython, App::LinkBaseExtension)

// explicit template instantiation
template class AppExport ExtensionPythonT<LinkBaseExtension>;

}

//////////////////////////////////////////////////////////////////////////////

EXTENSION_PROPERTY_SOURCE(App::LinkExtension, App::LinkBaseExtension)

LinkExtension::LinkExtension(void)
{
    initExtensionType(LinkExtension::getExtensionClassTypeId());

    LINK_PROPS_ADD_EXTENSION(LINK_PARAMS_EXT);
}

LinkExtension::~LinkExtension()
{
}

///////////////////////////////////////////////////////////////////////////////////////////

namespace App {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(App::LinkExtensionPython, App::LinkExtension)

// explicit template instantiation
template class AppExport ExtensionPythonT<App::LinkExtension>;

}

///////////////////////////////////////////////////////////////////////////////////////////

PROPERTY_SOURCE_WITH_EXTENSIONS(App::Link, App::DocumentObject)

Link::Link() {
    LINK_PROPS_ADD(LINK_PARAMS_LINK);
    LinkExtension::initExtension(this);
    static const PropertyIntegerConstraint::Constraints s_constraints = {0,INT_MAX,1};
    ElementCount.setConstraints(&s_constraints);
}

bool Link::canLinkProperties() const {
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

namespace App {
PROPERTY_SOURCE_TEMPLATE(App::LinkPython, App::Link)
template<> const char* App::LinkPython::getViewProviderName(void) const {
    return "Gui::ViewProviderLinkPython";
}
template class AppExport FeaturePythonT<App::Link>;
}

//////////////////////////////////////////////////////////////////////////////////////////

PROPERTY_SOURCE_WITH_EXTENSIONS(App::LinkElement, App::DocumentObject)

LinkElement::LinkElement() {
    LINK_PROPS_ADD(LINK_PARAMS_ELEMENT);
    LinkBaseExtension::initExtension(this);
}

bool LinkElement::canDelete() const {
    if(!myOwner)
        return true;

    auto owner = getContainer();
    return !owner || !owner->getDocument()->getObjectByID(myOwner);
}

//////////////////////////////////////////////////////////////////////////////////////////

namespace App {
PROPERTY_SOURCE_TEMPLATE(App::LinkElementPython, App::LinkElement)
template<> const char* App::LinkElementPython::getViewProviderName(void) const {
    return "Gui::ViewProviderLinkPython";
}
template class AppExport FeaturePythonT<App::LinkElement>;
}

//////////////////////////////////////////////////////////////////////////////////////////

PROPERTY_SOURCE_WITH_EXTENSIONS(App::LinkGroup, App::DocumentObject)

LinkGroup::LinkGroup() {
    LINK_PROPS_ADD(LINK_PARAMS_GROUP);
    LinkBaseExtension::initExtension(this);
}

//////////////////////////////////////////////////////////////////////////////////////////

namespace App {
PROPERTY_SOURCE_TEMPLATE(App::LinkGroupPython, App::LinkGroup)
template<> const char* App::LinkGroupPython::getViewProviderName(void) const {
    return "Gui::ViewProviderLinkPython";
}
template class AppExport FeaturePythonT<App::LinkGroup>;
}

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

