/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2010     *
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
#include <sstream>
#include "Mod/Sketcher/App/Constraint.h"

// inclusion of the generated files (generated out of ConstraintPy.xml)
#include "ConstraintPy.h"
#include "ConstraintPy.cpp"

using namespace Sketcher;

PyObject *ConstraintPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ConstraintPy and the Twin object 
    return new ConstraintPy(new Constraint);
}

// constructor method
int ConstraintPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();

    char *ConstraintType;
    int  FirstIndex = Constraint::GeoUndef;
    int  FirstPos   = none;
    int  SecondIndex= Constraint::GeoUndef;
    int  SecondPos  = none;
    int  ThirdIndex = Constraint::GeoUndef;
    int  ThirdPos   = none;
    double Value    = 0;
    // Note: In Python 2.x PyArg_ParseTuple prints a warning if a float is given but an integer is expected.
    // This means we must use a PyObject and check afterwards if it's a float or integer.
    PyObject* index_or_value;
    int any_index;

    // ConstraintType, GeoIndex
    if (PyArg_ParseTuple(args, "si", &ConstraintType, &FirstIndex)) {
        if (strcmp("Horizontal",ConstraintType) == 0) {
            this->getConstraintPtr()->Type = Horizontal;
            this->getConstraintPtr()->First = FirstIndex;
            return 0;
        }
        else if (strcmp("Vertical",ConstraintType) == 0) {
            this->getConstraintPtr()->Type = Vertical;
            this->getConstraintPtr()->First = FirstIndex;
            return 0;
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args, "siO", &ConstraintType, &FirstIndex, &index_or_value)) {
        // ConstraintType, GeoIndex1, GeoIndex2
        if (PyInt_Check(index_or_value)) {
            SecondIndex = PyInt_AsLong(index_or_value);
            bool valid = false;
            if (strcmp("Tangent",ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Tangent;
                valid = true;
            }
            else if (strcmp("Parallel",ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Parallel;
                valid = true;
            }
            else if (strcmp("Perpendicular",ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Perpendicular;
                valid = true;
            }
            else if (strcmp("Equal",ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Equal;
                valid = true;
            }
            if (valid) {
                this->getConstraintPtr()->First = FirstIndex;
                this->getConstraintPtr()->Second = SecondIndex;
                return 0;
            }
        }
        // ConstraintType, GeoIndex, Value
        if (PyNumber_Check(index_or_value)) { // can be float or int
            Value = PyFloat_AsDouble(index_or_value);
            bool valid = false;
            if (strcmp("Distance",ConstraintType) == 0 ) {
                this->getConstraintPtr()->Type = Distance;
                valid = true;
            }
            else if (strcmp("Angle",ConstraintType) == 0 ) {
                this->getConstraintPtr()->Type = Angle;
                valid = true;
            }
            else if (strcmp("DistanceX",ConstraintType) == 0) {
                this->getConstraintPtr()->Type = DistanceX;
                valid = true;
            }
            else if (strcmp("DistanceY",ConstraintType) == 0) {
                this->getConstraintPtr()->Type = DistanceY;
                valid = true;
            }
            else if (strcmp("Radius",ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Radius;
                valid = true;
            }
            if (valid) {
                this->getConstraintPtr()->First    = FirstIndex;
                this->getConstraintPtr()->Value    = Value;
                return 0;
            }
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args, "siiO", &ConstraintType, &FirstIndex, &any_index, &index_or_value)) {
        // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2
        if (PyInt_Check(index_or_value)) {
            FirstPos = any_index;
            SecondIndex = PyInt_AsLong(index_or_value);
            bool valid = false;
            if (strcmp("Perpendicular", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Perpendicular;
                valid = true;
            }
            else if (strcmp("Tangent", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Tangent;
                valid = true;
            }
            else if (strcmp("PointOnObject", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = PointOnObject;
                valid = true;
            }
            if (valid) {
                this->getConstraintPtr()->First    = FirstIndex;
                this->getConstraintPtr()->FirstPos = (Sketcher::PointPos) FirstPos;
                this->getConstraintPtr()->Second   = SecondIndex;
                return 0;
            }
        }
        // ConstraintType, GeoIndex1, GeoIndex2, Value
        // ConstraintType, GeoIndex, PosIndex, Value
        if (PyNumber_Check(index_or_value)) { // can be float or int
            SecondIndex = any_index;
            Value = PyFloat_AsDouble(index_or_value);
            //if (strcmp("Distance",ConstraintType) == 0) {
            //    this->getConstraintPtr()->Type   = Distance;
            //    this->getConstraintPtr()->First  = FirstIndex;
            //    this->getConstraintPtr()->Second = SecondIndex;
            //    this->getConstraintPtr()->Value  = Value;
            //    return 0;
            //}
            //else
            if (strcmp("Angle",ConstraintType) == 0) {
                this->getConstraintPtr()->Type   = Angle;
                this->getConstraintPtr()->First  = FirstIndex;
                this->getConstraintPtr()->Second = SecondIndex;
                this->getConstraintPtr()->Value  = Value;
                return 0;
            }
            else if (strcmp("DistanceX",ConstraintType) == 0) {
                FirstPos = SecondIndex;
                SecondIndex = -1;
                this->getConstraintPtr()->Type = DistanceX;
                this->getConstraintPtr()->First    = FirstIndex;
                this->getConstraintPtr()->FirstPos = (Sketcher::PointPos) FirstPos;
                this->getConstraintPtr()->Value    = Value;
                return 0;
            }
            else if (strcmp("DistanceY",ConstraintType) == 0) {
                FirstPos = SecondIndex;
                SecondIndex = -1;
                this->getConstraintPtr()->Type = DistanceY;
                this->getConstraintPtr()->First    = FirstIndex;
                this->getConstraintPtr()->FirstPos = (Sketcher::PointPos) FirstPos;
                this->getConstraintPtr()->Value    = Value;
                return 0;
            }
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args, "siiiO", &ConstraintType, &FirstIndex, &FirstPos, &SecondIndex, &index_or_value)) {
        // Value, ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, PosIndex2
        if (PyInt_Check(index_or_value)) {
            SecondPos = PyInt_AsLong(index_or_value);
            bool valid = false;
            if (strcmp("Coincident", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Coincident;
                valid = true;
            }
            else if (strcmp("Horizontal", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Horizontal;
                valid = true;
            }
            else if (strcmp("Vertical", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Vertical;
                valid = true;
            }
            else if (strcmp("Perpendicular", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Perpendicular;
                valid = true;
            }
            else if (strcmp("Tangent", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Tangent;
                valid = true;
            }
            if (valid) {
                this->getConstraintPtr()->First     = FirstIndex;
                this->getConstraintPtr()->FirstPos  = (Sketcher::PointPos) FirstPos;
                this->getConstraintPtr()->Second    = SecondIndex;
                this->getConstraintPtr()->SecondPos = (Sketcher::PointPos) SecondPos;
                return 0;
            }
        }
        // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, Value
        if (PyNumber_Check(index_or_value)) { // can be float or int
            Value = PyFloat_AsDouble(index_or_value);
            if (strcmp("Distance",ConstraintType) == 0 ) {
                this->getConstraintPtr()->Type = Distance;
                this->getConstraintPtr()->First    = FirstIndex;
                this->getConstraintPtr()->FirstPos = (Sketcher::PointPos) FirstPos;
                this->getConstraintPtr()->Second   = SecondIndex;
                this->getConstraintPtr()->Value    = Value;
                return 0;
            }
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args, "siiiiO", &ConstraintType, &FirstIndex, &FirstPos, &SecondIndex, &SecondPos, &index_or_value)) {
        // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, PosIndex2, GeoIndex3
        if (PyInt_Check(index_or_value)) {
            ThirdIndex = PyInt_AsLong(index_or_value);
            if (strcmp("Symmetric",ConstraintType) == 0 ) {
                this->getConstraintPtr()->Type = Symmetric;
                this->getConstraintPtr()->First     = FirstIndex;
                this->getConstraintPtr()->FirstPos  = (Sketcher::PointPos) FirstPos;
                this->getConstraintPtr()->Second    = SecondIndex;
                this->getConstraintPtr()->SecondPos = (Sketcher::PointPos) SecondPos;
                this->getConstraintPtr()->Third     = ThirdIndex;
                return 0;
            }
        }
        // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, PosIndex2, Value
        if (PyNumber_Check(index_or_value)) { // can be float or int
            Value = PyFloat_AsDouble(index_or_value);
            bool valid=false;
            if (strcmp("Distance",ConstraintType) == 0 ) {
                this->getConstraintPtr()->Type = Distance;
                valid = true;
            }
            else if (strcmp("DistanceX",ConstraintType) == 0) {
                this->getConstraintPtr()->Type = DistanceX;
                valid = true;
            }
            else if (strcmp("DistanceY",ConstraintType) == 0) {
                this->getConstraintPtr()->Type = DistanceY;
                valid = true;
            }
            else if (strcmp("Angle",ConstraintType) == 0 ) {
                this->getConstraintPtr()->Type = Angle;
                valid = true;
            }
            if (valid) {
                this->getConstraintPtr()->First     = FirstIndex;
                this->getConstraintPtr()->FirstPos  = (Sketcher::PointPos) FirstPos;
                this->getConstraintPtr()->Second    = SecondIndex;
                this->getConstraintPtr()->SecondPos = (Sketcher::PointPos) SecondPos;
                this->getConstraintPtr()->Value     = Value;
                return 0;
            }
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args, "siiiiii", &ConstraintType, &FirstIndex, &FirstPos, &SecondIndex, &SecondPos, &ThirdIndex, &ThirdPos)) {
        // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, PosIndex2, GeoIndex3, PosIndex3
        if (strcmp("Symmetric",ConstraintType) == 0 ) {
            this->getConstraintPtr()->Type = Symmetric;
            this->getConstraintPtr()->First     = FirstIndex;
            this->getConstraintPtr()->FirstPos  = (Sketcher::PointPos) FirstPos;
            this->getConstraintPtr()->Second    = SecondIndex;
            this->getConstraintPtr()->SecondPos = (Sketcher::PointPos) SecondPos;
            this->getConstraintPtr()->Third     = ThirdIndex;
            this->getConstraintPtr()->ThirdPos  = (Sketcher::PointPos) ThirdPos;
            return 0;
        }
    }

    std::stringstream str;
    str << "Invalid parameters: ";
    Py::Tuple tuple(args);
    str << tuple.as_string() << std::endl;
    str << "Constraint constructor accepts:" << std::endl
        << "-- empty parameter list" << std::endl
        << "-- Constraint type and index" << std::endl;

    PyErr_SetString(PyExc_TypeError, str.str().c_str());
    return -1;
}

// returns a string which represents the object e.g. when printed in python
std::string ConstraintPy::representation(void) const
{
    std::stringstream result;
    result << "<Constraint " ;
    switch(this->getConstraintPtr()->Type) {
        case None       : result << "'None'>";break;
        case DistanceX  : result << "'DistanceX'>";break;
        case DistanceY  : result << "'DistanceY'>";break;
        case Coincident : result << "'Coincident'>";break;
        case Horizontal : result << "'Horizontal' (" << getConstraintPtr()->First << ")>";break;
        case Vertical   : result << "'Vertical' (" << getConstraintPtr()->First << ")>";break;
        case Parallel   : result << "'Parallel'>";break;
        case Tangent    : result << "'Tangent'>";break;
        case Distance   : result << "'Distance'>";break;
        case Angle      : result << "'Angle'>";break;
        default         : result << "'?'>";break;
    }
    return result.str();
}

Py::Int ConstraintPy::getFirst(void) const
{
    return Py::Int(this->getConstraintPtr()->First);
}

void  ConstraintPy::setFirst(Py::Int arg)
{
    this->getConstraintPtr()->First = arg;
}

Py::Int ConstraintPy::getSecond(void) const
{
    return Py::Int(this->getConstraintPtr()->Second);
}

void  ConstraintPy::setSecond(Py::Int arg)
{
    this->getConstraintPtr()->Second = arg;
}

PyObject *ConstraintPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ConstraintPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
