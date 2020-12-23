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


#ifndef GUI_ACTION_H
#define GUI_ACTION_H

#include <QAction>
#include <QComboBox>
#include <QKeySequence>

namespace Gui 
{
class Command;

/**
 * The Action class is the link between Qt's QAction class and FreeCAD's 
 * command classes (@ref Command). So, it is possible to have all actions
 * (from toolbars, menus, ...) implemented in classes instead of many slot
 * methods in the class @ref MainWindow.
 * @author Werner Mayer
 */
class GuiExport Action : public QObject
{
    Q_OBJECT

public:
    Action (Command* pcCmd, QObject * parent = 0);
    /// Action takes ownership of the 'action' object.
    Action (Command* pcCmd, QAction* action, QObject * parent);
    virtual ~Action();

    virtual void addTo (QWidget * w);
    virtual void setEnabled(bool);
    virtual void setVisible(bool);

    void setCheckable(bool);
    void setChecked (bool, bool no_signal=false);
    bool isChecked() const;

    void setShortcut (const QString &);
    QKeySequence shortcut() const;
    void setIcon (const QIcon &);
    QIcon icon() const;
    void setStatusTip (const QString &);
    QString statusTip() const;
    void setText (const QString &);
    QString text() const;
    void setToolTip (const QString &);
    QString toolTip() const;
    void setWhatsThis (const QString &);
    QString whatsThis() const;
    void setMenuRole(QAction::MenuRole menuRole);
    QAction *action() {return _action;};

public Q_SLOTS:
    virtual void onActivated ();
    virtual void onToggled   (bool); 

protected:
    QAction* _action;
    Command *_pcCmd;
};

// --------------------------------------------------------------------

/**
 * The ActionGroup class is the link between Qt's QActionGroup class and 
 * FreeCAD's command classes (@ref Command). Compared to Action with an
 * ActionGroup it is possible to implement a single command with a group
 * of toggable actions where e.g. one is set exclusive.
 * @author Werner Mayer
 */
class GuiExport ActionGroup : public Action
{
    Q_OBJECT

public:
    ActionGroup (Command* pcCmd, QObject * parent = 0);
    virtual ~ActionGroup();

    void addTo (QWidget * w);
    void setEnabled (bool);
    void setDisabled (bool);
    void setExclusive (bool);
    bool isExclusive() const;
    void setVisible (bool);

    void setDropDownMenu(bool b) { _dropDown = b; }
    QAction* addAction(QAction*);
    QAction* addAction(const QString&);
    QList<QAction*> actions() const;
    int checkedAction() const;
    void setCheckedAction(int);

public Q_SLOTS:
    void onActivated ();
    void onToggled(bool);
    void onActivated (QAction*);
    void onHovered   (QAction*);

protected:
    QActionGroup* _group;
    bool _dropDown;
    bool _external;
    bool _toggle;
};

// --------------------------------------------------------------------

class WorkbenchGroup;
class GuiExport WorkbenchComboBox : public QComboBox
{
    Q_OBJECT

public:
    WorkbenchComboBox(WorkbenchGroup* wb, QWidget* parent=0);
    virtual ~WorkbenchComboBox();
    void showPopup();

public Q_SLOTS:
    void onActivated(int);
    void onActivated(QAction*);

protected Q_SLOTS:
    void onWorkbenchActivated(const QString&);

protected:
    void actionEvent (QActionEvent*);

private:
    WorkbenchGroup* group;
};

/**
 * The WorkbenchGroup class represents a list of workbenches. When it is added
 * to a menu a submenu gets created, if added to a toolbar a combo box gets created.
 * @author Werner Mayer
 */
class GuiExport WorkbenchGroup : public ActionGroup
{
    Q_OBJECT

public:
    /** 
     * Creates an action for the command \a pcCmd to load the workbench \a name
     * when it gets activated.
     */
    WorkbenchGroup (Command* pcCmd, QObject * parent);
    virtual ~WorkbenchGroup();
    void addTo (QWidget * w);
    void refreshWorkbenchList();

    void slotActivateWorkbench(const char*);
    void slotAddWorkbench(const char*);
    void slotRemoveWorkbench(const char*);

protected:
    void customEvent(QEvent* e);

private:
    void setWorkbenchData(int i, const QString& wb);
};

// --------------------------------------------------------------------

/**
 * The RecentFilesAction class holds a menu listed with the recent files. 
 * @author Werner Mayer
 */
class GuiExport RecentFilesAction : public ActionGroup
{
    Q_OBJECT

public:
    RecentFilesAction (Command* pcCmd, QObject * parent = 0);
    virtual ~RecentFilesAction();

    void appendFile(const QString&);
    void activateFile(int);
    void resizeList(int);

private:
    void setFiles(const QStringList&);
    QStringList files() const;
    void restore();
    void save();

private:
    int visibleItems; /**< Number of visible items */
    int maximumItems; /**< Number of maximum items */ 
};

// --------------------------------------------------------------------

/**
 * The UndoAction class reimplements a special behaviour to make a menu 
 * appearing when the button with the arrow is clicked.
 * @author Werner Mayer
 */
class GuiExport UndoAction : public Action
{
    Q_OBJECT

public:
    UndoAction (Command* pcCmd,QObject * parent = 0);
    ~UndoAction();
    void addTo (QWidget * w);
    void setEnabled(bool);
    void setVisible(bool);

private Q_SLOTS:
    void actionChanged();

private:
    QAction* _toolAction;
};

// --------------------------------------------------------------------

/**
 * The RedoAction class reimplements a special behaviour to make a menu 
 * appearing when the button with the arrow is clicked.
 * @author Werner Mayer
 */
class GuiExport RedoAction : public Action
{
    Q_OBJECT

public:
    RedoAction (Command* pcCmd,QObject * parent = 0);
    ~RedoAction();
    void addTo ( QWidget * w );
    void setEnabled(bool);
    void setVisible(bool);

private Q_SLOTS:
    void actionChanged();

private:
    QAction* _toolAction;
};

// --------------------------------------------------------------------

/**
 * Special action to show all dockable views -- except of toolbars -- in an own popup menu.
 * @author Werner Mayer
 */
class GuiExport DockWidgetAction : public Action
{
    Q_OBJECT

public:
    DockWidgetAction (Command* pcCmd, QObject * parent = 0);
    virtual ~DockWidgetAction();
    void addTo (QWidget * w);

private:
    QMenu* _menu;
};

// --------------------------------------------------------------------

/**
 * Special action to show all toolbars in an own popup menu.
 * @author Werner Mayer
 */
class GuiExport ToolBarAction : public Action
{
    Q_OBJECT

public:
    ToolBarAction (Command* pcCmd, QObject * parent = 0);
    virtual ~ToolBarAction();
    void addTo (QWidget * w);

private:
    QMenu* _menu;
};

// --------------------------------------------------------------------

/**
 * @author Werner Mayer
 */
class GuiExport WindowAction : public ActionGroup
{
    Q_OBJECT

public:
    WindowAction (Command* pcCmd, QObject * parent = 0);
    virtual ~WindowAction();
    void addTo (QWidget * w);

private:
    QMenu* _menu;
};

} // namespace Gui

#endif // GUI_ACTION_H
