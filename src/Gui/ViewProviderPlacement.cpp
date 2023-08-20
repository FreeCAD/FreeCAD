/***************************************************************************
 *   Copyright (c) 2012 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <sstream>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/nodes/SoDrawStyle.h>
#endif

#include <App/DocumentObject.h>

#include "ViewProviderPlacement.h"
#include "SoFCSelection.h"
#include "SoFCUnifiedSelection.h"


using namespace Gui;

PROPERTY_SOURCE(Gui::ViewProviderPlacement, Gui::ViewProviderGeometryObject)

ViewProviderPlacement::ViewProviderPlacement()
{
    // Change root node to SoFCSelectionRoot because we share the same
    // AxisOrigin node for all instances of Placement
    auto newRoot = new SoFCSelectionRoot(true);
    for(int i=0;i<pcRoot->getNumChildren();++i)
        newRoot->addChild(pcRoot->getChild(i));
    pcRoot->unref();
    pcRoot = newRoot;
    pcRoot->ref();
    sPixmap = "Std_Placement";

    OnTopWhenSelected.setValue(1);
}

ViewProviderPlacement::~ViewProviderPlacement() = default;

void ViewProviderPlacement::onChanged(const App::Property* prop)
{
        ViewProviderGeometryObject::onChanged(prop);
}

std::vector<std::string> ViewProviderPlacement::getDisplayModes() const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.emplace_back("Base");
    return StrList;
}

void ViewProviderPlacement::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Base") == 0)
        setDisplayMaskMode("Base");
    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

static std::unique_ptr<AxisOrigin> Axis;

void ViewProviderPlacement::attach(App::DocumentObject* pcObject)
{
    ViewProviderGeometryObject::attach(pcObject);
    if(!Axis) {
        Axis = std::make_unique<AxisOrigin>();
        std::map<std::string,std::string> labels;
        labels["O"] = "Origin";
        labels["X"] = "X-Axis";
        labels["Y"] = "Y-Axis";
        labels["Z"] = "Z-Axis";
        labels["XY"] = "XY-Plane";
        labels["XZ"] = "XZ-Plane";
        labels["YZ"] = "YZ-Plane";
        Axis->setLabels(labels);
    }
    addDisplayMaskMode(Axis->getNode(), "Base");
}

void ViewProviderPlacement::updateData(const App::Property* prop)
{
    ViewProviderGeometryObject::updateData(prop);
}

bool ViewProviderPlacement::getElementPicked(const SoPickedPoint *pp, std::string &subname) const {
    if(!Axis)
        return false;
    return Axis->getElementPicked(pp,subname);
}

bool ViewProviderPlacement::getDetailPath(
            const char *subname, SoFullPath *pPath, bool append, SoDetail *&det) const
{
    if(!Axis)
        return false;
    int length = pPath->getLength();
    if(append) {
        pPath->append(pcRoot);
        pPath->append(pcModeSwitch);
    }
    if(!Axis->getDetailPath(subname,pPath,det)) {
        pPath->truncate(length);
        return false;
    }
    return true;
}

bool ViewProviderPlacement::isSelectable() const
{
    return true;
}
// ----------------------------------------------------------------------------

// Python feature -----------------------------------------------------------------------

namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderPlacementPython, Gui::ViewProviderPlacement)
/// @endcond

// explicit template instantiation
template class GuiExport ViewProviderPythonFeatureT<ViewProviderPlacement>;
}

