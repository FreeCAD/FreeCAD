/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_TASKVIEW_TASKVIEW_H
#define GUI_TASKVIEW_TASKVIEW_H

#include <vector>
#include <QScrollArea>

#include <Gui/QSint/include/QSint>
#include <Gui/Selection.h>
#include "TaskWatcher.h"


namespace App {
class Property;
}

namespace Gui {
class ControlSingleton;
namespace DockWnd{
class ComboView;
}
namespace TaskView {

typedef boost::signals2::connection Connection;
class TaskEditControl;
class TaskDialog;

/// Father class of all content in TaskView
class GuiExport TaskContent 
{

public:
    //TaskContent();
    //~TaskContent();
};

class GuiExport TaskGroup : public QSint::ActionBox, public TaskContent
{
    Q_OBJECT

public:
    explicit TaskGroup(QWidget *parent = nullptr);
    explicit TaskGroup(const QString & headerText, QWidget *parent = nullptr);
    explicit TaskGroup(const QPixmap & icon, const QString & headerText, QWidget *parent = nullptr);
    ~TaskGroup();

protected:
    void actionEvent (QActionEvent*);
};

/// Father class of content with header and Icon
class GuiExport TaskBox : public QSint::ActionGroup, public TaskContent
{
    Q_OBJECT

public:
    /** Constructor. Creates TaskBox without header.
      */
    explicit TaskBox(QWidget *parent = nullptr);

    /** Constructor. Creates TaskBox with header's
        text set to \a title, but with no icon.

        If \a expandable set to \a true (default), the group can be expanded/collapsed by the user.
      */
    explicit TaskBox(const QString& title,
                     bool expandable = true,
                     QWidget *parent = nullptr);

    /** Constructor. Creates TaskBox with header's
        text set to \a title and icon set to \a icon.

        If \a expandable set to \a true (default), the group can be expanded/collapsed by the user.
      */
    explicit TaskBox(const QPixmap& icon,
                     const QString& title,
                     bool expandable = true,
                     QWidget *parent = nullptr);
    virtual QSize minimumSizeHint() const;

    ~TaskBox();
    void hideGroupBox();
    bool isGroupVisible() const;

protected:
    void showEvent(QShowEvent*);
    void actionEvent (QActionEvent*);

private:
    bool wasShown;
};

class GuiExport TaskPanel : public QSint::ActionPanel
{
    Q_OBJECT

public:
    explicit TaskPanel(QWidget *parent = nullptr);
    virtual ~TaskPanel();
    virtual QSize minimumSizeHint() const;
};

/// Father class of content of a Free widget (without header and Icon), shut be an exception!
class GuiExport TaskWidget : public QWidget, public TaskContent
{
    Q_OBJECT

public:
    TaskWidget(QWidget *parent=nullptr);
    ~TaskWidget();
};

/** TaskView class
  * handles the FreeCAD task view panel. Keeps track of the inserted content elements.
  * This elements get injected mostly by the ViewProvider classes of the selected
  * DocumentObjects. 
  */
class GuiExport TaskView : public QScrollArea, public Gui::SelectionSingleton::ObserverType
{
    Q_OBJECT

public:
    TaskView(QWidget *parent = nullptr);
    ~TaskView();

    /// Observer message from the Selection
    virtual void OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                          Gui::SelectionSingleton::MessageType Reason);

    friend class Gui::DockWnd::ComboView;
    friend class Gui::ControlSingleton;

    void addTaskWatcher(const std::vector<TaskWatcher*> &Watcher);
    void clearTaskWatcher(void);

    void clearActionStyle();
    void restoreActionStyle();

protected Q_SLOTS:
    void accept();
    void reject();
    void helpRequested();
    void clicked (QAbstractButton * button);

protected:
    virtual void keyPressEvent(QKeyEvent*);
    virtual bool event(QEvent*);

    void addTaskWatcher(void);
    void removeTaskWatcher(void);
    /// update the visibility of the TaskWatcher accordant to the selection
    void updateWatcher(void);
    /// used by Gui::Control to register Dialogs
    void showDialog(TaskDialog *dlg);
    // removes the running dialog after accept() or reject() from the TaskView
    void removeDialog(void);

    void slotActiveDocument(const App::Document&);
    void slotDeletedDocument();
    void slotUndoDocument(const App::Document&);
    void slotRedoDocument(const App::Document&);

    std::vector<TaskWatcher*> ActiveWatcher;

    QSint::ActionPanel* taskPanel;
    TaskDialog *ActiveDialog;
    TaskEditControl *ActiveCtrl;

    Connection connectApplicationActiveDocument;
    Connection connectApplicationDeleteDocument;
    Connection connectApplicationUndoDocument;
    Connection connectApplicationRedoDocument;
};

} //namespace TaskView
} //namespace Gui

#endif // GUI_TASKVIEW_TASKVIEW_H
