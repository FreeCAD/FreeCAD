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
std::string VectorPy::representation(void) const
{
    VectorPy::PointerType ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    Py::Float x(ptr->x);
    Py::Float y(ptr->y);
    Py::Float z(ptr->z);
    std::stringstream str;
    str << "Vector (";
    str << (std::string)x.repr() << ", "<< (std::string)y.repr() << ", "<< (std::string)z.repr();
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
    PyObject *object;
    VectorPy::PointerType ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    if (PyArg_ParseTuple(args, "|ddd", &x,&y,&z)) {
        ptr->Set(x,y,z);
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()
    if (PyArg_ParseTuple(args,"O!",&(Base::VectorPy::Type), &object)) {
        // Note: must be static_cast, not reinterpret_cast
        *ptr = *(static_cast<Base::VectorPy*>(object)->getVectorPtr());
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()
    if (PyArg_ParseTuple(args,"O!",&(PyTuple_Type), &object)) {
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

PyObject* VectorPy::number_add_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return 0;
    }
    if (!PyObject_TypeCheck(other, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return 0;
    }
    Base::Vector3d a = static_cast<VectorPy*>(self)->value();
    Base::Vector3d b = static_cast<VectorPy*>(other)->value();
    return new VectorPy(a+b);
}

PyObject* VectorPy::number_subtract_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return 0;
    }
    if (!PyObject_TypeCheck(other, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return 0;
    }
    Base::Vector3d a = static_cast<VectorPy*>(self)->value();
    Base::Vector3d b = static_cast<VectorPy*>(other)->value();
    return new VectorPy(a-b);
}

PyObject* VectorPy::number_multiply_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return 0;
    }

    if (PyObject_TypeCheck(other, &(VectorPy::Type))) {
        Base::Vector3d a = static_cast<VectorPy*>(self) ->value();
        Base::Vector3d b = static_cast<VectorPy*>(other)->value();
        Py::Float mult(a * b);
        return Py::new_reference_to(mult);
    }
    else if (PyFloat_Check(other)) {
        Base::Vector3d a = static_cast<VectorPy*>(self) ->value();
        double b = PyFloat_AsDouble(other);
        return new VectorPy(a * b);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "A Vector can only be multiplied by Vector or number");
        return 0;
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
        return 0;
    }
    if (index < 0 || index > 2) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return 0;
    }

    Base::Vector3d a = static_cast<VectorPy*>(self)->value();
    return Py_BuildValue("d", a[index]);
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

    if (PyFloat_Check(value)) {
        VectorPy::PointerType ptr = static_cast<VectorPy*>(self)->getVectorPtr();
        (*ptr)[index] = PyFloat_AsDouble(value);
    }
    else {
        PyErr_SetString(PyExc_ValueError, "value must be float");
        return -1;
    }

    return 0;
}

PyObject*  VectorPy::add(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &obj))
        return 0;

    VectorPy* vec = static_cast<VectorPy*>(obj);

    VectorPy::PointerType this_ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    VectorPy::PointerType vect_ptr = reinterpret_cast<VectorPy::PointerType>(vec->_pcTwinPointer);

    Base::Vector3d v = (*this_ptr) + (*vect_ptr);
    return new VectorPy(v);
}

PyObject*  VectorPy::sub(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &obj))
        return 0;

    VectorPy* vec = static_cast<VectorPy*>(obj);

    VectorPy::PointerType this_ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    VectorPy::PointerType vect_ptr = reinterpret_cast<VectorPy::PointerType>(vec->_pcTwinPointer);

    Base::Vector3d v = (*this_ptr) - (*vect_ptr);
    return new VectorPy(v);
}

PyObject*  VectorPy::negative(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
        
    VectorPy::PointerType this_ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    Base::Vector3d v = -(*this_ptr);
    return new VectorPy(v);
}

PyObject* VectorPy::richCompare(PyObject *v, PyObject *w, int op)
{
    if (PyObject_TypeCheck(v, &(VectorPy::Type)) &&
        PyObject_TypeCheck(w, &(VectorPy::Type))) {
        Vector3d v1 = static_cast<VectorPy*>(v)->value();
        Vector3d v2 = static_cast<VectorPy*>(w)->value();

        PyObject *res=0;
        if (op != Py_EQ && op != Py_NE) {
            PyErr_SetString(PyExc_TypeError,
            "no ordering relation is defined for Vector");
            return 0;
        }
        else if (op == Py_EQ) {
            res = (v1 == v2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else {
            res = (v1 != v2) ? Py_True : Py_False;
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

PyObject*  VectorPy::scale(PyObject *args)
{
    double factorX, factorY, factorZ;
    if (!PyArg_ParseTuple(args, "ddd", &factorX, &factorY, &factorZ))
        return 0;
    VectorPy::PointerType ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    ptr->Scale(factorX, factorY, factorZ);

    return Py::new_reference_to(this);
}

PyObject*  VectorPy::multiply(PyObject *args)
{
    double factor;
    if (!PyArg_ParseTuple(args, "d", &factor))
        return 0;
    VectorPy::PointerType ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    ptr->Scale(factor, factor, factor);

    return Py::new_reference_to(this);
}

PyObject*  VectorPy::dot(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &obj))
        return 0;

    VectorPy* vec = static_cast<VectorPy*>(obj);

    VectorPy::PointerType this_ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    VectorPy::PointerType vect_ptr = reinterpret_cast<VectorPy::PointerType>(vec->_pcTwinPointer);

    Py::Float mult((*this_ptr) * (*vect_ptr));
    return Py::new_reference_to(mult);
}

PyObject*  VectorPy::cross(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &obj))
        return 0;

    VectorPy* vec = static_cast<VectorPy*>(obj);

    VectorPy::PointerType this_ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    VectorPy::PointerType vect_ptr = reinterpret_cast<VectorPy::PointerType>(vec->_pcTwinPointer);

    Base::Vector3d v = (*this_ptr) % (*vect_ptr);
    return new VectorPy(v);
}

PyObject*  VectorPy::getAngle(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &obj))
        return 0;

    VectorPy* vec = static_cast<VectorPy*>(obj);

    VectorPy::PointerType this_ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    VectorPy::PointerType vect_ptr = reinterpret_cast<VectorPy::PointerType>(vec->_pcTwinPointer);

    Py::Float angle(this_ptr->GetAngle(*vect_ptr));
    return Py::new_reference_to(angle);
}

PyObject*  VectorPy::normalize(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    VectorPy::PointerType ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    if (ptr->Length() < 1.0e-6) {
        PyErr_SetString(PyExc_Exception, "Cannot normalize null vector");
        return 0;
    }

    ptr->Normalize();

    return Py::new_reference_to(this);
}

PyObject*  VectorPy::projectToLine(PyObject *args)
{
    PyObject *base, *line;
    if (!PyArg_ParseTuple(args, "OO",&base, &line))
        return 0;
    if (!PyObject_TypeCheck(base, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return 0;
    }
    if (!PyObject_TypeCheck(line, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return 0;
    }

    VectorPy* base_vec = static_cast<VectorPy*>(base);
    VectorPy* line_vec = static_cast<VectorPy*>(line);

    VectorPy::PointerType this_ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    VectorPy::PointerType base_ptr = reinterpret_cast<VectorPy::PointerType>(base_vec->_pcTwinPointer);
    VectorPy::PointerType line_ptr = reinterpret_cast<VectorPy::PointerType>(line_vec->_pcTwinPointer);

    this_ptr->ProjToLine(*base_ptr, *line_ptr);

    return Py::new_reference_to(this);
}

PyObject*  VectorPy::projectToPlane(PyObject *args)
{
    PyObject *base, *line;
    if (!PyArg_ParseTuple(args, "OO",&base, &line))
        return 0;
    if (!PyObject_TypeCheck(base, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return 0;
    }
    if (!PyObject_TypeCheck(line, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return 0;
    }

    VectorPy* base_vec = static_cast<VectorPy*>(base);
    VectorPy* line_vec = static_cast<VectorPy*>(line);

    VectorPy::PointerType this_ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    VectorPy::PointerType base_ptr = reinterpret_cast<VectorPy::PointerType>(base_vec->_pcTwinPointer);
    VectorPy::PointerType line_ptr = reinterpret_cast<VectorPy::PointerType>(line_vec->_pcTwinPointer);

    this_ptr->ProjToPlane(*base_ptr, *line_ptr);

    return Py::new_reference_to(this);
}

PyObject*  VectorPy::distanceToPoint(PyObject *args)
{
    PyObject *pnt;
    if (!PyArg_ParseTuple(args, "O!",&(VectorPy::Type),&pnt))
        return 0;

    VectorPy* base_vec = static_cast<VectorPy*>(pnt);
    VectorPy::PointerType this_ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    VectorPy::PointerType base_ptr = reinterpret_cast<VectorPy::PointerType>(base_vec->_pcTwinPointer);

    Py::Float dist(Base::Distance(*this_ptr, *base_ptr));
    return Py::new_reference_to(dist);
}

PyObject*  VectorPy::distanceToLine(PyObject *args)
{
    PyObject *base, *line;
    if (!PyArg_ParseTuple(args, "OO",&base, &line))
        return 0;
    if (!PyObject_TypeCheck(base, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return 0;
    }
    if (!PyObject_TypeCheck(line, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return 0;
    }

    VectorPy* base_vec = static_cast<VectorPy*>(base);
    VectorPy* line_vec = static_cast<VectorPy*>(line);

    VectorPy::PointerType this_ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    VectorPy::PointerType base_ptr = reinterpret_cast<VectorPy::PointerType>(base_vec->_pcTwinPointer);
    VectorPy::PointerType line_ptr = reinterpret_cast<VectorPy::PointerType>(line_vec->_pcTwinPointer);

    Py::Float dist(this_ptr->DistanceToLine(*base_ptr, *line_ptr));
    return Py::new_reference_to(dist);
}

PyObject*  VectorPy::distanceToLineSegment(PyObject *args)
{
    PyObject *base, *line;
    if (!PyArg_ParseTuple(args, "OO",&base, &line))
        return 0;
    if (!PyObject_TypeCheck(base, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return 0;
    }
    if (!PyObject_TypeCheck(line, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return 0;
    }

    VectorPy* base_vec = static_cast<VectorPy*>(base);
    VectorPy* line_vec = static_cast<VectorPy*>(line);

    VectorPy::PointerType this_ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    VectorPy::PointerType base_ptr = reinterpret_cast<VectorPy::PointerType>(base_vec->_pcTwinPointer);
    VectorPy::PointerType line_ptr = reinterpret_cast<VectorPy::PointerType>(line_vec->_pcTwinPointer);

    Vector3d v = this_ptr->DistanceToLineSegment(*base_ptr, *line_ptr);
    return new VectorPy(v);
}

PyObject*  VectorPy::distanceToPlane(PyObject *args)
{
    PyObject *base, *line;
    if (!PyArg_ParseTuple(args, "OO",&base, &line))
        return 0;
    if (!PyObject_TypeCheck(base, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Vector");
        return 0;
    }
    if (!PyObject_TypeCheck(line, &(VectorPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Vector");
        return 0;
    }

    VectorPy* base_vec = static_cast<VectorPy*>(base);
    VectorPy* line_vec = static_cast<VectorPy*>(line);

    VectorPy::PointerType this_ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    VectorPy::PointerType base_ptr = reinterpret_cast<VectorPy::PointerType>(base_vec->_pcTwinPointer);
    VectorPy::PointerType line_ptr = reinterpret_cast<VectorPy::PointerType>(line_vec->_pcTwinPointer);

    Py::Float dist(this_ptr->DistanceToPlane(*base_ptr, *line_ptr));
    return Py::new_reference_to(dist);
}

Py::Float VectorPy::getLength(void) const
{
    VectorPy::PointerType ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    return Py::Float(ptr->Length());
}

void  VectorPy::setLength(Py::Float arg)
{
    VectorPy::PointerType ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    double len = ptr->Length();
    if (len < 1.0e-6) {
        throw Py::Exception(std::string("Cannot set length of null vector"));
    }

    double val = (double)arg/len;
    ptr->x *= val;
    ptr->y *= val;
    ptr->z *= val;
}

Py::Float VectorPy::getx(void) const
{
    VectorPy::PointerType ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    return Py::Float(ptr->x);
}

void  VectorPy::setx(Py::Float arg)
{
    VectorPy::PointerType ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    ptr->x = (double)arg;
}

Py::Float VectorPy::gety(void) const
{
    VectorPy::PointerType ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    return Py::Float(ptr->y);
}

void  VectorPy::sety(Py::Float arg)
{
    VectorPy::PointerType ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    ptr->y = (double)arg;
}

Py::Float VectorPy::getz(void) const
{
    VectorPy::PointerType ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    return Py::Float(ptr->z);
}

void  VectorPy::setz(Py::Float arg)
{
    VectorPy::PointerType ptr = reinterpret_cast<VectorPy::PointerType>(_pcTwinPointer);
    ptr->z = (double)arg;
}

PyObject *VectorPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int VectorPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


