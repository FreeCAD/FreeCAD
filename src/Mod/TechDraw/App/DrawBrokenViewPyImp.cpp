/***************************************************************************
 *   Copyright (c) 2024 WandererFan <wandererfan@gmail.com>                *
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

#include <Base/Console.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

#include "DrawBrokenView.h"
#include "DrawViewPart.h"
// inclusion of the generated files
#include <Mod/TechDraw/App/DrawViewPartPy.h>
#include <Mod/TechDraw/App/DrawBrokenViewPy.h>
#include <Mod/TechDraw/App/DrawBrokenViewPy.cpp>


using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawBrokenViewPy::representation() const
{
    return std::string("<DrawBrokenView object>");
}

PyObject* DrawBrokenViewPy::mapPoint3dToView(PyObject *args)
{
    PyObject* pPoint3d = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &pPoint3d)) {
        return nullptr;
    }

    DrawBrokenView* dvp = getDrawBrokenViewPtr();
    Base::Vector3d point3d = static_cast<Base::VectorPy*>(pPoint3d)->value();
    Base::Vector3d point2d = dvp->mapPoint3dToView(point3d);

    return new Base::VectorPy(new Base::Vector3d(point2d));
}

PyObject* DrawBrokenViewPy::mapPoint2dFromView(PyObject *args)
{
    PyObject* pPoint2d = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &pPoint2d)) {
        return nullptr;
    }

    DrawBrokenView* dvp = getDrawBrokenViewPtr();
    Base::Vector3d pointIn = static_cast<Base::VectorPy*>(pPoint2d)->value();
    Base::Vector3d pointOut = dvp->mapPoint2dFromView(pointIn);

    return new Base::VectorPy(new Base::Vector3d(pointOut));
}

PyObject* DrawBrokenViewPy::getCompressedCenter(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawBrokenView* dvp = getDrawBrokenViewPtr();
    Base::Vector3d pointOut = dvp->getCompressedCentroid();
    return new Base::VectorPy(new Base::Vector3d(pointOut));
}


PyObject *DrawBrokenViewPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int DrawBrokenViewPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
