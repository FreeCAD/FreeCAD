/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer@users.sourceforge.net>        *
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
# include <memory>
#endif

#include "DocumentProtectorPy.h"
#include "DocumentProtector.h"

#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <App/DocumentPy.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>

using namespace Sandbox;


void DocumentProtectorPy::init_type()
{
    behaviors().name("DocumentProtectorPy");
    behaviors().doc("Python binding class for the document protector class");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();

    add_varargs_method("addObject",&DocumentProtectorPy::addObject,"addObject(type,name)");
    add_varargs_method("recompute",&DocumentProtectorPy::recompute,"recompute()");
}

DocumentProtectorPy::DocumentProtectorPy(App::DocumentPy *doc)
{
    _dp = new DocumentProtector(doc->getDocumentPtr());
}

DocumentProtectorPy::~DocumentProtectorPy()
{
    delete _dp;
}

Py::Object DocumentProtectorPy::repr()
{
    std::string s;
    std::ostringstream s_out;
    if (!_dp)
        throw Py::RuntimeError("Cannot print representation of deleted object");
    s_out << "Document protector";
    return Py::String(s_out.str());
}

DocumentProtectorPy::method_varargs_handler DocumentProtectorPy::pycxx_handler = 0;

PyObject *DocumentProtectorPy::method_varargs_ext_handler(PyObject *_self_and_name_tuple, PyObject *_args)
{
    try {
        return pycxx_handler(_self_and_name_tuple, _args);
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch (const std::exception& e) {
        throw Py::RuntimeError(e.what());
    }
    catch(...) {
        throw Py::RuntimeError("Unknown C++ exception");
    }
}

Py::Object DocumentProtectorPy::getattr(const char * attr)
{
    if (!_dp) {
        std::string s;
        std::ostringstream s_out;
        s_out << "Cannot access attribute '" << attr << "' of deleted object";
        throw Py::RuntimeError(s_out.str());
    }
    else {
        Py::Object obj = Py::PythonExtension<DocumentProtectorPy>::getattr(attr);
        if (PyCFunction_Check(obj.ptr())) {
            PyCFunctionObject* op = reinterpret_cast<PyCFunctionObject*>(obj.ptr());
            if (!pycxx_handler)
                pycxx_handler = op->m_ml->ml_meth;
            op->m_ml->ml_meth = method_varargs_ext_handler;
        }
        return obj;
    }
}

int DocumentProtectorPy::setattr(const char * attr, const Py::Object & value)
{
    if (!_dp) {
        std::string s;
        std::ostringstream s_out;
        s_out << "Cannot access attribute '" << attr << "' of deleted object";
        throw Py::RuntimeError(s_out.str());
    }
    else {
        Base::PyGILStateRelease unlock;
        return Py::PythonExtension<DocumentProtectorPy>::setattr(attr, value);
    }
}

Py::Object DocumentProtectorPy::addObject(const Py::Tuple& args)
{
    char* type;
    char* name=0;
    if (!PyArg_ParseTuple(args.ptr(), "s|s",&type, &name))
        throw Py::Exception();
    Base::PyGILStateRelease unlock;
    if (!name)
        name = type;
    App::DocumentObject* obj = _dp->addObject(type, name);
    if (!obj) {
        std::string s;
        std::ostringstream s_out;
        s_out << "Couldn't create an object of type '" << type << "'";
        throw Py::RuntimeError(s_out.str());
    }
    //return Py::asObject(obj->getPyObject());
    return Py::asObject(new DocumentObjectProtectorPy(obj));
}

Py::Object DocumentProtectorPy::recompute(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    Base::PyGILStateRelease unlock;
    _dp->recompute();
    return Py::None();
}

// ----------------------------------------------------------------------------

void DocumentObjectProtectorPy::init_type()
{
    behaviors().name("DocumentObjectProtectorPy");
    behaviors().doc("Python binding class for the document object protector class");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();

    add_varargs_method("purgeTouched",&DocumentObjectProtectorPy::purgeTouched,"purgeTouched()");
}

DocumentObjectProtectorPy::DocumentObjectProtectorPy(App::DocumentObject *obj)
{
    _dp = new DocumentObjectProtector(obj);
}

DocumentObjectProtectorPy::DocumentObjectProtectorPy(App::DocumentObjectPy *obj)
{
    _dp = new DocumentObjectProtector(obj->getDocumentObjectPtr());
}

DocumentObjectProtectorPy::~DocumentObjectProtectorPy()
{
    delete _dp;
}

Py::Object DocumentObjectProtectorPy::getObject() const
{
    App::DocumentObject* obj = _dp->getObject();
    PyObject* py = obj->getPyObject();
    return Py::Object(py, true);
}

Py::Object DocumentObjectProtectorPy::repr()
{
    std::string s;
    std::ostringstream s_out;
    if (!_dp)
        throw Py::RuntimeError("Cannot print representation of deleted object");
    s_out << "Document object protector";
    return Py::String(s_out.str());
}

Py::Object DocumentObjectProtectorPy::getattr(const char * attr)
{
    if (!_dp) {
        std::string s;
        std::ostringstream s_out;
        s_out << "Cannot access attribute '" << attr << "' of deleted object";
        throw Py::RuntimeError(s_out.str());
    }
    else {
        App::DocumentObject* obj = _dp->getObject();
        App::Property* prop = obj->getPropertyByName(attr);
        if (!prop) {
            return Py::PythonExtension<DocumentObjectProtectorPy>::getattr(attr);
            //std::string s;
            //std::ostringstream s_out;
            //s_out << "No such attribute '" << attr << "'";
            //throw Py::AttributeError(s_out.str());
        }

        return Py::asObject(prop->getPyObject());
    }
}

int DocumentObjectProtectorPy::setattr(const char * attr, const Py::Object & value)
{
    if (!_dp) {
        std::string s;
        std::ostringstream s_out;
        s_out << "Cannot access attribute '" << attr << "' of deleted object";
        throw Py::RuntimeError(s_out.str());
    }
    else {
        App::DocumentObject* obj = _dp->getObject();
        App::Property* prop = obj->getPropertyByName(attr);
        if (!prop) {
            std::string s;
            std::ostringstream s_out;
            s_out << "No such attribute '" << attr << "'";
            throw Py::AttributeError(s_out.str());
        }
        Base::PyGILStateRelease unlock;
        std::unique_ptr<App::Property> copy(static_cast<App::Property*>
            (prop->getTypeId().createInstance()));
        if (PyObject_TypeCheck(value.ptr(), DocumentObjectProtectorPy::type_object())) {
            copy->setPyObject(static_cast<const DocumentObjectProtectorPy*>(value.ptr())->getObject().ptr());
        }
        else {
            copy->setPyObject(value.ptr());
        }
        return _dp->setProperty(attr, *copy) ? 0 : -1;
    }
}

Py::Object DocumentObjectProtectorPy::purgeTouched(const Py::Tuple&)
{
    _dp->purgeTouched();
    return Py::None();
}
