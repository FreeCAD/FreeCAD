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
//# include <gp_Ax1.hxx>
//# include <gp_Dir.hxx>
//# include <gp_Pnt.hxx>
//# include <gp_Vec.hxx>
//# include <gp_Trsf.hxx>
//# include <Geom_GeometryFormat.hxx>
//# include <Geom_Curve.hxx>
//# include <Geom_Surface.hxx>
//# include <Precision.hxx>
//# include <Standard_Failure.hxx>

# include <boost/uuid/uuid_io.hpp>
#endif

//#include <Base/GeomFormatPyCXX.h>
//#include <Base/Matrix.h>
//#include <Base/MatrixPy.h>
//#include <Base/Vector3D.h>
//#include <Base/VectorPy.h>
//#include <Base/Rotation.h>
//#include <Base/Placement.h>
//#include <Base/PlacementPy.h>

//#include "OCCError.h"
#include "Cosmetic.h"
#include "GeomFormatPy.h"
#include "GeomFormatPy.cpp"

//#include "TopoShape.h"
//#include "TopoShapePy.h"

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string GeomFormatPy::representation(void) const
{
    return "<GeomFormat object>";
}

PyObject *GeomFormatPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'GeomFormat'.");
    return 0;
}

// constructor method
int GeomFormatPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* GeomFormatPy::clone(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    TechDraw::GeomFormat* geom = this->getGeomFormatPtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = 0;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, 0);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create clone of GeomFormat");
        return 0;
    }

    TechDraw::GeomFormatPy* geompy = static_cast<TechDraw::GeomFormatPy*>(cpy);
    // the PyMake function must have created the corresponding instance of the 'GeomFormat' subclass
    // so delete it now to avoid a memory leak
    if (geompy->_pcTwinPointer) {
        TechDraw::GeomFormat* clone = static_cast<TechDraw::GeomFormat*>(geompy->_pcTwinPointer);
        delete clone;
    }
    geompy->_pcTwinPointer = geom->clone();
    return cpy;
}

PyObject* GeomFormatPy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    TechDraw::GeomFormat* geom = this->getGeomFormatPtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = 0;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, 0);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of GeomFormat");
        return 0;
    }

    TechDraw::GeomFormatPy* geompy = static_cast<TechDraw::GeomFormatPy*>(cpy);
    // the PyMake function must have created the corresponding instance of the 'GeomFormat' subclass
    // so delete it now to avoid a memory leak
    if (geompy->_pcTwinPointer) {
        TechDraw::GeomFormat* copy = static_cast<TechDraw::GeomFormat*>(geompy->_pcTwinPointer);
        delete copy;
    }
    geompy->_pcTwinPointer = geom->copy();
    return cpy;
}

PyObject *GeomFormatPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int GeomFormatPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
