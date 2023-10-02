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

#include "ExternalGeometryExtensionPy.h"

#include "ExternalGeometryExtensionPy.cpp"


using namespace Sketcher;

// returns a string which represents the object e.g. when printed in python
std::string ExternalGeometryExtensionPy::representation() const
{
    std::stringstream str;

    std::string ref = getExternalGeometryExtensionPtr()->getRef();


    str << "<ExternalGeometryExtension (";

    if (!getExternalGeometryExtensionPtr()->getName().empty()) {
        str << "\'" << getExternalGeometryExtensionPtr()->getName() << "\', ";
    }

    str << "\"" << ref;

    if (!getExternalGeometryExtensionPtr()->isClear()) {
        str << "\",{";

        bool first = true;

        for (size_t i = 0; i < ExternalGeometryExtension::NumFlags; i++) {
            if (getExternalGeometryExtensionPtr()->testFlag(i)) {
                if (first) {
                    first = false;
                }
                else {
                    str << ", ";
                }

                str << getExternalGeometryExtensionPtr()->flag2str[i];
            }
        }

        str << "}";
    }
    else {
        str << "\") >";
    }

    str << ") >";

    return str.str();
}

// Python wrapper
PyObject* ExternalGeometryExtensionPy::PyMake(struct _typeobject*, PyObject*, PyObject*)
{
    // create a new instance of PointPy and the Twin object
    return new ExternalGeometryExtensionPy(new ExternalGeometryExtension);
}

// constructor method
int ExternalGeometryExtensionPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{

    if (PyArg_ParseTuple(args, "")) {
        // default extension
        return 0;
    }

    PyErr_SetString(PyExc_TypeError,
                    "ExternalGeometryExtension constructor accepts:\n"
                    "-- empty parameter list\n");
    return -1;
}

PyObject* ExternalGeometryExtensionPy::testFlag(PyObject* args)
{
    char* flag;
    if (PyArg_ParseTuple(args, "s", &flag)) {

        ExternalGeometryExtension::Flag flagtype;

        if (getExternalGeometryExtensionPtr()->getFlagsFromName(flag, flagtype)) {
            return new_reference_to(
                Py::Boolean(this->getExternalGeometryExtensionPtr()->testFlag(flagtype)));
        }

        PyErr_SetString(PyExc_TypeError, "Flag string does not exist.");
        return nullptr;
    }

    PyErr_SetString(PyExc_TypeError, "No flag string provided.");
    return nullptr;
}

PyObject* ExternalGeometryExtensionPy::setFlag(PyObject* args)
{
    char* flag;
    PyObject* bflag = Py_True;
    if (PyArg_ParseTuple(args, "s|O!", &flag, &PyBool_Type, &bflag)) {

        ExternalGeometryExtension::Flag flagtype;

        if (getExternalGeometryExtensionPtr()->getFlagsFromName(flag, flagtype)) {

            this->getExternalGeometryExtensionPtr()->setFlag(flagtype, Base::asBoolean(bflag));
            Py_Return;
        }

        PyErr_SetString(PyExc_TypeError, "Flag string does not exist.");
        return nullptr;
    }

    PyErr_SetString(PyExc_TypeError, "No flag string provided.");
    Py_Return;
}

Py::String ExternalGeometryExtensionPy::getRef() const
{
    return Py::String(this->getExternalGeometryExtensionPtr()->getRef());
}

void ExternalGeometryExtensionPy::setRef(Py::String value)
{
    this->getExternalGeometryExtensionPtr()->setRef(value.as_std_string());
}


PyObject* ExternalGeometryExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ExternalGeometryExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
