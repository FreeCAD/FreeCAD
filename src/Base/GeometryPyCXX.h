// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <CXX/Extensions.hxx>
#include <FCGlobal.h>

#include <Base/BoundBoxPy.h>
#include <Base/MatrixPy.h>
#include <Base/RotationPy.h>
#include <Base/PlacementPy.h>


namespace Base
{
class Matrix4D;
class Rotation;
class Placement;
template<typename T>
class Vector3;
using Vector3d = Vector3<double>;
using Vector3f = Vector3<float>;

template<typename T>
inline Vector3<T> getVectorFromTuple(PyObject* py)
{
    Py::Sequence tuple(py);
    if (tuple.size() != 3) {
        throw Py::ValueError("Expected sequence of size 3");
    }

    T vx = static_cast<T>(Py::Float(tuple[0]));
    T vy = static_cast<T>(Py::Float(tuple[1]));
    T vz = static_cast<T>(Py::Float(tuple[2]));

    return Vector3<T>(vx, vy, vz);
}

class BaseExport Vector2dPy: public Py::PythonClass<Vector2dPy>  // NOLINT
{
public:
    static Py::PythonType& behaviors();
    static PyTypeObject* type_object();
    static bool check(PyObject* py);

    static Py::PythonClassObject<Vector2dPy> create(const Vector2d&);
    static Py::PythonClassObject<Vector2dPy> create(double vx, double vy);
    Vector2dPy(Py::PythonClassInstance* self, Py::Tuple& args, Py::Dict& kwds);
    ~Vector2dPy() override;

    static void init_type();
    Py::Object getattro(const Py::String& name_) override;
    int setattro(const Py::String& name_, const Py::Object& value) override;
    Py::Object repr() override;
    inline const Vector2d& value() const
    {
        return v;
    }
    inline void setValue(const Vector2d& n)
    {
        v = n;
    }
    inline void setValue(double vx, double vy)
    {
        v.x = vx;
        v.y = vy;
    }

    /** @name methods for group handling */
    //@{
    Py::Object number_negative() override;
    Py::Object number_positive() override;
    Py::Object number_absolute() override;
    Py::Object number_invert() override;
    Py::Object number_int() override;
    Py::Object number_float() override;
    Py::Object number_add(const Py::Object&) override;
    Py::Object number_subtract(const Py::Object&) override;
    Py::Object number_multiply(const Py::Object&) override;
    Py::Object number_remainder(const Py::Object&) override;
    Py::Object number_divmod(const Py::Object&) override;
    Py::Object number_lshift(const Py::Object&) override;
    Py::Object number_rshift(const Py::Object&) override;
    Py::Object number_and(const Py::Object&) override;
    Py::Object number_xor(const Py::Object&) override;
    Py::Object number_or(const Py::Object&) override;
    Py::Object number_power(const Py::Object&, const Py::Object&) override;
    //@}

    Py::Object isNull(const Py::Tuple&);
    Py::Object length(const Py::Tuple&);
    Py::Object atan2(const Py::Tuple&);
    Py::Object square(const Py::Tuple&);
    Py::Object scale(const Py::Tuple&);
    Py::Object rotate(const Py::Tuple&);
    Py::Object normalize(const Py::Tuple&);
    Py::Object perpendicular(const Py::Tuple&);
    Py::Object distance(const Py::Tuple&);
    Py::Object isEqual(const Py::Tuple&);
    Py::Object getAngle(const Py::Tuple&);
    Py::Object projectToLine(const Py::Tuple&);

private:
    Vector2d v;
};

}  // namespace Base

namespace Py
{

using Vector2d = PythonClassObject<Base::Vector2dPy>;

inline Base::Vector2d toVector2d(PyObject* py)
{
    Base::Vector2dPy* py2d = Py::Vector2d(py).getCxxObject();
    return py2d ? py2d->value() : Base::Vector2d();
}

inline Base::Vector2d toVector2d(const Object& py)
{
    Base::Vector2dPy* py2d = Py::Vector2d(py).getCxxObject();
    return py2d ? py2d->value() : Base::Vector2d();
}

// Implementing the vector class in the fashion of the PyCXX library.
class BaseExport Vector: public Object  // NOLINT
{
public:
    explicit Vector(PyObject* pyob, bool owned)
        : Object(pyob, owned)
    {
        validate();
    }

    Vector(const Vector& ob)
        : Object(ob)
    {
        validate();
    }

    explicit Vector(const Base::Vector3d&);
    explicit Vector(const Base::Vector3f&);
    bool accepts(PyObject* obj) const override;

    Vector(const Object& other)
        : Object(other.ptr())
    {
        validate();
    }
    Vector& operator=(const Object& rhs)
    {
        return (*this = *rhs);
    }
    Vector& operator=(const Vector& rhs)
    {
        return (*this = *rhs);
    }

    Vector& operator=(PyObject* rhsp);
    Vector& operator=(const Base::Vector3d&);
    Vector& operator=(const Base::Vector3f&);
    operator Base::Vector3d() const
    {
        return toVector();
    }

    Base::Vector3d toVector() const;

private:
    static int Vector_TypeCheck(PyObject*);
};

/**
 * This is a template class to provide wrapper classes for geometric classes like
 * Base::Matrix4D, Base::Rotation and their Python binding classes.
 * Since the class inherits from Py::Object it can be used in the same fashion as
 * Py::String, Py::List, etc. to simplify the usage with them.
 * @author Werner Mayer
 */
// The first template parameter represents the basic geometric class e.g. Rotation,
// the second parameter is reserved for its Python binding class, i.e. RotationPy.
// The third template parameter is the definition of a pointer to the method
// of the Python binding class to return the managed geometric instance. In our
// example this is the method RotationPy::getRotationPtr.
template<class T, class PyT, T* (PyT::*valuePtr)() const>
class GeometryT: public Object  // NOLINT
{
public:
    explicit GeometryT(PyObject* pyob, bool owned)
        : Object(pyob, owned)
    {
        validate();
    }
    GeometryT(const GeometryT& ob)
        : Object(*ob)
    {
        validate();
    }
    explicit GeometryT()
    {
        set(new PyT(new T()), true);
        validate();
    }
    explicit GeometryT(const T& val)
    {
        set(new PyT(new T(val)), true);
        validate();
    }
    GeometryT(const Object& other)
        : Object(other.ptr())
    {
        validate();
    }
    bool accepts(PyObject* pyob) const override
    {
        return pyob && Geometry_TypeCheck(pyob);
    }
    GeometryT& operator=(const Object& rhs)
    {
        *this = *rhs;
        return *this;
    }
    GeometryT& operator=(PyObject* rhsp)
    {
        if (ptr() == rhsp) {
            return *this;
        }
        set(rhsp, false);
        return *this;
    }
    GeometryT& operator=(const T& val)
    {
        set(new PyT(val), true);
        return *this;
    }
    const T& getValue() const
    {
        // cast the PyObject pointer to the matching sub-class
        // and call then the defined member function
        PyT* py = static_cast<PyT*>(ptr());
        T* val = (py->*valuePtr)();
        return *val;
    }
    operator T() const
    {
        return getValue();
    }
    PyT* getPy() const
    {
        PyT* py = static_cast<PyT*>(ptr());
        return py;
    }

private:
    static int Geometry_TypeCheck(PyObject* obj)
    {
        return PyObject_TypeCheck(obj, &(PyT::Type));
    }
};

// PyCXX wrapper classes Py::Matrix, Py::Rotation, Py::Placement, ...
using BoundingBox = GeometryT<Base::BoundBox3d, Base::BoundBoxPy, &Base::BoundBoxPy::getBoundBoxPtr>;
using Matrix = GeometryT<Base::Matrix4D, Base::MatrixPy, &Base::MatrixPy::getMatrixPtr>;
using Rotation = GeometryT<Base::Rotation, Base::RotationPy, &Base::RotationPy::getRotationPtr>;
using Placement = GeometryT<Base::Placement, Base::PlacementPy, &Base::PlacementPy::getPlacementPtr>;

}  // namespace Py
