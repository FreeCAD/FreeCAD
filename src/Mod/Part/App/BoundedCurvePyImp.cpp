/***************************************************************************
 *   Copyright (c) 2019 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com      *
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

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "Geometry.h"
#include "BoundedCurvePy.h"
#include "BoundedCurvePy.cpp"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string BoundedCurvePy::representation(void) const
{
    return "<Curve object>";
}

PyObject *BoundedCurvePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
                    "You cannot create an instance of the abstract class 'BoundedCurve'.");
    return 0;
}

// constructor method
int BoundedCurvePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

Py::Object BoundedCurvePy::getStartPoint(void) const
{
    return Py::Vector(getGeomBoundedCurvePtr()->getStartPoint());
}

Py::Object BoundedCurvePy::getEndPoint(void) const
{
    return Py::Vector(getGeomBoundedCurvePtr()->getEndPoint());
}

PyObject *BoundedCurvePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int BoundedCurvePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
