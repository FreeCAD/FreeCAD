/***************************************************************************
 *   Copyright (c) 2008 Juergen Riegel <juergen.riegel@web.de>             *
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
#include <boost/math/special_functions/fpclassify.hpp>
#endif

#include <Base/Builder3D.h>
#include <Base/Converter.h>
#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "Points.h"
// inclusion of the generated files (generated out of PointsPy.xml)
// clang-format off
#include "PointsPy.h"
#include "PointsPy.cpp"
// clang-format on


using namespace Points;

// returns a string which represents the object e.g. when printed in python
std::string PointsPy::representation() const
{
    return {"<PointKernel object>"};
}

PyObject* PointsPy::PyMake(struct _typeobject*, PyObject*, PyObject*)
{
    // create a new instance of PointsPy and the Twin object
    return new PointsPy(new PointKernel);
}

// constructor method
int PointsPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* pcObj = nullptr;
    if (!PyArg_ParseTuple(args, "|O", &pcObj)) {
        return -1;
    }

    // if no mesh is given
    if (!pcObj) {
        return 0;
    }
    if (PyObject_TypeCheck(pcObj, &(PointsPy::Type))) {
        *getPointKernelPtr() = *(static_cast<PointsPy*>(pcObj)->getPointKernelPtr());
    }
    else if (PyList_Check(pcObj)) {
        if (!addPoints(args)) {
            return -1;
        }
    }
    else if (PyTuple_Check(pcObj)) {
        if (!addPoints(args)) {
            return -1;
        }
    }
    else if (PyUnicode_Check(pcObj)) {
        getPointKernelPtr()->load(PyUnicode_AsUTF8(pcObj));
    }
    else {
        PyErr_SetString(PyExc_TypeError, "optional argument must be list, tuple or string");
        return -1;
    }

    return 0;
}

PyObject* PointsPy::copy(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    PointKernel* kernel = new PointKernel();
    // assign data
    *kernel = *getPointKernelPtr();
    return new PointsPy(kernel);
}

PyObject* PointsPy::read(PyObject* args)
{
    const char* Name;
    if (!PyArg_ParseTuple(args, "s", &Name)) {
        return nullptr;
    }

    PY_TRY
    {
        getPointKernelPtr()->load(Name);
    }
    PY_CATCH;

    Py_Return;
}

PyObject* PointsPy::write(PyObject* args)
{
    const char* Name;
    if (!PyArg_ParseTuple(args, "s", &Name)) {
        return nullptr;
    }

    PY_TRY
    {
        getPointKernelPtr()->save(Name);
    }
    PY_CATCH;

    Py_Return;
}

PyObject* PointsPy::writeInventor(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    std::stringstream result;
    Base::InventorBuilder builder(result);
    builder.beginSeparator();
    std::vector<Base::Vector3f> points;
    PointKernel* kernel = getPointKernelPtr();
    points.reserve(kernel->size());
    for (const auto& it : *kernel) {
        points.push_back(Base::convertTo<Base::Vector3f>(it));
    }
    builder.addNode(Base::Coordinate3Item {points});
    builder.addNode(Base::PointSetItem {});
    builder.endSeparator();

    return Py::new_reference_to(Py::String(result.str()));
}

PyObject* PointsPy::addPoints(PyObject* args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return nullptr;
    }

    try {
        Py::Sequence list(obj);
        Py::Type vType(Base::getTypeAsObject(&Base::VectorPy::Type));

        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            if ((*it).isType(vType)) {
                Py::Vector p(*it);
                getPointKernelPtr()->push_back(p.toVector());
            }
            else {
                Base::Vector3d pnt;
                Py::Tuple tuple(*it);
                pnt.x = (double)Py::Float(tuple[0]);
                pnt.y = (double)Py::Float(tuple[1]);
                pnt.z = (double)Py::Float(tuple[2]);
                getPointKernelPtr()->push_back(pnt);
            }
        }
    }
    catch (const Py::Exception&) {
        PyErr_SetString(PyExc_TypeError,
                        "either expect\n"
                        "-- [Vector,...] \n"
                        "-- [(x,y,z),...]");
        return nullptr;
    }

    Py_Return;
}

PyObject* PointsPy::fromSegment(PyObject* args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return nullptr;
    }

    try {
        const PointKernel* points = getPointKernelPtr();
        Py::Sequence list(obj);
        std::unique_ptr<PointKernel> pts(new PointKernel());
        pts->reserve(list.size());
        int numPoints = static_cast<int>(points->size());
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            long index = static_cast<long>(Py::Long(*it));
            if (index >= 0 && index < numPoints) {
                pts->push_back(points->getPoint(index));
            }
        }

        return new PointsPy(pts.release());
    }
    catch (const Py::Exception&) {
        PyErr_SetString(PyExc_TypeError, "expect a list of int");
        return nullptr;
    }
}

PyObject* PointsPy::fromValid(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    try {
        const PointKernel* points = getPointKernelPtr();
        std::unique_ptr<PointKernel> pts(new PointKernel());
        pts->reserve(points->size());
        for (const auto& point : *points) {
            if (!boost::math::isnan(point.x) && !boost::math::isnan(point.y)
                && !boost::math::isnan(point.z)) {
                pts->push_back(point);
            }
        }

        return new PointsPy(pts.release());
    }
    catch (const Py::Exception&) {
        PyErr_SetString(PyExc_TypeError, "expect a list of int");
        return nullptr;
    }
}

Py::Long PointsPy::getCountPoints() const
{
    return Py::Long((long)getPointKernelPtr()->size());
}

Py::List PointsPy::getPoints() const
{
    Py::List PointList;
    const PointKernel* points = getPointKernelPtr();
    for (const auto& point : *points) {
        PointList.append(Py::asObject(new Base::VectorPy(point)));
    }
    return PointList;
}

PyObject* PointsPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int PointsPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
