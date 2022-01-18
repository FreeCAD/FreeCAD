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
    qApp->translate("Workbench", "Dimensions");
    qApp->translate("Workbench", "Extensions: Attributes/Modifications");
    qApp->translate("Workbench", "Extensions: Centerlines/Threading");
    qApp->translate("Workbench", "Extensions: Dimensions");
    qApp->translate("Workbench", "Annotations");
    qApp->translate("Workbench", "Add Lines");
    qApp->translate("Workbench", "Add Vertices");    
    qApp->translate("Workbench", "TechDraw");
    // Translations for View > Toolbars
    qApp->translate("Workbench", "TechDraw Pages");
    qApp->translate("Workbench", "TechDraw Views");
    qApp->translate("Workbench", "TechDraw Clips");
    qApp->translate("Workbench", "TechDraw Dimensions");
    qApp->translate("Workbench", "TechDraw Tool Attributes");
    qApp->translate("Workbench", "TechDraw File Access");
    qApp->translate("Workbench", "TechDraw Decoration");
    qApp->translate("Workbench", "TechDraw Annotation");
    qApp->translate("Workbench", "Views");
    qApp->translate("Workbench", "Extensions: Centerlines/Threading");
#endif

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
    *dimensions << "TechDraw_LengthDimension";
    *dimensions << "TechDraw_HorizontalDimension";
    *dimensions << "TechDraw_VerticalDimension";
    *dimensions << "TechDraw_RadiusDimension";
    *dimensions << "TechDraw_DiameterDimension";
    *dimensions << "TechDraw_AngleDimension";
    *dimensions << "TechDraw_HorizontalExtentDimension";
    *dimensions << "TechDraw_VerticalExtentDimension";
    *dimensions << "TechDraw_LinkDimension";
    *dimensions << "TechDraw_LandmarkDimension";

    // extension: attributes and modifications
    Gui::MenuItem* toolattrib = new Gui::MenuItem;
    toolattrib->setCommand("Extensions: Attributes/Modifications");
    *toolattrib << "TechDraw_ExtensionSelectLineAttributes";
    *toolattrib << "TechDraw_ExtensionChangeLineAttributes";
    *toolattrib << "Separator";
    *toolattrib << "TechDraw_ExtensionExtendLine";
    *toolattrib << "TechDraw_ExtensionShortenLine";
    *toolattrib << "Separator";
    *toolattrib << "TechDraw_ExtensionLockUnlockView";
    *toolattrib << "TechDraw_ExtensionPositionSectionView";
    *toolattrib << "Separator";
    *toolattrib << "TechDraw_ExtensionPosHorizChainDimension";
    *toolattrib << "TechDraw_ExtensionPosVertChainDimension";
    *toolattrib << "TechDraw_ExtensionPosObliqueChainDimension";
    *toolattrib << "TechDraw_ExtensionCascadeHorizDimension";
    *toolattrib << "TechDraw_ExtensionCascadeVertDimension";
    *toolattrib << "TechDraw_ExtensionCascadeObliqueDimension";

    // extension: centerlines and threading
    Gui::MenuItem* toolcenter = new Gui::MenuItem;
    toolcenter->setCommand("Extensions: Centerlines/Threading");
    *toolcenter << "TechDraw_ExtensionCircleCenterLines";
    *toolcenter << "TechDraw_ExtensionHoleCircle";
    *toolcenter << "Separator";
    *toolcenter << "TechDraw_ExtensionThreadHoleSide";
    *toolcenter << "TechDraw_ExtensionThreadBoltSide";
    *toolcenter << "TechDraw_ExtensionThreadHoleBottom";
    *toolcenter << "TechDraw_ExtensionThreadBoltBottom";
    *toolcenter << "Separator";
    *toolcenter << "TechDraw_ExtensionVertexAtIntersection";
    *toolcenter << "TechDraw_ExtensionDrawCosmCircle";
    *toolcenter << "TechDraw_ExtensionDrawCosmArc";
    *toolcenter << "TechDraw_ExtensionLineParallel";
    *toolcenter << "TechDraw_ExtensionLinePerpendicular";

    // extension: dimensions
    Gui::MenuItem* tooldimensions = new Gui::MenuItem;
    tooldimensions->setCommand("Extensions: Dimensions");
    *tooldimensions << "TechDraw_ExtensionCreateHorizChainDimension";
    *tooldimensions << "TechDraw_ExtensionCreateVertChainDimension";
    *tooldimensions << "TechDraw_ExtensionCreateObliqueChainDimension";
    *tooldimensions << "TechDraw_ExtensionCreateHorizCoordDimension";
    *tooldimensions << "TechDraw_ExtensionCreateVertCoordDimension";
    *tooldimensions << "TechDraw_ExtensionCreateObliqueCoordDimension";
    *tooldimensions << "TechDraw_ExtensionCreateHorizChamferDimension";
    *tooldimensions << "TechDraw_ExtensionCreateVertChamferDimension";
    *tooldimensions << "TechDraw_ExtensionCreateLengthArc";
    *tooldimensions << "Separator";
    *tooldimensions << "TechDraw_ExtensionInsertDiameter";
    *tooldimensions << "TechDraw_ExtensionInsertSquare";
    *tooldimensions << "Separator";
    *tooldimensions << "TechDraw_ExtensionIncreaseDecimal";
    *tooldimensions << "TechDraw_ExtensionDecreaseDecimal";

    // annotations
    Gui::MenuItem* annotations = new Gui::MenuItem;
    annotations->setCommand("Annotations");
    *annotations << "TechDraw_Annotation";
    *annotations << "TechDraw_RichTextAnnotation";
    *annotations << "TechDraw_Balloon";

    // lines
    Gui::MenuItem* lines = new Gui::MenuItem;
    lines->setCommand("Add Lines");
    *lines << "TechDraw_LeaderLine";
    *lines << "TechDraw_FaceCenterLine";
    *lines << "TechDraw_2LineCenterLine";
    *lines << "TechDraw_2PointCenterLine";
    *lines << "TechDraw_2PointCosmeticLine";

    // vertices
    Gui::MenuItem* vertices = new Gui::MenuItem;
    vertices->setCommand("Add Vertices");
    *vertices << "TechDraw_CosmeticVertex";
    *vertices << "TechDraw_Midpoints";
    *vertices << "TechDraw_Quadrants";

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
    *draw << "TechDraw_MoveView";
    *draw << "TechDraw_CopyView";
    *draw << dimensions;
    *draw << toolattrib;
    *draw << toolcenter;
    *draw << tooldimensions;
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
    *views << "TechDraw_MoveView";
    *views << "TechDraw_CopyView";


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
    *dims << "TechDraw_LandmarkDimension";
//    *dims << "TechDraw_Dimension"

    Gui::ToolBarItem *extattribs = new Gui::ToolBarItem(root);
    extattribs->setCommand("TechDraw Attributes");
    *extattribs << "TechDraw_ExtensionSelectLineAttributes";
    *extattribs << "TechDraw_ExtensionExtendShortenLineGroup";
//    *extattribs << "TechDraw_ExtensionExtendLine";
//    *extattribs << "TechDraw_ExtensionShortenLine";
    *extattribs << "TechDraw_ExtensionLockUnlockView";
    *extattribs << "TechDraw_ExtensionPositionSectionView";
    *extattribs << "TechDraw_ExtensionChangeLineAttributes";
    *extattribs << "TechDraw_ExtensionPosChainDimensionGroup";
//    *extattribs << "TechDraw_ExtensionPosHorizChainDimension";
//    *extattribs << "TechDraw_ExtensionPosVertChainDimension";
//    *extattribs << "TechDraw_ExtensionPosObliqueChainDimension";
    *extattribs << "TechDraw_ExtensionCascadeDimensionGroup";
//    *extattribs << "TechDraw_ExtensionCascadeHorizDimension";
//    *extattribs << "TechDraw_ExtensionCascadeVertDimension";
//    *extattribs << "TechDraw_ExtensionCascadeObliqueDimension";

    Gui::ToolBarItem *extcenter = new Gui::ToolBarItem(root);
    extcenter->setCommand("TechDraw Centerlines");
    *extcenter << "TechDraw_ExtensionCircleCenterLinesGroup";
//    *extcenter << "TechDraw_ExtensionCircleCenterLines";
//    *extcenter << "TechDraw_ExtensionHoleCircle";
    *extcenter << "TechDraw_ExtensionVertexAtIntersection";
    *extcenter << "TechDraw_ExtensionDrawCirclesGroup";
//    *extcenter << "TechDraw_ExtensionDrawCosmCircle";
//    *extcenter << "TechDraw_ExtensionDrawCosmArc";
    *extcenter << "TechDraw_ExtensionLinePPGroup";
//    *extcenter << "TechDraw_ExtensionLineParallel";
//    *extcenter << "TechDraw_ExtensionLinePerpendicular";
    *extcenter << "TechDraw_ExtensionThreadsGroup";
//    *extcenter << "TechDraw_ExtensionThreadHoleSide";
//    *extcenter << "TechDraw_ExtensionThreadHoleBottom";
//    *extcenter << "TechDraw_ExtensionThreadBoltSide";
//    *extcenter << "TechDraw_ExtensionThreadBoltBottom";

    Gui::ToolBarItem *extdimensions = new Gui::ToolBarItem(root);
    extdimensions->setCommand("TechDraw Extend Dimensions");
    *extdimensions << "TechDraw_ExtensionInsertPrefixGroup";
//    *extdimensions << "TechDraw_ExtensionInsertDiameter";
//    *extdimensions << "TechDraw_ExtensionInsertSquare";
    *extdimensions << "TechDraw_ExtensionCreateChainDimensionGroup";
//    *extdimensions << "TechDraw_ExtensionCreateHorizChainDimension";
//    *extdimensions << "TechDraw_ExtensionCreateVertChainDimension";
//    *extdimensions << "TechDraw_ExtensionCreateObliqueChainDimension";
    *extdimensions << "TechDraw_ExtensionCreateCoordDimensionGroup";
//    *extdimensions << "TechDraw_ExtensionCreateHorizCoordDimension";
//    *extdimensions << "TechDraw_ExtensionCreateVertCoordDimension";
//    *extdimensions << "TechDraw_ExtensionCreateObliqueCoordDimension";
    *extdimensions << "TechDraw_ExtensionChamferDimensionGroup";
//    *extdimensions << "TechDraw_ExtensionCreateHorizChamferDimension";
//    *extdimensions << "TechDraw_ExtensionCreateVertChamferDimension";
    *extdimensions << "TechDraw_ExtensionCreateLengthArc";
    *extdimensions << "TechDraw_ExtensionIncreaseDecreaseGroup";
//    *extdimensions << "TechDraw_ExtensionIncreaseDecimal";
//    *extdimensions << "TechDraw_ExtensionDecreaseDecimal";

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
    *anno << "TechDraw_2PointCosmeticLine";
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
    *views << "TechDraw_MoveView";
    *views << "TechDraw_CopyView";


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
    *dims << "TechDraw_LandmarkDimension";
//    *dims << "TechDraw_Dimension";

    Gui::ToolBarItem *extattribs = new Gui::ToolBarItem(root);
    extattribs->setCommand("TechDraw Attributes");
    *extattribs << "TechDraw_ExtensionSelectLineAttributes";
    *extattribs << "TechDraw_ExtensionExtendShortenLineGroup";
//    *extattribs << "TechDraw_ExtensionExtendLine";
//    *extattribs << "TechDraw_ExtensionShortenLine";
    *extattribs << "TechDraw_ExtensionLockUnlockView";
    *extattribs << "TechDraw_ExtensionPositionSectionView";
    *extattribs << "TechDraw_ExtensionChangeLineAttributes";
    *extattribs << "TechDraw_ExtensionPosChainDimensionGroup";
//    *extattribs << "TechDraw_ExtensionPosHorizChainDimension";
//    *extattribs << "TechDraw_ExtensionPosVertChainDimension";
//    *extattribs << "TechDraw_ExtensionPosObliqueChainDimension";
    *extattribs << "TechDraw_ExtensionCascadeDimensionGroup";
//    *extattribs << "TechDraw_ExtensionCascadeHorizDimension";
//    *extattribs << "TechDraw_ExtensionCascadeVertDimension";
//    *extattribs << "TechDraw_ExtensionCascadeObliqueDimension";

    Gui::ToolBarItem *extcenter = new Gui::ToolBarItem(root);
    extcenter->setCommand("TechDraw Centerlines");
    *extcenter << "TechDraw_ExtensionCircleCenterLinesGroup";
//    *extcenter << "TechDraw_ExtensionCircleCenterLines";
//    *extcenter << "TechDraw_ExtensionHoleCircle";
    *extcenter << "TechDraw_ExtensionVertexAtIntersection";
    *extcenter << "TechDraw_ExtensionDrawCirclesGroup";
//    *extcenter << "TechDraw_ExtensionDrawCosmCircle";
//    *extcenter << "TechDraw_ExtensionDrawCosmArc";
    *extcenter << "TechDraw_ExtensionLinePPGroup";
//    *extcenter << "TechDraw_ExtensionLineParallel";
//    *extcenter << "TechDraw_ExtensionLinePerpendicular";
    *extcenter << "TechDraw_ExtensionThreadsGroup";
//    *extcenter << "TechDraw_ExtensionThreadHoleSide";
//    *extcenter << "TechDraw_ExtensionThreadHoleBottom";
//    *extcenter << "TechDraw_ExtensionThreadBoltSide";
//    *extcenter << "TechDraw_ExtensionThreadBoltBottom";

    Gui::ToolBarItem *extdimensions = new Gui::ToolBarItem(root);
    extdimensions->setCommand("TechDraw Extend Dimensions");
    *extdimensions << "TechDraw_ExtensionInsertPrefixGroup";
//    *extdimensions << "TechDraw_ExtensionInsertDiameter";
//    *extdimensions << "TechDraw_ExtensionInsertSquare";
    *extdimensions << "TechDraw_ExtensionCreateChainDimensionGroup";
//    *extdimensions << "TechDraw_ExtensionCreateHorizChainDimension";
//    *extdimensions << "TechDraw_ExtensionCreateVertChainDimension";
//    *extdimensions << "TechDraw_ExtensionCreateObliqueChainDimension";
    *extdimensions << "TechDraw_ExtensionCreateCoordDimensionGroup";
//    *extdimensions << "TechDraw_ExtensionCreateHorizCoordDimension";
//    *extdimensions << "TechDraw_ExtensionCreateVertCoordDimension";
//    *extdimensions << "TechDraw_ExtensionCreateObliqueCoordDimension";
    *extdimensions << "TechDraw_ExtensionChamferDimensionGroup";
//    *extdimensions << "TechDraw_ExtensionCreateHorizChamferDimension";
//    *extdimensions << "TechDraw_ExtensionCreateVertChamferDimension";
    *extdimensions << "TechDraw_ExtensionCreateLengthArc";
    *extdimensions << "TechDraw_ExtensionIncreaseDecreaseGroup";
//    *extdimensions << "TechDraw_ExtensionIncreaseDecimal";
//    *extdimensions << "TechDraw_ExtensionDecreaseDecimal";

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
    *anno << "TechDraw_2PointCosmeticLine";
    *anno << "TechDraw_CosmeticEraser";
    *anno << "TechDraw_DecorateLine";
    *anno << "TechDraw_ShowAll";
    *anno << "TechDraw_WeldSymbol";

    return root;
}
