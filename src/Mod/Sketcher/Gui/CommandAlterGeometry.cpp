/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <QMessageBox>
#endif

#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>
#include <Gui/DlgEditFileIncludeProptertyExternal.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "ViewProviderSketch.h"
#include "GeometryCreationMode.h"
#include "CommandConstraints.h"

using namespace std;
using namespace SketcherGui;
using namespace Sketcher;

bool isAlterGeoActive(Gui::Document *doc)
{
   if (doc) {
        // checks if a Sketch Viewprovider is in Edit
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom
            (SketcherGui::ViewProviderSketch::getClassTypeId())) {
                return true;
        }
    }

    return false;
}

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode;

/* Constrain commands =======================================================*/
DEF_STD_CMD_A(CmdSketcherToggleConstruction);

CmdSketcherToggleConstruction::CmdSketcherToggleConstruction()
    :Command("Sketcher_ToggleConstruction")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Toggle construction geometry");
    sToolTipText    = QT_TR_NOOP("Toggles the toolbar or selected geometry to/from construction mode");
    sWhatsThis      = "Sketcher_ToggleConstruction";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_AlterConstruction";
    sAccel          = "C,M";
    eType           = ForEdit;

    // list of toggle construction commands
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateLine");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateRectangle");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreatePolyline");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateSlot");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CompCreateArc");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CompCreateConic");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CompCreateCircle");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CompCreateRegularPolygon");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CompCreateBSpline");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CarbonCopy");
}

void CmdSketcherToggleConstruction::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // Option A: nothing is selected change creation mode from/to construction
    if(Gui::Selection().countObjectsOfType(Sketcher::SketchObject::getClassTypeId()) == 0){

        Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

        if (geometryCreationMode == Construction) {
            geometryCreationMode = Normal;
        }
        else {
            geometryCreationMode = Construction;
        }

        rcCmdMgr.updateCommands("ToggleConstruction", static_cast<int>(geometryCreationMode));
    }
    else // there was a selection, so operate in toggle mode.
    {
        // get the selection
        std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

        // only one sketch with its subelements are allowed to be selected
        if (selection.size() != 1) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Select edge(s) from the sketch."));
            return;
        }

        // get the needed lists and objects
        const std::vector<std::string> &SubNames = selection[0].getSubNames();
        if (SubNames.empty()) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Select edge(s) from the sketch."));
            return;
        }

        // undo command open
        openCommand("Toggle draft from/to draft");

        // go through the selected subelements
        for (std::vector<std::string>::const_iterator it=SubNames.begin();it!=SubNames.end();++it){
            // only handle edges
            if (it->size() > 4 && it->substr(0,4) == "Edge") {
                int GeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
                // issue the actual commands to toggle
                doCommand(Doc,"App.ActiveDocument.%s.toggleConstruction(%d) ",selection[0].getFeatName(),GeoId);
            }
        }
        // finish the transaction and update
        commitCommand();

        tryAutoRecompute();

        // clear the selection (convenience)
        getSelection().clearSelection();
    }
}

bool CmdSketcherToggleConstruction::isActive(void)
{
    return isAlterGeoActive( getActiveGuiDocument() );
}

}

void CreateSketcherCommandsAlterGeo(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherToggleConstruction());
}


