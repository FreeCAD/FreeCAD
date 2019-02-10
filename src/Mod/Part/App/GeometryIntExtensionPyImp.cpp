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

#include "GeometryIntExtensionPy.h"
#include "GeometryIntExtensionPy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string GeometryIntExtensionPy::representation(void) const
{
    std::stringstream str;
    long id = getGeometryIntExtensionPtr()->getValue();
    str << "<GeometryIntExtension (" ;

    if(getGeometryIntExtensionPtr()->getName().size()>0)
        str << "\'" << getGeometryIntExtensionPtr()->getName() << "\', ";

    str << id << ") >";


    return str.str();
}

PyObject *GeometryIntExtensionPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of PointPy and the Twin object
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
    long Id;
    if (PyArg_ParseTuple(args, "l", &Id)) {
        this->getGeometryIntExtensionPtr()->setValue(Id);
        return 0;
    }



    PyErr_SetString(PyExc_TypeError, "GeometryIntExtension constructor accepts:\n"
    "-- empty parameter list\n"
    "-- long int\n");
    return -1;
}

Py::Long GeometryIntExtensionPy::getValue(void) const
{
    return Py::Long(this->getGeometryIntExtensionPtr()->getValue());
}

void GeometryIntExtensionPy::setValue(Py::Long value)
{
    this->getGeometryIntExtensionPtr()->setValue(long(value));
}



PyObject *GeometryIntExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int GeometryIntExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
