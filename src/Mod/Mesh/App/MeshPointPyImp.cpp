/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <sstream>
#endif

#include <Base/VectorPy.h>

#include "Mesh.h"
#include "MeshPoint.h"
// clang-format off
#include "MeshPointPy.h"
#include "MeshPointPy.cpp"
// clang-format on


using namespace Mesh;

// returns a string which represents the object e.g. when printed in python
std::string MeshPointPy::representation() const
{
    MeshPointPy::PointerType ptr = getMeshPointPtr();
    Base::Vector3d vec = *ptr;  // NOLINT

    std::stringstream str;
    str << "MeshPoint (";
    if (ptr->isBound()) {
        if (getMeshPointPtr()->Mesh->countPoints() <= getMeshPointPtr()->Index) {
            str << vec.x << ", " << vec.y << ", " << vec.z << ", Idx=" << ptr->Index
                << " (Out of range)";
        }
        else {
            vec = getMeshPointPtr()->Mesh->getPoint(getMeshPointPtr()->Index);
            str << vec.x << ", " << vec.y << ", " << vec.z << ", Idx=" << ptr->Index;
        }
    }
    else {
        str << vec.x << ", " << vec.y << ", " << vec.z;
    }

    str << ")";
    return str.str();
}

PyObject* MeshPointPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of MeshPointPy and the Twin object
    return new MeshPointPy(new MeshPoint);
}

// constructor method
int MeshPointPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    double x = 0.0, y = 0.0, z = 0.0;
    if (!PyArg_ParseTuple(args, "|ddd", &x, &y, &z)) {
        return -1;
    }

    getMeshPointPtr()->Set(x, y, z);
    return 0;
}

PyObject* MeshPointPy::unbound(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }
    getMeshPointPtr()->Index = UINT_MAX;
    getMeshPointPtr()->Mesh = nullptr;
    Py_Return;
}

Py::Long MeshPointPy::getIndex() const
{
    return Py::Long((long)getMeshPointPtr()->Index);
}

Py::Boolean MeshPointPy::getBound() const
{
    return {getMeshPointPtr()->Index != UINT_MAX};
}

Py::Object MeshPointPy::getNormal() const
{
    if (!getMeshPointPtr()->isBound()) {
        throw Py::RuntimeError(
            "This object is not bound to a mesh, so no topological operation is possible!");
    }
    if (getMeshPointPtr()->Mesh->countPoints() <= getMeshPointPtr()->Index) {
        throw Py::IndexError("Index out of range");
    }

    Base::Vector3d* v =
        new Base::Vector3d(getMeshPointPtr()->Mesh->getPointNormal(getMeshPointPtr()->Index));
    Base::VectorPy* normal = new Base::VectorPy(v);
    normal->setConst();
    return Py::Object(normal, true);
}

Py::Object MeshPointPy::getVector() const
{
    MeshPointPy::PointerType ptr = static_cast<MeshPointPy::PointerType>(_pcTwinPointer);

    Base::VectorPy* vec = new Base::VectorPy(*ptr);
    vec->setConst();
    return Py::Object(vec, true);
}

Py::Float MeshPointPy::getx() const
{
    MeshPointPy::PointerType ptr = static_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    double x = ptr->x;

    if (getMeshPointPtr()->isBound()) {
        if (getMeshPointPtr()->Mesh->countPoints() > getMeshPointPtr()->Index) {
            x = getMeshPointPtr()->Mesh->getPoint(getMeshPointPtr()->Index).x;
        }
    }

    return Py::Float(x);
}

Py::Float MeshPointPy::gety() const
{
    MeshPointPy::PointerType ptr = static_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    double y = ptr->y;

    if (getMeshPointPtr()->isBound()) {
        if (getMeshPointPtr()->Mesh->countPoints() > getMeshPointPtr()->Index) {
            y = getMeshPointPtr()->Mesh->getPoint(getMeshPointPtr()->Index).y;
        }
    }

    return Py::Float(y);
}

Py::Float MeshPointPy::getz() const
{
    MeshPointPy::PointerType ptr = static_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    double z = ptr->z;

    if (getMeshPointPtr()->isBound()) {
        if (getMeshPointPtr()->Mesh->countPoints() > getMeshPointPtr()->Index) {
            z = getMeshPointPtr()->Mesh->getPoint(getMeshPointPtr()->Index).z;
        }
    }

    return Py::Float(z);
}

PyObject* MeshPointPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MeshPointPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
