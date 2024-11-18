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
}

namespace Gui
{
namespace TaskView
{
    class TaskDialog;
    class TaskView;
}


/** The control class
 */
class GuiExport ControlSingleton : public QObject
{
     Q_OBJECT

public:
    static ControlSingleton& instance();
    static void destruct ();

    /** @name dialog handling
     *  These methods are used to control the TaskDialog stuff.
     */
    //@{
    /// This method starts a task dialog in the task view
    void showDialog(Gui::TaskView::TaskDialog *dlg);
    Gui::TaskView::TaskDialog* activeDialog() const;
    //void closeDialog();
    //@}

    /** @name task view handling
     */
    //@{
    Gui::TaskView::TaskView* taskPanel() const;
    /// raising the model view
    void showModelView();
    //@}

    /*!
      If a task dialog is open then it indicates whether this task dialog allows other commands to modify
      the document while it is open. If no task dialog is open true is returned.
     */
    bool isAllowedAlterDocument() const;
    /*!
      If a task dialog is open then it indicates whether this task dialog allows other commands to modify
      the 3d view while it is open. If no task dialog is open true is returned.
     */
    bool isAllowedAlterView() const;
    /*!
      If a task dialog is open then it indicates whether this task dialog allows other commands to modify
      the selection while it is open. If no task dialog is open true is returned.
     */
    bool isAllowedAlterSelection() const;

public Q_SLOTS:
    void accept();
    void reject();
    void closeDialog();
    /// raises the task view panel
    void showTaskView();

private Q_SLOTS:
    /// This get called by the TaskView when the Dialog is finished
    void closedDialog();

private:
    struct status {
        std::bitset<32> StatusBits;
    } CurrentStatus;

    std::stack<status> StatusStack;

    Gui::TaskView::TaskDialog *ActiveDialog;
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

    static ControlSingleton* _pcSingleton;
};

/// Get the global instance
inline ControlSingleton& Control()
{
    return ControlSingleton::instance();
}

} //namespace Gui

#endif // GUI_CONTROL_H
