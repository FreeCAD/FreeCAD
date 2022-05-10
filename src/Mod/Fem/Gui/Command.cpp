/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QAction>
# include <QApplication>
# include <QMessageBox>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoEventCallback.h>
# include <SMESH_Mesh.hxx>
# include <SMESHDS_Mesh.hxx>
#endif

#include <App/Document.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/SelectionFilter.h>
#include <Gui/SelectionObject.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/WaitCursor.h>

#include <Mod/Fem/App/FemAnalysis.h>
#include <Mod/Fem/App/FemConstraint.h>
#include <Mod/Fem/App/FemMeshObject.h>
#include <Mod/Fem/App/FemSetNodesObject.h>

#include "ActiveAnalysisObserver.h"
#include "FemSettings.h"

#ifdef FC_USE_VTK
#include <Mod/Fem/App/FemPostPipeline.h>
#endif


using namespace std;

//================================================================================================
//================================================================================================
// helpers
bool getConstraintPrerequisits(Fem::FemAnalysis** Analysis)
{
    Fem::FemAnalysis* ActiveAnalysis = FemGui::ActiveAnalysisObserver::instance()->getActiveObject();
    if (!ActiveAnalysis || !ActiveAnalysis->getTypeId().isDerivedFrom(Fem::FemAnalysis::getClassTypeId())) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No active Analysis"),
            QObject::tr("You need to create or activate a Analysis"));
        return true;
    }

    *Analysis = static_cast<Fem::FemAnalysis*>(ActiveAnalysis);

    // return with no error
    return false;

}

//OvG: Visibility automation show parts and hide meshes on activation of a constraint
std::string gethideMeshShowPartStr(std::string showConstr = "")
{
    return "for amesh in App.activeDocument().Objects:\n\
    if \"" + showConstr + "\" == amesh.Name:\n\
        amesh.ViewObject.Visibility = True\n\
    elif \"Mesh\" in amesh.TypeId:\n\
        aparttoshow = amesh.Name.replace(\"_Mesh\",\"\")\n\
        for apart in App.activeDocument().Objects:\n\
            if aparttoshow == apart.Name:\n\
                apart.ViewObject.Visibility = True\n\
        amesh.ViewObject.Visibility = False\n";
}


//================================================================================================
//================================================================================================
// commands Part, Analysis, Solver

//================================================================================================
/* ATM no gui command implemented in workbench.cpp, user does it in single steps
DEF_STD_CMD_A(CmdFemAddPart)

CmdFemAddPart::CmdFemAddPart()
  : Command("FEM_FemAddPart")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Add a part to the analysis");
    sToolTipText    = QT_TR_NOOP("Add a part to the Analysis");
    sWhatsThis      = "FEM_FemAddPart";
    sStatusTip      = sToolTipText;
    sPixmap         = "fem-add-fem-mesh";
}

void CmdFemAddPart::activated(int)
{
#ifndef FCWithNetgen
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Your FreeCAD is built without NETGEN support. Meshing will not work...."));
    return;
#endif

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select an edge, face, or body. Only one body is allowed."));
        return;
    }

    if (!selection[0].isObjectTypeOf(Part::Feature::getClassTypeId())){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong object type"),
            QObject::tr("Fillet works only on parts"));
        return;
    }

    Part::Feature *base = static_cast<Part::Feature*>(selection[0].getObject());

    std::string AnalysisName = getUniqueObjectName("FemAnalysis");
    std::string MeshName = getUniqueObjectName((std::string(base->getNameInDocument()) +"_Mesh").c_str());

    openCommand(QT_TRANSLATE_NOOP("Command", "Create FEM analysis"));
    doCommand(Doc,"App.activeDocument().addObject('Fem::FemAnalysis','%s')",AnalysisName.c_str());
    doCommand(Doc,"App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject','%s')",MeshName.c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Shape = App.activeDocument().%s",base->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)",AnalysisName.c_str(),MeshName.c_str());
    addModule(Gui,"FemGui");
    doCommand(Gui,"FemGui.setActiveAnalysis(App.activeDocument().%s)",AnalysisName.c_str());
    commitCommand();

    updateActive();
}

bool CmdFemAddPart::isActive(void)
{
    if (Gui::Control().activeDialog())
        return false;
    Base::Type type = Base::Type::fromName("Part::Feature");
    return Gui::Selection().countObjectsOfType(type) > 0;
}
*/

//================================================================================================
// analysis
/* done in Python
DEF_STD_CMD_A(CmdFemCreateAnalysis)

CmdFemCreateAnalysis::CmdFemCreateAnalysis()
  : Command("FEM_CreateAnalysis")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Create a FEM analysis");
    sToolTipText    = QT_TR_NOOP("Create a FEM analysis");
    sWhatsThis      = "FEM_CreateAnalysis";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_Analysis";
}

void CmdFemCreateAnalysis::activated(int)
{
#ifndef FCWithNetgen
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Your FreeCAD is built without NETGEN support. Meshing will not work...."));
    return;
#endif

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select an edge, face, or body. Only one body is allowed."));
        return;
    }

    if (!selection[0].isObjectTypeOf(Part::Feature::getClassTypeId())){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong object type"),
            QObject::tr("Fillet works only on parts"));
        return;
    }

    Part::Feature *base = static_cast<Part::Feature*>(selection[0].getObject());

    std::string AnalysisName = getUniqueObjectName("FemAnalysis");

    std::string MeshName = getUniqueObjectName((std::string(base->getNameInDocument()) +"_Mesh").c_str());


    openCommand(QT_TRANSLATE_NOOP("Command", "Create FEM analysis"));
    doCommand(Doc,"App.activeDocument().addObject('Fem::FemAnalysis','%s')",AnalysisName.c_str());
    doCommand(Doc,"App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject','%s')",MeshName.c_str());
    doCommand(Doc,"App.activeDocument().ActiveObject.Shape = App.activeDocument().%s",base->getNameInDocument());
    doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)",AnalysisName.c_str(),MeshName.c_str());
    addModule(Gui,"FemGui");
    doCommand(Gui,"FemGui.setActiveAnalysis(App.activeDocument().%s)",AnalysisName.c_str());
    commitCommand();

    updateActive();
}

bool CmdFemCreateAnalysis::isActive(void)
{
    return !FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}
*/

//================================================================================================
// solver
/* done in Python
DEF_STD_CMD_A(CmdFemCreateSolver)

CmdFemCreateSolver::CmdFemCreateSolver()
  : Command("FEM_CreateSolver")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Add a solver to the analysis");
    sToolTipText    = QT_TR_NOOP("Add a solver to the Analysis");
    sWhatsThis      = "FEM_CreateSolver";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_SolverStandard";
}

void CmdFemCreateSolver::activated(int)
{
#ifndef FCWithNetgen
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Your FreeCAD is built without NETGEN support. Meshing will not work...."));
    return;
#endif

    Fem::FemAnalysis        *Analysis;

    if(getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("Solver");

    openCommand(QT_TRANSLATE_NOOP("Command", "Create solver for FEM or CFD analysis"));
    doCommand(Doc,"App.activeDocument().addObject(\"Fem::FemSolverObject\",\"%s\")",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)",Analysis->getNameInDocument(),FeatName.c_str());
    updateActive();

    doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());
}

bool CmdFemCreateSolver::isActive(void)
{
    return hasActiveDocument();
}
*/

//================================================================================================
//================================================================================================
// commands Constraints

//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintBearing)

CmdFemConstraintBearing::CmdFemConstraintBearing()
    : Command("FEM_ConstraintBearing")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Constraint bearing");
    sToolTipText    = QT_TR_NOOP("Creates a FEM constraint for a bearing");
    sWhatsThis      = "FEM_ConstraintBearing";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintBearing";
}

void CmdFemConstraintBearing::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("ConstraintBearing");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make FEM constraint for bearing"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintBearing\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)", Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr(FeatName).c_str()); //OvG: Hide meshes and show parts

    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintBearing::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintContact)

CmdFemConstraintContact::CmdFemConstraintContact()
    : Command("FEM_ConstraintContact")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Constraint contact");
    sToolTipText    = QT_TR_NOOP("Creates a FEM constraint for contact between faces");
    sWhatsThis      = "FEM_ConstraintContact";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintContact";
}

void CmdFemConstraintContact::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("ConstraintContact");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make FEM constraint contact on face"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintContact\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Slope = 1000000.00", FeatName.c_str()); //OvG: set default not equal to 0
    doCommand(Doc, "App.activeDocument().%s.Friction = 0.0", FeatName.c_str()); //OvG: set default not equal to 0
    doCommand(Doc, "App.activeDocument().%s.Scale = 1", FeatName.c_str()); //OvG: set initial scale to 1
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)",
        Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr(FeatName).c_str()); //OvG: Hide meshes and show parts

    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintContact::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintDisplacement)

CmdFemConstraintDisplacement::CmdFemConstraintDisplacement()
    : Command("FEM_ConstraintDisplacement")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Constraint displacement");
    sToolTipText    = QT_TR_NOOP("Creates a FEM constraint for a displacement acting on a geometric entity");
    sWhatsThis      = "FEM_ConstraintDisplacement";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintDisplacement";
}

void CmdFemConstraintDisplacement::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("ConstraintDisplacement");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make FEM constraint displacement on face"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintDisplacement\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Scale = 1", FeatName.c_str()); //OvG: set initial scale to 1
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)",
        Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr(FeatName).c_str()); //OvG: Hide meshes and show parts

    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintDisplacement::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintFixed)

CmdFemConstraintFixed::CmdFemConstraintFixed()
    : Command("FEM_ConstraintFixed")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Constraint fixed");
    sToolTipText    = QT_TR_NOOP("Creates a FEM constraint for a fixed geometric entity");
    sWhatsThis      = "FEM_ConstraintFixed";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintFixed";
}

void CmdFemConstraintFixed::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("ConstraintFixed");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make FEM constraint fixed geometry"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintFixed\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Scale = 1", FeatName.c_str()); //OvG: set initial scale to 1
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)", Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr(FeatName).c_str()); //OvG: Hide meshes and show parts

    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintFixed::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintFluidBoundary)

CmdFemConstraintFluidBoundary::CmdFemConstraintFluidBoundary()
    : Command("FEM_ConstraintFluidBoundary")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Fluid boundary condition");
    sToolTipText    = QT_TR_NOOP("Create fluid boundary condition on face entity for Computional Fluid Dynamics");
    sWhatsThis      = "FEM_ConstraintFluidBoundary";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintFluidBoundary";
}

void CmdFemConstraintFluidBoundary::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("ConstraintFluidBoundary");

    openCommand(QT_TRANSLATE_NOOP("Command", "Create fluid boundary condition"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintFluidBoundary\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Scale = 1", FeatName.c_str()); //OvG: set initial scale to 1
    //BoundaryValue is already the default value, zero is acceptable
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)",
        Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr(FeatName).c_str()); //OvG: Hide meshes and show parts
    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintFluidBoundary::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintForce)

CmdFemConstraintForce::CmdFemConstraintForce()
    : Command("FEM_ConstraintForce")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Constraint force");
    sToolTipText    = QT_TR_NOOP("Creates a FEM constraint for a force acting on a geometric entity");
    sWhatsThis      = "FEM_ConstraintForce";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintForce";
}

void CmdFemConstraintForce::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("ConstraintForce");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make FEM constraint force on geometry"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintForce\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Force = 1.0", FeatName.c_str()); //OvG: set default not equal to 0
    doCommand(Doc, "App.activeDocument().%s.Reversed = False", FeatName.c_str()); //OvG: set default to False
    doCommand(Doc, "App.activeDocument().%s.Scale = 1", FeatName.c_str()); //OvG: set initial scale to 1
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)", Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr(FeatName).c_str()); //OvG: Hide meshes and show parts

    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintForce::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintGear)

CmdFemConstraintGear::CmdFemConstraintGear()
    : Command("FEM_ConstraintGear")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Constraint gear");
    sToolTipText    = QT_TR_NOOP("Creates a FEM constraint for a gear");
    sWhatsThis      = "FEM_ConstraintGear";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintGear";
}

void CmdFemConstraintGear::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;
    std::string FeatName = getUniqueObjectName("ConstraintGear");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make FEM constraint for gear"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintGear\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Diameter = 100.0", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)", Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr(FeatName).c_str()); //OvG: Hide meshes and show parts

    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintGear::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintHeatflux)

CmdFemConstraintHeatflux::CmdFemConstraintHeatflux()
    : Command("FEM_ConstraintHeatflux")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Constraint heatflux");
    sToolTipText    = QT_TR_NOOP("Creates a FEM constraint for a heatflux acting on a face");
    sWhatsThis      = "FEM_ConstraintHeatflux";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintHeatflux";
}

void CmdFemConstraintHeatflux::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("ConstraintHeatflux");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make FEM constraint heatflux on face"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintHeatflux\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.AmbientTemp = 300.0", FeatName.c_str()); //OvG: set default not equal to 0
    doCommand(Doc, "App.activeDocument().%s.FilmCoef = 10.0", FeatName.c_str()); //OvG: set default not equal to 0
    doCommand(Doc, "App.activeDocument().%s.Scale = 1", FeatName.c_str()); //OvG: set initial scale to 1
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)",
        Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr().c_str()); //OvG: Hide meshes and show parts

    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintHeatflux::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintInitialTemperature)

CmdFemConstraintInitialTemperature::CmdFemConstraintInitialTemperature()
    : Command("FEM_ConstraintInitialTemperature")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Constraint initial temperature");
    sToolTipText    = QT_TR_NOOP("Creates a FEM constraint for initial temperature acting on a body");
    sWhatsThis      = "FEM_ConstraintInitialTemperature";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintInitialTemperature";
}

void CmdFemConstraintInitialTemperature::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("ConstraintInitialTemperature");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make FEM constraint initial temperature on body"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintInitialTemperature\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Scale = 1", FeatName.c_str()); //OvG: set initial scale to 1
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)",
        Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr().c_str()); //OvG: Hide meshes and show parts

    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintInitialTemperature::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintPlaneRotation)

CmdFemConstraintPlaneRotation::CmdFemConstraintPlaneRotation()
    : Command("FEM_ConstraintPlaneRotation")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Constraint plane rotation");
    sToolTipText    = QT_TR_NOOP("Creates a FEM constraint for plane rotation face");
    sWhatsThis      = "FEM_ConstraintPlaneRotation";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintPlaneRotation";
}

void CmdFemConstraintPlaneRotation::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("ConstraintPlaneRotation");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make FEM constraint Plane Rotation face"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintPlaneRotation\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Scale = 1", FeatName.c_str()); //OvG: set initial scale to 1
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)", Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr(FeatName).c_str()); //OvG: Hide meshes and show parts

    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintPlaneRotation::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintPressure)

CmdFemConstraintPressure::CmdFemConstraintPressure()
    : Command("FEM_ConstraintPressure")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Constraint pressure");
    sToolTipText    = QT_TR_NOOP("Creates a FEM constraint for a pressure acting on a face");
    sWhatsThis      = "FEM_ConstraintPressure";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintPressure";
}

void CmdFemConstraintPressure::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("ConstraintPressure");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make FEM constraint pressure on face"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintPressure\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Pressure = 1000.0", FeatName.c_str()); //OvG: set default not equal to 0
    doCommand(Doc, "App.activeDocument().%s.Reversed = False", FeatName.c_str()); //OvG: set default to False
    doCommand(Doc, "App.activeDocument().%s.Scale = 1", FeatName.c_str()); //OvG: set initial scale to 1
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)",
        Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr(FeatName).c_str()); //OvG: Hide meshes and show parts

    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintPressure::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintSpring)

CmdFemConstraintSpring::CmdFemConstraintSpring()
    : Command("FEM_ConstraintSpring")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Constraint spring");
    sToolTipText    = QT_TR_NOOP("Creates a FEM constraint for a spring acting on a face");
    sWhatsThis      = "FEM_ConstraintSpring";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintSpring";
}

void CmdFemConstraintSpring::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("ConstraintSpring");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make FEM constraint spring on face"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintSpring\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.normalStiffness = 1.0", FeatName.c_str()); //OvG: set default not equal to 0
    doCommand(Doc, "App.activeDocument().%s.tangentialStiffness = 0.0", FeatName.c_str()); //OvG: set default to False
    doCommand(Doc, "App.activeDocument().%s.Scale = 1", FeatName.c_str()); //OvG: set initial scale to 1
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)",
        Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr(FeatName).c_str()); //OvG: Hide meshes and show parts

    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintSpring::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintPulley)

CmdFemConstraintPulley::CmdFemConstraintPulley()
    : Command("FEM_ConstraintPulley")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Constraint pulley");
    sToolTipText    = QT_TR_NOOP("Creates a FEM constraint for a pulley");
    sWhatsThis      = "FEM_ConstraintPulley";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintPulley";
}

void CmdFemConstraintPulley::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("ConstraintPulley");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make FEM constraint for pulley"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintPulley\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Diameter = 300.0", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.OtherDiameter = 100.0", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.CenterDistance = 500.0", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Force = 100.0", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.TensionForce = 100.0", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)", Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr(FeatName).c_str()); //OvG: Hide meshes and show parts

    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintPulley::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintTemperature)

CmdFemConstraintTemperature::CmdFemConstraintTemperature()
    : Command("FEM_ConstraintTemperature")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Constraint temperature");
    sToolTipText    = QT_TR_NOOP("Creates a FEM constraint for a temperature/concentrated heat flux acting on a face");
    sWhatsThis      = "FEM_ConstraintTemperature";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintTemperature";
}

void CmdFemConstraintTemperature::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("ConstraintTemperature");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make FEM constraint temperature on face"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintTemperature\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Scale = 1", FeatName.c_str()); //OvG: set initial scale to 1
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)",
        Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr().c_str()); //OvG: Hide meshes and show parts

    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintTemperature::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemConstraintTransform)

CmdFemConstraintTransform::CmdFemConstraintTransform()
    : Command("FEM_ConstraintTransform")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Constraint transform");
    sToolTipText    = QT_TR_NOOP("Create FEM constraint for transforming a face");
    sWhatsThis      = "FEM_ConstraintTransform";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_ConstraintTransform";
}

void CmdFemConstraintTransform::activated(int)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    std::string FeatName = getUniqueObjectName("ConstraintTransform");

    openCommand(QT_TRANSLATE_NOOP("Command", "Make FEM constraint transform on face"));
    doCommand(Doc, "App.activeDocument().addObject(\"Fem::ConstraintTransform\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.X_rot = 0.0", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Y_rot = 0.0", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Z_rot = 0.0", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.Scale = 1", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.addObject(App.activeDocument().%s)",
        Analysis->getNameInDocument(), FeatName.c_str());

    doCommand(Doc, "%s", gethideMeshShowPartStr(FeatName).c_str()); //OvG: Hide meshes and show parts

    updateActive();

    doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}

bool CmdFemConstraintTransform::isActive(void)
{
    return FemGui::ActiveAnalysisObserver::instance()->hasActiveObject();
}


//================================================================================================
//================================================================================================
// commands mesh

//================================================================================================
DEF_STD_CMD_A(CmdFemDefineNodesSet)

void DefineNodesCallback(void* ud, SoEventCallback* n)
{
    Fem::FemAnalysis* Analysis;

    if (getConstraintPrerequisits(&Analysis))
        return;

    // show the wait cursor because this could take quite some time
    Gui::WaitCursor wc;

    // When this callback function is invoked we must in either case leave the edit mode
    Gui::View3DInventorViewer* view = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());
    view->setEditing(false);
    view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), DefineNodesCallback, ud);
    n->setHandled();

    Gui::SelectionRole role;
    std::vector<SbVec2f> clPoly = view->getGLPolygon(&role);
    if (clPoly.size() < 3)
        return;
    if (clPoly.front() != clPoly.back())
        clPoly.push_back(clPoly.front());

    SoCamera* cam = view->getSoRenderManager()->getCamera();
    SbViewVolume vv = cam->getViewVolume();
    Gui::ViewVolumeProjection proj(vv);
    Base::Polygon2d polygon;
    for (std::vector<SbVec2f>::const_iterator it = clPoly.begin(); it != clPoly.end(); ++it)
        polygon.Add(Base::Vector2d((*it)[0], (*it)[1]));


    std::vector<App::DocumentObject*> docObj = Gui::Selection().getObjectsOfType(Fem::FemMeshObject::getClassTypeId());
    if (docObj.size() != 1)
        return;

    const SMESHDS_Mesh* data = const_cast<SMESH_Mesh*>(static_cast<Fem::FemMeshObject*>(docObj[0])->FemMesh.getValue().getSMesh())->GetMeshDS();

    SMDS_NodeIteratorPtr aNodeIter = data->nodesIterator();
    Base::Vector3f pt2d;
    std::set<int> IntSet;

    while (aNodeIter->more()) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        Base::Vector3f vec(aNode->X(), aNode->Y(), aNode->Z());
        pt2d = proj(vec);
        if (polygon.Contains(Base::Vector2d(pt2d.x, pt2d.y)) == true)
            IntSet.insert(aNode->GetID());
    }

    std::stringstream  set;

    set << "[";
    for (std::set<int>::const_iterator it = IntSet.begin(); it != IntSet.end(); ++it)
        if (it == IntSet.begin())
            set << *it;
        else
            set << "," << *it;
    set << "]";


    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Place robot"));
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.addObject('Fem::FemSetNodesObject','NodeSet')");
    Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.ActiveObject.Nodes = %s", set.str().c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.addObject(App.activeDocument().NodeSet)", Analysis->getNameInDocument());
    // Gui::Command::updateActive();
    Gui::Command::commitCommand();

    //std::vector<Gui::ViewProvider*> views = view->getViewProvidersOfType(ViewProviderMesh::getClassTypeId());
    //if (!views.empty()) {
    //    Gui::Application::Instance->activeDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Cut"));
    //    for (std::vector<Gui::ViewProvider*>::iterator it = views.begin(); it != views.end(); ++it) {
    //        ViewProviderMesh* that = static_cast<ViewProviderMesh*>(*it);
    //        if (that->getEditingMode() > -1) {
    //            that->finishEditing();
    //            that->cutMesh(clPoly, *view, clip_inner);
    //        }
    //    }

    //    Gui::Application::Instance->activeDocument()->commitCommand();

    //    view->render();
    //}
}



CmdFemDefineNodesSet::CmdFemDefineNodesSet()
    : Command("FEM_DefineNodesSet")
{
    sAppModule    = "Fem";
    sGroup        = QT_TR_NOOP("Fem");
    sMenuText     = QT_TR_NOOP("Node set by poly");
    sToolTipText  = QT_TR_NOOP("Create node set by Poly");
    sWhatsThis    = "FEM_DefineNodesSet";
    sStatusTip    = QT_TR_NOOP("Create node set by Poly");
    sPixmap       = "FEM_CreateNodesSet";
}

void CmdFemDefineNodesSet::activated(int)
{
    std::vector<App::DocumentObject*> docObj = Gui::Selection().getObjectsOfType(Fem::FemMeshObject::getClassTypeId());

    for (std::vector<App::DocumentObject*>::iterator it = docObj.begin(); it != docObj.end(); ++it) {
        if (it == docObj.begin()) {
            Gui::Document* doc = getActiveGuiDocument();
            Gui::MDIView* view = doc->getActiveView();
            if (view->getTypeId().isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
                Gui::View3DInventorViewer* viewer = ((Gui::View3DInventor*)view)->getViewer();
                viewer->setEditing(true);
                viewer->startSelection(Gui::View3DInventorViewer::Clip);
                viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), DefineNodesCallback);
            }
            else {
                return;
            }
        }

        //Gui::ViewProvider* pVP = getActiveGuiDocument()->getViewProvider(*it);
        //if (pVP->isVisible())
        //    pVP->startEditing();
    }
}

bool CmdFemDefineNodesSet::isActive(void)
{
    // Check for the selected mesh feature (all Mesh types)
    if (getSelection().countObjectsOfType(Fem::FemMeshObject::getClassTypeId()) != 1)
        return false;

    Gui::MDIView* view = Gui::getMainWindow()->activeWindow();
    if (view && view->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        return !viewer->isEditing();
    }

    return false;
}


//================================================================================================
DEF_STD_CMD_A(CmdFemCreateNodesSet)

CmdFemCreateNodesSet::CmdFemCreateNodesSet()
    : Command("FEM_CreateNodesSet")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Nodes set");
    sToolTipText    = QT_TR_NOOP("Creates a FEM mesh nodes set");
    sWhatsThis      = "FEM_CreateNodesSet";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_CreateNodesSet";
}

void CmdFemCreateNodesSet::activated(int)
{
    Gui::SelectionFilter ObjectFilter("SELECT Fem::FemSetNodesObject COUNT 1");
    Gui::SelectionFilter FemMeshFilter("SELECT Fem::FemMeshObject COUNT 1");

    if (ObjectFilter.match()) {
        Fem::FemSetNodesObject* NodesObj = static_cast<Fem::FemSetNodesObject*>(ObjectFilter.Result[0][0].getObject());
        openCommand(QT_TRANSLATE_NOOP("Command", "Edit nodes set"));
        doCommand(Gui, "Gui.activeDocument().setEdit('%s')", NodesObj->getNameInDocument());
    }
    else if (FemMeshFilter.match()) {
        Fem::FemMeshObject* MeshObj = static_cast<Fem::FemMeshObject*>(FemMeshFilter.Result[0][0].getObject());

        std::string FeatName = getUniqueObjectName("NodesSet");

        openCommand(QT_TRANSLATE_NOOP("Command", "Create nodes set"));
        doCommand(Doc, "App.activeDocument().addObject('Fem::FemSetNodesObject','%s')", FeatName.c_str());
        doCommand(Gui, "App.activeDocument().%s.FemMesh = App.activeDocument().%s", FeatName.c_str(), MeshObj->getNameInDocument());
        doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
    }
    else {
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("CmdFemCreateNodesSet", "Wrong selection"),
            qApp->translate("CmdFemCreateNodesSet", "Select a single FEM mesh or nodes set, please."));
    }
}

bool CmdFemCreateNodesSet::isActive(void)
{
    return hasActiveDocument();
}


//================================================================================================
//================================================================================================
// commands vtk post processing

#ifdef FC_USE_VTK

//================================================================================================
// helper vtk post processing

void setupFilter(Gui::Command* cmd, std::string Name) {
    // In the isActive() functions it is already assured that the filters are only active on allowed objects
    // For the case the clip filter is set by Python code, we check that the input is a post object
    // and issue an error if not.

    if (Gui::Selection().getSelection().size() > 1) {
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("setupFilter", "Error: A filter can only be applied to a single object."),
            qApp->translate("setupFilter", "The filter could not be set up."));
        return;
    }

    auto selObject = Gui::Selection().getSelection()[0].pObject;

    // issue error if no post object
    if (!((selObject->getTypeId() == Base::Type::fromName("Fem::FemPostPipeline"))
        || (selObject->getTypeId() == Base::Type::fromName("Fem::FemPostWarpVectorFilter"))
        || (selObject->getTypeId() == Base::Type::fromName("Fem::FemPostScalarClipFilter"))
        || (selObject->getTypeId() == Base::Type::fromName("Fem::FemPostCutFilter"))
        || (selObject->getTypeId() == Base::Type::fromName("Fem::FemPostClipFilter")) )
        ) {
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("setupFilter", "Error: no post processing object selected."),
            qApp->translate("setupFilter", "The filter could not be set up."));
        return;
    }

    std::string FeatName = cmd->getUniqueObjectName(Name.c_str());

    // at first we must determine the pipeline of the selection object (which can be a pipeline itself)
    bool selectionIsPipeline = false;
    Fem::FemPostPipeline* pipeline = nullptr;
    if (selObject->getTypeId() == Base::Type::fromName("Fem::FemPostPipeline")) {
        pipeline = static_cast<Fem::FemPostPipeline*>(selObject);
        selectionIsPipeline = true;
    }
    else {
        auto parents = selObject->getInList();
        if (!parents.empty()) {
            for (auto parentObject : parents) {
                if (parentObject->getTypeId() == Base::Type::fromName("Fem::FemPostPipeline")) {
                    pipeline = static_cast<Fem::FemPostPipeline*>(parentObject);
                }
            }
        }
    }

    if (!pipeline) {
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("setupFilter", "Error: no post processing object selected."),
            qApp->translate("setupFilter", "The filter could not be set up."));
        return;
    }

    // create the object and add it to the pipeline
    cmd->openCommand(QT_TRANSLATE_NOOP("Command", "Create filter"));
    cmd->doCommand(Gui::Command::Doc, "App.activeDocument().addObject('Fem::FemPost%sFilter','%s')", Name.c_str(), FeatName.c_str());
    // add it as subobject to the pipeline
    cmd->doCommand(Gui::Command::Doc, "__list__ = App.ActiveDocument.%s.Filter", pipeline->getNameInDocument());
    cmd->doCommand(Gui::Command::Doc, "__list__.append(App.ActiveDocument.%s)", FeatName.c_str());
    cmd->doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.Filter = __list__", pipeline->getNameInDocument());
    cmd->doCommand(Gui::Command::Doc, "del __list__");
        
    // set display to assure the user sees the new object
    cmd->doCommand(Gui::Command::Doc, "App.activeDocument().ActiveObject.ViewObject.DisplayMode = \"Surface\"");

    // in case selObject is no pipeline we must set it as input object
    auto objFilter = App::GetApplication().getActiveDocument()->getActiveObject();
    auto femFilter = static_cast<Fem::FemPostFilter*>(objFilter);
    if (!selectionIsPipeline)
        femFilter->Input.setValue(selObject);

    cmd->updateActive();
    // open the dialog to edit the filter
    cmd->doCommand(Gui::Command::Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
}


std::string Plot() {

    return "t=t_coords[len(t_coords)-1]\n\
for i in range(len(t_coords)):\n\
    dum = t_coords[i]\n\
    t_coords[i] = dum - t_coords[len(t_coords)-1]*0.5\n\
m = 0\n\
for i in range(len(sValues)-1):\n\
    m = m +(t_coords[i+1] - t_coords[i])*(sValues[i+1]+sValues[i])\n\
m = (1/t)*0.5*m\n\
membrane = []\n\
for i in range(len(sValues)):\n\
    membrane.append(m)\n\
b = 0\n\
for i in range(len(sValues)-1):\n\
    d = (t_coords[i+1] - t_coords[i])\n\
    b = b + d*(-3/t**2)*(sValues[i+1]*t_coords[i+1] + sValues[i]*t_coords[i])\n\
b2 = -b\n\
bending =[]\n\
for i in range(len(t_coords)):\n\
    func = ((b2-b)/t)*t_coords[i]\n\
    bending.append(func)\n\
peak = []\n\
mb = []\n\
for i in range(len(sValues)):\n\
    peak.append(sValues[i])\n\
    mb.append(bending[i] + membrane[0])\n\
import FreeCAD\n\
import numpy as np\n\
from matplotlib import pyplot as plt\n\
plt.figure(1)\n\
plt.plot(t_coords, membrane, \"k--\")\n\
plt.plot(t_coords, mb, \"b*-\")\n\
plt.plot(t_coords, peak, \"r-x\")\n\
plt.annotate(str(round(membrane[0],2)), xy=(t_coords[0], membrane[0]), xytext=(t_coords[0], membrane[0]))\n\
plt.annotate(str(round(mb[0],2)), xy=(t_coords[0], mb[0]), xytext=(t_coords[0], mb[0]))\n\
plt.annotate(str(round(mb[len(t_coords)-1],2)), xy=(t_coords[len(t_coords)-1], mb[len(t_coords)-1]), xytext=(t_coords[len(t_coords)-1], mb[len(t_coords)-1]))\n\
plt.annotate(str(round(peak[0],2)), xy=(t_coords[0], peak[0]), xytext=(t_coords[0], peak[0]))\n\
plt.annotate(str(round(peak[len(t_coords)-1],2)), xy=(t_coords[len(t_coords)-1], peak[len(t_coords)-1]), xytext=(t_coords[len(t_coords)-1], peak[len(t_coords)-1]))\n\
FreeCAD.Console.PrintError('membrane stress = ')\n\
FreeCAD.Console.PrintError([str(round(membrane[0],2))])\n\
FreeCAD.Console.PrintError('membrane + bending min = ')\n\
FreeCAD.Console.PrintError([str(round(mb[0],2))])\n\
FreeCAD.Console.PrintError('membrane + bending  max = ')\n\
FreeCAD.Console.PrintError([str(round(mb[len(t_coords)-1],2))])\n\
FreeCAD.Console.PrintError('Total stress min = ')\n\
FreeCAD.Console.PrintError([str(round(peak[0],2))])\n\
FreeCAD.Console.PrintError('Total stress max = ')\n\
FreeCAD.Console.PrintError([str(round(peak[len(t_coords)-1],2))])\n\
plt.legend([\"Membrane\", \"Membrane and Bending\", \"Total\"], loc = \"best\")\n\
plt.xlabel(\"Thickness [mm] \")\n\
plt.ylabel(\"Stress [MPa]\")\n\
plt.title(\"Linearized Stresses\")\n\
plt.grid()\n\
plt.show()\n";
}


//================================================================================================
DEF_STD_CMD_A(CmdFemPostClipFilter)

CmdFemPostClipFilter::CmdFemPostClipFilter()
    : Command("FEM_PostFilterClipRegion")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Region clip filter");
    sToolTipText    = QT_TR_NOOP("Define/create a clip filter which uses functions to define the cliped region");
    sWhatsThis      = "FEM_PostFilterClipRegion";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_PostFilterClipRegion";
}

void CmdFemPostClipFilter::activated(int)
{
    setupFilter(this, "Clip");
}

bool CmdFemPostClipFilter::isActive(void)
{
    // only allow one object
    if (getSelection().getSelection().size() > 1)
        return false;
    // only activate if a result is either a post pipeline, scalar, cut or warp filter, itself or along line filter
    if (getSelection().getObjectsOfType<Fem::FemPostPipeline>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostScalarClipFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostCutFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostWarpVectorFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostClipFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostDataAlongLineFilter>().size() == 1)
        return true;
    else
        return false;
}


//================================================================================================
DEF_STD_CMD_A(CmdFemPostCutFilter)

CmdFemPostCutFilter::CmdFemPostCutFilter()
    : Command("FEM_PostFilterCutFunction")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Function cut filter");
    sToolTipText    = QT_TR_NOOP("Cut the data along an implicit function");
    sWhatsThis      = "FEM_PostFilterCutFunction";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_PostFilterCutFunction";
}

void CmdFemPostCutFilter::activated(int)
{
    setupFilter(this, "Cut");
}

bool CmdFemPostCutFilter::isActive(void)
{
    // only allow one object
    if (getSelection().getSelection().size() > 1)
        return false;
    // only activate if a result is either a post pipeline, scalar, clip or warp filter, itself, or along line filter
    if (getSelection().getObjectsOfType<Fem::FemPostPipeline>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostScalarClipFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostClipFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostWarpVectorFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostCutFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostDataAlongLineFilter>().size() == 1)
        return true;
    else
        return false;
}


//================================================================================================
DEF_STD_CMD_A(CmdFemPostDataAlongLineFilter)

CmdFemPostDataAlongLineFilter::CmdFemPostDataAlongLineFilter()
    : Command("FEM_PostFilterDataAlongLine")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Line clip filter");
    sToolTipText    = QT_TR_NOOP("Define/create a clip filter which clips a field along a line");
    sWhatsThis      = "FEM_PostFilterDataAlongLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_PostFilterDataAlongLine";
}

void CmdFemPostDataAlongLineFilter::activated(int)
{
    setupFilter(this, "DataAlongLine");
}

bool CmdFemPostDataAlongLineFilter::isActive(void)
{
    // only allow one object
    if (getSelection().getSelection().size() > 1)
        return false;
    // only activate if a result is either a post pipeline, scalar, cut, clip or warp filter
    if (getSelection().getObjectsOfType<Fem::FemPostPipeline>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostScalarClipFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostCutFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostClipFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostWarpVectorFilter>().size() == 1)
        return true;
    else
        return false;
}


//================================================================================================
DEF_STD_CMD_A(CmdFemPostDataAtPointFilter)

CmdFemPostDataAtPointFilter::CmdFemPostDataAtPointFilter()
    : Command("FEM_PostFilterDataAtPoint")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Data at point clip filter");
    sToolTipText    = QT_TR_NOOP("Define/create a clip filter which clips a field data at point");
    sWhatsThis      = "FEM_PostFilterDataAtPoint";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_PostFilterDataAtPoint";
}

void CmdFemPostDataAtPointFilter::activated(int)
{

    setupFilter(this, "DataAtPoint");

}

bool CmdFemPostDataAtPointFilter::isActive(void)
{
    // only allow one object
    if (getSelection().getSelection().size() > 1)
        return false;
    // only activate if a result is either a post pipeline, scalar, cut, clip, warp or along line filter
    if (getSelection().getObjectsOfType<Fem::FemPostPipeline>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostScalarClipFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostCutFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostClipFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostWarpVectorFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostDataAlongLineFilter>().size() == 1)
        return true;
    else
        return false;
}


//================================================================================================
DEF_STD_CMD_A(CmdFemPostLinearizedStressesFilter)

CmdFemPostLinearizedStressesFilter::CmdFemPostLinearizedStressesFilter()
    : Command("FEM_PostFilterLinearizedStresses")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Stress linearization plot");
    sToolTipText    = QT_TR_NOOP("Define/create stress linearization plots");
    sWhatsThis      = "FEM_PostFilterLinearizedStresses";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_PostFilterLinearizedStresses";
}

void CmdFemPostLinearizedStressesFilter::activated(int)
{

    Gui::SelectionFilter DataAlongLineFilter("SELECT Fem::FemPostDataAlongLineFilter COUNT 1");

    if (DataAlongLineFilter.match()) {
        Fem::FemPostDataAlongLineFilter* DataAlongLine = static_cast<Fem::FemPostDataAlongLineFilter*>(DataAlongLineFilter.Result[0][0].getObject());
        std::string FieldName = DataAlongLine->PlotData.getValue();
        if (
            (FieldName == "Tresca Stress")
            || (FieldName == "von Mises Stress")
            || (FieldName == "Major Principal Stress")
            || (FieldName == "Intermediate Principal Stress")
            || (FieldName == "Minor Principal Stress")
            // names need to match with names in FemVTKTools.cpp, this is not failsafe, but ATM there is no better way for test on a stress result in vtk pipeline
            ) {
            // TODO FIXME only works if the data along the line object has the name DataAlongLine
            // we should get the selected data along the line object 
            doCommand(Gui::Command::Doc, "t_coords = App.ActiveDocument.DataAlongLine.XAxisData");
            doCommand(Gui::Command::Doc, "sValues = App.ActiveDocument.DataAlongLine.YAxisData");
            doCommand(Gui::Command::Doc, Plot().c_str());
        }
        else {
            QMessageBox::warning(Gui::getMainWindow(),
                qApp->translate("CmdFemPostLinearizedStressesFilter", "Wrong selection"),
                qApp->translate("CmdFemPostLinearizedStressesFilter", "Select a Clip filter which clips a STRESS field along a line, please."));
        }
    }
    else {
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("CmdFemPostLinearizedStressesFilter", "Wrong selection"),
            qApp->translate("CmdFemPostLinearizedStressesFilter", "Select a Clip filter which clips a STRESS field along a line, please."));
    }
}

bool CmdFemPostLinearizedStressesFilter::isActive(void)
{
    // only allow one object
    if (getSelection().getSelection().size() > 1)
        return false;

    // we purposely allow it also not non-along line filters because we issue an error message that
    // also explains what the feature is for and how it is set up
    return hasActiveDocument();
}


//================================================================================================
DEF_STD_CMD_A(CmdFemPostScalarClipFilter)

CmdFemPostScalarClipFilter::CmdFemPostScalarClipFilter()
    : Command("FEM_PostFilterClipScalar")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Scalar clip filter");
    sToolTipText    = QT_TR_NOOP("Define/create a clip filter which clips a field with a scalar value");
    sWhatsThis      = "FEM_PostFilterClipScalar";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_PostFilterClipScalar";
}

void CmdFemPostScalarClipFilter::activated(int)
{
    setupFilter(this, "ScalarClip");
}

bool CmdFemPostScalarClipFilter::isActive(void)
{
    // only allow one object
    if (getSelection().getSelection().size() > 1)
        return false;
    // only activate if a result is either a post pipeline, clip, cut, warp or along line filter
    if (getSelection().getObjectsOfType<Fem::FemPostPipeline>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostClipFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostCutFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostWarpVectorFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostDataAlongLineFilter>().size() == 1)
        return true;
    else
        return false;
}


//================================================================================================
DEF_STD_CMD_A(CmdFemPostWarpVectorFilter)

CmdFemPostWarpVectorFilter::CmdFemPostWarpVectorFilter()
    : Command("FEM_PostFilterWarp")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Warp filter");
    sToolTipText    = QT_TR_NOOP("Warp the geometry along a vector field by a certain factor");
    sWhatsThis      = "FEM_PostFilterWarp";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_PostFilterWarp";
}

void CmdFemPostWarpVectorFilter::activated(int)
{
    setupFilter(this, "WarpVector");
}

bool CmdFemPostWarpVectorFilter::isActive(void)
{
    // only allow one object
    if (getSelection().getSelection().size() > 1)
        return false;
    // only activate if a result is either a post pipeline, scalar, clip, cut or along line filter
    if (getSelection().getObjectsOfType<Fem::FemPostPipeline>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostScalarClipFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostCutFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostClipFilter>().size() == 1)
        return true;
    else if (getSelection().getObjectsOfType<Fem::FemPostDataAlongLineFilter>().size() == 1)
        return true;
    else
        return false;
}


//================================================================================================
DEF_STD_CMD_ACL(CmdFemPostFunctions)

CmdFemPostFunctions::CmdFemPostFunctions()
    : Command("FEM_PostCreateFunctions")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Filter functions");
    sToolTipText    = QT_TR_NOOP("Functions for use in postprocessing filter...");
    sWhatsThis      = "FEM_PostCreateFunctions";
    sStatusTip      = sToolTipText;
    eType           = eType|ForEdit;
}

void CmdFemPostFunctions::activated(int iMsg)
{

    std::string name;
    if (iMsg == 0)
        name = "Plane";
    else if (iMsg == 1)
        name = "Sphere";
    else
        return;

    //create the object
    std::vector<Fem::FemPostPipeline*> pipelines = App::GetApplication().getActiveDocument()->getObjectsOfType<Fem::FemPostPipeline>();
    if (!pipelines.empty()) {
        Fem::FemPostPipeline* pipeline = pipelines.front();

        openCommand(QT_TRANSLATE_NOOP("Command", "Create function"));

        //check if the pipeline has a filter provider and add one if needed
        Fem::FemPostFunctionProvider* provider;
        if (!pipeline->Functions.getValue() || pipeline->Functions.getValue()->getTypeId() != Fem::FemPostFunctionProvider::getClassTypeId()) {
            std::string FuncName = getUniqueObjectName("Functions");
            doCommand(Doc, "App.ActiveDocument.addObject('Fem::FemPostFunctionProvider','%s')", FuncName.c_str());
            doCommand(Doc, "App.ActiveDocument.%s.Functions = App.ActiveDocument.%s", pipeline->getNameInDocument(), FuncName.c_str());
            provider = static_cast<Fem::FemPostFunctionProvider*>(getDocument()->getObject(FuncName.c_str()));
        }
        else
            provider = static_cast<Fem::FemPostFunctionProvider*>(pipeline->Functions.getValue());

        //build the object
        std::string FeatName = getUniqueObjectName(name.c_str());
        doCommand(Doc, "App.activeDocument().addObject('Fem::FemPost%sFunction','%s')", name.c_str(), FeatName.c_str());
        doCommand(Doc, "__list__ = App.ActiveDocument.%s.Functions", provider->getNameInDocument());
        doCommand(Doc, "__list__.append(App.ActiveDocument.%s)", FeatName.c_str());
        doCommand(Doc, "App.ActiveDocument.%s.Functions = __list__", provider->getNameInDocument());
        doCommand(Doc, "del __list__");

        //set the default values, for this get the bounding box
        vtkBoundingBox box = pipeline->getBoundingBox();

        double center[3];
        box.GetCenter(center);

        if (iMsg == 0)
            doCommand(Doc, "App.ActiveDocument.%s.Origin = App.Vector(%f, %f, %f)", FeatName.c_str(), center[0],
                center[1], center[2]);
        else if (iMsg == 1) {
            doCommand(Doc, "App.ActiveDocument.%s.Center = App.Vector(%f, %f, %f)", FeatName.c_str(), center[0],
                center[1] + box.GetLength(1) / 2, center[2] + box.GetLength(2) / 2);
            doCommand(Doc, "App.ActiveDocument.%s.Radius = %f", FeatName.c_str(), box.GetDiagonalLength() / 2);
        }


        this->updateActive();
        //most of the times functions are added inside of a filter, make sure this still works
        if (Gui::Application::Instance->activeDocument()->getInEdit() == nullptr)
            doCommand(Gui, "Gui.activeDocument().setEdit('%s')", FeatName.c_str());
    }
    else {
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("CmdFemPostClipFilter", "Wrong selection"),
            qApp->translate("CmdFemPostClipFilter", "Select a pipeline, please."));
    }

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action* CmdFemPostFunctions::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* cmd0 = pcAction->addAction(QString());
    cmd0->setIcon(Gui::BitmapFactory().iconFromTheme("fem-post-geo-plane"));

    QAction* cmd1 = pcAction->addAction(QString());
    cmd1->setIcon(Gui::BitmapFactory().iconFromTheme("fem-post-geo-sphere"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(cmd1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdFemPostFunctions::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* cmd = a[0];
    cmd->setText(QApplication::translate("CmdFemPostFunctions", "Plane"));
    cmd->setToolTip(QApplication::translate("FEM_PostCreateFunctions", "Create a plane function, defined by its origin and normal"));
    cmd->setStatusTip(cmd->toolTip());

    cmd = a[1];
    cmd->setText(QApplication::translate("CmdFemPostFunctions", "Sphere"));
    cmd->setToolTip(QApplication::translate("FEM_PostCreateFunctions", "Create a sphere function, defined by its center and radius"));
    cmd->setStatusTip(cmd->toolTip());

}

bool CmdFemPostFunctions::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}


//================================================================================================
DEF_STD_CMD_AC(CmdFemPostApllyChanges)

CmdFemPostApllyChanges::CmdFemPostApllyChanges()
    : Command("FEM_PostApplyChanges")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Apply changes to pipeline");
    sToolTipText    = QT_TR_NOOP("Apply changes to parameters directly and not on recompute only...");
    sWhatsThis      = "FEM_PostApplyChanges";
    sStatusTip      = sToolTipText;
    sPixmap         = "view-refresh";
    eType           = eType|ForEdit;
}

void CmdFemPostApllyChanges::activated(int iMsg)
{
    FemGui::FemSettings().setPostAutoRecompute(iMsg == 1);
}

bool CmdFemPostApllyChanges::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

Gui::Action* CmdFemPostApllyChanges::createAction(void)
{
    Gui::Action* pcAction = Command::createAction();
    pcAction->setCheckable(true);
    pcAction->setChecked(FemGui::FemSettings().getPostAutoRecompute());

    return pcAction;
}


//================================================================================================
DEF_STD_CMD_A(CmdFemPostPipelineFromResult)

CmdFemPostPipelineFromResult::CmdFemPostPipelineFromResult()
    : Command("FEM_PostPipelineFromResult")
{
    sAppModule      = "Fem";
    sGroup          = QT_TR_NOOP("Fem");
    sMenuText       = QT_TR_NOOP("Post pipeline from result");
    sToolTipText    = QT_TR_NOOP("Creates a post processing pipeline from a result object");
    sWhatsThis      = "FEM_PostPipelineFromResult";
    sStatusTip      = sToolTipText;
    sPixmap         = "FEM_PostPipelineFromResult";
}

void CmdFemPostPipelineFromResult::activated(int)
{
    /*
    Gui::SelectionFilter ResultFilter("SELECT Fem::FemResultObject COUNT 1");
    if (ResultFilter.match()) {
        Base::Console().Message("Debug: `SELECT Fem::FemResultObject COUNT 1` has matched obj");
        Fem::FemResultObject* result = static_cast<Fem::FemResultObject*>(ResultFilter.Result[0][0].getObject());
        //static_cast failed here
        Base::Console().Message("Debug: FemResultObject pointer = %p", result );

    */

    // go through active document change some Visibility
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    App::Document* app = doc->getDocument();
    const std::vector<App::DocumentObject*> obj = app->getObjectsOfType
    (App::DocumentObject::getClassTypeId());

    for (std::vector<App::DocumentObject*>::const_iterator it = obj.begin(); it != obj.end(); ++it) {
        doCommand(Gui, "Gui.getDocument(\"%s\").getObject(\"%s\").Visibility=False"
            , app->getName(), (*it)->getNameInDocument());
    }

    // we need single result object to attach the pipeline to
    std::vector<Fem::FemResultObject*> results = getSelection().getObjectsOfType<Fem::FemResultObject>();
    if (results.size() == 1) {
        // the pipeline should be inside the analysis container if possible
        bool foundAnalysis = false;
        Fem::FemAnalysis* pcAnalysis;
        std::string FeatName = getUniqueObjectName("ResultPipeline");
        auto parents = results[0]->getInList();
        if (!parents.empty()) {
            for (auto parentObject : parents) {
                if (parentObject->getTypeId() == Base::Type::fromName("Fem::FemAnalysis")) {
                    pcAnalysis = static_cast<Fem::FemAnalysis*>(parentObject);
                    foundAnalysis = true;
                }
            }
        }
        // create the pipeline object
        openCommand(QT_TRANSLATE_NOOP("Command", "Create pipeline from result"));
        if (foundAnalysis)
            pcAnalysis->addObject("Fem::FemPostPipeline", FeatName.c_str());
        else
            doCommand(Doc, "App.activeDocument().addObject('Fem::FemPostPipeline','%s')", FeatName.c_str());
        // load the contents of the result object to the pipeline
        doCommand(Doc, "App.activeDocument().ActiveObject.load("
            "App.activeDocument().getObject(\"%s\"))", results[0]->getNameInDocument());
        // set display to assure the user sees the new object
        doCommand(Doc, "App.activeDocument().ActiveObject.ViewObject.DisplayMode = \"Surface\"");
        commitCommand();

        this->updateActive();

    }
    else {
        QMessageBox::warning(Gui::getMainWindow(),
            qApp->translate("CmdFemPostPipelineFromResult", "Wrong selection type"),
            qApp->translate("CmdFemPostPipelineFromResult", "Select a result object, please."));
    }
}

bool CmdFemPostPipelineFromResult::isActive(void)
{
    // only activate if a result object is selected from which the pipeline can be loaded
    std::vector<Fem::FemResultObject*> results = getSelection().getObjectsOfType<Fem::FemResultObject>();
    return (results.size() == 1) ? true : false;
}

#endif


//================================================================================================
//================================================================================================
void CreateFemCommands(void)
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    // part, analysis, solver
    //rcCmdMgr.addCommand(new CmdFemAddPart()); // not implemented as GUI menu or click icon
    //rcCmdMgr.addCommand(new CmdFemCreateAnalysis()); // Analysis is created in python
    //rcCmdMgr.addCommand(new CmdFemCreateSolver());  // Solver will be extended and created in python

    // constraints
    rcCmdMgr.addCommand(new CmdFemConstraintBearing());
    rcCmdMgr.addCommand(new CmdFemConstraintContact());
    rcCmdMgr.addCommand(new CmdFemConstraintDisplacement());
    rcCmdMgr.addCommand(new CmdFemConstraintFixed());
    rcCmdMgr.addCommand(new CmdFemConstraintFluidBoundary());
    rcCmdMgr.addCommand(new CmdFemConstraintForce());
    rcCmdMgr.addCommand(new CmdFemConstraintGear());
    rcCmdMgr.addCommand(new CmdFemConstraintHeatflux());
    rcCmdMgr.addCommand(new CmdFemConstraintInitialTemperature());
    rcCmdMgr.addCommand(new CmdFemConstraintPlaneRotation());
    rcCmdMgr.addCommand(new CmdFemConstraintPressure());
    rcCmdMgr.addCommand(new CmdFemConstraintPulley());
    rcCmdMgr.addCommand(new CmdFemConstraintTemperature());
    rcCmdMgr.addCommand(new CmdFemConstraintTransform());
    rcCmdMgr.addCommand(new CmdFemConstraintSpring());

    // mesh
    rcCmdMgr.addCommand(new CmdFemCreateNodesSet());
    rcCmdMgr.addCommand(new CmdFemDefineNodesSet());

    // vtk post processing
#ifdef FC_USE_VTK
    rcCmdMgr.addCommand(new CmdFemPostClipFilter);
    rcCmdMgr.addCommand(new CmdFemPostCutFilter);
    rcCmdMgr.addCommand(new CmdFemPostDataAlongLineFilter);
    rcCmdMgr.addCommand(new CmdFemPostDataAtPointFilter);
    rcCmdMgr.addCommand(new CmdFemPostLinearizedStressesFilter);
    rcCmdMgr.addCommand(new CmdFemPostScalarClipFilter);
    rcCmdMgr.addCommand(new CmdFemPostWarpVectorFilter);
    rcCmdMgr.addCommand(new CmdFemPostFunctions);
    rcCmdMgr.addCommand(new CmdFemPostApllyChanges);
    rcCmdMgr.addCommand(new CmdFemPostPipelineFromResult);
#endif
}
