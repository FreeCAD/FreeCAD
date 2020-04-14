/***************************************************************************
 *   Copyright (c) 2018 WandererFan <wandererfan@gmail.com>                *
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
#endif

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Base/Vector3D.h>

#include "DrawViewDimension.h"

// inclusion of the generated files (generated out of DrawViewDimensionPy.xml)
#include <Base/VectorPy.h>
#include <Mod/TechDraw/App/DrawViewDimensionPy.h>
#include <Mod/TechDraw/App/DrawViewDimensionPy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawViewDimensionPy::representation(void) const
{
    return std::string("<DrawViewDimension object>");
}
PyObject* DrawViewDimensionPy::getText(PyObject* args)
{
    (void) args;
//    PyObject* asShape = Py_False;
//    PyObject* pagePos = Py_False;
//    if (!PyArg_ParseTuple(args, "|OO", &asShape, &pagePos)) {
//        return 0;
//    }
    DrawViewDimension* dvd = getDrawViewDimensionPtr();
    std::string  textString = dvd->getFormatedValue();
//TODO: check multiversion code!
#if PY_MAJOR_VERSION >= 3
    PyObject* pyText = Base::PyAsUnicodeObject(textString);
#else
    PyObject *pyText = PyString_FromString(textString.c_str());
#endif
    return pyText;
}

PyObject* DrawViewDimensionPy::getLinearPoints(PyObject* args)
{
    (void) args;
    DrawViewDimension* dvd = getDrawViewDimensionPtr();
    pointPair pts = dvd->getLinearPoints();
    PyObject* ret = PyList_New(0);
    PyList_Append(ret,new Base::VectorPy(new Base::Vector3d(pts.first)));
    PyList_Append(ret,new Base::VectorPy(new Base::Vector3d(pts.second)));
    return ret;
}

PyObject* DrawViewDimensionPy::getArcPoints(PyObject* args)
{
    (void) args;
    DrawViewDimension* dvd = getDrawViewDimensionPtr();
    arcPoints pts = dvd->getArcPoints();
    PyObject* ret = PyList_New(0);
    PyList_Append(ret,new Base::VectorPy(new Base::Vector3d(pts.center)));
    PyList_Append(ret,new Base::VectorPy(new Base::Vector3d(pts.onCurve.first)));
    PyList_Append(ret,new Base::VectorPy(new Base::Vector3d(pts.onCurve.second)));
    PyList_Append(ret,new Base::VectorPy(new Base::Vector3d(pts.arcEnds.first)));
    PyList_Append(ret,new Base::VectorPy(new Base::Vector3d(pts.arcEnds.second)));
    PyList_Append(ret,new Base::VectorPy(new Base::Vector3d(pts.midArc)));
    return ret;
}

PyObject* DrawViewDimensionPy::getAnglePoints(PyObject* args)
{
    (void) args;
    DrawViewDimension* dvd = getDrawViewDimensionPtr();
    anglePoints pts = dvd->getAnglePoints();
    PyObject* ret = PyList_New(0);
    PyList_Append(ret,new Base::VectorPy(new Base::Vector3d(pts.ends.first)));
    PyList_Append(ret,new Base::VectorPy(new Base::Vector3d(pts.ends.second)));
    PyList_Append(ret,new Base::VectorPy(new Base::Vector3d(pts.vertex)));
    return ret;
}

PyObject* DrawViewDimensionPy::getArrowPositions(PyObject* args)
{
    (void) args;
    DrawViewDimension* dvd = getDrawViewDimensionPtr();
    pointPair pts = dvd->getArrowPositions();
    PyObject* ret = PyList_New(0);
    PyList_Append(ret,new Base::VectorPy(new Base::Vector3d(pts.first)));
    PyList_Append(ret,new Base::VectorPy(new Base::Vector3d(pts.second)));
    return ret;
}
PyObject *DrawViewDimensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawViewDimensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
