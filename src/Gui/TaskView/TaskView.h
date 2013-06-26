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

#include <map>
#include <string>
#include <vector>
#include <boost/signals.hpp>

#include <Gui/iisTaskPanel/include/iisTaskPanel>
#include <Gui/Selection.h>
#include "TaskWatcher.h"

namespace App {
class Property;
}

namespace Gui {
class ControlSingleton;
namespace DockWnd{
class CombiView;
}
namespace TaskView {

typedef boost::signals::connection Connection;
class TaskEditControl;
class TaskDialog;

/// Father class of all content in TaskView
class GuiExport TaskContent 
{

public:
    //TaskContent();
    //~TaskContent();
};

class GuiExport TaskGroup : public iisTaskGroup, public TaskContent
{
    Q_OBJECT

public:
    TaskGroup(QWidget *parent = 0);
    ~TaskGroup();

protected:
    void actionEvent (QActionEvent*);
};

/// Father class of content with header and Icon
class GuiExport TaskBox : public iisTaskBox, public TaskContent
{
    Q_OBJECT

public:
    TaskBox(const QPixmap &icon, const QString &title, bool expandable, QWidget *parent);
    ~TaskBox();
    void hideGroupBox();

protected:
    void showEvent(QShowEvent*);
    void actionEvent (QActionEvent*);

private:
    bool wasShown;
};

/// Father class of content of a Free widget (without header and Icon), shut be an exception!
class GuiExport TaskWidget : public QWidget, public TaskContent
{
    Q_OBJECT

public:
    TaskWidget(QWidget *parent=0);
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
    TaskView(QWidget *parent = 0);
    ~TaskView();

    /// Observer message from the Selection
    virtual void OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                          Gui::SelectionSingleton::MessageType Reason);

    friend class Gui::DockWnd::CombiView;
    friend class Gui::ControlSingleton;

    void addTaskWatcher(const std::vector<TaskWatcher*> &Watcher);
    void clearTaskWatcher(void);

protected Q_SLOTS:
    void accept();
    void reject();
    void helpRequested();
    void clicked (QAbstractButton * button);

protected:
    void keyPressEvent(QKeyEvent*);
    void addTaskWatcher(void);
    void removeTaskWatcher(void);
    /// update the visibility of the TaskWatcher accordant to the selection
    void updateWatcher(void);
    /// used by Gui::Contol to register Dialogs
    void showDialog(TaskDialog *dlg);
    // removes the running dialog after accept() or reject() from the TaskView
    void removeDialog(void);

    void slotActiveDocument(const App::Document&);
    void slotDeletedDocument();
    void slotUndoDocument(const App::Document&);
    void slotRedoDocument(const App::Document&);

    std::vector<TaskWatcher*> ActiveWatcher;

    iisTaskPanel* taskPanel;
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
