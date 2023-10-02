/***************************************************************************
 *   Copyright (c) 2014-2015 Nathan Miller <Nathan.A.Mill[at]gmail.com>    *
 *   Copyright (c) 2014-2015 Balázs Bámer                                  *
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
#include <QApplication>
#include <QMessageBox>
#include <sstream>

#include <BRepAdaptor_Curve.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#endif

#include "Mod/Part/App/PartFeature.h"
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/SelectionFilter.h>
#include <Gui/SelectionObject.h>


//===========================================================================
// CmdSurfaceCut THIS IS THE SURFACE CUT COMMAND
//===========================================================================
DEF_STD_CMD(CmdSurfaceCut)

CmdSurfaceCut::CmdSurfaceCut()
    : Command("Surface_Cut")
{
    sAppModule = "Surface";
    sGroup = QT_TR_NOOP("Surface");
    sMenuText = QT_TR_NOOP("Surface Cut function");
    sToolTipText = QT_TR_NOOP("Cuts a shape with another Shape.\n"
                              "It returns a modified version of the first shape");
    sWhatsThis = "Surface_Cut";
    sStatusTip = sToolTipText;
    sPixmap = "Surface_Cut";
    // sAccel        = "CTRL+H";
}

void CmdSurfaceCut::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    /*    std::vector<Gui::SelectionObject> Sel = getSelection().getSelectionEx(0,
       Part::Feature::getClassTypeId()); if (Sel.size() != 2) {
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
                    int ret = QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Non-solids
       selected"), QObject::tr("The use of non-solids for boolean operations may lead to unexpected
       results.\n" "Do you want to continue?"), QMessageBox::Yes, QMessageBox::No); if (ret ==
       QMessageBox::No) return; askUser = true;
                }
            }
        }

        std::string FeatName = getUniqueObjectName("Cut");
        std::string BaseName  = Sel[0].getFeatName();
        std::string ToolName  = Sel[1].getFeatName();

        openCommand(QT_TRANSLATE_NOOP("Command", "Part Cut"));
        doCommand(Doc,"App.activeDocument().addObject(\"Part::Cut\",\"%s\")",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Base =
       App.activeDocument().%s",FeatName.c_str(),BaseName.c_str());
        doCommand(Doc,"App.activeDocument().%s.Tool =
       App.activeDocument().%s",FeatName.c_str(),ToolName.c_str());
        doCommand(Gui,"Gui.activeDocument().hide('%s')",BaseName.c_str());
        doCommand(Gui,"Gui.activeDocument().hide('%s')",ToolName.c_str());
        copyVisual(FeatName.c_str(), "ShapeColor", BaseName.c_str());
        copyVisual(FeatName.c_str(), "DisplayMode", BaseName.c_str());
        updateActive();
        commitCommand();*/
}


DEF_STD_CMD_A(CmdSurfaceFilling)

CmdSurfaceFilling::CmdSurfaceFilling()
    : Command("Surface_Filling")
{
    sAppModule = "Surface";
    sGroup = QT_TR_NOOP("Surface");
    sMenuText = QT_TR_NOOP("Filling...");
    sToolTipText = QT_TR_NOOP("Creates a surface from a series of picked boundary edges.\n"
                              "Additionally, the surface may be constrained by non-boundary edges\n"
                              "and non-boundary vertices.");
    sStatusTip = sToolTipText;
    sWhatsThis = "Surface_Filling";
    sPixmap = "Surface_Filling";
}

void CmdSurfaceFilling::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string FeatName = getUniqueObjectName("Surface");

    openCommand(QT_TRANSLATE_NOOP("Command", "Create surface"));
    doCommand(Doc, "App.ActiveDocument.addObject(\"Surface::Filling\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "Gui.ActiveDocument.setEdit('%s',0)", FeatName.c_str());
}

bool CmdSurfaceFilling::isActive()
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
    sAppModule = "Surface";
    sGroup = QT_TR_NOOP("Surface");
    sMenuText = QT_TR_NOOP("Fill boundary curves");
    sToolTipText = QT_TR_NOOP("Creates a surface from two, three or four boundary edges.");
    sWhatsThis = "Surface_GeomFillSurface";
    sStatusTip = sToolTipText;
    sPixmap = "Surface_GeomFillSurface";
}

bool CmdSurfaceGeomFillSurface::isActive()
{
    return hasActiveDocument();
}

void CmdSurfaceGeomFillSurface::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string FeatName = getUniqueObjectName("Surface");

    openCommand(QT_TRANSLATE_NOOP("Command", "Create surface"));
    doCommand(Doc,
              "App.ActiveDocument.addObject(\"Surface::GeomFillSurface\",\"%s\")",
              FeatName.c_str());
    doCommand(Doc, "Gui.ActiveDocument.setEdit('%s',0)", FeatName.c_str());
}


DEF_STD_CMD_A(CmdSurfaceCurveOnMesh)

CmdSurfaceCurveOnMesh::CmdSurfaceCurveOnMesh()
    : Command("Surface_CurveOnMesh")
{
    sAppModule = "MeshPart";
    sGroup = QT_TR_NOOP("Surface");
    sMenuText = QT_TR_NOOP("Curve on mesh...");
    sToolTipText = QT_TR_NOOP("Creates an approximated curve on top of a mesh.\n"
                              "This command only works with a 'mesh' object.");
    sWhatsThis = "Surface_CurveOnMesh";
    sStatusTip = sToolTipText;
    sPixmap = "Surface_CurveOnMesh";
}


void CmdSurfaceCurveOnMesh::activated(int)
{
    doCommand(Doc,
              "import MeshPartGui, FreeCADGui\n"
              "FreeCADGui.runCommand('MeshPart_CurveOnMesh')\n");
}

bool CmdSurfaceCurveOnMesh::isActive()
{
    if (Gui::Control().activeDialog()) {
        return false;
    }

    // Check for the selected mesh feature (all Mesh types)
    Base::Type meshType = Base::Type::fromName("Mesh::Feature");
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc && doc->countObjectsOfType(meshType) > 0) {
        return true;
    }

    return false;
}

//===========================================================================
// CmdBlendCurve : Blend Curve Command
//===========================================================================
DEF_STD_CMD_A(CmdBlendCurve)

CmdBlendCurve::CmdBlendCurve()
    : Command("Surface_BlendCurve")
{
    sAppModule = "Surface";
    sGroup = QT_TR_NOOP("Surface");
    sMenuText = QT_TR_NOOP("Blend Curve");
    sToolTipText = QT_TR_NOOP("Join two edges with high continuity");
    sStatusTip = sToolTipText;
    sWhatsThis = "BlendCurve";
    sPixmap = "Surface_BlendCurve";
}

void CmdBlendCurve::activated(int)
{
    std::string docName = App::GetApplication().getActiveDocument()->getName();
    std::string objName[2];
    std::string edge[2];
    std::string featName = getUniqueObjectName("BlendCurve");
    std::vector<Gui::SelectionObject> sel =
        getSelection().getSelectionEx(nullptr, Part::Feature::getClassTypeId());

    objName[0] = sel[0].getFeatName();
    edge[0] = sel[0].getSubNames()[0];

    if (sel.size() == 1) {
        objName[1] = sel[0].getFeatName();
        edge[1] = sel[0].getSubNames()[1];
    }
    else {
        objName[1] = sel[1].getFeatName();
        edge[1] = sel[1].getSubNames()[0];
    }
    openCommand(QT_TRANSLATE_NOOP("Command", "Blend Curve"));
    doCommand(Doc,
              "App.ActiveDocument.addObject(\"Surface::FeatureBlendCurve\",\"%s\")",
              featName.c_str());
    doCommand(Doc,
              "App.ActiveDocument.%s.StartEdge = (App.getDocument('%s').getObject('%s'),['%s'])",
              featName.c_str(),
              docName.c_str(),
              objName[0].c_str(),
              edge[0].c_str());
    doCommand(Doc,
              "App.ActiveDocument.%s.EndEdge = (App.getDocument('%s').getObject('%s'),['%s'])",
              featName.c_str(),
              docName.c_str(),
              objName[1].c_str(),
              edge[1].c_str());
    updateActive();
    commitCommand();
}

bool CmdBlendCurve::isActive()
{
    Gui::SelectionFilter edgeFilter("SELECT Part::Feature SUBELEMENT Edge COUNT 2");
    return edgeFilter.match();
}

DEF_STD_CMD_A(CmdSurfaceExtendFace)

CmdSurfaceExtendFace::CmdSurfaceExtendFace()
    : Command("Surface_ExtendFace")
{
    sAppModule = "Surface";
    sGroup = QT_TR_NOOP("Surface");
    sMenuText = QT_TR_NOOP("Extend face");
    sToolTipText = QT_TR_NOOP("Extrapolates the selected face or surface at its boundaries\n"
                              "with its local U and V parameters.");
    sWhatsThis = "Surface_ExtendFace";
    sStatusTip = sToolTipText;
    sPixmap = "Surface_ExtendFace";
}

void CmdSurfaceExtendFace::activated(int)
{
    Gui::SelectionFilter faceFilter("SELECT Part::Feature SUBELEMENT Face COUNT 1");
    if (faceFilter.match()) {
        const std::vector<std::string>& sub = faceFilter.Result[0][0].getSubNames();
        if (sub.size() == 1) {
            openCommand(QT_TRANSLATE_NOOP("Command", "Extend surface"));
            std::string FeatName = getUniqueObjectName("Surface");
            std::string supportString = faceFilter.Result[0][0].getAsPropertyLinkSubString();
            doCommand(Doc,
                      "App.ActiveDocument.addObject(\"Surface::Extend\",\"%s\")",
                      FeatName.c_str());
            doCommand(Doc,
                      "App.ActiveDocument.%s.Face = %s",
                      FeatName.c_str(),
                      supportString.c_str());
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

bool CmdSurfaceExtendFace::isActive()
{
    return Gui::Selection().countObjectsOfType(Part::Feature::getClassTypeId()) == 1;
}

DEF_STD_CMD_A(CmdSurfaceSections)

CmdSurfaceSections::CmdSurfaceSections()
    : Command("Surface_Sections")
{
    sAppModule = "Surface";
    sGroup = QT_TR_NOOP("Surface");
    sMenuText = QT_TR_NOOP("Sections...");
    sToolTipText = QT_TR_NOOP("Creates a surface from a series of sectional edges.");
    sStatusTip = sToolTipText;
    sWhatsThis = "Surface_Sections";
    sPixmap = "Surface_Sections";
}

void CmdSurfaceSections::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string FeatName = getUniqueObjectName("Surface");

    openCommand(QT_TRANSLATE_NOOP("Command", "Create surface"));
    doCommand(Doc, "App.ActiveDocument.addObject(\"Surface::Sections\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "Gui.ActiveDocument.setEdit('%s',0)", FeatName.c_str());
}

bool CmdSurfaceSections::isActive()
{
    return hasActiveDocument();
}

void CreateSurfaceCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    /*
        rcCmdMgr.addCommand(new CmdSurfaceCut());
    */
    rcCmdMgr.addCommand(new CmdSurfaceFilling());
    rcCmdMgr.addCommand(new CmdSurfaceGeomFillSurface());
    rcCmdMgr.addCommand(new CmdSurfaceSections());
    rcCmdMgr.addCommand(new CmdSurfaceExtendFace());
    rcCmdMgr.addCommand(new CmdSurfaceCurveOnMesh());
    rcCmdMgr.addCommand(new CmdBlendCurve());
}
