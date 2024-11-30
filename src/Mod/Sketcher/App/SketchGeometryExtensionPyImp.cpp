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

#include "SketchGeometryExtensionPy.h"

#include "SketchGeometryExtensionPy.cpp"


using namespace Sketcher;

// returns a string which represents the object e.g. when printed in python
std::string SketchGeometryExtensionPy::representation() const
{
    std::stringstream str;
    str << "<SketchGeometryExtension (";

    if (!getSketchGeometryExtensionPtr()->getName().empty()) {
        str << "\'" << getSketchGeometryExtensionPtr()->getName() << "\', ";
    }

    str << "\"";

    str << getSketchGeometryExtensionPtr()->getId() << "\") >";
    return str.str();
}

// Python wrapper
PyObject* SketchGeometryExtensionPy::PyMake(struct _typeobject*, PyObject*, PyObject*)
{
    // create a new instance of PointPy and the Twin object
    return new SketchGeometryExtensionPy(new SketchGeometryExtension);
}

// constructor method
int SketchGeometryExtensionPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{

    if (PyArg_ParseTuple(args, "")) {
        // default extension
        return 0;
    }

    PyErr_Clear();
    int Id;
    if (PyArg_ParseTuple(args, "i", &Id)) {
        this->getSketchGeometryExtensionPtr()->setId(Id);
        return 0;
    }


    PyErr_SetString(PyExc_TypeError,
                    "SketchGeometryExtension constructor accepts:\n"
                    "-- empty parameter list\n"
                    "-- int\n");
    return -1;
}

Py::Long SketchGeometryExtensionPy::getId() const
{
    return Py::Long(this->getSketchGeometryExtensionPtr()->getId());
}

void SketchGeometryExtensionPy::setId(Py::Long Id)
{
    this->getSketchGeometryExtensionPtr()->setId(long(Id));
}

Py::String SketchGeometryExtensionPy::getInternalType() const
{
    int internaltypeindex = (int)this->getSketchGeometryExtensionPtr()->getInternalType();

    if (internaltypeindex >= InternalType::NumInternalGeometryType) {
        throw Py::NotImplementedError("String name of enum not implemented");
    }

    std::string typestr =
        this->getSketchGeometryExtensionPtr()->internaltype2str[internaltypeindex];

    return Py::String(typestr);
}

void SketchGeometryExtensionPy::setInternalType(Py::String arg)
{
    std::string argstr = arg;
    InternalType::InternalType type;

    if (SketchGeometryExtension::getInternalTypeFromName(argstr, type)) {
        this->getSketchGeometryExtensionPtr()->setInternalType(type);
        return;
    }

    throw Py::ValueError("Argument is not a valid internal geometry type.");
}

Py::Boolean SketchGeometryExtensionPy::getBlocked() const
{
    return Py::Boolean(getSketchGeometryExtensionPtr()->testGeometryMode(GeometryMode::Blocked));
}

void SketchGeometryExtensionPy::setBlocked(Py::Boolean arg)
{
    getSketchGeometryExtensionPtr()->setGeometryMode(GeometryMode::Blocked, arg);
}

Py::Boolean SketchGeometryExtensionPy::getConstruction() const
{
    return Py::Boolean(
        getSketchGeometryExtensionPtr()->testGeometryMode(GeometryMode::Construction));
}

void SketchGeometryExtensionPy::setConstruction(Py::Boolean arg)
{
    getSketchGeometryExtensionPtr()->setGeometryMode(GeometryMode::Construction, arg);
}

PyObject* SketchGeometryExtensionPy::testGeometryMode(PyObject* args)
{
    char* flag;
    if (PyArg_ParseTuple(args, "s", &flag)) {

        GeometryMode::GeometryMode mode;

        if (getSketchGeometryExtensionPtr()->getGeometryModeFromName(flag, mode)) {
            return new_reference_to(
                Py::Boolean(getSketchGeometryExtensionPtr()->testGeometryMode(mode)));
        }

        PyErr_SetString(PyExc_TypeError, "Flag string does not exist.");
        return nullptr;
    }

    PyErr_SetString(PyExc_TypeError, "No flag string provided.");
    return nullptr;
}

PyObject* SketchGeometryExtensionPy::setGeometryMode(PyObject* args)
{
    char* flag;
    PyObject* bflag = Py_True;
    if (PyArg_ParseTuple(args, "s|O!", &flag, &PyBool_Type, &bflag)) {

        GeometryMode::GeometryMode mode;

        if (getSketchGeometryExtensionPtr()->getGeometryModeFromName(flag, mode)) {
            getSketchGeometryExtensionPtr()->setGeometryMode(mode, Base::asBoolean(bflag));
            Py_Return;
        }

        PyErr_SetString(PyExc_TypeError, "Flag string does not exist.");
        return nullptr;
    }

    PyErr_SetString(PyExc_TypeError, "No flag string provided.");
    Py_Return;
}

Py::Long SketchGeometryExtensionPy::getGeometryLayerId() const
{
    return Py::Long(this->getSketchGeometryExtensionPtr()->getGeometryLayerId());
}

void SketchGeometryExtensionPy::setGeometryLayerId(Py::Long Id)
{
    this->getSketchGeometryExtensionPtr()->setGeometryLayerId(long(Id));
}

PyObject* SketchGeometryExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int SketchGeometryExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
