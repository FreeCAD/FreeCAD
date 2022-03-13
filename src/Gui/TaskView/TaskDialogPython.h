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


#ifndef GUI_TASKVIEW_TASKDIALOGPYTHON_H
#define GUI_TASKVIEW_TASKDIALOGPYTHON_H

#include "TaskDialog.h"
#include "TaskWatcher.h"


namespace Gui {
namespace TaskView {

class ControlPy : public Py::PythonExtension<ControlPy> 
{
public:
    static void init_type(void);    // announce properties and methods
    static ControlPy* getInstance();

    ControlPy();
    ~ControlPy();

    Py::Object repr();
    Py::Object showDialog(const Py::Tuple&);
    Py::Object activeDialog(const Py::Tuple&);
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

class GuiExport TaskWatcherPython : public TaskWatcher
{
public:
    TaskWatcherPython(const Py::Object&);
    ~TaskWatcherPython();
    bool shouldShow();

private:
    Py::Object watcher;
};

class GuiExport TaskDialogPython : public TaskDialog
{
public:
    TaskDialogPython(const Py::Object&);
    ~TaskDialogPython();

    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const;
    virtual void modifyStandardButtons(QDialogButtonBox*);

    /*!
      Indicates whether this task dialog allows other commands to modify
      the document while it is open.
    */
    virtual bool isAllowedAlterDocument(void) const;
    /*!
      Indicates whether this task dialog allows other commands to modify
      the 3d view while it is open.
    */
    virtual bool isAllowedAlterView(void) const;
    /*!
      Indicates whether this task dialog allows other commands to modify
      the selection while it is open.
    */
    virtual bool isAllowedAlterSelection(void) const;
    virtual bool needsFullSpace() const;

public:
    /// is called by the framework when the dialog is opened
    virtual void open();
    /// is called by the framework if a button is clicked which has no accept or reject role
    virtual void clicked(int);
    /// is called by the framework if the dialog is accepted (Ok)
    virtual bool accept();
    /// is called by the framework if the dialog is rejected (Cancel)
    virtual bool reject();
    /// is called by the framework if the user press the help button 
    virtual void helpRequested();

private:
    void clearForm();

private:
    Py::Object dlg;
};

} //namespace TaskView
} //namespace Gui

#endif // GUI_TASKVIEW_TASKDIALOGPYTHON_H

