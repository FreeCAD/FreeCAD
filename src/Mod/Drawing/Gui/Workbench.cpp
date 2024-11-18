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


using namespace DrawingGui;

#if 0  // needed for Qt's lupdate utility
    qApp->translate("Workbench", "Drawing");
#endif

/// @namespace DrawingGui @class Workbench
TYPESYSTEM_SOURCE(DrawingGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{}

Workbench::~Workbench()
{}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    Gui::MenuItem* part = new Gui::MenuItem;
    root->insertItem(item, part);
    part->setCommand("Drawing");
    *part << "Drawing_Open";
    //*part << "Drawing_NewA3Landscape";
    *part << "Drawing_NewPage";
    *part << "Drawing_NewView";
    *part << "Drawing_OrthoViews";
    *part << "Drawing_OpenBrowserView";
    *part << "Drawing_Annotation";
    *part << "Drawing_Clip";
    *part << "Drawing_Symbol";
    *part << "Drawing_DraftView";
    *part << "Drawing_SpreadsheetView";
    *part << "Drawing_ExportPage";
    *part << "Separator";
    *part << "Drawing_ProjectShape";

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* part = new Gui::ToolBarItem(root);
    part->setCommand("Drawing");
    *part << "Drawing_Open";
    //*part << "Drawing_NewA3Landscape";
    *part << "Drawing_NewPage";
    *part << "Drawing_NewView";
    *part << "Drawing_OrthoViews";
    *part << "Drawing_OpenBrowserView";
    *part << "Drawing_Annotation";
    *part << "Drawing_Clip";
    *part << "Drawing_Symbol";
    *part << "Drawing_DraftView";
    *part << "Drawing_SpreadsheetView";
    *part << "Drawing_ExportPage";
    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    // Part tools
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    Gui::ToolBarItem* img = new Gui::ToolBarItem(root);
    img->setCommand("I/O");
    *img << "Drawing_Open";
    img = new Gui::ToolBarItem(root);
    img->setCommand("Drawing types");
    //*img << "Drawing_NewA3Landscape";
    *img << "Drawing_NewPage";
    *img << "Drawing_OrthoViews";
    *img << "Drawing_OpenBrowserView";
    *img << "Drawing_Annotation";
    *img << "Drawing_Clip";
    *img << "Drawing_DraftView";
    img = new Gui::ToolBarItem(root);
    img->setCommand("Views");
    *img << "Drawing_NewView";
    return root;
}
