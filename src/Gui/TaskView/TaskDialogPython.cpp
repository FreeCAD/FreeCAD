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
# include <QPointer>
#endif

#include "TaskDialogPython.h"
#include "TaskView.h"

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/WidgetFactory.h>
#include <Base/Interpreter.h>
#include <Base/Console.h>
#include <CXX/Objects.hxx>

using namespace Gui;
using namespace Gui::TaskView;

ControlPy* ControlPy::instance = 0;

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
    add_varargs_method("showDialog",&ControlPy::showDialog,"showDialog()");
    add_varargs_method("activeDialog",&ControlPy::activeDialog,"activeDialog()");
    add_varargs_method("closeDialog",&ControlPy::closeDialog,"closeDialog()");
    add_varargs_method("addTaskWatcher",&ControlPy::addTaskWatcher,"addTaskWatcher()");
    add_varargs_method("clearTaskWatcher",&ControlPy::clearTaskWatcher,"clearTaskWatcher()");
    add_varargs_method("isAllowedAlterDocument",&ControlPy::isAllowedAlterDocument,"isAllowedAlterDocument()");
    add_varargs_method("isAllowedAlterView",&ControlPy::isAllowedAlterView,"isAllowedAlterView()");
    add_varargs_method("isAllowedAlterSelection",&ControlPy::isAllowedAlterSelection,"isAllowedAlterSelection()");
    add_varargs_method("showTaskView",&ControlPy::showTaskView,"showTaskView()");
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
    Gui::TaskView::TaskDialog* act = Gui::Control().activeDialog();
    if (act)
        throw Py::Exception("Active task dialog found");
    TaskDialogPython* dlg = new TaskDialogPython(args[0]);
    Gui::Control().showDialog(dlg);
    return Py::None();
}

Py::Object ControlPy::activeDialog(const Py::Tuple& args)
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    return Py::Boolean(dlg!=0);
}

Py::Object ControlPy::closeDialog(const Py::Tuple&)
{
    Gui::Control().closeDialog();
    return Py::None();
}

Py::Object ControlPy::addTaskWatcher(const Py::Tuple& args)
{
    std::vector<Gui::TaskView::TaskWatcher*> watcher;
    Py::Sequence list(args[0]);
    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
        TaskWatcherPython* w = new TaskWatcherPython(*it);
        watcher.push_back(w);
    }

    Gui::TaskView::TaskView* taskView = Gui::Control().taskPanel();
    if (taskView)
        taskView->addTaskWatcher(watcher);
    return Py::None();
}

Py::Object ControlPy::clearTaskWatcher(const Py::Tuple&)
{
    Gui::TaskView::TaskView* taskView = Gui::Control().taskPanel();
    if (taskView)
        taskView->clearTaskWatcher();
    return Py::None();
}

Py::Object ControlPy::isAllowedAlterDocument(const Py::Tuple&)
{
    bool ok = Gui::Control().isAllowedAlterDocument();
    return Py::Boolean(ok);
}

Py::Object ControlPy::isAllowedAlterView(const Py::Tuple&)
{
    bool ok = Gui::Control().isAllowedAlterView();
    return Py::Boolean(ok);
}

Py::Object ControlPy::isAllowedAlterSelection(const Py::Tuple&)
{
    bool ok = Gui::Control().isAllowedAlterSelection();
    return Py::Boolean(ok);
}

Py::Object ControlPy::showTaskView(const Py::Tuple&)
{
    Gui::Control().showTaskView();
    return Py::None();
}

// ------------------------------------------------------------------

TaskWatcherPython::TaskWatcherPython(const Py::Object& o)
  : TaskWatcher(0), watcher(o)
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

    Gui::TaskView::TaskBox *tb = 0;
    if (watcher.hasAttr(std::string("commands"))) {
        if (!tb) tb = new Gui::TaskView::TaskBox(icon, title, true, 0);
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
            tb = new Gui::TaskView::TaskBox(icon, title, true, 0);
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
            Py::Tuple args(0);
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
#if QT_VERSION >= 0x040500
        loader.setLanguageChangeEnabled(true);
#endif
        QString fn, icon;
        Py::String ui(dlg.getAttr(std::string("ui")));
        std::string path = (std::string)ui;
        fn = QString::fromUtf8(path.c_str());

        QFile file(fn);
        QWidget* form = 0;
        if (file.open(QFile::ReadOnly))
            form = loader.load(&file, 0);
        file.close();
        if (form) {
            Gui::TaskView::TaskBox* taskbox = new Gui::TaskView::TaskBox(
                QPixmap(icon), form->windowTitle(), true, 0);
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
                            form->windowIcon().pixmap(32), form->windowTitle(), true, 0);
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
    this->dlg = Py::None();
    Content.insert(Content.begin(), guarded.begin(), guarded.end());
}

void TaskDialogPython::open()
{
    Base::PyGILStateLocker lock;
    try {
        if (dlg.hasAttr(std::string("open"))) {
            Py::Callable method(dlg.getAttr(std::string("open")));
            Py::Tuple args(0);
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
            Py::Tuple args(0);
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
            Py::Tuple args(0);
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
            Py::Tuple args(0);
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
            Py::Tuple args(0);
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

void TaskDialogPython::modifyStandardButtons(QDialogButtonBox*)
{
}

bool TaskDialogPython::isAllowedAlterDocument(void) const
{
    Base::PyGILStateLocker lock;
    try {
        if (dlg.hasAttr(std::string("isAllowedAlterDocument"))) {
            Py::Callable method(dlg.getAttr(std::string("isAllowedAlterDocument")));
            Py::Tuple args(0);
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
            Py::Tuple args(0);
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
            Py::Tuple args(0);
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
            Py::Tuple args(0);
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

