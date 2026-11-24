// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Nathan Miller
// SPDX-FileCopyrightText: 2014 Balázs Bámer
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>

#include "Workbench.h"


using namespace SurfaceGui;

/// @namespace SurfaceGui @class Workbench
TYPESYSTEM_SOURCE(SurfaceGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench() = default;

Workbench::~Workbench() = default;

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    Gui::MenuItem* surface = new Gui::MenuItem;
    root->insertItem(item, surface);
    surface->setCommand("S&urface");
    *surface << "Surface_Filling"
             << "Surface_GeomFillSurface"
             << "Surface_Sections"
             << "Surface_ExtendFace"
             << "Surface_CurveOnMesh"
             << "Surface_BlendCurve";
    /*
     *surface << "Surface_Cut";
     */

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();

    Gui::ToolBarItem* surface = new Gui::ToolBarItem(root);
    surface->setCommand("Surface");
    *surface << "Surface_Filling"
             << "Surface_GeomFillSurface"
             << "Surface_Sections"
             << "Surface_ExtendFace"
             << "Surface_CurveOnMesh"
             << "Surface_BlendCurve";
    /*
     *surface << "Surface_Cut";
     */

    return root;
}
