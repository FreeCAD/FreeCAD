// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "TaskDialog.h"
#include "TaskWatcher.h"


namespace Gui
{
namespace TaskView
{

class ControlPy: public Py::PythonExtension<ControlPy>
{
public:
    static void init_type();  // announce properties and methods
    static ControlPy* getInstance();

    ControlPy();
    ~ControlPy() override;

    Py::Object repr() override;
    Py::Object showDialog(const Py::Tuple&);
    Py::Object activeDialog(const Py::Tuple&);
    Py::Object activeTaskDialog(const Py::Tuple&);
    Py::Object closeDialog(const Py::Tuple&);
    Py::Object addTaskWatcher(const Py::Tuple&);
    Py::Object clearTaskWatcher(const Py::Tuple&);
    Py::Object isAllowedAlterDocument(const Py::Tuple&);
    Py::Object isAllowedAlterView(const Py::Tuple&);
    Py::Object isAllowedAlterSelection(const Py::Tuple&);
    Py::Object showTaskView(const Py::Tuple&);
    Py::Object showModelView(const Py::Tuple&);

private:
    static ControlPy* instance;
};

class GuiExport TaskWatcherPython: public TaskWatcher
{
public:
    explicit TaskWatcherPython(const Py::Object&);
    ~TaskWatcherPython() override;
    bool shouldShow() override;

private:
    Py::Object watcher;
};

/**
 * @brief The TaskDialogPy class
 * This class exposes a TaskDialog written in C++ to Python.
 */
class TaskDialogPy: public Py::PythonExtension<TaskDialogPy>
{
public:
    using BaseType = Py::PythonExtension<TaskDialogPy>;
    static void init_type();  // announce properties and methods

    explicit TaskDialogPy(TaskDialog*);
    ~TaskDialogPy() override;

    Py::Object repr() override;
    Py::Object getattr(const char*) override;
    int setattr(const char*, const Py::Object&) override;

public:
    Py::Object getDialogContent(const Py::Tuple&);

    /// tells the framework which buttons are wished for the dialog
    Py::Object getStandardButtons(const Py::Tuple&);

    /// Defines whether a task dialog can be rejected by pressing Esc
    Py::Object setEscapeButtonEnabled(const Py::Tuple&);
    Py::Object isEscapeButtonEnabled(const Py::Tuple&);

    /// Defines whether a task dialog must be closed if the document changed the
    /// active transaction.
    Py::Object setAutoCloseOnTransactionChange(const Py::Tuple&);
    Py::Object isAutoCloseOnTransactionChange(const Py::Tuple&);
    Py::Object setAutoCloseOnDeletedDocument(const Py::Tuple&);
    Py::Object isAutoCloseOnDeletedDocument(const Py::Tuple&);

    Py::Object getDocumentName(const Py::Tuple&);
    Py::Object setDocumentName(const Py::Tuple&);

    /*!
      Indicates whether this task dialog allows other commands to modify
      the document while it is open.
    */
    Py::Object isAllowedAlterDocument(const Py::Tuple&);

    /*!
      Indicates whether this task dialog allows other commands to modify
      the 3d view while it is open.
    */
    Py::Object isAllowedAlterView(const Py::Tuple&);

    /*!
      Indicates whether this task dialog allows other commands to modify
      the selection while it is open.
    */
    Py::Object isAllowedAlterSelection(const Py::Tuple&);
    Py::Object needsFullSpace(const Py::Tuple&);

    /// is called by the framework if the dialog is accepted (Ok)
    Py::Object accept(const Py::Tuple&);
    /// is called by the framework if the dialog is rejected (Cancel)
    Py::Object reject(const Py::Tuple&);

private:
    QPointer<TaskDialog> dialog;
};

/**
 * @brief The TaskDialogPython class
 * This wraps a task dialog that is written in Python.
 */
class GuiExport TaskDialogPython: public TaskDialog
{
public:
    explicit TaskDialogPython(const Py::Object&);
    ~TaskDialogPython() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override;
    void modifyStandardButtons(QDialogButtonBox*) override;

    /*!
      Indicates whether this task dialog allows other commands to modify
      the document while it is open.
    */
    bool isAllowedAlterDocument() const override;
    /*!
      Indicates whether this task dialog allows other commands to modify
      the 3d view while it is open.
    */
    bool isAllowedAlterView() const override;
    /*!
      Indicates whether this task dialog allows other commands to modify
      the selection while it is open.
    */
    bool isAllowedAlterSelection() const override;
    bool needsFullSpace() const override;

    void autoClosedOnTransactionChange() override;
    void autoClosedOnDeletedDocument() override;

public:
    /// is called by the framework when the dialog is opened
    void open() override;
    /// is called by the framework if a button is clicked which has no accept or reject role
    void clicked(int) override;
    /// is called by the framework if the dialog is accepted (Ok)
    bool accept() override;
    /// is called by the framework if the dialog is rejected (Cancel)
    bool reject() override;
    /// is called by the framework if the user press the help button
    void helpRequested() override;

    /// event handling
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    bool tryLoadUiFile();
    bool tryLoadForm();
    void appendForm(QWidget* widget, const QPixmap& icon);
    void clearForm();

private:
    Py::Object dlg;
};

}  // namespace TaskView
}  // namespace Gui
