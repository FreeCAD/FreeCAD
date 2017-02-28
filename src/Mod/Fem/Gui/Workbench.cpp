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
           << "FEM_ClearMesh"
           << "FEM_PrintMeshInfo";
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* fem = new Gui::ToolBarItem(root);
    fem->setCommand("FEM");
    *fem << "FEM_FemMesh2Mesh"
         << "FEM_Analysis"
         << "FEM_SolverCalculix"
         // << "FEM_SolverZ88"
         << "Separator"
         << "FEM_MeshNetgenFromShape"
         << "FEM_MeshGmshFromShape"
         << "FEM_MeshRegion"
         << "FEM_MeshGroup"
         //<< "FEM_CreateNodesSet"
         << "Separator"
         << "FEM_MaterialSolid"
         << "FEM_MaterialFluid"
         << "FEM_MaterialMechanicalNonlinear"
         << "FEM_BeamSection"
         << "FEM_ShellThickness"
         << "FEM_FluidSection"
         << "Separator"
         << "FEM_ConstraintFixed"
         << "FEM_ConstraintDisplacement"
         << "FEM_ConstraintPlaneRotation"
         << "FEM_ConstraintContact"
         << "FEM_ConstraintTransform"
         << "Separator"
         << "FEM_ConstraintSelfWeight"
         << "FEM_ConstraintForce"
         << "FEM_ConstraintPressure"
         //<< "Separator"
         //<< "FEM_ConstraintBearing"
         //<< "FEM_ConstraintGear"
         //<< "FEM_ConstraintPulley"
         //<< "FEM_ConstraintFluidBoundary"
         << "Separator"
         << "FEM_ConstraintTemperature"
         << "FEM_ConstraintHeatflux"
         << "FEM_ConstraintInitialTemperature"
         << "Separator"
         << "FEM_ControlSolver"
         << "FEM_RunSolver"
         << "Separator"
         << "FEM_PurgeResults"
         << "FEM_ShowResult";

#ifdef FC_USE_VTK
     Gui::ToolBarItem* post = new Gui::ToolBarItem(root);
     post->setCommand("Post Processing");
     *post  << "FEM_PostApplyChanges"
            << "FEM_PostPipelineFromResult"
            << "Separator"
            << "FEM_PostCreateClipFilter"
            << "FEM_PostCreateScalarClipFilter"
            << "FEM_PostCreateCutFilter"
            << "FEM_PostCreateWarpVectorFilter"
            << "FEM_PostCreateDataAlongLineFilter"
            << "FEM_PostCreateLinearizedStressesFilter"
            << "Separator"
            << "FEM_PostCreateFunctions";
#endif

    return root;
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    Gui::MenuItem* util = new Gui::MenuItem;
    util->setCommand("&Utilities");
    *util << "FEM_FemMesh2Mesh";

    Gui::MenuItem* fem = new Gui::MenuItem;
    root->insertItem(item, fem);
    fem->setCommand("&FEM");
    *fem << util
         << "Separator"
         << "FEM_Analysis"
         << "FEM_SolverCalculix"
         << "FEM_SolverZ88"
         << "Separator"
         << "FEM_MeshNetgenFromShape"
         << "FEM_MeshGmshFromShape"
         << "FEM_MeshRegion"
         << "FEM_MeshGroup"
         << "FEM_CreateNodesSet"
         << "Separator"
         << "FEM_MaterialSolid"
         << "FEM_MaterialFluid"
         << "FEM_MaterialMechanicalNonlinear"
         << "FEM_BeamSection"
         << "FEM_ShellThickness"
         << "FEM_FluidSection"
         << "Separator"
         << "FEM_ConstraintFixed"
         << "FEM_ConstraintDisplacement"
         << "FEM_ConstraintPlaneRotation"
         << "FEM_ConstraintContact"
         << "FEM_ConstraintTransform"
         << "Separator"
         << "FEM_ConstraintSelfWeight"
         << "FEM_ConstraintForce"
         << "FEM_ConstraintPressure"
         << "Separator"
         << "FEM_ConstraintBearing"
         << "FEM_ConstraintGear"
         << "FEM_ConstraintPulley"
         << "Separator"
         << "FEM_ConstraintFluidBoundary"
         << "Separator"
         << "FEM_ConstraintTemperature"
         << "FEM_ConstraintHeatflux"
         << "FEM_ConstraintInitialTemperature"
         << "Separator"
         << "FEM_ControlSolver"
         << "FEM_RunSolver"
         << "Separator"
         << "FEM_PurgeResults"
         << "FEM_ShowResult";

    return root;
}
