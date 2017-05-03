/***************************************************************************
 *   Copyright (c) Victor Titov (DeepSOIC)   <vv.titov@gmail.com> 2017     *
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
#endif

#include <App/Containers/ContainerBase.h>
#include <App/Containers/Exceptions.h>

// inclusion of the generated files (generated out of ContainerBasePy.xml)
#include <Containers/ContainerBasePy.h>
#include <Containers/ContainerBasePy.cpp>
#include <App/PropertyContainerPy.h>
#include <App/DocumentObjectPy.h>
#include <App/DocumentPy.h>

using namespace App;

// returns a string which represents the object e.g. when printed in python
std::string ContainerBasePy::representation(void) const
{
    std::stringstream repr;
    repr << "<App::ContainerBase around " ;
    if (getContainerBasePtr()->object())
        repr << getContainerBasePtr()->getName();
    else
        repr << "None";
    repr << ">";
    return std::string(repr.str());
}

Py::Object ContainerBasePy::getObject(void) const
{
    if (getContainerBasePtr()->object())
        return Py::asObject(getContainerBasePtr()->object()->getPyObject());
    else
        return Py::None();
}

Py::List makePyList(std::vector<DocumentObject*> objects) {
    Py::List ret;
    for (DocumentObject* obj : objects)
        ret.append(Py::asObject(obj->getPyObject()));
    return ret;
}
Py::List makePyList(std::vector<PropertyContainer*> objects) {
    Py::List ret;
    for (PropertyContainer* obj : objects)
        ret.append(Py::asObject(obj->getPyObject()));
    return ret;
}

Py::List ContainerBasePy::getStaticChildren(void) const
{
    try {
        return makePyList(getContainerBasePtr()->staticChildren());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}
Py::List ContainerBasePy::getDynamicChildren(void) const
{
    try {
        return makePyList(getContainerBasePtr()->dynamicChildren());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}
Py::List ContainerBasePy::getAllChildren(void) const
{
    try {
        return makePyList(getContainerBasePtr()->allChildren());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}


Py::List ContainerBasePy::getStaticChildrenRecursive(void) const
{
    try {
        return makePyList(getContainerBasePtr()->staticChildrenRecursive());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}
Py::List ContainerBasePy::getDynamicChildrenRecursive(void) const
{
    try {
        return makePyList(getContainerBasePtr()->dynamicChildrenRecursive());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}
Py::List ContainerBasePy::getAllChildrenRecursive(void) const
{
    try {
        return makePyList(getContainerBasePtr()->allChildrenRecursive());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}

Py::String ContainerBasePy::getName(void) const
{
    try {
        return Py::String(getContainerBasePtr()->getName());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}

Py::Object ContainerBasePy::getDocument(void) const
{
    try {
        return Py::asObject(getContainerBasePtr()->getDocument()->getPyObject());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}

Py::Object ContainerBasePy::getParent(void) const
{
    try {
        App::PropertyContainer* parent = getContainerBasePtr()->parent();
        if (parent)
            return Py::asObject(parent->getPyObject());
        else
            return Py::None();
    } CONTAINERBASEPY_STDCATCH_ATTR;
}

Py::List ContainerBasePy::getParents(void) const
{
    try {
        return makePyList(getContainerBasePtr()->parents());
    } CONTAINERBASEPY_STDCATCH_ATTR;
}



PyObject* ContainerBasePy::getObject(PyObject* args)
{
    char* objName;
    if (!PyArg_ParseTuple(args, "s", &objName))
        return 0;

    try {
        return getContainerBasePtr()->getObject(objName)->getPyObject();
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::hasObject(PyObject* args)
{
    PyObject* obj = nullptr;
    if (PyArg_ParseTuple(args, "O!", &(DocumentPy::Type), &obj))
        return Py::new_reference_to(Py::False());

    PyErr_Clear();
    if (!PyArg_ParseTuple(args, "O!", &(DocumentObjectPy::Type), &obj))
        return 0;

    try {
        assert(obj);
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->hasObject(static_cast<DocumentObjectPy*>(obj)->getDocumentObjectPtr())   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::isRoot(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->isRoot()   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::isAWorkspace(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->isAWorkspace()   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::isADocument(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->isADocument()   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::isAGroup(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->isAGroup()   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::isAGeoGroup(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->isAGeoGroup()   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::isAnOrigin(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->isAnOrigin()   ));
    } CONTAINERPY_STDCATCH_METH;
}

PyObject* ContainerBasePy::isADocumentObject(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        return Py::new_reference_to(Py::Boolean(
            getContainerBasePtr()->isADocumentObject()   ));
    } CONTAINERPY_STDCATCH_METH;
}



PyObject* ContainerBasePy::getCustomAttributes(const char* attr) const
{
    //objects accessible as attributes of Container...

    if (this->ob_type->tp_dict == NULL) {
        if (PyType_Ready(this->ob_type) < 0)
            return 0;
    }
    PyObject* item = PyDict_GetItemString(this->ob_type->tp_dict, attr);
    if (item) return 0; //don't replace an existing attribute if object name happens to match it...

    if (getContainerBasePtr()->isNull())
        return nullptr;
    // search for an object with this name
    try {
        DocumentObject* obj = getContainerBasePtr()->getObject(attr);
        return obj->getPyObject();
    } catch (ObjectNotFoundError&){
        return nullptr;
    }
}

int ContainerBasePy::setCustomAttributes(const char*,PyObject*)
{
    return 0;
}

