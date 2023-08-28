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

#include "DocumentObject.h"

// inclusion of the generated files (generated out of GroupExtensionPy.xml)
#include "GroupExtensionPy.h"
#include "GroupExtensionPy.cpp"
#include "DocumentObjectPy.h"


using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string GroupExtensionPy::representation() const
{
    return {"<group extension object>"};
}

PyObject*  GroupExtensionPy::newObject(PyObject *args)
{
    char *sType,*sName=nullptr;
    if (!PyArg_ParseTuple(args, "s|s", &sType,&sName))
        return nullptr;

    DocumentObject *object = getGroupExtensionPtr()->addObject(sType, sName);
    if ( object ) {
        return object->getPyObject();
    }
    else {
        PyErr_Format(PyExc_TypeError, "Cannot create object of type '%s'", sType);
        return nullptr;
    }
}

PyObject*  GroupExtensionPy::addObject(PyObject *args)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &object))
        return nullptr;

    DocumentObjectPy* docObj = static_cast<DocumentObjectPy*>(object);
    if (!docObj->getDocumentObjectPtr() || !docObj->getDocumentObjectPtr()->getNameInDocument()) {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot add an invalid object");
        return nullptr;
    }
    
    if (docObj->getDocumentObjectPtr()->getDocument() != getGroupExtensionPtr()->getExtendedObject()->getDocument()) {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot add an object from another document to this group");
        return nullptr;
    }
    if (docObj->getDocumentObjectPtr() == this->getGroupExtensionPtr()->getExtendedObject()) {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot add a group object to itself");
        return nullptr;
    }
    if (docObj->getDocumentObjectPtr()->hasExtension(GroupExtension::getExtensionClassTypeId())) {
        App::GroupExtension* docGrp = docObj->getDocumentObjectPtr()->getExtensionByType<GroupExtension>();
        if (docGrp->hasObject(getGroupExtensionPtr()->getExtendedObject())) {
            PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot add a group object to a child group");
            return nullptr;
        }
    }

    GroupExtension* grp = getGroupExtensionPtr();

    auto vec = grp->addObject(docObj->getDocumentObjectPtr()); 
    Py::List list;
    for (App::DocumentObject* obj : vec)
        list.append(Py::asObject(obj->getPyObject()));

    return Py::new_reference_to(list);
}

PyObject* GroupExtensionPy::addObjects(PyObject *args) {
    
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O", &object))
        return nullptr;
        
    if (PyTuple_Check(object) || PyList_Check(object)) {
        Py::Sequence list(object);
        Py::Sequence::size_type size = list.size();
        std::vector<DocumentObject*> values;
        values.resize(size);

        for (Py::Sequence::size_type i = 0; i < size; i++) {
            Py::Object item = list[i];
            if (!PyObject_TypeCheck(*item, &(DocumentObjectPy::Type))) {
                std::string error = std::string("type in list must be 'DocumentObject', not ");
                error += (*item)->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<DocumentObjectPy*>(*item)->getDocumentObjectPtr();
        }

        GroupExtension* grp = getGroupExtensionPtr();
        auto vec = grp->addObjects(values); 
        Py::List result;
        for (App::DocumentObject* obj : vec)
            result.append(Py::asObject(obj->getPyObject()));

        return Py::new_reference_to(result);
    }
    
    std::string error = std::string("type must be list of 'DocumentObject', not ");
    error += object->ob_type->tp_name;
    throw Base::TypeError(error);
}

PyObject* GroupExtensionPy::setObjects(PyObject *args) {

    PyObject *object;
    if (!PyArg_ParseTuple(args, "O", &object))
        return nullptr;

    if (PyTuple_Check(object) || PyList_Check(object)) {
        Py::Sequence list(object);
        Py::Sequence::size_type size = list.size();
        std::vector<DocumentObject*> values;
        values.resize(size);

        for (Py::Sequence::size_type i = 0; i < size; i++) {
            Py::Object item = list[i];
            if (!PyObject_TypeCheck(*item, &(DocumentObjectPy::Type))) {
                std::string error = std::string("type in list must be 'DocumentObject', not ");
                error += (*item)->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<DocumentObjectPy*>(*item)->getDocumentObjectPtr();
        }

        GroupExtension* grp = getGroupExtensionPtr();
        auto vec = grp->setObjects(values); 
        Py::List result;
        for (App::DocumentObject* obj : vec)
            result.append(Py::asObject(obj->getPyObject()));

        return Py::new_reference_to(result);
    }
    
    std::string error = std::string("type must be list of 'DocumentObject', not ");
    error += object->ob_type->tp_name;
    throw Base::TypeError(error);
}

PyObject*  GroupExtensionPy::removeObject(PyObject *args)
{
    PyObject *object;
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &object))
        return nullptr;

    DocumentObjectPy* docObj = static_cast<DocumentObjectPy*>(object);
    if (!docObj->getDocumentObjectPtr() || !docObj->getDocumentObjectPtr()->getNameInDocument()) {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot remove an invalid object");
        return nullptr;
    }
    if (docObj->getDocumentObjectPtr()->getDocument() != getGroupExtensionPtr()->getExtendedObject()->getDocument()) {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot remove an object from another document from this group");
        return nullptr;
    }

    GroupExtension* grp = getGroupExtensionPtr();

    auto vec = grp->removeObject(docObj->getDocumentObjectPtr());
    Py::List list;
    for (App::DocumentObject* obj : vec)
        list.append(Py::asObject(obj->getPyObject()));

    return Py::new_reference_to(list);
}

PyObject* GroupExtensionPy::removeObjects(PyObject *args) {

    PyObject *object;
    if (!PyArg_ParseTuple(args, "O", &object))
        return nullptr;
        
    if (PyTuple_Check(object) || PyList_Check(object)) {
        Py::Sequence list(object);
        Py::Sequence::size_type size = list.size();
        std::vector<DocumentObject*> values;
        values.resize(size);

        for (Py::Sequence::size_type i = 0; i < size; i++) {
            Py::Object item = list[i];
            if (!PyObject_TypeCheck(*item, &(DocumentObjectPy::Type))) {
                std::string error = std::string("type in list must be 'DocumentObject', not ");
                error += (*item)->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<DocumentObjectPy*>(*item)->getDocumentObjectPtr();
        }

        GroupExtension* grp = getGroupExtensionPtr();
        auto vec = grp->removeObjects(values); 
        Py::List result;
        for (App::DocumentObject* obj : vec)
            result.append(Py::asObject(obj->getPyObject()));

        return Py::new_reference_to(result);
    }

    std::string error = std::string("type must be list of 'DocumentObject', not ");
    error += object->ob_type->tp_name;
    throw Base::TypeError(error);
}

PyObject*  GroupExtensionPy::removeObjectsFromDocument(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getGroupExtensionPtr()->removeObjectsFromDocument();
    Py_Return;
}

PyObject*  GroupExtensionPy::getObject(PyObject *args)
{
    char* pcName;
    if (!PyArg_ParseTuple(args, "s", &pcName))
        return nullptr;

    DocumentObject* obj = getGroupExtensionPtr()->getObject(pcName);
    if ( obj ) {
        return obj->getPyObject();
    } else {
        Py_Return;
    }
}

PyObject*  GroupExtensionPy::hasObject(PyObject *args)
{
    PyObject *object;
    PyObject *recursivePy = Py_False;
    if (!PyArg_ParseTuple(args, "O!|O!", &(DocumentObjectPy::Type), &object, &PyBool_Type, &recursivePy))
        return nullptr;

    DocumentObjectPy* docObj = static_cast<DocumentObjectPy*>(object);
    bool recursive = Base::asBoolean(recursivePy);
    if (!docObj->getDocumentObjectPtr() || !docObj->getDocumentObjectPtr()->getNameInDocument()) {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot check an invalid object");
        return nullptr;
    }
    if (docObj->getDocumentObjectPtr()->getDocument() != getGroupExtensionPtr()->getExtendedObject()->getDocument()) {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot check an object from another document with this group");
        return nullptr;
    }

    bool v = getGroupExtensionPtr()->hasObject(docObj->getDocumentObjectPtr(), recursive);
    return PyBool_FromLong(v ? 1 : 0);
}

PyObject *GroupExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int GroupExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
