/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>
#include <Gui/Selection.h>

#include "../App/MeshFeature.h"

using namespace MeshGui;

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Workbench", "Analyze");
    qApp->translate("Workbench", "Boolean");
    qApp->translate("Workbench", "&Meshes");
    qApp->translate("Workbench", "Mesh tools");
#endif

/// @namespace MeshGui @class Workbench
TYPESYSTEM_SOURCE(MeshGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
}

Workbench::~Workbench()
{
}

void Workbench::setupContextMenu(const char* recipient,Gui::MenuItem* item) const
{
    StdWorkbench::setupContextMenu( recipient, item );
    if (Gui::Selection().countObjectsOfType(Mesh::Feature::getClassTypeId()) > 0)
    {
        *item << "Separator" << "Mesh_Import" << "Mesh_Export" << "Mesh_VertexCurvature";
    }
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");
    Gui::MenuItem* mesh = new Gui::MenuItem;
    root->insertItem( item, mesh );

    // analyze
    Gui::MenuItem* analyze = new Gui::MenuItem;
    analyze->setCommand("Analyze");
    *analyze << "Mesh_Evaluation" << "Mesh_EvaluateFacet" << "Mesh_CurvatureInfo" << "Separator" 
             << "Mesh_EvaluateSolid" << "Mesh_BoundingBox";

    // boolean
    Gui::MenuItem* boolean = new Gui::MenuItem;
    boolean->setCommand("Boolean");
    *boolean << "Mesh_Union" << "Mesh_Intersection" << "Mesh_Difference";
 
    mesh->setCommand("&Meshes");
    *mesh << "Mesh_Import" << "Mesh_Export" << "Mesh_FromGeometry" << "Separator"
          << analyze << "Mesh_HarmonizeNormals" << "Mesh_FlipNormals" << "Separator" 
          << "Mesh_FillupHoles" << "Mesh_FillInteractiveHole" << "Mesh_RemoveComponents"
          << "Mesh_RemoveCompByHand" << "Mesh_AddFacet" << "Mesh_Smoothing" << "Separator" 
          << "Mesh_BuildRegularSolid" << boolean << "Separator" << "Mesh_PolySelect" << "Mesh_PolyCut"
          << "Mesh_PolySplit" << "Mesh_PolySegm" << /*"Mesh_ToolMesh" <<*/ "Mesh_VertexCurvature";
    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* mesh = new Gui::ToolBarItem(root);
    mesh->setCommand("Mesh tools");
    *mesh << "Mesh_Import" << "Mesh_Export" << "Separator" << "Mesh_PolyCut" << "Mesh_VertexCurvature";
    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    // Mesh tools
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    Gui::ToolBarItem* mesh;

    mesh = new Gui::ToolBarItem( root );
    mesh->setCommand("Mesh tools");
    *mesh << "Mesh_Import" << "Mesh_Export" << "Mesh_PolyCut";

    mesh = new Gui::ToolBarItem( root );
    mesh->setCommand("Mesh test suite");
    *mesh << "Mesh_Demolding" << "Mesh_Transform" << "Separator" ;

    return root;
}

