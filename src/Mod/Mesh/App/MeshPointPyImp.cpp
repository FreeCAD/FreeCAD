/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2008     *
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

// returns a string which represent the object e.g. when printed in python
std::string MeshPointPy::representation(void) const
{
    MeshPointPy::PointerType ptr = getMeshPointPtr();
    std::stringstream str;
    str << "MeshPoint (";
    if(ptr->isBound())
        str << ptr->x << ", "<< ptr->y << ", "<< ptr->z << ", Idx=" << ptr->Index;
    else
        str << ptr->x << ", "<< ptr->y << ", "<< ptr->z ;
    str << ")";
 
    return str.str();
}

PyObject *MeshPointPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of MeshPointPy and the Twin object 
    return new MeshPointPy(new MeshPoint);
}

// constructor method
int MeshPointPy::PyInit(PyObject* args, PyObject*k)
{
    double  x=0.0,y=0.0,z=0.0;
    if (!PyArg_ParseTuple(args, "|ddd", &x,&y,&z))
        return -1;

    getMeshPointPtr()->Set(x,y,z);
    return 0;
}

PyObject*  MeshPointPy::unbound(PyObject *args)
{
    getMeshPointPtr()->Index = UINT_MAX;
    getMeshPointPtr()->Mesh = 0;
    Py_Return;
}

PyObject*  MeshPointPy::move(PyObject *args)
{
    if (!getMeshPointPtr()->isBound())
        PyErr_SetString(PyExc_Exception, "This object is not bounded to a mesh, so no topological operation is possible!");

    double  x=0.0,y=0.0,z=0.0;
    PyObject *object;
    Base::Vector3d vec;
    if (PyArg_ParseTuple(args, "ddd", &x,&y,&z)) {
        vec.Set(x,y,z);
    } 
    else if (PyArg_ParseTuple(args,"O!",&(Base::VectorPy::Type), &object)) {
        PyErr_Clear(); // set by PyArg_ParseTuple()
        // Note: must be static_cast, not reinterpret_cast
        vec = *(static_cast<Base::VectorPy*>(object)->getVectorPtr());
    }
    else {
        return 0;
    }

    getMeshPointPtr()->Mesh->movePoint(getMeshPointPtr()->Index,vec);
    Py_Return;
}

Py::Int MeshPointPy::getIndex(void) const
{
    return Py::Int((long) getMeshPointPtr()->Index);
}

Py::Boolean MeshPointPy::getBound(void) const
{
    return Py::Boolean(getMeshPointPtr()->Index != UINT_MAX);
}

Py::Object MeshPointPy::getNormal(void) const
{
    if (!getMeshPointPtr()->isBound())
        PyErr_SetString(PyExc_Exception, "This object is not bounded to a mesh, so no topological operation is possible!");

    Base::Vector3d* v = new Base::Vector3d(getMeshPointPtr()->Mesh->getPointNormal(getMeshPointPtr()->Index));
    Base::VectorPy* normal = new Base::VectorPy(v);
    normal->setConst();
    return Py::Object(normal,true);
}

Py::Object MeshPointPy::getVector(void) const
{
    MeshPointPy::PointerType ptr = reinterpret_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    
    Base::VectorPy* vec = new Base::VectorPy(*ptr);
    vec->setConst();
    return Py::Object(vec,true);
}

Py::Float MeshPointPy::getx(void) const
{
    MeshPointPy::PointerType ptr = reinterpret_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    return Py::Float(ptr->x);
}

void  MeshPointPy::setx(Py::Float arg)
{
    MeshPointPy::PointerType ptr = reinterpret_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    ptr->x = (double)arg;

    if (getMeshPointPtr()->isBound()) {
        getMeshPointPtr()->Mesh->movePoint(getMeshPointPtr()->Index,*ptr);
    }
}

Py::Float MeshPointPy::gety(void) const
{
    MeshPointPy::PointerType ptr = reinterpret_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    return Py::Float(ptr->y);
}

void  MeshPointPy::sety(Py::Float arg)
{
    MeshPointPy::PointerType ptr = reinterpret_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    ptr->y = (double)arg;

    if (getMeshPointPtr()->isBound()) {
        getMeshPointPtr()->Mesh->movePoint(getMeshPointPtr()->Index,*ptr);
    }
}

Py::Float MeshPointPy::getz(void) const
{
    MeshPointPy::PointerType ptr = reinterpret_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    return Py::Float(ptr->z);
}

void  MeshPointPy::setz(Py::Float arg)
{
    MeshPointPy::PointerType ptr = reinterpret_cast<MeshPointPy::PointerType>(_pcTwinPointer);
    ptr->z = (double)arg;

    if (getMeshPointPtr()->isBound()) {
        getMeshPointPtr()->Mesh->movePoint(getMeshPointPtr()->Index,*ptr);
    }
}

PyObject *MeshPointPy::getCustomAttributes(const char* attr) const
{
    return 0;
}

int MeshPointPy::setCustomAttributes(const char* attr, PyObject *obj)
{
    return 0; 
}
