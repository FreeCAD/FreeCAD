/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QDockWidget>
# include <QStatusBar>
#endif

#include "Workbench.h"
#include "WorkbenchPy.h"
#include "PythonWorkbenchPy.h"
#include "MenuManager.h"
#include "ToolBarManager.h"
#include "DockWindowManager.h"
#include "Application.h"
#include "Action.h"
#include "Command.h"
#include "Control.h"
#include "ToolBoxManager.h"
#include "Window.h"
#include "Selection.h"
#include "MainWindow.h"
#include <Gui/CombiView.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskWatcher.h>

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <Base/Interpreter.h>

using namespace Gui;

/** \defgroup workbench Workbench Framework
    \ingroup GUI

    FreeCAD provides the possibility to have one or more workbenches for a module.
    A workbench changes the appearance of the main window in that way that it defines toolbars, items in the toolbox, menus or the context menu and dockable windows that are shown to the user.
    The idea behind this concept is that the user should see only the functions that are required for the task that he is doing at this moment and not to show dozens of unneeded functions which the user never uses.

    \section stepbystep Step by step
    Here follows a short description of how your own workbench can be added to a module.

    \subsection newClass Inherit either from Workbench or StdWorkbench
    First you have to subclass either \ref Gui::Workbench "Workbench" or \ref Gui::StdWorkbench "StdWorkbench" and reimplement the methods \ref Gui::Workbench::setupMenuBar() "setupMenuBar()", \ref Gui::Workbench::setupToolBars() "setupToolBars()", \ref Gui::Workbench::setupCommandBars() "setupCommandBars()" and \ref Gui::Workbench::setupDockWindows() "setupDockWindows()".

    The difference between both classes is that these methods of %Workbench are pure virtual while StdWorkbench defines already the standard menus and toolbars, such as the 'File', 'Edit', ..., 'Help' menus with their common functions.

    If your class derives from %Workbench then you have to define your menus, toolbars and toolbox items from scratch while deriving from StdWorkbench you have the possibility to add your preferred functions or even remove some unneeded functions.
 * \code
 *
 * class MyWorkbench : public StdWorkbench
 * {
 *  ...
 * protected:
 *   MenuItem* setupMenuBar() const
 *   {
 *     MenuItem* root = StdWorkbench::setupMenuBar();
 *     // your changes
 *     return root;
 *   }
 *   ToolBarItem* setupToolBars() const
 *   {
 *     ToolBarItem* root = StdWorkbench::setupToolBars();
 *     // your changes
 *     return root;
 *   }
 *   ToolBarItem* setupCommandBars() const
 *   {
 *     ToolBarItem* root = StdWorkbench::setupCommandBars();
 *     // your changes
 *     return root;
 *   }
 * };
 *
 * \endcode
 * or
 * \code
 *
 * class MyWorkbench : public Workbench
 * {
 *  ...
 * protected:
 *   MenuItem* setupMenuBar() const
 *   {
 *     MenuItem* root = new MenuItem;
 *     // setup from scratch
 *     return root;
 *   }
 *   ToolBarItem* setupToolBars() const
 *   {
 *     ToolBarItem* root = new ToolBarItem;
 *     // setup from scratch
 *     return root;
 *   }
 *   ToolBarItem* setupCommandBars() const
 *   {
 *     ToolBarItem* root = new ToolBarItem;
 *     // setup from scratch
 *     return root;
 *   }
 * };
 * 
 * \endcode
 *
 * \subsection customizeWorkbench Customizing the workbench
 * If you want to customize your workbench by adding or removing items you can use the ToolBarItem class for customizing toolbars and the MenuItem class
 * for menus. Both classes behave basically the same.
 * To add a new menu item you can do it as follows
 * \code
 *   MenuItem* setupMenuBar() const
 *   {
 *     MenuItem* root = StdWorkbench::setupMenuBar();
 *     // create a sub menu
 *     MenuItem* mySub = new MenuItem; // note: no parent is given
 *     mySub->setCommand( "My &Submenu" );
 *     *mySub << "Std_Undo" << "Std_Redo";
 *
 *     // My menu
 *     MenuItem* myMenu = new MenuItem( root );
 *     myMenu->setCommand( "&My Menu" );
 *     // fill up the menu with some command items
 *     *myMenu << mySub << "Separator" << "Std_Cut" << "Std_Copy" << "Std_Paste" << "Separator" << "Std_Undo" << "Std_Redo";
 *   }
 * \endcode
 *
 * Toolbars can be customized the same way unless that you shouldn't create subitems (there are no subtoolbars).
 *
 * \subsection regWorkbench Register your workbench
 * Once you have implemented your workbench class you have to register it to make it known to the FreeCAD core system. You must make sure that the step
 * of registration is performed only once. A good place to do it is e.g. in the global function initMODULEGui in AppMODULEGui.cpp where MODULE stands
 * for the name of your module. Just add the line
 * \code
 * MODULEGui::MyWorkbench::init();
 * \endcode
 * somewhere there.
 *
 * \subsection itemWorkbench Create an item for your workbench
 * Though your workbench has been registered now,  at this stage you still cannot invoke it yet. Therefore you must create an item in the list of all visible
 * workbenches. To perform this step you must open your InitGui.py (a Python file) and do some adjustments. The file contains already a Python class
 * MODULEWorkbench that implements the Activate() method (it imports the needed library). You can also implement the GetIcon() method to set your own icon for
 * your workbench, if not, the default FreeCAD icon is taken, and finally the most important method GetClassName(). that represents the link between
 * Python and C++. This method must return the name of the associated C++ including namespace. In this case it must the string "ModuleGui::MyWorkbench".
 * At the end you can change the line from
 * \code
 * Gui.addWorkbench("MODULE design",MODULEWorkbench())
 * \endcode
 * to
 * \code
 * Gui.addWorkbench("My workbench",MODULEWorkbench())
 * \endcode
 * or whatever you want.
 * \note You must make sure to choose a unique name for your workbench (in this example "My workbench"). Since FreeCAD doesn't provide a mechanism for
 * this you have to care on your own.
 *
 * \section moredetails More details and limitations
 * One of the key concepts of the workbench framework is to load a module at runtime when the user needs some function that it
 * provides. So, if the user doesn't need a module it never gets loaded into RAM. This speeds up the startup procedure of

 * FreeCAD and saves memory.
 * At startup FreeCAD scans all module directories and invokes InitGui.py. So an item for a workbench gets created. If the user
 * clicks on such an item the matching module gets loaded, the C++ workbench gets registered and activated.
 *
 * The user is able to modify a workbench (Edit|Customize). E.g. he can add new toolbars or items for the toolbox and add his preferred
 * functions to them. But he has only full control over "his" toolbars, the default workbench items cannot be modified or even removed.
 *
 * FreeCAD provides also the possibility to define pure Python workbenches. Such workbenches are temporarily only and are lost after exiting
 * the FreeCAD session. But if you want to keep your Python workbench you can write a macro and attach it with a user defined button or just
 * perform the macro during the next FreeCAD session.
 * Here follows a short example of how to create and embed a workbench in Python
 * \code
 * w=Workbench()                                              # creates a standard workbench (the same as StdWorkbench in C++)
 * w.MenuText = "My Workbench"                                # the text that will appear in the combo box
 * dir(w)                                                     # lists all available function of the object
 * FreeCADGui.addWorkbench(w)                                 # Creates an item for our workbenmch now
 *                                                            # Note: We must first add the workbench to run some initialization code
 *                                                            # Then we are ready to customize the workbench
 * list = ["Std_Test1", "Std_Test2", "Std_Test3"]             # creates a list of new functions
 * w.appendMenu("Test functions", list)                       # creates a new menu with these functions
 * w.appendToolbar("Test", list)                              # ... and also a new toolbar
 * \endcode
 */

/// @namespace Gui @class Workbench
TYPESYSTEM_SOURCE_ABSTRACT(Gui::Workbench, Base::BaseClass)

Workbench::Workbench()
  : _name("")
{
}

Workbench::~Workbench()
{
}

std::string Workbench::name() const
{
    return _name;
}

void Workbench::setName(const std::string& name)
{
    _name = name;
}

void Workbench::setupCustomToolbars(ToolBarItem* root, const char* toolbar) const
{
    std::string name = this->name();
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
        ->GetGroup("Workbench");
    // workbench specific custom toolbars
    if (hGrp->HasGroup(name.c_str())) {
        hGrp = hGrp->GetGroup(name.c_str());
        if (hGrp->HasGroup(toolbar)) {
            hGrp = hGrp->GetGroup(toolbar);
            setupCustomToolbars(root, hGrp);
        }
    }

    // for this workbench global toolbars are not allowed
    if (getTypeId() == NoneWorkbench::getClassTypeId())
        return;

    // application-wide custom toolbars
    hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
        ->GetGroup("Workbench");
    if (hGrp->HasGroup("Global")) {
        hGrp = hGrp->GetGroup("Global");
        if (hGrp->HasGroup(toolbar)) {
            hGrp = hGrp->GetGroup(toolbar);
            setupCustomToolbars(root, hGrp);
        }
    }
}

void Workbench::setupCustomToolbars(ToolBarItem* root, const Base::Reference<ParameterGrp>& hGrp) const
{
    std::vector<Base::Reference<ParameterGrp> > hGrps = hGrp->GetGroups();
    CommandManager& rMgr = Application::Instance->commandManager();
    std::string separator = "Separator";
    for (std::vector<Base::Reference<ParameterGrp> >::iterator it = hGrps.begin(); it != hGrps.end(); ++it) {
        bool active = (*it)->GetBool("Active", true);
        if (!active) // ignore this toolbar
            continue;
        ToolBarItem* bar = new ToolBarItem(root);
        bar->setCommand("Custom");

        // get the elements of the subgroups
        std::vector<std::pair<std::string,std::string> > items = hGrp->GetGroup((*it)->GetGroupName())->GetASCIIMap();
        for (std::vector<std::pair<std::string,std::string> >::iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            if (it2->first.substr(0, separator.size()) == separator) {
                *bar << "Separator";
            }
            else if (it2->first == "Name") {
                bar->setCommand(it2->second);
            }
            else {
                Command* pCmd = rMgr.getCommandByName(it2->first.c_str());
                if (!pCmd) { // unknown command
                    // first try the module name as is
                    std::string pyMod = it2->second;
                    try {
                        Base::Interpreter().loadModule(pyMod.c_str());
                        // Try again
                        pCmd = rMgr.getCommandByName(it2->first.c_str());
                    }
                    catch(const Base::Exception&) {
                    }
                }

                // still not there?
                if (!pCmd) {
                    // add the 'Gui' suffix
                    std::string pyMod = it2->second + "Gui";
                    try {
                        Base::Interpreter().loadModule(pyMod.c_str());
                        // Try again
                        pCmd = rMgr.getCommandByName(it2->first.c_str());
                    }
                    catch(const Base::Exception&) {
                    }
                }

                if (pCmd) {
                    *bar << it2->first; // command name
                }
            }
        }
    }
}

void Workbench::setupCustomShortcuts() const
{
    // Assigns user defined accelerators
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter();
    if (hGrp->HasGroup("Shortcut")) {
        hGrp = hGrp->GetGroup("Shortcut");
        // Get all user defined shortcuts
        const CommandManager& cCmdMgr = Application::Instance->commandManager();
        std::vector<std::pair<std::string,std::string> > items = hGrp->GetASCIIMap();
        for (std::vector<std::pair<std::string,std::string> >::iterator it = items.begin(); it != items.end(); ++it) {
            Command* cmd = cCmdMgr.getCommandByName(it->first.c_str());
            if (cmd && cmd->getAction()) {
                // may be UTF-8 encoded
                QString str = QString::fromUtf8(it->second.c_str());
                QKeySequence shortcut = str;
                cmd->getAction()->setShortcut(shortcut.toString(QKeySequence::NativeText));
            }
        }
    }
}

void Workbench::setupContextMenu(const char* recipient,MenuItem* item) const
{
    Q_UNUSED(recipient);
    Q_UNUSED(item);
}

void Workbench::createMainWindowPopupMenu(MenuItem*) const
{
}

void Workbench::activated()
{
}

void Workbench::deactivated()
{
}

bool Workbench::activate()
{
    ToolBarItem* tb = setupToolBars();
    setupCustomToolbars(tb, "Toolbar");
    ToolBarManager::getInstance()->setup( tb );
    delete tb;

    //ToolBarItem* cb = setupCommandBars();
    //setupCustomToolbars(cb, "Toolboxbar");
    //ToolBoxManager::getInstance()->setup( cb );
    //delete cb;

    DockWindowItems* dw = setupDockWindows();
    DockWindowManager::instance()->setup( dw );
    delete dw;

    MenuItem* mb = setupMenuBar();
    MenuManager::getInstance()->setup( mb );
    delete mb;

    setupCustomShortcuts();

    return true;
}

void Workbench::retranslate() const
{
    ToolBarManager::getInstance()->retranslate();
    //ToolBoxManager::getInstance()->retranslate();
    DockWindowManager::instance()->retranslate();
    MenuManager::getInstance()->retranslate();
}

PyObject* Workbench::getPyObject()
{
    return new WorkbenchPy(this);
}

void Workbench::addTaskWatcher(const std::vector<Gui::TaskView::TaskWatcher*> &Watcher)
{
    Gui::TaskView::TaskView* taskView = Control().taskPanel();
    if (taskView)
        taskView->addTaskWatcher(Watcher);
}

void Workbench::removeTaskWatcher(void)
{
    Gui::TaskView::TaskView* taskView = Control().taskPanel();
    if (taskView)
        taskView->clearTaskWatcher();
}

// --------------------------------------------------------------------

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Workbench", "&File");
    qApp->translate("Workbench", "&Edit");
    qApp->translate("Workbench", "Standard views");
    qApp->translate("Workbench", "&Stereo");
    qApp->translate("Workbench", "&Zoom");
    qApp->translate("Workbench", "Visibility");
    qApp->translate("Workbench", "&View");
    qApp->translate("Workbench", "&Tools");
    qApp->translate("Workbench", "&Macro");
    qApp->translate("Workbench", "&Windows");
    qApp->translate("Workbench", "&On-line help");
    qApp->translate("Workbench", "&Help");
    qApp->translate("Workbench", "File");
    qApp->translate("Workbench", "Macro");
    qApp->translate("Workbench", "View");
    qApp->translate("Workbench", "Special Ops");
#endif

#if 0 // needed for the application menu on OSX
    qApp->translate("MAC_APPLICATION_MENU", "Services");
    qApp->translate("MAC_APPLICATION_MENU", "Hide %1");
    qApp->translate("MAC_APPLICATION_MENU", "Hide Others");
    qApp->translate("MAC_APPLICATION_MENU", "Show All");
    qApp->translate("MAC_APPLICATION_MENU", "Preferences...");
    qApp->translate("MAC_APPLICATION_MENU", "Quit %1");
    qApp->translate("MAC_APPLICATION_MENU", "About %1");
#endif

TYPESYSTEM_SOURCE(Gui::StdWorkbench, Gui::Workbench)

StdWorkbench::StdWorkbench()
  : Workbench()
{
}

StdWorkbench::~StdWorkbench()
{
}

void StdWorkbench::setupContextMenu(const char* recipient, MenuItem* item) const
{
    if (strcmp(recipient,"View") == 0)
    {
        MenuItem* StdViews = new MenuItem;
        StdViews->setCommand( "Standard views" );

        *StdViews << "Std_ViewIsometric" << "Separator" << "Std_ViewFront" << "Std_ViewTop" << "Std_ViewRight"
                  << "Std_ViewRear" << "Std_ViewBottom" << "Std_ViewLeft"
                  << "Separator" << "Std_ViewRotateLeft" << "Std_ViewRotateRight";

        MenuItem *measure = new MenuItem();
        measure->setCommand("Measure");
        *measure << "View_Measure_Toggle_All" << "View_Measure_Clear_All";

        *item << "Std_ViewFitAll" << "Std_ViewFitSelection" << "Std_DrawStyle" << StdViews << measure
              << "Separator" << "Std_ViewDockUndockFullscreen";

        if (Gui::Selection().countObjectsOfType(App::DocumentObject::getClassTypeId()) > 0) {
            *item << "Separator" << "Std_SetAppearance" << "Std_ToggleVisibility"
                  << "Std_ToggleSelectability" << "Std_TreeSelection" 
                  << "Std_RandomColor" << "Separator" << "Std_Delete";
        }
    }
    else if (strcmp(recipient,"Tree") == 0)
    {
        if (Gui::Selection().countObjectsOfType(App::DocumentObject::getClassTypeId()) > 0) {
            *item << "Std_ToggleVisibility" << "Std_ShowSelection" << "Std_HideSelection"
                  << "Std_ToggleSelectability" << "Separator" << "Std_SetAppearance"
                  << "Std_RandomColor" << "Std_Cut" << "Std_Copy" << "Std_Paste"
                  << "Separator" << "Std_Delete";
        }
    }
}

void StdWorkbench::createMainWindowPopupMenu(MenuItem* item) const
{
    *item << "Std_DlgCustomize";
}

MenuItem* StdWorkbench::setupMenuBar() const
{
    // Setup the default menu bar
    MenuItem* menuBar = new MenuItem;

    // File
    MenuItem* file = new MenuItem( menuBar );
    file->setCommand("&File");
    *file << "Std_New" << "Std_Open" << "Separator" << "Std_CloseActiveWindow"
          << "Std_CloseAllWindows" << "Separator" << "Std_Save" << "Std_SaveAs"
          << "Std_SaveCopy" << "Std_Revert" << "Separator" << "Std_Import"
          << "Std_Export" << "Std_MergeProjects" << "Std_ProjectInfo"
          << "Separator" << "Std_Print" << "Std_PrintPreview" << "Std_PrintPdf"
          << "Separator" << "Std_RecentFiles" << "Separator" << "Std_Quit";

    // Edit
    MenuItem* edit = new MenuItem( menuBar );
    edit->setCommand("&Edit");
    *edit << "Std_Undo" << "Std_Redo" << "Separator" << "Std_Cut" << "Std_Copy"
          << "Std_Paste" << "Std_DuplicateSelection" << "Separator"
          << "Std_Refresh" << "Std_BoxSelection" << "Std_SelectAll" << "Std_Delete"
          << "Separator" << "Std_Placement" /*<< "Std_TransformManip"*/ << "Std_Alignment"
          << "Std_Edit" << "Separator" << "Std_DlgPreferences";

    MenuItem* axoviews = new MenuItem;
    axoviews->setCommand("Axonometric");
    *axoviews << "Std_ViewIsometric"
              << "Std_ViewDimetric"
              << "Std_ViewTrimetric";

    // Standard views
    MenuItem* stdviews = new MenuItem;
    stdviews->setCommand("Standard views");
    *stdviews << "Std_ViewFitAll" << "Std_ViewFitSelection" << axoviews
              << "Separator" << "Std_ViewFront" << "Std_ViewTop"
              << "Std_ViewRight" << "Separator" << "Std_ViewRear"
              << "Std_ViewBottom" << "Std_ViewLeft"
              << "Separator" << "Std_ViewRotateLeft" << "Std_ViewRotateRight";

    // stereo
    MenuItem* view3d = new MenuItem;
    view3d->setCommand("&Stereo");
    *view3d << "Std_ViewIvStereoRedGreen" << "Std_ViewIvStereoQuadBuff" 
            << "Std_ViewIvStereoInterleavedRows" << "Std_ViewIvStereoInterleavedColumns"
            << "Std_ViewIvStereoOff" << "Separator" << "Std_ViewIvIssueCamPos";

    // zoom
    MenuItem* zoom = new MenuItem;
    zoom->setCommand("&Zoom");
    *zoom << "Std_ViewZoomIn" << "Std_ViewZoomOut" << "Separator" << "Std_ViewBoxZoom";

    // Visibility
    MenuItem* visu = new MenuItem;
    visu->setCommand("Visibility");
    *visu << "Std_ToggleVisibility" << "Std_ShowSelection" << "Std_HideSelection"
          << "Std_SelectVisibleObjects"
          << "Separator" << "Std_ToggleObjects" << "Std_ShowObjects" << "Std_HideObjects" 
          << "Separator" << "Std_ToggleSelectability"
          << "Separator" << "View_Measure_Toggle_All" << "View_Measure_Clear_All";

    // View
    MenuItem* view = new MenuItem( menuBar );
    view->setCommand("&View");
    *view << "Std_ViewCreate" << "Std_OrthographicCamera" << "Std_PerspectiveCamera" << "Std_MainFullscreen" << "Separator"
          << stdviews << "Std_FreezeViews" << "Std_DrawStyle" << "Separator" << view3d << zoom
          << "Std_ViewDockUndockFullscreen" << "Std_TreeViewDocument" << "Std_AxisCross" << "Std_ToggleClipPlane"
          << "Std_TextureMapping" 
#ifdef BUILD_VR
          << "Std_ViewVR"
#endif 
          << "Separator" << visu
          << "Std_ToggleVisibility" << "Std_ToggleNavigation"
          << "Std_SetAppearance" << "Std_RandomColor" << "Separator"
          << "Std_Workbench" << "Std_ToolBarMenu" << "Std_DockViewMenu" << "Separator"
          << "Std_ViewStatusBar";

    // Tools
    MenuItem* tool = new MenuItem( menuBar );
    tool->setCommand("&Tools");
    *tool << "Std_DlgParameter" << "Separator"
          << "Std_ViewScreenShot" << "Std_SceneInspector"
          << "Std_ExportGraphviz" << "Std_ProjectUtil" << "Separator"
          << "Std_MeasureDistance" << "Separator" 
          << "Std_DemoMode" << "Std_UnitsCalculator" << "Separator" << "Std_DlgCustomize";
#ifdef BUILD_ADDONMGR
    *tool << "Std_AddonMgr";
#endif

    // Macro
    MenuItem* macro = new MenuItem( menuBar );
    macro->setCommand("&Macro");
    *macro << "Std_DlgMacroRecord" << "Std_MacroStopRecord" << "Std_DlgMacroExecute"
           << "Separator" << "Std_DlgMacroExecuteDirect" << "Std_MacroStartDebug"
           << "Std_MacroStopDebug" << "Std_MacroStepOver" << "Std_MacroStepInto"
           << "Std_ToggleBreakpoint";

    // Windows
    MenuItem* wnd = new MenuItem( menuBar );
    wnd->setCommand("&Windows");
    *wnd << "Std_ActivateNextWindow" << "Std_ActivatePrevWindow" << "Separator"
         << "Std_TileWindows" << "Std_CascadeWindows"
         << "Std_ArrangeIcons" << "Separator" << "Std_WindowsMenu" << "Std_Windows";

    // Separator
    MenuItem* sep = new MenuItem( menuBar );
    sep->setCommand( "Separator" );

    // Help
    MenuItem* help = new MenuItem( menuBar );
    help->setCommand("&Help");
    *help << "Std_OnlineHelp" << "Std_FreeCADWebsite"
          << "Std_FreeCADUserHub" << "Std_FreeCADPowerUserHub"
          << "Std_PythonHelp" << "Std_FreeCADForum"
          << "Std_FreeCADFAQ" << "Std_About" << "Std_WhatsThis";

    return menuBar;
}

ToolBarItem* StdWorkbench::setupToolBars() const
{
    ToolBarItem* root = new ToolBarItem;

    // File
    ToolBarItem* file = new ToolBarItem( root );
    file->setCommand("File");
    *file << "Std_New" << "Std_Open" << "Std_Save" << "Std_Print" << "Separator" << "Std_Cut"
          << "Std_Copy" << "Std_Paste" << "Separator" << "Std_Undo" << "Std_Redo" << "Separator"
          << "Std_Refresh" << "Separator" << "Std_WhatsThis";

    // Workbench switcher
    ToolBarItem* wb = new ToolBarItem( root );
    wb->setCommand("Workbench");
    *wb << "Std_Workbench";

    // Macro
    ToolBarItem* macro = new ToolBarItem( root );
    macro->setCommand("Macro");
    *macro << "Std_DlgMacroRecord" << "Std_MacroStopRecord" << "Std_DlgMacroExecute"
           << "Std_DlgMacroExecuteDirect";

    // View
    ToolBarItem* view = new ToolBarItem( root );
    view->setCommand("View");
    *view << "Std_ViewFitAll" << "Std_ViewFitSelection" << "Std_DrawStyle" << "Separator" << "Std_ViewIsometric" << "Separator" << "Std_ViewFront"
          << "Std_ViewTop" << "Std_ViewRight" << "Separator" << "Std_ViewRear" << "Std_ViewBottom"
          << "Std_ViewLeft" << "Separator" << "Std_MeasureDistance" ;
    
    // Structure
    ToolBarItem* structure = new ToolBarItem( root );
    structure->setCommand("Structure");
    *structure << "Std_Part" << "Std_Group";
          
    return root;
}

ToolBarItem* StdWorkbench::setupCommandBars() const
{
    ToolBarItem* root = new ToolBarItem;

    // View
    ToolBarItem* view = new ToolBarItem( root );
    view->setCommand("Standard views");
    *view << "Std_ViewFitAll" << "Std_ViewFitSelection" << "Std_ViewIsometric" << "Separator"
          << "Std_ViewFront" << "Std_ViewRight" << "Std_ViewTop" << "Separator"
          << "Std_ViewRear" << "Std_ViewLeft" << "Std_ViewBottom";
    // Special Ops
    ToolBarItem* macro = new ToolBarItem( root );
    macro->setCommand("Special Ops");
    *macro << "Std_DlgParameter" << "Std_DlgPreferences" << "Std_DlgMacroRecord" << "Std_MacroStopRecord"
           << "Std_DlgMacroExecute" << "Std_DlgCustomize";

    return root;
}

DockWindowItems* StdWorkbench::setupDockWindows() const
{
    DockWindowItems* root = new DockWindowItems();
    root->addDockWidget("Std_ToolBox", Qt::RightDockWidgetArea, false, false);
    //root->addDockWidget("Std_HelpView", Qt::RightDockWidgetArea, true, false);
    root->addDockWidget("Std_TreeView", Qt::LeftDockWidgetArea, true, false);
    root->addDockWidget("Std_PropertyView", Qt::LeftDockWidgetArea, true, false);
    root->addDockWidget("Std_SelectionView", Qt::LeftDockWidgetArea, false, false);
    root->addDockWidget("Std_CombiView", Qt::LeftDockWidgetArea, false, false);
    root->addDockWidget("Std_ReportView", Qt::BottomDockWidgetArea, true, true);
    root->addDockWidget("Std_PythonView", Qt::BottomDockWidgetArea, true, true);
    
    //Dagview through parameter.
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
          GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("DAGView");
    bool enabled = group->GetBool("Enabled", false);
    if (enabled)
      root->addDockWidget("Std_DAGView", Qt::RightDockWidgetArea, false, false);
    
    return root;
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::BlankWorkbench, Gui::Workbench)

BlankWorkbench::BlankWorkbench()
  : Workbench()
{
}

BlankWorkbench::~BlankWorkbench()
{
}

void BlankWorkbench::activated()
{
    QList<QDockWidget*> dw = getMainWindow()->findChildren<QDockWidget*>();
    for (QList<QDockWidget*>::iterator it = dw.begin(); it != dw.end(); ++it)
        (*it)->toggleViewAction()->setVisible(false);
    getMainWindow()->statusBar()->hide();
}

void BlankWorkbench::deactivated()
{
    getMainWindow()->statusBar()->show();
}

void BlankWorkbench::setupContextMenu(const char* recipient,MenuItem* item) const
{
    Q_UNUSED(recipient);
    Q_UNUSED(item);
}

MenuItem* BlankWorkbench::setupMenuBar() const
{
    return new MenuItem();
}

ToolBarItem* BlankWorkbench::setupToolBars() const
{
    return new ToolBarItem();
}

ToolBarItem* BlankWorkbench::setupCommandBars() const
{
    return new ToolBarItem();
}

DockWindowItems* BlankWorkbench::setupDockWindows() const
{
    return new DockWindowItems();
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::NoneWorkbench, Gui::StdWorkbench)

NoneWorkbench::NoneWorkbench()
  : StdWorkbench()
{
}

NoneWorkbench::~NoneWorkbench()
{
}

void NoneWorkbench::setupContextMenu(const char* recipient,MenuItem* item) const
{
    Q_UNUSED(recipient);
    Q_UNUSED(item);
}

MenuItem* NoneWorkbench::setupMenuBar() const
{
    // Setup the default menu bar
    MenuItem* menuBar = new MenuItem;

    // File
    MenuItem* file = new MenuItem( menuBar );
    file->setCommand("&File");
    *file << "Std_Quit";

    // Edit
    MenuItem* edit = new MenuItem( menuBar );
    edit->setCommand("&Edit");
    *edit << "Std_DlgPreferences";

    // View
    MenuItem* view = new MenuItem( menuBar );
    view->setCommand("&View");
    *view << "Std_Workbench";

    // Separator
    MenuItem* sep = new MenuItem( menuBar );
    sep->setCommand("Separator");

    // Help
    MenuItem* help = new MenuItem( menuBar );
    help->setCommand("&Help");
    *help << "Std_OnlineHelp" << "Std_About";

    return menuBar;
}

ToolBarItem* NoneWorkbench::setupToolBars() const
{
    ToolBarItem* root = new ToolBarItem;
    return root;
}

ToolBarItem* NoneWorkbench::setupCommandBars() const
{
    ToolBarItem* root = new ToolBarItem;
    return root;
}

DockWindowItems* NoneWorkbench::setupDockWindows() const
{
    DockWindowItems* root = new DockWindowItems();
    root->addDockWidget("Std_ReportView", Qt::BottomDockWidgetArea, true, false);
    return root;
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::TestWorkbench, Gui::Workbench)

TestWorkbench::TestWorkbench()
  : StdWorkbench()
{
}

TestWorkbench::~TestWorkbench()
{
}

MenuItem* TestWorkbench::setupMenuBar() const
{
    // Setup the default menu bar
    MenuItem* menuBar = StdWorkbench::setupMenuBar();

    MenuItem* item = menuBar->findItem("&Help");
    item->removeItem(item->findItem("Std_WhatsThis"));

    // Test commands
    MenuItem* test = new MenuItem;
    menuBar->insertItem( item, test );
    test->setCommand( "Test &Commands" );
    *test << "Std_Test1" << "Std_Test2" << "Std_Test3" << "Std_Test4" << "Std_Test5"
          << "Std_Test6" << "Std_Test7" << "Std_Test8";

    // Inventor View
    MenuItem* opiv = new MenuItem;
    menuBar->insertItem( item, opiv );
    opiv->setCommand("&Inventor View");
    *opiv << "Std_ViewExample1" << "Std_ViewExample2" << "Std_ViewExample3";

    return menuBar;
}

ToolBarItem* TestWorkbench::setupToolBars() const
{
    return 0;
}

ToolBarItem* TestWorkbench::setupCommandBars() const
{
    return 0;
}

// -----------------------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Gui::PythonBaseWorkbench, Gui::Workbench)

PythonBaseWorkbench::PythonBaseWorkbench()
  : _menuBar(0), _contextMenu(0), _toolBar(0), _commandBar(0), _workbenchPy(0)
{
}

PythonBaseWorkbench::~PythonBaseWorkbench()
{
    delete _menuBar;
    delete _contextMenu;
    delete _toolBar;
    delete _commandBar;
    if (_workbenchPy) {
        _workbenchPy->setInvalid();
        _workbenchPy->DecRef();
    }
}

PyObject* PythonBaseWorkbench::getPyObject()
{
    if (!_workbenchPy)
    {
        _workbenchPy = new PythonWorkbenchPy(this);
    }

    // Increment every time when this object is returned
    _workbenchPy->IncRef();

    return _workbenchPy;
}

MenuItem* PythonBaseWorkbench::setupMenuBar() const
{
    return _menuBar->copy();
}

ToolBarItem* PythonBaseWorkbench::setupToolBars() const
{
    return _toolBar->copy();
}

ToolBarItem* PythonBaseWorkbench::setupCommandBars() const
{
    return _commandBar->copy();
}

DockWindowItems* PythonBaseWorkbench::setupDockWindows() const
{
    return new DockWindowItems();
}

void PythonBaseWorkbench::setupContextMenu(const char* recipient, MenuItem* item) const
{
    Q_UNUSED(recipient);
    QList<MenuItem*> items = _contextMenu->getItems();
    for (QList<MenuItem*>::Iterator it = items.begin(); it != items.end(); ++it) {
        item->appendItem((*it)->copy());
    }
}

void PythonBaseWorkbench::appendMenu(const std::list<std::string>& menu, const std::list<std::string>& items) const
{
    if ( menu.empty() || items.empty() )
        return;

    std::list<std::string>::const_iterator jt=menu.begin();
    MenuItem* item = _menuBar->findItem( *jt );
    if (!item)
    {
        item = new MenuItem;
        item->setCommand( *jt );
        Gui::MenuItem* wnd = _menuBar->findItem( "&Windows" );
        if (wnd)
            _menuBar->insertItem(wnd, item);
        else
            _menuBar->appendItem(item);
    }

    // create sub menus
    for ( jt++; jt != menu.end(); jt++ )
    {
        MenuItem* subitem = item->findItem( *jt );
        if ( !subitem )
        {
            subitem = new MenuItem(item);
            subitem->setCommand( *jt );
        }
        item = subitem;
    }

    for (std::list<std::string>::const_iterator it = items.begin(); it != items.end(); ++it)
        *item << *it;
}

void PythonBaseWorkbench::removeMenu(const std::string& menu) const
{
    MenuItem* item = _menuBar->findItem(menu);
    if ( item ) {
        _menuBar->removeItem(item);
        delete item;
    }
}

std::list<std::string> PythonBaseWorkbench::listMenus() const
{
    std::list<std::string> menus;
    QList<MenuItem*> items = _menuBar->getItems();
    for ( QList<MenuItem*>::ConstIterator it = items.begin(); it != items.end(); ++it )
        menus.push_back((*it)->command());
    return menus;
}

void PythonBaseWorkbench::appendContextMenu(const std::list<std::string>& menu, const std::list<std::string>& items) const
{
    MenuItem* item = _contextMenu;
    for (std::list<std::string>::const_iterator jt=menu.begin();jt!=menu.end();++jt) {
        MenuItem* subitem = item->findItem(*jt);
        if (!subitem) {
            subitem = new MenuItem(item);
            subitem->setCommand(*jt);
        }
        item = subitem;
    }

    for (std::list<std::string>::const_iterator it = items.begin(); it != items.end(); ++it)
        *item << *it;
}

void PythonBaseWorkbench::removeContextMenu(const std::string& menu) const
{
    MenuItem* item = _contextMenu->findItem(menu);
    if (item) {
        _contextMenu->removeItem(item);
        delete item;
    }
}

void PythonBaseWorkbench::clearContextMenu()
{
    _contextMenu->clear();
}

void PythonBaseWorkbench::appendToolbar(const std::string& bar, const std::list<std::string>& items) const
{
    ToolBarItem* item = _toolBar->findItem(bar);
    if (!item)
    {
        item = new ToolBarItem(_toolBar);
        item->setCommand(bar);
    }

    for (std::list<std::string>::const_iterator it = items.begin(); it != items.end(); ++it)
        *item << *it;
}

void PythonBaseWorkbench::removeToolbar(const std::string& bar) const
{
    ToolBarItem* item = _toolBar->findItem(bar);
    if (item) {
        _toolBar->removeItem(item);
        delete item;
    }
}

std::list<std::string> PythonBaseWorkbench::listToolbars() const
{
    std::list<std::string> bars;
    QList<ToolBarItem*> items = _toolBar->getItems();
    for (QList<ToolBarItem*>::ConstIterator item = items.begin(); item != items.end(); ++item)
        bars.push_back((*item)->command());
    return bars;
}

void PythonBaseWorkbench::appendCommandbar(const std::string& bar, const std::list<std::string>& items) const
{
    ToolBarItem* item = _commandBar->findItem( bar );
    if ( !item )
    {
        item = new ToolBarItem(_commandBar);
        item->setCommand(bar);
    }

    for (std::list<std::string>::const_iterator it = items.begin(); it != items.end(); ++it)
        *item << *it;
}

void PythonBaseWorkbench::removeCommandbar(const std::string& bar) const
{
    ToolBarItem* item = _commandBar->findItem(bar);
    if ( item ) {
        _commandBar->removeItem(item);
        delete item;
    }
}

std::list<std::string> PythonBaseWorkbench::listCommandbars() const
{
    std::list<std::string> bars;
    QList<ToolBarItem*> items = _commandBar->getItems();
    for (QList<ToolBarItem*>::ConstIterator item = items.begin(); item != items.end(); ++item)
        bars.push_back((*item)->command());
    return bars;
}

// -----------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PythonBlankWorkbench, Gui::PythonBaseWorkbench)

PythonBlankWorkbench::PythonBlankWorkbench()
{
    _menuBar = new MenuItem;
    _contextMenu = new MenuItem;
    _toolBar = new ToolBarItem;
    _commandBar = new ToolBarItem;
}

PythonBlankWorkbench::~PythonBlankWorkbench()
{
}

// -----------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::PythonWorkbench, Gui::PythonBaseWorkbench)

PythonWorkbench::PythonWorkbench()
{
    StdWorkbench wb;
    _menuBar = wb.setupMenuBar();
    _contextMenu = new MenuItem;
    _toolBar = wb.setupToolBars();
    _commandBar = new ToolBarItem;
}

PythonWorkbench::~PythonWorkbench()
{
}

MenuItem* PythonWorkbench::setupMenuBar() const
{
    return _menuBar->copy();
}

ToolBarItem* PythonWorkbench::setupToolBars() const
{
    return _toolBar->copy();
}

ToolBarItem* PythonWorkbench::setupCommandBars() const
{
    return _commandBar->copy();
}

DockWindowItems* PythonWorkbench::setupDockWindows() const
{
    StdWorkbench wb;
    return wb.setupDockWindows();
}

void PythonWorkbench::setupContextMenu(const char* recipient, MenuItem* item) const
{
    StdWorkbench wb;
    wb.setupContextMenu(recipient, item);
    PythonBaseWorkbench::setupContextMenu(recipient, item);
}

void PythonWorkbench::createMainWindowPopupMenu(MenuItem* item) const
{
    StdWorkbench wb;
    wb.createMainWindowPopupMenu(item);
}
