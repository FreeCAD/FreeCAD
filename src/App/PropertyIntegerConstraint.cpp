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

#include <Base/PyObjectBase.h>
#include <Base/PyWrapParseTupleAndKeywords.h>

#include "PropertyIntegerConstraint.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyIntegerConstraint, App::PropertyInteger)

//**************************************************************************
// Construction/Destruction


PropertyIntegerConstraint::PropertyIntegerConstraint() = default;

PropertyIntegerConstraint::~PropertyIntegerConstraint()
{
    if (_ConstStruct && _ConstStruct->isDeletable()) {
        delete _ConstStruct;
    }
}

void PropertyIntegerConstraint::setConstraints(const Constraints* sConstrain)
{
    if (_ConstStruct != sConstrain) {
        if (_ConstStruct && _ConstStruct->isDeletable()) {
            delete _ConstStruct;
        }
    }

    _ConstStruct = sConstrain;
}

const PropertyIntegerConstraint::Constraints* PropertyIntegerConstraint::getConstraints() const
{
    return _ConstStruct;
}

long PropertyIntegerConstraint::getMinimum() const
{
    if (_ConstStruct) {
        return _ConstStruct->LowerBound;
    }
    // return the min of int, not long
    return std::numeric_limits<int>::lowest();
}

long PropertyIntegerConstraint::getMaximum() const
{
    if (_ConstStruct) {
        return _ConstStruct->UpperBound;
    }
    // return the max of int, not long
    return std::numeric_limits<int>::max();
}

long PropertyIntegerConstraint::getStepSize() const
{
    if (_ConstStruct) {
        return _ConstStruct->StepSize;
    }
    return 1;
}

void PropertyIntegerConstraint::setPyObject(PyObject* value)
{
    if (PyLong_Check(value)) {
        long temp = PyLong_AsLong(value);
        if (_ConstStruct) {
            if (temp > _ConstStruct->UpperBound) {
                temp = _ConstStruct->UpperBound;
            }
            else if (temp < _ConstStruct->LowerBound) {
                temp = _ConstStruct->LowerBound;
            }
        }

        aboutToSetValue();
        _lValue = temp;
        hasSetValue();
    }
    else {
        long valConstr[] = {0,
                            std::numeric_limits<int>::lowest(),
                            std::numeric_limits<int>::max(),
                            1};

        if (PyDict_Check(value)) {
            Py::Tuple dummy;
            static const std::array<const char*, 5> kw = {"value",
                                                          "min",
                                                          "max",
                                                          "step",
                                                          nullptr};

            if (!Base::Wrapped_ParseTupleAndKeywords(dummy.ptr(),
                                                     value,
                                                     "l|lll",
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
                                  "llll",
                                  &(valConstr[0]),
                                  &(valConstr[1]),
                                  &(valConstr[2]),
                                  &(valConstr[3]))) {
                throw Py::Exception();
            }
        }
        else {
            std::string error = std::string("type must be int, dict or tuple, not ");
            error += value->ob_type->tp_name;
            throw Base::TypeError(error);
        }

        Constraints* c = new Constraints();
        c->setDeletable(true);
        c->LowerBound = valConstr[1];
        c->UpperBound = valConstr[2];
        c->StepSize = std::max<long>(1, valConstr[3]);
        if (valConstr[0] > c->UpperBound) {
            valConstr[0] = c->UpperBound;
        }
        else if (valConstr[0] < c->LowerBound) {
            valConstr[0] = c->LowerBound;
        }
        setConstraints(c);

        aboutToSetValue();
        _lValue = valConstr[0];
        hasSetValue();
    }
}

}  // namespace App