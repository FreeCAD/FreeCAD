/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include "ViewProvider.h"
#include "Workbench.h"
#include <Mod/PartDesign/App/Body.h>
#include <Mod/Part/App/PropertyTopoShape.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Base/Exception.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProvider,PartGui::ViewProviderPart)

ViewProvider::ViewProvider()
{
}

ViewProvider::~ViewProvider()
{
}

bool ViewProvider::doubleClicked(void)
{
    std::string Msg("Edit ");
    Msg += this->pcObject->Label.getValue();
    Gui::Command::openCommand(Msg.c_str());
    try {
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().setEdit('%s',0)",this->pcObject->getNameInDocument());
    }
    catch (const Base::Exception&) {
        Gui::Command::abortCommand();
    }
    return true;
}

void ViewProvider::updateData(const App::Property* prop)
{
    if (prop->getTypeId() == Part::PropertyPartShape::getClassTypeId() && 
        strcmp(prop->getName(),"AddShape") == 0) {
        return;
    }
    if (prop->getTypeId() == Part::PropertyPartShape::getClassTypeId() && 
        strcmp(prop->getName(),"SubShape") == 0) {
        return;
    }
    inherited::updateData(prop);
}

bool ViewProvider::onDelete(const std::vector<std::string> &)
{
    // Body feature housekeeping
    PartDesign::Body* body = PartDesign::Body::findBodyOf(getObject());
    if (body != NULL) {
        body->removeFeature(getObject());
        // Make the new Tip and the previous solid feature visible again
        App::DocumentObject* tip = body->Tip.getValue();
        App::DocumentObject* prev = body->getPrevSolidFeature();
        if (tip != NULL) {
            Gui::Application::Instance->getViewProvider(tip)->show();
            if ((tip != prev) && (prev != NULL))
                Gui::Application::Instance->getViewProvider(prev)->show();
        }
    }

    // TODO: Ask user what to do about dependent objects, e.g. Sketches that have this feature as their support
    // 1. Delete
    // 2. Suppress
    // 3. Re-route

    return true;
}
