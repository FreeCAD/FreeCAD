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


#ifndef GUI_WORKBENCH_H
#define GUI_WORKBENCH_H

#include <list>
#include <string>
#include <Base/BaseClass.h>
#include <Base/Parameter.h>
#include <Gui/TaskView/TaskWatcher.h>

namespace Base {
class PyObjectBase;
}

namespace Gui {

class MenuItem;
class ToolBarItem;
class DockWindowItems;
class WorkbenchManager;

/**
 * This is the base class for the workbench facility. Each FreeCAD module can provide its own
 * workbench implementation. The workbench defines which GUI elements (such as toolbars, menus,
 * dockable windows, ...) are added to the mainwindow and which gets removed or hidden.
 * When a workbench object gets activated the first time the module - it stands for - gets
 * loaded into RAM.
 * @author Werner Mayer
 */
class GuiExport Workbench : public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /** Constructs a workbench object. */
    Workbench();
    ~Workbench() override;
    /**
     * Returns the name of the workbench object.
     */
    std::string name() const;
    /**
     * Set the name to the workbench object.
     */
    void setName(const std::string&);
    /**
     * The default implementation returns an instance of @ref WorkbenchPy.
     */
    PyObject* getPyObject() override;
    /** Sets up the contextmenu for this workbench.
     */
    void createContextMenu(const char* recipient, MenuItem*) const;
    /** Sets up the contextmenu for the main window for this workbench.
     * The default implementation does nothing.
     */
    virtual void createMainWindowPopupMenu(MenuItem*) const;
    /**
     * Activates the workbench and adds/removes GUI elements.
     */
    bool activate();
    /**
     * Translates the window titles of all menus, toolbars and dock windows.
     */
    void retranslate() const;
    /** Run some actions when the workbench gets activated. */
    virtual void activated();
    /** Run some actions when the workbench gets deactivated. */
    virtual void deactivated();

    /// helper to add TaskWatcher to the TaskView
    void addTaskWatcher(const std::vector<Gui::TaskView::TaskWatcher*> &Watcher);
    /// remove the added TaskWatcher
    void removeTaskWatcher();

    static void createLinkMenu(MenuItem *);

    //// Shows a list of all toolbars
    std::list<std::string> listToolbars() const;
    /// Shows a list of all toolbars and their commands
    std::list<std::pair<std::string, std::list<std::string>>> getToolbarItems() const;
    //// Shows a list of all menus
    std::list<std::string> listMenus() const;
    //// Shows a list of all command bars
    std::list<std::string> listCommandbars() const;
    /// Add a permanent menu item \a cmd after an existing command \a after.
    /// Permanent menu items are always added independent of what the active workbench is.
    /// Adding it will only fail if the item \a after doesn't exist.
    static void addPermanentMenuItem(const std::string& cmd, const std::string& after);
    /// Removes the command \a cmd from the permanent menu items.
    static void removePermanentMenuItem(const std::string& cmd);

protected:
    /** Returns a MenuItem tree structure of menus for this workbench. */
    virtual MenuItem* setupMenuBar() const=0;
    /** Returns a ToolBarItem tree structure of toolbars for this workbench. */
    virtual ToolBarItem* setupToolBars() const=0;
    /** Returns a ToolBarItem tree structure of command bars for this workbench. */
    virtual ToolBarItem* setupCommandBars() const=0;
    /** Returns a DockWindowItems structure of dock windows this workbench. */
    virtual DockWindowItems* setupDockWindows() const=0;
    /** Sets up the contextmenu for this workbench.
     * The default implementation does nothing.
     */
    virtual void setupContextMenu(const char* recipient,MenuItem*) const;
    /** Add permanent menu items to the structure */
    void addPermanentMenuItems(MenuItem*) const;

private:
    /**
     * The method imports the user defined toolbars or toolbox bars and creates
     * a ToolBarItem tree structure.
     */
    void setupCustomToolbars(ToolBarItem* root, const char* toolbar) const;
    void setupCustomToolbars(ToolBarItem* root, const Base::Reference<ParameterGrp> hGrp) const;
    void setupCustomShortcuts() const;

private:
    std::string _name;
    static std::vector<std::pair<std::string, std::string>> staticMenuItems;
};

/**
 * The StdWorkbench class defines the standard menus, toolbars, commandbars etc.
 * To define own workbenches you should inherit from StdWorkbench instead of Workbench
 * to have defined the standard GUI elements.
 * @author Werner Mayer
 */
class GuiExport StdWorkbench : public Workbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    StdWorkbench();
    ~StdWorkbench() override;

public:
    /** Defines the standard context menu. */
    void setupContextMenu(const char* recipient, MenuItem*) const override;
    void createMainWindowPopupMenu(MenuItem*) const override;

protected:
    /** Defines the standard menus. */
    MenuItem* setupMenuBar() const override;
    /** Defines the standard toolbars. */
    ToolBarItem* setupToolBars() const override;
    /** Defines the standard command bars. */
    ToolBarItem* setupCommandBars() const override;
    /** Returns a DockWindowItems structure of dock windows this workbench. */
    DockWindowItems* setupDockWindows() const override;

    friend class PythonWorkbench;
};

/**
 * The BlankWorkbench class defines a complete empty workbench.
 * @author Werner Mayer
 */
class GuiExport BlankWorkbench : public Workbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    BlankWorkbench();
    ~BlankWorkbench() override;

    /** Defines the standard context menu. */
    void setupContextMenu(const char* recipient,MenuItem*) const override;
    /** Run some actions when the workbench gets activated. */
    void activated() override;
    /** Run some actions when the workbench gets deactivated. */
    void deactivated() override;

protected:
    /** Defines the standard menus. */
    MenuItem* setupMenuBar() const override;
    /** Defines the standard toolbars. */
    ToolBarItem* setupToolBars() const override;
    /** Defines the standard command bars. */
    ToolBarItem* setupCommandBars() const override;
    /** Returns a DockWindowItems structure of dock windows this workbench. */
    DockWindowItems* setupDockWindows() const override;
};

/**
 * The NoneWorkbench class defines a slim workbench.
 * @author Werner Mayer
 */
class GuiExport NoneWorkbench : public StdWorkbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    NoneWorkbench();
    ~NoneWorkbench() override;

    /** Defines the standard context menu. */
    void setupContextMenu(const char* recipient,MenuItem*) const override;

protected:
    /** Defines the standard menus. */
    MenuItem* setupMenuBar() const override;
    /** Defines the standard toolbars. */
    ToolBarItem* setupToolBars() const override;
    /** Defines the standard command bars. */
    ToolBarItem* setupCommandBars() const override;
    /** Returns a DockWindowItems structure of dock windows this workbench. */
    DockWindowItems* setupDockWindows() const override;
};

class GuiExport TestWorkbench : public StdWorkbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    TestWorkbench();
    ~TestWorkbench() override;

protected:
    MenuItem* setupMenuBar() const override;
    ToolBarItem* setupToolBars() const override;
    ToolBarItem* setupCommandBars() const override;
};

/**
 * The PythonBaseWorkbench class allows the manipulation of the workbench from Python.
 * Therefore PythonWorkbenchPy provides the required Python interface.
 * @author Werner Mayer
 */
class GuiExport PythonBaseWorkbench : public Workbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PythonBaseWorkbench();
    ~PythonBaseWorkbench() override;
    /**
     * Creates and returns immediately the corresponding Python workbench object.
     */
    PyObject* getPyObject() override;

    /** @name Manipulation methods */
    //@{
    /// Appends a new menu
    void appendMenu(const std::list<std::string>& menu, const std::list<std::string>& items) const;
    /// Removes a menu
    void removeMenu(const std::string& menu ) const;

    /// Appends new context menu items
    void appendContextMenu(const std::list<std::string>& menu, const std::list<std::string>& items) const;
    /// Removes a context menu
    void removeContextMenu(const std::string& menu ) const;
    void setupContextMenu(const char* recipient,MenuItem*) const override;
    void clearContextMenu();

    /// Appends a new toolbar
    void appendToolbar(const std::string& bar, const std::list<std::string>& items) const;
    /// Removes a toolbar
    void removeToolbar(const std::string& bar) const;

    /// Appends a new command bar
    void appendCommandbar(const std::string& bar, const std::list<std::string>& items) const;
    /// Removes a command bar
    void removeCommandbar(const std::string& bar) const;
    //@}

protected:
    MenuItem* setupMenuBar() const override;
    ToolBarItem* setupToolBars() const override;
    ToolBarItem* setupCommandBars() const override;
    DockWindowItems* setupDockWindows() const override;

protected:
    MenuItem* _menuBar{nullptr};
    MenuItem* _contextMenu{nullptr};
    ToolBarItem* _toolBar{nullptr};
    ToolBarItem* _commandBar{nullptr};
    Base::PyObjectBase* _workbenchPy{nullptr};
};

class GuiExport PythonBlankWorkbench : public PythonBaseWorkbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PythonBlankWorkbench();
    ~PythonBlankWorkbench() override;
};

/**
 * The PythonWorkbench class allows the manipulation of the workbench from Python.
 * Therefore PythonWorkbenchPy provides the required Python interface.
 * @author Werner Mayer
 */
class GuiExport PythonWorkbench : public PythonBaseWorkbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PythonWorkbench();
    ~PythonWorkbench() override;

    /** Defines the standard context menu. */
    void setupContextMenu(const char* recipient, MenuItem*) const override;
    void createMainWindowPopupMenu(MenuItem*) const override;

protected:
    MenuItem* setupMenuBar() const override;
    ToolBarItem* setupToolBars() const override;
    ToolBarItem* setupCommandBars() const override;
    DockWindowItems* setupDockWindows() const override;
};

} // namespace Gui


#endif // GUI_WORKBENCH_H
