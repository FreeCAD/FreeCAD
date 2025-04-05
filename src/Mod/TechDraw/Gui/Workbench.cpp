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
#include <qobject.h>
#endif

#include "Workbench.h"
#include <App/Application.h>
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>

#include <Mod/TechDraw/App/Preferences.h>

using namespace TechDrawGui;
using namespace TechDraw;

#if 0// needed for Qt's lupdate utility
    qApp->translate("Workbench", "Dimensions");
    qApp->translate("Workbench", "Extensions: Attributes/Modifications");
    qApp->translate("Workbench", "Extensions: Centerlines/Threading");
    qApp->translate("Workbench", "Extensions: Dimensions");
    qApp->translate("Workbench", "Annotations");
    qApp->translate("Workbench", "Stacking");
    qApp->translate("Workbench", "Add Lines");
    qApp->translate("Workbench", "Add Vertices");
    qApp->translate("Workbench", "Page");
    qApp->translate("Workbench", "TechDraw");
    // Translations for View > Toolbars
    qApp->translate("Workbench", "TechDraw Annotation");
    qApp->translate("Workbench", "TechDraw Attributes");
    qApp->translate("Workbench", "TechDraw Centerlines");
    qApp->translate("Workbench", "TechDraw Decoration");
    qApp->translate("Workbench", "TechDraw Dimensions");
    qApp->translate("Workbench", "TechDraw Extend Dimensions");
    qApp->translate("Workbench", "TechDraw File Access");
    qApp->translate("Workbench", "TechDraw Pages");
    qApp->translate("Workbench", "TechDraw Stacking");
    qApp->translate("Workbench", "TechDraw Tool Attributes");
    qApp->translate("Workbench", "TechDraw Views");
    qApp->translate("Workbench", "Views From Other Workbenches");
    qApp->translate("Workbench", "Clipped Views");
    qApp->translate("Workbench", "Hatching");
    qApp->translate("Workbench", "Symbols");
    qApp->translate("Workbench", "Views");
#endif

TYPESYSTEM_SOURCE(TechDrawGui::Workbench, Gui::StdWorkbench)

Gui::MenuItem* Workbench::setupMenuBar() const
{
    auto* root = StdWorkbench::setupMenuBar();
    auto* item = root->findItem("&Windows");
    auto* draw = new Gui::MenuItem;
    root->insertItem(item, draw);

    // dimensions
    auto* dimensions = new Gui::MenuItem;
    dimensions->setCommand("Dimensions");
    *dimensions << "TechDraw_Dimension";
    *dimensions << "TechDraw_LengthDimension";
    *dimensions << "TechDraw_HorizontalDimension";
    *dimensions << "TechDraw_VerticalDimension";
    *dimensions << "TechDraw_RadiusDimension";
    *dimensions << "TechDraw_DiameterDimension";
    *dimensions << "TechDraw_AngleDimension";
    *dimensions << "TechDraw_3PtAngleDimension";
    *dimensions << "TechDraw_AreaDimension";
    *dimensions << "TechDraw_HorizontalExtentDimension";
    *dimensions << "TechDraw_VerticalExtentDimension";
    // TechDraw_LinkDimension is DEPRECATED.  Use TechDraw_DimensionRepair instead.
    *dimensions << "TechDraw_LinkDimension";
    *dimensions << "TechDraw_LandmarkDimension";
    *dimensions << "TechDraw_DimensionRepair";

    // extension: attributes and modifications
    auto* toolattrib = new Gui::MenuItem;
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
    *toolattrib << "Separator";
    *toolattrib << "TechDraw_ExtensionAreaAnnotation";
    *toolattrib << "TechDraw_ExtensionArcLengthAnnotation";
    *toolattrib << "TechDraw_ExtensionCustomizeFormat";

    // extension: centerlines and threading
    auto* toolcenter = new Gui::MenuItem;
    toolcenter->setCommand("Extensions: Centerlines/Threading");
    *toolcenter << "TechDraw_ExtensionCircleCenterLines";
    *toolcenter << "TechDraw_ExtensionHoleCircle";
    *toolcenter << "Separator";
    *toolcenter << "TechDraw_ExtensionThreadHoleSide";
    *toolcenter << "TechDraw_ExtensionThreadHoleBottom";
    *toolcenter << "TechDraw_ExtensionThreadBoltSide";
    *toolcenter << "TechDraw_ExtensionThreadBoltBottom";
    *toolcenter << "Separator";
    *toolcenter << "TechDraw_ExtensionVertexAtIntersection";
    *toolcenter << "TechDraw_CommandAddOffsetVertex";
    *toolcenter << "Separator";
    *toolcenter << "TechDraw_ExtensionDrawCosmCircle";
    *toolcenter << "TechDraw_ExtensionDrawCosmArc";
    *toolcenter << "TechDraw_ExtensionDrawCosmCircle3Points";
    *toolcenter << "TechDraw_ExtensionLineParallel";
    *toolcenter << "TechDraw_ExtensionLinePerpendicular";

    // extension: dimensions
    auto* tooldimensions = new Gui::MenuItem;
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
    *tooldimensions << "TechDraw_ExtensionInsertRepetition";
    *tooldimensions << "TechDraw_ExtensionRemovePrefixChar";
    *tooldimensions << "Separator";
    *tooldimensions << "TechDraw_ExtensionIncreaseDecimal";
    *tooldimensions << "TechDraw_ExtensionDecreaseDecimal";

    // annotations
    auto* annotations = new Gui::MenuItem;
    annotations->setCommand("Annotations");
    *annotations << "TechDraw_Annotation";
    *annotations << "TechDraw_RichTextAnnotation";
    *annotations << "TechDraw_Balloon";
    *annotations << "TechDraw_AxoLengthDimension";

    // stacking
    auto* stacking = new Gui::MenuItem;
    stacking->setCommand("Stacking");
    *stacking << "TechDraw_StackTop";
    *stacking << "TechDraw_StackBottom";
    *stacking << "TechDraw_StackUp";
    *stacking << "TechDraw_StackDown";

    // lines
    auto* lines = new Gui::MenuItem;
    lines->setCommand("Add Lines");
    *lines << "TechDraw_LeaderLine";
    *lines << "TechDraw_FaceCenterLine";
    *lines << "TechDraw_2LineCenterLine";
    *lines << "TechDraw_2PointCenterLine";
    *lines << "TechDraw_2PointCosmeticLine";
    *lines << "TechDraw_CosmeticCircle";
    *lines << "Separator";
    *lines << "TechDraw_DecorateLine";
    *lines << "TechDraw_ShowAll";

    // vertices
    auto* vertices = new Gui::MenuItem;
    vertices->setCommand("Add Vertices");
    *vertices << "TechDraw_CosmeticVertex";
    *vertices << "TechDraw_Midpoints";
    *vertices << "TechDraw_Quadrants";

    // pages
    auto* pages = new Gui::MenuItem;
    pages->setCommand("Page");
    *pages << "TechDraw_PageDefault";
    *pages << "TechDraw_PageTemplate";
    *pages << "TechDraw_FillTemplateFields";
    *pages << "TechDraw_RedrawPage";
    *pages << "TechDraw_PrintAll";
    *pages << "Separator";
    *pages << "TechDraw_ExportPageSVG";
    *pages << "TechDraw_ExportPageDXF";

    // views
    auto* views = new Gui::MenuItem;
    views->setCommand("TechDraw Views");
    *views << "TechDraw_View";
    *views << "TechDraw_BrokenView";
    *views << "TechDraw_SectionView";
    *views << "TechDraw_ComplexSection";
    *views << "TechDraw_DetailView";
    *views << "TechDraw_ProjectionGroup";
    *views << "TechDraw_ClipGroup";
    *views << "Separator";
    *views << "TechDraw_Symbol";
    *views << "TechDraw_Image";
    *views << "Separator";
    *views << "TechDraw_ShareView";
    *views << "Separator";
    *views << "TechDraw_ToggleFrame";
    *views << "Separator";
    *views << "TechDraw_ProjectShape";

    // views from other workbenches
    auto* other = new Gui::MenuItem;
    other->setCommand("Views From Other Workbenches");
    *other << "TechDraw_ActiveView";
    *other << "TechDraw_DraftView";
    *other << "TechDraw_ArchView";
    *other << "TechDraw_SpreadsheetView";

    // hatching
    auto* hatch = new Gui::MenuItem;
    hatch->setCommand("Hatching");
    *hatch << "TechDraw_Hatch";
    *hatch << "TechDraw_GeometricHatch";

    // symbols
    auto* symbols = new Gui::MenuItem;
    symbols->setCommand("Symbols");
    *symbols << "TechDraw_WeldSymbol";
    *symbols << "TechDraw_SurfaceFinishSymbols";
    *symbols << "TechDraw_HoleShaftFit";


    auto* aligning = new Gui::MenuItem;
    aligning->setCommand("Aligning");
    *aligning << "TechDraw_AlignVertexesVertically";
    *aligning << "TechDraw_AlignVertexesHorizontally";

    // main menu
    draw->setCommand("TechDraw");
    *draw << pages;
    *draw << "Separator";
    *draw << views;
    *draw << "Separator";
    *draw << other;
    *draw << "Separator";
    *draw << dimensions;
    *draw << "Separator";
    *draw << hatch;
    *draw << "Separator";
    *draw << symbols;
    *draw << "Separator";
    *draw << stacking;
    *draw << aligning;
    *draw << "Separator";
    *draw << toolattrib;
    *draw << toolcenter;
    *draw << tooldimensions;
    *draw << "Separator";
    *draw << annotations;
    *draw << lines;
    *draw << vertices;
    *draw << "Separator";
    *draw << "TechDraw_CosmeticEraser";

    return root;
}

/// https://answers.microsoft.com/en-us/windows/forum/all/whats-the-difference-between-a-taskbar-and-a/20c7746c-b6d3-40f4-b5f4-b44bfcca6e0c/ Taskbar shows the Windows logo/ Start button and Open programs and the Action center where alters are shown,
/// A toolbar like the Main browser bar/ Favorites bar/ Bing bar and even other third party toolbars like google toolbar are usually located on the top of the screen,
/// Where as the Taskbar is usually on the bottom of the screen but can be on either side of the screen.

/// https://learn.microsoft.com/en-us/windows/apps/design/controls/command-bar
/// the command bar shows a row of icon buttons and an optional "see more" button, which is represented by an ellipsis [...].

/// toolbars are the groups of icons that run horizontally across the top of the screen.  Unless you move them somewhere else. :)
Gui::ToolBarItem* Workbench::setupToolBars() const
{
    auto* root = StdWorkbench::setupToolBars();
    auto* pages = new Gui::ToolBarItem(root);
    pages->setCommand("TechDraw Pages");
    *pages << "TechDraw_PageDefault";
    *pages << "TechDraw_PageTemplate";
    *pages << "TechDraw_FillTemplateFields";
    *pages << "TechDraw_RedrawPage";
    *pages << "TechDraw_PrintAll";

    auto* views = new Gui::ToolBarItem(root);
    views->setCommand("TechDraw Views");
    if (Preferences::useSingleInsertTool()) {
        *views << "TechDraw_View";
    } else {
        *views << "TechDraw_ShapeView";
    }

    *views << "TechDraw_BrokenView";
    *views << "TechDraw_ActiveView";
    *views << "TechDraw_SectionGroup";
    *views << "TechDraw_DetailView";
    *views << "TechDraw_DraftView";
    *views << "TechDraw_ClipGroup";

    if (!Preferences::useSingleInsertTool()) {
        *views << "TechDraw_Symbol";
        *views << "TechDraw_Image";
        *views << "TechDraw_ArchView";
        *views << "TechDraw_SpreadsheetView";
    }

    auto* stacking = new Gui::ToolBarItem(root);
    stacking->setCommand("TechDraw Stacking");
    *stacking << "TechDraw_StackGroup";

    auto* dims = new Gui::ToolBarItem(root);
    dims->setCommand("TechDraw Dimensions");

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/TechDraw/dimensioning");
    bool separatedTools = hGrp->GetBool("SeparatedDimensioningTools", false);
    if (hGrp->GetBool("SingleDimensioningTool", true)) {
        if (separatedTools) {
            *dims << "TechDraw_Dimension";
        }
        else {
            *dims << "TechDraw_CompDimensionTools";
        }
    }
    if (separatedTools) {
        *dims << "TechDraw_LengthDimension";
        *dims << "TechDraw_HorizontalDimension";
        *dims << "TechDraw_VerticalDimension";
        *dims << "TechDraw_RadiusDimension";
        *dims << "TechDraw_DiameterDimension";
        *dims << "TechDraw_AngleDimension";
        *dims << "TechDraw_3PtAngleDimension";
        *dims << "TechDraw_AreaDimension";
        *dims << "TechDraw_ExtentGroup";
    }

    // TechDraw_LinkDimension is DEPRECATED.  Use TechDraw_DimensionRepair instead.
    // *dims << "TechDraw_LinkDimension";
    *dims << "TechDraw_Balloon";
    *dims << "TechDraw_AxoLengthDimension";
    *dims << "TechDraw_LandmarkDimension";
    *dims << "TechDraw_DimensionRepair";

    auto* extattribs = new Gui::ToolBarItem(root);
    extattribs->setCommand("TechDraw Attributes");
    *extattribs << "TechDraw_ExtensionSelectLineAttributes";
    *extattribs << "TechDraw_ExtensionChangeLineAttributes";
    *extattribs << "TechDraw_ExtensionExtendShortenLineGroup";
    *extattribs << "TechDraw_ExtensionLockUnlockView";
    *extattribs << "TechDraw_ExtensionPositionSectionView";
    if (separatedTools) {
        *extattribs << "TechDraw_ExtensionAreaAnnotation";
        *extattribs << "TechDraw_ExtensionArcLengthAnnotation";
    }
    *extattribs << "TechDraw_ExtensionCustomizeFormat";

    auto* extcenter = new Gui::ToolBarItem(root);
    extcenter->setCommand("TechDraw Centerlines");
    *extcenter << "TechDraw_ExtensionCircleCenterLinesGroup";
    *extcenter << "TechDraw_ExtensionThreadsGroup";
    *extcenter << "TechDraw_CommandVertexCreationGroup";
    //*extcenter << "TechDraw_ExtensionVertexAtIntersection";
    *extcenter << "TechDraw_ExtensionDrawCirclesGroup";
    *extcenter << "TechDraw_ExtensionLinePPGroup";

    auto* extdimensions = new Gui::ToolBarItem(root);
    extdimensions->setCommand("TechDraw Extend Dimensions");
    if (separatedTools) {
        *extdimensions << "TechDraw_ExtensionCreateChainDimensionGroup";
        *extdimensions << "TechDraw_ExtensionCreateCoordDimensionGroup";
        *extdimensions << "TechDraw_ExtensionChamferDimensionGroup";
        *extdimensions << "TechDraw_ExtensionCreateLengthArc";
    }
    *extdimensions << "TechDraw_ExtensionInsertPrefixGroup";
    *extdimensions << "TechDraw_ExtensionIncreaseDecreaseGroup";

    auto* file = new Gui::ToolBarItem(root);
    file->setCommand("TechDraw File Access");
    *file << "TechDraw_ExportPageSVG";
    *file << "TechDraw_ExportPageDXF";

    auto* decor = new Gui::ToolBarItem(root);
    decor->setCommand("TechDraw Decoration");
    *decor << "TechDraw_Hatch";
    *decor << "TechDraw_GeometricHatch";
    *decor << "TechDraw_ToggleFrame";

    auto* anno = new Gui::ToolBarItem(root);
    anno->setCommand("TechDraw Annotation");
    *anno << "TechDraw_Annotation";
    *anno << "TechDraw_LeaderLine";
    *anno << "TechDraw_RichTextAnnotation";
    *anno << "TechDraw_CosmeticVertexGroup";
    *anno << "TechDraw_CenterLineGroup";
    *anno << "TechDraw_2PointCosmeticLine";
    *anno << "TechDraw_CosmeticCircle";
    *anno << "TechDraw_DecorateLine";
    *anno << "TechDraw_ShowAll";
    *anno << "TechDraw_WeldSymbol";
    *anno << "TechDraw_SurfaceFinishSymbols";
    *anno << "TechDraw_HoleShaftFit";
    return root;
}

// wf: AFAICT this method doesn't do anything.
Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    // from Part/Gui/Workbench.cpp
    Gui::ToolBarItem* root = new Gui::ToolBarItem;      //NOLINT
    return root;
}
