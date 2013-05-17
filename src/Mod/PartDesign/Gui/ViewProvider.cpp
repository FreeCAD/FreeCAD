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
#include <Gui/Control.h>
#include <Gui/Application.h>
#include <Base/Exception.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProvider,PartGui::ViewProviderPart)

ViewProvider::ViewProvider()
{
    oldWb = "";
    oldTip = NULL;
}

ViewProvider::~ViewProvider()
{
}

bool ViewProvider::doubleClicked(void)
{    
    if (PartDesignGui::ActivePartObject != NULL) {
        // Drop into insert mode so that the user doesn't see all the geometry that comes later in the tree
        // Also, this way the user won't be tempted to use future geometry as external references for the sketch
        oldTip = ActivePartObject->Tip.getValue();
        if (oldTip != this->pcObject)
            Gui::Command::doCommand(Gui::Command::Gui,"FreeCADGui.runCommand('PartDesign_MoveTip')");
        else
            oldTip = NULL;
    } else {
        oldTip = NULL;
    }

    try {
        std::string Msg("Edit ");
        Msg += this->pcObject->Label.getValue();
        Gui::Command::openCommand(Msg.c_str());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().setEdit('%s',0)",this->pcObject->getNameInDocument());
    }
    catch (const Base::Exception&) {
        Gui::Command::abortCommand();
    }
    return true;
}

void ViewProvider::unsetEdit(int ModNum)
{
    // return to the WB we were in before editing the PartDesign feature
    Gui::Command::assureWorkbench(oldWb.c_str());

    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
        if ((PartDesignGui::ActivePartObject != NULL) && (oldTip != NULL)) {
            Gui::Selection().clearSelection();
            Gui::Selection().addSelection(oldTip->getDocument()->getName(), oldTip->getNameInDocument());
            Gui::Command::doCommand(Gui::Command::Gui,"FreeCADGui.runCommand('PartDesign_MoveTip')");
        }
        oldTip = NULL;
    }
    else {
        PartGui::ViewProviderPart::unsetEdit(ModNum);
        oldTip = NULL;
    }
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
