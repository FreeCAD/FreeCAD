/***************************************************************************
 *   Copyright (c) 2019 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include "GeometryExtension.h"
#include "GeometryExtensionPy.h"
#include "GeometryExtensionPy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string GeometryExtensionPy::representation(void) const
{
    return "<GeometryExtension object>";
}

PyObject *GeometryExtensionPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'GeometryExtension'.");
    return 0;
}

// constructor method
int GeometryExtensionPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* GeometryExtensionPy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    Part::GeometryExtension* ext = this->getGeometryExtensionPtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = 0;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, 0);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of geometry");
        return 0;
    }

    Part::GeometryExtensionPy* extpy = static_cast<Part::GeometryExtensionPy*>(cpy);
    // the PyMake function must have created the corresponding instance of the 'GeometryExtension' subclass
    // so delete it now to avoid a memory leak
    if (extpy->_pcTwinPointer) {
        Part::GeometryExtension* clone = static_cast<Part::GeometryExtension*>(extpy->_pcTwinPointer);
        delete clone;
    }
    extpy->_pcTwinPointer = ext->copy().get();
    return cpy;
}

Py::String GeometryExtensionPy::getName(void) const
{
    std::string name = this->getGeometryExtensionPtr()->getName();

    return Py::String(name);
}

void GeometryExtensionPy::setName(Py::String arg)
{
    std::string name = arg.as_std_string();

    this->getGeometryExtensionPtr()->setName(name);
}

PyObject *GeometryExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int GeometryExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
