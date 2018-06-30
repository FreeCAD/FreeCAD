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

#include <Base/Rotation.h>
#include <Base/Tools.h>
#include <Base/GeometryPyCXX.h>

// inclusion of the generated files (generated out of RotationPy.xml)
#include "VectorPy.h"
#include "RotationPy.h"
#include "RotationPy.cpp"

using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string RotationPy::representation(void) const
{
    RotationPy::PointerType ptr = reinterpret_cast<RotationPy::PointerType>(_pcTwinPointer);
    Py::Float q0(ptr->getValue()[0]);
    Py::Float q1(ptr->getValue()[1]);
    Py::Float q2(ptr->getValue()[2]);
    Py::Float q3(ptr->getValue()[3]);
    std::stringstream str;
    str << "Rotation (";
    str << (std::string)q0.repr() << ", "
        << (std::string)q1.repr() << ", "
        << (std::string)q2.repr() << ", "
        << (std::string)q3.repr();
    str << ")";

    return str.str();
}

PyObject *RotationPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of RotationPy and the Twin object 
    return new RotationPy(new Rotation);
}

// constructor method
int RotationPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(Base::RotationPy::Type), &o)) {
        Base::Rotation *rot = static_cast<Base::RotationPy*>(o)->getRotationPtr();
        getRotationPtr()->setValue(rot->getValue());
        return 0;
    }

    PyErr_Clear();
    double angle;
    if (PyArg_ParseTuple(args, "O!d", &(Base::VectorPy::Type), &o, &angle)) {
      // NOTE: The last parameter defines the rotation angle in degree.
      getRotationPtr()->setValue(static_cast<Base::VectorPy*>(o)->value(), Base::toRadians<double>(angle));
      return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type), &o)) {
      getRotationPtr()->setValue(static_cast<Base::MatrixPy*>(o)->value());
      return 0;
    }

    PyErr_Clear();
    double q0, q1, q2, q3;
    if (PyArg_ParseTuple(args, "dddd", &q0, &q1, &q2, &q3)) {
        getRotationPtr()->setValue(q0, q1, q2, q3);
        return 0;
    }

    PyErr_Clear();
    double y, p, r;
    if (PyArg_ParseTuple(args, "ddd", &y, &p, &r)) {
        getRotationPtr()->setYawPitchRoll(y, p, r);
        return 0;
    }

    double a11 = 1.0, a12 = 0.0, a13 = 0.0, a14 = 0.0;
    double a21 = 0.0, a22 = 1.0, a23 = 0.0, a24 = 0.0;
    double a31 = 0.0, a32 = 0.0, a33 = 1.0, a34 = 0.0;
    double a41 = 0.0, a42 = 0.0, a43 = 0.0, a44 = 1.0;

    // try read a 4x4 matrix
    PyErr_Clear();
    if (PyArg_ParseTuple(args, "dddddddddddddddd",
      &a11, &a12, &a13, &a14,
      &a21, &a22, &a23, &a24,
      &a31, &a32, &a33, &a34,
      &a41, &a42, &a43, &a44))
    {
      Matrix4D mtx(a11, a12, a13, a14,
        a21, a22, a23, a24,
        a31, a32, a33, a34,
        a41, a42, a43, a44);
      getRotationPtr()->setValue(mtx);
      return 0;
    }

    // try read a 3x3 matrix
    PyErr_Clear();
    if (PyArg_ParseTuple(args, "ddddddddd",
      &a11, &a12, &a13,
      &a21, &a22, &a23,
      &a31, &a32, &a33))
    {
      Matrix4D mtx(a11, a12, a13, a14,
        a21, a22, a23, a24,
        a31, a32, a33, a34,
        a41, a42, a43, a44);
      getRotationPtr()->setValue(mtx);
      return 0;
    }


    PyErr_Clear();
    PyObject *v1, *v2;
    if (PyArg_ParseTuple(args, "O!O!", &(Base::VectorPy::Type), &v1,
                                       &(Base::VectorPy::Type), &v2)) {
        Py::Vector from(v1, false);
        Py::Vector to(v2, false);
        getRotationPtr()->setValue(from.toVector(), to.toVector());
        return 0;
    }

    PyErr_Clear();
    PyObject *v3;
    char *priority = nullptr;
    if (PyArg_ParseTuple(args, "O!O!O!|s", &(Base::VectorPy::Type), &v1,
                                           &(Base::VectorPy::Type), &v2,
                                           &(Base::VectorPy::Type), &v3,
                                           &priority)) {
        Py::Vector xdir(v1, false);
        Py::Vector ydir(v2, false);
        Py::Vector zdir(v3, false);
        if (!priority)
            priority = "ZXY";
        try {
            *getRotationPtr() = (Rotation::makeRotationByAxes(xdir.toVector(), ydir.toVector(), zdir.toVector(), priority));
        } catch(Base::Exception &e) {
            std::string str;
            str += "FreeCAD exception thrown (";
            str += e.what();
            str += ")";
            PyErr_SetString(Base::BaseExceptionFreeCADError,str.c_str());
            return -1;
        }

        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Rotation constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Rotation object"
        "-- four floats (a quaternion)\n"
        "-- three floats (yaw, pitch, roll)"
        "-- Vector (rotation axis) and float (rotation angle)\n"
        "-- two Vectors (two axes)\n"
        "-- Matrix object\n"
        "-- 16 floats (4x4 matrix)\n"
        "-- 9 floats (3x3 matrix)\n"
        "-- 3 vectors + optional string"
       );
    return -1;
}

PyObject* RotationPy::richCompare(PyObject *v, PyObject *w, int op)
{
    if (PyObject_TypeCheck(v, &(RotationPy::Type)) &&
        PyObject_TypeCheck(w, &(RotationPy::Type))) {
        Base::Rotation r1 = *static_cast<RotationPy*>(v)->getRotationPtr();
        Base::Rotation r2 = *static_cast<RotationPy*>(w)->getRotationPtr();

        PyObject *res=0;
        if (op != Py_EQ && op != Py_NE) {
            PyErr_SetString(PyExc_TypeError,
            "no ordering relation is defined for Rotation");
            return 0;
        }
        else if (op == Py_EQ) {
            res = (r1 == r2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else {
            res = (r1 != r2) ? Py_True : Py_False;
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

PyObject* RotationPy::invert(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    this->getRotationPtr()->invert();
    Py_Return;
}

PyObject* RotationPy::inverted(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    Rotation mult = this->getRotationPtr()->inverse();
    return new RotationPy(new Rotation(mult));
}

PyObject* RotationPy::multiply(PyObject * args)
{
    PyObject *rot;
    if (!PyArg_ParseTuple(args, "O!", &(RotationPy::Type), &rot))
        return NULL;
    Rotation mult = (*getRotationPtr()) * (*static_cast<RotationPy*>(rot)->getRotationPtr());
    return new RotationPy(new Rotation(mult));
}

PyObject* RotationPy::multVec(PyObject * args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &obj))
        return NULL;
    Base::Vector3d vec(static_cast<VectorPy*>(obj)->value());
    getRotationPtr()->multVec(vec, vec);
    return new VectorPy(new Vector3d(vec));
}

PyObject* RotationPy::toEuler(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    double A,B,C;
    this->getRotationPtr()->getYawPitchRoll(A,B,C);

    Py::Tuple tuple(3);
    tuple.setItem(0, Py::Float(A));
    tuple.setItem(1, Py::Float(B));
    tuple.setItem(2, Py::Float(C));
    return Py::new_reference_to(tuple);
}

PyObject* RotationPy::isSame(PyObject *args)
{
    PyObject *rot;
    if (!PyArg_ParseTuple(args, "O!", &(RotationPy::Type), &rot))
        return NULL;
    Base::Rotation rot1 = * getRotationPtr();
    Base::Rotation rot2 = * static_cast<RotationPy*>(rot)->getRotationPtr();
    bool same = rot1.isSame(rot2);
    return Py_BuildValue("O", (same ? Py_True : Py_False));
}

PyObject* RotationPy::isIdentity(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    bool null = getRotationPtr()->isIdentity();
    return Py_BuildValue("O", (null ? Py_True : Py_False));
}

PyObject* RotationPy::isNull(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    bool null = getRotationPtr()->isNull();
    return Py_BuildValue("O", (null ? Py_True : Py_False));
}

Py::Tuple RotationPy::getQ(void) const
{
    double q0, q1, q2, q3;
    this->getRotationPtr()->getValue(q0,q1,q2,q3);

    Py::Tuple tuple(4);
    tuple.setItem(0, Py::Float(q0));
    tuple.setItem(1, Py::Float(q1));
    tuple.setItem(2, Py::Float(q2));
    tuple.setItem(3, Py::Float(q3));
    return tuple;
}

void RotationPy::setQ(Py::Tuple arg)
{
    double q0 = (double)Py::Float(arg.getItem(0));
    double q1 = (double)Py::Float(arg.getItem(1));
    double q2 = (double)Py::Float(arg.getItem(2));
    double q3 = (double)Py::Float(arg.getItem(3));
    this->getRotationPtr()->setValue(q0,q1,q2,q3);
}

Py::Object RotationPy::getAxis(void) const
{
    Base::Vector3d axis; double angle;
    this->getRotationPtr()->getValue(axis, angle);
    return Py::Vector(axis);
}

void RotationPy::setAxis(Py::Object arg)
{
    Base::Vector3d axis; double angle;
    this->getRotationPtr()->getValue(axis, angle);
    axis = Py::Vector(arg).toVector();
    this->getRotationPtr()->setValue(axis, angle);
}

Py::Float RotationPy::getAngle(void) const
{
    Base::Vector3d axis; double angle;
    this->getRotationPtr()->getValue(axis, angle);
    return Py::Float(angle);
}

void RotationPy::setAngle(Py::Float arg)
{
    Base::Vector3d axis; double angle;
    this->getRotationPtr()->getValue(axis, angle);
    angle = static_cast<double>(arg);
    this->getRotationPtr()->setValue(axis, angle);
}

PyObject *RotationPy::getCustomAttributes(const char* attr) const
{
    if (strcmp(attr, "Matrix") == 0) {
        Matrix4D mat;
        this->getRotationPtr()->getValue(mat);
        return new MatrixPy(mat);
    }
    return 0;
}

int RotationPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    if (strcmp(attr, "Matrix") == 0) {
        if (PyObject_TypeCheck(obj, &(MatrixPy::Type))) {
            this->getRotationPtr()->setValue(*static_cast<MatrixPy*>(obj)->getMatrixPtr());
            return 1;
        }
    }
    else if (strcmp(attr, "Axes") == 0) {
        if (PySequence_Check(obj) && PySequence_Size(obj) == 2) {
            PyObject* vec1 = PySequence_GetItem(obj, 0);
            PyObject* vec2 = PySequence_GetItem(obj, 1);
            if (PyObject_TypeCheck(vec1, &(VectorPy::Type)) &&
                PyObject_TypeCheck(vec2, &(VectorPy::Type))) {
                this->getRotationPtr()->setValue(
                    *static_cast<VectorPy*>(vec1)->getVectorPtr(),
                    *static_cast<VectorPy*>(vec2)->getVectorPtr());
                return 1;
            }
        }
    }
    return 0; 
}


