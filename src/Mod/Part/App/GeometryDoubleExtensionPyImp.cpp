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

#include "GeometryDoubleExtensionPy.h"
#include "GeometryDoubleExtensionPy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string GeometryDoubleExtensionPy::representation() const
{
    std::stringstream str;
    double val = getGeometryDoubleExtensionPtr()->getValue();
    str << "<GeometryDoubleExtension (" ;

    if(!getGeometryDoubleExtensionPtr()->getName().empty())
        str << "\'" << getGeometryDoubleExtensionPtr()->getName() << "\', ";

    str << val << ") >";


    return str.str();
}

PyObject *GeometryDoubleExtensionPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of the python object and the Twin object
    return new GeometryDoubleExtensionPy(new GeometryDoubleExtension);
}

// constructor method
int GeometryDoubleExtensionPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{

    if (PyArg_ParseTuple(args, "")) {
        // default extension
        return 0;
    }

    PyErr_Clear();
    double val;
    if (PyArg_ParseTuple(args, "d", &val)) {
        this->getGeometryDoubleExtensionPtr()->setValue(val);
        return 0;
    }

    PyErr_Clear();
    char * pystr;
    if (PyArg_ParseTuple(args, "ds", &val,&pystr)) {
        this->getGeometryDoubleExtensionPtr()->setValue(val);
        this->getGeometryDoubleExtensionPtr()->setName(pystr);
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "GeometryDoubleExtension constructor accepts:\n"
    "-- empty parameter list\n"
    "-- double\n"
    "-- double, string\n");
    return -1;
}

Py::Float GeometryDoubleExtensionPy::getValue() const
{
    return Py::Float(this->getGeometryDoubleExtensionPtr()->getValue());
}

void GeometryDoubleExtensionPy::setValue(Py::Float value)
{
    this->getGeometryDoubleExtensionPtr()->setValue(float(value));
}



PyObject *GeometryDoubleExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int GeometryDoubleExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
