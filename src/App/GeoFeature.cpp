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
    ADD_PROPERTY_TYPE(Placement,(Base::Placement()),nullptr,Prop_NoRecompute,nullptr);
    ADD_PROPERTY_TYPE(_ElementMapVersion,(""),"Base",
            (App::PropertyType)(Prop_Output|Prop_Hidden|Prop_Transient),"");
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
    if (group) {
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
            FC_LOG("element mapped name not found " << name << " in " << getFullName());
            ret.first = name;
            if(dot) {
                // deliberately mangle the old style element name to signal a
                // missing reference
                ret.second = Data::ComplexGeoData::missingPrefix();
                ret.second += dot+1;
            }
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
        ElementNameType type, const DocumentObject *filter, 
        const char **_element, GeoFeature **geoFeature)
{
    if(!obj || !obj->getNameInDocument())
        return 0;
    if(!subname)
        subname = "";
    const char *element = Data::ComplexGeoData::findElementName(subname);
    if(_element) *_element = element;
    auto sobj = obj->getSubObject(subname);
    if(!sobj)
        return 0;
    obj = sobj->getLinkedObject(true);
    auto geo = dynamic_cast<GeoFeature*>(obj);
    if(geoFeature) 
        *geoFeature = geo;
    if(!obj || (filter && obj!=filter))
        return 0;
    if(!element || !element[0]) {
        if(append) 
            elementName.second = Data::ComplexGeoData::oldElementName(subname);
        return sobj;
    }

    if(!geo || hasHiddenMarker(element)) {
        if(!append) 
            elementName.second = element;
        else
            elementName.second = Data::ComplexGeoData::oldElementName(subname);
        return sobj;
    }
    if(!append) 
        elementName = geo->getElementName(element,type);
    else{
        const auto &names = geo->getElementName(element,type);
        std::string prefix(subname,element-subname);
        if(names.first.size())
            elementName.first = prefix + names.first;
        elementName.second = prefix + names.second;
    }
    return sobj;
}

bool GeoFeature::hasMissingElement(const char *subname) {
    return Data::ComplexGeoData::hasMissingElement(subname);
    if(!subname)
        return false;
    auto dot = strrchr(subname,'.');
    if(!dot)
        return subname[0]=='?';
    return dot[1]=='?';
}

void GeoFeature::updateElementReference() {
    auto prop = getPropertyOfGeometry();
    if(!prop) return;
    auto geo = prop->getComplexData();
    if(!geo || !geo->getElementMapSize()) return;
    auto elementMap = geo->getElementMap();
    bool reset = false;
    auto version = getElementMapVersion(prop);
    if(_ElementMapVersion.getStrValue().empty()) 
        _ElementMapVersion.setValue(version);
    else if(_ElementMapVersion.getStrValue()!=version) {
        reset = true;
        _ElementMapVersion.setValue(version);
    }
    if(reset || _elementMapCache!=elementMap) {
        _elementMapCache.swap(elementMap);
        PropertyLinkBase::updateElementReferences(this,reset);
    }
}

void GeoFeature::onChanged(const Property *prop) {
    if(prop==getPropertyOfGeometry()) {
        if(!isRestoring() && getDocument() && !getDocument()->isPerformingTransaction())
            updateElementReference();
    }
    DocumentObject::onChanged(prop);
}

void GeoFeature::onDocumentRestored() {
    if(!getDocument()->testStatus(Document::Status::Importing))
        _ElementMapVersion.setValue(getElementMapVersion(getPropertyOfGeometry(),true));
    DocumentObject::onDocumentRestored();
}
