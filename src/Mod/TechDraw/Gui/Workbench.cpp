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
    *draw << "Separator";
    *draw << "TechDraw_NewView";
//    *draw << "TechDraw_NewMulti";     //deprecated
    *draw << "TechDraw_ProjGroup";
    *draw << "TechDraw_NewViewSection";
    *draw << "TechDraw_NewViewDetail";
    *draw << "Separator";
    *draw << "TechDraw_DraftView";
    *draw << "TechDraw_ArchView";
    *draw << "TechDraw_Spreadsheet";
    *draw << "Separator";
    *draw << "TechDraw_Clip";
    *draw << "TechDraw_ClipPlus";
    *draw << "TechDraw_ClipMinus";
    *draw << "Separator";
    *draw << "TechDraw_NewLengthDimension";
    *draw << "TechDraw_NewDistanceXDimension";
    *draw << "TechDraw_NewDistanceYDimension";
    *draw << "TechDraw_NewRadiusDimension";
    *draw << "TechDraw_NewDiameterDimension";
    *draw << "TechDraw_NewAngleDimension";
    *draw << "TechDraw_NewAngle3PtDimension";
    *draw << "TechDraw_LinkDimension";
    *draw << "TechDraw_NewBalloon";
    *draw << "Separator";
    *draw << "TechDraw_ExportPage";
    *draw << "TechDraw_ExportPageDxf";
    *draw << "Separator";
    *draw << "TechDraw_NewHatch";
    *draw << "TechDraw_NewGeomHatch";
    *draw << "TechDraw_Symbol";
    *draw << "TechDraw_Image";
    *draw << "TechDraw_ToggleFrame";
//    *decor << "TechDraw_RedrawPage";
    *draw << "TechDraw_Annotation";
    *draw << "TechDraw_LeaderLine";
    *draw << "TechDraw_RichAnno";
    *draw << "TechDraw_CosmeticVertex";
    *draw << "TechDraw_Midpoints";
    *draw << "TechDraw_Quadrant";
    *draw << "TechDraw_FaceCenterLine";
    *draw << "TechDraw_2LineCenterLine";
    *draw << "TechDraw_2PointCenterLine";
    *draw << "TechDraw_CosmeticEraser";
    *draw << "TechDraw_DecorateLine";
    *draw << "TechDraw_ShowAll";

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
//    *views << "TechDraw_NewMulti";    //deprecated
    *views << "TechDraw_ProjGroup";
    *views << "TechDraw_NewViewSection";
    *views << "TechDraw_NewViewDetail";
    *views << "TechDraw_DraftView";
    *views << "TechDraw_ArchView";
    *views << "TechDraw_Spreadsheet";

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
    *dims << "TechDraw_NewAngle3PtDimension";
    *dims << "TechDraw_LinkDimension";
    *dims << "TechDraw_NewBalloon";
//    *dims << "TechDraw_NewDimension"

    Gui::ToolBarItem *file = new Gui::ToolBarItem(root);
    file->setCommand("TechDraw File Access");
    *file << "TechDraw_ExportPage";
    *file << "TechDraw_ExportPageDxf";

    Gui::ToolBarItem *decor = new Gui::ToolBarItem(root);
    decor->setCommand("TechDraw Decoration");
    *decor << "TechDraw_NewHatch";
    *decor << "TechDraw_NewGeomHatch";
    *decor << "TechDraw_Symbol";
    *decor << "TechDraw_Image";
    *decor << "TechDraw_ToggleFrame";
//    *decor << "TechDraw_RedrawPage";

    Gui::ToolBarItem *anno = new Gui::ToolBarItem(root);
    anno->setCommand("TechDraw Annotation");
    *anno << "TechDraw_Annotation";
    *anno << "TechDraw_LeaderLine";
    *anno << "TechDraw_RichAnno";
    *anno << "TechDraw_CosmeticVertexGrp";
    *anno << "TechDraw_CenterLineGrp";
//    *anno << "TechDraw_FaceCenterLine";
//    *anno << "TechDraw_2LineCenterLine";
//    *anno << "TechDraw_2PointCenterLine";
    *anno << "TechDraw_CosmeticEraser";
    *anno << "TechDraw_DecorateLine";
    *anno << "TechDraw_ShowAll";
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
//    *views << "TechDraw_NewMulti";    //deprecated
    *views << "TechDraw_ProjGroup";
    *views << "TechDraw_NewViewSection";
    *views << "TechDraw_NewViewDetail";
    *views << "TechDraw_DraftView";
    *views << "TechDraw_Spreadsheet";

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
    *dims << "TechDraw_NewAngle3PtDimension";
    *dims << "TechDraw_LinkDimension";
    *dims << "TechDraw_NewBalloon";
//    *dims << "TechDraw_NewDimension";

    Gui::ToolBarItem *file = new Gui::ToolBarItem(root);
    file->setCommand("TechDraw File Access");
    *file << "TechDraw_ExportPage";
    *file << "TechDraw_ExportPageDxf";
 
    Gui::ToolBarItem *decor = new Gui::ToolBarItem(root);
    decor->setCommand("TechDraw Decoration");
    *decor << "TechDraw_NewHatch";
    *decor << "TechDraw_NewGeomHatch";
    *decor << "TechDraw_Symbol";
    *decor << "TechDraw_Image";
    *decor << "TechDraw_ToggleFrame";

    Gui::ToolBarItem *anno = new Gui::ToolBarItem(root);
    anno->setCommand("TechDraw Annotation");
    *anno << "TechDraw_Annotation";
    *anno << "TechDraw_LeaderLine";
    *anno << "TechDraw_RichAnno";
    *anno << "TechDraw_CosmeticVertexGrp";
    *anno << "TechDraw_CenterLineGrp";
//    *anno << "TechDraw_FaceCenterLine";
//    *anno << "TechDraw_2LineCenterLine";
//    *anno << "TechDraw_2PointCenterLine";
    *anno << "TechDraw_CosmeticEraser";
    *anno << "TechDraw_DecorateLine";
    *anno << "TechDraw_ShowAll";

    return root;
}
