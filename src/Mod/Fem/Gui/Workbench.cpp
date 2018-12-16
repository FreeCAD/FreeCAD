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
# include <qobject.h>
#endif

#include "Workbench.h"
#include <Gui/ToolBarManager.h>
#include <Gui/MenuManager.h>


using namespace FemGui;

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Workbench", "FEM");
    qApp->translate("Workbench", "&FEM");
    qApp->translate("Workbench", "Model");
    qApp->translate("Workbench", "M&odel");
    qApp->translate("Workbench", "Mechanical Constraints");
    qApp->translate("Workbench", "&Mechanical Constraints");
    qApp->translate("Workbench", "Thermal Constraints");
    qApp->translate("Workbench", "&Thermal Constraints");
    qApp->translate("Workbench", "Mesh");
    qApp->translate("Workbench", "M&esh");
    qApp->translate("Workbench", "Fluid Constraints");
    qApp->translate("Workbench", "&Fluid Constraints");
    qApp->translate("Workbench", "Electrostatic Constraints");
    qApp->translate("Workbench", "&Electrostatic Constraints");
    qApp->translate("Workbench", "Solve");
    qApp->translate("Workbench", "&Solve");
    qApp->translate("Workbench", "Results");
    qApp->translate("Workbench", "&Results");
    qApp->translate("Workbench", "Materials");
    qApp->translate("Workbench", "&Element Geometry");
    qApp->translate("Workbench", "Utilities");
#endif

/// @namespace FemGui @class Workbench
TYPESYSTEM_SOURCE(FemGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
}

Workbench::~Workbench()
{
}

void Workbench::setupContextMenu(const char* recipient, Gui::MenuItem* item) const
{
     StdWorkbench::setupContextMenu( recipient, item );
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
           << "FEM_MaterialEditor"
           << "Separator"
           << "FEM_ElementGeometry1D"
           << "FEM_ElementRotation1D"
           << "FEM_ElementGeometry2D"
           << "FEM_ElementFluid1D";

    Gui::ToolBarItem* mech = new Gui::ToolBarItem(root);
    mech->setCommand("Mechanical Constraints");
    *mech << "FEM_ConstraintFixed"
          << "FEM_ConstraintDisplacement"
          << "FEM_ConstraintPlaneRotation"
          << "FEM_ConstraintContact"
          << "FEM_ConstraintTransform"
          << "Separator"
          << "FEM_ConstraintForce"
          << "FEM_ConstraintPressure"
          << "FEM_ConstraintSelfWeight";

    Gui::ToolBarItem* thermal = new Gui::ToolBarItem(root);
    thermal->setCommand("Thermal Constraints");
    *thermal << "FEM_ConstraintInitialTemperature"
             << "Separator"
             << "FEM_ConstraintTemperature"
             << "FEM_ConstraintHeatflux";

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

    Gui::ToolBarItem* fluid = new Gui::ToolBarItem(root);
    fluid->setCommand("Fluid Constraints");
    *fluid << "FEM_ConstraintInitialFlowVelocity"
           << "Separator"
           << "FEM_ConstraintFluidBoundary"
           << "FEM_ConstraintFlowVelocity";

    Gui::ToolBarItem* electrostat = new Gui::ToolBarItem(root);
    electrostat->setCommand("Electrostatic Constraints");
    *electrostat << "FEM_ConstraintElectrostaticPotential";

     Gui::ToolBarItem* solve = new Gui::ToolBarItem(root);
     solve->setCommand("Solve");
     *solve << "FEM_SolverCalculixCxxtools"
           << "FEM_SolverCalculiX"
           << "FEM_SolverElmer"
           << "Separator"
           << "FEM_EquationHeat"
           << "FEM_EquationElasticity"
           << "FEM_EquationFluxsolver"
           << "FEM_EquationElectrostatic"
           << "FEM_EquationFlow"
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
              << "FEM_PostCreateWarpVectorFilter"
              << "FEM_PostCreateScalarClipFilter"
              << "FEM_PostCreateCutFilter"
              << "FEM_PostCreateClipFilter"
              << "FEM_PostCreateDataAlongLineFilter"
              << "FEM_PostCreateLinearizedStressesFilter"
              << "FEM_PostCreateDataAtPointFilter"
              << "Separator"
              << "FEM_PostCreateFunctions";
#endif

     Gui::ToolBarItem* utils = new Gui::ToolBarItem(root);
     utils->setCommand("Results");
     *utils << "FEM_ClippingPlaneAdd"
            << "FEM_ClippingPlaneRemoveAll";

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
              << "FEM_MaterialEditor";

    Gui::MenuItem* elec = new Gui::MenuItem;
    elec->setCommand("&Electrostatic Constraints");
    *elec << "FEM_ConstraintElectrostaticPotential";

    Gui::MenuItem* elegeom = new Gui::MenuItem;
    elegeom->setCommand("&Element Geometry");
    *elegeom << "FEM_ElementGeometry1D"
             << "FEM_ElementRotation1D"
             << "FEM_ElementGeometry2D"
             << "FEM_ElementFluid1D";

    Gui::MenuItem* mech = new Gui::MenuItem;
    mech->setCommand("&Mechanical Constraints");
    *mech << "FEM_ConstraintFixed"
          << "FEM_ConstraintDisplacement"
          << "FEM_ConstraintPlaneRotation"
          << "FEM_ConstraintContact"
          << "FEM_ConstraintTransform"
          << "Separator"
          << "FEM_ConstraintForce"
          << "FEM_ConstraintPressure"
          << "FEM_ConstraintSelfWeight"
          << "Separator"
          << "FEM_ConstraintBearing"
          << "FEM_ConstraintGear"
          << "FEM_ConstraintPulley";

    Gui::MenuItem* thermal = new Gui::MenuItem;
    thermal->setCommand("&Thermal Constraints");
    *thermal << "FEM_ConstraintInitialTemperature"
             << "Separator"
             << "FEM_ConstraintHeatflux"
             << "FEM_ConstraintTemperature"
             << "FEM_ConstraintBodyHeatSource";

    Gui::MenuItem* fluid = new Gui::MenuItem;
    fluid->setCommand("&Fluid Constraints");
    *fluid << "FEM_ConstraintInitialFlowVelocity"
           << "Separator"
           << "FEM_ConstraintFluidBoundary"
           << "FEM_ConstraintFlowVelocity";

    Gui::MenuItem* model = new Gui::MenuItem;
    root->insertItem(item, model);
    model->setCommand("M&odel");
    *model << "FEM_Analysis"
           << "Separator"
           << material
           << elegeom
           << "Separator"
           << elec
           << fluid
           << mech
           << thermal;

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
           << "FEM_CreateNodesSet"
           << "FEM_FEMMesh2Mesh";

    Gui::MenuItem* solve = new Gui::MenuItem;
    root->insertItem(item, solve);
    solve->setCommand("&Solve");
    *solve << "FEM_SolverCalculixCxxtools"
           << "FEM_SolverCalculiX"
           << "FEM_SolverElmer"
           << "FEM_SolverZ88"
           << "Separator"
           << "FEM_EquationHeat"
           << "FEM_EquationElasticity"
           << "FEM_EquationElectrostatic"
           << "FEM_EquationFluxsolver"
           << "FEM_EquationFlow"
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
             << "FEM_PostCreateWarpVectorFilter"
             << "FEM_PostCreateScalarClipFilter"
             << "FEM_PostCreateCutFilter"
             << "FEM_PostCreateClipFilter"
             << "FEM_PostCreateDataAlongLineFilter"
             << "FEM_PostCreateLinearizedStressesFilter"
             << "FEM_PostCreateDataAtPointFilter"
             << "Separator"
             << "FEM_PostCreateFunctions";
#endif

    Gui::MenuItem* utils = new Gui::MenuItem;
    root->insertItem(item, utils);
    utils->setCommand("Utilities");
    *utils << "FEM_ClippingPlaneAdd"
           << "FEM_ClippingPlaneRemoveAll";

    return root;
}
