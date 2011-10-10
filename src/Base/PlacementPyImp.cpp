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

#include "Placement.h"
#include "GeometryPyCXX.h"

// inclusion of the generated files (generated out of PlacementPy.xml)
#include "PlacementPy.h"
#include "PlacementPy.cpp"
#include "Matrix.h"
#include "MatrixPy.h"
#include "RotationPy.h"
#include "VectorPy.h"

using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string PlacementPy::representation(void) const
{
    double A,B,C;
    PlacementPy::PointerType ptr = reinterpret_cast<PlacementPy::PointerType>(_pcTwinPointer);
    std::stringstream str;
    ptr->getRotation().getYawPitchRoll(A,B,C);

    str << "Placement [Pos=(";
    str << ptr->getPosition().x << ","<< ptr->getPosition().y << "," << ptr->getPosition().z;
    str << "), Yaw-Pitch-Roll=(" << A << "," << B << "," << C << ")]";

    return str.str();
}

PyObject *PlacementPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of PlacementPy and the Twin object 
    return new PlacementPy(new Placement);
}

// constructor method
int PlacementPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type), &o)) {
        Base::Matrix4D mat = static_cast<Base::MatrixPy*>(o)->value();
        getPlacementPtr()->fromMatrix(mat);
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(Base::PlacementPy::Type), &o)) {
        Base::Placement *plm = static_cast<Base::PlacementPy*>(o)->getPlacementPtr();
        *(getPlacementPtr()) = *plm;
        return 0;
    }

    PyErr_Clear();
    PyObject* d;
    double angle;
    if (PyArg_ParseTuple(args, "O!O!d", &(Base::VectorPy::Type), &o,
                                        &(Base::VectorPy::Type), &d, &angle)) {
        // NOTE: The first parameter defines the translation, the second the rotation axis
        // and the last parameter defines the rotation angle in degree.
        Base::Rotation rot(static_cast<Base::VectorPy*>(d)->value(), angle/180.0*D_PI);
        *getPlacementPtr() = Base::Placement(static_cast<Base::VectorPy*>(o)->value(),rot);
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!O!", &(Base::VectorPy::Type), &o,
                                       &(Base::RotationPy::Type), &d)) {
        Base::Vector3d *pos = static_cast<Base::VectorPy*>(o)->getVectorPtr();
        getPlacementPtr()->setPosition(*pos);
        Base::Rotation *rot = static_cast<Base::RotationPy*>(d)->getRotationPtr();
        getPlacementPtr()->setRotation(*rot);
        return 0;
    }

    PyErr_Clear();
    PyObject* c;
    if (PyArg_ParseTuple(args, "O!O!O!", &(Base::VectorPy::Type), &o,
                                         &(Base::RotationPy::Type), &d,
                                         &(Base::VectorPy::Type), &c)) {
        Base::Vector3d *pos = static_cast<Base::VectorPy*>(o)->getVectorPtr();
        Base::Rotation *rot = static_cast<Base::RotationPy*>(d)->getRotationPtr();
        Base::Vector3d *cnt = static_cast<Base::VectorPy*>(c)->getVectorPtr();
        Base::Placement p(*pos,*rot,*cnt);
        getPlacementPtr()->operator = (p);
        return 0;
    }

    PyErr_SetString(PyExc_Exception, "empty parameter list, matrix or placement expected");
    return -1;
}

PyObject* PlacementPy::move(PyObject * args)
{
    PyObject *vec;
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &vec))
        return NULL;
    getPlacementPtr()->move(static_cast<VectorPy*>(vec)->value());
    Py_Return;
}

PyObject* PlacementPy::multiply(PyObject * args)
{
    PyObject *plm;
    if (!PyArg_ParseTuple(args, "O!", &(PlacementPy::Type), &plm))
        return NULL;
    Placement mult = (*getPlacementPtr()) * (*static_cast<PlacementPy*>(plm)->getPlacementPtr());
    return new PlacementPy(new Placement(mult));
}

PyObject* PlacementPy::multVec(PyObject * args)
{
    PyObject *vec;
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &vec))
        return NULL;
    Base::Vector3d pnt(static_cast<VectorPy*>(vec)->value());
    getPlacementPtr()->multVec(pnt, pnt);
    return new VectorPy(new Vector3d(pnt));
}

PyObject* PlacementPy::copy(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    return new PlacementPy(new Placement(*getPlacementPtr()));
}

PyObject* PlacementPy::toMatrix(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Base::Matrix4D mat = getPlacementPtr()->toMatrix();
    return new MatrixPy(new Matrix4D(mat));
}

PyObject* PlacementPy::inverse(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    Base::Placement p = getPlacementPtr()->inverse();
    return new PlacementPy(new Placement(p));
}

Py::Object PlacementPy::getBase(void) const
{
    return Py::Vector(getPlacementPtr()->getPosition());
}

void PlacementPy::setBase(Py::Object arg)
{
    getPlacementPtr()->setPosition(Py::Vector(arg).toVector());
}

Py::Object PlacementPy::getRotation(void) const
{
    return Py::Rotation(getPlacementPtr()->getRotation());
}

void PlacementPy::setRotation(Py::Object arg)
{
    Py::Rotation rot;
    if (rot.accepts(arg.ptr())) {
        getPlacementPtr()->setRotation((Base::Rotation)Py::Rotation(arg));
        return;
    }
    Py::Tuple tuple;
    if (tuple.accepts(arg.ptr())) {
        tuple = arg;
        getPlacementPtr()->setRotation(Base::Rotation((double)Py::Float(tuple[0]),
                                                      (double)Py::Float(tuple[1]),
                                                      (double)Py::Float(tuple[2]),
                                                      (double)Py::Float(tuple[3])
                                                     ));
        return;
    }

    throw Py::TypeError("either Rotation or tuple of four floats expected");
}

PyObject *PlacementPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int PlacementPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
