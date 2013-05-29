/***************************************************************************
 *   Copyright (c) YEAR YOUR NAME         <Your e-mail address>            *
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

#include <Base/Console.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Inspection/App/InspectionFeature.h>

#include "VisualInspection.h"
#include "ViewProviderInspection.h"


DEF_STD_CMD_A(CmdVisualInspection);

CmdVisualInspection::CmdVisualInspection()
  : Command("Inspection_VisualInspection")
{
    sAppModule    = "Inspection";
    sGroup        = QT_TR_NOOP("Inspection");
    sMenuText     = QT_TR_NOOP("Visual inspection...");
    sToolTipText  = QT_TR_NOOP("Visual inspection");
    sStatusTip    = QT_TR_NOOP("Visual inspection");
    sWhatsThis    = "Inspection_VisualInspection";
}

void CmdVisualInspection::activated(int iMsg)
{
    InspectionGui::VisualInspection dlg(Gui::getMainWindow());
    dlg.exec();
}

bool CmdVisualInspection::isActive(void)
{
    return App::GetApplication().getActiveDocument();
}

//--------------------------------------------------------------------------------------

DEF_STD_CMD_A(CmdInspectElement);

CmdInspectElement::CmdInspectElement()
  : Command("Inspection_InspectElement")
{
    sAppModule    = "Inspection";
    sGroup        = "Inspection";
    sMenuText     = "Inspection...";
    sToolTipText  = "Get distance information";
    sWhatsThis    = "Inspection_InspectElement";
    sStatusTip    = sToolTipText;
    sPixmap       = "mesh_pipette";
}

void CmdInspectElement::activated(int iMsg)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->setRedirectToSceneGraphEnabled(true);
        viewer->setRedirectToSceneGraph(true);
        viewer->setEditingCursor(QCursor(Gui::BitmapFactory().pixmap("mesh_pipette"),4,29));
        viewer->addEventCallback(SoButtonEvent::getClassTypeId(),
            InspectionGui::ViewProviderInspection::inspectCallback);
     }
}

bool CmdInspectElement::isActive(void)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc || doc->countObjectsOfType(Inspection::Feature::getClassTypeId()) == 0)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}

void CreateInspectionCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdVisualInspection());
    rcCmdMgr.addCommand(new CmdInspectElement());
}
