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

#include "Base/BoundBox.h"

// inclusion of the generated files (generated out of BoundBoxPy.xml)
#include "VectorPy.h"
#include "GeometryPyCXX.h"
#include "BoundBoxPy.h"
#include "BoundBoxPy.cpp"

using namespace Base;

// returns a string which represent the object e.g. when printed in python
std::string BoundBoxPy::representation(void) const
{
    std::stringstream str;
    str << "BoundBox (";
    str << getBoundBoxPtr()->MinX << ", "
        << getBoundBoxPtr()->MinY << ", "
        << getBoundBoxPtr()->MinZ << ", "
        << getBoundBoxPtr()->MaxX << ", "
        << getBoundBoxPtr()->MaxY << ", "
        << getBoundBoxPtr()->MaxZ ;
    str << ")";

    return str.str();
}

PyObject *BoundBoxPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of BoundBoxPy and the Twin object 
    return new BoundBoxPy(new BoundBox3d);
}

// constructor method
int BoundBoxPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    double xMin=0.0,yMin=0.0,zMin=0.0,xMax=0.0,yMax=0.0,zMax=0.0;
    PyObject *object1, *object2;
    BoundBoxPy::PointerType ptr = getBoundBoxPtr();
    if (PyArg_ParseTuple(args, "|dddddd", &xMin, &yMin, &zMin, &xMax, &yMax, &zMax)) {
        ptr->MaxX = xMax;
        ptr->MaxY = yMax;
        ptr->MaxZ = zMax;
        ptr->MinX = xMin;
        ptr->MinY = yMin;
        ptr->MinZ = zMin;
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()
    if (PyArg_ParseTuple(args,"O!O!",&PyTuple_Type, &object1,
                                     &PyTuple_Type, &object2)) {
        try {
            Vector3d v1 = getVectorFromTuple<double>(object1);
            Vector3d v2 = getVectorFromTuple<double>(object2);
            ptr->Add(v1);
            ptr->Add(v2);
            return 0;
        }
        catch (const Py::Exception&) {
            return -1;
        }
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()
    if (PyArg_ParseTuple(args,"O!O!",&(Base::VectorPy::Type), &object1,
                                     &(Base::VectorPy::Type), &object2)) {
        // Note: must be static_cast, not reinterpret_cast
        ptr->Add(*(static_cast<Base::VectorPy*>(object1)->getVectorPtr()));
        ptr->Add(*(static_cast<Base::VectorPy*>(object2)->getVectorPtr()));
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()
    if (PyArg_ParseTuple(args,"O!",&(Base::BoundBoxPy::Type), &object1)) {
        // Note: must be static_cast, not reinterpret_cast
        *ptr = *(static_cast<Base::BoundBoxPy*>(object1)->getBoundBoxPtr());
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Either six floats, two instances of "
            "Vector/Tuple or instance of BoundBox expected");
    return -1;
}

PyObject*  BoundBoxPy::add(PyObject *args)
{
    double x,y,z;
    PyObject *object;
    if (PyArg_ParseTuple(args, "ddd", &x,&y,&z)) {
        getBoundBoxPtr()->Add(Vector3d(x,y,z));
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args,"O!",&PyTuple_Type, &object)) {
        getBoundBoxPtr()->Add(getVectorFromTuple<double>(object));
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args,"O!",&(Base::VectorPy::Type), &object)) {
        getBoundBoxPtr()->Add(*(static_cast<Base::VectorPy*>(object)->getVectorPtr()));
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args,"O!;Need a Vector, BoundBox or three floats as argument",&(Base::BoundBoxPy::Type), &object)) {
        getBoundBoxPtr()->Add(*(static_cast<Base::BoundBoxPy*>(object)->getBoundBoxPtr()));
        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "Either three floats, instance of Vector or instance of BoundBox expected");
    return 0;
}

PyObject*  BoundBoxPy::isIntersection(PyObject *args)
{
    double x,y,z;
    PyObject *object,*object2;
    Py::Boolean retVal;
    if (PyArg_ParseTuple(args, "ddd", &x,&y,&z)) {
        retVal = getBoundBoxPtr()->IsInBox(Vector3d(x,y,z));
    }
    else if (PyArg_ParseTuple(args,"O!",&PyTuple_Type, &object)) {
        PyErr_Clear();
        retVal = getBoundBoxPtr()->IsInBox(getVectorFromTuple<double>(object));
    }
    else if (PyArg_ParseTuple(args,"O!",&(Base::VectorPy::Type), &object)) {
        PyErr_Clear();
        retVal = getBoundBoxPtr()->IsInBox(*(static_cast<Base::VectorPy*>(object)->getVectorPtr()));
    }
    else if (PyArg_ParseTuple(args,"O!O!",&(Base::VectorPy::Type), &object,
                                          &(Base::VectorPy::Type), &object2)) {
        PyErr_Clear();
        retVal = getBoundBoxPtr()->IsCutLine(
            *(static_cast<Base::VectorPy*>(object )->getVectorPtr()),
            *(static_cast<Base::VectorPy*>(object2)->getVectorPtr()));
    }
    else if (PyArg_ParseTuple(args,"O!;Need vector, bounding box or three floats as argument",
        &(Base::BoundBoxPy::Type), &object)) {
        PyErr_Clear();
        retVal = getBoundBoxPtr()->IsInBox(*(static_cast<Base::BoundBoxPy*>(object)->getBoundBoxPtr()));
    }
    else {
        PyErr_SetString(PyExc_TypeError, "Either three floats, Vector(s) or BoundBox expected");
        return 0;
    }

    return Py::new_reference_to(retVal);
}

PyObject*  BoundBoxPy::enlarge(PyObject *args)
{
    double s;
    if (!PyArg_ParseTuple(args, "d;Need float parameter to enlarge", &s))
        return 0;
    getBoundBoxPtr()->Enlarge(s);
    Py_Return;
}

PyObject*  BoundBoxPy::getIntersectionPoint(PyObject *args)
{
    PyObject *object,*object2;
    if (PyArg_ParseTuple(args,"O!O!:Need base and direction vector",
        &(Base::VectorPy::Type), &object,&(Base::VectorPy::Type), &object2)) {
        Base::Vector3d point = getBoundBoxPtr()->IntersectionPoint(
            *(static_cast<Base::VectorPy*>(object)->getVectorPtr()),
            *(static_cast<Base::VectorPy*>(object2)->getVectorPtr()));
        // IsInBox() doesn't handle border points correctly
        BoundBoxPy::PointerType bb = getBoundBoxPtr();
        if ((bb->MinX <= point.x && bb->MaxX >= point.x) &&
            (bb->MinY <= point.y && bb->MaxY >= point.y) &&
            (bb->MinZ <= point.z && bb->MaxZ >= point.z)) {
            return new VectorPy(point);
        }
        else {
            PyErr_SetString(PyExc_Exception, "No intersection");
            return 0;
        }
    }
    else
        return 0;
}

PyObject*  BoundBoxPy::move(PyObject *args)
{
   double x,y,z;
    PyObject *object;
    Base::Vector3d vec;

    if (PyArg_ParseTuple(args, "ddd", &x,&y,&z)) {
        vec = Vector3d(x,y,z);
    }
    else if (PyArg_ParseTuple(args,"O!:Need vector to move",&PyTuple_Type, &object)) {
        PyErr_Clear();
        vec = getVectorFromTuple<double>(object);
    }
    else if (PyArg_ParseTuple(args,"O!:Need vector to move",&(Base::VectorPy::Type), &object)) {
        PyErr_Clear();
        vec = *(static_cast<Base::VectorPy*>(object)->getVectorPtr());
    }
    else {
        PyErr_SetString(PyExc_TypeError, "Either three floats or vector expected");
        return 0;
    }

    getBoundBoxPtr()->MoveX(vec.x);
    getBoundBoxPtr()->MoveY(vec.y);
    getBoundBoxPtr()->MoveZ(vec.z);

    Py_Return;
}

PyObject*  BoundBoxPy::isCutPlane(PyObject *args)
{
    PyObject *object,*object2;
    Py::Boolean retVal;

    if (PyArg_ParseTuple(args,"O!O!:Need base and normal vector of a plane",
        &(Base::VectorPy::Type), &object,&(Base::VectorPy::Type), &object2))
        retVal = getBoundBoxPtr()->IsCutPlane(
            *(static_cast<Base::VectorPy*>(object)->getVectorPtr()),
            *(static_cast<Base::VectorPy*>(object2)->getVectorPtr()));
    else
        return 0;

    return Py::new_reference_to(retVal);
}

Py::Object BoundBoxPy::getCenter(void) const
{
    return Py::Vector(getBoundBoxPtr()->CalcCenter());
}

Py::Float BoundBoxPy::getXMax(void) const
{
    return Py::Float(getBoundBoxPtr()->MaxX);
}

void  BoundBoxPy::setXMax(Py::Float arg)
{
    getBoundBoxPtr()->MaxX = arg;
}

Py::Float BoundBoxPy::getYMax(void) const
{
    return Py::Float(getBoundBoxPtr()->MaxY);
}

void  BoundBoxPy::setYMax(Py::Float arg)
{
    getBoundBoxPtr()->MaxY = arg;
}

Py::Float BoundBoxPy::getZMax(void) const
{
    return Py::Float(getBoundBoxPtr()->MaxZ);
}

void  BoundBoxPy::setZMax(Py::Float arg)
{
    getBoundBoxPtr()->MaxZ = arg;
}

Py::Float BoundBoxPy::getXMin(void) const
{
    return Py::Float(getBoundBoxPtr()->MinX);
}

void  BoundBoxPy::setXMin(Py::Float arg)
{
    getBoundBoxPtr()->MinX = arg;
}

Py::Float BoundBoxPy::getYMin(void) const
{
    return Py::Float(getBoundBoxPtr()->MinY);
}

void  BoundBoxPy::setYMin(Py::Float arg)
{
    getBoundBoxPtr()->MinY = arg;
}

Py::Float BoundBoxPy::getZMin(void) const
{
    return Py::Float(getBoundBoxPtr()->MinZ);
}

void  BoundBoxPy::setZMin(Py::Float arg)
{
    getBoundBoxPtr()->MinZ = arg;
}

Py::Float BoundBoxPy::getXLength(void) const
{
    return Py::Float(getBoundBoxPtr()->LengthX());
}

Py::Float BoundBoxPy::getYLength(void) const
{
    return Py::Float(getBoundBoxPtr()->LengthY());
}

Py::Float BoundBoxPy::getZLength(void) const
{
    return Py::Float(getBoundBoxPtr()->LengthZ());
}

Py::Float BoundBoxPy::getDiagonalLength(void) const
{
    return Py::Float(getBoundBoxPtr()->CalcDiagonalLength());
}

PyObject *BoundBoxPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int BoundBoxPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


