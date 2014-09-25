/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2014     *
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

#include <App/Document.h>
#include "GeoFeatureGroup.h"
#include "GeoFeatureGroupPy.h"
#include "FeaturePythonPyImp.h"

// #define new DEBUG_CLIENTBLOCK
using namespace App;


PROPERTY_SOURCE(App::GeoFeatureGroup, App::GeoFeature)


//===========================================================================
// Feature
//===========================================================================

GeoFeatureGroup::GeoFeatureGroup(void)
{
    ADD_PROPERTY(Items,(0));
}

GeoFeatureGroup::~GeoFeatureGroup(void)
{
}

DocumentObject* GeoFeatureGroup::addObject(const char* sType, const char* pObjectName)
{
    DocumentObject* obj = getDocument()->addObject(sType, pObjectName);
    if (obj) addObject(obj);
    return obj;
}

void GeoFeatureGroup::addObject(DocumentObject* obj)
{
    if (!hasObject(obj)) {
        std::vector<DocumentObject*> grp = Items.getValues();
        grp.push_back(obj);
        Items.setValues(grp);
    }
}

void GeoFeatureGroup::removeObject(DocumentObject* obj)
{
    std::vector<DocumentObject*> grp = Items.getValues();
    for (std::vector<DocumentObject*>::iterator it = grp.begin(); it != grp.end(); ++it) {
        if (*it == obj) {
            grp.erase(it);
            Items.setValues(grp);
            break;
        }
    }
}

void GeoFeatureGroup::removeObjectsFromDocument()
{
    std::vector<DocumentObject*> grp = Items.getValues();
    for (std::vector<DocumentObject*>::iterator it = grp.begin(); it != grp.end(); ++it) {
        removeObjectFromDocument(*it);
    }
}

void GeoFeatureGroup::removeObjectFromDocument(DocumentObject* obj)
{
    // remove all children
    if (obj->getTypeId().isDerivedFrom(GeoFeatureGroup::getClassTypeId())) {
        std::vector<DocumentObject*> grp = static_cast<GeoFeatureGroup*>(obj)->Items.getValues();
        for (std::vector<DocumentObject*>::iterator it = grp.begin(); it != grp.end(); ++it) {
            // recursive call to remove all subgroups
            removeObjectFromDocument(*it);
        }
    }

    this->getDocument()->remObject(obj->getNameInDocument());
}

DocumentObject *GeoFeatureGroup::getObject(const char *Name) const
{
    DocumentObject* obj = getDocument()->getObject(Name);
    if (obj && hasObject(obj))
        return obj;
    return 0;
}

bool GeoFeatureGroup::hasObject(const DocumentObject* obj) const
{
    const std::vector<DocumentObject*>& grp = Items.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if (*it == obj)
            return true;
    }

    return false;
}

bool GeoFeatureGroup::isChildOf(const GeoFeatureGroup* group) const
{
    const std::vector<DocumentObject*>& grp = group->Items.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if (*it == this)
            return true;
        if ((*it)->getTypeId().isDerivedFrom(GeoFeatureGroup::getClassTypeId())) {
            if (this->isChildOf(static_cast<GeoFeatureGroup*>(*it)))
                return true;
        }
    }

    return false;
}

std::vector<DocumentObject*> GeoFeatureGroup::getObjects() const
{
    return Items.getValues();
}

std::vector<DocumentObject*> GeoFeatureGroup::getObjectsOfType(const Base::Type& typeId) const
{
    std::vector<DocumentObject*> type;
    const std::vector<DocumentObject*>& grp = Items.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if ( (*it)->getTypeId().isDerivedFrom(typeId))
            type.push_back(*it);
    }

    return type;
}

int GeoFeatureGroup::countObjectsOfType(const Base::Type& typeId) const
{
    int type=0;
    const std::vector<DocumentObject*>& grp = Items.getValues();
    for (std::vector<DocumentObject*>::const_iterator it = grp.begin(); it != grp.end(); ++it) {
        if ( (*it)->getTypeId().isDerivedFrom(typeId))
            type++;
    }

    return type;
}


PyObject *GeoFeatureGroup::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new GeoFeatureGroupPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

// Python feature ---------------------------------------------------------


namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(App::GeoFeatureGroupPython, App::GeoFeatureGroup)
template<> const char* App::GeoFeatureGroupPython::getViewProviderName(void) const {
    return "Gui::ViewProviderGeoFeatureGroupPython";
}
template<> PyObject* App::GeoFeatureGroupPython::getPyObject(void) {
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<App::GeoFeatureGroupPy>(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class AppExport FeaturePythonT<App::GeoFeatureGroup>;
}
