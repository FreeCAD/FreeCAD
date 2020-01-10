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

	// dimensions
	Gui::MenuItem* dimensions = new Gui::MenuItem;
	dimensions->setCommand("Dimensions");
	*dimensions << "TechDraw_LengthDimension" << "TechDraw_HorizontalDimension" << "TechDraw_VerticalDimension"
		<< "TechDraw_RadiusDimension" << "TechDraw_DiameterDimension" << "TechDraw_AngleDimension"
		<< "TechDraw_HorizontalExtentDimension" << "TechDraw_VerticalExtentDimension" << "TechDraw_LinkDimension";

	// annotations
	Gui::MenuItem* annotations = new Gui::MenuItem;
	annotations->setCommand("Annotations");
	*annotations << "TechDraw_Annotation" << "TechDraw_RichTextAnnotation" << "TechDraw_Balloon";

	// lines
	Gui::MenuItem* lines = new Gui::MenuItem;
	lines->setCommand("Add Lines");
	*lines << "TechDraw_LeaderLine" << "TechDraw_FaceCenterLine"
		<< "TechDraw_2LineCenterLine" << "TechDraw_2PointCenterLine";

	// vertices
	Gui::MenuItem* vertices = new Gui::MenuItem;
	vertices->setCommand("Add Vertices");
	*vertices << "TechDraw_CosmeticVertex" << "TechDraw_Midpoints"
		<< "TechDraw_Quadrant";

	// main menu
	draw->setCommand("TechDraw");
    *draw << "TechDraw_PageDefault";
    *draw << "TechDraw_PageTemplate";
    *draw << "TechDraw_RedrawPage";
    *draw << "Separator";
    *draw << "TechDraw_View";
    *draw << "TechDraw_ActiveView";
    *draw << "TechDraw_ProjectionGroup";
    *draw << "TechDraw_SectionView";
    *draw << "TechDraw_DetailView";
    *draw << "Separator";
    *draw << "TechDraw_DraftView";
    *draw << "TechDraw_ArchView";
    *draw << "TechDraw_SpreadsheetView";
    *draw << "Separator";
    *draw << "TechDraw_ClipGroup";
    *draw << "TechDraw_ClipGroupAdd";
    *draw << "TechDraw_ClipGroupRemove";
    *draw << "Separator";
	*draw << dimensions;
    *draw << "Separator";
    *draw << "TechDraw_ExportPageSVG";
    *draw << "TechDraw_ExportPageDXF";
    *draw << "Separator";
    *draw << "TechDraw_Hatch";
    *draw << "TechDraw_GeometricHatch";
    *draw << "TechDraw_Symbol";
    *draw << "TechDraw_Image";
    *draw << "TechDraw_ToggleFrame";
    *draw << "Separator";
    *draw << annotations;
	*draw << lines;
    *draw << vertices;
    *draw << "TechDraw_CosmeticEraser";
    *draw << "TechDraw_DecorateLine";
    *draw << "TechDraw_ShowAll";
    *draw << "TechDraw_WeldSymbol";
    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* pages = new Gui::ToolBarItem(root);
    pages->setCommand("TechDraw Pages");
    *pages << "TechDraw_PageDefault";
    *pages << "TechDraw_PageTemplate";
    *pages << "TechDraw_RedrawPage";

    Gui::ToolBarItem *views = new Gui::ToolBarItem(root);
    views->setCommand("TechDraw Views");
    *views << "TechDraw_View";
    *views << "TechDraw_ActiveView";
    *views << "TechDraw_ProjectionGroup";
    *views << "TechDraw_SectionView";
    *views << "TechDraw_DetailView";
    *views << "TechDraw_DraftView";
    *views << "TechDraw_ArchView";
    *views << "TechDraw_SpreadsheetView";

    Gui::ToolBarItem *clips = new Gui::ToolBarItem(root);
    clips->setCommand("TechDraw Clips");
    *clips << "TechDraw_ClipGroup";
    *clips << "TechDraw_ClipGroupAdd";
    *clips << "TechDraw_ClipGroupRemove";

    Gui::ToolBarItem *dims = new Gui::ToolBarItem(root);
    dims->setCommand("TechDraw Dimensions");
    *dims << "TechDraw_LengthDimension";
    *dims << "TechDraw_HorizontalDimension";
    *dims << "TechDraw_VerticalDimension";
    *dims << "TechDraw_RadiusDimension";
    *dims << "TechDraw_DiameterDimension";
    *dims << "TechDraw_AngleDimension";
    *dims << "TechDraw_3PtAngleDimension";
    *dims << "TechDraw_ExtentGroup";
//    *dims << "TechDraw_HorizontalExtentDimension";
//    *dims << "TechDraw_VerticalExtentDimension";
    *dims << "TechDraw_LinkDimension";
    *dims << "TechDraw_Balloon";
//    *dims << "TechDraw_Dimension"

    Gui::ToolBarItem *file = new Gui::ToolBarItem(root);
    file->setCommand("TechDraw File Access");
    *file << "TechDraw_ExportPageSVG";
    *file << "TechDraw_ExportPageDXF";

    Gui::ToolBarItem *decor = new Gui::ToolBarItem(root);
    decor->setCommand("TechDraw Decoration");
    *decor << "TechDraw_Hatch";
    *decor << "TechDraw_GeometricHatch";
    *decor << "TechDraw_Symbol";
    *decor << "TechDraw_Image";
    *decor << "TechDraw_ToggleFrame";

    Gui::ToolBarItem *anno = new Gui::ToolBarItem(root);
    anno->setCommand("TechDraw Annotation");
    *anno << "TechDraw_Annotation";
    *anno << "TechDraw_LeaderLine";
    *anno << "TechDraw_RichTextAnnotation";
    *anno << "TechDraw_CosmeticVertexGroup";
    *anno << "TechDraw_CenterLineGroup";
//    *anno << "TechDraw_FaceCenterLine";
//    *anno << "TechDraw_2LineCenterLine";
//    *anno << "TechDraw_2PointCenterLine";
    *anno << "TechDraw_CosmeticEraser";
    *anno << "TechDraw_DecorateLine";
    *anno << "TechDraw_ShowAll";
    *anno << "TechDraw_WeldSymbol";
    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    Gui::ToolBarItem *pages = new Gui::ToolBarItem(root);
    pages->setCommand("TechDraw Pages");
    *pages << "TechDraw_PageDefault";
    *pages << "TechDraw_PageTemplate";
    *pages << "TechDraw_RedrawPage";

    Gui::ToolBarItem *views = new Gui::ToolBarItem(root);
    views->setCommand("Views");
    *views << "TechDraw_View";
    *views << "TechDraw_ActiveView";
//    *views << "TechDraw_NewMulti";    //deprecated
    *views << "TechDraw_ProjectionGroup";
    *views << "TechDraw_SectionView";
    *views << "TechDraw_DetailView";
    *views << "TechDraw_DraftView";
    *views << "TechDraw_SpreadsheetView";

    Gui::ToolBarItem *clips = new Gui::ToolBarItem(root);
    clips->setCommand("TechDraw Clips");
    *clips << "TechDraw_ClipGroup";
    *clips << "TechDraw_ClipGroupAdd";
    *clips << "TechDraw_ClipGroupRemove";

    Gui::ToolBarItem *dims = new Gui::ToolBarItem(root);
    dims->setCommand("TechDraw Dimensions");
    *dims << "TechDraw_LengthDimension";
    *dims << "TechDraw_HorizontalDimension";
    *dims << "TechDraw_VerticalDimension";
    *dims << "TechDraw_RadiusDimension";
    *dims << "TechDraw_DiameterDimension";
    *dims << "TechDraw_AngleDimension";
    *dims << "TechDraw_3PtAngleDimension";
    *dims << "TechDraw_ExtentGroup";
//    *dims << "TechDraw_HorizontalExtentDimension";
//    *dims << "TechDraw_VerticalExtentDimension";
    *dims << "TechDraw_LinkDimension";
    *dims << "TechDraw_Balloon";
//    *dims << "TechDraw_Dimension";

    Gui::ToolBarItem *file = new Gui::ToolBarItem(root);
    file->setCommand("TechDraw File Access");
    *file << "TechDraw_ExportPageSVG";
    *file << "TechDraw_ExportPageDXF";
 
    Gui::ToolBarItem *decor = new Gui::ToolBarItem(root);
    decor->setCommand("TechDraw Decoration");
    *decor << "TechDraw_Hatch";
    *decor << "TechDraw_GeometricHatch";
    *decor << "TechDraw_Symbol";
    *decor << "TechDraw_Image";
    *decor << "TechDraw_ToggleFrame";

    Gui::ToolBarItem *anno = new Gui::ToolBarItem(root);
    anno->setCommand("TechDraw Annotation");
    *anno << "TechDraw_Annotation";
    *anno << "TechDraw_LeaderLine";
    *anno << "TechDraw_RichTextAnnotation";
    *anno << "TechDraw_CosmeticVertexGroup";
    *anno << "TechDraw_CenterLineGroup";
//    *anno << "TechDraw_FaceCenterLine";
//    *anno << "TechDraw_2LineCenterLine";
//    *anno << "TechDraw_2PointCenterLine";
    *anno << "TechDraw_CosmeticEraser";
    *anno << "TechDraw_DecorateLine";
    *anno << "TechDraw_ShowAll";
    *anno << "TechDraw_WeldSymbol";

    return root;
}
