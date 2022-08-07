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

#include "GeometryIntExtensionPy.h"
#include "GeometryIntExtensionPy.cpp"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string GeometryIntExtensionPy::representation() const
{
    std::stringstream str;
    long val = getGeometryIntExtensionPtr()->getValue();
    str << "<GeometryIntExtension (" ;

    if(!getGeometryIntExtensionPtr()->getName().empty())
        str << "\'" << getGeometryIntExtensionPtr()->getName() << "\', ";

    str << val << ") >";


    return str.str();
}

PyObject *GeometryIntExtensionPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of the python object and the Twin object
    return new GeometryIntExtensionPy(new GeometryIntExtension);
}

// constructor method
int GeometryIntExtensionPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{

    if (PyArg_ParseTuple(args, "")) {
        // default extension
        return 0;
    }

    PyErr_Clear();
    long val;
    if (PyArg_ParseTuple(args, "l", &val)) {
        this->getGeometryIntExtensionPtr()->setValue(val);
        return 0;
    }

    PyErr_Clear();
    char * pystr;
    if (PyArg_ParseTuple(args, "ls", &val,&pystr)) {
        this->getGeometryIntExtensionPtr()->setValue(val);
        this->getGeometryIntExtensionPtr()->setName(pystr);
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "GeometryIntExtension constructor accepts:\n"
    "-- empty parameter list\n"
    "-- long int\n"
    "-- long int, string\n");
    return -1;
}

Py::Long GeometryIntExtensionPy::getValue() const
{
    return Py::Long(this->getGeometryIntExtensionPtr()->getValue());
}

void GeometryIntExtensionPy::setValue(Py::Long value)
{
    this->getGeometryIntExtensionPtr()->setValue(long(value));
}



PyObject *GeometryIntExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int GeometryIntExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
