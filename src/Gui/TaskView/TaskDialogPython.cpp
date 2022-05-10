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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
# include <QFile>
# include <QPointer>
#endif

#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/UiLoader.h>
#include <Gui/PythonWrapper.h>

#include "TaskDialogPython.h"
#include "TaskView.h"


using namespace Gui;
using namespace Gui::TaskView;

ControlPy* ControlPy::instance = nullptr;

ControlPy* ControlPy::getInstance()
{
    if (!instance)
        instance = new ControlPy();
    return instance;
}

void ControlPy::init_type()
{
    behaviors().name("Control");
    behaviors().doc("Control for task dialogs");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();
    add_varargs_method("showDialog",&ControlPy::showDialog,
                        "show the given dialog in the task panel\n"
                        "showDialog(dialog)\n"
                        "--\n"
                        "if a task is already active a RuntimeError is raised");
    add_varargs_method("activeDialog",&ControlPy::activeDialog,
                        "check if a dialog is active in the task panel\n"
                        "activeDialog() --> bool");
    add_varargs_method("closeDialog",&ControlPy::closeDialog,
                        "close the active dialog\n"
                        "closeDialog()");
    add_varargs_method("addTaskWatcher",&ControlPy::addTaskWatcher,
                        "install a (list of) TaskWatcher\n"
                        "addTaskWatcher(TaskWatcher | list)");
    add_varargs_method("clearTaskWatcher",&ControlPy::clearTaskWatcher,
                        "remove all TaskWatchers\n"
                        "clearTaskWatcher()");
    add_varargs_method("isAllowedAlterDocument",&ControlPy::isAllowedAlterDocument,
                        "return the permission to alter the current Document\n"
                        "isAllowedAlterDocument() --> bool");
    add_varargs_method("isAllowedAlterView",&ControlPy::isAllowedAlterView,
                        "return the permission to alter the current View\n"
                        "isAllowedAlterView() --> bool");
    add_varargs_method("isAllowedAlterSelection",&ControlPy::isAllowedAlterSelection,
                        "return the permission to alter the current Selection\n"
                        "isAllowedAlterSelection() --> bool");
    add_varargs_method("showTaskView",&ControlPy::showTaskView,
                        "show the Task panel\n"
                        "showTaskView()");
    add_varargs_method("showModelView",&ControlPy::showModelView,
                        "show the Model panel\n"
                        "showModelView()");
}

ControlPy::ControlPy()
{
}

ControlPy::~ControlPy()
{
}

Py::Object ControlPy::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "Control Task Dialog";
    return Py::String(s_out.str());
}

Py::Object ControlPy::showDialog(const Py::Tuple& args)
{
    PyObject* arg0;
    if (!PyArg_ParseTuple(args.ptr(), "O", &arg0))
        throw Py::Exception();
    Gui::TaskView::TaskDialog* act = Gui::Control().activeDialog();
    if (act)
        throw Py::RuntimeError("Active task dialog found");
    TaskDialogPython* dlg = new TaskDialogPython(Py::Object(arg0));
    Gui::Control().showDialog(dlg);
    return Py::None();
}

Py::Object ControlPy::activeDialog(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    return Py::Boolean(dlg!=nullptr);
}

Py::Object ControlPy::closeDialog(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    Gui::Control().closeDialog();
    return Py::None();
}

Py::Object ControlPy::addTaskWatcher(const Py::Tuple& args)
{
    PyObject* arg0;
    if (!PyArg_ParseTuple(args.ptr(), "O", &arg0))
        throw Py::Exception();

    std::vector<Gui::TaskView::TaskWatcher*> watcher;
    Py::Sequence list(arg0);
    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        TaskWatcherPython* w = new TaskWatcherPython(*it);
        watcher.push_back(w);
    }

    Gui::TaskView::TaskView* taskView = Gui::Control().taskPanel();
    if (taskView)
        taskView->addTaskWatcher(watcher);
    return Py::None();
}

Py::Object ControlPy::clearTaskWatcher(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    Gui::TaskView::TaskView* taskView = Gui::Control().taskPanel();
    if (taskView)
        taskView->clearTaskWatcher();
    return Py::None();
}

Py::Object ControlPy::isAllowedAlterDocument(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    bool ok = Gui::Control().isAllowedAlterDocument();
    return Py::Boolean(ok);
}

Py::Object ControlPy::isAllowedAlterView(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    bool ok = Gui::Control().isAllowedAlterView();
    return Py::Boolean(ok);
}

Py::Object ControlPy::isAllowedAlterSelection(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    bool ok = Gui::Control().isAllowedAlterSelection();
    return Py::Boolean(ok);
}

Py::Object ControlPy::showTaskView(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    Gui::Control().showTaskView();
    return Py::None();
}

Py::Object ControlPy::showModelView(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    Gui::Control().showModelView();
    return Py::None();
}

// ------------------------------------------------------------------

TaskWatcherPython::TaskWatcherPython(const Py::Object& o)
  : TaskWatcher(nullptr), watcher(o)
{
    QString title;
    if (watcher.hasAttr(std::string("title"))) {
        Py::String name(watcher.getAttr(std::string("title")));
        std::string s = (std::string)name;
        title = QString::fromUtf8(s.c_str());
    }

    QPixmap icon;
    if (watcher.hasAttr(std::string("icon"))) {
        Py::String name(watcher.getAttr(std::string("icon")));
        std::string s = (std::string)name;
        icon = BitmapFactory().pixmap(s.c_str());
    }

    Gui::TaskView::TaskBox *tb = nullptr;
    if (watcher.hasAttr(std::string("commands"))) {
        tb = new Gui::TaskView::TaskBox(icon, title, true, nullptr);
        Py::Sequence cmds(watcher.getAttr(std::string("commands")));
        CommandManager &mgr = Gui::Application::Instance->commandManager();
        for (Py::Sequence::iterator it = cmds.begin(); it != cmds.end(); ++it) {
            Py::String name(*it);
            std::string s = (std::string)name;
            Command *c = mgr.getCommandByName(s.c_str());
            if (c)
                c->addTo(tb);
        }
    }

    if (watcher.hasAttr(std::string("widgets"))) {
        if (!tb && !title.isEmpty())
            tb = new Gui::TaskView::TaskBox(icon, title, true, nullptr);
        Py::Sequence list(watcher.getAttr(std::string("widgets")));

        Gui::PythonWrapper wrap;
        if (wrap.loadCoreModule()) {
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                QObject* object = wrap.toQObject(*it);
                if (object) {
                    QWidget* w = qobject_cast<QWidget*>(object);
                    if (w) {
                        if (tb)
                            tb->groupLayout()->addWidget(w);
                        else
                            Content.push_back(w);
                    }
                }
            }
        }
    }

    if (tb) Content.push_back(tb);

    if (watcher.hasAttr(std::string("filter"))) {
        Py::String name(watcher.getAttr(std::string("filter")));
        std::string s = (std::string)name;
        this->setFilter(s.c_str());
    }
}

TaskWatcherPython::~TaskWatcherPython()
{
    std::vector< QPointer<QWidget> > guarded;
    guarded.insert(guarded.begin(), Content.begin(), Content.end());
    Content.clear();
    Base::PyGILStateLocker lock;
    this->watcher = Py::None();
    Content.insert(Content.begin(), guarded.begin(), guarded.end());
}

bool TaskWatcherPython::shouldShow()
{
    Base::PyGILStateLocker lock;
    try {
        if (watcher.hasAttr(std::string("shouldShow"))) {
            Py::Callable method(watcher.getAttr(std::string("shouldShow")));
            Py::Tuple args;
            Py::Boolean ret(method.apply(args));
            return (bool)ret;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    if (!this->Filter.empty())
        return match();
    else
        return TaskWatcher::shouldShow();
}

// ------------------------------------------------------------------

TaskDialogPython::TaskDialogPython(const Py::Object& o) : dlg(o)
{
    if (dlg.hasAttr(std::string("ui"))) {
        UiLoader loader;
        loader.setLanguageChangeEnabled(true);
        QString fn, icon;
        Py::String ui(dlg.getAttr(std::string("ui")));
        std::string path = (std::string)ui;
        fn = QString::fromUtf8(path.c_str());

        QFile file(fn);
        QWidget* form = nullptr;
        if (file.open(QFile::ReadOnly))
            form = loader.load(&file, nullptr);
        file.close();
        if (form) {
            Gui::TaskView::TaskBox* taskbox = new Gui::TaskView::TaskBox(
                QPixmap(icon), form->windowTitle(), true, nullptr);
            taskbox->groupLayout()->addWidget(form);
            Content.push_back(taskbox);
        }
        else {
            Base::Console().Error("Failed to load UI file from '%s'\n",
                (const char*)fn.toUtf8());
        }
    }
    else if (dlg.hasAttr(std::string("form"))) {
        Py::Object f(dlg.getAttr(std::string("form")));
        Py::List widgets;
        if (f.isList()) {
            widgets = f;
        }
        else {
            widgets.append(f);
        }

        Gui::PythonWrapper wrap;
        if (wrap.loadCoreModule()) {
            for (Py::List::iterator it = widgets.begin(); it != widgets.end(); ++it) {
                QObject* object = wrap.toQObject(*it);
                if (object) {
                    QWidget* form = qobject_cast<QWidget*>(object);
                    if (form) {
                        Gui::TaskView::TaskBox* taskbox = new Gui::TaskView::TaskBox(
                            form->windowIcon().pixmap(32), form->windowTitle(), true, nullptr);
                        taskbox->groupLayout()->addWidget(form);
                        Content.push_back(taskbox);
                    }
                }
            }
        }
    }
}

TaskDialogPython::~TaskDialogPython()
{
    std::vector< QPointer<QWidget> > guarded;
    guarded.insert(guarded.begin(), Content.begin(), Content.end());
    Content.clear();

    Base::PyGILStateLocker lock;
    clearForm();

    // Assigning None to 'dlg' may destroy some of the stored widgets.
    // By guarding them with QPointer their pointers will be set to null
    // so that the destructor of the base class can reliably call 'delete'.
    Content.insert(Content.begin(), guarded.begin(), guarded.end());
}

void TaskDialogPython::clearForm()
{
    try {
        // The widgets stored in the 'form' attribute will be deleted.
        // Thus, set this attribute to None to make sure that when using
        // the same dialog instance for a task panel won't segfault.
        if (this->dlg.hasAttr(std::string("form"))) {
            this->dlg.setAttr(std::string("form"), Py::None());
        }
        this->dlg = Py::None();
    }
    catch (Py::AttributeError& e) {
        e.clear();
    }
}

void TaskDialogPython::open()
{
    Base::PyGILStateLocker lock;
    try {
        if (dlg.hasAttr(std::string("open"))) {
            Py::Callable method(dlg.getAttr(std::string("open")));
            Py::Tuple args;
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void TaskDialogPython::clicked(int i)
{
    Base::PyGILStateLocker lock;
    try {
        if (dlg.hasAttr(std::string("clicked"))) {
            Py::Callable method(dlg.getAttr(std::string("clicked")));
            Py::Tuple args(1);
            args.setItem(0, Py::Int(i));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

bool TaskDialogPython::accept()
{
    Base::PyGILStateLocker lock;
    try {
        if (dlg.hasAttr(std::string("accept"))) {
            Py::Callable method(dlg.getAttr(std::string("accept")));
            Py::Tuple args;
            Py::Boolean ret(method.apply(args));
            return (bool)ret;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return TaskDialog::accept();
}

bool TaskDialogPython::reject()
{
    Base::PyGILStateLocker lock;
    try {
        if (dlg.hasAttr(std::string("reject"))) {
            Py::Callable method(dlg.getAttr(std::string("reject")));
            Py::Tuple args;
            Py::Boolean ret(method.apply(args));
            return (bool)ret;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return TaskDialog::reject();
}

void TaskDialogPython::helpRequested()
{
    Base::PyGILStateLocker lock;
    try {
        if (dlg.hasAttr(std::string("helpRequested"))) {
            Py::Callable method(dlg.getAttr(std::string("helpRequested")));
            Py::Tuple args;
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

QDialogButtonBox::StandardButtons TaskDialogPython::getStandardButtons(void) const
{
    Base::PyGILStateLocker lock;
    try {
        if (dlg.hasAttr(std::string("getStandardButtons"))) {
            Py::Callable method(dlg.getAttr(std::string("getStandardButtons")));
            Py::Tuple args;
            Py::Int ret(method.apply(args));
            int value = (int)ret;
            return QDialogButtonBox::StandardButtons(value);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return TaskDialog::getStandardButtons();
}

void TaskDialogPython::modifyStandardButtons(QDialogButtonBox *buttonBox)
{
    Base::PyGILStateLocker lock;
    try {
        if (dlg.hasAttr(std::string("modifyStandardButtons"))) {
            Gui::PythonWrapper wrap;
            wrap.loadGuiModule();
            wrap.loadWidgetsModule();
            Py::Callable method(dlg.getAttr(std::string("modifyStandardButtons")));
            Py::Tuple args(1);
            args.setItem(0, wrap.fromQWidget(buttonBox, "QDialogButtonBox"));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

bool TaskDialogPython::isAllowedAlterDocument(void) const
{
    Base::PyGILStateLocker lock;
    try {
        if (dlg.hasAttr(std::string("isAllowedAlterDocument"))) {
            Py::Callable method(dlg.getAttr(std::string("isAllowedAlterDocument")));
            Py::Tuple args;
            Py::Boolean ret(method.apply(args));
            return (bool)ret;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return TaskDialog::isAllowedAlterDocument();
}

bool TaskDialogPython::isAllowedAlterView(void) const
{
    Base::PyGILStateLocker lock;
    try {
        if (dlg.hasAttr(std::string("isAllowedAlterView"))) {
            Py::Callable method(dlg.getAttr(std::string("isAllowedAlterView")));
            Py::Tuple args;
            Py::Boolean ret(method.apply(args));
            return (bool)ret;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return TaskDialog::isAllowedAlterView();
}

bool TaskDialogPython::isAllowedAlterSelection(void) const
{
    Base::PyGILStateLocker lock;
    try {
        if (dlg.hasAttr(std::string("isAllowedAlterSelection"))) {
            Py::Callable method(dlg.getAttr(std::string("isAllowedAlterSelection")));
            Py::Tuple args;
            Py::Boolean ret(method.apply(args));
            return (bool)ret;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return TaskDialog::isAllowedAlterSelection();
}

bool TaskDialogPython::needsFullSpace() const
{
    Base::PyGILStateLocker lock;
    try {
        if (dlg.hasAttr(std::string("needsFullSpace"))) {
            Py::Callable method(dlg.getAttr(std::string("needsFullSpace")));
            Py::Tuple args;
            Py::Boolean ret(method.apply(args));
            return (bool)ret;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return TaskDialog::needsFullSpace();
}

