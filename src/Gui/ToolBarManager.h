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

#include <boost_signals2.hpp>
#include <string>
#include <QStringList>
#include <QPointer>
#include <QTimer>
#include <Base/Parameter.h>

class QAction;
class QToolBar;
class QMouseEvent;
class StatusBarArea;

namespace Gui {

class Workbench;

class GuiExport ToolBarItem
{
public:
    ToolBarItem();
    ToolBarItem(ToolBarItem* item);
    ~ToolBarItem();

    void setCommand(const std::string&);
    const std::string & command() const;

    void setID(const std::string&);
    const std::string & id() const;

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

private:
    std::string _name;
    std::string _id;
    QList<ToolBarItem*> _items;
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
    /// The one and only instance.
    static ToolBarManager* getInstance();
    static void destruct();
    /** Sets up the toolbars of a given workbench. */
    void setup(ToolBarItem*);
    void saveState() const;
    void restoreState();
    void setDefaultMovable(bool enable);
    bool isDefaultMovable() const;
    void retranslate();
    static void checkToolbar();

protected Q_SLOTS:
    void onToggleToolBar(bool);
    void onMovableChanged(bool);
    void onTimer();

protected:
    void setup(ToolBarItem*, QToolBar*) const;
    /** Returns a list of all currently existing toolbars. */
    std::map<QString, QPointer<QToolBar>> toolBars();
    QAction* findAction(const QList<QAction*>&, const QString&) const;
    QToolBar *createToolBar(const QString &name);
    void connectToolBar(QToolBar *);
    void setToolBarVisible(QToolBar *toolbar, bool show);
    void getGlobalToolbarNames();
    bool eventFilter(QObject *, QEvent *);

    bool addToolbarToStatusBar(QObject *, QMouseEvent*);
    void showStatusBarContextMenu();
    void onToggleStatusBarWidget(QWidget *, bool);

    ToolBarManager();
    ~ToolBarManager();

private:
    QTimer timer;
    QTimer timerChild;
    QStringList toolbarNames;
    static ToolBarManager* _instance;
    ParameterGrp::handle hPref;
    ParameterGrp::handle hMovable;
    ParameterGrp::handle hMainWindow;
    ParameterGrp::handle hGlobal;
    ParameterGrp::handle hStatusBar;
    boost::signals2::scoped_connection connParam;
    bool restored = false;
    bool migrating = false;
    bool adding = false;
    Qt::ToolBarArea defaultArea;
    Qt::ToolBarArea globalArea;
    std::set<QString> globalToolBarNames;
    std::map<QToolBar*, QPointer<QToolBar>> connectedToolBars;
    StatusBarArea *statusBarArea = nullptr;
};

} // namespace Gui


#endif // GUI_TOOLBARMANAGER_H
