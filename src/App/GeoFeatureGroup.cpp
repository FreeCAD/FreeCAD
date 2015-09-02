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

std::vector<App::DocumentObject*> GeoFeatureGroup::getGeoSubObjects () const {
    const auto & objs = Group.getValues();

    std::set<const App::DocumentObjectGroup*> processedGroups;
    std::set<App::DocumentObject*> rvSet;
    std::set<App::DocumentObject*> curSearchSet (objs.begin(), objs.end());

    processedGroups.insert ( this );

    while ( !curSearchSet.empty() ) {
        rvSet.insert ( curSearchSet.begin (), curSearchSet.end () );

        std::set<App::DocumentObject*> nextSearchSet;
        for ( auto obj: curSearchSet) {
            if ( isNonGeoGroup (obj) ) {
                const App::DocumentObjectGroup *grp = static_cast<const App::DocumentObjectGroup *> (obj);
                // Check if we havent already processed the element may happen in case of nontree structure
                // Note: if the condition is false this generally indicates malformed structure
                if ( processedGroups.find (grp) == processedGroups.end() ) {
                    processedGroups.insert ( grp );
                    const auto & objs = grp->Group.getValues();
                    nextSearchSet.insert (objs.begin(), objs.end());
                }
            }
        }
        nextSearchSet.swap (curSearchSet);
    }

    return std::vector<App::DocumentObject*> ( rvSet.begin(), rvSet.end() );
}

bool GeoFeatureGroup::geoHasObject (const DocumentObject* obj) const {
    const auto & objs = Group.getValues();

    if (!obj) {
        return false;
    }

    std::set<const App::DocumentObjectGroup*> processedGroups;
    std::set<const App::DocumentObject*> curSearchSet (objs.begin(), objs.end());

    processedGroups.insert ( this );

    while ( !curSearchSet.empty() ) {
        if ( curSearchSet.find (obj) != curSearchSet.end() ) {
            return true;
        }
        std::set<const App::DocumentObject*> nextSearchSet;
        for ( auto obj: curSearchSet) {
            if ( isNonGeoGroup (obj) ) {
                const App::DocumentObjectGroup *grp = static_cast<const App::DocumentObjectGroup *> (obj);
                if ( processedGroups.find (grp) == processedGroups.end() ) {
                    processedGroups.insert ( grp );
                    const auto & objs = grp->Group.getValues();
                    nextSearchSet.insert (objs.begin(), objs.end());
                }
            }
        }
        nextSearchSet.swap (curSearchSet);
    }
    return false;
}

GeoFeatureGroup* GeoFeatureGroup::getGroupOfObject(const DocumentObject* obj, bool indirect)
{
    const Document* doc = obj->getDocument();
    std::vector<DocumentObject*> grps = doc->getObjectsOfType(GeoFeatureGroup::getClassTypeId());
    for (std::vector<DocumentObject*>::const_iterator it = grps.begin(); it != grps.end(); ++it) {
        GeoFeatureGroup* grp = (GeoFeatureGroup*)(*it);
        if ( indirect ) {
            if (grp->geoHasObject(obj)) {
                return grp;
            }
        } else {
            if (grp->hasObject(obj)) {
                return grp;
            }
        }
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
