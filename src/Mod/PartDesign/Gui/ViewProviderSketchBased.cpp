/***************************************************************************
 *   Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#include <Gui/Application.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>

#include "ViewProviderSketchBased.h"


using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderSketchBased, PartDesignGui::ViewProvider)


ViewProviderSketchBased::ViewProviderSketchBased() = default;

ViewProviderSketchBased::~ViewProviderSketchBased() = default;


std::vector<App::DocumentObject*> ViewProviderSketchBased::claimChildren() const {
    std::vector<App::DocumentObject*> temp;
    App::DocumentObject* sketch = static_cast<PartDesign::ProfileBased*>(getObject())->Profile.getValue();
    if (sketch && sketch->isDerivedFrom(Part::Part2DObject::getClassTypeId()))
        temp.push_back(sketch);

    return temp;
}


bool ViewProviderSketchBased::onDelete(const std::vector<std::string> &s) {
    PartDesign::ProfileBased* feature = static_cast<PartDesign::ProfileBased*>(getObject());

    // get the Sketch
    Sketcher::SketchObject *pcSketch = nullptr;
    if (feature->Profile.getValue())
        pcSketch = static_cast<Sketcher::SketchObject*>(feature->Profile.getValue());

    // if abort command deleted the object the sketch is visible again
    if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
        Gui::Application::Instance->getViewProvider(pcSketch)->show();

    return ViewProvider::onDelete(s);
}

