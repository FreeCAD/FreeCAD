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

#include <memory>
#include <QAction>
#include <QComboBox>
#include <QKeySequence>
#include <FCGlobal.h>

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
    explicit Action (Command* pcCmd, QObject * parent = nullptr);
    /// Action takes ownership of the 'action' object.
    Action (Command* pcCmd, QAction* action, QObject * parent);
    ~Action() override;

    virtual void addTo (QWidget * widget);
    virtual void setEnabled(bool);
    virtual void setVisible(bool);

    void setCheckable(bool);
    void setChecked (bool, bool no_signal=false);
    bool isChecked() const;
    bool isEnabled() const;

    void setShortcut (const QString &);
    QKeySequence shortcut() const;
    void setIcon (const QIcon &);
    QIcon icon() const;
    void setStatusTip (const QString &);
    QString statusTip() const;
    void setText (const QString &);
    QString text() const;
    void setToolTip (const QString &, const QString &title = QString());
    QString toolTip() const;
    void setWhatsThis (const QString &);
    QString whatsThis() const;
    void setMenuRole(QAction::MenuRole menuRole);
    QAction *action() const {
        return _action;
    }

    static QString createToolTip(QString helpText,
                                 const QString &title,
                                 const QFont &font,
                                 const QString &shortCut,
                                 const Command *command = nullptr);

    /** Obtain tool tip of a given command
     * @param cmd: input command
     * @param richFormat: whether to output rich text formatted tooltip
     */
    static QString commandToolTip(const Command *cmd, bool richFormat = true);

    /** Obtain the menu text of a given command
     * @param cmd: input command
     * @return Return the command menu text that is stripped with its mnemonic
     * symbol '&' and ending punctuations
     */
    static QString commandMenuText(const Command *cmd);

    /// Clean the title by stripping the mnemonic symbol '&' and ending punctuations
    static QString cleanTitle(const QString &title);

    Command *command() const {
        return _pcCmd;
    }

public Q_SLOTS:
    virtual void onActivated ();
    virtual void onToggled   (bool);

private:
    QAction* _action;
    Command *_pcCmd;
    QString _tooltip;
    QString _title;
    QMetaObject::Connection _connection;

private:
    Q_DISABLE_COPY(Action)
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
    explicit ActionGroup (Command* pcCmd, QObject * parent = nullptr);
    ~ActionGroup() override;

    void addTo (QWidget * widget) override;
    void setEnabled (bool) override;
    void setDisabled (bool);
    void setExclusive (bool);
    bool isExclusive() const;
    void setVisible (bool) override;
    void setIsMode(bool check) { _isMode = check; }

    void setDropDownMenu(bool check) { _dropDown = check; }
    QAction* addAction(QAction*);
    QAction* addAction(const QString&);
    QList<QAction*> actions() const;
    int checkedAction() const;
    void setCheckedAction(int);

protected:
    QActionGroup* groupAction() const {
        return _group;
    }

public Q_SLOTS:
    void onActivated () override;
    void onToggled(bool) override;
    void onActivated (QAction*);
    void onHovered   (QAction*);

Q_SIGNALS:
    /// When drop down menu is enabled, the signal is triggered just before hiding the menu
    void aboutToHide(QMenu*);
    /// When drop down menu is enabled, the signal is triggered just before showing the menu
    void aboutToShow(QMenu*);

private:
    QActionGroup* _group;
    bool _dropDown;
    bool _isMode;

private:
    Q_DISABLE_COPY(ActionGroup)
};

// --------------------------------------------------------------------
class GuiExport WorkbenchComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit WorkbenchComboBox(QWidget* parent=nullptr);
    void showPopup() override;

public Q_SLOTS:
    void refreshList(QList<QAction*>);

private:
    Q_DISABLE_COPY(WorkbenchComboBox)
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
    void addTo (QWidget * widget) override;
    void refreshWorkbenchList();

    void slotActivateWorkbench(const char*);

Q_SIGNALS:
    void workbenchListRefreshed(QList<QAction*>);

protected Q_SLOTS:
    void onWorkbenchActivated(const QString&);

private:
    Q_DISABLE_COPY(WorkbenchGroup)
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
    explicit RecentFilesAction (Command* pcCmd, QObject * parent = nullptr);
    ~RecentFilesAction() override;

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

    class Private;
    friend class Private;
    std::unique_ptr<Private> _pimpl;

    Q_DISABLE_COPY(RecentFilesAction)
};

// --------------------------------------------------------------------

/**
 * The RecentMacrosAction class holds a menu listed with the recent macros
 * that were executed, edited, or created
 */
class GuiExport RecentMacrosAction : public ActionGroup
{
    Q_OBJECT

public:
    explicit RecentMacrosAction (Command* pcCmd, QObject * parent = nullptr);

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
    std::string shortcut_modifiers; /**< default = "Ctrl+Shift+" */
    int shortcut_count; /**< Number of dynamic shortcuts to create -- default = 3*/

    Q_DISABLE_COPY(RecentMacrosAction)
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
    explicit UndoAction (Command* pcCmd,QObject * parent = nullptr);
    ~UndoAction() override;
    void addTo (QWidget * widget) override;
    void setEnabled(bool) override;
    void setVisible(bool) override;

private Q_SLOTS:
    void actionChanged();

private:
    QAction* _toolAction;

    Q_DISABLE_COPY(UndoAction)
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
    explicit RedoAction (Command* pcCmd,QObject * parent = nullptr);
    ~RedoAction() override;
    void addTo ( QWidget * widget ) override;
    void setEnabled(bool) override;
    void setVisible(bool) override;

private Q_SLOTS:
    void actionChanged();

private:
    QAction* _toolAction;

    Q_DISABLE_COPY(RedoAction)
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
    explicit DockWidgetAction (Command* pcCmd, QObject * parent = nullptr);
    ~DockWidgetAction() override;
    void addTo (QWidget * widget) override;

private:
    QMenu* _menu;

    Q_DISABLE_COPY(DockWidgetAction)
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
    explicit ToolBarAction (Command* pcCmd, QObject * parent = nullptr);
    ~ToolBarAction() override;
    void addTo (QWidget * widget) override;

private:
    QMenu* _menu;

    Q_DISABLE_COPY(ToolBarAction)
};

// --------------------------------------------------------------------

/**
 * @author Werner Mayer
 */
class GuiExport WindowAction : public ActionGroup
{
    Q_OBJECT

public:
    explicit WindowAction (Command* pcCmd, QObject * parent = nullptr);
    void addTo (QWidget * widget) override;

private:
    QMenu* _menu;

    Q_DISABLE_COPY(WindowAction)
};

} // namespace Gui

#endif // GUI_ACTION_H
