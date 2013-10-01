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
#include <Gui/ToolBarManager.h>
#include <Gui/MenuManager.h>

using namespace RaytracingGui;

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Workbench", "&Raytracing");
#endif

/// @namespace RaytracingGui @class Workbench
TYPESYSTEM_SOURCE(RaytracingGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
}

Workbench::~Workbench()
{
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    Gui::MenuItem* ray = new Gui::MenuItem;
    root->insertItem(item, ray);
    
    // utilities
    Gui::MenuItem* utilities = new Gui::MenuItem;
    utilities->setCommand("Utilities");
    *utilities
        << "Raytracing_WriteView" 
        << "Raytracing_WriteCamera" 
        << "Raytracing_WritePart";
    
    ray->setCommand("&Raytracing");
    *ray
        << utilities
        << "Raytracing_NewPovrayProject"
        << "Raytracing_NewLuxProject" 
        << "Raytracing_NewPartSegment" 
        << "Raytracing_ResetCamera"
        << "Raytracing_ExportProject"
        << "Raytracing_Render"; 

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* ray = new Gui::ToolBarItem(root);
    ray->setCommand("Raytracing tools");
    *ray
        << "Raytracing_NewPovrayProject"
        << "Raytracing_NewLuxProject" 
        << "Raytracing_NewPartSegment" 
        << "Raytracing_ResetCamera"
        << "Raytracing_ExportProject"
        << "Raytracing_Render";
    return root;
}
