/***************************************************************************
 *   Copyright (c) 2016 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <gp_Dir2d.hxx>
# include <gp_Pnt2d.hxx>
# include <gp_Vec2d.hxx>
# include <gp_Trsf2d.hxx>
# include <gp_Trsf.hxx>
# include <Geom2d_Geometry.hxx>
# include <Geom2d_Curve.hxx>
# include <Precision.hxx>
# include <Standard_Failure.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/Matrix.h>
#include <Base/MatrixPy.h>
#include <Base/Tools2D.h>
#include <Base/Rotation.h>
#include <Base/Placement.h>
#include <Base/PlacementPy.h>

#include <Mod/Part/App/OCCError.h>
#include <Mod/Part/App/Geometry2d.h>
#include <Mod/Part/App/Geom2d/Geometry2dPy.h>
#include <Mod/Part/App/Geom2d/Geometry2dPy.cpp>

#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapePy.h>

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string Geometry2dPy::representation(void) const
{
    return "<Geometry2d object>";
}

PyObject *Geometry2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'Geometry'.");
    return 0;
}

// constructor method
int Geometry2dPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* Geometry2dPy::mirror(PyObject *args)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", Base::Vector2dPy::type_object(),&o)) {
        Base::Vector2d vec = Py::toVector2d(o);
        gp_Pnt2d pnt(vec.x, vec.y);
        getGeometry2dPtr()->handle()->Mirror(pnt);
        Py_Return;
    }

    PyErr_Clear();
    PyObject* axis;
    if (PyArg_ParseTuple(args, "O!O!", Base::Vector2dPy::type_object(),&o,
                                       Base::Vector2dPy::type_object(),&axis)) {
        Base::Vector2d pnt = Py::toVector2d(o);
        Base::Vector2d dir = Py::toVector2d(axis);
        gp_Ax2d ax1(gp_Pnt2d(pnt.x,pnt.y), gp_Dir2d(dir.x,dir.y));
        getGeometry2dPtr()->handle()->Mirror(ax1);
        Py_Return;
    }

    PyErr_SetString(PartExceptionOCCError, "either a point (vector) or axis (vector, vector) must be given");
    return 0;
}

PyObject* Geometry2dPy::rotate(PyObject *args)
{
    PyObject* o;
    double angle;
    Base::Vector2d vec;
    if (PyArg_ParseTuple(args, "O!d", Base::Vector2dPy::type_object(), &o, &angle)) {
        vec = Py::toVector2d(o);
        gp_Pnt2d pnt(vec.x, vec.y);
        getGeometry2dPtr()->handle()->Rotate(pnt, angle);
        Py_Return;
    }

    PyErr_SetString(PartExceptionOCCError, "Vector2d and float expected");
    return 0;
}

PyObject* Geometry2dPy::scale(PyObject *args)
{
    PyObject* o;
    double scale;
    Base::Vector2d vec;
    if (PyArg_ParseTuple(args, "O!d", Base::Vector2dPy::type_object(), &o, &scale)) {
        vec = Py::toVector2d(o);
        gp_Pnt2d pnt(vec.x, vec.y);
        getGeometry2dPtr()->handle()->Scale(pnt, scale);
        Py_Return;
    }

    PyErr_SetString(PartExceptionOCCError, "Vector2d and float expected");
    return 0;
}

PyObject* Geometry2dPy::transform(PyObject *args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O", &o))
        return 0;
    Py::Sequence list(o);
    double a11 = static_cast<double>(Py::Float(list.getItem(0)));
    double a12 = static_cast<double>(Py::Float(list.getItem(1)));
    double a13 = static_cast<double>(Py::Float(list.getItem(2)));
    double a21 = static_cast<double>(Py::Float(list.getItem(3)));
    double a22 = static_cast<double>(Py::Float(list.getItem(4)));
    double a23 = static_cast<double>(Py::Float(list.getItem(5)));

    gp_Trsf mat;
    mat.SetValues(a11, a12, 0, a13,
                  a21, a22, 0, a23,
                  0  ,   0, 1,   0
#if OCC_VERSION_HEX < 0x060800
                  , 0.00001,0.00001
#endif
                ); //precision was removed in OCCT CR0025194
    gp_Trsf2d trf(mat);

    getGeometry2dPtr()->handle()->Transform(trf);
    Py_Return;
}

PyObject* Geometry2dPy::translate(PyObject *args)
{
    PyObject* o;
    Base::Vector2d vec;
    if (PyArg_ParseTuple(args, "O!", Base::Vector2dPy::type_object(),&o)) {
        vec = Py::toVector2d(o);
        gp_Vec2d trl(vec.x, vec.y);
        getGeometry2dPtr()->handle()->Translate(trl);
        Py_Return;
    }

    PyErr_SetString(PartExceptionOCCError, "Vector2d expected");
    return 0;
}

PyObject* Geometry2dPy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    Part::Geometry2d* geom = this->getGeometry2dPtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = 0;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, 0);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of geometry");
        return 0;
    }

    Part::Geometry2dPy* geompy = static_cast<Part::Geometry2dPy*>(cpy);
    // the PyMake function must have created the corresponding instance of the 'Geometry' subclass
    // so delete it now to avoid a memory leak
    if (geompy->_pcTwinPointer) {
        Part::Geometry2d* clone = static_cast<Part::Geometry2d*>(geompy->_pcTwinPointer);
        delete clone;
    }
    geompy->_pcTwinPointer = geom->clone();
    return cpy;
}

PyObject *Geometry2dPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int Geometry2dPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
