/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <Gui/ToolBarManager.h>
#include <Gui/DockWindowManager.h>
#include <Gui/Application.h>
#include <Gui/Action.h>
#include <Gui/Command.h>
#include <Gui/Selection.h>
#include <Gui/ToolBoxManager.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Mod/Complete/App/CompleteConfiguration.h>

using namespace CompleteGui;

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Workbench", "S&ketch");
    qApp->translate("Workbench", "Dr&awing");
    qApp->translate("Workbench", "&Raytracing");
    qApp->translate("Workbench", "&Drafting");
    qApp->translate("Workbench", "Sketch based");
    qApp->translate("Workbench", "Primitives");
    qApp->translate("Workbench", "Object appearence");
    qApp->translate("Workbench", "Wire Tools");
    // taken from TestGui.py
    qApp->translate("Test_Test", "Self-test...");
    qApp->translate("Test_Test", "Runs a self-test to check if the application works properly");
#endif

/// @namespace CompleteGui @class Workbench
TYPESYSTEM_SOURCE(CompleteGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
}

Workbench::~Workbench()
{
}

void Workbench::setupContextMenu(const char* recipient,Gui::MenuItem* item) const
{
    if (strcmp(recipient,"View") == 0)
    {
        Gui::MenuItem* StdViews = new Gui::MenuItem();
        StdViews->setCommand( "Standard views" );

        *StdViews << "Std_ViewAxo" << "Separator" << "Std_ViewFront" << "Std_ViewTop" << "Std_ViewRight"
                  << "Std_ViewRear" << "Std_ViewBottom" << "Std_ViewLeft";

        *item << "Std_ViewFitAll" << "Std_ViewFitSelection" << StdViews
              << "Separator" << "Std_ViewDockUndockFullscreen";

        if ( Gui::Selection().countObjectsOfType(App::DocumentObject::getClassTypeId()) > 0 )
            {
            Gui::MenuItem* DraftContext = new Gui::MenuItem();
            DraftContext->setCommand("Display options");

            *DraftContext << "Draft_ApplyStyle" << "Draft_ToggleDisplayMode"
                          << "Draft_AddToGroup";
            *item << "Separator" << "Std_SetAppearance" << "Std_ToggleVisibility"
                  << "Std_ToggleSelectability" << "Std_TreeSelection"
                  << "Std_RandomColor" << "Separator" << "Std_Delete" << DraftContext;
            }
    }
    else if (strcmp(recipient,"Tree") == 0)
    {
        if (Gui::Selection().countObjectsOfType(App::DocumentObject::getClassTypeId()) > 0 ) {
            Gui::MenuItem* DraftContext = new Gui::MenuItem();
            DraftContext->setCommand("Display options");

            *DraftContext << "Draft_ApplyStyle" << "Draft_ToggleDisplayMode"
                          << "Draft_AddToGroup";

            *item << "Std_ToggleVisibility" << "Std_ShowSelection" << "Std_HideSelection"
                  << "Std_ToggleSelectability" << "Separator" << "Std_SetAppearance"
                  << "Std_ToggleVisibility" << "Std_RandomColor" << "Separator" << "Std_Delete"
                  << DraftContext;
        }
    }
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::CommandManager &mgr = Gui::Application::Instance->commandManager();
    // Setup the default menu bar
    Gui::MenuItem* menuBar = new Gui::MenuItem;

    // File
    Gui::MenuItem* file = new Gui::MenuItem( menuBar );
    file->setCommand("&File");
    *file << "Std_New" << "Std_Open" << "Separator" << "Std_CloseActiveWindow"
          << "Std_CloseAllWindows" << "Separator" << "Std_Save" << "Std_SaveAs"
          << "Separator" << "Std_Import" << "Std_Export"
          << "Std_MergeProjects" << "Std_ProjectInfo"
          << "Separator" << "Std_Print" << "Std_PrintPreview" << "Std_PrintPdf"
          << "Separator" << "Std_RecentFiles" << "Separator" << "Std_Quit";

    // Edit
    Gui::MenuItem* edit = new Gui::MenuItem( menuBar );
    edit->setCommand("&Edit");
    *edit << "Std_Undo" << "Std_Redo" << "Separator" << "Std_Cut" << "Std_Copy"
          << "Std_Paste" << "Std_DuplicateSelection" << "Separator"
          << "Std_Refresh" << "Std_BoxSelection" << "Std_SelectAll" << "Std_Delete"
          << "Std_Placement" << "Std_Alignment"
          << "Separator" << "Std_DlgPreferences";

    // Standard views
    Gui::MenuItem* stdviews = new Gui::MenuItem;
    stdviews->setCommand("Standard views");
    *stdviews << "Std_ViewFitAll" << "Std_ViewFitSelection" << "Std_ViewAxo"
              << "Separator" << "Std_ViewFront" << "Std_ViewTop"
              << "Std_ViewRight" << "Separator" << "Std_ViewRear"
              << "Std_ViewBottom" << "Std_ViewLeft";

    // stereo
    Gui::MenuItem* view3d = new Gui::MenuItem;
    view3d->setCommand("&Stereo");
    *view3d << "Std_ViewIvStereoRedGreen" << "Std_ViewIvStereoQuadBuff"
            << "Std_ViewIvStereoInterleavedRows" << "Std_ViewIvStereoInterleavedColumns"
            << "Std_ViewIvStereoOff" << "Separator" << "Std_ViewIvIssueCamPos";

    // zoom
    Gui::MenuItem* zoom = new Gui::MenuItem;
    zoom->setCommand("&Zoom");
    *zoom << "Std_ViewZoomIn" << "Std_ViewZoomOut" << "Separator" << "Std_ViewBoxZoom";

    // Visibility
    Gui::MenuItem* visu = new Gui::MenuItem;
    visu->setCommand("Visibility");
    *visu << "Std_ToggleVisibility" << "Std_ShowSelection" << "Std_HideSelection"
          << "Separator" << "Std_ToggleObjects" << "Std_ShowObjects" << "Std_HideObjects"
          << "Separator" << "Std_ToggleSelectability";

    // View
    Gui::MenuItem* view = new Gui::MenuItem( menuBar );
    view->setCommand("&View");
    *view << "Std_ViewCreate" << "Std_OrthographicCamera" << "Std_PerspectiveCamera" << "Separator"
          << stdviews << "Std_FreezeViews" << "Separator" << view3d << "Std_DrawStyle" << zoom
          << "Std_ViewDockUndockFullscreen" << "Std_AxisCross" << "Std_ToggleClipPlane"
          << "Std_TextureMapping" << "Separator" << visu
          << "Std_ToggleVisibility" << "Std_ToggleNavigation"
          << "Std_SetAppearance" << "Std_RandomColor" << "Separator"
          << "Std_MeasureDistance" << "Separator"
          << "Std_Workbench" << "Std_ToolBarMenu" << "Std_DockViewMenu" << "Separator"
          << "Std_ViewStatusBar";

    // Tools
    Gui::MenuItem* tool = new Gui::MenuItem( menuBar );
    tool->setCommand("&Tools");
    *tool << "Std_DlgParameter" << "Separator"
          << "Std_DlgMacroRecord" << "Std_MacroStopRecord"
          << "Std_DlgMacroExecute" << "Std_DlgMacroExecuteDirect"
          << "Separator" << "Std_ViewScreenShot" << "Std_SceneInspector"
          << "Std_ExportGraphviz" << "Std_ProjectUtil"
          << "Std_DemoMode" << "Separator" << "Std_DlgCustomize";

    // Mesh ****************************************************************************************************
    Gui::MenuItem* mesh = new Gui::MenuItem( menuBar );

    // submenu analyze
    Gui::MenuItem* analyze = new Gui::MenuItem();
    analyze->setCommand("Analyze");
    *analyze << "Mesh_Evaluation"
             << "Mesh_EvaluateFacet"
             << "Mesh_CurvatureInfo"
             << "Separator"
             << "Mesh_EvaluateSolid"
             << "Mesh_BoundingBox";

    // submenu boolean
    Gui::MenuItem* boolean = new Gui::MenuItem();
    boolean->setCommand("Boolean");
    *boolean << "Mesh_Union"
             << "Mesh_Intersection"
             << "Mesh_Difference";

    mesh->setCommand("&Meshes");
    *mesh << "Mesh_Import"
          << "Mesh_Export"
          << "MeshPart_Mesher"
          << "Separator"
          << analyze
          << "Mesh_HarmonizeNormals"
          << "Mesh_FlipNormals"
          << "Separator"
          << "Mesh_FillupHoles"
          << "Mesh_FillInteractiveHole"
          << "Mesh_RemoveComponents"
          << "Mesh_RemoveCompByHand"
          << "Mesh_AddFacet"
          << "Mesh_Smoothing"
          << "Separator"
          << "Mesh_BuildRegularSolid"
          << boolean << "Separator"
          << "Mesh_PolySelect"
          << "Mesh_PolyCut"
          << "Mesh_PolySplit"
          << "Mesh_PolySegm"
          << "Mesh_ToolMesh"
          << "Mesh_Segmentation"
          << "Mesh_VertexCurvature";

    // Sketch **************************************************************************************************

    Gui::MenuItem* sketch = new Gui::MenuItem(menuBar);
    sketch->setCommand("S&ketch");

    Gui::MenuItem* geom = new Gui::MenuItem();
    geom->setCommand("Sketcher geometries");
    *geom << "Sketcher_CreatePoint"
          << "Sketcher_CreateArc"
          << "Sketcher_CreateCircle"
          << "Sketcher_CreateLine"
          << "Sketcher_CreatePolyline"
          << "Sketcher_CreateRectangle"
          << "Separator"
          << "Sketcher_CreateFillet"
          << "Sketcher_Trimming"
          << "Sketcher_External"
          << "Sketcher_ToggleConstruction";

    Gui::MenuItem* cons = new Gui::MenuItem();
    cons->setCommand("Sketcher constraints");
    *cons << "Sketcher_ConstrainCoincident"
          << "Sketcher_ConstrainPointOnObject"
          << "Sketcher_ConstrainVertical"
          << "Sketcher_ConstrainHorizontal"
          << "Sketcher_ConstrainParallel"
          << "Sketcher_ConstrainPerpendicular"
          << "Sketcher_ConstrainTangent"
          << "Sketcher_ConstrainEqual"
          << "Sketcher_ConstrainSymmetric"
          << "Separator"
          << "Sketcher_ConstrainLock"
          << "Sketcher_ConstrainDistanceX"
          << "Sketcher_ConstrainDistanceY"
          << "Sketcher_ConstrainDistance"
          << "Sketcher_ConstrainRadius"
          << "Sketcher_ConstrainAngle";

    *sketch
        << "Sketcher_NewSketch"
        << "Sketcher_LeaveSketch"
        << "Sketcher_ViewSketch"
        << "Sketcher_MapSketch"
        << geom
        << cons
    ;

    // Part ****************************************************************************************************

    Gui::MenuItem* part = new Gui::MenuItem(menuBar);
    part->setCommand("&Part");

    // submenu boolean
    Gui::MenuItem* para = new Gui::MenuItem();
    para->setCommand("Primitives");
    *para << "Part_Box"
          << "Part_Cylinder"
          << "Part_Sphere"
          << "Part_Cone"
          << "Part_Torus"
          << "Part_Primitives";

    Gui::MenuItem* PartDesign = new Gui::MenuItem();
    PartDesign->setCommand("Part design");

    *PartDesign   << "PartDesign_Pad"
                  << "PartDesign_Pocket"
                  << "PartDesign_Revolution"
                  << "PartDesign_Groove"
                  << "PartDesign_Fillet"
                  << "PartDesign_Chamfer"
                  << "PartDesign_Mirrored"
                  << "PartDesign_LinearPattern"
                  << "PartDesign_PolarPattern"
                  << "PartDesign_MultiTransform";

    *part << para
          << PartDesign
          << "Part_ShapeFromMesh"
          << "Part_MakeSolid"
          << "Part_ReverseShape"
          << "Part_SimpleCopy"
          << "Separator"
          << "Part_Boolean"
          << "Part_Extrude"
          << "Part_Revolve"
          << "Part_Mirror"
          << "Part_Fillet"
          << "Part_Chamfer";


    // Drawing ****************************************************************************************************

    Gui::MenuItem* drawing = new Gui::MenuItem(menuBar);

    drawing->setCommand("Dr&awing");
    *drawing
        << "Drawing_Open"
        << "Separator"
        << "Drawing_NewA3Landscape"
        << "Drawing_NewView"
        << "Drawing_ExportPage"
        << "Separator"
        << "Drawing_ProjectShape"
    ;

    // Raytracing ****************************************************************************************************

    Gui::MenuItem* raytracing = new Gui::MenuItem(menuBar);

    raytracing->setCommand("&Raytracing");
    *raytracing
        << "Raytracing_WriteView"
        << "Raytracing_WriteCamera"
        << "Raytracing_WritePart"
        << "Separator"
        << "Raytracing_NewPovrayProject"
        << "Raytracing_NewPartSegment"
        << "Raytracing_ExportProject";
    ;

    // Drafting ****************************************************************************************************
#   ifdef COMPLETE_USE_DRAFTING
    if (mgr.getCommandByName("Draft_Line")) {
        Gui::MenuItem* Drafting = new Gui::MenuItem(menuBar);
        Drafting->setCommand("&Drafting");

        Gui::MenuItem* DraftContext = new Gui::MenuItem();
        DraftContext->setCommand("Object appearence");

        *DraftContext << "Draft_ApplyStyle" << "Draft_ToggleDisplayMode";

        Gui::MenuItem* DraftWireTools = new Gui::MenuItem();
        DraftWireTools->setCommand("Wire Tools");

        *DraftWireTools << "Draft_WireToBSpline" << "Draft_AddPoint" << "Draft_DelPoint";

        *Drafting
            << "Draft_Line"
            << "Draft_Wire"
            << "Draft_Circle"
            << "Draft_Arc"
            << "Draft_Rectangle"
            << "Draft_Polygon"
            << "Draft_BSpline"
            << "Draft_Text"
            << "Draft_Dimension"
            << "Separator"
            << "Draft_Move"
            << "Draft_Rotate"
            << "Draft_Offset"
            << "Draft_Trimex"
            << "Draft_Upgrade"
            << "Draft_Downgrade"
            << "Draft_Scale"
            << "Draft_Edit"
            << "Draft_Drawing"
            << "Draft_Shape2DView"
            << DraftWireTools
            << DraftContext
        ;
    }
#   endif

    // xxx ****************************************************************************************************


    // Windows
    Gui::MenuItem* wnd = new Gui::MenuItem( menuBar );
    wnd->setCommand("&Windows");
    *wnd << "Std_ActivateNextWindow" << "Std_ActivatePrevWindow" << "Separator"
         << "Std_TileWindows" << "Std_CascadeWindows"
         << "Std_ArrangeIcons" << "Separator" << "Std_WindowsMenu" << "Std_Windows";

    // help ****************************************************************************************************
    // Separator
    Gui::MenuItem* sep = new Gui::MenuItem( menuBar );
    sep->setCommand( "Separator" );

    // Help
    Gui::MenuItem* helpWebsites = new Gui::MenuItem;
    helpWebsites->setCommand("&Online-help");
    *helpWebsites << "Std_OnlineHelpWebsite"
                  << "Std_FreeCADWebsite"
                  << "Std_PythonWebsite";

    Gui::MenuItem* help = new Gui::MenuItem( menuBar );
    help->setCommand("&Help");
    *help << "Std_OnlineHelp"
          << "Std_PythonHelp"
          << helpWebsites
          << "Separator"
          << "Test_Test"
          << "Separator"
          << "Std_About"
          << "Std_AboutQt"
          << "Separator"
          << "Std_WhatsThis" ;

    return menuBar;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::CommandManager &mgr = Gui::Application::Instance->commandManager();
    Gui::ToolBarItem* root = new Gui::ToolBarItem;

    // File
    Gui::ToolBarItem* file = new Gui::ToolBarItem( root );
    file->setCommand("File");
    *file << "Std_New"
          << "Std_Open"
          << "Std_Save"
          << "Std_Print"
          << "Separator"
          << "Std_Cut"
          << "Std_Copy"
          << "Std_Paste"
          << "Separator"
          << "Std_Undo"
          << "Std_Redo"
          << "Separator"
          << "Std_Refresh"
          << "Separator"
          //<< "Std_Workbench"
          << "Std_WhatsThis";

    // Macro
    Gui::ToolBarItem* macro = new Gui::ToolBarItem( root );
    macro->setCommand("Macro");
    *macro << "Std_DlgMacroRecord" << "Std_MacroStopRecord" << "Std_DlgMacroExecute"
           << "Std_DlgMacroExecuteDirect";

    // View
    Gui::ToolBarItem* view = new Gui::ToolBarItem( root );
    view->setCommand("View");
    *view << "Std_ViewFitAll" << "Separator" << "Std_ViewAxo" << "Separator" << "Std_ViewFront"
          << "Std_ViewTop" << "Std_ViewRight" << "Separator" << "Std_ViewRear" << "Std_ViewBottom"
          << "Std_ViewLeft" << "Separator" << "Std_MeasureDistance";

    // Part Design
    Gui::ToolBarItem* part_design = new Gui::ToolBarItem( root );
    part_design->setCommand("Part");
    *part_design
        << "Part_Box"
        << "Part_Cylinder"
        << "Part_Sphere"
        << "Part_Cone"
        << "Part_Torus"
        //<< "Part_Primitives"
        << "Separator"
        << "Part_Boolean"
        << "Part_Cut"
        << "Part_Fuse"
        << "Part_Common"
        << "Part_Section"
        << "Separator"
        << "Part_Extrude"
        << "Part_Revolve"
        << "Part_Mirror"
        << "Part_Fillet"
        << "Part_Chamfer"
    ;

    Gui::ToolBarItem* geom = new Gui::ToolBarItem(root);
    geom->setCommand("Sketcher geometries");
    *geom << "Sketcher_NewSketch"
          << "Sketcher_LeaveSketch"
          << "Separator"
          << "Sketcher_CreatePoint"
          << "Sketcher_CreateArc"
          << "Sketcher_CreateCircle"
          << "Sketcher_CreateLine"
          << "Sketcher_CreatePolyline"
          << "Sketcher_CreateRectangle"
          << "Separator"
          << "Sketcher_CreateFillet"
          << "Sketcher_Trimming"
          << "Sketcher_External"
          << "Sketcher_ToggleConstruction"
          /*<< "Sketcher_CreateText"*/
          /*<< "Sketcher_CreateDraftLine"*/;

    Gui::ToolBarItem* cons = new Gui::ToolBarItem(root);
    cons->setCommand("Sketcher constraints");
    *cons << "Sketcher_ConstrainCoincident"
          << "Sketcher_ConstrainPointOnObject"
          << "Sketcher_ConstrainVertical"
          << "Sketcher_ConstrainHorizontal"
          << "Sketcher_ConstrainParallel"
          << "Sketcher_ConstrainPerpendicular"
          << "Sketcher_ConstrainTangent"
          << "Sketcher_ConstrainEqual"
          << "Sketcher_ConstrainSymmetric"
          << "Separator"
          << "Sketcher_ConstrainLock"
          << "Sketcher_ConstrainDistanceX"
          << "Sketcher_ConstrainDistanceY"
          << "Sketcher_ConstrainDistance"
          << "Sketcher_ConstrainRadius"
          << "Sketcher_ConstrainAngle";

    // Part Design
    Gui::ToolBarItem* partdesign = new Gui::ToolBarItem(root);
    partdesign->setCommand("Part Design");
    *partdesign
              << "PartDesign_Pad"
              << "PartDesign_Pocket"
              << "PartDesign_Revolution"
              << "PartDesign_Groove"
              << "PartDesign_Fillet"
              << "PartDesign_Chamfer"
              << "PartDesign_Mirrored"
              << "PartDesign_LinearPattern"
              << "PartDesign_PolarPattern"
              << "PartDesign_MultiTransform";

    // Drawing
    Gui::ToolBarItem* drawing = new Gui::ToolBarItem( root );
    drawing->setCommand("Drawings");
    *drawing << "Drawing_Open"
             << "Separator"
             << "Drawing_NewA3Landscape"
             << "Drawing_NewView"
             << "Drawing_ExportPage" ;

    // Raytracing
    Gui::ToolBarItem* raytracing = new Gui::ToolBarItem( root );
    raytracing->setCommand("Raytracing");
    *raytracing << "Raytracing_WriteView"
                << "Raytracing_WriteCamera"
                << "Raytracing_WritePart"
                << "Separator"
                << "Raytracing_NewPovrayProject" 
                << "Raytracing_NewPartSegment" 
                << "Raytracing_ExportProject"; 

    // Drafting ****************************************************************************************************
#   ifdef COMPLETE_USE_DRAFTING
    if (mgr.getCommandByName("Draft_Line")) {
        Gui::ToolBarItem* Drafting = new Gui::ToolBarItem( root );
        Drafting->setCommand("Drafting");
        *Drafting
            << "Draft_Line"
            << "Draft_Wire"
            << "Draft_Circle"
            << "Draft_Arc"
            << "Draft_Rectangle"
            << "Draft_Polygon"
            << "Draft_BSpline"
            << "Draft_Text"
            << "Draft_Dimension"
            << "Separator"
            << "Draft_Move"
            << "Draft_Rotate"
            << "Draft_Offset"
            << "Draft_Trimex"
            << "Draft_Upgrade"
            << "Draft_Downgrade"
            << "Draft_Scale"
            << "Draft_Edit"
            << "Draft_Drawing"
            << "Draft_WireToBSpline"
            << "Draft_AddPoint"
            << "Draft_DelPoint"
            << "Draft_Shape2DView"
        ;
    }
#   endif

    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    return root;
}

Gui::DockWindowItems* Workbench::setupDockWindows() const
{
    Gui::DockWindowItems* root = Gui::StdWorkbench::setupDockWindows();
    root->setVisibility(false); // hide all dock windows by default
    root->setVisibility("Std_CombiView",true); // except of the combi view
    return root;
}
