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
# include <sstream>
#endif

#include "Mesh.h"
#include "MeshPoint.h"
#include "MeshPointPy.h"
#include "MeshPointPy.cpp"

#include <Base/VectorPy.h>

using namespace Mesh;

// returns a string which represents the object e.g. when printed in python
std::string MeshPointPy::representation(void) const
{
    MeshPointPy::PointerType ptr = getMeshPointPtr();
    Base::Vector3d vec = *ptr;

    std::stringstream str;
    str << "MeshPoint (";
    if (ptr->isBound()) {
        if (getMeshPointPtr()->Mesh->countPoints() <= getMeshPointPtr()->Index) {
            str << vec.x << ", " << vec.y << ", " << vec.z << ", Idx=" << ptr->Index << " (Out of range)";
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

PyObject *MeshPointPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of MeshPointPy and the Twin object
    return new MeshPointPy(new MeshPoint);
}

// constructor method
int MeshPointPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    double  x=0.0,y=0.0,z=0.0;
    if (!PyArg_ParseTuple(args, "|ddd", &x,&y,&z))
        return -1;

    getMeshPointPtr()->Set(x,y,z);
    return 0;
}

PyObject*  MeshPointPy::unbound(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    getMeshPointPtr()->Index = UINT_MAX;
    getMeshPointPtr()->Mesh = 0;
    Py_Return;
}

PyObject*  MeshPointPy::move(PyObject *args)
{
    if (!getMeshPointPtr()->isBound()) {
        PyErr_SetString(PyExc_RuntimeError, "This object is not bounded to a mesh, so no topological operation is possible!");
        return 0;
    }
    if (getMeshPointPtr()->Mesh->countPoints() <= getMeshPointPtr()->Index) {
        PyErr_SetString(PyExc_IndexError, "Index out of range");
        return 0;
    }

    double  x=0.0,y=0.0,z=0.0;
    PyObject *object;
    Base::Vector3d vec;

    do {
        if (PyArg_ParseTuple(args, "ddd", &x,&y,&z)) {
            vec.Set(x,y,z);
            break;
        }

        PyErr_Clear(); // set by PyArg_ParseTuple()
        if (PyArg_ParseTuple(args,"O!",&(Base::VectorPy::Type), &object)) {
            vec = *(static_cast<Base::VectorPy*>(object)->getVectorPtr());
            break;
        }

        PyErr_SetString(PyExc_TypeError, "Tuple of three floats or Vector expected");
        return 0;
    }
    while (false);

    getMeshPointPtr()->Mesh->movePoint(getMeshPointPtr()->Index,vec);
    Py_Return;
}

Py::Long MeshPointPy::getIndex(void) const
{
    return Py::Long((long) getMeshPointPtr()->Index);
}

Py::Boolean MeshPointPy::getBound(void) const
{
    return Py::Boolean(getMeshPointPtr()->Index != UINT_MAX);
}

Py::Object MeshPointPy::getNormal(void) const
{
    if (!getMeshPointPtr()->isBound())
        throw Py::RuntimeError("This object is not bound to a mesh, so no topological operation is possible!");
    if (getMeshPointPtr()->Mesh->countPoints() <= getMeshPointPtr()->Index)
        throw Py::IndexError("Index out of range");

    Base::Vector3d* v = new Base::Vector3d(getMeshPointPtr()->Mesh->getPointNormal(getMeshPointPtr()->Index));
    Base::VectorPy* normal = new Base::VectorPy(v);
    normal->setConst();
    return Py::Object(normal,true);
}

Py::Object MeshPointPy::getVector(void) const
{
    MeshPointPy::PointerType ptr = static_cast<MeshPointPy::PointerType>(_pcTwinPointer);

    Base::VectorPy* vec = new Base::VectorPy(*ptr);
    vec->setConst();
    return Py::Object(vec,true);
}

Py::Float MeshPointPy::getx(void) const
{
    MeshPointPy::PointerType ptr = static_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    double x = ptr->x;

    if (getMeshPointPtr()->isBound()) {
        if (getMeshPointPtr()->Mesh->countPoints() > getMeshPointPtr()->Index)
            x = getMeshPointPtr()->Mesh->getPoint(getMeshPointPtr()->Index).x;
    }

    return Py::Float(x);
}

void  MeshPointPy::setx(Py::Float arg)
{
    MeshPointPy::PointerType ptr = static_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    ptr->x = (double)arg;

    if (getMeshPointPtr()->isBound()) {
        if (getMeshPointPtr()->Mesh->countPoints() > getMeshPointPtr()->Index)
            getMeshPointPtr()->Mesh->setPoint(getMeshPointPtr()->Index,*ptr);
    }
}

Py::Float MeshPointPy::gety(void) const
{
    MeshPointPy::PointerType ptr = static_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    double y = ptr->y;

    if (getMeshPointPtr()->isBound()) {
        if (getMeshPointPtr()->Mesh->countPoints() > getMeshPointPtr()->Index)
            y = getMeshPointPtr()->Mesh->getPoint(getMeshPointPtr()->Index).y;
    }

    return Py::Float(y);
}

void  MeshPointPy::sety(Py::Float arg)
{
    MeshPointPy::PointerType ptr = static_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    ptr->y = (double)arg;

    if (getMeshPointPtr()->isBound()) {
        if (getMeshPointPtr()->Mesh->countPoints() > getMeshPointPtr()->Index)
            getMeshPointPtr()->Mesh->setPoint(getMeshPointPtr()->Index,*ptr);
    }
}

Py::Float MeshPointPy::getz(void) const
{
    MeshPointPy::PointerType ptr = static_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    double z = ptr->z;

    if (getMeshPointPtr()->isBound()) {
        if (getMeshPointPtr()->Mesh->countPoints() > getMeshPointPtr()->Index)
            z = getMeshPointPtr()->Mesh->getPoint(getMeshPointPtr()->Index).z;
    }

    return Py::Float(z);
}

void  MeshPointPy::setz(Py::Float arg)
{
    MeshPointPy::PointerType ptr = static_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    ptr->z = (double)arg;

    if (getMeshPointPtr()->isBound()) {
        if (getMeshPointPtr()->Mesh->countPoints() > getMeshPointPtr()->Index)
            getMeshPointPtr()->Mesh->setPoint(getMeshPointPtr()->Index,*ptr);
    }
}

PyObject *MeshPointPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int MeshPointPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
