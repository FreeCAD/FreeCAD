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

#include <App/GeoFeaturePy.h>

#include <Base/Tools.h>

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
    ADD_PROPERTY_TYPE(Placement, (Base::Placement()), nullptr, Prop_NoRecompute, nullptr);
    ADD_PROPERTY_TYPE(_ElementMapVersion,
                    (""),
                    "Base",
                    (App::PropertyType)(Prop_Output | Prop_Hidden | Prop_Transient),
                    "");
}

GeoFeature::~GeoFeature() = default;

void GeoFeature::transformPlacement(const Base::Placement& transform)
{
    Base::Placement plm = this->Placement.getValue();
    plm = transform * plm;
    this->Placement.setValue(plm);
}

Base::Placement GeoFeature::globalPlacement() const
{
    return GeoFeature::getGlobalPlacement(this);
}

const PropertyComplexGeoData* GeoFeature::getPropertyOfGeometry() const
{
    return nullptr;
}

PyObject* GeoFeature::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new GeoFeaturePy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

ElementNamePair GeoFeature::getElementName(const char* name, ElementNameType type) const
{
    (void)type;

    if (!name) {
        return {};
    }

    auto prop = getPropertyOfGeometry();
    if (!prop) {
        return ElementNamePair("", name);
    }

    auto geo = prop->getComplexData();
    if (!geo) {
        return ElementNamePair("", name);
    }

    return _getElementName(name, geo->getElementName(name));
}

ElementNamePair GeoFeature::_getElementName(const char* name,
                                            const Data::MappedElement& mapped) const
{
    ElementNamePair ret;
    if (mapped.index && mapped.name) {
        std::ostringstream ss;
        ss << Data::ComplexGeoData::elementMapPrefix() << mapped.name << '.' << mapped.index;
        std::string result;
        mapped.index.appendToStringBuffer(result);
        return ElementNamePair(ss.str().c_str(), result.c_str());
    }
    else if (mapped.name) {
        //        FC_TRACE("element mapped name " << name << " not found in " << getFullName());
        const char* dot = strrchr(name, '.');
        if (dot) {
            // deliberately mangle the old style element name to signal a
            // missing reference
            std::ostringstream ss;
            ss << Data::MISSING_PREFIX << dot + 1;
            return ElementNamePair(name, ss.str().c_str());
        }
        return ElementNamePair(name, "");
    }
    else {
        std::string oldName;
        mapped.index.appendToStringBuffer(oldName);
        return ElementNamePair("", oldName.c_str());
    }
}

DocumentObject* GeoFeature::resolveElement(const DocumentObject* obj,
                                           const char* subname,
                                           ElementNamePair& elementName,
                                           bool append,
                                           ElementNameType type,
                                           const DocumentObject* filter,
                                           const char** _element,
                                           GeoFeature** geoFeature)
{
    elementName.newName.clear();
    elementName.oldName.clear();
    if (!obj || !obj->isAttachedToDocument()) {
        return nullptr;
    }
    if (!subname) {
        subname = "";
    }
    const char* element = Data::findElementName(subname);
    if (_element) {
        *_element = element;
    }
    auto sobj = obj->getSubObject(std::string(subname, element).c_str());
    if (!sobj) {
        return nullptr;
    }
    auto linked = sobj->getLinkedObject(true);
    auto geo = freecad_cast<GeoFeature*>(linked);
    if (!geo && linked) {
        auto ext = linked->getExtensionByType<LinkBaseExtension>(true);
        if (ext) {
            geo = freecad_cast<GeoFeature*>(ext->getTrueLinkedObject(true));
        }
    }
    if (geoFeature) {
        *geoFeature = geo;
    }
    if (filter && geo != filter) {
        return nullptr;
    }
    if (!element || !element[0]) {
        if (append) {
            elementName.oldName = Data::oldElementName(subname);
        }
        return sobj;
    }

    if (!geo || hasHiddenMarker(element)) {
        if (!append) {
            elementName.oldName = element;
        }
        else {
            elementName.oldName = Data::oldElementName(subname);
        }
        return sobj;
    }
    if (!append) {
        elementName = geo->getElementName(element, type);
    }
    else {
        const auto& names = geo->getElementName(element, type);
        std::string prefix(subname, element - subname);
        if (!names.newName.empty()) {
            elementName.newName = prefix + names.newName;
        }
        elementName.oldName = prefix + names.oldName;
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

bool GeoFeature::getCameraAlignmentDirection(Base::Vector3d& directionZ, Base::Vector3d& directionX, const char* subname) const
{
    Q_UNUSED(subname)
    Q_UNUSED(directionZ)
    Q_UNUSED(directionX)
    return false;
}

bool GeoFeature::hasMissingElement(const char* subname)
{
    return Data::hasMissingElement(subname);
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

    auto version = getElementMapVersion(prop);
    if (_ElementMapVersion.getStrValue().empty()) {
        _ElementMapVersion.setValue(version);
    }
    else if (_ElementMapVersion.getStrValue() != version) {
        reset = true;
        _ElementMapVersion.setValue(version);
    }

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

void GeoFeature::onDocumentRestored()
{
    if (!getDocument()->testStatus(Document::Status::Importing)) {
        _ElementMapVersion.setValue(getElementMapVersion(getPropertyOfGeometry(), true));
    }
    DocumentObject::onDocumentRestored();
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

std::vector<const char*> GeoFeature::getElementTypes(bool /*all*/) const
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

Base::Placement GeoFeature::getPlacementFromProp(App::DocumentObject* obj, const char* propName)
{
    Base::Placement plc = Base::Placement();
    if (!obj) {
        return plc;
    }

    auto* propPlacement = dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName(propName));
    if (propPlacement) {
        plc = propPlacement->getValue();
    }
    return plc;
}

Base::Placement GeoFeature::getGlobalPlacement(App::DocumentObject* targetObj,
                                               App::DocumentObject* rootObj,
                                               const std::string& sub)
{
    if (!rootObj) {
        return Base::Placement();
    }

    return rootObj->getPlacementOf(sub, targetObj);
}

Base::Placement GeoFeature::getGlobalPlacement(App::DocumentObject* targetObj,
                                               App::PropertyXLinkSub* prop)
{
    if (!targetObj || !prop) {
        return Base::Placement();
    }

    std::vector<std::string> subs = prop->getSubValues();
    if (subs.empty()) {
        return Base::Placement();
    }

    return getGlobalPlacement(targetObj, prop->getValue(), subs[0]);
}

Base::Placement GeoFeature::getGlobalPlacement(const DocumentObject* obj)
{
    auto placementProperty = obj->getPropertyByName<App::PropertyPlacement>("Placement");

    if (!placementProperty) {
        return {};
    }

    auto* group = GeoFeatureGroupExtension::getGroupOfObject(obj);
    if (group) {
        auto ext = group->getExtensionByType<GeoFeatureGroupExtension>();
        return ext->globalGroupPlacement() * placementProperty->getValue();
    }

    return placementProperty->getValue();
}

