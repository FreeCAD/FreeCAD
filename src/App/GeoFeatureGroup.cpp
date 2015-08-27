/***************************************************************************
 *   Copyright (c) Juergen Riegel          (juergen.riegel@web.de) 2014    *
 *   Copyright (c) Alexander Golubev (Fat-Zer) <fatzer2@gmail.com> 2015    *
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

using namespace App;


PROPERTY_SOURCE(App::GeoFeatureGroup, App::DocumentObjectGroup)


//===========================================================================
// Feature
//===========================================================================

GeoFeatureGroup::GeoFeatureGroup(void)
{
    ADD_PROPERTY(Placement,(Base::Placement()));
}

GeoFeatureGroup::~GeoFeatureGroup(void)
{
}

void GeoFeatureGroup::transformPlacement(const Base::Placement &transform)
{
    // NOTE: Keep in sync with APP::GeoFeature
    Base::Placement plm = this->Placement.getValue();
    plm = transform * plm;
    this->Placement.setValue(plm);
}

GeoFeatureGroup* GeoFeatureGroup::getGroupOfObject(const DocumentObject* obj)
{
    const Document* doc = obj->getDocument();
    std::vector<DocumentObject*> grps = doc->getObjectsOfType(GeoFeatureGroup::getClassTypeId());
    for (std::vector<DocumentObject*>::const_iterator it = grps.begin(); it != grps.end(); ++it) {
        GeoFeatureGroup* grp = (GeoFeatureGroup*)(*it);
        if (grp->hasObject(obj))
            return grp;
    }

    return 0;
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
