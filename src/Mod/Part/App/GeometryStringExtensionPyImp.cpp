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

#ifndef _PreComp_
# include <sstream>
#endif

#include "GeometryStringExtensionPy.h"
#include "GeometryStringExtensionPy.cpp"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string GeometryStringExtensionPy::representation() const
{
    std::stringstream str;
    str << "<GeometryStringExtension (" ;

    if(!getGeometryStringExtensionPtr()->getName().empty())
        str << "\'" << getGeometryStringExtensionPtr()->getName() << "\', ";

    str << getGeometryStringExtensionPtr()->getValue() << ") >";

    return str.str();
}

PyObject *GeometryStringExtensionPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of the python object and the Twin object
    return new GeometryStringExtensionPy(new GeometryStringExtension);
}

// constructor method
int GeometryStringExtensionPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{

    if (PyArg_ParseTuple(args, "")) {
        // default extension
        return 0;
    }

    PyErr_Clear();
    char *pstr;
    if (PyArg_ParseTuple(args, "s", &pstr)) {
        this->getGeometryStringExtensionPtr()->setValue(pstr);
        return 0;
    }

    PyErr_Clear();
    char * pystr;
    if (PyArg_ParseTuple(args, "ss", &pstr, &pystr)) {
        this->getGeometryStringExtensionPtr()->setValue(pstr);
        this->getGeometryStringExtensionPtr()->setName(pystr);
        return 0;
    }


    PyErr_SetString(PyExc_TypeError, "GeometryStringExtension constructor accepts:\n"
    "-- empty parameter list\n"
    "-- string\n"
    "-- string, string\n");
    return -1;
}

Py::String GeometryStringExtensionPy::getValue() const
{
    return {this->getGeometryStringExtensionPtr()->getValue()};
}

void GeometryStringExtensionPy::setValue(Py::String value)
{
    this->getGeometryStringExtensionPtr()->setValue(value.as_std_string());
}

PyObject *GeometryStringExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int GeometryStringExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
