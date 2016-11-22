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
#if 0
PyObject* Geometry2dPy::mirror(PyObject *args)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", Base::Vector2dPy::type_object(),&o)) {
        Base::Vector2d vec = static_cast<Base::Vector2dPy*>(o)->getValue();
        gp_Pnt2d pnt(vec.x, vec.y);
        getGeometry2dPtr()->handle()->Mirror(pnt);
        Py_Return;
    }

    PyErr_Clear();
    PyObject* axis;
    if (PyArg_ParseTuple(args, "O!O!", Base::Vector2dPy::type_object(),&o,
                                       Base::Vector2dPy::type_object(),&axis)) {
        Base::Vector2d pnt = static_cast<Base::Vector2dPy*>(o)->getValue();
        Base::Vector2d dir = static_cast<Base::Vector2dPy*>(axis)->getValue();
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
    if (!PyArg_ParseTuple(args, "O!", &(Base::PlacementPy::Type),&o))
        return 0;

    Base::Placement* plm = static_cast<Base::PlacementPy*>(o)->getPlacementPtr();
    Base::Rotation rot(plm->getRotation());
    Base::Vector3d pnt, dir;
    double angle;

    rot.getValue(dir, angle);
    pnt = plm->getPosition();
    
    getGeometry2dPtr()->handle()->Rotate(gp_Pnt2d(pnt.x,pnt.y), angle);
    Py_Return;
}

PyObject* Geometry2dPy::scale(PyObject *args)
{
    PyObject* o;
    double scale;
    Base::Vector2d vec;
    if (PyArg_ParseTuple(args, "O!d", Base::Vector2dPy::type_object(),&o, &scale)) {
        vec = static_cast<Base::Vector2dPy*>(o)->getValue();
        gp_Pnt2d pnt(vec.x, vec.y);
        getGeometry2dPtr()->handle()->Scale(pnt, scale);
        Py_Return;
    }

    PyErr_SetString(PartExceptionOCCError, "either vector or tuple and float expected");
    return 0;
}

PyObject* Geometry2dPy::transform(PyObject *args)
{
    PyObject* o;
    if (!PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type),&o))
        return 0;
    Base::Matrix4D mat = static_cast<Base::MatrixPy*>(o)->value();
    gp_Trsf trf;
    trf.SetValues(mat[0][0],mat[0][1],mat[0][2],mat[0][3],
                  mat[1][0],mat[1][1],mat[1][2],mat[1][3],
                  mat[2][0],mat[2][1],mat[2][2],mat[2][3]
#if OCC_VERSION_HEX < 0x060800
                  , 0.00001,0.00001
#endif
                ); //precision was removed in OCCT CR0025194
    getGeometry2dPtr()->handle()->Transform(trf);
    Py_Return;
}

PyObject* Geometry2dPy::translate(PyObject *args)
{
    PyObject* o;
    Base::Vector2d vec;
    if (PyArg_ParseTuple(args, "O!", Base::Vector2dPy::type_object(),&o)) {
        vec = static_cast<Base::Vector2dPy*>(o)->getValue();
        gp_Vec2d trl(vec.x, vec.y);
        getGeometry2dPtr()->handle()->Translate(trl);
        Py_Return;
    }

    PyErr_SetString(PartExceptionOCCError, "either vector or tuple expected");
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
#endif
PyObject *Geometry2dPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int Geometry2dPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
