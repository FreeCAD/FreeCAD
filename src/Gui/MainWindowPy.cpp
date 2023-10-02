/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <list>
#endif

#include <Base/TypePy.h>

#include "DocumentPy.h"
#include "MainWindowPy.h"
#include "MainWindow.h"
#include "MDIView.h"
#include "MDIViewPy.h"
#include "MDIViewPyWrap.h"
#include "PythonWrapper.h"


using namespace Gui;


void MainWindowPy::init_type()
{
    behaviors().name("MainWindowPy");
    behaviors().doc("Python binding class for the MainWindow class");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();
    behaviors().set_tp_new(extension_object_new);

    add_varargs_method("getWindows",&MainWindowPy::getWindows,"getWindows()");
    add_varargs_method("getWindowsOfType",&MainWindowPy::getWindowsOfType,"getWindowsOfType(typeid)");
    add_varargs_method("setActiveWindow", &MainWindowPy::setActiveWindow, "setActiveWindow(MDIView)");
    add_varargs_method("getActiveWindow", &MainWindowPy::getActiveWindow, "getActiveWindow()");
    add_varargs_method("addWindow", &MainWindowPy::addWindow, "addWindow(MDIView)");
    add_varargs_method("removeWindow", &MainWindowPy::removeWindow, "removeWindow(MDIView)");
}

PyObject *MainWindowPy::extension_object_new(struct _typeobject * /*type*/, PyObject * /*args*/, PyObject * /*kwds*/)
{
    return new MainWindowPy(nullptr);
}

Py::Object MainWindowPy::type()
{
    return Py::Object( reinterpret_cast<PyObject *>( behaviors().type_object() ) );
}

Py::ExtensionObject<MainWindowPy> MainWindowPy::create(MainWindow *mw)
{
    Py::Callable class_type(type());
    Py::Tuple arg;
    auto inst = Py::ExtensionObject<MainWindowPy>(class_type.apply(arg, Py::Dict()));
    inst.extensionObject()->_mw = mw;
    return inst;
}

Py::Object MainWindowPy::createWrapper(MainWindow *mw)
{
    PythonWrapper wrap;
    if (!wrap.loadCoreModule() ||
        !wrap.loadGuiModule() ||
        !wrap.loadWidgetsModule()) {
        throw Py::RuntimeError("Failed to load Python wrapper for Qt");
    }

    // copy attributes
    std::list<std::string> attr = {"getWindows", "getWindowsOfType", "setActiveWindow", "getActiveWindow", "addWindow", "removeWindow"};

    Py::Object py = wrap.fromQWidget(mw, "QMainWindow");
    Py::ExtensionObject<MainWindowPy> inst(create(mw));
    for (const auto& it : attr) {
        py.setAttr(it, inst.getAttr(it));
    }
    return py;
}

MainWindowPy::MainWindowPy(MainWindow *mw)
  : _mw(mw)
{
}

MainWindowPy::~MainWindowPy()
{
    // in case the class is instantiated on the stack
    ob_refcnt = 0;
}

Py::Object MainWindowPy::repr()
{
    std::string s;
    std::ostringstream s_out;
    if (!_mw)
        throw Py::RuntimeError("Cannot print representation of deleted object");
    s_out << "MainWindow";
    return Py::String(s_out.str());
}

Py::Object MainWindowPy::getWindows(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    Py::List mdis;
    if (_mw) {
        QList<QWidget*> windows = _mw->windows();
        for (auto it : windows) {
            auto view = qobject_cast<MDIView*>(it);
            if (view) {
                mdis.append(Py::asObject(view->getPyObject()));
            }
        }
    }

    return mdis;
}

Py::Object MainWindowPy::getWindowsOfType(const Py::Tuple& args)
{
    PyObject* t;
    if (!PyArg_ParseTuple(args.ptr(), "O!", &Base::TypePy::Type, &t))
        throw Py::Exception();

    Base::Type typeId = *static_cast<Base::TypePy*>(t)->getBaseTypePtr();

    Py::List mdis;
    if (_mw) {
        QList<QWidget*> windows = _mw->windows();
        for (auto it : windows) {
            auto view = qobject_cast<MDIView*>(it);
            if (view && view->isDerivedFrom(typeId)) {
                mdis.append(Py::asObject(view->getPyObject()));
            }
        }
    }

    return mdis;
}

Py::Object MainWindowPy::setActiveWindow(const Py::Tuple& args)
{
    Py::ExtensionObject<MDIViewPy> mdi(args[0].callMemberFunction("cast_to_base"));
    if (_mw) {
        _mw->setActiveWindow(mdi.extensionObject()->getMDIViewPtr());
    }

    return Py::None();
}

Py::Object MainWindowPy::getActiveWindow(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    if (_mw) {
        MDIView* mdi = _mw->activeWindow();
        if (mdi) {
            return Py::asObject(mdi->getPyObject());
        }
    }
    return Py::None();
}

Py::Object MainWindowPy::addWindow(const Py::Tuple& args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args.ptr(), "O", &obj))
        throw Py::Exception();

    if (_mw) {
        Py::Object py(obj);
        Gui::Document* document{nullptr};
        // Check if the py object has a reference to a Gui document
        if (py.hasAttr("document")) {
            Py::Object attr(py.getAttr("document"));
            if (PyObject_TypeCheck(attr.ptr(), &DocumentPy::Type)) {
                document = static_cast<DocumentPy*>(attr.ptr())->getDocumentPtr();
            }
        }

        MDIViewPyWrap* mdi = new MDIViewPyWrap(py, document);
        _mw->addWindow(mdi);
        return Py::asObject(mdi->getPyObject());
    }
    return Py::None();
}

Py::Object MainWindowPy::removeWindow(const Py::Tuple& args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args.ptr(), "O!", MDIViewPy::type_object(), &obj))
        throw Py::Exception();

    if (_mw) {
        MDIViewPy* mdi = static_cast<MDIViewPy*>(obj);
        _mw->removeWindow(mdi->getMDIViewPtr());
    }
    return Py::None();
}
