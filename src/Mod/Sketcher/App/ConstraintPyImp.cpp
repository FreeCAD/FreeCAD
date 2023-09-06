/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <sstream>
#endif

#include <Base/QuantityPy.h>

#include "ConstraintPy.h"

#include "ConstraintPy.cpp"


using namespace Sketcher;

PyObject* ConstraintPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
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

    char* ConstraintType;
    int FirstIndex = GeoEnum::GeoUndef;
    int FirstPos = static_cast<int>(PointPos::none);
    int SecondIndex = GeoEnum::GeoUndef;
    int SecondPos = static_cast<int>(PointPos::none);
    int ThirdIndex = GeoEnum::GeoUndef;
    int ThirdPos = static_cast<int>(PointPos::none);
    double Value = 0;
    int intArg1, intArg2, intArg3, intArg4, intArg5;
    // Note: In Python 2.x PyArg_ParseTuple prints a warning if a float is given but an integer is
    // expected. This means we must use a PyObject and check afterwards if it's a float or integer.
    PyObject* index_or_value;
    PyObject* oNumArg4;
    PyObject* oNumArg5;
    int any_index;

    // ConstraintType, GeoIndex
    if (PyArg_ParseTuple(args, "si", &ConstraintType, &FirstIndex)) {
        if (strcmp("Horizontal", ConstraintType) == 0) {
            this->getConstraintPtr()->Type = Horizontal;
            this->getConstraintPtr()->First = FirstIndex;
            return 0;
        }
        else if (strcmp("Vertical", ConstraintType) == 0) {
            this->getConstraintPtr()->Type = Vertical;
            this->getConstraintPtr()->First = FirstIndex;
            return 0;
        }
        else if (strcmp("Block", ConstraintType) == 0) {
            this->getConstraintPtr()->Type = Block;
            this->getConstraintPtr()->First = FirstIndex;
            return 0;
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args, "siO", &ConstraintType, &FirstIndex, &index_or_value)) {
        // ConstraintType, GeoIndex1, GeoIndex2
        if (PyLong_Check(index_or_value)) {
            SecondIndex = PyLong_AsLong(index_or_value);
            bool valid = false;
            if (strcmp("Tangent", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Tangent;
                valid = true;
            }
            else if (strcmp("Parallel", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Parallel;
                valid = true;
            }
            else if (strcmp("Perpendicular", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Perpendicular;
                valid = true;
            }
            else if (strcmp("Equal", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Equal;
                valid = true;
            }
            else if (strstr(ConstraintType, "InternalAlignment")) {
                this->getConstraintPtr()->Type = InternalAlignment;

                valid = true;
                if (strstr(ConstraintType, "EllipseMajorDiameter")) {
                    this->getConstraintPtr()->AlignmentType = EllipseMajorDiameter;
                }
                else if (strstr(ConstraintType, "EllipseMinorDiameter")) {
                    this->getConstraintPtr()->AlignmentType = EllipseMinorDiameter;
                }
                else {
                    this->getConstraintPtr()->AlignmentType = Undef;
                    valid = false;
                }
            }

            if (valid) {
                this->getConstraintPtr()->First = FirstIndex;
                this->getConstraintPtr()->Second = SecondIndex;
                return 0;
            }
        }
        // ConstraintType, GeoIndex, Value
        if (PyNumber_Check(index_or_value)) {  // can be float or int
            Value = PyFloat_AsDouble(index_or_value);
            bool valid = false;
            if (strcmp("Distance", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Distance;
                valid = true;
            }
            else if (strcmp("Angle", ConstraintType) == 0) {
                if (PyObject_TypeCheck(index_or_value, &(Base::QuantityPy::Type))) {
                    Base::Quantity q =
                        *(static_cast<Base::QuantityPy*>(index_or_value)->getQuantityPtr());
                    if (q.getUnit() == Base::Unit::Angle) {
                        Value = q.getValueAs(Base::Quantity::Radian);
                    }
                }
                this->getConstraintPtr()->Type = Angle;
                valid = true;
            }
            else if (strcmp("DistanceX", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = DistanceX;
                valid = true;
            }
            else if (strcmp("DistanceY", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = DistanceY;
                valid = true;
            }
            else if (strcmp("Radius", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Radius;
                // set a value that is out of range of result of atan2
                // this value is handled in ViewProviderSketch
                this->getConstraintPtr()->LabelPosition = 10;
                valid = true;
            }
            else if (strcmp("Diameter", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Diameter;
                // set a value that is out of range of result of atan2
                // this value is handled in ViewProviderSketch
                this->getConstraintPtr()->LabelPosition = 10;
                valid = true;
            }
            else if (strcmp("Weight", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Weight;
                // set a value that is out of range of result of atan2
                // this value is handled in ViewProviderSketch
                this->getConstraintPtr()->LabelPosition = 10;
                valid = true;
            }
            if (valid) {
                this->getConstraintPtr()->First = FirstIndex;
                this->getConstraintPtr()->setValue(Value);
                return 0;
            }
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args, "siiO", &ConstraintType, &FirstIndex, &any_index, &index_or_value)) {
        // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2
        if (PyLong_Check(index_or_value)) {
            FirstPos = any_index;
            SecondIndex = PyLong_AsLong(index_or_value);
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
            else if (strstr(ConstraintType, "InternalAlignment")) {
                this->getConstraintPtr()->Type = InternalAlignment;

                valid = true;

                if (strstr(ConstraintType, "EllipseFocus1")) {
                    this->getConstraintPtr()->AlignmentType = EllipseFocus1;
                }
                else if (strstr(ConstraintType, "EllipseFocus2")) {
                    this->getConstraintPtr()->AlignmentType = EllipseFocus2;
                }
                else {
                    this->getConstraintPtr()->AlignmentType = Undef;
                    valid = false;
                }
            }
            if (valid) {
                this->getConstraintPtr()->First = FirstIndex;
                this->getConstraintPtr()->FirstPos = static_cast<Sketcher::PointPos>(FirstPos);
                this->getConstraintPtr()->Second = SecondIndex;
                return 0;
            }
        }
        // ConstraintType, GeoIndex1, GeoIndex2, Value
        // ConstraintType, GeoIndex, PosIndex, Value
        if (PyNumber_Check(index_or_value)) {  // can be float or int
            SecondIndex = any_index;
            Value = PyFloat_AsDouble(index_or_value);
            if (strcmp("Angle", ConstraintType) == 0) {
                if (PyObject_TypeCheck(index_or_value, &(Base::QuantityPy::Type))) {
                    Base::Quantity q =
                        *(static_cast<Base::QuantityPy*>(index_or_value)->getQuantityPtr());
                    if (q.getUnit() == Base::Unit::Angle) {
                        Value = q.getValueAs(Base::Quantity::Radian);
                    }
                }
                this->getConstraintPtr()->Type = Angle;
                this->getConstraintPtr()->First = FirstIndex;
                this->getConstraintPtr()->Second = SecondIndex;
                this->getConstraintPtr()->setValue(Value);
                return 0;
            }
            else if (strcmp("Distance", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Distance;
                this->getConstraintPtr()->First = FirstIndex;
                this->getConstraintPtr()->Second = SecondIndex;
                this->getConstraintPtr()->setValue(Value);
                return 0;
            }
            else if (strcmp("DistanceX", ConstraintType) == 0) {
                FirstPos = SecondIndex;
                SecondIndex = -1;
                this->getConstraintPtr()->Type = DistanceX;
                this->getConstraintPtr()->First = FirstIndex;
                this->getConstraintPtr()->FirstPos = static_cast<Sketcher::PointPos>(FirstPos);
                this->getConstraintPtr()->setValue(Value);
                return 0;
            }
            else if (strcmp("DistanceY", ConstraintType) == 0) {
                FirstPos = SecondIndex;
                SecondIndex = -1;
                this->getConstraintPtr()->Type = DistanceY;
                this->getConstraintPtr()->First = FirstIndex;
                this->getConstraintPtr()->FirstPos = static_cast<Sketcher::PointPos>(FirstPos);
                this->getConstraintPtr()->setValue(Value);
                return 0;
            }
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args, "siiiO", &ConstraintType, &intArg1, &intArg2, &intArg3, &oNumArg4)) {
        // Value, ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, PosIndex2
        if (PyLong_Check(oNumArg4)) {
            intArg4 = PyLong_AsLong(oNumArg4);
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
            else if (strcmp("TangentViaPoint", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Tangent;
                // valid = true;//non-standard assignment
                this->getConstraintPtr()->First = intArg1;
                this->getConstraintPtr()->FirstPos = Sketcher::PointPos::none;
                this->getConstraintPtr()->Second = intArg2;
                this->getConstraintPtr()->SecondPos = Sketcher::PointPos::none;
                this->getConstraintPtr()->Third = intArg3;
                this->getConstraintPtr()->ThirdPos = static_cast<Sketcher::PointPos>(intArg4);
                return 0;
            }
            else if (strcmp("PerpendicularViaPoint", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Perpendicular;
                // valid = true;//non-standard assignment
                this->getConstraintPtr()->First = intArg1;
                this->getConstraintPtr()->FirstPos = Sketcher::PointPos::none;
                this->getConstraintPtr()->Second = intArg2;
                this->getConstraintPtr()->SecondPos = Sketcher::PointPos::none;
                this->getConstraintPtr()->Third = intArg3;
                this->getConstraintPtr()->ThirdPos = static_cast<Sketcher::PointPos>(intArg4);
                return 0;
            }
            else if (strstr(ConstraintType,
                            "InternalAlignment")) {  // InteralAlignment with
                                                     // InternalElementIndex argument
                this->getConstraintPtr()->Type = InternalAlignment;

                valid = true;

                if (strstr(ConstraintType, "BSplineControlPoint")) {
                    this->getConstraintPtr()->AlignmentType = BSplineControlPoint;
                }
                else if (strstr(ConstraintType, "BSplineKnotPoint")) {
                    this->getConstraintPtr()->AlignmentType = BSplineKnotPoint;
                }
                else {
                    this->getConstraintPtr()->AlignmentType = Undef;
                    valid = false;
                }

                if (valid) {
                    this->getConstraintPtr()->First = intArg1;
                    this->getConstraintPtr()->FirstPos = static_cast<Sketcher::PointPos>(intArg2);
                    this->getConstraintPtr()->Second = intArg3;
                    this->getConstraintPtr()->InternalAlignmentIndex = intArg4;
                    return 0;
                }
            }
            if (valid) {
                this->getConstraintPtr()->First = intArg1;
                this->getConstraintPtr()->FirstPos = static_cast<Sketcher::PointPos>(intArg2);
                this->getConstraintPtr()->Second = intArg3;
                this->getConstraintPtr()->SecondPos = static_cast<Sketcher::PointPos>(intArg4);
                return 0;
            }
        }
        // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, Value
        if (PyNumber_Check(oNumArg4)) {  // can be float or int
            Value = PyFloat_AsDouble(oNumArg4);
            if (strcmp("Distance", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Distance;
                this->getConstraintPtr()->First = intArg1;
                this->getConstraintPtr()->FirstPos = static_cast<Sketcher::PointPos>(intArg2);
                this->getConstraintPtr()->Second = intArg3;
                this->getConstraintPtr()->setValue(Value);
                return 0;
            }
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args,
                         "siiiiO",
                         &ConstraintType,
                         &intArg1,
                         &intArg2,
                         &intArg3,
                         &intArg4,
                         &oNumArg5)) {
        // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, PosIndex2, GeoIndex3
        if (PyLong_Check(oNumArg5)) {
            intArg5 = PyLong_AsLong(oNumArg5);
            if (strcmp("Symmetric", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Symmetric;
                this->getConstraintPtr()->First = intArg1;
                this->getConstraintPtr()->FirstPos = static_cast<Sketcher::PointPos>(intArg2);
                this->getConstraintPtr()->Second = intArg3;
                this->getConstraintPtr()->SecondPos = static_cast<Sketcher::PointPos>(intArg4);
                this->getConstraintPtr()->Third = intArg5;
                return 0;
            }
        }
        // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, PosIndex2, Value
        if (PyNumber_Check(oNumArg5)) {  // can be float or int
            Value = PyFloat_AsDouble(oNumArg5);
            bool valid = false;
            if (strcmp("Distance", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Distance;
                valid = true;
            }
            else if (strcmp("DistanceX", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = DistanceX;
                valid = true;
            }
            else if (strcmp("DistanceY", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = DistanceY;
                valid = true;
            }
            else if (strcmp("Angle", ConstraintType) == 0) {
                if (PyObject_TypeCheck(oNumArg5, &(Base::QuantityPy::Type))) {
                    Base::Quantity q =
                        *(static_cast<Base::QuantityPy*>(oNumArg5)->getQuantityPtr());
                    if (q.getUnit() == Base::Unit::Angle) {
                        Value = q.getValueAs(Base::Quantity::Radian);
                    }
                }
                this->getConstraintPtr()->Type = Angle;
                valid = true;
            }
            else if (strcmp("AngleViaPoint", ConstraintType) == 0) {
                if (PyObject_TypeCheck(oNumArg5, &(Base::QuantityPy::Type))) {
                    Base::Quantity q =
                        *(static_cast<Base::QuantityPy*>(oNumArg5)->getQuantityPtr());
                    if (q.getUnit() == Base::Unit::Angle) {
                        Value = q.getValueAs(Base::Quantity::Radian);
                    }
                }
                this->getConstraintPtr()->Type = Angle;
                // valid = true;//non-standard assignment
                this->getConstraintPtr()->First = intArg1;
                this->getConstraintPtr()->FirstPos = Sketcher::PointPos::none;
                this->getConstraintPtr()->Second = intArg2;  // let's goof up all the terminology =)
                this->getConstraintPtr()->SecondPos = Sketcher::PointPos::none;
                this->getConstraintPtr()->Third = intArg3;
                this->getConstraintPtr()->ThirdPos = static_cast<Sketcher::PointPos>(intArg4);
                this->getConstraintPtr()->setValue(Value);
                return 0;
            }
            if (valid) {
                this->getConstraintPtr()->First = intArg1;
                this->getConstraintPtr()->FirstPos = static_cast<Sketcher::PointPos>(intArg2);
                this->getConstraintPtr()->Second = intArg3;
                this->getConstraintPtr()->SecondPos = static_cast<Sketcher::PointPos>(intArg4);
                this->getConstraintPtr()->setValue(Value);
                return 0;
            }
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args,
                         "siiiiiO",
                         &ConstraintType,
                         &FirstIndex,
                         &FirstPos,
                         &SecondIndex,
                         &SecondPos,
                         &ThirdIndex,
                         &index_or_value)) {
        if (PyLong_Check(index_or_value)) {
            ThirdPos = PyLong_AsLong(index_or_value);
            // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, PosIndex2, GeoIndex3, PosIndex3
            if (strcmp("Symmetric", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = Symmetric;
                this->getConstraintPtr()->First = FirstIndex;
                this->getConstraintPtr()->FirstPos = static_cast<Sketcher::PointPos>(FirstPos);
                this->getConstraintPtr()->Second = SecondIndex;
                this->getConstraintPtr()->SecondPos = static_cast<Sketcher::PointPos>(SecondPos);
                this->getConstraintPtr()->Third = ThirdIndex;
                this->getConstraintPtr()->ThirdPos = static_cast<Sketcher::PointPos>(ThirdPos);
                return 0;
            }
        }
        if (PyNumber_Check(index_or_value)) {  // can be float or int
            Value = PyFloat_AsDouble(index_or_value);
            if (strcmp("SnellsLaw", ConstraintType) == 0) {
                this->getConstraintPtr()->Type = SnellsLaw;
                this->getConstraintPtr()->First = FirstIndex;
                this->getConstraintPtr()->FirstPos = static_cast<Sketcher::PointPos>(FirstPos);
                this->getConstraintPtr()->Second = SecondIndex;
                this->getConstraintPtr()->SecondPos = static_cast<Sketcher::PointPos>(SecondPos);
                this->getConstraintPtr()->Third = ThirdIndex;
                this->getConstraintPtr()->ThirdPos = Sketcher::PointPos::none;
                this->getConstraintPtr()->setValue(Value);
                return 0;
            }
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
std::string ConstraintPy::representation() const
{
    std::stringstream result;
    result << "<Constraint ";
    switch (this->getConstraintPtr()->Type) {
        case None:
            result << "'None'>";
            break;
        case DistanceX:
            result << "'DistanceX'>";
            break;
        case DistanceY:
            result << "'DistanceY'>";
            break;
        case Coincident:
            result << "'Coincident'>";
            break;
        case Horizontal:
            result << "'Horizontal' (" << getConstraintPtr()->First << ")>";
            break;
        case Vertical:
            result << "'Vertical' (" << getConstraintPtr()->First << ")>";
            break;
        case Block:
            result << "'Block' (" << getConstraintPtr()->First << ")>";
            break;
        case Radius:
            result << "'Radius'>";
            break;
        case Diameter:
            result << "'Diameter'>";
            break;
        case Weight:
            result << "'Weight'>";
            break;
        case Parallel:
            result << "'Parallel'>";
            break;
        case Tangent:
            if (this->getConstraintPtr()->Third == GeoEnum::GeoUndef) {
                result << "'Tangent'>";
            }
            else {
                result << "'TangentViaPoint'>";
            }
            break;
        case Perpendicular:
            if (this->getConstraintPtr()->Third == GeoEnum::GeoUndef) {
                result << "'Perpendicular'>";
            }
            else {
                result << "'PerpendicularViaPoint'>";
            }
            break;
        case Distance:
            result << "'Distance'>";
            break;
        case Angle:
            if (this->getConstraintPtr()->Third == GeoEnum::GeoUndef) {
                result << "'Angle'>";
            }
            else {
                result << "'AngleViaPoint'>";
            }
            break;
        case Symmetric:
            result << "'Symmetric'>";
            break;
        case SnellsLaw:
            result << "'SnellsLaw'>";
            break;
        case InternalAlignment:
            switch (this->getConstraintPtr()->AlignmentType) {
                case Undef:
                    result << "'InternalAlignment:Undef'>";
                    break;
                case EllipseMajorDiameter:
                    result << "'InternalAlignment:EllipseMajorDiameter'>";
                    break;
                case EllipseMinorDiameter:
                    result << "'InternalAlignment:EllipseMinorDiameter'>";
                    break;
                case EllipseFocus1:
                    result << "'InternalAlignment:EllipseFocus1'>";
                    break;
                case EllipseFocus2:
                    result << "'InternalAlignment:EllipseFocus2'>";
                    break;
                default:
                    result << "'InternalAlignment:?'>";
                    break;
            }
            break;
        case Equal:
            result << "'Equal' (" << getConstraintPtr()->First << "," << getConstraintPtr()->Second
                   << ")>";
            break;
        case PointOnObject:
            result << "'PointOnObject' (" << getConstraintPtr()->First << ","
                   << getConstraintPtr()->Second << ")>";
            break;
        default:
            result << "'?'>";
            break;
    }
    return result.str();
}

Py::String ConstraintPy::getType() const
{
    switch (this->getConstraintPtr()->Type) {
        case None:
            return Py::String("None");
            break;
        case DistanceX:
            return Py::String("DistanceX");
            break;
        case DistanceY:
            return Py::String("DistanceY");
            break;
        case Coincident:
            return Py::String("Coincident");
            break;
        case Horizontal:
            return Py::String("Horizontal");
            break;
        case Vertical:
            return Py::String("Vertical");
            break;
        case Block:
            return Py::String("Block");
            break;
        case Radius:
            return Py::String("Radius");
            break;
        case Diameter:
            return Py::String("Diameter");
            break;
        case Weight:
            return Py::String("Weight");
            break;
        case Parallel:
            return Py::String("Parallel");
            break;
        case Tangent:
            return Py::String("Tangent");
            break;
        case Perpendicular:
            return Py::String("Perpendicular");
            break;
        case Distance:
            return Py::String("Distance");
            break;
        case Angle:
            return Py::String("Angle");
            break;
        case Symmetric:
            return Py::String("Symmetric");
            break;
        case SnellsLaw:
            return Py::String("SnellsLaw");
            break;
        case InternalAlignment:
            return Py::String("InternalAlignment");
            break;
        case Equal:
            return Py::String("Equal");
            break;
        case PointOnObject:
            return Py::String("PointOnObject");
            break;
        default:
            return Py::String("Undefined");
            break;
    }
}

Py::Long ConstraintPy::getFirst() const
{
    return Py::Long(this->getConstraintPtr()->First);
}

void ConstraintPy::setFirst(Py::Long arg)
{
    this->getConstraintPtr()->First = arg;
}

Py::Long ConstraintPy::getFirstPos() const
{
    return Py::Long(static_cast<int>(this->getConstraintPtr()->FirstPos));
}

void ConstraintPy::setFirstPos(Py::Long arg)
{
    int pos = arg;

    if (pos >= static_cast<int>(Sketcher::PointPos::none)
        && pos <= static_cast<int>(Sketcher::PointPos::mid)) {
        this->getConstraintPtr()->FirstPos = static_cast<Sketcher::PointPos>(pos);
    }
    else {
        std::stringstream str;
        str << "Invalid PointPos parameter: " << arg << std::endl;

        PyErr_SetString(PyExc_TypeError, str.str().c_str());
    }
}

Py::Long ConstraintPy::getSecond() const
{
    return Py::Long(this->getConstraintPtr()->Second);
}

void ConstraintPy::setSecond(Py::Long arg)
{
    this->getConstraintPtr()->Second = arg;
}

Py::Long ConstraintPy::getSecondPos() const
{
    return Py::Long(static_cast<int>(this->getConstraintPtr()->SecondPos));
}

void ConstraintPy::setSecondPos(Py::Long arg)
{
    int pos = arg;

    if (pos >= static_cast<int>(Sketcher::PointPos::none)
        && pos <= static_cast<int>(Sketcher::PointPos::mid)) {
        this->getConstraintPtr()->SecondPos = static_cast<Sketcher::PointPos>(pos);
    }
    else {
        std::stringstream str;
        str << "Invalid PointPos parameter: " << arg << std::endl;

        PyErr_SetString(PyExc_TypeError, str.str().c_str());
    }
}

Py::Long ConstraintPy::getThird() const
{
    return Py::Long(this->getConstraintPtr()->Third);
}

void ConstraintPy::setThird(Py::Long arg)
{
    this->getConstraintPtr()->Third = arg;
}

Py::Long ConstraintPy::getThirdPos() const
{
    return Py::Long(static_cast<int>(this->getConstraintPtr()->ThirdPos));
}

void ConstraintPy::setThirdPos(Py::Long arg)
{
    int pos = arg;

    if (pos >= static_cast<int>(Sketcher::PointPos::none)
        && pos <= static_cast<int>(Sketcher::PointPos::mid)) {
        this->getConstraintPtr()->ThirdPos = static_cast<Sketcher::PointPos>(pos);
    }
    else {
        std::stringstream str;
        str << "Invalid PointPos parameter: " << arg << std::endl;

        PyErr_SetString(PyExc_TypeError, str.str().c_str());
    }
}

Py::String ConstraintPy::getName() const
{
    return Py::String(this->getConstraintPtr()->Name);
}

void ConstraintPy::setName(Py::String arg)
{
    this->getConstraintPtr()->Name = arg;
}

Py::Float ConstraintPy::getValue() const
{
    return Py::Float(this->getConstraintPtr()->getValue());
}

Py::Boolean ConstraintPy::getDriving() const
{
    return Py::Boolean(this->getConstraintPtr()->isDriving);
}

Py::Boolean ConstraintPy::getInVirtualSpace() const
{
    return Py::Boolean(this->getConstraintPtr()->isInVirtualSpace);
}

Py::Boolean ConstraintPy::getIsActive() const
{
    return Py::Boolean(this->getConstraintPtr()->isActive);
}

PyObject* ConstraintPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ConstraintPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
