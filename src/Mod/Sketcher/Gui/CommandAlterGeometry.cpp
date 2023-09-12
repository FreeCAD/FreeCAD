/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Notifications.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "GeometryCreationMode.h"
#include "Utils.h"
#include "ViewProviderSketch.h"


using namespace std;
using namespace SketcherGui;
using namespace Sketcher;

bool isAlterGeoActive(Gui::Document* doc)
{
    if (doc) {
        // checks if a Sketch Viewprovider is in Edit
        if (doc->getInEdit()
            && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            return true;
        }
    }

    return false;
}

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;

/* Constrain commands =======================================================*/
DEF_STD_CMD_A(CmdSketcherToggleConstruction)

CmdSketcherToggleConstruction::CmdSketcherToggleConstruction()
    : Command("Sketcher_ToggleConstruction")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Toggle construction geometry");
    sToolTipText = QT_TR_NOOP("Toggles the toolbar or selected geometry to/from construction mode");
    sWhatsThis = "Sketcher_ToggleConstruction";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_ToggleConstruction";
    sAccel = "G, N";
    eType = ForEdit;

    // list of toggle construction commands
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateLine");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateRectangle");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateRectangle_Center");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateOblong");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CompCreateRectangles");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreatePolyline");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateSlot");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateArc");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_Create3PointArc");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CompCreateArc");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateEllipseByCenter");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateEllipseBy3Points");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateArcOfEllipse");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateArcOfHyperbola");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateArcOfParabola");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CompCreateConic");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateCircle");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_Create3PointCircle");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CompCreateCircle");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateTriangle");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateSquare");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreatePentagon");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateHexagon");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateHeptagon");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateOctagon");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateRegularPolygon");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CompCreateRegularPolygon");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreateBSpline");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CreatePeriodicBSpline");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CompCreateBSpline");
    rcCmdMgr.addCommandMode("ToggleConstruction", "Sketcher_CarbonCopy");
}

void CmdSketcherToggleConstruction::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // Option A: nothing is selected change creation mode from/to construction
    if (Gui::Selection().countObjectsOfType(Sketcher::SketchObject::getClassTypeId()) == 0) {

        Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

        if (geometryCreationMode == Construction) {
            geometryCreationMode = Normal;
        }
        else {
            geometryCreationMode = Construction;
        }

        rcCmdMgr.updateCommands("ToggleConstruction", static_cast<int>(geometryCreationMode));
    }
    else  // there was a selection, so operate in toggle mode.
    {
        // get the selection
        std::vector<Gui::SelectionObject> selection;
        selection =
            getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

        Sketcher::SketchObject* Obj =
            static_cast<Sketcher::SketchObject*>(selection[0].getObject());

        // only one sketch with its subelements are allowed to be selected
        if (selection.size() != 1) {
            Gui::TranslatedUserWarning(Obj,
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select edge(s) from the sketch."));
            return;
        }

        // get the needed lists and objects
        const std::vector<std::string>& SubNames = selection[0].getSubNames();
        if (SubNames.empty()) {
            Gui::TranslatedUserWarning(Obj,
                                       QObject::tr("Wrong selection"),
                                       QObject::tr("Select edge(s) from the sketch."));
            return;
        }

        // undo command open
        openCommand(QT_TRANSLATE_NOOP("Command", "Toggle draft from/to draft"));

        // go through the selected subelements
        bool verticesonly = true;

        for (const auto& subname : SubNames) {
            if (subname.size() > 4 && subname.substr(0, 4) == "Edge") {
                verticesonly = false;
            }
        }

        for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
             ++it) {
            // It was decided to provide a special behaviour:
            // Vertices will only be toggled to/from construction IF ONLY
            // vertices are within the group.
            // If there are a mixture of edges and vertices, vertices will be ignored.
            //
            // Why?
            // Because it is quite common to box select geometry for toggling (specially in
            // connection with carbon copy operations). In 99% of the cases the user does not
            // want to toggle individual points during such operations. For the remaining 1%,
            // in 90% of the cases the uses will select just the points only naturally.


            // only handle edges
            if (it->size() > 4 && it->substr(0, 4) == "Edge") {
                int GeoId = std::atoi(it->substr(4, 4000).c_str()) - 1;
                // issue the actual commands to toggle
                Gui::cmdAppObjectArgs(selection[0].getObject(), "toggleConstruction(%d) ", GeoId);
            }
            else if (verticesonly && it->size() > 6 && it->substr(0, 6) == "Vertex") {
                int vertexId = std::atoi(it->substr(6, 4000).c_str()) - 1;

                int geoId;
                PointPos pos;
                Obj->getGeoVertexIndex(vertexId, geoId, pos);

                auto geo = Obj->getGeometry(geoId);

                if (geo && geo->getTypeId() == Part::GeomPoint::getClassTypeId()) {
                    // issue the actual commands to toggle
                    Gui::cmdAppObjectArgs(selection[0].getObject(),
                                          "toggleConstruction(%d) ",
                                          geoId);
                }
            }
        }
        // finish the transaction and update
        commitCommand();

        tryAutoRecompute(Obj);

        // clear the selection (convenience)
        getSelection().clearSelection();
    }
}

bool CmdSketcherToggleConstruction::isActive()
{
    return isAlterGeoActive(getActiveGuiDocument());
}

}  // namespace SketcherGui

void CreateSketcherCommandsAlterGeo()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherToggleConstruction());
}
