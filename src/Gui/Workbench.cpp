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
#include "WorkbenchManipulator.h"
#include "WorkbenchPy.h"
#include "Action.h"
#include "Application.h"
#include "Command.h"
#include "Control.h"
#include "DockWindowManager.h"
#include "MainWindow.h"
#include "MenuManager.h"
#include "PythonWorkbenchPy.h"
#include "Selection.h"
#include "ToolBarManager.h"
#include "ToolBoxManager.h"
#include "UserSettings.h"
#include "Window.h"

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <Base/Interpreter.h>
#include <Gui/ComboView.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskWatcher.h>

using namespace Gui;

/** \defgroup workbench Workbench Framework
    \ingroup GUI

    FreeCAD provides the possibility to have one or more workbenches for a module.
    A workbench changes the appearance of the main window in that way that it defines toolbars, items in the toolbox, menus or the context menu and dockable windows that are shown to the user.
    The idea behind this concept is that the user should see only the functions that are required for the task that they are doing at this moment and not to show dozens of unneeded functions which the user never uses.

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
 *
 * At startup FreeCAD scans all module directories and invokes InitGui.py. So an item for a workbench gets created. If the user
 * clicks on such an item the matching module gets loaded, the C++ workbench gets registered and activated.
 *
 * The user is able to modify a workbench (Edit|Customize). E.g. they can add new toolbars or items for the toolbox and add their preferred
 * functions to them. But the user only has full control over "their" own toolbars, the default workbench items cannot be modified or even removed.
 *
 * FreeCAD provides also the possibility to define pure Python workbenches. Such workbenches are temporarily only and are lost after exiting
 * the FreeCAD session. But if you want to keep your Python workbench you can write a macro and attach it with a user defined button or just
 * perform the macro during the next FreeCAD session.
 * Here follows a short example of how to create and embed a workbench in Python
 * \code
 * w=Workbench()                                              # creates a standard workbench (the same as StdWorkbench in C++)
 * w.MenuText = "My Workbench"                                # the text that will appear in the combo box
 * dir(w)                                                     # lists all available function of the object
 * FreeCADGui.addWorkbench(w)                                 # Creates an item for our workbench now
 *                                                            # Note: We must first add the workbench to run some initialization code
 *                                                            # Then we are ready to customize the workbench
 * list = ["Std_Test1", "Std_Test2", "Std_Test3"]             # creates a list of new functions
 * w.appendMenu("Test functions", list)                       # creates a new menu with these functions
 * w.appendToolbar("Test", list)                              # ... and also a new toolbar
 * \endcode
 */

/// @namespace Gui @class Workbench
TYPESYSTEM_SOURCE_ABSTRACT(Gui::Workbench, Base::BaseClass)

Workbench::Workbench() = default;

Workbench::~Workbench() = default;

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
    const auto workbenchGroup {
        App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Workbench")
    };
    // workbench specific custom toolbars
    if (workbenchGroup->HasGroup(name.c_str())) {
        const auto customGroup = workbenchGroup->GetGroup(name.c_str());
        if (customGroup->HasGroup(toolbar)) {
            const auto customToolbarGroup = customGroup->GetGroup(toolbar);
            setupCustomToolbars(root, customToolbarGroup);
        }
    }

    // for this workbench global toolbars are not allowed
    if (getTypeId() == NoneWorkbench::getClassTypeId()) {
        return;
    }

    // application-wide custom toolbars
    if (workbenchGroup->HasGroup("Global")) {
        const auto globalGroup = workbenchGroup->GetGroup("Global");
        if (globalGroup->HasGroup(toolbar)) {
            const auto customToolbarGroup = globalGroup->GetGroup(toolbar);
            setupCustomToolbars(root, customToolbarGroup);
        }
    }
}

void Workbench::setupCustomToolbars(ToolBarItem* root, const Base::Reference<ParameterGrp> hGrp) const
{
    std::vector<Base::Reference<ParameterGrp> > hGrps = hGrp->GetGroups();
    CommandManager& rMgr = Application::Instance->commandManager();
    std::string separator = "Separator";
    for (const auto & it : hGrps) {
        bool active = it->GetBool("Active", true);
        if (!active) {
            // ignore this toolbar
            continue;
        }

        auto bar = new ToolBarItem(root);
        bar->setCommand("Custom");

        // get the elements of the subgroups
        std::vector<std::pair<std::string,std::string> > items = hGrp->GetGroup(it->GetGroupName())->GetASCIIMap();
        for (const auto & item : items) {
            if (item.first.substr(0, separator.size()) == separator) {
                *bar << "Separator";
            }
            else if (item.first == "Name") {
                bar->setCommand(item.second);
            }
            else {
                Command* pCmd = rMgr.getCommandByName(item.first.c_str());
                if (!pCmd) { // unknown command
                    // first try the module name as is
                    std::string pyMod = item.second;
                    try {
                        Base::Interpreter().loadModule(pyMod.c_str());
                        // Try again
                        pCmd = rMgr.getCommandByName(item.first.c_str());
                    }
                    catch(const Base::Exception&) {
                    }
                }

                // still not there?
                if (!pCmd) {
                    // add the 'Gui' suffix
                    std::string pyMod = item.second + "Gui";
                    try {
                        Base::Interpreter().loadModule(pyMod.c_str());
                        // Try again
                        pCmd = rMgr.getCommandByName(item.first.c_str());
                    }
                    catch(const Base::Exception&) {
                    }
                }

                if (pCmd) {
                    *bar << item.first; // command name
                }
            }
        }
    }
}

void Workbench::setupCustomShortcuts() const
{
    // Now managed by ShortcutManager
}

void Workbench::createContextMenu(const char* recipient, MenuItem* item) const
{
    setupContextMenu(recipient, item);
    WorkbenchManipulator::changeContextMenu(recipient, item);
}

void Workbench::setupContextMenu(const char* recipient,MenuItem* item) const
{
    Q_UNUSED(recipient);
    Q_UNUSED(item);
}

void Workbench::createMainWindowPopupMenu(MenuItem*) const
{
}

void Workbench::createLinkMenu(MenuItem *item) {
    if(!item || !App::GetApplication().getActiveDocument()) {
        return;
    }

    auto linkMenu = new MenuItem;
    linkMenu->setCommand("Link actions");
    *linkMenu << "Std_LinkMakeGroup" << "Std_LinkMake";

    auto &rMgr = Application::Instance->commandManager();
    const char *cmds[] = {"Std_LinkMakeRelative",nullptr,"Std_LinkUnlink","Std_LinkReplace",
        "Std_LinkImport","Std_LinkImportAll",nullptr,"Std_LinkSelectLinked",
        "Std_LinkSelectLinkedFinal","Std_LinkSelectAllLinks"};
    bool separator = true;
    for(const auto & it : cmds) {
        if(!it) {
            if(separator) {
                separator = false;
                *linkMenu << "Separator";
            }
            continue;
        }
        auto cmd = rMgr.getCommandByName(it);
        if(cmd->isActive()) {
            separator = true;
            *linkMenu << it;
        }
    }
    *item << linkMenu;
}

std::vector<std::pair<std::string, std::string>> Workbench::staticMenuItems;

void Workbench::addPermanentMenuItem(const std::string& cmd, const std::string& after)
{
    staticMenuItems.emplace_back(cmd, after);
}

void Workbench::removePermanentMenuItem(const std::string& cmd)
{
    auto it = std::find_if(staticMenuItems.begin(), staticMenuItems.end(), [cmd](const std::pair<std::string, std::string>& pmi) {
        return (pmi.first == cmd);
    });

    if (it != staticMenuItems.end()) {
        staticMenuItems.erase(it);
    }
}

void  Workbench::addPermanentMenuItems(MenuItem* mb) const
{
    for (const auto& it : staticMenuItems) {
        MenuItem* par = mb->findParentOf(it.second);
        if (par) {
            Gui::MenuItem* item = par->findItem(it.second);
            item = par->afterItem(item);

            auto add = new Gui::MenuItem();
            add->setCommand(it.first);
            par->insertItem(item, add);
        }
    }
}

void Workbench::activated()
{
    Application::Instance->commandManager().signalPyCmdInitialized();
}

void Workbench::deactivated()
{
}

bool Workbench::activate()
{
    ToolBarItem* tb = setupToolBars();
    setupCustomToolbars(tb, "Toolbar");
    WorkbenchManipulator::changeToolBars(tb);
    ToolBarManager::getInstance()->setup( tb );
    delete tb;

    //ToolBarItem* cb = setupCommandBars();
    //setupCustomToolbars(cb, "Toolboxbar");
    //ToolBoxManager::getInstance()->setup( cb );
    //delete cb;

    DockWindowItems* dw = setupDockWindows();
    WorkbenchManipulator::changeDockWindows(dw);
    DockWindowManager::instance()->setup( dw );
    delete dw;

    MenuItem* mb = setupMenuBar();
    addPermanentMenuItems(mb);
    WorkbenchManipulator::changeMenuBar(mb);
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
    if (taskView) {
        taskView->addTaskWatcher(Watcher);
    }
}

void Workbench::removeTaskWatcher()
{
    Gui::TaskView::TaskView* taskView = Control().taskPanel();
    if (taskView) {
        taskView->clearTaskWatcher();
    }
}

std::list<std::string> Workbench::listToolbars() const
{
    std::unique_ptr<ToolBarItem> tb(setupToolBars());
    std::list<std::string> bars;
    QList<ToolBarItem*> items = tb->getItems();
    for (const auto & item : items) {
        bars.push_back(item->command());
    }
    return bars;
}

std::list<std::pair<std::string, std::list<std::string>>> Workbench::getToolbarItems() const
{
    std::unique_ptr<ToolBarItem> tb(setupToolBars());

    std::list<std::pair<std::string, std::list<std::string>>> itemsList;
    QList<ToolBarItem*> items = tb->getItems();
    for (const auto & item : items) {
        QList<ToolBarItem*> sub = item->getItems();
        std::list<std::string> cmds;
        for (const auto & jt : sub) {
            cmds.push_back(jt->command());
        }

        itemsList.emplace_back(item->command(), cmds);
    }
    return itemsList;
}

std::list<std::string> Workbench::listMenus() const
{
    std::unique_ptr<MenuItem> mb(setupMenuBar());
    std::list<std::string> menus;
    QList<MenuItem*> items = mb->getItems();
    for (const auto & item : items) {
        menus.push_back(item->command());
    }
    return menus;
}

std::list<std::string> Workbench::listCommandbars() const
{
    std::unique_ptr<ToolBarItem> cb(setupCommandBars());
    std::list<std::string> bars;
    QList<ToolBarItem*> items = cb->getItems();
    for (const auto & item : items) {
        bars.push_back(item->command());
    }
    return bars;
}

// --------------------------------------------------------------------

#if 0 // needed for Qt's lupdate utility
    qApp->translate("CommandGroup", "File");
    qApp->translate("CommandGroup", "Edit");
    qApp->translate("CommandGroup", "Help");
    qApp->translate("CommandGroup", "Link");
    qApp->translate("CommandGroup", "Tools");
    qApp->translate("CommandGroup", "View");
    qApp->translate("CommandGroup", "Window");
    qApp->translate("CommandGroup", "Standard");
    qApp->translate("CommandGroup", "Macros");
    qApp->translate("CommandGroup", "Macro");
    qApp->translate("CommandGroup", "Structure");
    qApp->translate("CommandGroup", "Standard-Test");
    qApp->translate("CommandGroup", "Standard-View");
    qApp->translate("CommandGroup", "TreeView");
    qApp->translate("CommandGroup", "Measure");

    qApp->translate("Workbench", "&File");
    qApp->translate("Workbench", "&Edit");
    qApp->translate("Workbench", "Edit");
    qApp->translate("Workbench", "Clipboard");
    qApp->translate("Workbench", "Workbench");
    qApp->translate("Workbench", "Structure");
    qApp->translate("Workbench", "Standard views");
    qApp->translate("Workbench", "Axonometric");
    qApp->translate("Workbench", "&Stereo");
    qApp->translate("Workbench", "&Zoom");
    qApp->translate("Workbench", "Visibility");
    qApp->translate("Workbench", "&View");
    qApp->translate("Workbench", "&Tools");
    qApp->translate("Workbench", "&Macro");
    qApp->translate("Workbench", "&Windows");
    qApp->translate("Workbench", "&On-line help");
    qApp->translate("Workbench", "&Help");
    qApp->translate("Workbench", "Help");
    qApp->translate("Workbench", "File");
    qApp->translate("Workbench", "Macro");
    qApp->translate("Workbench", "View");
    qApp->translate("Workbench", "Special Ops");
    // needed for Structure toolbar
    qApp->translate("Workbench", "Link actions");
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

StdWorkbench::~StdWorkbench() = default;

void StdWorkbench::setupContextMenu(const char* recipient, MenuItem* item) const
{
    if (strcmp(recipient,"View") == 0)
    {
        createLinkMenu(item);
        *item << "Separator";

        auto StdViews = new MenuItem;
        StdViews->setCommand( "Standard views" );

        *StdViews << "Std_ViewIsometric" << "Separator" << "Std_ViewHome" << "Std_ViewFront" << "Std_ViewTop" << "Std_ViewRight"
                  << "Std_ViewRear" << "Std_ViewBottom" << "Std_ViewLeft"
                  << "Separator" << "Std_ViewRotateLeft" << "Std_ViewRotateRight";

        auto measure = new MenuItem();
        measure->setCommand("Measure");
        *measure << "View_Measure_Toggle_All" << "View_Measure_Clear_All";


        *item << "Std_ViewFitAll" << "Std_ViewFitSelection" << "Std_DrawStyle" 
              << StdViews << measure << "Separator"
              << "Std_ViewDockUndockFullscreen";

        if (Gui::Selection().countObjectsOfType(App::DocumentObject::getClassTypeId()) > 0) {
            *item << "Separator" << "Std_SetAppearance" << "Std_ToggleVisibility"
                  << "Std_ToggleSelectability" << "Std_TreeSelection"
                  << "Std_RandomColor" << "Separator" << "Std_Delete"
                  << "Std_SendToPythonConsole" << "Std_TransformManip";
        }
    }
    else if (strcmp(recipient,"Tree") == 0)
    {
        if (Gui::Selection().countObjectsOfType(App::DocumentObject::getClassTypeId()) > 0) {
            *item << "Std_ToggleVisibility" << "Std_ShowSelection" << "Std_HideSelection"
                  << "Std_ToggleSelectability" << "Std_TreeSelectAllInstances" << "Separator"
                  << "Std_SetAppearance" << "Std_RandomColor" << "Separator"
                  << "Std_Cut" << "Std_Copy" << "Std_Paste" << "Std_Delete"
                  << "Std_SendToPythonConsole" << "Separator";
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
    auto menuBar = new MenuItem;

    // File
    auto file = new MenuItem( menuBar );
    file->setCommand("&File");
    *file << "Std_New" << "Std_Open" << "Std_RecentFiles" << "Separator" << "Std_CloseActiveWindow"
          << "Std_CloseAllWindows" << "Separator" << "Std_Save" << "Std_SaveAs"
          << "Std_SaveCopy" << "Std_SaveAll" << "Std_Revert" << "Separator" << "Std_Import"
          << "Std_Export" << "Std_MergeProjects" << "Std_ProjectInfo"
          << "Separator" << "Std_Print" << "Std_PrintPreview" << "Std_PrintPdf"
          << "Separator" << "Std_Quit";

    // Edit
    auto edit = new MenuItem( menuBar );
    edit->setCommand("&Edit");
    *edit << "Std_Undo" << "Std_Redo" << "Separator" << "Std_Cut" << "Std_Copy"
          << "Std_Paste" << "Std_DuplicateSelection" << "Separator"
          << "Std_Refresh" << "Std_BoxSelection" << "Std_BoxElementSelection"
          << "Std_SelectAll" << "Std_Delete" << "Std_SendToPythonConsole"
          << "Separator" << "Std_Placement" << "Std_TransformManip" << "Std_Alignment"
          << "Std_Edit" << "Separator" << "Std_UserEditMode" << "Separator" << "Std_DlgPreferences";

    auto axoviews = new MenuItem;
    axoviews->setCommand("Axonometric");
    *axoviews << "Std_ViewIsometric"
              << "Std_ViewDimetric"
              << "Std_ViewTrimetric";

    // Standard views
    auto stdviews = new MenuItem;
    stdviews->setCommand("Standard views");
    *stdviews << "Std_ViewFitAll" << "Std_ViewFitSelection" << axoviews
              << "Separator" << "Std_ViewHome" << "Std_ViewFront" << "Std_ViewTop"
              << "Std_ViewRight" << "Separator" << "Std_ViewRear"
              << "Std_ViewBottom" << "Std_ViewLeft"
              << "Separator" << "Std_ViewRotateLeft" << "Std_ViewRotateRight"
              << "Separator" << "Std_StoreWorkingView" << "Std_RecallWorkingView";

    // stereo
    auto view3d = new MenuItem;
    view3d->setCommand("&Stereo");
    *view3d << "Std_ViewIvStereoRedGreen" << "Std_ViewIvStereoQuadBuff"
            << "Std_ViewIvStereoInterleavedRows" << "Std_ViewIvStereoInterleavedColumns"
            << "Std_ViewIvStereoOff" << "Separator" << "Std_ViewIvIssueCamPos";

    // zoom
    auto zoom = new MenuItem;
    zoom->setCommand("&Zoom");
    *zoom << "Std_ViewZoomIn" << "Std_ViewZoomOut" << "Separator" << "Std_ViewBoxZoom";

    // Visibility
    auto visu = new MenuItem;
    visu->setCommand("Visibility");
    *visu << "Std_ToggleVisibility" << "Std_ShowSelection" << "Std_HideSelection"
          << "Std_SelectVisibleObjects"
          << "Separator" << "Std_ToggleObjects" << "Std_ShowObjects" << "Std_HideObjects"
          << "Separator" << "Std_ToggleSelectability"
          << "Separator" << "View_Measure_Toggle_All" << "View_Measure_Clear_All";

    // View
    auto view = new MenuItem( menuBar );
    view->setCommand("&View");
    *view << "Std_ViewCreate" << "Std_OrthographicCamera" << "Std_PerspectiveCamera" << "Std_MainFullscreen" << "Separator"
          << stdviews << "Std_FreezeViews" << "Std_DrawStyle" << "Std_SelBoundingBox"
          << "Separator" << view3d << zoom
          << "Std_ViewDockUndockFullscreen" << "Std_AxisCross" << "Std_ToggleClipPlane"
          << "Std_TextureMapping"
#ifdef BUILD_VR
          << "Std_ViewVR"
#endif
          << "Separator" << visu
          << "Std_ToggleNavigation"
          << "Std_SetAppearance" << "Std_RandomColor" << "Separator"
          << "Std_Workbench" << "Std_ToolBarMenu" << "Std_DockViewMenu" << "Separator"
          << "Std_LinkSelectActions"
          << "Std_TreeViewActions"
          << "Std_ViewStatusBar";

    // Tools
    auto tool = new MenuItem( menuBar );
    tool->setCommand("&Tools");
    *tool << "Std_DlgParameter"
          << "Separator"
          << "Std_ViewScreenShot"
          << "Std_ViewLoadImage"
          << "Std_SceneInspector"
          << "Std_DependencyGraph"
          << "Std_ExportDependencyGraph"
          << "Std_ProjectUtil"
          << "Separator"
          << "Std_MeasureDistance"
          << "Separator"
          << "Std_TextDocument"
          << "Separator"
          << "Std_DemoMode"
          << "Std_UnitsCalculator"
          << "Separator"
          << "Std_DlgCustomize";
#ifdef BUILD_ADDONMGR
    *tool << "Std_AddonMgr";
#endif

    // Macro
    auto macro = new MenuItem( menuBar );
    macro->setCommand("&Macro");
    *macro << "Std_DlgMacroRecord"
           << "Std_DlgMacroExecute"
           << "Std_RecentMacros"
           << "Separator"
           << "Std_DlgMacroExecuteDirect"
           << "Std_MacroAttachDebugger"
           << "Std_MacroStartDebug"
           << "Std_MacroStopDebug"
           << "Std_MacroStepOver"
           << "Std_MacroStepInto"
           << "Std_ToggleBreakpoint";

    // Windows
    auto wnd = new MenuItem( menuBar );
    wnd->setCommand("&Windows");
    *wnd << "Std_ActivateNextWindow" << "Std_ActivatePrevWindow" << "Separator"
         << "Std_TileWindows" << "Std_CascadeWindows" << "Separator"
         << "Std_WindowsMenu" << "Std_Windows";

    // Separator
    auto sep = new MenuItem( menuBar );
    sep->setCommand( "Separator" );

    // Help
    auto help = new MenuItem( menuBar );
    help->setCommand("&Help");
    *help << "Std_OnlineHelp" << "Std_FreeCADWebsite" << "Std_FreeCADDonation"
          << "Std_FreeCADUserHub" << "Std_FreeCADPowerUserHub"
          << "Std_PythonHelp" << "Std_FreeCADForum" << "Std_FreeCADFAQ"
          << "Std_ReportBug" << "Std_About" << "Std_WhatsThis";

    return menuBar;
}

ToolBarItem* StdWorkbench::setupToolBars() const
{
    auto root = new ToolBarItem;

    // File
    auto file = new ToolBarItem( root );
    file->setCommand("File");
    *file << "Std_New" << "Std_Open" << "Std_Save";

    // Edit
    auto edit = new ToolBarItem( root );
    edit->setCommand("Edit");
    *edit << "Std_Undo" << "Std_Redo"
          << "Separator" << "Std_Refresh";
    
    // Clipboard
    auto clipboard = new ToolBarItem( root , ToolBarItem::DefaultVisibility::Hidden );
    clipboard->setCommand("Clipboard");
    *clipboard << "Std_Cut" << "Std_Copy" << "Std_Paste";
    
    // Workbench switcher
    if (WorkbenchSwitcher::isToolbar(WorkbenchSwitcher::getValue())) {
        auto wb = new ToolBarItem(root);
        wb->setCommand("Workbench");
        *wb << "Std_Workbench";
    }

    // Macro
    auto macro = new ToolBarItem( root );
    macro->setCommand("Macro");
    *macro << "Std_DlgMacroRecord" << "Std_DlgMacroExecute"
           << "Std_DlgMacroExecuteDirect";

    // View
    auto view = new ToolBarItem( root );
    view->setCommand("View");
    *view << "Std_ViewFitAll" << "Std_ViewFitSelection" << "Std_ViewIsometric"
          << "Std_ViewFront"<< "Std_ViewTop" << "Std_ViewRight"
          << "Std_ViewRear" << "Std_ViewBottom"<< "Std_ViewLeft"
          << "Separator" << "Std_DrawStyle" << "Std_TreeViewActions"
          << "Separator" << "Std_MeasureDistance";

    // Structure
    auto structure = new ToolBarItem( root );
    structure->setCommand("Structure");
    *structure << "Std_Part" << "Std_Group" << "Std_LinkActions";

    // Help
    auto help = new ToolBarItem( root );
    help->setCommand("Help");
    *help << "Std_WhatsThis";
    
    return root;
}

ToolBarItem* StdWorkbench::setupCommandBars() const
{
    auto root = new ToolBarItem;

    // View
    auto view = new ToolBarItem( root );
    view->setCommand("Standard views");
    *view << "Std_ViewFitAll" << "Std_ViewFitSelection" << "Std_ViewIsometric" << "Separator"
          << "Std_ViewFront" << "Std_ViewRight" << "Std_ViewTop" << "Separator"
          << "Std_ViewRear" << "Std_ViewLeft" << "Std_ViewBottom";

    // Special Ops
    auto macro = new ToolBarItem( root );
    macro->setCommand("Special Ops");
    *macro << "Std_DlgParameter" << "Std_DlgPreferences" << "Std_DlgMacroRecord"
           << "Std_DlgMacroExecute" << "Std_DlgCustomize";

    return root;
}

DockWindowItems* StdWorkbench::setupDockWindows() const
{
    auto root = new DockWindowItems();
    root->addDockWidget("Std_ToolBox", Qt::RightDockWidgetArea, false, false);
    //root->addDockWidget("Std_HelpView", Qt::RightDockWidgetArea, true, false);
    root->addDockWidget("Std_TreeView", Qt::LeftDockWidgetArea, true, false);
    root->addDockWidget("Std_PropertyView", Qt::LeftDockWidgetArea, true, false);
    root->addDockWidget("Std_SelectionView", Qt::LeftDockWidgetArea, false, false);
    root->addDockWidget("Std_ComboView", Qt::LeftDockWidgetArea, true, true);
    root->addDockWidget("Std_TaskView", Qt::LeftDockWidgetArea, true, true);
    root->addDockWidget("Std_ReportView", Qt::BottomDockWidgetArea, true, true);
    root->addDockWidget("Std_PythonView", Qt::BottomDockWidgetArea, true, true);

    //Dagview through parameter.
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
          GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("DockWindows")->GetGroup("DAGView");

    bool enabled = group->GetBool("Enabled", false);
    if (enabled) {
      root->addDockWidget("Std_DAGView", Qt::RightDockWidgetArea, false, false);
    }

    return root;
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::BlankWorkbench, Gui::Workbench)

BlankWorkbench::BlankWorkbench()
  : Workbench()
{
}

BlankWorkbench::~BlankWorkbench() = default;

void BlankWorkbench::activated()
{
    QList<QDockWidget*> dw = getMainWindow()->findChildren<QDockWidget*>();
    for (auto & it : dw) {
        it->toggleViewAction()->setVisible(false);
    }
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

NoneWorkbench::~NoneWorkbench() = default;

void NoneWorkbench::setupContextMenu(const char* recipient,MenuItem* item) const
{
    Q_UNUSED(recipient);
    Q_UNUSED(item);
}

MenuItem* NoneWorkbench::setupMenuBar() const
{
    // Setup the default menu bar
    auto menuBar = new MenuItem;

    // File
    auto file = new MenuItem( menuBar );
    file->setCommand("&File");
    *file << "Std_Quit";

    // Edit
    auto edit = new MenuItem( menuBar );
    edit->setCommand("&Edit");
    *edit << "Std_DlgPreferences";

    // View
    auto view = new MenuItem( menuBar );
    view->setCommand("&View");
    *view << "Std_Workbench";

    // Separator
    auto sep = new MenuItem( menuBar );
    sep->setCommand("Separator");

    // Help
    auto help = new MenuItem( menuBar );
    help->setCommand("&Help");
    *help << "Std_OnlineHelp" << "Std_About";

    return menuBar;
}

ToolBarItem* NoneWorkbench::setupToolBars() const
{
    auto root = new ToolBarItem;
    return root;
}

ToolBarItem* NoneWorkbench::setupCommandBars() const
{
    auto root = new ToolBarItem;
    return root;
}

DockWindowItems* NoneWorkbench::setupDockWindows() const
{
    auto root = new DockWindowItems();
    root->addDockWidget("Std_ReportView", Qt::BottomDockWidgetArea, true, false);
    return root;
}

// --------------------------------------------------------------------

TYPESYSTEM_SOURCE(Gui::TestWorkbench, Gui::Workbench)

TestWorkbench::TestWorkbench()
  : StdWorkbench()
{
}

TestWorkbench::~TestWorkbench() = default;

MenuItem* TestWorkbench::setupMenuBar() const
{
    // Setup the default menu bar
    MenuItem* menuBar = StdWorkbench::setupMenuBar();

    MenuItem* item = menuBar->findItem("&Help");
    item->removeItem(item->findItem("Std_WhatsThis"));

    // Test commands
    auto test = new MenuItem;
    menuBar->insertItem( item, test );
    test->setCommand( "Test &Commands" );
    *test << "Std_Test1" << "Std_Test2" << "Std_Test3" << "Std_Test4" << "Std_Test5"
          << "Std_Test6" << "Std_Test7" << "Std_Test8";

    // Inventor View
    auto opiv = new MenuItem;
    menuBar->insertItem( item, opiv );
    opiv->setCommand("&Inventor View");
    *opiv << "Std_ViewExample1" << "Std_ViewExample2" << "Std_ViewExample3";

    return menuBar;
}

ToolBarItem* TestWorkbench::setupToolBars() const
{
    return nullptr;
}

ToolBarItem* TestWorkbench::setupCommandBars() const
{
    return nullptr;
}

// -----------------------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Gui::PythonBaseWorkbench, Gui::Workbench)

PythonBaseWorkbench::PythonBaseWorkbench() = default;

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
    for (const auto & it : items) {
        item->appendItem(it->copy());
    }
}

void PythonBaseWorkbench::appendMenu(const std::list<std::string>& menu, const std::list<std::string>& items) const
{
    if ( menu.empty() || items.empty() ) {
        return;
    }

    auto jt=menu.begin();
    MenuItem* item = _menuBar->findItem( *jt );
    if (!item) {
        item = new MenuItem;
        item->setCommand( *jt );
        Gui::MenuItem* wnd = _menuBar->findItem( "&Windows" );
        if (wnd) {
            _menuBar->insertItem(wnd, item);
        }
        else {
            _menuBar->appendItem(item);
        }
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

    for (const auto & it : items) {
        *item << it;
    }
}

void PythonBaseWorkbench::removeMenu(const std::string& menu) const
{
    MenuItem* item = _menuBar->findItem(menu);
    if ( item ) {
        _menuBar->removeItem(item);
        delete item;
    }
}

void PythonBaseWorkbench::appendContextMenu(const std::list<std::string>& menu, const std::list<std::string>& items) const
{
    MenuItem* item = _contextMenu;
    for (const auto & jt : menu) {
        MenuItem* subitem = item->findItem(jt);
        if (!subitem) {
            subitem = new MenuItem(item);
            subitem->setCommand(jt);
        }
        item = subitem;
    }

    for (const auto & it : items) {
        *item << it;
    }
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
    if (!item) {
        item = new ToolBarItem(_toolBar);
        item->setCommand(bar);
    }

    for (const auto & it : items) {
        *item << it;
    }
}

void PythonBaseWorkbench::removeToolbar(const std::string& bar) const
{
    ToolBarItem* item = _toolBar->findItem(bar);
    if (item) {
        _toolBar->removeItem(item);
        delete item;
    }
}

void PythonBaseWorkbench::appendCommandbar(const std::string& bar, const std::list<std::string>& items) const
{
    ToolBarItem* item = _commandBar->findItem( bar );
    if (!item) {
        item = new ToolBarItem(_commandBar);
        item->setCommand(bar);
    }

    for (const auto & it : items) {
        *item << it;
    }
}

void PythonBaseWorkbench::removeCommandbar(const std::string& bar) const
{
    ToolBarItem* item = _commandBar->findItem(bar);
    if (item) {
        _commandBar->removeItem(item);
        delete item;
    }
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

PythonBlankWorkbench::~PythonBlankWorkbench() = default;

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

PythonWorkbench::~PythonWorkbench() = default;

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
