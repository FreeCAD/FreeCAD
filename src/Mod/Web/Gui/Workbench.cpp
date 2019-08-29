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

using namespace WebGui;

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Workbench", "Navigation");
#endif

/// @namespace WebGui @class Workbench
TYPESYSTEM_SOURCE(WebGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{
}

Workbench::~Workbench()
{
}

void Workbench::setupContextMenu(const char* recipient,Gui::MenuItem* item) const
{
    Q_UNUSED(recipient);
    Q_UNUSED(item);
    //if (strcmp(recipient,"View") == 0)
    //{
    //    Gui::MenuItem* StdViews = new Gui::MenuItem();
    //    StdViews->setCommand( "Standard views" );

    //    *StdViews << "Std_ViewAxo" << "Separator" << "Std_ViewFront" << "Std_ViewTop" << "Std_ViewRight"
    //              << "Std_ViewRear" << "Std_ViewBottom" << "Std_ViewLeft";

    //    *item << "Std_ViewFitAll" << "Std_ViewFitSelection" << StdViews
    //          << "Separator" << "Std_ViewDockUndockFullscreen";

    //    if ( Gui::Selection().countObjectsOfType(App::DocumentObject::getClassTypeId()) > 0 )
    //        *item << "Separator" << "Std_SetAppearance" << "Std_ToggleVisibility" << "Std_TreeSelection" 
    //              << "Std_RandomColor" << "Separator" << "Std_Delete";
    //}
    //else if (strcmp(recipient,"Tree") == 0)
    //{
    //    if ( Gui::Selection().countObjectsOfType(App::DocumentObject::getClassTypeId()) > 0 )
    //        *item << "Std_SetAppearance" << "Std_ToggleVisibility" 
    //              << "Std_RandomColor" << "Separator" << "Std_Delete";
    //}
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    return Gui::StdWorkbench::setupMenuBar();
    //Gui::CommandManager &mgr = Gui::Application::Instance->commandManager();
    //// Setup the default menu bar
    //Gui::MenuItem* menuBar = new Gui::MenuItem;

//    // File
//    Gui::MenuItem* file = new Gui::MenuItem( menuBar );
//    file->setCommand("&File");
//    *file << "Std_New" << "Std_Open" << "Separator" << "Std_CloseActiveWindow"
//          << "Std_CloseAllWindows" << "Separator" << "Std_Save" << "Std_SaveAs"
//          << "Separator" << "Std_Import" << "Std_Export" << "Std_ProjectInfo" 
//        //<< "Separator" << "Std_Print" << "Std_PrintPdf"
//          << "Separator" << "Std_RecentFiles" << "Separator" << "Std_Quit";
//
//    // Edit
//    Gui::MenuItem* edit = new Gui::MenuItem( menuBar );
//    edit->setCommand("&Edit");
//    *edit << "Std_Undo" << "Std_Redo" << "Separator" << "Std_Cut" << "Std_Copy"
//          << "Std_Paste" << "Std_DuplicateSelection" << "Separator"
//          << "Std_Refresh" << "Std_SelectAll" << "Std_Delete" << "Std_Placement"
//          << "Separator" << "Std_DlgPreferences";
//
//    // Standard views
//    Gui::MenuItem* stdviews = new Gui::MenuItem;
//    stdviews->setCommand("Standard views");
//    *stdviews << "Std_ViewFitAll" << "Std_ViewFitSelection" << "Std_ViewAxo"
//              << "Separator" << "Std_ViewFront" << "Std_ViewRight"
//              << "Std_ViewTop" << "Separator" << "Std_ViewRear" 
//              << "Std_ViewLeft" << "Std_ViewBottom";
//
//    // stereo
//    Gui::MenuItem* view3d = new Gui::MenuItem;
//    view3d->setCommand("&Stereo");
//    *view3d << "Std_ViewIvStereoRedGreen" << "Std_ViewIvStereoQuadBuff" 
//            << "Std_ViewIvStereoInterleavedRows" << "Std_ViewIvStereoInterleavedColumns" 
//            << "Std_ViewIvStereoOff" << "Separator" << "Std_ViewIvIssueCamPos";
//
//    // zoom
//    Gui::MenuItem* zoom = new Gui::MenuItem;
//    zoom->setCommand("&Zoom");
//    *zoom << "Std_ViewZoomIn" << "Std_ViewZoomOut" << "Separator" << "Std_ViewBoxZoom";
//
//    // Visibility
//    Gui::MenuItem* visu = new Gui::MenuItem;
//    visu->setCommand("Visibility");
//    *visu << "Std_ToggleVisibility" << "Std_ShowSelection" << "Std_HideSelection"
//          << "Separator" << "Std_ToggleObjects" << "Std_ShowObjects" << "Std_HideObjects";
//
//    // View
//    Gui::MenuItem* view = new Gui::MenuItem( menuBar );
//    view->setCommand("&View");
//    *view << "Std_ViewCreate" << "Std_OrthographicCamera" << "Std_PerspectiveCamera" << "Separator" 
//          << stdviews << "Std_FreezeViews" << "Separator" << view3d << zoom
//          << "Std_ViewDockUndockFullscreen" << "Std_ToggleClipPlane" << "Separator" << visu
//          << "Std_SetAppearance" << "Std_ToggleVisibility" << "Std_RandomColor" << "Separator" 
//          //<< "Std_MeasureDistance" << "Separator" 
//          << "Std_Workbench" << "Std_ToolBarMenu" << "Std_DockViewMenu" << "Separator" 
//          << "Std_ViewStatusBar" << "Std_UserInterface";
//
//    // Tools
//    Gui::MenuItem* tool = new Gui::MenuItem( menuBar );
//    tool->setCommand("&Tools");
//    *tool << "Std_CommandLine" << "Std_DlgParameter" << "Separator" << "Std_DlgMacroRecord"
//          << "Std_MacroStopRecord" << "Std_DlgMacroExecute" << "Std_DlgMacroExecuteDirect" 
//          << "Separator" << "Std_ViewScreenShot" << "Separator" << "Std_DlgCustomize";
//
//    // Mesh ****************************************************************************************************
//    Gui::MenuItem* mesh = new Gui::MenuItem( menuBar );
//
//    // submenu analyze
//    Gui::MenuItem* analyze = new Gui::MenuItem();
//    analyze->setCommand("Analyze");
//    *analyze << "Mesh_Evaluation" 
//             << "Mesh_EvaluateFacet" 
//             << "Mesh_CurvatureInfo" 
//             << "Separator" 
//             << "Mesh_EvaluateSolid" 
//             << "Mesh_BoundingBox";
//
//    // submenu boolean
//    Gui::MenuItem* boolean = new Gui::MenuItem();
//    boolean->setCommand("Boolean");
//    *boolean << "Mesh_Union" 
//             << "Mesh_Intersection" 
//             << "Mesh_Difference";
//
//    mesh->setCommand("&Meshes");
//    *mesh << "Mesh_Import" 
//          << "Mesh_Export" 
//          << "Mesh_FromGeometry" 
//          << "Separator"
//          << analyze 
//          << "Mesh_HarmonizeNormals" 
//          << "Mesh_FlipNormals" 
//          << "Separator" 
//          << "Mesh_FillupHoles" 
//          << "Mesh_FillInteractiveHole" 
//          << "Mesh_RemoveComponents"
//          << "Mesh_RemoveCompByHand" 
//          << "Separator" 
//          << "Mesh_BuildRegularSolid" 
//          << boolean << "Separator" 
//          << "Mesh_PolyCut"
//          << "Mesh_PolySplit" 
//          << "Mesh_PolySegm" 
//          << "Mesh_ToolMesh" 
//          << "Mesh_VertexCurvature";
//
//    // Part ****************************************************************************************************
//
//    Gui::MenuItem* part = new Gui::MenuItem(menuBar);
//    part->setCommand("&Part");
//
//    // submenu boolean
//    Gui::MenuItem* para = new Gui::MenuItem();
//    para->setCommand("Parametric");
//    *para << "Part_Box"
//          << "Part_Cylinder"
//          << "Part_Sphere"
//          << "Part_Cone"
//          << "Part_Torus"
//          << "Part_Primitives";
//    *part << para
//          << "Part_Boolean"
//          << "Part_Extrude"
//          << "Part_Revolve"
//          << "Part_Fillet";
//
//#   ifdef WEB_SHOW_SKETCHER
//    if (mgr.getCommandByName("Sketcher_NewSketch")) {
//        Gui::MenuItem* sketch = new Gui::MenuItem(menuBar);
//        sketch->setCommand("Ske&tch");
//        *sketch 
//            << "Sketcher_NewSketch"
//            << "Separator" 
//            << "PartDesign_Pad" 
//            << "PartDesign_Fillet"
//        ;
//    }
//#   endif
//
//    // Drawing ****************************************************************************************************
//
//    Gui::MenuItem* drawing = new Gui::MenuItem(menuBar);
//
//    drawing->setCommand("&Drawing");
//    *drawing
//        << "Drawing_Open" 
//        << "Separator" 
//        << "Drawing_NewA3Landscape"  
//        << "Drawing_NewView" 
//        << "Drawing_ExportPage"
//    ;
//
//    // Raytracing ****************************************************************************************************
//
//    Gui::MenuItem* raytracing = new Gui::MenuItem(menuBar);
//
//    raytracing->setCommand("&Raytracing");
//    *raytracing 
//        << "Raytracing_WriteView" 
//        << "Raytracing_WriteCamera" 
//        << "Raytracing_WritePart"; 
//    ;
//
//    // Drafting ****************************************************************************************************
//#   ifdef WEB_USE_DRAFTING
//    if (mgr.getCommandByName("Draft_Line")) {
//        Gui::MenuItem* Drafting = new Gui::MenuItem(menuBar);
//
//        Drafting->setCommand("&Drafting");
//        *Drafting 
//            << "Draft_SelectPlane"
//            << "Draft_Line"
//            << "Draft_Polyline"
//            << "Draft_Circle"
//            << "Draft_Arc"
//            << "Draft_Rectangle"
//            << "Draft_Text"
//            << "Draft_Dimension"
//            << "Separator" 
//            << "Draft_Move" 
//            << "Draft_Rotate" 
//            << "Draft_Offset" 
//            << "Draft_Trimex" 
//            << "Draft_Upgrade" 
//            << "Draft_Downgrade" 
//            << "Draft_Scale" 
//            << "Separator" 
//            << "Draft_ApplyStyle"
//        ;
//    }
//#   endif
//
//    // xxx ****************************************************************************************************
//
//
//    // Windows
//    Gui::MenuItem* wnd = new Gui::MenuItem( menuBar );
//    wnd->setCommand("&Windows");
//    *wnd << "Std_ActivateNextWindow" << "Std_ActivatePrevWindow" << "Separator"
//         << "Std_TileWindows" << "Std_CascadeWindows"
//         << "Std_ArrangeIcons" << "Separator" << "Std_WindowsMenu" << "Std_Windows";
//    
//    // help ****************************************************************************************************
//    // Separator
//    Gui::MenuItem* sep = new Gui::MenuItem( menuBar );
//    sep->setCommand( "Separator" );
//
//    // Help
//    Gui::MenuItem* helpWebsites = new Gui::MenuItem;
//    helpWebsites->setCommand("&Online-help");
//    *helpWebsites << "Std_OnlineHelpWebsite" 
//                  << "Std_FreeCADWebsite" 
//                  << "Std_PythonWebsite";
//
//    Gui::MenuItem* help = new Gui::MenuItem( menuBar );
//    help->setCommand("&Help");
//    *help << "Std_OnlineHelp" 
//          << "Std_PythonHelp"
//          << helpWebsites  
//          << "Separator" 
//          << "Test_Test" 
//          << "Separator" 
//          << "Std_About"
//          << "Std_AboutQt" 
//          << "Separator" 
//          << "Std_WhatsThis" ;
//
//      return menuBar;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();

    // web navigation toolbar
    Gui::ToolBarItem* navigation = new Gui::ToolBarItem(root);
    navigation->setCommand("Navigation");
    *navigation << "Web_BrowserSetURL"
                << "Separator"
                << "Web_OpenWebsite"
                << "Separator" 
                << "Web_BrowserBack" 
                << "Web_BrowserNext" 
                << "Web_BrowserRefresh"
                << "Web_BrowserStop"
                << "Separator"
                << "Web_BrowserZoomIn"
                << "Web_BrowserZoomOut";

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
    //root->setVisibility(false); // hide all dock windows by default
    //root->setVisibility("Std_CombiView",true); // except of the combi view
    return root;
}
