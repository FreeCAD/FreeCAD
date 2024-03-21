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

#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>

#include "Workbench.h"


using namespace PointsGui;

#if 0  // needed for Qt's lupdate utility
    qApp->translate("Workbench", "Points tools");
    qApp->translate("Workbench", "&Points");
#endif

/// @namespace PointsGui @class Workbench
TYPESYSTEM_SOURCE(PointsGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench() = default;

Workbench::~Workbench() = default;

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* pnt = new Gui::ToolBarItem(root);
    pnt->setCommand("Points tools");
    *pnt << "Points_Import" << "Points_Export" << "Separator" << "Points_Convert"
         << "Points_Structure" << "Points_Merge" << "Points_PolyCut";
    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    // point tools
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    Gui::ToolBarItem* pnt = new Gui::ToolBarItem(root);
    pnt->setCommand("Points tools");
    *pnt << "Points_Import" << "Points_Export" << "Points_Convert" << "Points_Structure"
         << "Points_Merge";
    return root;
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");
    Gui::MenuItem* pnts = new Gui::MenuItem;
    root->insertItem(item, pnts);

    pnts->setCommand("&Points");
    *pnts << "Points_Convert" << "Points_Structure" << "Separator" << "Points_Import"
          << "Points_Export" << "Separator" << "Points_PolyCut" << "Points_Merge";
    return root;
}
