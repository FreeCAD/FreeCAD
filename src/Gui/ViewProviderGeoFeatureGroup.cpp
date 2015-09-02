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
    return static_cast<App::GeoFeatureGroup *>(getObject())->getGeoSubObjects ();
}

void ViewProviderGeoFeatureGroup::attach(App::DocumentObject* pcObject)
{
    Gui::ViewProviderDocumentObjectGroup::attach(pcObject);
    addDisplayMaskMode(pcGroupChildren, "Group");
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
    App::GeoFeatureGroup *obj = static_cast<App::GeoFeatureGroup*> ( getObject() );
    if (prop == &obj->Placement) {
        setTransformation ( obj->Placement.getValue().toMatrix() );
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
