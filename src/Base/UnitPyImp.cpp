/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Base/Unit.h>

// inclusion of the generated files (generated out of UnitPy.xml)
#include <Base/UnitPy.h>
#include <Base/QuantityPy.h>
#include <Base/UnitPy.cpp>

using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string UnitPy::representation() const
{
    const UnitSignature &  Sig = getUnitPtr()->getSignature();
    std::stringstream ret;
    ret << "Unit: ";
    ret << getUnitPtr()->getString().toUtf8().constData() << " (";
    ret << Sig.Length << ",";
    ret << Sig.Mass  << ",";
    ret << Sig.Time  << ",";
    ret << Sig.ElectricCurrent  << ",";
    ret << Sig.ThermodynamicTemperature << ",";
    ret << Sig.AmountOfSubstance  << ",";
    ret << Sig.LuminousIntensity  << ",";
    ret << Sig.Angle  << ")";
    std::string type = getUnitPtr()->getTypeString().toUtf8().constData();
    if (! type.empty())
        ret << " [" << type << "]";

    return ret.str();
}

PyObject *UnitPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of UnitPy and the Twin object
    return new UnitPy(new Unit);
}

// constructor method
int UnitPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject *object{};
    Unit *self = getUnitPtr();

    // get quantity
    if (PyArg_ParseTuple(args,"O!",&(Base::QuantityPy::Type), &object)) {
        *self = static_cast<Base::QuantityPy*>(object)->getQuantityPtr()->getUnit();
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()

    // get unit
    if (PyArg_ParseTuple(args,"O!",&(Base::UnitPy::Type), &object)) {
        *self = *(static_cast<Base::UnitPy*>(object)->getUnitPtr());
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()

    // get string
    char* string{};
    if (PyArg_ParseTuple(args,"et", "utf-8", &string)) {
        QString qstr = QString::fromUtf8(string);
        PyMem_Free(string);
        try {
            *self = Quantity::parse(qstr).getUnit();
            return 0;
        }
        catch (const Base::ParserError& e) {
            PyErr_SetString(PyExc_ValueError, e.what());
            return -1;
        }
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()

    int i1=0;
    int i2=0;
    int i3=0;
    int i4=0;
    int i5=0;
    int i6=0;
    int i7=0;
    int i8=0;
    if (PyArg_ParseTuple(args, "|iiiiiiii", &i1,&i2,&i3,&i4,&i5,&i6,&i7,&i8)) {
        try {
            *self = Unit(i1,i2,i3,i4,i5,i6,i7,i8);
            return 0;
        }
        catch (const Base::OverflowError& e) {
            PyErr_SetString(PyExc_OverflowError, e.what());
            return -1;
        }
    }

    PyErr_SetString(PyExc_TypeError, "Either string, (float,8 ints), Unit() or Quantity()");
    return -1;
}


PyObject* UnitPy::number_add_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Unit");
        return nullptr;
    }
    if (!PyObject_TypeCheck(other, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Unit");
        return nullptr;
    }
    Base::Unit *a = static_cast<UnitPy*>(self)->getUnitPtr();
    Base::Unit *b = static_cast<UnitPy*>(other)->getUnitPtr();

    if (*a != *b) {
        PyErr_SetString(PyExc_TypeError, "Units not matching!");
        return nullptr;
    }

    return new UnitPy(new Unit(*a));
}

PyObject* UnitPy::number_subtract_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Unit");
        return nullptr;
    }
    if (!PyObject_TypeCheck(other, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Unit");
        return nullptr;
    }
    Base::Unit *a = static_cast<UnitPy*>(self)->getUnitPtr();
    Base::Unit *b = static_cast<UnitPy*>(other)->getUnitPtr();

    if (*a != *b) {
        PyErr_SetString(PyExc_TypeError, "Units not matching!");
        return nullptr;
    }

    return new UnitPy(new Unit(*a));
}

PyObject* UnitPy::number_multiply_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Unit");
        return nullptr;
    }

    if (PyObject_TypeCheck(other, &(UnitPy::Type))) {
        Base::Unit *a = static_cast<UnitPy*>(self) ->getUnitPtr();
        Base::Unit *b = static_cast<UnitPy*>(other)->getUnitPtr();

        return new UnitPy(new Unit( (*a) * (*b) ) );
    }
    else {
        PyErr_SetString(PyExc_TypeError, "A Unit can only be multiplied by a Unit");
        return nullptr;
    }
}

PyObject* UnitPy::richCompare(PyObject *v, PyObject *w, int op)
{
    if (PyObject_TypeCheck(v, &(UnitPy::Type)) &&
        PyObject_TypeCheck(w, &(UnitPy::Type))) {
        const Unit * u1 = static_cast<UnitPy*>(v)->getUnitPtr();
        const Unit * u2 = static_cast<UnitPy*>(w)->getUnitPtr();

        PyObject *res=nullptr;
        if (op != Py_EQ && op != Py_NE) {
            PyErr_SetString(PyExc_TypeError,
            "no ordering relation is defined for Units");
            return nullptr;
        }
        else if (op == Py_EQ) {
            res = (*u1 == *u2) ? Py_True : Py_False; //NOLINT
            Py_INCREF(res);
            return res;
        }
        else {
            res = (*u1 != *u2) ? Py_True : Py_False; //NOLINT
            Py_INCREF(res);
            return res;
        }
    }
    else {
        // This always returns False
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
}

Py::String UnitPy::getType() const
{
    return {getUnitPtr()->getTypeString().toUtf8(),"utf-8"};
}

Py::Tuple UnitPy::getSignature() const
{
    const UnitSignature &  Sig = getUnitPtr()->getSignature();
    Py::Tuple tuple(8);
    tuple.setItem(0, Py::Long(Sig.Length));
    tuple.setItem(1, Py::Long(Sig.Mass));
    tuple.setItem(2, Py::Long(Sig.Time));
    tuple.setItem(3, Py::Long(Sig.ElectricCurrent));
    tuple.setItem(4, Py::Long(Sig.ThermodynamicTemperature));
    tuple.setItem(5, Py::Long(Sig.AmountOfSubstance));
    tuple.setItem(6, Py::Long(Sig.LuminousIntensity));
    tuple.setItem(7, Py::Long(Sig.Angle));
    return tuple;
}



PyObject *UnitPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int UnitPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

PyObject * UnitPy::number_divide_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * UnitPy::number_remainder_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * UnitPy::number_divmod_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * UnitPy::number_power_handler (PyObject* /*self*/, PyObject* /*other*/, PyObject* /*modulo*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * UnitPy::number_negative_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * UnitPy::number_positive_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * UnitPy::number_absolute_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

int UnitPy::number_nonzero_handler (PyObject* /*self*/)
{
    return 1;
}

PyObject * UnitPy::number_invert_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * UnitPy::number_lshift_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * UnitPy::number_rshift_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * UnitPy::number_and_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * UnitPy::number_xor_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * UnitPy::number_or_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * UnitPy::number_int_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}

PyObject * UnitPy::number_float_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return nullptr;
}
