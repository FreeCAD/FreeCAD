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

FeaturePythonImp::FeaturePythonImp(App::DocumentObject* o) : object(o)
{
}

FeaturePythonImp::~FeaturePythonImp()
{
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

    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(std::string("execute"))) {
                if (feature.hasAttr("__object__")) {
                    Base::ObjectStatusLocker<ObjectStatus, DocumentObject> exe(App::PythonCall, object);
                    Py::Callable method(feature.getAttr(std::string("execute")));
                    Py::Tuple args;
                    Py::Object res = method.apply(args);
                    if (res.isBoolean() && !res.isTrue())
                        return false;
                    return true;
                }
                else {
                    Base::ObjectStatusLocker<ObjectStatus, DocumentObject> exe(App::PythonCall, object);
                    Py::Callable method(feature.getAttr(std::string("execute")));
                    Py::Tuple args(1);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    Py::Object res = method.apply(args);
                    if (res.isBoolean() && !res.isTrue())
                        return false;
                    return true;
                }
            }
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
    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(std::string("mustExecute"))) {
                if (feature.hasAttr("__object__")) {
                    Py::Callable method(feature.getAttr(std::string("mustExecute")));
                    Py::Tuple args;
                    Py::Object res(method.apply(args));
                    return res.isTrue();
                }
                else {
                    Py::Callable method(feature.getAttr(std::string("mustExecute")));
                    Py::Tuple args(1);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    Py::Object res(method.apply(args));
                    return res.isTrue();
                }
            }
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
    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        const char *prop_name = object->getPropertyName(prop);
        if(prop_name == 0)
            return;
        Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(std::string("onBeforeChange"))) {
                if (feature.hasAttr("__object__")) {
                    Py::Callable method(feature.getAttr(std::string("onBeforeChange")));
                    Py::Tuple args(1);
                    args.setItem(0, Py::String(prop_name));
                    method.apply(args);
                }
                else {
                    Py::Callable method(feature.getAttr(std::string("onBeforeChange")));
                    Py::Tuple args(2);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    args.setItem(1, Py::String(prop_name));
                    method.apply(args);
                }
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

bool FeaturePythonImp::onBeforeChangeLabel(std::string &newLabel)
{
    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<PropertyPythonObject*>(proxy)->getValue();
            const char *funcName = "onBeforeChangeLabel";
            if (feature.hasAttr(funcName)) {
                Py::Callable method(feature.getAttr(funcName));
                bool hasObj = feature.hasAttr("__object__");
                Py::Tuple args(hasObj?1:2);
                int i = 0;
                if(!hasObj)
                    args.setItem(i++, Py::Object(object->getPyObject(), true));
                args.setItem(i++,Py::String(newLabel));
                Py::Object ret(method.apply(args));
                if(!ret.isNone()) {
                    if(!ret.isString())
                        throw Base::TypeError("onBeforeChangeLabel expects to return a string");
                    newLabel = ret.as_string();
                    return true;
                }
            }
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
    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        const char *prop_name = object->getPropertyName(prop);
        if(prop_name == 0)
            return;
        Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(std::string("onChanged"))) {
                if (feature.hasAttr("__object__")) {
                    Py::Callable method(feature.getAttr(std::string("onChanged")));
                    Py::Tuple args(1);
                    args.setItem(0, Py::String(prop_name));
                    method.apply(args);
                }
                else {
                    Py::Callable method(feature.getAttr(std::string("onChanged")));
                    Py::Tuple args(2);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    args.setItem(1, Py::String(prop_name));
                    method.apply(args);
                }
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void FeaturePythonImp::onDocumentRestored()
{
    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(std::string("onDocumentRestored"))) {
                if (feature.hasAttr("__object__")) {
                    Py::Callable method(feature.getAttr(std::string("onDocumentRestored")));
                    Py::Tuple args;
                    method.apply(args);
                }
                else {
                    Py::Callable method(feature.getAttr(std::string("onDocumentRestored")));
                    Py::Tuple args(1);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                    method.apply(args);
                }
            }
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
    Base::PyGILStateLocker lock;
    try {
        Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(std::string("getSubObject"))) {
                Py::Callable method(feature.getAttr(std::string("getSubObject")));
                int numArg = 5;
                if(!feature.hasAttr("__object__")) 
                    ++numArg;
                Py::Tuple args(numArg);
                int i = 0;
                if(numArg > 5)
                    args.setItem(i++, Py::Object(object->getPyObject(), true));
                if(!subname) subname = "";
                args.setItem(i++,Py::String(subname));
                args.setItem(i++,Py::Int(pyObj?2:1));
                Base::MatrixPy *pyMat = new Base::MatrixPy(new Base::Matrix4D);
                if(_mat) *pyMat->getMatrixPtr() = *_mat;
                args.setItem(i++,Py::Object(pyMat));
                args.setItem(i++,Py::Boolean(transform));
                args.setItem(i++,Py::Int(depth));

                Py::Object res(method.apply(args));
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
        }
        return false;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
        ret = 0;
        return true;
    }
}

bool FeaturePythonImp::getSubObjects(std::vector<std::string> &ret, int reason) const {
    Base::PyGILStateLocker lock;
    try {
        Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(std::string("getSubObjects"))) {
                Py::Callable method(feature.getAttr(std::string("getSubObjects")));
                Py::Tuple args(2);
                args.setItem(0, Py::Object(object->getPyObject(), true));
                args.setItem(1, Py::Int(reason));
                Py::Object res(method.apply(args));
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
        }
        return false;
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
    Base::PyGILStateLocker lock;
    try {
        Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(std::string("getLinkedObject"))) {
                Py::Callable method(feature.getAttr(std::string("getLinkedObject")));
                int numArg = 4;
                if(!feature.hasAttr("__object__")) 
                    ++numArg;
                Py::Tuple args(numArg);
                int i = 0;
                if(numArg > 4)
                    args.setItem(i++, Py::Object(object->getPyObject(), true));
                args.setItem(i++,Py::Boolean(recurse));
                Base::MatrixPy *pyMat = new Base::MatrixPy(new Base::Matrix4D);
                if(_mat) *pyMat->getMatrixPtr() = *_mat;
                args.setItem(i++,Py::Object(pyMat));
                args.setItem(i++,Py::Boolean(transform));
                args.setItem(i++,Py::Int(depth));

                Py::Object res(method.apply(args));
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
        }
        return false;
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
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(std::string("hasChildElement"))) {
                Py::Callable method(feature.getAttr(std::string("hasChildElement")));
                Py::Tuple args;
                if(!feature.hasAttr("__object__")) {
                    args = Py::Tuple(1);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                }
                Py::Boolean ok(method.apply(args));
                return static_cast<bool>(ok) ? 1 : 0;
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return -1;
}

int FeaturePythonImp::isElementVisible(const char *element) const {
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(std::string("isElementVisible"))) {
                Py::Callable method(feature.getAttr(std::string("isElementVisible")));
                int i = 0;
                Py::Tuple args(1);
                if(!feature.hasAttr("__object__")) {
                    args = Py::Tuple(2);
                    args.setItem(i++, Py::Object(object->getPyObject(), true));
                }
                args.setItem(i,Py::String(element?element:""));
                long ret = Py::Int(method.apply(args));
                return ret;
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
    return -2;
}

int FeaturePythonImp::setElementVisible(const char *element, bool visible) {
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(std::string("setElementVisible"))) {
                Py::Callable method(feature.getAttr(std::string("setElementVisible")));
                int i = 0;
                Py::Tuple args(2);
                if(!feature.hasAttr("__object__")) {
                    args = Py::Tuple(3);
                    args.setItem(i++, Py::Object(object->getPyObject(), true));
                }
                args.setItem(i++,Py::String(element?element:""));
                args.setItem(i++,Py::Boolean(visible));
                long ret = Py::Int(method.apply(args));
                return ret;
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return -2;
}

std::string FeaturePythonImp::getViewProviderName()
{
    // Run the execute method of the proxy object.
    Base::PyGILStateLocker lock;
    try {
        Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<PropertyPythonObject*>(proxy)->getValue();
            if (feature.hasAttr(std::string("getViewProviderName"))) {
                if (feature.hasAttr("__object__")) {
                    Py::Callable method(feature.getAttr(std::string("getViewProviderName")));
                    Py::Tuple args;
                    Py::String ret(method.apply(args));
                    return ret.as_string();
                } else {
                    Py::Callable method(feature.getAttr(std::string("getViewProviderName")));
                    Py::TupleN args(Py::Object(object->getPyObject(), true));
                    Py::String ret(method.apply(args));
                    return ret.as_string();
                }
            }
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }

    return std::string();
}

int FeaturePythonImp::canLinkProperties() const {
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            const char *funcName = "canLinkProperties";
            if (feature.hasAttr(funcName)) {
                Py::Callable method(feature.getAttr(funcName));
                Py::Tuple args;
                if(!feature.hasAttr("__object__")) {
                    args = Py::Tuple(1);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                }
                Py::Boolean ok(method.apply(args));
                return ok?1:0;
            }
        }
        return -1;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
        return 0;
    }
}

int FeaturePythonImp::allowDuplicateLabel() const {
    Base::PyGILStateLocker lock;
    try {
        App::Property* proxy = object->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object feature = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            const char *funcName = "allowDuplicateLabel";
            if (feature.hasAttr(funcName)) {
                Py::Callable method(feature.getAttr(funcName));
                Py::Tuple args;
                if(!feature.hasAttr("__object__")) {
                    args = Py::Tuple(1);
                    args.setItem(0, Py::Object(object->getPyObject(), true));
                }
                Py::Boolean ok(method.apply(args));
                return ok?1:0;
            }
        }
        return -1;
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
        return 0;
    }
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
