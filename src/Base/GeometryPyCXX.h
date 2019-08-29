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


#ifndef PY_GEOMETRYPY_H
#define PY_GEOMETRYPY_H

#include <CXX/Objects.hxx>
#include <CXX/Extensions.hxx>
#include <Base/Vector3D.h>
#include <Base/Matrix.h>
#include <Base/MatrixPy.h>
#include <Base/Rotation.h>
#include <Base/RotationPy.h>
#include <Base/Placement.h>
#include <Base/PlacementPy.h>
#include <Base/BoundBoxPy.h>
#include <Base/Tools2D.h>

namespace Base {
template <typename T>
inline Vector3<T> getVectorFromTuple(PyObject* o)
{
    Py::Sequence tuple(o);
    T x = (T)Py::Float(tuple.getItem(0));
    T y = (T)Py::Float(tuple.getItem(1));
    T z = (T)Py::Float(tuple.getItem(2));
    return Vector3<T>(x,y,z);
}

class BaseExport Vector2dPy : public Py::PythonClass<Vector2dPy>
{
public:
    Vector2dPy(Py::PythonClassInstance *self, Py::Tuple &args, Py::Dict &kwds);
    virtual ~Vector2dPy();

    static void init_type(void);
    Py::Object getattro(const Py::String &name_);
    int setattro(const Py::String &name_, const Py::Object &value);
    virtual Py::Object repr();
    inline const Vector2d& value() const {
        return v;
    }
    inline void setValue(const Vector2d& n) {
        v = n;
    }
    inline void setValue(double x, double y) {
        v.x = x;
        v.y = y;
    }

private:
    Vector2d v;
};

}

namespace Py {

typedef PythonClassObject<Base::Vector2dPy> Vector2d;

inline Base::Vector2d toVector2d(PyObject *py) {
    Base::Vector2dPy* py2d = Py::Vector2d(py).getCxxObject();
    return py2d ? py2d->value() : Base::Vector2d();
}

inline Base::Vector2d toVector2d(const Object& py) {
    Base::Vector2dPy* py2d = Py::Vector2d(py).getCxxObject();
    return py2d ? py2d->value() : Base::Vector2d();
}

// Implementing the vector class in the fashion of the PyCXX library.
class BaseExport Vector : public Object
{
public:
    explicit Vector (PyObject *pyob, bool owned): Object(pyob, owned) {
        validate();
    }

    Vector (const Vector& ob): Object(*ob) {
        validate();
    }

    explicit Vector (const Base::Vector3d&);
    explicit Vector (const Base::Vector3f&);
    virtual bool accepts (PyObject *pyob) const;

    Vector(const Object& other): Object(other.ptr()) {
        validate();
    }
    Vector& operator= (const Object& rhs)
    {
        return (*this = *rhs);
    }

    Vector& operator= (PyObject* rhsp);
    Vector& operator= (const Base::Vector3d&);
    Vector& operator= (const Base::Vector3f&);

    Base::Vector3d toVector() const;

private:
    static int Vector_TypeCheck(PyObject *);
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
template <class T, class PyT, T* (PyT::*valuePtr)() const>
class GeometryT : public Object
{
public:
    explicit GeometryT (PyObject *pyob, bool owned): Object(pyob, owned) {
        validate();
    }
    GeometryT (const GeometryT& ob): Object(*ob) {
        validate();
    }
    explicit GeometryT ()
    {
        set(new PyT(new T()), true);
        validate();
    }
    explicit GeometryT (const T& v)
    {
        set(new PyT(new T(v)), true);
        validate();
    }
    GeometryT(const Object& other): Object(other.ptr()) {
        validate();
    }
    virtual bool accepts (PyObject *pyob) const {
        return pyob && Geometry_TypeCheck (pyob);
    }
    GeometryT& operator= (const Object& rhs)
    {
        return (*this = *rhs);
    }
    GeometryT& operator= (PyObject* rhsp)
    {
        if(ptr() == rhsp) return *this;
        set (rhsp, false);
        return *this;
    }
    GeometryT& operator= (const T& v)
    {
        set (new PyT(v), true);
        return *this;
    }
    const T& getValue() const
    {
        // cast the PyObject pointer to the matching sub-class
        // and call then the defined member function
        PyT* py = static_cast<PyT*>(ptr());
        T* v = (py->*valuePtr)();
        return *v;
    }
    operator T() const
    {
        // cast the PyObject pointer to the matching sub-class
        // and call then the defined member function
        PyT* py = static_cast<PyT*>(ptr());
        T* v = (py->*valuePtr)();
        return *v;
    }

private:
    static int Geometry_TypeCheck(PyObject * obj)
    {
        return PyObject_TypeCheck(obj, &(PyT::Type));
    }
};

// PyCXX wrapper classes Py::Matrix, Py::Rotation, Py::Placement, ...
typedef GeometryT<Base::BoundBox3d, Base::BoundBoxPy,
                 &Base::BoundBoxPy::getBoundBoxPtr>     BoundingBox;
typedef GeometryT<Base::Matrix4D, Base::MatrixPy,
                 &Base::MatrixPy::getMatrixPtr>         Matrix;
typedef GeometryT<Base::Rotation, Base::RotationPy,
                 &Base::RotationPy::getRotationPtr>     Rotation;
typedef GeometryT<Base::Placement, Base::PlacementPy,
                 &Base::PlacementPy::getPlacementPtr>   Placement;

}

#endif // PY_GEOMETRYPY_H
