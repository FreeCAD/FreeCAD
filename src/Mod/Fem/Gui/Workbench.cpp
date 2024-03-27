/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <werner.wm.mayer@gmx.de>              *
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
#include <qobject.h>
#endif

#include <App/Application.h>
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>
#include <Mod/Fem/App/FemTools.h>

#include "Workbench.h"


using namespace FemGui;

#if 0  // needed for Qt's lupdate utility
    qApp->translate("Workbench", "FEM");
    qApp->translate("Workbench", "&FEM");
    //
    qApp->translate("Workbench", "Model");
    qApp->translate("Workbench", "M&odel");
    qApp->translate("Workbench", "Materials");
    qApp->translate("Workbench", "&Materials");
    qApp->translate("Workbench", "Element Geometry");
    qApp->translate("Workbench", "&Element Geometry");
    qApp->translate("Workbench", "Electrostatic boundary conditions");
    qApp->translate("Workbench", "&Electrostatic boundary conditions");
    qApp->translate("Workbench", "Fluid boundary conditions");
    qApp->translate("Workbench", "&Fluid boundary conditions");
    qApp->translate("Workbench", "Electromagnetic boundary conditions");
    qApp->translate("Workbench", "&Electromagnetic boundary conditions");
    qApp->translate("Workbench", "Geometrical analysis features");
    qApp->translate("Workbench", "&Geometrical analysis features");
    qApp->translate("Workbench", "Mechanical boundary conditions and loads");
    qApp->translate("Workbench", "&Mechanical boundary conditions and loads");
    qApp->translate("Workbench", "Thermal boundary conditions and loads");
    qApp->translate("Workbench", "&Thermal boundary conditions and loads");
    qApp->translate("Workbench", "Analysis features without solver");
    qApp->translate("Workbench", "&Analysis features without solver");
    qApp->translate("Workbench", "Overwrite Constants");
    qApp->translate("Workbench", "&Overwrite Constants");
    //
    qApp->translate("Workbench", "Mesh");
    qApp->translate("Workbench", "M&esh");
    //
    qApp->translate("Workbench", "Solve");
    qApp->translate("Workbench", "&Solve");
    //
    qApp->translate("Workbench", "Results");
    qApp->translate("Workbench", "&Results");
    qApp->translate("Workbench", "Filter functions");
    qApp->translate("Workbench", "&Filter functions");
    //
    qApp->translate("Workbench", "Utilities");
#endif


/// @namespace FemGui @class Workbench
TYPESYSTEM_SOURCE(FemGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench() = default;

Workbench::~Workbench() = default;

void Workbench::setupContextMenu(const char* recipient, Gui::MenuItem* item) const
{
    StdWorkbench::setupContextMenu(recipient, item);
    *item << "Separator"
          << "FEM_MeshClear"
          << "FEM_MeshDisplayInfo";
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();

    Gui::ToolBarItem* model = new Gui::ToolBarItem(root);
    model->setCommand("Model");
    *model << "FEM_Analysis"
           << "Separator"
           << "FEM_MaterialSolid"
           << "FEM_MaterialFluid"
           << "FEM_MaterialMechanicalNonlinear"
           << "FEM_MaterialReinforced"
           << "FEM_MaterialEditor"
           << "Separator"
           << "FEM_ElementGeometry1D"
           << "FEM_ElementRotation1D"
           << "FEM_ElementGeometry2D"
           << "FEM_ElementFluid1D";

    Gui::ToolBarItem* electromag = new Gui::ToolBarItem(root);
    electromag->setCommand("Electromagnetic boundary conditions");
    *electromag << "FEM_CompEmConstraints";

    Gui::ToolBarItem* fluid = new Gui::ToolBarItem(root);
    fluid->setCommand("Fluid boundary conditions");
    *fluid << "FEM_ConstraintInitialFlowVelocity"
           << "FEM_ConstraintInitialPressure"
           << "Separator"
           << "FEM_ConstraintFlowVelocity";

    Gui::ToolBarItem* geom = new Gui::ToolBarItem(root);
    geom->setCommand("Geometrical analysis features");
    *geom << "FEM_ConstraintPlaneRotation"
          << "FEM_ConstraintSectionPrint"
          << "FEM_ConstraintTransform";

    Gui::ToolBarItem* mech = new Gui::ToolBarItem(root);
    mech->setCommand("Mechanical boundary conditions and loads");
    *mech << "FEM_ConstraintFixed"
          << "FEM_ConstraintRigidBody"
          << "FEM_ConstraintDisplacement"
          << "FEM_ConstraintContact"
          << "FEM_ConstraintTie"
          << "FEM_ConstraintSpring"
          << "Separator"
          << "FEM_ConstraintForce"
          << "FEM_ConstraintPressure"
          << "FEM_ConstraintCentrif"
          << "FEM_ConstraintSelfWeight";

    Gui::ToolBarItem* thermal = new Gui::ToolBarItem(root);
    thermal->setCommand("Thermal boundary conditions and loads");
    *thermal << "FEM_ConstraintInitialTemperature"
             << "Separator"
             << "FEM_ConstraintHeatflux"
             << "FEM_ConstraintTemperature"
             << "FEM_ConstraintBodyHeatSource";

    Gui::ToolBarItem* mesh = new Gui::ToolBarItem(root);
    mesh->setCommand("Mesh");
#ifdef FCWithNetgen
    *mesh << "FEM_MeshNetgenFromShape";
#endif
    *mesh << "FEM_MeshGmshFromShape"
          << "Separator"
          << "FEM_MeshBoundaryLayer"
          << "FEM_MeshRegion"
          << "FEM_MeshGroup"
          << "Separator"
          << "FEM_FEMMesh2Mesh";

    Gui::ToolBarItem* solve = new Gui::ToolBarItem(root);
    solve->setCommand("Solve");
    if (!Fem::Tools::checkIfBinaryExists("CCX", "ccx", "ccx").empty()) {
        *solve << "FEM_SolverCalculixCxxtools";
    }
    if (!Fem::Tools::checkIfBinaryExists("Elmer", "elmer", "ElmerSolver").empty()) {
        *solve << "FEM_SolverElmer";
    }
    // also check the multi-CPU Elmer build
    else if (!Fem::Tools::checkIfBinaryExists("Elmer", "elmer", "ElmerSolver_mpi").empty()) {
        *solve << "FEM_SolverElmer";
    }
    if (!Fem::Tools::checkIfBinaryExists("Mystran", "mystran", "mystran").empty()) {
        *solve << "FEM_SolverMystran";
    }
    if (!Fem::Tools::checkIfBinaryExists("Z88", "z88", "z88r").empty()) {
        *solve << "FEM_SolverZ88";
    }
    *solve << "Separator"
           << "FEM_CompMechEquations"
           << "FEM_CompEmEquations"
           << "FEM_EquationFlow"
           << "FEM_EquationFlux"
           << "FEM_EquationHeat"
           << "Separator"
           << "FEM_SolverControl"
           << "FEM_SolverRun";

    Gui::ToolBarItem* results = new Gui::ToolBarItem(root);
    results->setCommand("Results");
    *results << "FEM_ResultsPurge"
             << "FEM_ResultShow";
#ifdef FC_USE_VTK
    *results << "Separator"
             << "FEM_PostApplyChanges"
             << "FEM_PostPipelineFromResult"
             << "Separator"
             << "FEM_PostFilterWarp"
             << "FEM_PostFilterClipScalar"
             << "FEM_PostFilterCutFunction"
             << "FEM_PostFilterClipRegion"
             << "FEM_PostFilterContours"
             << "FEM_PostFilterDataAlongLine"
             << "FEM_PostFilterLinearizedStresses"
             << "FEM_PostFilterDataAtPoint"
             << "Separator"
             << "FEM_PostCreateFunctions";
#endif

    Gui::ToolBarItem* utils = new Gui::ToolBarItem(root);
    utils->setCommand("Utilities");
    *utils << "FEM_ClippingPlaneAdd"
           << "FEM_ClippingPlaneRemoveAll"
           << "FEM_Examples";

    return root;
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    Gui::MenuItem* material = new Gui::MenuItem;
    material->setCommand("Materials");
    *material << "FEM_MaterialSolid"
              << "FEM_MaterialFluid"
              << "FEM_MaterialMechanicalNonlinear"
              << "FEM_MaterialReinforced"
              << "FEM_MaterialEditor";

    Gui::MenuItem* elegeom = new Gui::MenuItem;
    elegeom->setCommand("&Element Geometry");
    *elegeom << "FEM_ElementGeometry1D"
             << "FEM_ElementRotation1D"
             << "FEM_ElementGeometry2D"
             << "FEM_ElementFluid1D";

    Gui::MenuItem* elec = new Gui::MenuItem;
    elec->setCommand("&Electromagnetic boundary conditions");
    *elec << "FEM_ConstraintElectrostaticPotential"
          << "FEM_ConstraintCurrentDensity"
          << "FEM_ConstraintMagnetization";

    Gui::MenuItem* fluid = new Gui::MenuItem;
    fluid->setCommand("&Fluid boundary conditions");
    *fluid << "FEM_ConstraintInitialFlowVelocity"
           << "FEM_ConstraintInitialPressure"
           << "Separator"
           << "FEM_ConstraintFlowVelocity";

    Gui::MenuItem* geom = new Gui::MenuItem;
    geom->setCommand("&Geometrical analysis features");
    *geom << "FEM_ConstraintPlaneRotation"
          << "FEM_ConstraintSectionPrint"
          << "FEM_ConstraintTransform";

    Gui::MenuItem* mech = new Gui::MenuItem;
    mech->setCommand("&Mechanical boundary conditions and loads");
    *mech << "FEM_ConstraintFixed"
          << "FEM_ConstraintRigidBody"
          << "FEM_ConstraintDisplacement"
          << "FEM_ConstraintContact"
          << "FEM_ConstraintTie"
          << "FEM_ConstraintSpring"
          << "Separator"
          << "FEM_ConstraintForce"
          << "FEM_ConstraintPressure"
          << "FEM_ConstraintCentrif"
          << "FEM_ConstraintSelfWeight";

    Gui::MenuItem* thermal = new Gui::MenuItem;
    thermal->setCommand("&Thermal boundary conditions and loads");
    *thermal << "FEM_ConstraintInitialTemperature"
             << "Separator"
             << "FEM_ConstraintHeatflux"
             << "FEM_ConstraintTemperature"
             << "FEM_ConstraintBodyHeatSource";

    //    Gui::MenuItem* nosolver = new Gui::MenuItem;
    //    nosolver->setCommand("&Analysis features without solver");
    //    *nosolver
    //        << "FEM_ConstraintFluidBoundary"
    //        << "Separator"
    //        << "FEM_ConstraintBearing"
    //        << "FEM_ConstraintGear"
    //        << "FEM_ConstraintPulley";

    Gui::MenuItem* constants = new Gui::MenuItem;
    constants->setCommand("&Overwrite Constants");
    *constants << "FEM_ConstantVacuumPermittivity";

    Gui::MenuItem* model = new Gui::MenuItem;
    root->insertItem(item, model);
    model->setCommand("M&odel");
    *model << "FEM_Analysis"
           << "Separator" << material << elegeom << "Separator" << elec << fluid << geom << mech
           << thermal
           << "Separator"
           //        << nosolver
           //        << "Separator"
           << constants;

    Gui::MenuItem* mesh = new Gui::MenuItem;
    root->insertItem(item, mesh);
    mesh->setCommand("M&esh");
#ifdef FCWithNetgen
    *mesh << "FEM_MeshNetgenFromShape";
#endif
    *mesh << "FEM_MeshGmshFromShape"
          << "Separator"
          << "FEM_MeshBoundaryLayer"
          << "FEM_MeshRegion"
          << "FEM_MeshGroup"
          << "Separator"
          //          << "FEM_CreateNodesSet"
          << "FEM_FEMMesh2Mesh";

    Gui::MenuItem* solve = new Gui::MenuItem;
    root->insertItem(item, solve);
    solve->setCommand("&Solve");
    *solve << "FEM_SolverCalculixCxxtools"
           << "FEM_SolverElmer"
           << "FEM_SolverMystran"
           << "FEM_SolverZ88"
           << "Separator"
           << "FEM_CompMechEquations"
           << "FEM_CompEmEquations"
           << "FEM_EquationFlow"
           << "FEM_EquationFlux"
           << "FEM_EquationHeat"
           << "Separator"
           << "FEM_SolverControl"
           << "FEM_SolverRun";

    Gui::MenuItem* results = new Gui::MenuItem;
    root->insertItem(item, results);
    results->setCommand("&Results");
    *results << "FEM_ResultsPurge"
             << "FEM_ResultShow";
#ifdef FC_USE_VTK
    *results << "Separator"
             << "FEM_PostApplyChanges"
             << "FEM_PostPipelineFromResult"
             << "Separator"
             << "FEM_PostFilterWarp"
             << "FEM_PostFilterClipScalar"
             << "FEM_PostFilterCutFunction"
             << "FEM_PostFilterClipRegion"
             << "FEM_PostFilterContours"
             << "FEM_PostFilterDataAlongLine"
             << "FEM_PostFilterLinearizedStresses"
             << "FEM_PostFilterDataAtPoint"
             << "Separator"
             << "FEM_PostCreateFunctions";
#endif

    Gui::MenuItem* utils = new Gui::MenuItem;
    root->insertItem(item, utils);
    utils->setCommand("Utilities");
    *utils << "FEM_ClippingPlaneAdd"
           << "FEM_ClippingPlaneRemoveAll"
           << "FEM_Examples";

    return root;
}
