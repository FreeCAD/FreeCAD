/***************************************************************************
 *   Copyright (c) 2014 Nathan Miller <Nathan.A.Mill[at]gmail.com>         *
 *   Copyright (c) 2014 Balázs Bámer                                       *
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
