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


#pragma once

#include <string>
#include <fastsignals/signal.h>

#include <QStringList>
#include <QPointer>
#include <QTimer>
#include <QToolBar>
#include <QPointer>

#include <FCGlobal.h>
#include <Base/Parameter.h>

class QAction;
class QLayout;
class QMenu;
class QMouseEvent;

namespace Gui
{

class ToolBarAreaWidget;
enum class ToolBarArea;

class GuiExport ToolBarItem
{
public:
    /** Manages the default visibility status of a toolbar item, as well as the default status
     * of the toggleViewAction usable by the contextual menu to enable and disable its visibility
     */
    enum class DefaultVisibility
    {
        Visible,      // toolbar is hidden by default, visibility toggle action is enabled
        Hidden,       // toolbar hidden by default, visibility toggle action is enabled
        Unavailable,  // toolbar visibility is managed independently by client code and defaults to
                      // hidden, visibility toggle action is disabled by default (it is unavailable
                      // to the UI). Upon being forced to be available, these toolbars default to
                      // visible.
    };

    ToolBarItem();
    explicit ToolBarItem(
        ToolBarItem* item,
        DefaultVisibility visibilityPolicy = DefaultVisibility::Visible
    );
    ~ToolBarItem();

    void setCommand(const std::string&);
    const std::string& command() const;

    bool hasItems() const;
    ToolBarItem* findItem(const std::string&);
    ToolBarItem* copy() const;
    uint count() const;

    void appendItem(ToolBarItem* item);
    bool insertItem(ToolBarItem*, ToolBarItem* item);
    void removeItem(ToolBarItem* item);
    void clear();

    ToolBarItem& operator<<(ToolBarItem* item);
    ToolBarItem& operator<<(const std::string& command);
    QList<ToolBarItem*> getItems() const;

    DefaultVisibility visibilityPolicy;

private:
    std::string _name;
    QList<ToolBarItem*> _items;
};

class ToolBarGrip: public QWidget
{
    Q_OBJECT

public:
    explicit ToolBarGrip(QToolBar*);

    void attach();
    void detach();

    bool isAttached() const;

protected:
    void paintEvent(QPaintEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);

    void updateSize();

private:
    QPointer<QAction> _action = nullptr;
};

/**
 * QToolBar from Qt lacks few abilities like ability to float toolbar from code.
 * This class allows us to provide custom behaviors for toolbars if needed.
 */
class GuiExport ToolBar: public QToolBar
{
    Q_OBJECT

    friend class ToolBarGrip;

public:
    ToolBar();
    explicit ToolBar(QWidget* parent);

    virtual ~ToolBar() = default;

    void undock();
    void updateCustomGripVisibility();

protected:
    void setupConnections();
};

/**
 * The ToolBarManager class is responsible for the creation of toolbars and appending them
 * to the main window.
 * @see ToolBoxManager
 * @see MenuManager
 * @author Werner Mayer
 */
class GuiExport ToolBarManager: public QObject
{
    Q_OBJECT
public:
    enum class State
    {
        ForceHidden,     // Forces a toolbar to hide and hides the toggle action
        ForceAvailable,  // Forces a toolbar toggle action to show, visibility depends on user config
        RestoreDefault,  // Restores a toolbar toggle action default, visibility as user config
        SaveState,       // Saves the state of the toolbars
    };

    /// The one and only instance.
    static ToolBarManager* getInstance();
    static void destruct();
    /** Sets up the toolbars of a given workbench. */
    void setup(ToolBarItem*);
    void saveState() const;
    void restoreState() const;
    void retranslate() const;

    bool areToolBarsLocked() const;
    void setToolBarsLocked(bool locked) const;

    void setState(const QList<QString>& names, State state);
    void setState(const QString& name, State state);

    int toolBarIconSize(QWidget* widget = nullptr) const;
    void setupToolBarIconSize();

    ToolBarArea toolBarArea(QWidget* toolBar) const;
    ToolBarAreaWidget* toolBarAreaWidget(QWidget* toolBar) const;

protected:
    void setup(ToolBarItem*, QToolBar*) const;

    void setMovable(bool movable) const;

    ToolBarItem::DefaultVisibility getToolbarPolicy(const QToolBar*) const;

    bool addToolBarToArea(QObject* source, QMouseEvent* ev);
    bool showContextMenu(QObject* source);
    void onToggleStatusBarWidget(QWidget* widget, bool visible);
    void setToolBarIconSize(QToolBar* toolbar);
    void onTimer();

    bool eventFilter(QObject* source, QEvent* ev) override;

    /** Returns a list of all currently existing toolbars. */
    QList<ToolBar*> toolBars() const;
    ToolBar* findToolBar(const QList<ToolBar*>&, const QString&) const;
    QAction* findAction(const QList<QAction*>&, const QString&) const;
    ToolBarManager();
    ~ToolBarManager() override;

private:
    void setupParameters();
    void setupStatusBar();
    void setupMenuBar();
    void setupConnection();
    void setupTimer();
    void setupSizeTimer();
    void setupResizeTimer();
    void setupMenuBarTimer();
    void setupWidgetProducers();

    void addToMenu(QLayout* layout, QWidget* area, QMenu* menu);
    QLayout* findLayoutOfObject(QObject* source, QWidget* area) const;
    ToolBarAreaWidget* findToolBarAreaWidget() const;

private:
    QStringList toolbarNames;
    static ToolBarManager* _instance;

    QTimer timer;
    QTimer menuBarTimer;
    QTimer sizeTimer;
    QTimer resizeTimer;
    fastsignals::advanced_scoped_connection connParam;
    ToolBarAreaWidget* statusBarAreaWidget = nullptr;
    ToolBarAreaWidget* menuBarLeftAreaWidget = nullptr;
    ToolBarAreaWidget* menuBarRightAreaWidget = nullptr;
    ParameterGrp::handle hGeneral;
    ParameterGrp::handle hPref;
    ParameterGrp::handle hStatusBar;
    ParameterGrp::handle hMenuBarLeft;
    ParameterGrp::handle hMenuBarRight;
    std::map<QToolBar*, QPointer<QToolBar>> resizingToolbars;
    int _toolBarIconSize = 0;
    int _statusBarIconSize = 0;
    int _menuBarIconSize = 0;
    bool blockRestore = false;
};

}  // namespace Gui
