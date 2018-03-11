/***************************************************************************
 *   Copyright (c) Juergen Riegel          (juergen.riegel@web.de) 2002    *
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
#endif

#include <Base/Console.h>
#include "Document.h"
#include "GeoFeature.h"
#include "GeoFeatureGroupExtension.h"
#include <App/GeoFeaturePy.h>

FC_LOG_LEVEL_INIT("GeoFeature",true,true);

using namespace App;


PROPERTY_SOURCE(App::GeoFeature, App::DocumentObject)


//===========================================================================
// Feature
//===========================================================================

GeoFeature::GeoFeature(void)
{
    ADD_PROPERTY(Placement,(Base::Placement()));
}

GeoFeature::~GeoFeature(void)
{
}

void GeoFeature::transformPlacement(const Base::Placement &transform)
{
    Base::Placement plm = this->Placement.getValue();
    plm = transform * plm;
    this->Placement.setValue(plm);
}

Base::Placement GeoFeature::globalPlacement() const
{
    auto* group = GeoFeatureGroupExtension::getGroupOfObject(this);
    if(group) {
        auto ext = group->getExtensionByType<GeoFeatureGroupExtension>();
        return ext->globalGroupPlacement() * Placement.getValue();
    }
    return Placement.getValue();    
}

const PropertyComplexGeoData* GeoFeature::getPropertyOfGeometry() const
{
    return nullptr;
}

PyObject* GeoFeature::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new GeoFeaturePy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}


std::pair<std::string,std::string> GeoFeature::getElementName(
        const char *name, ElementNameType type) const
{
    (void)type;

    std::pair<std::string,std::string> ret;
    if(!name) return ret;

    auto prop = getPropertyOfGeometry();
    if(!prop) return ret;

    auto geo = prop->getComplexData();
    if(!geo) return ret;

    if(Data::ComplexGeoData::isMappedElement(name)) {
        const char *oldName = geo->getElementName(name);
        const char *dot = strrchr(name,'.');
        if(oldName != name) {
            if(!dot) {
                ret.first = name;
                ret.first += '.';
            }else
                ret.first.assign(name,dot-name+1);
            ret.first += oldName;
            ret.second = oldName;
        }else{
            if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                FC_WARN("element mapped name not found " << name);
            ret.first = name;
            if(dot)
                ret.second = dot+1;
        }
        return ret;
    }
    const char *newName = geo->getElementName(name,true);
    if(newName != name) {
        std::ostringstream ss;
        ss << Data::ComplexGeoData::elementMapPrefix() << newName << '.' << name;
        ret.first = ss.str();
        ret.second = name;
    }else
        ret.second = name;
    return ret;
}

DocumentObject *GeoFeature::resolveElement(DocumentObject *obj, const char *subname, 
        std::pair<std::string,std::string> &elementName, bool append, 
        ElementNameType type, const DocumentObject *filter, const char **_element)
{
    const char *element = 0;
    if(!obj || !obj->getNameInDocument())
        return 0;
    obj = obj->resolve(subname,0,0,&element);
    if(_element) *_element = element;
    if(!obj)
        return 0;
    obj = obj->getLinkedObject(true);
    if(!obj || (filter && obj!=filter))
        return 0;
    if(!element || !element[0])
        return obj;

    auto geo = dynamic_cast<GeoFeature*>(obj);
    if(!geo)
        return obj;
    if(!append) 
        elementName = geo->getElementName(element,type);
    else{
        const auto &names = geo->getElementName(element,type);
        if(names.first.size() && names.second.size()) {
            std::string prefix(subname,element-subname);
            elementName.first = prefix + names.first;
            elementName.second = prefix + names.second;
        }
    }
    return obj;
}

std::string GeoFeature::getElementMapVersion() const {
    auto prop = getPropertyOfGeometry();
    if(!prop || !prop->getComplexData()) return std::string();
    std::ostringstream ss;
    if(getDocument() && getDocument()->Hasher==prop->getComplexData()->Hasher)
        ss << "1.";
    else
        ss << "0.";
    ss << prop->getComplexData()->getElementMapVersion();
    return ss.str();
}

void GeoFeature::updateElementReference() {
    auto prop = getPropertyOfGeometry();
    if(!prop) return;
    auto geo = prop->getComplexData();
    if(!geo || !geo->getElementMapSize()) return;
    auto elementMap = geo->getElementMap();
    bool reset = false;
    auto version = getElementMapVersion();
    if(_elementMapVersion.empty()) 
        _elementMapVersion.swap(version);
    else if(_elementMapVersion!=version) {
        reset = true;
        _elementMapVersion.swap(version);
    }
    if(reset || _elementMapCache!=elementMap) {
        _elementMapCache.swap(elementMap);
        for(auto obj : getInListEx(true))
            PropertyLinkSub::updateElementReferences(this,obj,reset);
    }
}

void GeoFeature::onChanged(const Property *prop) {
    if(!isRestoring() && prop==getPropertyOfGeometry())
        updateElementReference();
    DocumentObject::onChanged(prop);
}

void GeoFeature::onDocumentRestored() {
    _elementMapVersion = getElementMapVersion();
    if(!getDocument()->testStatus(Document::Status::Importing))
        updateElementReference();
    DocumentObject::onDocumentRestored();
}
