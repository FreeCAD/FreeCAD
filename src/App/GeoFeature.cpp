/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <App/GeoFeaturePy.h>

#include "ComplexGeoData.h"
#include "Document.h"
#include "GeoFeature.h"
#include "GeoFeatureGroupExtension.h"
#include "ElementNamingUtils.h"
#include "Link.h"


using namespace App;


PROPERTY_SOURCE(App::GeoFeature, App::DocumentObject)


//===========================================================================
// Feature
//===========================================================================

GeoFeature::GeoFeature()
{
    ADD_PROPERTY_TYPE(Placement,(Base::Placement()),nullptr,Prop_NoRecompute,nullptr);
}

GeoFeature::~GeoFeature() = default;

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

PyObject* GeoFeature::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new GeoFeaturePy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

std::pair<std::string,std::string>
GeoFeature::getElementName(const char *name, ElementNameType type) const
{
    (void)type;

    std::pair<std::string,std::string> ret;
    if(!name)
        return ret;

#ifndef FC_USE_TNP_FIX
    ret.second = name;

    return ret;
#else
    auto prop = getPropertyOfGeometry();
    if (!prop) {
        return std::make_pair("", name);
    }

    auto geo = prop->getComplexData();
    if (!geo) {
        return std::make_pair("", name);
    }

    return _getElementName(name, geo->getElementName(name));
#endif
}

std::pair<std::string, std::string>
GeoFeature::_getElementName(const char* name, const Data::MappedElement& mapped) const
{
    std::pair<std::string, std::string> ret;
    if (mapped.index && mapped.name) {
        std::ostringstream ss;
        ss << Data::ComplexGeoData::elementMapPrefix() << mapped.name << '.' << mapped.index;
        ret.first = ss.str();
        mapped.index.appendToStringBuffer(ret.second);
    }
    else if (mapped.name) {
        //        FC_TRACE("element mapped name " << name << " not found in " << getFullName());
        ret.first = name;
        const char* dot = strrchr(name, '.');
        if (dot) {
            // deliberately mangle the old style element name to signal a
            // missing reference
            ret.second = Data::MISSING_PREFIX;
            ret.second += dot + 1;
        }
    }
    else {
        mapped.index.appendToStringBuffer(ret.second);
    }

    return ret;
}

DocumentObject *GeoFeature::resolveElement(DocumentObject *obj, const char *subname, 
        std::pair<std::string,std::string> &elementName, bool append, 
        ElementNameType type, const DocumentObject *filter, 
        const char **_element, GeoFeature **geoFeature)
{
#ifdef FC_USE_TNP_FIX
    elementName.first.clear();
    elementName.second.clear();
#endif
    if(!obj || !obj->isAttachedToDocument())
        return nullptr;
    if(!subname)
        subname = "";
    const char *element = Data::findElementName(subname);
    if(_element) *_element = element;
#ifdef FC_USE_TNP_FIX
    auto sobj = obj->getSubObject(std::string(subname, element).c_str());
    if(!sobj)
        return nullptr;
    auto linked = sobj->getLinkedObject(true);
    auto geo = Base::freecad_dynamic_cast<GeoFeature>(linked);
    if(!geo && linked) {
        auto ext = linked->getExtensionByType<LinkBaseExtension>(true);
        if(ext)
            geo = Base::freecad_dynamic_cast<GeoFeature>(ext->getTrueLinkedObject(true));
    }
#else
    auto sobj = obj->getSubObject(subname);
    if(!sobj)
        return nullptr;
    obj = sobj->getLinkedObject(true);
    auto geo = dynamic_cast<GeoFeature*>(obj);
#endif
    if(geoFeature)
        *geoFeature = geo;
    if(!obj || (filter && obj!=filter))
        return nullptr;
    if(!element || !element[0]) {
        if(append) 
            elementName.second = Data::oldElementName(subname);
        return sobj;
    }

    if(!geo || hasHiddenMarker(element)) {
        if(!append) 
            elementName.second = element;
        else
            elementName.second = Data::oldElementName(subname);
        return sobj;
    }
    if(!append) 
        elementName = geo->getElementName(element,type);
    else{
        const auto &names = geo->getElementName(element,type);
        std::string prefix(subname,element-subname);
        if(!names.first.empty())
            elementName.first = prefix + names.first;
        elementName.second = prefix + names.second;
    }
    return sobj;
}

App::Material GeoFeature::getMaterialAppearance() const
{
    return App::Material(App::Material::DEFAULT);
}

void GeoFeature::setMaterialAppearance(const App::Material& material)
{
    Q_UNUSED(material)
}

bool GeoFeature::getCameraAlignmentDirection(Base::Vector3d& direction, const char* subname) const
{
    Q_UNUSED(subname)
    Q_UNUSED(direction)
    return false;
}

#ifdef FC_USE_TNP_FIX
bool GeoFeature::hasMissingElement(const char* subname)
{
    return Data::hasMissingElement(subname);
    if (!subname) {
        return false;
    }
    auto dot = strrchr(subname, '.');
    if (!dot) {
        return subname[0] == '?';
    }
    return dot[1] == '?';
}

void GeoFeature::updateElementReference()
{
    auto prop = getPropertyOfGeometry();
    if (!prop) {
        return;
    }
    auto geo = prop->getComplexData();
    if (!geo) {
        return;
    }
    bool reset = false;
    PropertyLinkBase::updateElementReferences(this, reset);
}

void GeoFeature::onChanged(const Property* prop)
{
    if (prop == getPropertyOfGeometry()) {
        if (getDocument() && !getDocument()->testStatus(Document::Restoring)
            && !getDocument()->isPerformingTransaction()) {
            updateElementReference();
        }
    }
    DocumentObject::onChanged(prop);
}

const std::vector<std::string>& GeoFeature::searchElementCache(const std::string& element,
                                                               Data::SearchOptions options,
                                                               double tol,
                                                               double atol) const
{
    static std::vector<std::string> none;
    (void)element;
    (void)options;
    (void)tol;
    (void)atol;
    return none;
}

const std::vector<const char*>& GeoFeature::getElementTypes(bool /*all*/) const
{
    static std::vector<const char*> nil;
    auto prop = getPropertyOfGeometry();
    if (!prop) {
        return nil;
    }
    return prop->getComplexData()->getElementTypes();
}

std::vector<Data::IndexedName> GeoFeature::getHigherElements(const char* element, bool silent) const
{
    auto prop = getPropertyOfGeometry();
    if (!prop) {
        return {};
    }
    return prop->getComplexData()->getHigherElements(element, silent);
}
#endif
