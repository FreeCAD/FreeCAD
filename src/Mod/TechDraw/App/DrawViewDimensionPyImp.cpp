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

#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

#include "DrawViewDimension.h"
// inclusion of the generated files (generated out of DrawViewDimensionPy.xml)
#include <Mod/TechDraw/App/DrawViewDimensionPy.h>
#include <Mod/TechDraw/App/DrawViewDimensionPy.cpp>


using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawViewDimensionPy::representation() const
{
    return std::string("<DrawViewDimension object>");
}

PyObject* DrawViewDimensionPy::getRawValue(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawViewDimension* dvd = getDrawViewDimensionPtr();
    double val = dvd->getDimValue();
    PyObject* pyVal = PyFloat_FromDouble(val);
    return pyVal;
}

PyObject* DrawViewDimensionPy::getText(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawViewDimension* dvd = getDrawViewDimensionPtr();
    std::string  textString = dvd->getFormattedDimensionValue();
//TODO: check multiversion code!
    PyObject* pyText = Base::PyAsUnicodeObject(textString);
    return pyText;
}

PyObject* DrawViewDimensionPy::getLinearPoints(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawViewDimension* dvd = getDrawViewDimensionPtr();
    pointPair pts = dvd->getLinearPoints();
    Py::List ret;
    ret.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(pts.first()))));
    ret.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(pts.second()))));
    return Py::new_reference_to(ret);
}

PyObject* DrawViewDimensionPy::getArcPoints(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawViewDimension* dvd = getDrawViewDimensionPtr();
    arcPoints pts = dvd->getArcPoints();
    Py::List ret;
    ret.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(pts.center))));
    ret.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(pts.onCurve.first()))));
    ret.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(pts.onCurve.second()))));
    ret.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(pts.arcEnds.first()))));
    ret.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(pts.arcEnds.second()))));
    ret.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(pts.midArc))));
    return Py::new_reference_to(ret);
}

PyObject* DrawViewDimensionPy::getAnglePoints(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawViewDimension* dvd = getDrawViewDimensionPtr();
    anglePoints pts = dvd->getAnglePoints();
    Py::List ret;
    ret.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(pts.first()))));
    ret.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(pts.second()))));
    ret.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(pts.vertex()))));
    return Py::new_reference_to(ret);
}

PyObject* DrawViewDimensionPy::getArrowPositions(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawViewDimension* dvd = getDrawViewDimensionPtr();
    pointPair pts = dvd->getArrowPositions();
    Py::List ret;
    ret.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(pts.first()))));
    ret.append(Py::asObject(new Base::VectorPy(new Base::Vector3d(pts.second()))));
    return Py::new_reference_to(ret);
}
PyObject *DrawViewDimensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int DrawViewDimensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
