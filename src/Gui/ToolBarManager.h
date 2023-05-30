/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_TOOLBARMANAGER_H
#define GUI_TOOLBARMANAGER_H

#include <string>
#include <QStringList>
#include <FCGlobal.h>

class QAction;
class QToolBar;

namespace Gui {

class GuiExport ToolBarItem
{
public:
    /** Manages the default visibility status of a toolbar item, as well as the default status
     * of the toggleViewAction usable by the contextual menu to enable and disable its visibility
    */
    enum class DefaultVisibility {
        Visible,     // toolbar is hidden by default, visibility toggle action is enabled
        Hidden,      // toolbar hidden by default, visibility toggle action is enabled
        Unavailable, // toolbar visibility is managed independently by client code and defaults to
                     // hidden, visibility toggle action is disabled by default (it is unavailable
                     // to the UI). Upon being forced to be available, these toolbars default to
                     // visible.
    };

    ToolBarItem();
    explicit ToolBarItem(ToolBarItem* item, DefaultVisibility visibilityPolicy = DefaultVisibility::Visible);
    ~ToolBarItem();

    void setCommand(const std::string&);
    const std::string &command() const;

    bool hasItems() const;
    ToolBarItem* findItem(const std::string&);
    ToolBarItem* copy() const;
    uint count() const;

    void appendItem(ToolBarItem* item);
    bool insertItem(ToolBarItem*, ToolBarItem* item);
    void removeItem(ToolBarItem* item);
    void clear();

    ToolBarItem& operator << (ToolBarItem* item);
    ToolBarItem& operator << (const std::string& command);
    QList<ToolBarItem*> getItems() const;

    DefaultVisibility visibilityPolicy;

private:
    std::string _name;
    QList<ToolBarItem*> _items;
};

/**
 * The ToolBarManager class is responsible for the creation of toolbars and appending them
 * to the main window.
 * @see ToolBoxManager
 * @see MenuManager
 * @author Werner Mayer
 */
class GuiExport ToolBarManager
{
public:

    enum class State {
        ForceHidden,    // Forces a toolbar to hide and hides the toggle action
        ForceAvailable, // Forces a toolbar toggle action to show, visibility depends on user config
        RestoreDefault, // Restores a toolbar toggle action default, visibility as user config
        SaveState,      // Saves the state of the toolbars
    };

    /// The one and only instance.
    static ToolBarManager* getInstance();
    static void destruct();
    /** Sets up the toolbars of a given workbench. */
    void setup(ToolBarItem*);
    void saveState() const;
    void restoreState() const;
    void retranslate() const;

    void setMovable(bool movable) const;

    void setState(const QList<QString>& names, State state);
    void setState (const QString& name, State state);

protected:
    void setup(ToolBarItem*, QToolBar*) const;

    ToolBarItem::DefaultVisibility getToolbarPolicy(const QToolBar *) const;

    /** Returns a list of all currently existing toolbars. */
    QList<QToolBar*> toolBars() const;
    QToolBar* findToolBar(const QList<QToolBar*>&, const QString&) const;
    QAction* findAction(const QList<QAction*>&, const QString&) const;
    ToolBarManager();
    ~ToolBarManager();

private:
    QStringList toolbarNames;
    static ToolBarManager* _instance;
};

} // namespace Gui


#endif // GUI_TOOLBARMANAGER_H
