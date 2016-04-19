/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

using namespace TechDrawGui;

#if 0 // needed for Qt's lupdate utility
//    qApp->translate("Workbench", "TechDraw");
    qApp->translate("Workbench", "TechDraw Pages");
    qApp->translate("Workbench", "TechDraw Views");
    qApp->translate("Workbench", "TechDraw Dimensions");
#endif

/// @namespace TechDrawGui @class Workbench
TYPESYSTEM_SOURCE(TechDrawGui::Workbench, Gui::StdWorkbench)

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

    Gui::MenuItem* draw = new Gui::MenuItem;
    root->insertItem(item, draw);
    draw->setCommand("TechDraw");
    *draw << "TechDraw_NewPageDef";
    *draw << "TechDraw_NewPage";
    *draw << "TechDraw_NewView";
    *draw << "TechDraw_ProjGroup";
    *draw << "TechDraw_NewViewSection";
    *draw << "TechDraw_Annotation";
    *draw << "TechDraw_Symbol";
    *draw << "TechDraw_Clip";
    *draw << "TechDraw_ClipPlus";
    *draw << "TechDraw_ClipMinus";
    *draw << "TechDraw_NewDimension";
    *draw << "TechDraw_DraftView";
    *draw << "TechDraw_ExportPage";
    //*draw << "TechDraw_Open";
    //*part << "TechDraw_NewA3Landscape";
    //*part << "TechDraw_OpenBrowserView";
    //*part << "TechDraw_DraftView";
    //*draw << "Separator";
    //*draw << "TechDraw_ProjectShape";

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* pages = new Gui::ToolBarItem(root);
    pages->setCommand("TechDraw Pages");
    *pages << "TechDraw_NewPageDef";
    *pages << "TechDraw_NewPage";

    Gui::ToolBarItem *views = new Gui::ToolBarItem(root);
    views->setCommand("TechDraw Views");
    *views << "TechDraw_NewView";
    *views << "TechDraw_ProjGroup";
    *views << "TechDraw_NewViewSection";
    *views << "TechDraw_Annotation";
    *views << "TechDraw_DraftView";

    Gui::ToolBarItem *clips = new Gui::ToolBarItem(root);
    clips->setCommand("TechDraw Clips");
    *clips << "TechDraw_Clip";
    *clips << "TechDraw_ClipPlus";
    *clips << "TechDraw_ClipMinus";

    Gui::ToolBarItem *dims = new Gui::ToolBarItem(root);
    dims->setCommand("TechDraw Dimensions");
    *dims << "TechDraw_NewLengthDimension";
    *dims << "TechDraw_NewDistanceXDimension";
    *dims << "TechDraw_NewDistanceYDimension";
    *dims << "TechDraw_NewRadiusDimension";
    *dims << "TechDraw_NewDiameterDimension";
    *dims << "TechDraw_NewAngleDimension";
    *dims << "TechDraw_LinkDimension";
//    *dims << "TechDraw_NewDimension"

    Gui::ToolBarItem *file = new Gui::ToolBarItem(root);
    file->setCommand("TechDraw File Access");
    *file << "TechDraw_ExportPage";
    *file << "TechDraw_Symbol";

    Gui::ToolBarItem *decor = new Gui::ToolBarItem(root);
    decor->setCommand("TechDraw Decoration");
    *decor << "TechDraw_NewHatch";
    *decor << "TechDraw_ToggleFrame";
    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    Gui::ToolBarItem *pages = new Gui::ToolBarItem(root);
    pages->setCommand("TechDraw Pages");
    *pages << "TechDraw_NewPageDef";
    *pages << "TechDraw_NewPage";

    Gui::ToolBarItem *views = new Gui::ToolBarItem(root);
    views->setCommand("Views");
    *views << "TechDraw_NewView";
    *views << "TechDraw_ProjGroup";
    *views << "TechDraw_NewViewSection";
    *views << "TechDraw_Annotation";
    *views << "TechDraw_DraftView";

    Gui::ToolBarItem *clips = new Gui::ToolBarItem(root);
    clips->setCommand("TechDraw Clips");
    *clips << "TechDraw_Clip";
    *clips << "TechDraw_ClipPlus";
    *clips << "TechDraw_ClipMinus";

    Gui::ToolBarItem *dims = new Gui::ToolBarItem(root);
    dims->setCommand("TechDraw Dimensions");
    *dims << "TechDraw_NewLengthDimension";
    *dims << "TechDraw_NewDistanceXDimension";
    *dims << "TechDraw_NewDistanceYDimension";
    *dims << "TechDraw_NewRadiusDimension";
    *dims << "TechDraw_NewDiameterDimension";
    *dims << "TechDraw_NewAngleDimension";
    *dims << "TechDraw_LinkDimension";
//    *dims << "TechDraw_NewDimension";

    Gui::ToolBarItem *file = new Gui::ToolBarItem(root);
    file->setCommand("TechDraw File Access");
    *file << "TechDraw_ExportPage";
    *file << "TechDraw_Symbol";

    Gui::ToolBarItem *decor = new Gui::ToolBarItem(root);
    decor->setCommand("TechDraw Decoration");
    *decor << "TechDraw_NewHatch";
    *decor << "TechDraw_ToggleFrame";

    return root;
}
