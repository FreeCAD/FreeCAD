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

#include "GeometryDefaultExtension.h"

#include "GeometryBoolExtensionPy.h"
#include "GeometryBoolExtensionPy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string GeometryBoolExtensionPy::representation(void) const
{
    std::stringstream str;
    long id = getGeometryBoolExtensionPtr()->getValue();
    str << "<GeometryBoolExtension (" ;

    if(getGeometryBoolExtensionPtr()->getName().size()>0)
        str << "\'" << getGeometryBoolExtensionPtr()->getName() << "\', ";

    str << (id==0?"False":"True") << ") >";


    return str.str();
}

PyObject *GeometryBoolExtensionPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of the python object and the Twin object
    return new GeometryBoolExtensionPy(new GeometryBoolExtension);
}

// constructor method
int GeometryBoolExtensionPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{

    if (PyArg_ParseTuple(args, "")) {
        // default extension
        return 0;
    }

    PyErr_Clear();
    PyObject* Id;
    if (PyArg_ParseTuple(args, "O!", &PyBool_Type, &Id)) {
        this->getGeometryBoolExtensionPtr()->setValue(PyObject_IsTrue(Id) ? true : false);
        return 0;
    }

    PyErr_Clear();
    char * pystr;
    if (PyArg_ParseTuple(args, "O!s", &PyBool_Type, &Id, &pystr)) {
        this->getGeometryBoolExtensionPtr()->setValue(PyObject_IsTrue(Id) ? true : false);
        this->getGeometryBoolExtensionPtr()->setName(pystr);
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "GeometryBoolExtension constructor accepts:\n"
    "-- empty parameter list\n"
    "-- Boolean\n"
    "-- Boolean, string\n");
    return -1;
}

Py::Boolean GeometryBoolExtensionPy::getValue(void) const
{
    return Py::Boolean(this->getGeometryBoolExtensionPtr()->getValue());
}

void GeometryBoolExtensionPy::setValue(Py::Boolean value)
{
    this->getGeometryBoolExtensionPtr()->setValue(value);
}

PyObject *GeometryBoolExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int GeometryBoolExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
