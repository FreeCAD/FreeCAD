/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "DocumentObjectGroup.h"
#include "Document.h"
#include <CXX/Objects.hxx>

// inclusion of the generated files (generated out of DocumentObjectGroupPy.xml)
#include "DocumentObjectGroupPy.h"
#include "DocumentObjectGroupPy.cpp"

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string DocumentObjectGroupPy::representation(void) const
{
    return std::string("<group object>");
}

PyObject*  DocumentObjectGroupPy::newObject(PyObject *args)
{
    char *sType,*sName=0;
    if (!PyArg_ParseTuple(args, "s|s", &sType,&sName))     // convert args: Python->C
        return NULL;

    DocumentObject *object = getDocumentObjectGroupPtr()->addObject(sType, sName);
    if ( object ) {
        return object->getPyObject();
    } 
    else {
        PyErr_Format(PyExc_Exception, "Cannot create object of type '%s'", sType);
        return NULL;
    }
}

PyObject*  DocumentObjectGroupPy::addObject(PyObject *args)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &object))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    DocumentObjectPy* docObj = static_cast<DocumentObjectPy*>(object);
    if (!docObj->getDocumentObjectPtr() || !docObj->getDocumentObjectPtr()->getNameInDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot add an invalid object");
        return NULL;
    }
    if (docObj->getDocumentObjectPtr()->getDocument() != getDocumentObjectGroupPtr()->getDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot add an object from another document to this group");
        return NULL;
    }
    if (docObj->getDocumentObjectPtr() == this->getDocumentObjectGroupPtr()) {
        PyErr_SetString(PyExc_Exception, "Cannot add a group object to itself");
        return NULL;
    }
    if (docObj->getDocumentObjectPtr()->getTypeId().isDerivedFrom(DocumentObjectGroup::getClassTypeId())) {
        App::DocumentObjectGroup* docGrp = static_cast<DocumentObjectGroup*>(docObj->getDocumentObjectPtr());
        if (this->getDocumentObjectGroupPtr()->isChildOf(docGrp)) {
            PyErr_SetString(PyExc_Exception, "Cannot add a group object to a child group");
            return NULL;
        }
    }

    DocumentObjectGroup* grp = getDocumentObjectGroupPtr();

    if (grp->getTypeId().isDerivedFrom(App::DocumentObjectGroupPython::getClassTypeId())) {
        DocumentObjectGroupPython* grppy = static_cast<DocumentObjectGroupPython*>(grp);
        App::Property* proxy = grppy->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("addObject"))) {
                Py::Callable method(vp.getAttr(std::string("addObject")));
                Py::Tuple args(1);
                args[0] = Py::Object(object);
                method.apply(args);
                Py_Return;
            }
        }
    }

    grp->addObject(docObj->getDocumentObjectPtr());
    Py_Return;
}

PyObject*  DocumentObjectGroupPy::removeObject(PyObject *args)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &object))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    DocumentObjectPy* docObj = static_cast<DocumentObjectPy*>(object);
    if (!docObj->getDocumentObjectPtr() || !docObj->getDocumentObjectPtr()->getNameInDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot remove an invalid object");
        return NULL;
    }
    if (docObj->getDocumentObjectPtr()->getDocument() != getDocumentObjectGroupPtr()->getDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot remove an object from another document from this group");
        return NULL;
    }

    DocumentObjectGroup* grp = getDocumentObjectGroupPtr();

    if (grp->getTypeId().isDerivedFrom(App::DocumentObjectGroupPython::getClassTypeId())) {
        DocumentObjectGroupPython* grppy = static_cast<DocumentObjectGroupPython*>(grp);
        App::Property* proxy = grppy->getPropertyByName("Proxy");
        if (proxy && proxy->getTypeId() == App::PropertyPythonObject::getClassTypeId()) {
            Py::Object vp = static_cast<App::PropertyPythonObject*>(proxy)->getValue();
            if (vp.hasAttr(std::string("removeObject"))) {
                Py::Callable method(vp.getAttr(std::string("removeObject")));
                Py::Tuple args(1);
                args[0] = Py::Object(object);
                method.apply(args);
                Py_Return;
            }
        }
    }

    grp->removeObject(docObj->getDocumentObjectPtr());
    Py_Return;
}

PyObject*  DocumentObjectGroupPy::removeObjectsFromDocument(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 

    getDocumentObjectGroupPtr()->removeObjectsFromDocument();
    Py_Return;
}

PyObject*  DocumentObjectGroupPy::getObject(PyObject *args)
{
    char* pcName;
    if (!PyArg_ParseTuple(args, "s", &pcName))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception 

    DocumentObject* obj = getDocumentObjectGroupPtr()->getObject(pcName);
    if ( obj ) {
        return obj->getPyObject();
    } else {
        Py_Return;
    }
}

PyObject*  DocumentObjectGroupPy::hasObject(PyObject *args)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &object))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    DocumentObjectPy* docObj = static_cast<DocumentObjectPy*>(object);
    if (!docObj->getDocumentObjectPtr() || !docObj->getDocumentObjectPtr()->getNameInDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot check an invalid object");
        return NULL;
    }
    if (docObj->getDocumentObjectPtr()->getDocument() != getDocumentObjectGroupPtr()->getDocument()) {
        PyErr_SetString(PyExc_Exception, "Cannot check an object from another document with this group");
        return NULL;
    }

    if (getDocumentObjectGroupPtr()->hasObject(docObj->getDocumentObjectPtr())) {
        Py_INCREF(Py_True);
        return Py_True;
    } 
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

PyObject *DocumentObjectGroupPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DocumentObjectGroupPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}

