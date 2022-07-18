///***************************************************************************
// *   Copyright (c) 2022 Matteo Grellier <matteogrellier@gmail.com>         *
// *                                                                         *
// *   This file is part of the FreeCAD CAx development system.              *
// *                                                                         *
// *   This library is free software; you can redistribute it and/or         *
// *   modify it under the terms of the GNU Library General Public           *
// *   License as published by the Free Software Foundation; either          *
// *   version 2 of the License, or (at your option) any later version.      *
// *                                                                         *
// *   This library  is distributed in the hope that it will be useful,      *
// *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
// *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
// *   GNU Library General Public License for more details.                  *
// *                                                                         *
// *   You should have received a copy of the GNU Library General Public     *
// *   License along with this library; see the file COPYING.LIB. If not,    *
// *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
// *   Suite 330, Boston, MA  02111-1307, USA                                *
// *                                                                         *
// ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <TColStd_Array1OfReal.hxx>
#include <TColgp_Array1OfPnt.hxx>
#endif
#include "Blending/BlendCurvePy.h"
#include "Blending/BlendPointPy.h"
// #include "Mod/Part/App/Geometry.h"
// #include <Base/GeometryPyCXX.h>
#include "Blending/BlendCurvePy.cpp"
#include <Base/VectorPy.h>
#include <Mod/Part/App/BezierCurvePy.h>

using namespace Surface;

std::string BlendCurvePy::representation(void) const
{
    return "BlendCurve";
}

PyObject *BlendCurvePy::PyMake(struct _typeobject *, PyObject *, PyObject *)// Python wrapper
{
    // create a new instance of BlendCurvePy
    return new BlendCurvePy(new BlendCurve);
}

int BlendCurvePy::PyInit(PyObject *args, PyObject * /*kwds*/)
{
    PyObject *b1;
    PyObject *b2;
    std::vector<BlendPoint> bpList;

    if (PyArg_ParseTuple(args, "O!O!", &(Surface::BlendPointPy::Type), &b1, &(Surface::BlendPointPy::Type), &b2)) {
        BlendPoint *geom1 = static_cast<BlendPointPy *>(b1)->getBlendPointPtr();
        BlendPoint *geom2 = static_cast<BlendPointPy *>(b2)->getBlendPointPtr();
        bpList.emplace_back(*geom1);
        bpList.emplace_back(*geom2);
        this->getBlendCurvePtr()->blendPoints = bpList;
        return 0;
    }
    return -1;
}
PyObject *BlendCurvePy::compute(PyObject * /*args*/)
{
    BlendCurve *bc = getBlendCurvePtr();
    Handle(Geom_BezierCurve) gc = bc->compute();
    return new Part::BezierCurvePy(new Part::GeomBezierCurve(gc));
}


PyObject *BlendCurvePy::setSize(PyObject *args)
{
    int i;
    double size;
    PyObject *relative = Py_True;
    if (!PyArg_ParseTuple(args, "idO!", &i, &size, &PyBool_Type, &relative)) {
        return nullptr;
    }
    try {
        getBlendCurvePtr()->setSize(i, size, Base::asBoolean(relative));
    }
    catch (Standard_Failure &e) {
        PyErr_SetString(PyExc_Exception, e.GetMessageString());
        return nullptr;
    }
    Py_Return;
}

PyObject *BlendCurvePy::getCustomAttributes(const char * /*attr*/) const
{
    return nullptr;
}

int BlendCurvePy::setCustomAttributes(const char * /*attr*/, PyObject * /*obj*/)
{
    return 0;
}
