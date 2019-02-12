/***************************************************************************
 *   Copyright (c) 2014-2015 Nathan Miller    <Nathan.A.Mill[at]gmail.com> *
 *                           Balázs Bámer                                  *
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
#include <sstream>
#include <QApplication>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <Standard_math.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <Python.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#endif

#include <Base/Console.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/WaitCursor.h>

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/PropertyLinks.h>
#include "Mod/Part/App/PartFeature.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//===========================================================================
// CmdSurfaceCut THIS IS THE SURFACE CUT COMMAND
//===========================================================================
DEF_STD_CMD(CmdSurfaceCut);

CmdSurfaceCut::CmdSurfaceCut()
  :Command("Surface_Cut")
{
    sAppModule    = "Surface";
    sGroup        = QT_TR_NOOP("Surface");
    sMenuText     = QT_TR_NOOP("Surface Cut function");
    sToolTipText  = QT_TR_NOOP("Cuts a Shape with another Shape.\nReturns a modified version of the first shape");
    sWhatsThis    = "Surface_Cut";
    sStatusTip    = QT_TR_NOOP("Surface Cut function");
    sPixmap       = "Surface_Cut";
    sAccel        = "CTRL+H";
}

void CmdSurfaceCut::activated(int iMsg)
{
    Q_UNUSED(iMsg);
/*    std::vector<Gui::SelectionObject> Sel = getSelection().getSelectionEx(0, Part::Feature::getClassTypeId());
    if (Sel.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Invalid selection"),
            QObject::tr("Select two shapes please."));
        return;
    }

    bool askUser = false;
    for (std::vector<Gui::SelectionObject>::iterator it = Sel.begin(); it != Sel.end(); ++it) {
        App::DocumentObject* obj = it->getObject();
        if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            const TopoDS_Shape& shape = static_cast<Part::Feature*>(obj)->Shape.getValue();
            if (!PartGui::checkForSolids(shape) && !askUser) {
                int ret = QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Non-solids selected"),
                    QObject::tr("The use of non-solids for boolean operations may lead to unexpected results.\n"
                                "Do you want to continue?"), QMessageBox::Yes, QMessageBox::No);
                if (ret == QMessageBox::No)
                    return;
                askUser = true;
            }
        }
    }

    std::string FeatName = getUniqueObjectName("Cut");
    std::string BaseName  = Sel[0].getFeatName();
    std::string ToolName  = Sel[1].getFeatName();

    openCommand("Part Cut");
    doCommand(Doc,"App.activeDocument().addObject(\"Part::Cut\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Base = App.activeDocument().%s",FeatName.c_str(),BaseName.c_str());
    doCommand(Doc,"App.activeDocument().%s.Tool = App.activeDocument().%s",FeatName.c_str(),ToolName.c_str());
    doCommand(Gui,"Gui.activeDocument().hide('%s')",BaseName.c_str());
    doCommand(Gui,"Gui.activeDocument().hide('%s')",ToolName.c_str());
    copyVisual(FeatName.c_str(), "ShapeColor", BaseName.c_str());
    copyVisual(FeatName.c_str(), "DisplayMode", BaseName.c_str());
    updateActive();
    commitCommand();*/
}


DEF_STD_CMD_A(CmdSurfaceFilling)

CmdSurfaceFilling::CmdSurfaceFilling()
  :Command("Surface_Filling")
{
    sAppModule    = "Surface";
    sGroup        = QT_TR_NOOP("Surface");
    sMenuText     = QT_TR_NOOP("Filling...");
    sToolTipText  = QT_TR_NOOP("Fills a series of boundary curves, constraint curves and vertexes with a surface");
    sStatusTip    = QT_TR_NOOP("Fills a series of boundary curves, constraint curves and vertexes with a surface");
    sWhatsThis    = "Surface_Filling";
    sPixmap       = "Surface_Filling";
}

void CmdSurfaceFilling::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string FeatName = getUniqueObjectName("Surface");

    openCommand("Create surface");
    doCommand(Doc, "App.ActiveDocument.addObject(\"Surface::Filling\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "Gui.ActiveDocument.setEdit('%s',0)", FeatName.c_str());
}

bool CmdSurfaceFilling::isActive(void)
{
    return hasActiveDocument();
}

//===========================================================================
// Bezier and BSpline surfaces
//===========================================================================
DEF_STD_CMD_A(CmdSurfaceGeomFillSurface)

CmdSurfaceGeomFillSurface::CmdSurfaceGeomFillSurface()
    : Command("Surface_GeomFillSurface")
{
    sAppModule    = "Surface";
    sGroup        = QT_TR_NOOP("Surface");
    sMenuText     = QT_TR_NOOP("Fill boundary curves");
    sToolTipText  = QT_TR_NOOP("Creates a surface from two, three or four boundary edges");
    sWhatsThis    = "Surface_GeomFillSurface";
    sStatusTip    = sToolTipText;
    sPixmap       = "BSplineSurf";
}

bool CmdSurfaceGeomFillSurface::isActive(void)
{
    return hasActiveDocument();
}

void CmdSurfaceGeomFillSurface::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string FeatName = getUniqueObjectName("Surface");

    openCommand("Create surface");
    doCommand(Doc, "App.ActiveDocument.addObject(\"Surface::GeomFillSurface\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "Gui.ActiveDocument.setEdit('%s',0)", FeatName.c_str());
}


DEF_STD_CMD_A(CmdSurfaceCurveOnMesh)

CmdSurfaceCurveOnMesh::CmdSurfaceCurveOnMesh()
  : Command("Surface_CurveOnMesh")
{
    sAppModule    = "MeshPart";
    sGroup        = QT_TR_NOOP("Surface");
    sMenuText     = QT_TR_NOOP("Curve on mesh...");
    sToolTipText  = QT_TR_NOOP("Curve on mesh");
    sWhatsThis    = "Surface_CurveOnMesh";
    sStatusTip    = sToolTipText;
}

void CmdSurfaceCurveOnMesh::activated(int)
{
    doCommand(Doc,"import MeshPartGui, FreeCADGui\n"
                  "FreeCADGui.runCommand('MeshPart_CurveOnMesh')\n");
}

bool CmdSurfaceCurveOnMesh::isActive(void)
{
    if (Gui::Control().activeDialog())
        return false;

    // Check for the selected mesh feature (all Mesh types)
    Base::Type meshType = Base::Type::fromName("Mesh::Feature");
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc && doc->countObjectsOfType(meshType) > 0)
        return true;

    return false;
}

DEF_STD_CMD_A(CmdSurfaceExtendFace)

CmdSurfaceExtendFace::CmdSurfaceExtendFace()
  : Command("Surface_ExtendFace")
{
    sAppModule    = "Surface";
    sGroup        = QT_TR_NOOP("Surface");
    sMenuText     = QT_TR_NOOP("Extend face");
    sToolTipText  = QT_TR_NOOP("Extend face");
    sWhatsThis    = "Surface_ExtendFace";
    sStatusTip    = sToolTipText;
}

void CmdSurfaceExtendFace::activated(int)
{
    Gui::SelectionFilter faceFilter("SELECT Part::Feature SUBELEMENT Face COUNT 1");
    if (faceFilter.match()) {
        const std::vector<std::string> &sub = faceFilter.Result[0][0].getSubNames();
        if (sub.size() == 1) {
            openCommand("Extend surface");
            std::string FeatName = getUniqueObjectName("Surface");
            std::string supportString = faceFilter.Result[0][0].getAsPropertyLinkSubString();
            doCommand(Doc, "App.ActiveDocument.addObject(\"Surface::Extend\",\"%s\")", FeatName.c_str());
            doCommand(Doc, "App.ActiveDocument.%s.Face = %s",FeatName.c_str(),supportString.c_str());
            updateActive();
            commitCommand();
        }
    }
    else {
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("Surface_ExtendFace", "Wrong selection"),
            qApp->translate("Surface_ExtendFace", "Select a single face"));
    }
}

bool CmdSurfaceExtendFace::isActive(void)
{
    return Gui::Selection().countObjectsOfType(Part::Feature::getClassTypeId()) == 1;
}

void CreateSurfaceCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
/*  rcCmdMgr.addCommand(new CmdSurfaceFilling());
    rcCmdMgr.addCommand(new CmdSurfaceCut());*/
    rcCmdMgr.addCommand(new CmdSurfaceFilling());
    rcCmdMgr.addCommand(new CmdSurfaceGeomFillSurface());
    rcCmdMgr.addCommand(new CmdSurfaceCurveOnMesh());
    rcCmdMgr.addCommand(new CmdSurfaceExtendFace());
}
