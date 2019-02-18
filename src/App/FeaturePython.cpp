/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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


#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Reader.h>
#include <Base/MatrixPy.h>
#include <Base/Tools.h>
#include <App/DocumentObjectPy.h>
#include "FeaturePython.h"
#include "FeaturePythonPyImp.h"

using namespace App;

FeaturePythonImp::FeaturePythonImp(App::DocumentObject* o) : object(o), has__object__(false)
{
}

FeaturePythonImp::~FeaturePythonImp()
{
}

void FeaturePythonImp::init(PyObject *pyobj) {
    Base::PyGILStateLocker lock;
    has__object__ = !!PyObject_HasAttrString(pyobj, "__object__");
#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name) FC_PY_GetCallable(pyobj,#_name,py_##_name);
    FC_PY_FEATURE_PYTHON
}

/*!
 Calls the execute() method of the Python feature class. If the Python feature class doesn't have an execute()
 method or if it returns False this method also return false and true otherwise.
 */
bool FeaturePythonImp::execute()
{
    // avoid recursive calls of execute()
    if (object->testStatus(App::PythonCall))
        return false;

    if(py_execute.isNone())
        return false;

    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        if (has__object__) {
            Base::ObjectStatusLocker<ObjectStatus, DocumentObject> exe(App::PythonCall, object);
            Py::Object res = Base::pyCall(py_execute.ptr());
            if (res.isBoolean() && !res.isTrue())
                return false;
            return true;
        }
        else {
            Base::ObjectStatusLocker<ObjectStatus, DocumentObject> exe(App::PythonCall, object);
            Py::Tuple args(1);
            args.setItem(0, Py::Object(object->getPyObject(), true));
            Py::Object res = Base::pyCall(py_execute.ptr(),args.ptr());
            if (res.isBoolean() && !res.isTrue())
                return false;
            return true;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
        std::stringstream str;
        str << object->Label.getValue() << ": " << e.what();
        throw Base::RuntimeError(str.str());
    }

    return false;
}

bool FeaturePythonImp::mustExecute() const
{
    if(py_mustExecute.isNone())
        return false;
    // Run the execute method of the proxy object.
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
    if(py_onBeforeChange.isNone())
        return;

    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        const char *prop_name = object->getPropertyName(prop);
        if(prop_name == 0)
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
                throw Base::TypeError("onBeforeChangeLabel expects to return a string");
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
    if(py_onChanged.isNone())
        return;
    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        const char *prop_name = object->getPropertyName(prop);
        if(prop_name == 0)
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

bool FeaturePythonImp::onParentChanged(App::DocumentObject *parent, const Property* prop)
{
    if(py_onParentChanged.isNone())
        return true;
    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        const char *prop_name = object->getPropertyName(prop);
        if(prop_name == 0)
            return true;
        if (has__object__) {
            Py::Tuple args(2);
            args.setItem(0, Py::asObject(parent->getPyObject()));
            args.setItem(1, Py::String(prop_name));
            return Base::pyCall(py_onParentChanged.ptr(),args.ptr()).isTrue();
        }
        else {
            Py::Tuple args(3);
            args.setItem(0, Py::asObject(parent->getPyObject()));
            args.setItem(1, Py::asObject(object->getPyObject()));
            args.setItem(2, Py::String(prop_name));
            return Base::pyCall(py_onParentChanged.ptr(),args.ptr()).isTrue();
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return true;
}

void FeaturePythonImp::onDocumentRestored()
{
    if(py_onDocumentRestored.isNone())
        return;
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

bool FeaturePythonImp::getSubObject(DocumentObject *&ret, const char *subname, 
    PyObject **pyObj, Base::Matrix4D *_mat, bool transform, int depth) const
{
    if(py_getSubObject.isNone())
        return false;

    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(6);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        if(!subname) subname = "";
        args.setItem(1,Py::String(subname));
        args.setItem(2,Py::Int(pyObj?2:1));
        Base::MatrixPy *pyMat = new Base::MatrixPy(new Base::Matrix4D);
        if(_mat) *pyMat->getMatrixPtr() = *_mat;
        args.setItem(3,Py::Object(pyMat));
        args.setItem(4,Py::Boolean(transform));
        args.setItem(5,Py::Int(depth));

        Py::Object res(Base::pyCall(py_getSubObject.ptr(),args.ptr()));
        if(res.isNone()) {
            ret = 0;
            return true;
        }
        if(!res.isTrue())
            return false;
        if(!res.isSequence())
            throw Base::TypeError("getSubObject expects return type of tuple");
        Py::Sequence seq(res);
        if(seq.length() < 2 ||
                (!seq.getItem(0).isNone() && 
                 !PyObject_TypeCheck(seq.getItem(0).ptr(),&DocumentObjectPy::Type)) ||
                !PyObject_TypeCheck(seq.getItem(1).ptr(),&Base::MatrixPy::Type))
        {
            throw Base::TypeError("getSubObject expects return type of (obj,matrix,pyobj)");
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
            ret = 0;
        else
            ret = static_cast<DocumentObjectPy*>(seq.getItem(0).ptr())->getDocumentObjectPtr();
        return true;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
        ret = 0;
        return true;
    }
}

bool FeaturePythonImp::getSubObjects(std::vector<std::string> &ret, int reason) const {
    if(py_getSubObjects.isNone())
        return false;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(2);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        args.setItem(1, Py::Int(reason));
        Py::Object res(Base::pyCall(py_getSubObjects.ptr(),args.ptr()));
        if(!res.isTrue())
            return true;
        if(!res.isSequence())
            throw Base::TypeError("getSubObjects expects return type of tuple");
        Py::Sequence seq(res);
        for(size_t i=0;i<seq.length();++i) {
            Py::Object name(seq[i].ptr());
            if(!name.isString())
                throw Base::TypeError("getSubObjects expects string in returned sequence");
            ret.push_back(name.as_string());
        }
        return true;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
        return true;
    }
}

bool FeaturePythonImp::getLinkedObject(DocumentObject *&ret, bool recurse, 
        Base::Matrix4D *_mat, bool transform, int depth) const
{
    if(py_getLinkedObject.isNone())
        return false;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(5);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        args.setItem(1,Py::Boolean(recurse));
        Base::MatrixPy *pyMat = new Base::MatrixPy(new Base::Matrix4D);
        if(_mat) *pyMat->getMatrixPtr() = *_mat;
        args.setItem(2,Py::Object(pyMat));
        args.setItem(3,Py::Boolean(transform));
        args.setItem(4,Py::Int(depth));

        Py::Object res(Base::pyCall(py_getLinkedObject.ptr(),args.ptr()));
        if(!res.isTrue()) {
            ret = object;
            return true;
        }
        if(!res.isSequence())
            throw Base::TypeError("getLinkedObject expects return type of (object,matrix)");
        Py::Sequence seq(res);
        if(seq.length() != 2 ||
                (!seq.getItem(0).isNone() && 
                 !PyObject_TypeCheck(seq.getItem(0).ptr(),&DocumentObjectPy::Type)) ||
                !PyObject_TypeCheck(seq.getItem(1).ptr(),&Base::MatrixPy::Type))
        {
            throw Base::TypeError("getLinkedObject expects return type of (object,matrix)");
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
        Base::PyException e; // extract the Python error text
        e.ReportException();
        ret = 0;
        return true;
    }
}

PyObject *FeaturePythonImp::getPyObject(void)
{
    // ref counter is set to 1
    return new FeaturePythonPyT<DocumentObjectPy>(object);
}

int FeaturePythonImp::hasChildElement() const {
    if(py_hasChildElement.isNone())
        return -1;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        Py::Boolean ok(Base::pyCall(py_hasChildElement.ptr(),args.ptr()));
        return static_cast<bool>(ok) ? 1 : 0;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return -1;
}

int FeaturePythonImp::isElementVisible(const char *element) const {
    if(py_isElementVisible.isNone())
        return -2;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(2);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        args.setItem(1,Py::String(element?element:""));
        return Py::Int(Base::pyCall(py_isElementVisible.ptr(),args.ptr()));
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return -2;
}

int FeaturePythonImp::setElementVisible(const char *element, bool visible) {
    if(py_setElementVisible.isNone())
        return -2;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(3);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        args.setItem(1,Py::String(element?element:""));
        args.setItem(2,Py::Boolean(visible));
        return Py::Int(Base::pyCall(py_setElementVisible.ptr(),args.ptr()));
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return -2;
}

std::string FeaturePythonImp::getViewProviderName()
{
    if(py_getViewProviderName.isNone())
        return std::string();
    // Run the execute method of the proxy object.
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

    return std::string();
}

int FeaturePythonImp::canLinkProperties() const {
    if(py_canLinkProperties.isNone())
        return -1;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        Py::Boolean ok(Base::pyCall(py_canLinkProperties.ptr(),args.ptr()));
        return ok?1:0;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
        return 0;
    }
}

int FeaturePythonImp::allowDuplicateLabel() const {
    if(py_allowDuplicateLabel.isNone())
        return -1;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        Py::Boolean ok(Base::pyCall(py_allowDuplicateLabel.ptr(),args.ptr()));
        return ok?1:0;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
        return 0;
    }
}

int FeaturePythonImp::canLoadPartial() const {
    if(py_canLoadPartial.isNone())
        return -1;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(1);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        Py::Int ret(Base::pyCall(py_canLoadPartial.ptr(),args.ptr()));
        return ret;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return -1;
}

bool FeaturePythonImp::redirectSubName(std::ostringstream &ss,
        App::DocumentObject *topParent, App::DocumentObject *child) const 
{
    if(py_redirectSubName.isNone())
        return false;
    Base::PyGILStateLocker lock;
    try {
        Py::Tuple args(4);
        args.setItem(0, Py::Object(object->getPyObject(), true));
        args.setItem(1,Py::String(ss.str()));
        args.setItem(2,topParent?Py::Object(topParent->getPyObject(),true):Py::Object());
        args.setItem(3,child?Py::Object(child->getPyObject(),true):Py::Object());
        Py::Object ret(Base::pyCall(py_redirectSubName.ptr(),args.ptr()));
        if(ret.isNone())
            return false;
        ss.str("");
        ss << ret.as_string();
        return true;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return false;
}

// ---------------------------------------------------------

namespace App {
PROPERTY_SOURCE_TEMPLATE(App::FeaturePython, App::DocumentObject)
template<> const char* App::FeaturePython::getViewProviderName(void) const {
    return "Gui::ViewProviderPythonFeature";
}
template<> PyObject* App::FeaturePython::getPyObject(void) {
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
template<> const char* App::GeometryPython::getViewProviderName(void) const {
    return "Gui::ViewProviderPythonGeometry";
}
// explicit template instantiation
template class AppExport FeaturePythonT<GeoFeature>;
}
