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

#include "GeoFeature.h"
#include "GeoFeatureGroupExtension.h"
#include "ElementNamingUtils.h"


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


std::pair<std::string,std::string> GeoFeature::getElementName(
        const char *name, ElementNameType type) const
{
    (void)type;

    std::pair<std::string,std::string> ret;
    if(!name)
        return ret;

    ret.second = name;
    return ret;
}

DocumentObject *GeoFeature::resolveElement(DocumentObject *obj, const char *subname, 
        std::pair<std::string,std::string> &elementName, bool append, 
        ElementNameType type, const DocumentObject *filter, 
        const char **_element, GeoFeature **geoFeature)
{
    if(!obj || !obj->getNameInDocument())
        return nullptr;
    if(!subname)
        subname = "";
    const char *element = Data::findElementName(subname);
    if(_element) *_element = element;
    auto sobj = obj->getSubObject(subname);
    if(!sobj)
        return nullptr;
    obj = sobj->getLinkedObject(true);
    auto geo = dynamic_cast<GeoFeature*>(obj);
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

