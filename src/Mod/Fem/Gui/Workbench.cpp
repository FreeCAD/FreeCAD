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
           << "Fem_ClearMesh"
           << "Fem_PrintMeshInfo";
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* fem = new Gui::ToolBarItem(root);
    fem->setCommand("FEM");
    *fem << "Fem_FemMesh2Mesh"
         << "Fem_Analysis"
         << "Fem_SolverCalculix"
         // << "Fem_SolverZ88"
         << "Separator"
         << "Fem_MeshNetgenFromShape"
         << "Fem_MeshGmshFromShape"
         << "Fem_MeshRegion"
         << "Fem_MeshGroup"
         //<< "Fem_CreateNodesSet"
         << "Separator"
         << "Fem_MaterialSolid"
         << "Fem_MaterialFluid"
         << "Fem_MaterialMechanicalNonlinear"
         << "Fem_BeamSection"
         << "Fem_ShellThickness"
         << "Separator"
         << "Fem_ConstraintFixed"
         << "Fem_ConstraintDisplacement"
         << "Fem_ConstraintPlaneRotation"
         << "Fem_ConstraintContact"
         << "Fem_ConstraintTransform"
         << "Separator"
         << "Fem_ConstraintSelfWeight"
         << "Fem_ConstraintForce"
         << "Fem_ConstraintPressure"
         //<< "Separator"
         //<< "Fem_ConstraintBearing"
         //<< "Fem_ConstraintGear"
         //<< "Fem_ConstraintPulley"
         //<< "Fem_ConstraintFluidBoundary"
         << "Separator"
         << "Fem_ConstraintTemperature"
         << "Fem_ConstraintHeatflux"
         << "Fem_ConstraintInitialTemperature"
         << "Separator"
         << "Fem_ControlSolver"
         << "Fem_RunSolver"
         << "Separator"
         << "Fem_PurgeResults"
         << "Fem_ShowResult";

#ifdef FC_USE_VTK
     Gui::ToolBarItem* post = new Gui::ToolBarItem(root);
     post->setCommand("Post Processing");
     *post  << "Fem_PostApplyChanges"
            << "Fem_PostPipelineFromResult"
            << "Separator"
            << "Fem_PostCreateClipFilter"
            << "Fem_PostCreateScalarClipFilter"
            << "Fem_PostCreateCutFilter"
            << "Fem_PostCreateWarpVectorFilter"
            << "Fem_PostCreateDataAlongLineFilter"
            << "Fem_PostCreateLinearizedStressesFilter"
            << "Separator"
            << "Fem_PostCreateFunctions";
#endif

    return root;
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    Gui::MenuItem* util = new Gui::MenuItem;
    util->setCommand("&Utilities");
    *util << "Fem_FemMesh2Mesh";

    Gui::MenuItem* fem = new Gui::MenuItem;
    root->insertItem(item, fem);
    fem->setCommand("&FEM");
    *fem << util
         << "Separator"
         << "Fem_Analysis"
         << "Fem_SolverCalculix"
         << "Fem_SolverZ88"
         << "Separator"
         << "Fem_MeshNetgenFromShape"
         << "Fem_MeshGmshFromShape"
         << "Fem_MeshRegion"
         << "Fem_MeshGroup"
         << "Fem_CreateNodesSet"
         << "Separator"
         << "Fem_MaterialSolid"
         << "Fem_MaterialFluid"
         << "Fem_MaterialMechanicalNonlinear"
         << "Fem_BeamSection"
         << "Fem_ShellThickness"
         << "Separator"
         << "Fem_ConstraintFixed"
         << "Fem_ConstraintDisplacement"
         << "Fem_ConstraintPlaneRotation"
         << "Fem_ConstraintContact"
         << "Fem_ConstraintTransform"
         << "Separator"
         << "Fem_ConstraintSelfWeight"
         << "Fem_ConstraintForce"
         << "Fem_ConstraintPressure"
         << "Separator"
         << "Fem_ConstraintBearing"
         << "Fem_ConstraintGear"
         << "Fem_ConstraintPulley"
         << "Separator"
         << "Fem_ConstraintFluidBoundary"
         << "Separator"
         << "Fem_ConstraintTemperature"
         << "Fem_ConstraintHeatflux"
         << "Fem_ConstraintInitialTemperature"
         << "Separator"
         << "Fem_ControlSolver"
         << "Fem_RunSolver"
         << "Separator"
         << "Fem_PurgeResults"
         << "Fem_ShowResult";

    return root;
}
