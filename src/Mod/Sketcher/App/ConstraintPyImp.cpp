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

    PyObject* activated;
    PyObject* driving;

    Sketcher::Constraint* constraint = this->getConstraintPtr();

    auto handleSi = [&]() -> bool {
        if (strcmp("Horizontal", ConstraintType) == 0) {
            constraint->Type = Horizontal;
        }
        else if (strcmp("Vertical", ConstraintType) == 0) {
            constraint->Type = Vertical;
        }
        else if (strcmp("Block", ConstraintType) == 0) {
            constraint->Type = Block;
        }
        else {
            return false;
        }
        constraint->First = FirstIndex;
        return true;
    };

    // ConstraintType, GeoIndex
    if (PyArg_ParseTuple(args, "si", &ConstraintType, &FirstIndex)) {
        if (handleSi()) {
            return 0;
        }
    }
    PyErr_Clear();

    // ConstraintType, GeoIndex, activated
    if (PyArg_ParseTuple(args, "siO", &ConstraintType, &FirstIndex, &activated)) {
        if (PyBool_Check(activated)) {
            if (handleSi()) {
                constraint->isActive = PyObject_IsTrue(activated);
                return 0;
            }
        }
    }
    PyErr_Clear();


    auto handleSiO = [&]() -> bool {
        // ConstraintType, GeoIndex1, GeoIndex2
        if (PyLong_Check(index_or_value)) {
            SecondIndex = PyLong_AsLong(index_or_value);
            bool valid = false;
            if (strcmp("Tangent", ConstraintType) == 0) {
                constraint->Type = Tangent;
                valid = true;
            }
            else if (strcmp("Parallel", ConstraintType) == 0) {
                constraint->Type = Parallel;
                valid = true;
            }
            else if (strcmp("Perpendicular", ConstraintType) == 0) {
                constraint->Type = Perpendicular;
                valid = true;
            }
            else if (strcmp("Equal", ConstraintType) == 0) {
                constraint->Type = Equal;
                valid = true;
            }
            else if (strstr(ConstraintType, "InternalAlignment")) {
                constraint->Type = InternalAlignment;

                valid = true;
                if (strstr(ConstraintType, "EllipseMajorDiameter")) {
                    constraint->AlignmentType = EllipseMajorDiameter;
                }
                else if (strstr(ConstraintType, "EllipseMinorDiameter")) {
                    constraint->AlignmentType = EllipseMinorDiameter;
                }
                else if (strstr(ConstraintType, "HyperbolaMajor")) {
                    constraint->AlignmentType = HyperbolaMajor;
                }
                else if (strstr(ConstraintType, "HyperbolaMinor")) {
                    constraint->AlignmentType = HyperbolaMinor;
                }
                else if (strstr(ConstraintType, "ParabolaFocalAxis")) {
                    constraint->AlignmentType = ParabolaFocalAxis;
                }
                else {
                    constraint->AlignmentType = Undef;
                    valid = false;
                }
            }

            if (valid) {
                constraint->First = FirstIndex;
                constraint->Second = SecondIndex;
                return true;
            }
        }
        // ConstraintType, GeoIndex, Value
        if (PyNumber_Check(index_or_value)) {  // can be float or int
            Value = PyFloat_AsDouble(index_or_value);
            if (strcmp("Distance", ConstraintType) == 0) {
                constraint->Type = Distance;
            }
            else if (strcmp("Angle", ConstraintType) == 0) {
                if (PyObject_TypeCheck(index_or_value, &(Base::QuantityPy::Type))) {
                    Base::Quantity q =
                        *(static_cast<Base::QuantityPy*>(index_or_value)->getQuantityPtr());
                    if (q.getUnit() == Base::Unit::Angle) {
                        Value = q.getValueAs(Base::Quantity::Radian);
                    }
                }
                constraint->Type = Angle;
            }
            else if (strcmp("DistanceX", ConstraintType) == 0) {
                constraint->Type = DistanceX;
            }
            else if (strcmp("DistanceY", ConstraintType) == 0) {
                constraint->Type = DistanceY;
            }
            else if (strcmp("Radius", ConstraintType) == 0) {
                constraint->Type = Radius;
                // set a value that is out of range of result of atan2
                // this value is handled in ViewProviderSketch
                constraint->LabelPosition = 10;
            }
            else if (strcmp("Diameter", ConstraintType) == 0) {
                constraint->Type = Diameter;
                // set a value that is out of range of result of atan2
                // this value is handled in ViewProviderSketch
                constraint->LabelPosition = 10;
            }
            else if (strcmp("Weight", ConstraintType) == 0) {
                constraint->Type = Weight;
                // set a value that is out of range of result of atan2
                // this value is handled in ViewProviderSketch
                constraint->LabelPosition = 10;
            }
            else {
                return false;
            }

            constraint->First = FirstIndex;
            constraint->setValue(Value);
            return true;
        }
        return false;
    };

    if (PyArg_ParseTuple(args, "siO", &ConstraintType, &FirstIndex, &index_or_value)) {
        if (handleSiO()) {
            return 0;
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args, "siOO", &ConstraintType, &FirstIndex, &index_or_value, &activated)) {
        if (PyBool_Check(activated)) {
            if (handleSiO()) {
                constraint->isActive = PyObject_IsTrue(activated);
                return 0;
            }
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args,
                         "siOOO",
                         &ConstraintType,
                         &FirstIndex,
                         &index_or_value,
                         &activated,
                         &driving)) {
        if (PyBool_Check(activated) && PyBool_Check(driving)) {
            if (handleSiO()) {
                constraint->isActive = PyObject_IsTrue(activated);
                if (constraint->isDimensional()) {
                    constraint->isDriving = PyObject_IsTrue(driving);
                }
                return 0;
            }
        }
    }
    PyErr_Clear();


    auto handleSiiO = [&]() -> bool {
        // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2
        if (PyLong_Check(index_or_value)) {
            FirstPos = any_index;
            SecondIndex = PyLong_AsLong(index_or_value);
            bool valid = false;
            if (strcmp("Perpendicular", ConstraintType) == 0) {
                constraint->Type = Perpendicular;
                valid = true;
            }
            else if (strcmp("Tangent", ConstraintType) == 0) {
                constraint->Type = Tangent;
                valid = true;
            }
            else if (strcmp("PointOnObject", ConstraintType) == 0) {
                constraint->Type = PointOnObject;
                valid = true;
            }
            else if (strstr(ConstraintType, "InternalAlignment")) {
                constraint->Type = InternalAlignment;

                valid = true;

                if (strstr(ConstraintType, "EllipseFocus1")) {
                    constraint->AlignmentType = EllipseFocus1;
                }
                else if (strstr(ConstraintType, "EllipseFocus2")) {
                    constraint->AlignmentType = EllipseFocus2;
                }
                else if (strstr(ConstraintType, "HyperbolaFocus")) {
                    constraint->AlignmentType = HyperbolaFocus;
                }
                else if (strstr(ConstraintType, "ParabolaFocus")) {
                    constraint->AlignmentType = ParabolaFocus;
                }
                else {
                    constraint->AlignmentType = Undef;
                    valid = false;
                }
            }

            if (valid) {
                constraint->First = FirstIndex;
                constraint->FirstPos = static_cast<Sketcher::PointPos>(FirstPos);
                constraint->Second = SecondIndex;
                return true;
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
                constraint->Type = Angle;
                constraint->Second = SecondIndex;
            }
            else if (strcmp("Distance", ConstraintType) == 0) {
                constraint->Type = Distance;
                constraint->Second = SecondIndex;
            }
            else if (strcmp("DistanceX", ConstraintType) == 0) {
                FirstPos = SecondIndex;
                SecondIndex = -1;
                constraint->Type = DistanceX;
                constraint->FirstPos = static_cast<Sketcher::PointPos>(FirstPos);
            }
            else if (strcmp("DistanceY", ConstraintType) == 0) {
                FirstPos = SecondIndex;
                SecondIndex = -1;
                constraint->Type = DistanceY;
                constraint->FirstPos = static_cast<Sketcher::PointPos>(FirstPos);
            }
            else {
                return false;
            }

            constraint->First = FirstIndex;
            constraint->setValue(Value);
            return true;
        }
        return false;
    };


    if (PyArg_ParseTuple(args, "siiO", &ConstraintType, &FirstIndex, &any_index, &index_or_value)) {
        if (handleSiiO()) {
            return 0;
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args,
                         "siiOO",
                         &ConstraintType,
                         &FirstIndex,
                         &any_index,
                         &index_or_value,
                         &activated)) {
        if (PyBool_Check(activated)) {
            if (handleSiiO()) {
                constraint->isActive = PyObject_IsTrue(activated);
                return 0;
            }
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args,
                         "siiOOO",
                         &ConstraintType,
                         &FirstIndex,
                         &any_index,
                         &index_or_value,
                         &activated,
                         &driving)) {
        if (PyBool_Check(activated) && PyBool_Check(driving)) {
            if (handleSiiO()) {
                constraint->isActive = PyObject_IsTrue(activated);
                if (constraint->isDimensional()) {
                    constraint->isDriving = PyObject_IsTrue(driving);
                }
                return 0;
            }
        }
    }
    PyErr_Clear();


    auto handleSiiiO = [&]() -> bool {
        // Value, ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, PosIndex2
        if (PyLong_Check(oNumArg4)) {
            intArg4 = PyLong_AsLong(oNumArg4);
            bool valid = false;
            if (strcmp("Coincident", ConstraintType) == 0) {
                constraint->Type = Coincident;
                valid = true;
            }
            else if (strcmp("Horizontal", ConstraintType) == 0) {
                constraint->Type = Horizontal;
                valid = true;
            }
            else if (strcmp("Vertical", ConstraintType) == 0) {
                constraint->Type = Vertical;
                valid = true;
            }
            else if (strcmp("Perpendicular", ConstraintType) == 0) {
                constraint->Type = Perpendicular;
                valid = true;
            }
            else if (strcmp("Tangent", ConstraintType) == 0) {
                constraint->Type = Tangent;
                valid = true;
            }
            else if (strcmp("TangentViaPoint", ConstraintType) == 0) {
                constraint->Type = Tangent;
                // valid = true;//non-standard assignment
                constraint->First = intArg1;
                constraint->FirstPos = Sketcher::PointPos::none;
                constraint->Second = intArg2;
                constraint->SecondPos = Sketcher::PointPos::none;
                constraint->Third = intArg3;
                constraint->ThirdPos = static_cast<Sketcher::PointPos>(intArg4);
                return true;
            }
            else if (strcmp("PerpendicularViaPoint", ConstraintType) == 0) {
                constraint->Type = Perpendicular;
                // valid = true;//non-standard assignment
                constraint->First = intArg1;
                constraint->FirstPos = Sketcher::PointPos::none;
                constraint->Second = intArg2;
                constraint->SecondPos = Sketcher::PointPos::none;
                constraint->Third = intArg3;
                constraint->ThirdPos = static_cast<Sketcher::PointPos>(intArg4);
                return true;
            }
            else if (strstr(ConstraintType,
                            "InternalAlignment")) {  // InteralAlignment with
                                                     // InternalElementIndex argument
                constraint->Type = InternalAlignment;

                valid = true;

                if (strstr(ConstraintType, "BSplineControlPoint")) {
                    constraint->AlignmentType = BSplineControlPoint;
                }
                else if (strstr(ConstraintType, "BSplineKnotPoint")) {
                    constraint->AlignmentType = BSplineKnotPoint;
                }
                else {
                    constraint->AlignmentType = Undef;
                    valid = false;
                }

                if (valid) {
                    constraint->First = intArg1;
                    constraint->FirstPos = static_cast<Sketcher::PointPos>(intArg2);
                    constraint->Second = intArg3;
                    constraint->InternalAlignmentIndex = intArg4;
                    return true;
                }
            }
            if (valid) {
                constraint->First = intArg1;
                constraint->FirstPos = static_cast<Sketcher::PointPos>(intArg2);
                constraint->Second = intArg3;
                constraint->SecondPos = static_cast<Sketcher::PointPos>(intArg4);
                return true;
            }
        }
        // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, Value
        if (PyNumber_Check(oNumArg4)) {  // can be float or int
            Value = PyFloat_AsDouble(oNumArg4);
            if (strcmp("Distance", ConstraintType) == 0) {
                constraint->Type = Distance;
                constraint->First = intArg1;
                constraint->FirstPos = static_cast<Sketcher::PointPos>(intArg2);
                constraint->Second = intArg3;
                constraint->setValue(Value);
                return true;
            }
        }
        return false;
    };

    if (PyArg_ParseTuple(args, "siiiO", &ConstraintType, &intArg1, &intArg2, &intArg3, &oNumArg4)) {
        if (handleSiiiO()) {
            return 0;
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args,
                         "siiiOO",
                         &ConstraintType,
                         &intArg1,
                         &intArg2,
                         &intArg3,
                         &oNumArg4,
                         &activated)) {
        if (PyBool_Check(activated)) {
            if (handleSiiiO()) {
                constraint->isActive = PyObject_IsTrue(activated);
                return 0;
            }
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args,
                         "siiiOOO",
                         &ConstraintType,
                         &intArg1,
                         &intArg2,
                         &intArg3,
                         &oNumArg4,
                         &activated,
                         &driving)) {
        if (PyBool_Check(activated) && PyBool_Check(driving)) {
            if (handleSiiiO()) {
                constraint->isActive = PyObject_IsTrue(activated);
                if (constraint->isDimensional()) {
                    constraint->isDriving = PyObject_IsTrue(driving);
                }
                return 0;
            }
        }
    }
    PyErr_Clear();


    auto handleSiiiiO = [&]() -> bool {
        // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, PosIndex2, GeoIndex3
        if (PyLong_Check(oNumArg5)) {
            intArg5 = PyLong_AsLong(oNumArg5);
            if (strcmp("Symmetric", ConstraintType) == 0) {
                constraint->Type = Symmetric;
                constraint->First = intArg1;
                constraint->FirstPos = static_cast<Sketcher::PointPos>(intArg2);
                constraint->Second = intArg3;
                constraint->SecondPos = static_cast<Sketcher::PointPos>(intArg4);
                constraint->Third = intArg5;
                return true;
            }
        }
        // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, PosIndex2, Value
        if (PyNumber_Check(oNumArg5)) {  // can be float or int
            Value = PyFloat_AsDouble(oNumArg5);
            if (strcmp("Distance", ConstraintType) == 0) {
                constraint->Type = Distance;
            }
            else if (strcmp("DistanceX", ConstraintType) == 0) {
                constraint->Type = DistanceX;
            }
            else if (strcmp("DistanceY", ConstraintType) == 0) {
                constraint->Type = DistanceY;
            }
            else if (strcmp("Angle", ConstraintType) == 0) {
                if (PyObject_TypeCheck(oNumArg5, &(Base::QuantityPy::Type))) {
                    Base::Quantity q =
                        *(static_cast<Base::QuantityPy*>(oNumArg5)->getQuantityPtr());
                    if (q.getUnit() == Base::Unit::Angle) {
                        Value = q.getValueAs(Base::Quantity::Radian);
                    }
                }
                constraint->Type = Angle;
            }
            else if (strcmp("AngleViaPoint", ConstraintType) == 0) {
                if (PyObject_TypeCheck(oNumArg5, &(Base::QuantityPy::Type))) {
                    Base::Quantity q =
                        *(static_cast<Base::QuantityPy*>(oNumArg5)->getQuantityPtr());
                    if (q.getUnit() == Base::Unit::Angle) {
                        Value = q.getValueAs(Base::Quantity::Radian);
                    }
                }
                constraint->Type = Angle;
                // valid = true;//non-standard assignment
                constraint->First = intArg1;
                constraint->FirstPos = Sketcher::PointPos::none;
                constraint->Second = intArg2;  // let's goof up all the terminology =)
                constraint->SecondPos = Sketcher::PointPos::none;
                constraint->Third = intArg3;
                constraint->ThirdPos = static_cast<Sketcher::PointPos>(intArg4);
                constraint->setValue(Value);
                return true;
            }
            else {
                return false;
            }

            constraint->First = intArg1;
            constraint->FirstPos = static_cast<Sketcher::PointPos>(intArg2);
            constraint->Second = intArg3;
            constraint->SecondPos = static_cast<Sketcher::PointPos>(intArg4);
            constraint->setValue(Value);
            return true;
        }
        return false;
    };

    if (PyArg_ParseTuple(args,
                         "siiiiO",
                         &ConstraintType,
                         &intArg1,
                         &intArg2,
                         &intArg3,
                         &intArg4,
                         &oNumArg5)) {
        if (handleSiiiiO()) {
            return 0;
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args,
                         "siiiiOO",
                         &ConstraintType,
                         &intArg1,
                         &intArg2,
                         &intArg3,
                         &intArg4,
                         &oNumArg5,
                         &activated)) {
        if (PyBool_Check(activated)) {
            if (handleSiiiiO()) {
                constraint->isActive = PyObject_IsTrue(activated);
                return 0;
            }
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args,
                         "siiiiOOO",
                         &ConstraintType,
                         &intArg1,
                         &intArg2,
                         &intArg3,
                         &intArg4,
                         &oNumArg5,
                         &activated,
                         &driving)) {
        if (PyBool_Check(activated) && PyBool_Check(driving)) {
            if (handleSiiiiO()) {
                constraint->isActive = PyObject_IsTrue(activated);
                if (constraint->isDimensional()) {
                    constraint->isDriving = PyObject_IsTrue(driving);
                }
                return 0;
            }
        }
    }
    PyErr_Clear();

    auto handleSiiiiiO = [&]() -> bool {
        if (PyLong_Check(index_or_value)) {
            ThirdPos = PyLong_AsLong(index_or_value);
            // ConstraintType, GeoIndex1, PosIndex1, GeoIndex2, PosIndex2, GeoIndex3, PosIndex3
            if (strcmp("Symmetric", ConstraintType) == 0) {
                constraint->Type = Symmetric;
                constraint->First = FirstIndex;
                constraint->FirstPos = static_cast<Sketcher::PointPos>(FirstPos);
                constraint->Second = SecondIndex;
                constraint->SecondPos = static_cast<Sketcher::PointPos>(SecondPos);
                constraint->Third = ThirdIndex;
                constraint->ThirdPos = static_cast<Sketcher::PointPos>(ThirdPos);
                return true;
            }
        }
        if (PyNumber_Check(index_or_value)) {  // can be float or int
            Value = PyFloat_AsDouble(index_or_value);
            if (strcmp("SnellsLaw", ConstraintType) == 0) {
                constraint->Type = SnellsLaw;
                constraint->First = FirstIndex;
                constraint->FirstPos = static_cast<Sketcher::PointPos>(FirstPos);
                constraint->Second = SecondIndex;
                constraint->SecondPos = static_cast<Sketcher::PointPos>(SecondPos);
                constraint->Third = ThirdIndex;
                constraint->ThirdPos = Sketcher::PointPos::none;
                constraint->setValue(Value);
                return true;
            }
        }
        return false;
    };

    if (PyArg_ParseTuple(args,
                         "siiiiiO",
                         &ConstraintType,
                         &FirstIndex,
                         &FirstPos,
                         &SecondIndex,
                         &SecondPos,
                         &ThirdIndex,
                         &index_or_value)) {
        if (handleSiiiiiO()) {
            return 0;
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args,
                         "siiiiiOO",
                         &ConstraintType,
                         &FirstIndex,
                         &FirstPos,
                         &SecondIndex,
                         &SecondPos,
                         &ThirdIndex,
                         &index_or_value,
                         &activated)) {
        if (PyBool_Check(activated)) {
            if (handleSiiiiiO()) {
                constraint->isActive = PyObject_IsTrue(activated);
                return 0;
            }
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args,
                         "siiiiiOOO",
                         &ConstraintType,
                         &FirstIndex,
                         &FirstPos,
                         &SecondIndex,
                         &SecondPos,
                         &ThirdIndex,
                         &index_or_value,
                         &activated,
                         &driving)) {
        if (PyBool_Check(activated) && PyBool_Check(driving)) {
            if (handleSiiiiiO()) {
                constraint->isActive = PyObject_IsTrue(activated);
                if (constraint->isDimensional()) {
                    constraint->isDriving = PyObject_IsTrue(driving);
                }
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
                case HyperbolaMajor:
                    result << "'InternalAlignment:HyperbolaMajor'>";
                    break;
                case HyperbolaMinor:
                    result << "'InternalAlignment:HyperbolaMinor'>";
                    break;
                case HyperbolaFocus:
                    result << "'InternalAlignment:HyperbolaFocus'>";
                    break;
                case ParabolaFocalAxis:
                    result << "'InternalAlignment:ParabolaFocalAxis'>";
                    break;
                case ParabolaFocus:
                    result << "'InternalAlignment:ParabolaFocus'>";
                    break;
                case BSplineControlPoint:
                    result << "'InternalAlignment:BSplineControlPoint'>";
                    break;
                case BSplineKnotPoint:
                    result << "'InternalAlignment:BSplineKnotPoint'>";
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

Py::Float ConstraintPy::getLabelDistance() const
{
    return Py::Float(this->getConstraintPtr()->LabelDistance);
}

Py::Float ConstraintPy::getLabelPosition() const
{
    return Py::Float(this->getConstraintPtr()->LabelPosition);
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
