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

#include "MDIViewPy.h"
#include "Application.h"
#include "Document.h"

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/PlacementPy.h>
#include <Base/Rotation.h>
#include <Base/RotationPy.h>
#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <App/GeoFeature.h>
#include <CXX/Objects.hxx>

using namespace Gui;


void MDIViewPy::init_type()
{
    behaviors().name("MDIViewPy");
    behaviors().doc("Python binding class for the MDI view class");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();

    add_varargs_method("message",&MDIViewPy::message,"message()");
    add_varargs_method("fitAll",&MDIViewPy::fitAll,"fitAll()");
    add_varargs_method("setActiveObject", &MDIViewPy::setActiveObject, "setActiveObject(name,object,subname=None)\nadd or set a new active object");
    add_varargs_method("getActiveObject", &MDIViewPy::getActiveObject, "getActiveObject(name,resolve=True)\nreturns the active object for the given type");
}

MDIViewPy::MDIViewPy(MDIView *mdi)
  : _view(mdi)
{
}

MDIViewPy::~MDIViewPy()
{
}

Py::Object MDIViewPy::repr()
{
    std::string s;
    std::ostringstream s_out;
    if (!_view)
        throw Py::RuntimeError("Cannot print representation of deleted object");
    s_out << "MDIView";
    return Py::String(s_out.str());
}

Py::Object MDIViewPy::message(const Py::Tuple& args)
{
    const char **ppReturn = 0;
    char *psMsgStr;
    if (!PyArg_ParseTuple(args.ptr(), "s;Message string needed (string)",&psMsgStr))     // convert args: Python->C
        throw Py::Exception();

    try {
        if (_view)
            _view->onMsg(psMsgStr,ppReturn);
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
    char *subname = 0;
    if (!PyArg_ParseTuple(args.ptr(), "s|Os", &name, &docObject, &subname))
        throw Py::Exception();

    if (_view) {
        if (docObject == Py_None) {
            _view->setActiveObject(0, name);
        }
        else {
            if (!PyObject_TypeCheck(docObject, &App::DocumentObjectPy::Type))
                throw Py::TypeError("Expect the second argument to be a document object or None");

            App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(docObject)->getDocumentObjectPtr();
            _view->setActiveObject(obj, name, subname);
        }
    }

    return Py::None();
}

Py::Object MDIViewPy::getActiveObject(const Py::Tuple& args)
{
    const char* name;
    PyObject *resolve = Py_True;
    if (!PyArg_ParseTuple(args.ptr(), "s|O", &name,&resolve))
        throw Py::Exception();

    App::DocumentObject *parent = nullptr;
    std::string subname;
    App::DocumentObject* obj = nullptr;
    if (_view)
        obj = _view->getActiveObject<App::DocumentObject*>(name,&parent,&subname);
    if (!obj)
        return Py::None();

    if (PyObject_IsTrue(resolve))
        return Py::asObject(obj->getPyObject());

    return Py::TupleN(
            Py::asObject(obj->getPyObject()),
            Py::asObject(parent->getPyObject()),
            Py::String(subname.c_str()));
}
