/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_CONTROL_H
#define GUI_CONTROL_H

// Std. configurations

#include <QObject>
#include <bitset>
#include <stack>

#include <Gui/TaskView/TaskDialog.h>

class QDockWidget;
class QTabBar;

namespace App
{
class DocumentObject;
class Document;
}  // namespace App

namespace Gui
{
namespace TaskView
{
class TaskDialog;
class TaskView;
}  // namespace TaskView


/** The control class
 */
class GuiExport ControlSingleton: public QObject
{
    Q_OBJECT

public:
    static ControlSingleton& instance();
    static void destruct();

    /** @name dialog handling
     *  These methods are used to control the TaskDialog stuff.
     */
    //@{
    /// This method starts a task dialog in the task view
    /// The dialog is relative to a specific document
    void showDialog(Gui::TaskView::TaskDialog* dlg, App::Document* attachTo = nullptr);
    Gui::TaskView::TaskDialog* activeDialog(App::Document* attachedTo = nullptr) const;
    // void closeDialog();
    //@}

    /** @name task view handling
     */
    //@{
    Gui::TaskView::TaskView* taskPanel() const;
    /// raising the model view
    void showModelView();
    //@}

    /*!
      If a task dialog is open then it indicates whether this task dialog allows other commands to
      modify the document while it is open. If no task dialog is open true is returned.
     */
    bool isAllowedAlterDocument(App::Document* attachedTo = nullptr) const;
    /*!
      If a task dialog is open then it indicates whether this task dialog allows other commands to
      modify the 3d view while it is open. If no task dialog is open true is returned.
     */
    bool isAllowedAlterView(App::Document* attachedTo = nullptr) const;
    /*!
      If a task dialog is open then it indicates whether this task dialog allows other commands to
      modify the selection while it is open. If no task dialog is open true is returned.
     */
    bool isAllowedAlterSelection(App::Document* attachedTo = nullptr) const;

public Q_SLOTS:
    void accept(App::Document* attachedTo = nullptr);
    void reject(App::Document* attachedTo = nullptr);
    void closeDialog(App::Document* attachedTo = nullptr);

    /// raises the task view panel
    void showTaskView();

private:
    /// This get called by the TaskView when the Dialog is finished
    void closedDialog(App::Document* attachedTo = nullptr);

private:
    struct status
    {
        std::bitset<32> StatusBits;
    } CurrentStatus;

    std::stack<status> StatusStack;

    std::map<App::Document*, Gui::TaskView::TaskDialog*> ActiveDialogs;
    int oldTabIndex;

private:
    /// Construction
    ControlSingleton();
    /// Destruction
    ~ControlSingleton() override;
    void showDockWidget(QWidget*);
    QTabBar* findTabBar(QDockWidget*) const;
    void aboutToShowDialog(QDockWidget* widget);
    void aboutToHideDialog(QDockWidget* widget);

    // Returns attachTo if not nullptr, otherwise return the active document
    static App::Document* docOrDefault(App::Document* attachedTo);

    static ControlSingleton* _pcSingleton;
};

/// Get the global instance
inline ControlSingleton& Control()
{
    return ControlSingleton::instance();
}

}  // namespace Gui

#endif  // GUI_CONTROL_H
