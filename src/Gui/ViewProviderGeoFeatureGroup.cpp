/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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
# include <Inventor/nodes/SoGroup.h>
#endif

#include <App/GeoFeatureGroup.h>

#include "ViewProviderGeoFeatureGroup.h"


using namespace Gui;


PROPERTY_SOURCE(Gui::ViewProviderGeoFeatureGroup, Gui::ViewProviderDocumentObjectGroup)

ViewProviderGeoFeatureGroup::ViewProviderGeoFeatureGroup()
{
    pcGroupChildren = new SoGroup();
    pcGroupChildren->ref();
}

ViewProviderGeoFeatureGroup::~ViewProviderGeoFeatureGroup()
{
    pcGroupChildren->unref();
    pcGroupChildren = 0;
}

std::vector<App::DocumentObject*> ViewProviderGeoFeatureGroup::claimChildren3D(void) const {
    App::GeoFeatureGroup *geoGroup = static_cast<App::GeoFeatureGroup *>(getObject());
    const auto & objs = geoGroup->Group.getValues();

    std::set<App::DocumentObject*> rvSet;
    // search recursively for all non-geoGroups and claim their children either
    std::set<App::DocumentObject*> curSearchSet (objs.begin(), objs.end());

    for ( auto objIt = curSearchSet.begin(); !curSearchSet.empty();
            curSearchSet.erase (objIt), objIt = curSearchSet.begin() ) {
        // Check if we havent already processed the element may happen in case of nontree structure
        // Note: this case generally indicates malformed structure
        if ( rvSet.find (*objIt) != rvSet.end() ) {
            continue;
        }

        rvSet.insert (*objIt);

        if ( (*objIt)->isDerivedFrom ( App::DocumentObjectGroup::getClassTypeId () ) &&
            !(*objIt)->isDerivedFrom ( App::GeoFeatureGroup::getClassTypeId () ) ) {

            // add the non-GeoGroup's content to search
            App::DocumentObjectGroup *group = static_cast<App::DocumentObjectGroup *> (*objIt);
            const auto & objs = group->Group.getValues();

            curSearchSet.insert ( objs.begin(), objs.end() );
        }
    }

    return std::vector<App::DocumentObject*> ( rvSet.begin(), rvSet.end() );
}

void ViewProviderGeoFeatureGroup::attach(App::DocumentObject* pcObject)
{
    addDisplayMaskMode(pcGroupChildren, "Group");
    Gui::ViewProviderDocumentObjectGroup::attach(pcObject);
}

void ViewProviderGeoFeatureGroup::setDisplayMode(const char* ModeName)
{
    if ( strcmp("Group",ModeName)==0 )
        setDisplayMaskMode("Group");

    ViewProviderDocumentObjectGroup::setDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderGeoFeatureGroup::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDocumentObjectGroup::getDisplayModes();

    // add your own modes
    StrList.push_back("Group");

    return StrList;
}

void ViewProviderGeoFeatureGroup::updateData(const App::Property* prop)
{
    if (prop->isDerivedFrom(App::PropertyPlacement::getClassTypeId()) &&
             strcmp(prop->getName(), "Placement") == 0) {
        setTransformation ( static_cast<const App::PropertyPlacement*>(prop)->getValue().toMatrix() );
    } else {
        ViewProviderDocumentObjectGroup::updateData ( prop );
    }
}

// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderGeoFeatureGroupPython, Gui::ViewProviderGeoFeatureGroup)
/// @endcond

// explicit template instantiation
template class GuiExport ViewProviderPythonFeatureT<ViewProviderGeoFeatureGroup>;
}
