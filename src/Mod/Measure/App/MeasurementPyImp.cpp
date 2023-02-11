/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#include <App/Application.h>
#include <App/Document.h>
#include <Base/GeometryPyCXX.h>

// inclusion of the generated files (generated out of SketchObjectSFPy.xml)
#include "MeasurementPy.h"
#include "MeasurementPy.cpp"


using namespace Measure;

// returns a string which represents the object e.g. when printed in python
std::string MeasurementPy::representation() const
{
    return "<Measure::Measurement>";
}

PyObject *MeasurementPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of BoundBoxPy and the Twin object
    return new MeasurementPy(new Measurement);
}

// constructor method
int MeasurementPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
   return 0;
}

PyObject* MeasurementPy::addReference3D(PyObject *args)
{
    char *ObjectName;
    char *SubName;
    if (!PyArg_ParseTuple(args, "ss:Give an object and subelement name", &ObjectName,&SubName))
        return nullptr;

    // get the target object for the external link
    App::DocumentObject * Obj = App::GetApplication().getActiveDocument()->getObject(ObjectName);
    if (!Obj) {
        std::stringstream str;
        str << ObjectName << "does not exist in the document";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return nullptr;
    }

    // add the external
    if (this->getMeasurementPtr()->addReference3D(Obj,SubName) < 0) {
        std::stringstream str;
        str << "Not able to add reference";
        PyErr_SetString(PyExc_ValueError, str.str().c_str());
        return nullptr;
    }

    Py_Return;
}

PyObject* MeasurementPy::has3DReferences(PyObject *args)
{
    PyObject *result=Py_False;
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    if (getMeasurementPtr()->has3DReferences()) {
        result = Py_True;
    }

    Py_IncRef(result);
    return result;
}

PyObject* MeasurementPy::clear(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    this->getMeasurementPtr()->clear();

    Py_Return;
}

PyObject* MeasurementPy::delta(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Py::Vector delta(this->getMeasurementPtr()->delta());

    return Py::new_reference_to(delta);
}

PyObject* MeasurementPy::length(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Py::Float length;
    length = this->getMeasurementPtr()->length();

    return Py::new_reference_to(length);
}

PyObject* MeasurementPy::radius(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Py::Float radius;
    radius = this->getMeasurementPtr()->radius();

    return Py::new_reference_to(radius);
}

PyObject* MeasurementPy::angle(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Py::Float angle;
    angle = this->getMeasurementPtr()->angle();

    return Py::new_reference_to(angle);
}

PyObject* MeasurementPy::com(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Py::Vector com(this->getMeasurementPtr()->massCenter());

    return Py::new_reference_to(com);
}

PyObject *MeasurementPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MeasurementPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
