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
}

void CmdSketcherToggleConstruction::activated(int iMsg)
{
    // Option A: nothing is selected change creation mode from/to construction
    if(Gui::Selection().countObjectsOfType(Sketcher::SketchObject::getClassTypeId()) == 0){

        Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

        if(geometryCreationMode==Construction) {
            geometryCreationMode=Normal;

            rcCmdMgr.getCommandByName("Sketcher_CreateLine")->getAction()->setIcon(
                Gui::BitmapFactory().pixmap("Sketcher_CreateLine"));
            rcCmdMgr.getCommandByName("Sketcher_CreateRectangle")->getAction()->setIcon(
                Gui::BitmapFactory().pixmap("Sketcher_CreateRectangle"));
            rcCmdMgr.getCommandByName("Sketcher_CreatePolyline")->getAction()->setIcon(
                Gui::BitmapFactory().pixmap("Sketcher_CreatePolyline"));
            rcCmdMgr.getCommandByName("Sketcher_CreateSlot")->getAction()->setIcon(
                Gui::BitmapFactory().pixmap("Sketcher_CreateSlot"));
            // Comp commands require a distinctive treatment
            Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(rcCmdMgr.getCommandByName("Sketcher_CompCreateArc")->getAction());
            QList<QAction*> a = pcAction->actions();
            int index = pcAction->property("defaultAction").toInt();
            a[0]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateArc"));
            a[1]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_Create3PointArc"));
            rcCmdMgr.getCommandByName("Sketcher_CompCreateArc")->getAction()->setIcon(
                index==0?Gui::BitmapFactory().pixmap("Sketcher_CreateArc"):
                Gui::BitmapFactory().pixmap("Sketcher_Create3PointArc"));
            // Conics
            pcAction = qobject_cast<Gui::ActionGroup*>(rcCmdMgr.getCommandByName("Sketcher_CompCreateConic")->getAction());
            a = pcAction->actions();
            index = pcAction->property("defaultAction").toInt();
            a[0]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateEllipse"));
            a[1]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateEllipse_3points"));
            a[2]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_Elliptical_Arc"));
            rcCmdMgr.getCommandByName("Sketcher_CompCreateConic")->getAction()->setIcon(
                index==0?Gui::BitmapFactory().pixmap("Sketcher_CreateEllipse"):
                index==1?Gui::BitmapFactory().pixmap("Sketcher_CreateEllipse_3points"):
                Gui::BitmapFactory().pixmap("Sketcher_Elliptical_Arc"));
            // Circle
            pcAction = qobject_cast<Gui::ActionGroup*>(rcCmdMgr.getCommandByName("Sketcher_CompCreateCircle")->getAction());
            a = pcAction->actions();
            index = pcAction->property("defaultAction").toInt();
            a[0]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateCircle"));
            a[1]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_Create3PointCircle"));
            rcCmdMgr.getCommandByName("Sketcher_CompCreateCircle")->getAction()->setIcon(
                index==0?Gui::BitmapFactory().pixmap("Sketcher_CreateCircle"):
                Gui::BitmapFactory().pixmap("Sketcher_Create3PointCircle"));
            // Polygon
            pcAction = qobject_cast<Gui::ActionGroup*>(rcCmdMgr.getCommandByName("Sketcher_CompCreateRegularPolygon")->getAction());
            a = pcAction->actions();
            index = pcAction->property("defaultAction").toInt();
            a[0]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateTriangle"));
            a[1]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateSquare"));
            a[2]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreatePentagon"));
            a[3]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateHexagon"));
            a[4]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateHeptagon"));
            a[5]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateOctagon"));
            rcCmdMgr.getCommandByName("Sketcher_CompCreateRegularPolygon")->getAction()->setIcon(
                index==0?Gui::BitmapFactory().pixmap("Sketcher_CreateTriangle"):
                index==1?Gui::BitmapFactory().pixmap("Sketcher_CreateSquare"):
                index==2?Gui::BitmapFactory().pixmap("Sketcher_CreatePentagon"):
                index==3?Gui::BitmapFactory().pixmap("Sketcher_CreateHexagon"):
                index==4?Gui::BitmapFactory().pixmap("Sketcher_CreateHeptagon"):
                Gui::BitmapFactory().pixmap("Sketcher_CreateOctagon"));
        }
        else {
            geometryCreationMode=Construction;
            
            rcCmdMgr.getCommandByName("Sketcher_CreateLine")->getAction()->setIcon(
                Gui::BitmapFactory().pixmap("Sketcher_CreateLine_Constr"));
            rcCmdMgr.getCommandByName("Sketcher_CreateRectangle")->getAction()->setIcon(
                Gui::BitmapFactory().pixmap("Sketcher_CreateRectangle_Constr"));
            rcCmdMgr.getCommandByName("Sketcher_CreatePolyline")->getAction()->setIcon(
                Gui::BitmapFactory().pixmap("Sketcher_CreatePolyline_Constr"));
            rcCmdMgr.getCommandByName("Sketcher_CreateSlot")->getAction()->setIcon(
                Gui::BitmapFactory().pixmap("Sketcher_CreateSlot_Constr"));
            // Comp commands require a distinctive treatment
            // Arc
            Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(rcCmdMgr.getCommandByName("Sketcher_CompCreateArc")->getAction());
            QList<QAction*> a = pcAction->actions();
            int index = pcAction->property("defaultAction").toInt();
            a[0]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateArc_Constr"));
            a[1]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_Create3PointArc_Constr"));
            rcCmdMgr.getCommandByName("Sketcher_CompCreateArc")->getAction()->setIcon(
                index==0?Gui::BitmapFactory().pixmap("Sketcher_CreateArc_Constr"):
                Gui::BitmapFactory().pixmap("Sketcher_Create3PointArc_Constr"));        
            // Conics
            pcAction = qobject_cast<Gui::ActionGroup*>(rcCmdMgr.getCommandByName("Sketcher_CompCreateConic")->getAction());
            a = pcAction->actions();
            index = pcAction->property("defaultAction").toInt();
            a[0]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateEllipse_Constr"));
            a[1]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateEllipse_3points_Constr"));
            a[2]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_Elliptical_Arc_Constr"));        
            rcCmdMgr.getCommandByName("Sketcher_CompCreateConic")->getAction()->setIcon(
                index==0?Gui::BitmapFactory().pixmap("Sketcher_CreateEllipse_Constr"):
                index==1?Gui::BitmapFactory().pixmap("Sketcher_CreateEllipse_3points_Constr"):
                Gui::BitmapFactory().pixmap("Sketcher_Elliptical_Arc_Constr"));
            // Circle
            pcAction = qobject_cast<Gui::ActionGroup*>(rcCmdMgr.getCommandByName("Sketcher_CompCreateCircle")->getAction());
            a = pcAction->actions();
            index = pcAction->property("defaultAction").toInt();
            a[0]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateCircle_Constr"));
            a[1]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_Create3PointCircle_Constr"));
            rcCmdMgr.getCommandByName("Sketcher_CompCreateCircle")->getAction()->setIcon(
                index==0?Gui::BitmapFactory().pixmap("Sketcher_CreateCircle_Constr"):
                Gui::BitmapFactory().pixmap("Sketcher_Create3PointCircle_Constr"));
            // Polygon
            pcAction = qobject_cast<Gui::ActionGroup*>(rcCmdMgr.getCommandByName("Sketcher_CompCreateRegularPolygon")->getAction());
            a = pcAction->actions();
            index = pcAction->property("defaultAction").toInt();
            a[0]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateTriangle_Constr"));
            a[1]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateSquare_Constr"));
            a[2]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreatePentagon_Constr"));
            a[3]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateHexagon_Constr"));
            a[4]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateHeptagon_Constr"));
            a[5]->setIcon(Gui::BitmapFactory().pixmap("Sketcher_CreateOctagon_Constr"));
            rcCmdMgr.getCommandByName("Sketcher_CompCreateRegularPolygon")->getAction()->setIcon(
                index==0?Gui::BitmapFactory().pixmap("Sketcher_CreateTriangle_Constr"):
                index==1?Gui::BitmapFactory().pixmap("Sketcher_CreateSquare_Constr"):
                index==2?Gui::BitmapFactory().pixmap("Sketcher_CreatePentagon_Constr"):
                index==3?Gui::BitmapFactory().pixmap("Sketcher_CreateHexagon_Constr"):
                index==4?Gui::BitmapFactory().pixmap("Sketcher_CreateHeptagon_Constr"):
                Gui::BitmapFactory().pixmap("Sketcher_CreateOctagon_Constr"));
        }
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

        // make sure the selected object is the sketch in edit mode
        const App::DocumentObject* obj = selection[0].getObject();
        ViewProviderSketch* sketchView = static_cast<ViewProviderSketch*>
            (Gui::Application::Instance->getViewProvider(obj));

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
        updateActive();

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


