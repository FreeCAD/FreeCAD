/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <float.h>

#include <Base/PyObjectBase.h>
#include <Base/PyWrapParseTupleAndKeywords.h>

#include "PropertyFloatConstraint.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyFloatConstraint, App::PropertyFloat)

PropertyFloatConstraint::PropertyFloatConstraint() = default;

PropertyFloatConstraint::~PropertyFloatConstraint()
{
    if (_ConstStruct && _ConstStruct->isDeletable()) {
        delete _ConstStruct;
    }
}

void PropertyFloatConstraint::setConstraints(const Constraints* sConstrain)
{
    if (_ConstStruct != sConstrain) {
        if (_ConstStruct && _ConstStruct->isDeletable()) {
            delete _ConstStruct;
        }
    }
    _ConstStruct = sConstrain;
}

const PropertyFloatConstraint::Constraints* PropertyFloatConstraint::getConstraints() const
{
    return _ConstStruct;
}

double PropertyFloatConstraint::getMinimum() const
{
    if (_ConstStruct) {
        return _ConstStruct->LowerBound;
    }
    return std::numeric_limits<double>::lowest();
}

double PropertyFloatConstraint::getMaximum() const
{
    if (_ConstStruct) {
        return _ConstStruct->UpperBound;
    }
    return std::numeric_limits<double>::max();
}

double PropertyFloatConstraint::getStepSize() const
{
    if (_ConstStruct) {
        return _ConstStruct->StepSize;
    }
    return 1.0;
}

void PropertyFloatConstraint::setPyObject(PyObject* value)
{
    if (PyFloat_Check(value)) {
        double temp = PyFloat_AsDouble(value);
        if (_ConstStruct) {
            if (temp > _ConstStruct->UpperBound) {
                temp = _ConstStruct->UpperBound;
            }
            else if (temp < _ConstStruct->LowerBound) {
                temp = _ConstStruct->LowerBound;
            }
        }

        aboutToSetValue();
        _dValue = temp;
        hasSetValue();
    }
    else if (PyLong_Check(value)) {
        double temp = static_cast<double>(PyLong_AsLong(value));
        if (_ConstStruct) {
            if (temp > _ConstStruct->UpperBound) {
                temp = _ConstStruct->UpperBound;
            }
            else if (temp < _ConstStruct->LowerBound) {
                temp = _ConstStruct->LowerBound;
            }
        }

        aboutToSetValue();
        _dValue = temp;
        hasSetValue();
    }
    else {
        double valConstr[] = {0.0,
                              std::numeric_limits<double>::lowest(),
                              std::numeric_limits<double>::max(),
                              1.0};

        if (PyDict_Check(value)) {
            Py::Tuple dummy;
            static const std::array<const char*, 5> kw = {"value",
                                                          "min",
                                                          "max",
                                                          "step",
                                                           nullptr};

            if (!Base::Wrapped_ParseTupleAndKeywords(dummy.ptr(),
                                                     value,
                                                     "d|ddd",
                                                     kw,
                                                     &(valConstr[0]),
                                                     &(valConstr[1]),
                                                     &(valConstr[2]),
                                                     &(valConstr[3]))) {
                throw Py::Exception();
            }
        }
        else if (PyTuple_Check(value)) {
            if (!PyArg_ParseTuple(value,
                                  "dddd",
                                  &(valConstr[0]),
                                  &(valConstr[1]),
                                  &(valConstr[2]),
                                  &(valConstr[3]))) {
                throw Py::Exception();
            }
        }
        else {
            std::string error = std::string("type must be float, dict or tuple, not ");
            error += value->ob_type->tp_name;
            throw Base::TypeError(error);
        }

        double stepSize = valConstr[3];
        // need a value > 0
        if (stepSize < DBL_EPSILON) {
            throw Base::ValueError("Step size must be greater than zero");
        }

        Constraints* c = new Constraints();
        c->setDeletable(true);
        c->LowerBound = valConstr[1];
        c->UpperBound = valConstr[2];
        c->StepSize = stepSize;
        if (valConstr[0] > c->UpperBound) {
            valConstr[0] = c->UpperBound;
        }
        else if (valConstr[0] < c->LowerBound) {
            valConstr[0] = c->LowerBound;
        }
        setConstraints(c);

        aboutToSetValue();
        _dValue = valConstr[0];
        hasSetValue();
    }
}


//**************************************************************************
// PropertyPrecision
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPrecision, App::PropertyFloatConstraint)

//**************************************************************************
// Construction/Destruction
//
const PropertyFloatConstraint::Constraints PrecisionStandard = {0.0, DBL_MAX, 0.001};

PropertyPrecision::PropertyPrecision()
{
    setConstraints(&PrecisionStandard);
}

PropertyPrecision::~PropertyPrecision() = default;

}  // namespace App
