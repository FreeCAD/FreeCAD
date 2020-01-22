/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "DualNumberPy.h"
#include "DualNumberPy.cpp"

using namespace Base;

int DualNumberPy::initialization()
{
    assert(_pcTwinPointer == nullptr /*only use `new DualNumberPy(nullptr)`, please*/);
    _pcTwinPointer = &this->value;
    return 0;
}

int DualNumberPy::finalization()
{
    return 0;
}

// returns a string which represent the object e.g. when printed in python
std::string DualNumberPy::representation(void) const
{
    return value.repr();
}

PyObject* DualNumberPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of DualNumberPy and the Twin object 
    DualNumberPy* pcn = new DualNumberPy(nullptr);
    return pcn;
}

// constructor method
int DualNumberPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    {
        double re = 0.0, du = 0.0;
        if (PyArg_ParseTuple(args, "d|d", &re, &du)){
            value.re = re;
            value.du = du;
            return 0;
        }
        PyErr_Clear();
    }
    PyErr_SetString(PyExc_TypeError,
        "wrong constructor arguments. Expect:"
        "\n"
        "(float) - a dual number with zero dual part"
        "(float,float) - a new dual number");
    return -1;
}

Py::Float DualNumberPy::getre(void) const
{
    return Py::Float(value.re);
}

Py::Float DualNumberPy::getdu(void) const
{
    return Py::Float(value.du);
}


PyObject* DualNumberPy::number_add_handler(PyObject* self, PyObject* other)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return nullptr;
}

PyObject* DualNumberPy::number_subtract_handler(PyObject* self, PyObject* other)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return nullptr;
}

PyObject* DualNumberPy::number_multiply_handler(PyObject* self, PyObject* other)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return nullptr;
}


PyObject* DualNumberPy::richCompare(PyObject* v, PyObject* w, int op)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return nullptr;
}

PyObject* DualNumberPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DualNumberPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

PyObject*  DualNumberPy::number_divide_handler (PyObject* self, PyObject* other)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return nullptr;
}

PyObject*  DualNumberPy::number_remainder_handler (PyObject* self, PyObject* other)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return nullptr;
}

PyObject*  DualNumberPy::number_divmod_handler (PyObject* self, PyObject* other)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return nullptr;
}

PyObject*  DualNumberPy::number_power_handler (PyObject* self, PyObject* other, PyObject* /*arg*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return nullptr;
}

PyObject*  DualNumberPy::number_negative_handler (PyObject* self)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return nullptr;
}

PyObject*  DualNumberPy::number_positive_handler (PyObject* self)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return nullptr;
}

PyObject*  DualNumberPy::number_absolute_handler (PyObject* self)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return nullptr;
}

int DualNumberPy::number_nonzero_handler (PyObject* /*self*/)
{
    return 1; // #FIXME
}

PyObject*  DualNumberPy::number_invert_handler (PyObject* self)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return nullptr;
}

PyObject*  DualNumberPy::number_lshift_handler (PyObject* self, PyObject* other)
{
    PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for <<: '%s' and '%s'",
                 Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return 0;
}

PyObject*  DualNumberPy::number_rshift_handler (PyObject* self, PyObject* other)
{
    PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for >>: '%s' and '%s'",
                 Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return 0;
}

PyObject*  DualNumberPy::number_and_handler (PyObject* self, PyObject* other)
{
    PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for &: '%s' and '%s'",
                 Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return 0;
}

PyObject*  DualNumberPy::number_xor_handler (PyObject* self, PyObject* other)
{
    PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for ^: '%s' and '%s'",
                 Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return 0;
}

PyObject*  DualNumberPy::number_or_handler (PyObject* self, PyObject* other)
{
    PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for |: '%s' and '%s'",
                 Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return 0;
}

#if PY_MAJOR_VERSION < 3
int DualNumberPy::number_coerce_handler (PyObject* * /*self*/, PyObject* * /*other*/)
{
    return 1;
}
#endif

PyObject*  DualNumberPy::number_int_handler (PyObject* self)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return nullptr;
}

#if PY_MAJOR_VERSION < 3
PyObject*  DualNumberPy::number_long_handler (PyObject* self)
{
    PyErr_Format(PyExc_TypeError, "long() argument must be a string or a number, not '%s'",
                 Py_TYPE(self)->tp_name);
    return 0;
}
#endif

PyObject*  DualNumberPy::number_float_handler (PyObject* self)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return nullptr;
}

#if PY_MAJOR_VERSION < 3
PyObject*  DualNumberPy::number_oct_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_TypeError, "oct() argument can't be converted to oct");
    return 0;
}

PyObject*  DualNumberPy::number_hex_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_TypeError, "hex() argument can't be converted to hex");
    return 0;
}
#endif
