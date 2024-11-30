/***************************************************************************
 *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <Base/Exception.h>

#include "MDIViewPy.h"
#include "MDIView.h"


using namespace Gui;


void MDIViewPy::init_type()
{
    behaviors().name("MDIViewPy");
    behaviors().doc("Python binding class for the MDI view class");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();
    behaviors().set_tp_new(extension_object_new);

    add_varargs_method("printView",&MDIViewPy::printView,"printView()");
    add_varargs_method("printPdf",&MDIViewPy::printPdf,"printPdf()");
    add_varargs_method("printPreview",&MDIViewPy::printPreview,"printPreview()");

    add_varargs_method("undoActions",&MDIViewPy::undoActions,"undoActions()");
    add_varargs_method("redoActions",&MDIViewPy::redoActions,"redoActions()");

    add_varargs_method("message",&MDIViewPy::sendMessage,"deprecated: use sendMessage");
    add_varargs_method("sendMessage",&MDIViewPy::sendMessage,"sendMessage(str)");
    add_varargs_method("supportMessage",&MDIViewPy::supportMessage,"supportMessage(str)");
    add_varargs_method("fitAll",&MDIViewPy::fitAll,"fitAll()");
    add_varargs_method("setActiveObject", &MDIViewPy::setActiveObject, "setActiveObject(name,object,subname=None)\nadd or set a new active object");
    add_varargs_method("getActiveObject", &MDIViewPy::getActiveObject, "getActiveObject(name,resolve=True)\nreturns the active object for the given type");
    add_varargs_method("cast_to_base", &MDIViewPy::cast_to_base, "cast_to_base() cast to MDIView class");
}

PyObject *MDIViewPy::extension_object_new(struct _typeobject * /*type*/, PyObject * /*args*/, PyObject * /*kwds*/)
{
    return new MDIViewPy(nullptr);
}

Py::Object MDIViewPy::type()
{
    return Py::Object( reinterpret_cast<PyObject *>( behaviors().type_object() ) );
}

Py::ExtensionObject<MDIViewPy> MDIViewPy::create(MDIView *mdi)
{
    Py::Callable class_type(type());
    Py::Tuple arg;
    auto inst = Py::ExtensionObject<MDIViewPy>(class_type.apply(arg, Py::Dict()));
    inst.extensionObject()->_view = mdi;
    return inst;
}

MDIViewPy::MDIViewPy(MDIView *mdi)
  : _view(mdi)
{
}

MDIViewPy::~MDIViewPy()
{
    // in case the class is instantiated on the stack
    ob_refcnt = 0;
}

Py::Object MDIViewPy::repr()
{
    std::string s;
    std::ostringstream s_out;
    if (!_view)
        throw Py::RuntimeError("Cannot print representation of deleted object");
    s_out << _view->getTypeId().getName();
    return Py::String(s_out.str());
}

Py::Object MDIViewPy::printView(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    if (_view)
        _view->print();

    return Py::None();
}

Py::Object MDIViewPy::printPdf(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    if (_view)
        _view->printPdf();

    return Py::None();
}

Py::Object MDIViewPy::printPreview(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    if (_view)
        _view->printPreview();

    return Py::None();
}

Py::Object MDIViewPy::undoActions(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    Py::List list;
    if (_view) {
        QStringList undo = _view->undoActions();
        for (const auto& it : undo)
            list.append(Py::String(it.toStdString()));
    }

    return list;
}

Py::Object MDIViewPy::redoActions(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    Py::List list;
    if (_view) {
        QStringList redo = _view->redoActions();
        for (const auto& it : redo)
            list.append(Py::String(it.toStdString()));
    }

    return list;
}

Py::Object MDIViewPy::sendMessage(const Py::Tuple& args)
{
    const char **ppReturn = nullptr;
    char *psMsgStr;
    if (!PyArg_ParseTuple(args.ptr(), "s;Message string needed (string)",&psMsgStr))
        throw Py::Exception();

    try {
        bool ok = false;
        if (_view)
            ok = _view->onMsg(psMsgStr,ppReturn);
        return Py::Boolean(ok);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object MDIViewPy::supportMessage(const Py::Tuple& args)
{
    char *psMsgStr;
    if (!PyArg_ParseTuple(args.ptr(), "s;Message string needed (string)",&psMsgStr))
        throw Py::Exception();

    try {
        bool ok = false;
        if (_view)
            _view->onHasMsg(psMsgStr);
        return Py::Boolean(ok);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object MDIViewPy::fitAll(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        if (_view)
            _view->viewAll();
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
    return Py::None();
}

Py::Object MDIViewPy::setActiveObject(const Py::Tuple& args)
{
    PyObject* docObject = Py_None;
    char* name;
    char *subname = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "s|Os", &name, &docObject, &subname))
        throw Py::Exception();

    try {
        Base::PyTypeCheck(&docObject, &App::DocumentObjectPy::Type,
            "Expect the second argument to be a document object or None");
        if (_view) {
            App::DocumentObject* obj = docObject ?
                static_cast<App::DocumentObjectPy*>(docObject)->getDocumentObjectPtr() : nullptr;
            _view->setActiveObject(obj, name, subname);
        }
    }
    catch (const Base::Exception& e) {
        throw Py::Exception(e.getPyExceptionType(), e.what());
    }

    return Py::None();
}

Py::Object MDIViewPy::getActiveObject(const Py::Tuple& args)
{
    const char* name{};
    PyObject *resolve = Py_True; // NOLINT
    if (!PyArg_ParseTuple(args.ptr(), "s|O!", &name, &PyBool_Type, &resolve)) {
        throw Py::Exception();
    }

    App::DocumentObject *parent = nullptr;
    std::string subname;
    App::DocumentObject* obj = nullptr;
    if (_view) {
        obj = _view->getActiveObject<App::DocumentObject*>(name,&parent,&subname);
    }

    if (Base::asBoolean(resolve)) {
        if (obj) {
            return Py::asObject(obj->getPyObject());
        }

        return Py::None();
    }

    // NOLINTBEGIN(cppcoreguidelines-slicing)
    if (obj) {
        return Py::TupleN(
                Py::asObject(obj->getPyObject()),
                Py::asObject(parent->getPyObject()),
                Py::String(subname.c_str()));
    }

    return Py::TupleN(Py::None(), Py::None(), Py::String());
    // NOLINTEND(cppcoreguidelines-slicing)
}

Py::Object MDIViewPy::cast_to_base(const Py::Tuple&)
{
    return Py::Object(this);
}
