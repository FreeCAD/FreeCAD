// SPDX-License-Identifier: LGPL-2.1-or-later
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


#pragma once

#include <vector>
#include <QScrollArea>

#include <Base/Parameter.h>
#include <Gui/QSint/include/QSint>
#include <Gui/Selection/Selection.h>
#include "TaskWatcher.h"


namespace App
{
class Property;
}

namespace Gui
{
class MDIView;
class ControlSingleton;
class ViewProviderDocumentObject;
namespace DockWnd
{
class ComboView;
}
namespace TaskView
{

using Connection = fastsignals::connection;
class TaskEditControl;
class TaskDialog;

/// Father class of all content in TaskView
class GuiExport TaskContent {

    public :
        // TaskContent();
        //~TaskContent();
};

class GuiExport TaskGroup: public QSint::ActionBox, public TaskContent
{
    Q_OBJECT

public:
    explicit TaskGroup(QWidget* parent = nullptr);
    explicit TaskGroup(const QString& headerText, QWidget* parent = nullptr);
    explicit TaskGroup(const QPixmap& icon, const QString& headerText, QWidget* parent = nullptr);
    ~TaskGroup() override;

protected:
    void actionEvent(QActionEvent*) override;
};

/// Father class of content with header and Icon
class GuiExport TaskBox: public QSint::ActionGroup, public TaskContent
{
    Q_OBJECT

public:
    /** Constructor. Creates TaskBox without header.
     */
    explicit TaskBox(QWidget* parent = nullptr);

    /** Constructor. Creates TaskBox with header's
        text set to \a title, but with no icon.

        If \a expandable set to \a true (default), the group can be expanded/collapsed by the user.
      */
    explicit TaskBox(const QString& title, bool expandable = true, QWidget* parent = nullptr);

    /** Constructor. Creates TaskBox with header's
        text set to \a title and icon set to \a icon.

        If \a expandable set to \a true (default), the group can be expanded/collapsed by the user.
      */
    explicit TaskBox(
        const QPixmap& icon,
        const QString& title,
        bool expandable = true,
        QWidget* parent = nullptr
    );
    QSize minimumSizeHint() const override;

    ~TaskBox() override;
    void hideGroupBox();
    bool isGroupVisible() const;

protected:
    void showEvent(QShowEvent*) override;
    void actionEvent(QActionEvent*) override;

private:
    bool wasShown;
};

class GuiExport TaskPanel: public QSint::ActionPanel
{
    Q_OBJECT

public:
    explicit TaskPanel(QWidget* parent = nullptr);
    ~TaskPanel() override;
    QSize minimumSizeHint() const override;
};

/// Father class of content of a Free widget (without header and Icon), shut be an exception!
class GuiExport TaskWidget: public QWidget, public TaskContent
{
    Q_OBJECT

public:
    explicit TaskWidget(QWidget* parent = nullptr);
    ~TaskWidget() override;
};

/** TaskView class
 * handles the FreeCAD task view panel. Keeps track of the inserted content elements.
 * This elements get injected mostly by the ViewProvider classes of the selected
 * DocumentObjects.
 */
class GuiExport TaskView: public QWidget, public Gui::SelectionSingleton::ObserverType
{
    Q_OBJECT

public:
    explicit TaskView(QWidget* parent = nullptr);
    ~TaskView() override;

    /// Observer message from the Selection
    void OnChange(
        Gui::SelectionSingleton::SubjectType& rCaller,
        Gui::SelectionSingleton::MessageType Reason
    ) override;

    friend class Gui::DockWnd::ComboView;
    friend class Gui::ControlSingleton;

    void addTaskWatcher(const std::vector<TaskWatcher*>& Watcher);
    void clearTaskWatcher();
    void takeTaskWatcher(TaskView* other);

    bool isEmpty(bool includeWatcher = true) const;

    void clearActionStyle();
    void restoreActionStyle();

    /// Add a persistent panel at the top of the task view, independent of the active dialog.
    void addContextualPanel(QWidget* panel);
    void removeContextualPanel(QWidget* panel);

    QSize minimumSizeHint() const override;

    // Restore width before opening a task panel
    void setRestoreWidth(bool on);
    bool shouldRestoreWidth() const;

Q_SIGNALS:
    void taskUpdate();

protected Q_SLOTS:
    void accept();
    void reject();
    void helpRequested();
    void clicked(QAbstractButton* button);

private:
    void triggerMinimumSizeHint();
    void adjustMinimumSizeHint();
    void saveCurrentWidth();
    void tryRestoreWidth();
    void slotActiveDocument(const App::Document&);
    void slotInEdit(const Gui::ViewProviderDocumentObject&);
    void slotDeletedDocument(const App::Document&);
    void slotViewClosed(const Gui::MDIView*);
    void slotUndoDocument(const App::Document&);
    void slotRedoDocument(const App::Document&);
    void transactionChangeOnDocument(const App::Document&, bool undo);
    QVBoxLayout* mainLayout;
    QScrollArea* scrollArea;
    QVBoxLayout* contextualPanelsLayout;
    QVBoxLayout* dialogLayout;
    QList<QWidget*> contextualPanels;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;

    void addTaskWatcher();
    void removeTaskWatcher();
    /// update the visibility of the TaskWatcher accordant to the selection
    void updateWatcher();
    /// used by Gui::Control to register Dialogs
    void showDialog(TaskDialog* dlg);
    // removes the running dialog after accept() or reject() from the TaskView
    void removeDialog();

    void setShowTaskWatcher(bool show);

    std::vector<TaskWatcher*> ActiveWatcher;

    QSint::ActionPanel* taskPanel;
    TaskDialog* ActiveDialog;
    TaskEditControl* ActiveCtrl;
    bool restoreWidth = false;
    int currentWidth = 0;
    ParameterGrp::handle hGrp;
    bool showTaskWatcher;

    Connection connectApplicationActiveDocument;
    Connection connectApplicationDeleteDocument;
    Connection connectApplicationClosedView;
    Connection connectApplicationUndoDocument;
    Connection connectApplicationRedoDocument;
    Connection connectApplicationInEdit;
    Connection connectShowTaskWatcherSetting;
};

}  // namespace TaskView
}  // namespace Gui
