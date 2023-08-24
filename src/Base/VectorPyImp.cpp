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

#include "Vector3D.h"

// inclusion of the generated files (generated out of VectorPy.xml)
#include "GeometryPyCXX.h"
#include "VectorPy.h"
#include "VectorPy.cpp"

using namespace Base;

// returns a string which represent the object e.g. when printed in python
std::string VectorPy::representation() const
{
    VectorPy::PointerType ptr = getVectorPtr();
    Py::Float x(ptr->x);
    Py::Float y(ptr->y);
    Py::Float z(ptr->z);
    std::stringstream str;
    str << "Vector (";
    str << static_cast<std::string>(x.repr()) << ", "
        << static_cast<std::string>(y.repr()) << ", "
        << static_cast<std::string>(z.repr());
    str << ")";

    return str.str();
}

PyObject *VectorPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of VectorPy and the Twin object
    return new VectorPy(new Vector3d);
}

// constructor method
int VectorPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    double  x=0.0,y=0.0,z=0.0;
    PyObject *object = nullptr;
    VectorPy::PointerType ptr = getVectorPtr();
    if (PyArg_ParseTuple(args, "|ddd", &x,&y,&z)) {
        ptr->Set(x,y,z);
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()
    if (PyArg_ParseTuple(args,"O!",&(Base::VectorPy::Type), &object)) {
        *ptr = *(static_cast<Base::VectorPy*>(object)->getVectorPtr());
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()
    if (PyArg_ParseTuple(args,"O", &object)) {
        try {
            *ptr = getVectorFromTuple<double>(object);
            return 0;
        }
        catch (const Py::Exception&) {
            return -1;
        }
    }

    PyErr_SetString(PyExc_TypeError, "Either three floats, tuple or Vector expected");
    return -1;
}

PyObject*  VectorPy::__reduce__(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Py::Tuple tuple(2);

    Py::Object type(Base::getTypeAsObject(&Base::VectorPy::Type));
    tuple.setItem(0, type);

    Base::Vector3d v = this->value();
    Py::Tuple xyz(3);
    xyz.setItem(0, Py::Float(v.x));
    xyz.setItem(1, Py::Float(v.y));
    xyz.setItem(2, Py::Float(v.z));
    tuple.setItem(1, xyz);
    return Py::new_reference_to(tuple);
}

PyObject* VectorPy::number_add_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return nullptr;
    }
    if (!PyObject_TypeCheck(other, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return nullptr;
    }
    Base::Vector3d a = static_cast<VectorPy*>(self)->value();
    Base::Vector3d b = static_cast<VectorPy*>(other)->value();
    return new VectorPy(a+b);
}

PyObject* VectorPy::number_subtract_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return nullptr;
    }
    if (!PyObject_TypeCheck(other, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return nullptr;
    }
    Base::Vector3d a = static_cast<VectorPy*>(self)->value();
    Base::Vector3d b = static_cast<VectorPy*>(other)->value();
    return new VectorPy(a-b);
}

PyObject* VectorPy::number_multiply_handler(PyObject *self, PyObject *other)
{
    if (PyObject_TypeCheck(self, &(VectorPy::Type))) {
        Base::Vector3d a = static_cast<VectorPy*>(self) ->value();

        if (PyObject_TypeCheck(other, &(VectorPy::Type))) {
            Base::Vector3d b = static_cast<VectorPy*>(other)->value();
            Py::Float mult(a * b);
            return Py::new_reference_to(mult);
        }
        else if (PyNumber_Check(other)) {
            double b = PyFloat_AsDouble(other);
            return new VectorPy(a * b);
        }
        else {
            PyErr_SetString(PyExc_TypeError, "A Vector can only be multiplied by Vector or number");
            return nullptr;
        }
    }
    else if (PyObject_TypeCheck(other, &(VectorPy::Type))) {
        Base::Vector3d a = static_cast<VectorPy*>(other) ->value();
        if (PyNumber_Check(self)) {
            double b = PyFloat_AsDouble(self);
            return new VectorPy(a * b);
        }
        else {
            PyErr_SetString(PyExc_TypeError, "A Vector can only be multiplied by Vector or number");
            return nullptr;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "First or second arg must be Vector");
        return nullptr;
    }
}

Py_ssize_t VectorPy::sequence_length(PyObject *)
{
    return 3;
}

PyObject * VectorPy::sequence_item (PyObject *self, Py_ssize_t index)
{
    if (!PyObject_TypeCheck(self, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "first arg must be Vector");
        return nullptr;
    }
    if ((index < 0) || (index > 2)) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return nullptr;
    }

    VectorPy* self_ = static_cast<VectorPy*>(self);
    if (self_->sequence.length() == 0) {
        self_->sequence = Py::List{3};
    }

    unsigned short pos = index % 3;
    Base::Vector3d vec = self_->value();
    Py::Float item{vec[pos]};
    self_->sequence.setItem(pos, item);

    return Py::new_reference_to(item);
}

int VectorPy::sequence_ass_item(PyObject *self, Py_ssize_t index, PyObject *value)
{
    if (!PyObject_TypeCheck(self, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "first arg must be Vector");
        return -1;
    }
    if (index < 0 || index > 2) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return -1;
    }

    unsigned short pos = index % 3;

    if (PyNumber_Check(value)) {
        VectorPy::PointerType ptr = static_cast<VectorPy*>(self)->getVectorPtr();
        (*ptr)[pos] = PyFloat_AsDouble(value);
    }
    else {
        PyErr_SetString(PyExc_ValueError, "value must be float");
        return -1;
    }

    return 0;
}

// http://renesd.blogspot.de/2009/07/python3-c-api-simple-slicing-sqslice.html
PyObject * VectorPy::mapping_subscript(PyObject *self, PyObject *item)
{
    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return nullptr;
        if (i < 0)
            i += sequence_length(self);
        return sequence_item(self, i);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start = 0, stop = 0, step = 0, slicelength = 0, cur = 0, i = 0;
        PyObject* slice = item;

        if (PySlice_GetIndicesEx(slice,
                         sequence_length(self),
                         &start, &stop, &step, &slicelength) < 0) {
            return nullptr;
        }

        if (slicelength <= 0) {
            return PyTuple_New(0);
        }
        else if (start == 0 && step == 1 &&
                 slicelength == sequence_length(self) &&
                 PyObject_TypeCheck(self, &(VectorPy::Type))) {
            Base::Vector3d v = static_cast<VectorPy*>(self) ->value();
            Py::Tuple xyz(3);
            xyz.setItem(0, Py::Float(v.x));
            xyz.setItem(1, Py::Float(v.y));
            xyz.setItem(2, Py::Float(v.z));
            return Py::new_reference_to(xyz);
        }
        else if (PyObject_TypeCheck(self, &(VectorPy::Type))) {
            Base::Vector3d v = static_cast<VectorPy*>(self) ->value();
            Py::Tuple xyz(static_cast<size_t>(slicelength));

            for (cur = start, i = 0; i < slicelength; cur += step, i++) {
                unsigned short pos = cur % 3;
                xyz.setItem(static_cast<Py::sequence_index_type>(i), Py::Float(v[pos]));
            }

            return Py::new_reference_to(xyz);
        }
    }

    PyErr_Format(PyExc_TypeError,
                 "Vector indices must be integers or slices, not %.200s",
                 Py_TYPE(item)->tp_name);
    return nullptr;
}

PyObject*  VectorPy::add(PyObject *args)
{
    PyObject *obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &obj))
        return nullptr;

    VectorPy* vec = static_cast<VectorPy*>(obj);

    VectorPy::PointerType this_ptr = getVectorPtr();
    VectorPy::PointerType vect_ptr = vec->getVectorPtr();

    Base::Vector3d v = (*this_ptr) + (*vect_ptr);
    return new VectorPy(v);
}

PyObject*  VectorPy::sub(PyObject *args)
{
    PyObject *obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &obj))
        return nullptr;

    VectorPy* vec = static_cast<VectorPy*>(obj);

    VectorPy::PointerType this_ptr = getVectorPtr();
    VectorPy::PointerType vect_ptr = vec->getVectorPtr();

    Base::Vector3d v = (*this_ptr) - (*vect_ptr);
    return new VectorPy(v);
}

PyObject*  VectorPy::negative(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    VectorPy::PointerType this_ptr = getVectorPtr();
    Base::Vector3d v = -(*this_ptr);
    return new VectorPy(v);
}

PyObject* VectorPy::richCompare(PyObject *v, PyObject *w, int op)
{
    if (PyObject_TypeCheck(v, &(VectorPy::Type)) &&
        PyObject_TypeCheck(w, &(VectorPy::Type))) {
        Vector3d v1 = static_cast<VectorPy*>(v)->value();
        Vector3d v2 = static_cast<VectorPy*>(w)->value();

        PyObject *res=nullptr;
        if (op != Py_EQ && op != Py_NE) {
            PyErr_SetString(PyExc_TypeError,
            "no ordering relation is defined for Vector");
            return nullptr;
        }
        else if (op == Py_EQ) {
            res = (v1 == v2) ? Py_True : Py_False; //NOLINT
            Py_INCREF(res);
            return res;
        }
        else {
            res = (v1 != v2) ? Py_True : Py_False; //NOLINT
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

PyObject*  VectorPy::isEqual(PyObject *args)
{
    PyObject *obj = nullptr;
    double tolerance=0;
    if (!PyArg_ParseTuple(args, "O!d", &(VectorPy::Type), &obj, &tolerance))
        return nullptr;

    VectorPy* vec = static_cast<VectorPy*>(obj);

    VectorPy::PointerType this_ptr = getVectorPtr();
    VectorPy::PointerType vect_ptr = vec->getVectorPtr();

    Py::Boolean eq((*this_ptr).IsEqual(*vect_ptr, tolerance));
    return Py::new_reference_to(eq);
}

PyObject*  VectorPy::scale(PyObject *args)
{
    double factorX = 0.0, factorY = 0.0, factorZ = 0.0;
    if (!PyArg_ParseTuple(args, "ddd", &factorX, &factorY, &factorZ))
        return nullptr;
    VectorPy::PointerType ptr = getVectorPtr();
    ptr->Scale(factorX, factorY, factorZ);

    return Py::new_reference_to(this);
}

PyObject*  VectorPy::multiply(PyObject *args)
{
    double factor = 0.0;
    if (!PyArg_ParseTuple(args, "d", &factor))
        return nullptr;
    VectorPy::PointerType ptr = getVectorPtr();
    ptr->Scale(factor, factor, factor);

    return Py::new_reference_to(this);
}

PyObject*  VectorPy::dot(PyObject *args)
{
    PyObject *obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &obj))
        return nullptr;

    VectorPy* vec = static_cast<VectorPy*>(obj);

    VectorPy::PointerType this_ptr = getVectorPtr();
    VectorPy::PointerType vect_ptr = vec->getVectorPtr();

    Py::Float mult((*this_ptr) * (*vect_ptr));
    return Py::new_reference_to(mult);
}

PyObject*  VectorPy::cross(PyObject *args)
{
    PyObject *obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &obj))
        return nullptr;

    VectorPy* vec = static_cast<VectorPy*>(obj);

    VectorPy::PointerType this_ptr = getVectorPtr();
    VectorPy::PointerType vect_ptr = vec->getVectorPtr();

    Base::Vector3d v = (*this_ptr) % (*vect_ptr);
    return new VectorPy(v);
}

PyObject*  VectorPy::isOnLineSegment(PyObject *args)
{
    PyObject *start = nullptr, *end = nullptr;
    if (!PyArg_ParseTuple(args, "OO",&start, &end))
        return nullptr;
    if (!PyObject_TypeCheck(start, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return nullptr;
    }
    if (!PyObject_TypeCheck(end, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return nullptr;
    }

    VectorPy* start_vec = static_cast<VectorPy*>(start);
    VectorPy* end_vec = static_cast<VectorPy*>(end);

    VectorPy::PointerType this_ptr = getVectorPtr();
    VectorPy::PointerType start_ptr = start_vec->getVectorPtr();
    VectorPy::PointerType end_ptr = end_vec->getVectorPtr();

    Py::Boolean result = this_ptr->IsOnLineSegment(*start_ptr, *end_ptr);

    return Py::new_reference_to(result);
}

PyObject*  VectorPy::getAngle(PyObject *args)
{
    PyObject *obj = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &obj))
        return nullptr;

    VectorPy* vec = static_cast<VectorPy*>(obj);

    VectorPy::PointerType this_ptr = getVectorPtr();
    VectorPy::PointerType vect_ptr = vec->getVectorPtr();

    Py::Float angle(this_ptr->GetAngle(*vect_ptr));
    return Py::new_reference_to(angle);
}

PyObject*  VectorPy::normalize(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    VectorPy::PointerType ptr = getVectorPtr();
    if (ptr->Length() < Vector3d::epsilon()) {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "Cannot normalize null vector");
        return nullptr;
    }

    ptr->Normalize();

    return Py::new_reference_to(this);
}

PyObject*  VectorPy::projectToLine(PyObject *args)
{
    PyObject *base = nullptr, *line = nullptr;
    if (!PyArg_ParseTuple(args, "OO",&base, &line))
        return nullptr;
    if (!PyObject_TypeCheck(base, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return nullptr;
    }
    if (!PyObject_TypeCheck(line, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return nullptr;
    }

    VectorPy* base_vec = static_cast<VectorPy*>(base);
    VectorPy* line_vec = static_cast<VectorPy*>(line);

    VectorPy::PointerType this_ptr = getVectorPtr();
    VectorPy::PointerType base_ptr = base_vec->getVectorPtr();
    VectorPy::PointerType line_ptr = line_vec->getVectorPtr();

    this_ptr->ProjectToLine(*base_ptr, *line_ptr);

    return Py::new_reference_to(this);
}

PyObject*  VectorPy::projectToPlane(PyObject *args)
{
    PyObject *base = nullptr, *line = nullptr;
    if (!PyArg_ParseTuple(args, "OO",&base, &line))
        return nullptr;
    if (!PyObject_TypeCheck(base, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return nullptr;
    }
    if (!PyObject_TypeCheck(line, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return nullptr;
    }

    VectorPy* base_vec = static_cast<VectorPy*>(base);
    VectorPy* line_vec = static_cast<VectorPy*>(line);

    VectorPy::PointerType this_ptr = getVectorPtr();
    VectorPy::PointerType base_ptr = base_vec->getVectorPtr();
    VectorPy::PointerType line_ptr = line_vec->getVectorPtr();

    this_ptr->ProjectToPlane(*base_ptr, *line_ptr);

    return Py::new_reference_to(this);
}

PyObject*  VectorPy::distanceToPoint(PyObject *args)
{
    PyObject *pnt = nullptr;
    if (!PyArg_ParseTuple(args, "O!",&(VectorPy::Type),&pnt))
        return nullptr;

    VectorPy* base_vec = static_cast<VectorPy*>(pnt);
    VectorPy::PointerType this_ptr = getVectorPtr();
    VectorPy::PointerType base_ptr = base_vec->getVectorPtr();

    Py::Float dist(Base::Distance(*this_ptr, *base_ptr));
    return Py::new_reference_to(dist);
}

PyObject*  VectorPy::distanceToLine(PyObject *args)
{
    PyObject *base = nullptr, *line = nullptr;
    if (!PyArg_ParseTuple(args, "OO",&base, &line))
        return nullptr;
    if (!PyObject_TypeCheck(base, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return nullptr;
    }
    if (!PyObject_TypeCheck(line, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return nullptr;
    }

    VectorPy* base_vec = static_cast<VectorPy*>(base);
    VectorPy* line_vec = static_cast<VectorPy*>(line);

    VectorPy::PointerType this_ptr = getVectorPtr();
    VectorPy::PointerType base_ptr = base_vec->getVectorPtr();
    VectorPy::PointerType line_ptr = line_vec->getVectorPtr();

    Py::Float dist(this_ptr->DistanceToLine(*base_ptr, *line_ptr));
    return Py::new_reference_to(dist);
}

PyObject*  VectorPy::distanceToLineSegment(PyObject *args)
{
    PyObject *base = nullptr, *line = nullptr;
    if (!PyArg_ParseTuple(args, "OO",&base, &line))
        return nullptr;
    if (!PyObject_TypeCheck(base, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return nullptr;
    }
    if (!PyObject_TypeCheck(line, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return nullptr;
    }

    VectorPy* base_vec = static_cast<VectorPy*>(base);
    VectorPy* line_vec = static_cast<VectorPy*>(line);

    VectorPy::PointerType this_ptr = getVectorPtr();
    VectorPy::PointerType base_ptr = base_vec->getVectorPtr();
    VectorPy::PointerType line_ptr = line_vec->getVectorPtr();

    Vector3d v = this_ptr->DistanceToLineSegment(*base_ptr, *line_ptr);
    return new VectorPy(v);
}

PyObject*  VectorPy::distanceToPlane(PyObject *args)
{
    PyObject *base = nullptr, *line = nullptr;
    if (!PyArg_ParseTuple(args, "OO",&base, &line))
        return nullptr;
    if (!PyObject_TypeCheck(base, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return nullptr;
    }
    if (!PyObject_TypeCheck(line, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return nullptr;
    }

    VectorPy* base_vec = static_cast<VectorPy*>(base);
    VectorPy* line_vec = static_cast<VectorPy*>(line);

    VectorPy::PointerType this_ptr = getVectorPtr();
    VectorPy::PointerType base_ptr = base_vec->getVectorPtr();
    VectorPy::PointerType line_ptr = line_vec->getVectorPtr();

    Py::Float dist(this_ptr->DistanceToPlane(*base_ptr, *line_ptr));
    return Py::new_reference_to(dist);
}

Py::Float VectorPy::getLength() const
{
    VectorPy::PointerType ptr = getVectorPtr();
    return Py::Float(ptr->Length());
}

void  VectorPy::setLength(Py::Float arg)
{
    VectorPy::PointerType ptr = getVectorPtr();
    double len = ptr->Length();
    if (len < Vector3d::epsilon()) {
        throw Py::RuntimeError(std::string("Cannot set length of null vector"));
    }

    double val = static_cast<double>(arg)/len;
    ptr->x *= val;
    ptr->y *= val;
    ptr->z *= val;
}

Py::Float VectorPy::getx() const
{
    VectorPy::PointerType ptr = getVectorPtr();
    return Py::Float(ptr->x);
}

void  VectorPy::setx(Py::Float arg)
{
    VectorPy::PointerType ptr = getVectorPtr();
    ptr->x = static_cast<double>(arg);
}

Py::Float VectorPy::gety() const
{
    VectorPy::PointerType ptr = getVectorPtr();
    return Py::Float(ptr->y);
}

void  VectorPy::sety(Py::Float arg)
{
    VectorPy::PointerType ptr = getVectorPtr();
    ptr->y = static_cast<double>(arg);
}

Py::Float VectorPy::getz() const
{
    VectorPy::PointerType ptr = getVectorPtr();
    return Py::Float(ptr->z);
}

void  VectorPy::setz(Py::Float arg)
{
    VectorPy::PointerType ptr = getVectorPtr();
    ptr->z = static_cast<double>(arg);
}

PyObject *VectorPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int VectorPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

// TODO for v0.18:
// In generation script allow to more precisely define which slots
// of the number protocol should be supported instead of setting all.

PyObject * VectorPy::number_divide_handler (PyObject* self, PyObject* other)
{
    if (PyObject_TypeCheck(self, &(VectorPy::Type)) &&
        PyNumber_Check(other)) {
        // Vector passes PyNumber_Check because it sets nb_int and nb_float
        // slots of the PyNumberMethods structure. So, it must be explicitly
        // filered out here.
        if (PyObject_TypeCheck(other, &(VectorPy::Type))) {
            PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for /: '%s' and '%s'",
                         Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
            return nullptr;
        }

        Base::Vector3d vec = static_cast<VectorPy*>(self) ->value();
        double div = PyFloat_AsDouble(other);
        if (div == 0.0) {
            PyErr_Format(PyExc_ZeroDivisionError, "'%s' division by zero",
                         Py_TYPE(self)->tp_name);
            return nullptr;
        }

        vec /= div;
        return new VectorPy(vec);
    }

    PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for /: '%s' and '%s'",
                 Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return nullptr;
}

PyObject * VectorPy::number_remainder_handler (PyObject* self, PyObject* other)
{
    if (PyObject_TypeCheck(self, &(VectorPy::Type)) &&
        PyObject_TypeCheck(other, &(VectorPy::Type)))
    {
        Base::Vector3d a = static_cast<VectorPy*>(self) ->value();
        Base::Vector3d b = static_cast<VectorPy*>(other) ->value();
        return new VectorPy(a % b);
    }

    PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for %%: '%s' and '%s'",
                 Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return nullptr;
}

PyObject * VectorPy::number_divmod_handler (PyObject* self, PyObject* other)
{
    PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for divmod(): '%s' and '%s'",
                 Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return nullptr;
}

PyObject * VectorPy::number_power_handler (PyObject* self, PyObject* other, PyObject* /*arg*/)
{
    PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for ** or pow(): '%s' and '%s'",
                 Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return nullptr;
}

PyObject * VectorPy::number_negative_handler (PyObject* self)
{
    if (PyObject_TypeCheck(self, &(VectorPy::Type))) {
        Base::Vector3d vec = static_cast<VectorPy*>(self) ->value();
        return new VectorPy(-vec);
    }

    PyErr_Format(PyExc_TypeError, "bad operand type for unary -: '%s'",
                 Py_TYPE(self)->tp_name);
    return nullptr;
}

PyObject * VectorPy::number_positive_handler (PyObject* self)
{
    if (PyObject_TypeCheck(self, &(VectorPy::Type))) {
        Base::Vector3d vec = static_cast<VectorPy*>(self) ->value();
        return new VectorPy(vec);
    }

    PyErr_Format(PyExc_TypeError, "bad operand type for unary +: '%s'",
                 Py_TYPE(self)->tp_name);
    return nullptr;
}

PyObject * VectorPy::number_absolute_handler (PyObject* self)
{
    if (PyObject_TypeCheck(self, &(VectorPy::Type))) {
        Base::Vector3d vec = static_cast<VectorPy*>(self) ->value();
        vec.x = fabs(vec.x);
        vec.y = fabs(vec.y);
        vec.z = fabs(vec.z);
        return new VectorPy(vec);
    }

    PyErr_Format(PyExc_TypeError, "bad operand type for abs(): '%s'",
                 Py_TYPE(self)->tp_name);
    return nullptr;
}

int VectorPy::number_nonzero_handler (PyObject* /*self*/)
{
    return 1;
}

PyObject * VectorPy::number_invert_handler (PyObject* self)
{
    PyErr_Format(PyExc_TypeError, "bad operand type for unary ~: '%s'",
                 Py_TYPE(self)->tp_name);
    return nullptr;
}

PyObject * VectorPy::number_lshift_handler (PyObject* self, PyObject* other)
{
    PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for <<: '%s' and '%s'",
                 Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return nullptr;
}

PyObject * VectorPy::number_rshift_handler (PyObject* self, PyObject* other)
{
    PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for >>: '%s' and '%s'",
                 Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return nullptr;
}

PyObject * VectorPy::number_and_handler (PyObject* self, PyObject* other)
{
    PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for &: '%s' and '%s'",
                 Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return nullptr;
}

PyObject * VectorPy::number_xor_handler (PyObject* self, PyObject* other)
{
    PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for ^: '%s' and '%s'",
                 Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return nullptr;
}

PyObject * VectorPy::number_or_handler (PyObject* self, PyObject* other)
{
    PyErr_Format(PyExc_TypeError, "unsupported operand type(s) for |: '%s' and '%s'",
                 Py_TYPE(self)->tp_name, Py_TYPE(other)->tp_name);
    return nullptr;
}

PyObject * VectorPy::number_int_handler (PyObject* self)
{
    PyErr_Format(PyExc_TypeError, "int() argument must be a string or a number, not '%s'",
                 Py_TYPE(self)->tp_name);
    return nullptr;
}

PyObject * VectorPy::number_float_handler (PyObject* self)
{
    PyErr_Format(PyExc_TypeError, "float() argument must be a string or a number, not '%s'",
                 Py_TYPE(self)->tp_name);
    return nullptr;
}
