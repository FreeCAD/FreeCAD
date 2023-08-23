/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
#endif

#include <App/DocumentObjectPy.h>
#include <Base/Interpreter.h>
#include <Base/MatrixPy.h>
#include <Base/Tools.h>

#include "FeaturePython.h"
#include "FeaturePythonPyImp.h"


using namespace App;

FeaturePythonImp::FeaturePythonImp(App::DocumentObject* o)
    : object(o)
{
}

FeaturePythonImp::~FeaturePythonImp()
{
    Base::PyGILStateLocker lock;
#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name) py_##_name = Py::None();

    try {
        FC_PY_FEATURE_PYTHON
    }
    catch (Py::Exception& e) {
        e.clear();
    }
}

void FeaturePythonImp::init(PyObject *pyobj) {
    Base::PyGILStateLocker lock;
    has__object__ = !!PyObject_HasAttrString(pyobj, "__object__");

#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name) FC_PY_ELEMENT_INIT(_name)

    FC_PY_FEATURE_PYTHON
}

#define FC_PY_CALL_CHECK(_name) _FC_PY_CALL_CHECK(_name,return(false))

/*!
 Calls the execute() method of the Python feature class. If the Python feature class doesn't have an execute()
 method or if it returns False this method also return false and true otherwise.
 */
bool FeaturePythonImp::execute()
{
    FC_PY_CALL_CHECK(execute)
    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            Py::Object res = Base::pyCall(py_execute.ptr());
            if (res.isBoolean() && !res.isTrue())
                return false;
            return true;
        }
        else {
            Py::Tuple args(1);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            Py::Object res = Base::pyCall(py_execute.ptr(),args.ptr());
            if (res.isBoolean() && !res.isTrue())
                return false;
            return true;
        }
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return false;
        }
        Base::PyException::ThrowException(); // extract the Python error text
    }

    return false;
}

bool FeaturePythonImp::mustExecute() const
{
    FC_PY_CALL_CHECK(mustExecute)
    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            Py::Object res(Base::pyCall(py_mustExecute.ptr()));
            return res.isTrue();
        }
        else {
            Py::Tuple args(1);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            Py::Object res(Base::pyCall(py_mustExecute.ptr(),args.ptr()));
            return res.isTrue();
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return false;
}


void FeaturePythonImp::onBeforeChange(const Property* prop)
{
    if (py_onBeforeChange.isNone())
        return;

    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        const char *prop_name = object->getPropertyName(prop);
        if (!prop_name)
            return;
        if (has__object__) {
            Py::Tuple args(1);
            args.setItem(0, Py::String(prop_name));
            Base::pyCall(py_onBeforeChange.ptr(),args.ptr());
        }
        else {
            Py::Tuple args(2);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            args.setItem(1, Py::String(prop_name));
            Base::pyCall(py_onBeforeChange.ptr(),args.ptr());
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

bool FeaturePythonImp::onBeforeChangeLabel(std::string &newLabel)
{
    if(py_onBeforeChangeLabel.isNone())
        return false;

    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(2);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        args.setItem(1,Py::String(newLabel));
        Py::Object ret(Base::pyCall(py_onBeforeChangeLabel.ptr(),args.ptr()));
        if(!ret.isNone()) {
            if(!ret.isString())
                throw Py::TypeError("onBeforeChangeLabel expects to return a string");
            newLabel = ret.as_string();
            return true;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return false;
}

void FeaturePythonImp::onChanged(const Property* prop)
{
    if (py_onChanged.isNone())
        return;
    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        const char *prop_name = object->getPropertyName(prop);
        if (!prop_name)
            return;
        if (has__object__) {
            Py::Tuple args(1);
            args.setItem(0, Py::String(prop_name));
            Base::pyCall(py_onChanged.ptr(),args.ptr());
        }
        else {
            Py::Tuple args(2);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            args.setItem(1, Py::String(prop_name));
            Base::pyCall(py_onChanged.ptr(),args.ptr());
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void FeaturePythonImp::onDocumentRestored()
{
    _FC_PY_CALL_CHECK(onDocumentRestored,return);

    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            Base::pyCall(py_onDocumentRestored.ptr());
        }
        else {
            Py::Tuple args(1);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            Base::pyCall(py_onDocumentRestored.ptr(),args.ptr());
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void FeaturePythonImp::unsetupObject()
{
    _FC_PY_CALL_CHECK(unsetupObject, return);

    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            Base::pyCall(py_unsetupObject.ptr());
        }
        else {
            Py::Tuple args(1);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            Base::pyCall(py_unsetupObject.ptr(), args.ptr());
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

bool FeaturePythonImp::getSubObject(DocumentObject *&ret, const char *subname,
    PyObject **pyObj, Base::Matrix4D *_mat, bool transform, int depth) const
{
    FC_PY_CALL_CHECK(getSubObject);
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(6);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        if(!subname) subname = "";
        args.setItem(1,Py::String(subname));
        args.setItem(2,Py::Int(pyObj?2:1));
        Base::MatrixPy *pyMat = new Base::MatrixPy(new Base::Matrix4D);
        if(_mat) *pyMat->getMatrixPtr() = *_mat;
        args.setItem(3,Py::asObject(pyMat));
        args.setItem(4,Py::Boolean(transform));
        args.setItem(5,Py::Int(depth));

        Py::Object res(Base::pyCall(py_getSubObject.ptr(),args.ptr()));
        if(res.isNone()) {
            ret = nullptr;
            return true;
        }
        if(!res.isTrue())
            return false;
        if(!res.isSequence())
            throw Py::TypeError("getSubObject expects return type of tuple");
        Py::Sequence seq(res);
        if(seq.length() < 2 ||
                (!seq.getItem(0).isNone() &&
                 !PyObject_TypeCheck(seq.getItem(0).ptr(),&DocumentObjectPy::Type)) ||
                !PyObject_TypeCheck(seq.getItem(1).ptr(),&Base::MatrixPy::Type))
        {
            throw Py::TypeError("getSubObject expects return type of (obj,matrix,pyobj)");
        }
        if(_mat)
            *_mat = *static_cast<Base::MatrixPy*>(seq.getItem(1).ptr())->getMatrixPtr();
        if(pyObj) {
            if(seq.length()>2)
                *pyObj = Py::new_reference_to(seq.getItem(2));
            else
                *pyObj = Py::new_reference_to(Py::None());
        }
        if(seq.getItem(0).isNone())
            ret = nullptr;
        else
            ret = static_cast<DocumentObjectPy*>(seq.getItem(0).ptr())->getDocumentObjectPtr();
        return true;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return false;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
        ret = nullptr;
        return true;
    }
}

bool FeaturePythonImp::getSubObjects(std::vector<std::string> &ret, int reason) const {
    FC_PY_CALL_CHECK(getSubObjects);
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(2);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        args.setItem(1, Py::Int(reason));
        Py::Object res(Base::pyCall(py_getSubObjects.ptr(),args.ptr()));
        if(!res.isTrue())
            return true;
        if(!res.isSequence())
            throw Py::TypeError("getSubObjects expects return type of tuple");
        Py::Sequence seq(res);
        for(Py_ssize_t i=0;i<seq.length();++i) {
            Py::Object name(seq[i].ptr());
            if(!name.isString())
                throw Py::TypeError("getSubObjects expects string in returned sequence");
            ret.push_back(name.as_string());
        }
        return true;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return false;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
        return true;
    }
}

bool FeaturePythonImp::getLinkedObject(DocumentObject *&ret, bool recurse,
        Base::Matrix4D *_mat, bool transform, int depth) const
{
    FC_PY_CALL_CHECK(getLinkedObject);
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(5);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        args.setItem(1,Py::Boolean(recurse));
        Base::MatrixPy *pyMat = new Base::MatrixPy(new Base::Matrix4D);
        if(_mat) *pyMat->getMatrixPtr() = *_mat;
        args.setItem(2,Py::asObject(pyMat));
        args.setItem(3,Py::Boolean(transform));
        args.setItem(4,Py::Int(depth));

        Py::Object res(Base::pyCall(py_getLinkedObject.ptr(),args.ptr()));
        if(!res.isTrue()) {
            ret = object;
            return true;
        }
        if(!res.isSequence())
            throw Py::TypeError("getLinkedObject expects return type of (object,matrix)");
        Py::Sequence seq(res);
        if(seq.length() != 2 ||
                (!seq.getItem(0).isNone() &&
                 !PyObject_TypeCheck(seq.getItem(0).ptr(),&DocumentObjectPy::Type)) ||
                !PyObject_TypeCheck(seq.getItem(1).ptr(),&Base::MatrixPy::Type))
        {
            throw Py::TypeError("getLinkedObject expects return type of (object,matrix)");
        }
        if(_mat)
            *_mat = *static_cast<Base::MatrixPy*>(seq.getItem(1).ptr())->getMatrixPtr();
        if(seq.getItem(0).isNone())
            ret = object;
        else
            ret = static_cast<DocumentObjectPy*>(seq.getItem(0).ptr())->getDocumentObjectPtr();
        return true;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return false;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
        ret = nullptr;
        return true;
    }
}

PyObject *FeaturePythonImp::getPyObject()
{
    // ref counter is set to 1
    return new FeaturePythonPyT<DocumentObjectPy>(object);
}

FeaturePythonImp::ValueT
FeaturePythonImp::hasChildElement() const
{
    _FC_PY_CALL_CHECK(hasChildElement,return(NotImplemented));
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        Py::Boolean ok(Base::pyCall(py_hasChildElement.ptr(),args.ptr()));
        return static_cast<bool>(ok) ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }

        Base::PyException e; // extract the Python error text
        e.ReportException();
        return Rejected;
    }
}

int FeaturePythonImp::isElementVisible(const char *element) const {
    _FC_PY_CALL_CHECK(isElementVisible,return(-2));
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(2);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        args.setItem(1,Py::String(element?element:""));
        return Py::Int(Base::pyCall(py_isElementVisible.ptr(),args.ptr()));
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return -2;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
        return -1;
    }
}

int FeaturePythonImp::setElementVisible(const char *element, bool visible) {
    _FC_PY_CALL_CHECK(setElementVisible,return(-2));
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(3);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        args.setItem(1,Py::String(element?element:""));
        args.setItem(2,Py::Boolean(visible));
        return Py::Int(Base::pyCall(py_setElementVisible.ptr(),args.ptr()));
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return -2;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
        return -1;
    }
}

std::string FeaturePythonImp::getViewProviderName()
{
    _FC_PY_CALL_CHECK(getViewProviderName,return(std::string()));
    Base::PyGILStateLocker lock;
    try {
        Py::TupleN args(Py::Object(object->getPyObject(), true));
        Py::String ret(Base::pyCall(py_getViewProviderName.ptr(),args.ptr()));
        return ret.as_string();
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return {};
}

FeaturePythonImp::ValueT
FeaturePythonImp::canLinkProperties() const
{
    _FC_PY_CALL_CHECK(canLinkProperties,return(NotImplemented));
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        Py::Boolean ok(Base::pyCall(py_canLinkProperties.ptr(),args.ptr()));
        return ok ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
        return Rejected;
    }
}

FeaturePythonImp::ValueT
FeaturePythonImp::allowDuplicateLabel() const
{
    _FC_PY_CALL_CHECK(allowDuplicateLabel,return(NotImplemented));
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        Py::Boolean ok(Base::pyCall(py_allowDuplicateLabel.ptr(),args.ptr()));
        return ok ? Accepted : Rejected;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }

        Base::PyException e; // extract the Python error text
        e.ReportException();
        return Rejected;
    }
}

int FeaturePythonImp::canLoadPartial() const {
    _FC_PY_CALL_CHECK(canLoadPartial,return(-1));
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        Py::Int ret(Base::pyCall(py_canLoadPartial.ptr(),args.ptr()));
        return ret;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return -1;
        }
        Base::PyException e; // extract the Python error text
        e.ReportException();
        return 0;
    }
}

FeaturePythonImp::ValueT
FeaturePythonImp::redirectSubName(std::ostringstream &ss,
                                  App::DocumentObject *topParent,
                                  App::DocumentObject *child) const
{
    _FC_PY_CALL_CHECK(redirectSubName,return(NotImplemented));
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(4);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        args.setItem(1,Py::String(ss.str()));
        args.setItem(2,topParent?Py::Object(topParent->getPyObject(),true):Py::Object());
        args.setItem(3,child?Py::Object(child->getPyObject(),true):Py::Object());
        Py::Object ret(Base::pyCall(py_redirectSubName.ptr(),args.ptr()));
        if (ret.isNone())
            return Rejected;
        ss.str("");
        ss << ret.as_string();
        return Accepted;
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return NotImplemented;
        }

        Base::PyException e; // extract the Python error text
        e.ReportException();
        return Rejected;
    }
}

bool FeaturePythonImp::editProperty(const char *name)
{
    _FC_PY_CALL_CHECK(editProperty,return false);
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::String(name));
        Py::Object ret(Base::pyCall(py_editProperty.ptr(),args.ptr()));
        return ret.isTrue();
    }
    catch (Py::Exception&) {
        if (PyErr_ExceptionMatches(PyExc_NotImplementedError)) {
            PyErr_Clear();
            return false;
        }

        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return false;
}

// ---------------------------------------------------------

namespace App {
PROPERTY_SOURCE_TEMPLATE(App::FeaturePython, App::DocumentObject)
template<> const char* App::FeaturePython::getViewProviderName() const {
    return "Gui::ViewProviderPythonFeature";
}
template<> PyObject* App::FeaturePython::getPyObject() {
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<DocumentObjectPy>(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
// explicit template instantiation
template class AppExport FeaturePythonT<DocumentObject>;
}

// ---------------------------------------------------------

namespace App {
PROPERTY_SOURCE_TEMPLATE(App::GeometryPython, App::GeoFeature)
template<> const char* App::GeometryPython::getViewProviderName() const {
    return "Gui::ViewProviderPythonGeometry";
}
// explicit template instantiation
template class AppExport FeaturePythonT<GeoFeature>;
}
