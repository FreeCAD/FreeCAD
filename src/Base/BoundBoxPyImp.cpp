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

// inclusion of the generated files (generated out of BoundBoxPy.xml)
#include "MatrixPy.h"
#include "VectorPy.h"
#include "GeometryPyCXX.h"
#include "BoundBoxPy.h"
#include "BoundBoxPy.cpp"

using namespace Base;

// returns a string which represent the object e.g. when printed in python
std::string BoundBoxPy::representation() const
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
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()

    double xMin=0.0,yMin=0.0,zMin=0.0,xMax=0.0,yMax=0.0,zMax=0.0;
    PyObject *object1{}, *object2{};
    BoundBoxPy::PointerType ptr = getBoundBoxPtr();
    if (PyArg_ParseTuple(args, "d|ddddd", &xMin, &yMin, &zMin, &xMax, &yMax, &zMax)) {
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
        ptr->Add(*(static_cast<Base::VectorPy*>(object1)->getVectorPtr()));
        ptr->Add(*(static_cast<Base::VectorPy*>(object2)->getVectorPtr()));
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()
    if (PyArg_ParseTuple(args,"O!",&(Base::BoundBoxPy::Type), &object1)) {
        *ptr = *(static_cast<Base::BoundBoxPy*>(object1)->getBoundBoxPtr());
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Either six floats, two instances of "
            "Vector/Tuple or instance of BoundBox expected");
    return -1;
}

PyObject*  BoundBoxPy::setVoid(PyObject *args)
{
    if (!PyArg_ParseTuple(args,""))
        return nullptr;

    getBoundBoxPtr()->SetVoid();
    Py_Return;
}

PyObject*  BoundBoxPy::isValid(PyObject *args)
{
    if (!PyArg_ParseTuple(args,""))
        return nullptr;

    return PyBool_FromLong(getBoundBoxPtr()->IsValid() ? 1 : 0);
}

PyObject*  BoundBoxPy::add(PyObject *args)
{
    double x{},y{},z{};
    PyObject *object{};
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
    return nullptr;
}

PyObject*  BoundBoxPy::getPoint(PyObject *args)
{
    unsigned short index{};
    if (!PyArg_ParseTuple(args,"H",&index))
        return nullptr;

    if (index > 7) {
        PyErr_SetString (PyExc_IndexError, "Invalid point index");
        return nullptr;
    }

    Base::Vector3d pnt = getBoundBoxPtr()->CalcPoint(index);
    return new Base::VectorPy(new Base::Vector3d(pnt));
}

PyObject*  BoundBoxPy::getEdge(PyObject *args)
{
    unsigned short index{};
    if (!PyArg_ParseTuple(args,"H",&index))
        return nullptr;

    if (index > 11) {
        PyErr_SetString (PyExc_IndexError, "Invalid edge index");
        return nullptr;
    }

    Base::Vector3d pnt1, pnt2;
    getBoundBoxPtr()->CalcEdge(index, pnt1, pnt2);
    Py::Tuple tuple(2);
    tuple.setItem(0, Py::Vector(pnt1));
    tuple.setItem(1, Py::Vector(pnt2));
    return Py::new_reference_to(tuple);
}

PyObject*  BoundBoxPy::closestPoint(PyObject *args)
{
    double x{},y{},z{};
    PyObject *object{};

    Base::Vector3d vec;

    do {
        if (PyArg_ParseTuple(args, "ddd", &x,&y,&z)) {
            vec = Vector3d(x,y,z);
            break;
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args,"O!",&PyTuple_Type, &object)) {
            vec = getVectorFromTuple<double>(object);
            break;
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args,"O!",&(Base::VectorPy::Type), &object)) {
            vec = *(static_cast<Base::VectorPy*>(object)->getVectorPtr());
            break;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Either three floats or vector expected");
            return nullptr;
        }
    }
    while(false);

    Base::Vector3d point = getBoundBoxPtr()->ClosestPoint(vec);
    return new Base::VectorPy(new Base::Vector3d(point));
}

PyObject*  BoundBoxPy::intersect(PyObject *args)
{
    PyObject *object{},*object2{};
    Py::Boolean retVal;

    if (!getBoundBoxPtr()->IsValid()) {
        PyErr_SetString (PyExc_FloatingPointError, "Invalid bounding box");
        return nullptr;
    }

    do {
        if (PyArg_ParseTuple(args,"O!O!",&(Base::VectorPy::Type), &object,
                                         &(Base::VectorPy::Type), &object2)) {
            retVal = getBoundBoxPtr()->IsCutLine(
                *(static_cast<Base::VectorPy*>(object )->getVectorPtr()),
                *(static_cast<Base::VectorPy*>(object2)->getVectorPtr()));
            break;
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args,"O!",&(Base::BoundBoxPy::Type), &object)) {
            if (!static_cast<Base::BoundBoxPy*>(object)->getBoundBoxPtr()->IsValid()) {
                PyErr_SetString (PyExc_FloatingPointError, "Invalid bounding box argument");
                return nullptr;
            }
            retVal = getBoundBoxPtr()->Intersect(*(static_cast<Base::BoundBoxPy*>(object)->getBoundBoxPtr()));
            break;
        }

        PyErr_SetString(PyExc_TypeError, "Either BoundBox or two Vectors expected");
        return nullptr;
    }
    while(false);

    return Py::new_reference_to(retVal);
}

PyObject*  BoundBoxPy::intersected(PyObject *args)
{
    if (!getBoundBoxPtr()->IsValid()) {
        PyErr_SetString (PyExc_FloatingPointError, "Invalid bounding box");
        return nullptr;
    }

    PyObject *object{};
    if (!PyArg_ParseTuple(args,"O!",&(Base::BoundBoxPy::Type), &object))
        return nullptr;
    if (!static_cast<Base::BoundBoxPy*>(object)->getBoundBoxPtr()->IsValid()) {
        PyErr_SetString (PyExc_FloatingPointError, "Invalid bounding box argument");
        return nullptr;
    }

    Base::BoundBox3d bbox = getBoundBoxPtr()->Intersected(*static_cast<Base::BoundBoxPy*>(object)->getBoundBoxPtr());
    return new Base::BoundBoxPy(new Base::BoundBox3d(bbox));
}

PyObject*  BoundBoxPy::united(PyObject *args)
{
    if (!getBoundBoxPtr()->IsValid()) {
        PyErr_SetString (PyExc_FloatingPointError, "Invalid bounding box");
        return nullptr;
    }

    PyObject *object{};
    if (!PyArg_ParseTuple(args,"O!",&(Base::BoundBoxPy::Type), &object))
        return nullptr;
    if (!static_cast<Base::BoundBoxPy*>(object)->getBoundBoxPtr()->IsValid()) {
        PyErr_SetString (PyExc_FloatingPointError, "Invalid bounding box argument");
        return nullptr;
    }

    Base::BoundBox3d bbox = getBoundBoxPtr()->United(*static_cast<Base::BoundBoxPy*>(object)->getBoundBoxPtr());
    return new Base::BoundBoxPy(new Base::BoundBox3d(bbox));
}

PyObject*  BoundBoxPy::enlarge(PyObject *args)
{
    double s{};
    if (!PyArg_ParseTuple(args, "d;Need float parameter to enlarge", &s))
        return nullptr;
    getBoundBoxPtr()->Enlarge(s);
    Py_Return;
}

PyObject*  BoundBoxPy::getIntersectionPoint(PyObject *args)
{
    PyObject *object{},*object2{};
    double epsilon=0.0001;
    if (!PyArg_ParseTuple(args,"O!O!|d;Need base and direction vector",
        &(Base::VectorPy::Type), &object,&(Base::VectorPy::Type), &object2, &epsilon))
        return nullptr;

    Base::Vector3d point;
    bool ok = getBoundBoxPtr()->IntersectionPoint(
        *(static_cast<Base::VectorPy*>(object)->getVectorPtr()),
        *(static_cast<Base::VectorPy*>(object2)->getVectorPtr()),
        point, epsilon);
    if (ok) {
        return new VectorPy(point);
    }
    else {
        PyErr_SetString(Base::PyExc_FC_GeneralError, "No intersection");
        return nullptr;
    }
}

PyObject*  BoundBoxPy::move(PyObject *args)
{
    double x{},y{},z{};
    PyObject *object{};

    Base::Vector3d vec;

    do {
        if (PyArg_ParseTuple(args, "ddd", &x,&y,&z)) {
            vec = Vector3d(x,y,z);
            break;
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args,"O!",&PyTuple_Type, &object)) {
            vec = getVectorFromTuple<double>(object);
            break;
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args,"O!",&(Base::VectorPy::Type), &object)) {
            vec = *(static_cast<Base::VectorPy*>(object)->getVectorPtr());
            break;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Either three floats or vector expected");
            return nullptr;
        }
    }
    while(false);

    getBoundBoxPtr()->MoveX(vec.x);
    getBoundBoxPtr()->MoveY(vec.y);
    getBoundBoxPtr()->MoveZ(vec.z);

    Py_Return;
}

PyObject*  BoundBoxPy::scale(PyObject *args)
{
    double x{},y{},z{};
    PyObject *object{};

    Base::Vector3d vec;

    do {
        if (PyArg_ParseTuple(args, "ddd", &x,&y,&z)) {
            vec = Vector3d(x,y,z);
            break;
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args,"O!",&PyTuple_Type, &object)) {
            vec = getVectorFromTuple<double>(object);
            break;
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args,"O!",&(Base::VectorPy::Type), &object)) {
            vec = *(static_cast<Base::VectorPy*>(object)->getVectorPtr());
            break;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Either three floats or vector expected");
            return nullptr;
        }
    }
    while(false);

    getBoundBoxPtr()->ScaleX(vec.x);
    getBoundBoxPtr()->ScaleY(vec.y);
    getBoundBoxPtr()->ScaleZ(vec.z);

    Py_Return;
}

PyObject*  BoundBoxPy::transformed(PyObject *args)
{
    PyObject *mat{};

    if (!PyArg_ParseTuple(args,"O!", &(Base::MatrixPy::Type), &mat))
        return nullptr;

    if (!getBoundBoxPtr()->IsValid())
        throw Py::FloatingPointError("Cannot transform invalid bounding box");
    Base::BoundBox3d bbox = getBoundBoxPtr()->Transformed(*static_cast<Base::MatrixPy*>(mat)->getMatrixPtr());
    return new Base::BoundBoxPy(new Base::BoundBox3d(bbox));
}

PyObject*  BoundBoxPy::isCutPlane(PyObject *args)
{
    PyObject *object{},*object2{};
    Py::Boolean retVal;

    if (!getBoundBoxPtr()->IsValid()) {
        PyErr_SetString (PyExc_FloatingPointError, "Invalid bounding box");
        return nullptr;
    }

    if (!PyArg_ParseTuple(args,"O!O!;Need base and normal vector of a plane",
        &(Base::VectorPy::Type), &object,&(Base::VectorPy::Type), &object2))
        return nullptr;

    retVal = getBoundBoxPtr()->IsCutPlane(
        *(static_cast<Base::VectorPy*>(object)->getVectorPtr()),
        *(static_cast<Base::VectorPy*>(object2)->getVectorPtr()));

    return Py::new_reference_to(retVal);
}

PyObject*  BoundBoxPy::isInside(PyObject *args)
{
    double x{},y{},z{};
    PyObject *object{};
    Py::Boolean retVal;

    if (!getBoundBoxPtr()->IsValid()) {
        PyErr_SetString (PyExc_FloatingPointError, "Invalid bounding box");
        return nullptr;
    }

    do {
        if (PyArg_ParseTuple(args, "ddd", &x,&y,&z)) {
            retVal = getBoundBoxPtr()->IsInBox(Vector3d(x,y,z));
            break;
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args,"O!",&PyTuple_Type, &object)) {
            retVal = getBoundBoxPtr()->IsInBox(getVectorFromTuple<double>(object));
            break;
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args,"O!",&(Base::VectorPy::Type), &object)) {
            retVal = getBoundBoxPtr()->IsInBox(*(static_cast<Base::VectorPy*>(object)->getVectorPtr()));
            break;
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args,"O!",&(Base::BoundBoxPy::Type), &object)) {
            if (!static_cast<Base::BoundBoxPy*>(object)->getBoundBoxPtr()->IsValid()) {
                PyErr_SetString (PyExc_FloatingPointError, "Invalid bounding box argument");
                return nullptr;
            }
            retVal = getBoundBoxPtr()->IsInBox(*(static_cast<Base::BoundBoxPy*>(object)->getBoundBoxPtr()));
            break;
        }

        PyErr_SetString(PyExc_TypeError, "Either three floats, Vector(s) or BoundBox expected");
        return nullptr;
    }
    while(false);

    return Py::new_reference_to(retVal);
}

Py::Object BoundBoxPy::getCenter() const
{
    return Py::Vector(getBoundBoxPtr()->GetCenter()); // NOLINT
}

Py::Float BoundBoxPy::getXMax() const
{
    return Py::Float(getBoundBoxPtr()->MaxX);
}

void  BoundBoxPy::setXMax(Py::Float arg)
{
    getBoundBoxPtr()->MaxX = arg;
}

Py::Float BoundBoxPy::getYMax() const
{
    return Py::Float(getBoundBoxPtr()->MaxY);
}

void  BoundBoxPy::setYMax(Py::Float arg)
{
    getBoundBoxPtr()->MaxY = arg;
}

Py::Float BoundBoxPy::getZMax() const
{
    return Py::Float(getBoundBoxPtr()->MaxZ);
}

void  BoundBoxPy::setZMax(Py::Float arg)
{
    getBoundBoxPtr()->MaxZ = arg;
}

Py::Float BoundBoxPy::getXMin() const
{
    return Py::Float(getBoundBoxPtr()->MinX);
}

void  BoundBoxPy::setXMin(Py::Float arg)
{
    getBoundBoxPtr()->MinX = arg;
}

Py::Float BoundBoxPy::getYMin() const
{
    return Py::Float(getBoundBoxPtr()->MinY);
}

void  BoundBoxPy::setYMin(Py::Float arg)
{
    getBoundBoxPtr()->MinY = arg;
}

Py::Float BoundBoxPy::getZMin() const
{
    return Py::Float(getBoundBoxPtr()->MinZ);
}

void  BoundBoxPy::setZMin(Py::Float arg)
{
    getBoundBoxPtr()->MinZ = arg;
}

Py::Float BoundBoxPy::getXLength() const
{
    return Py::Float(getBoundBoxPtr()->LengthX());
}

Py::Float BoundBoxPy::getYLength() const
{
    return Py::Float(getBoundBoxPtr()->LengthY());
}

Py::Float BoundBoxPy::getZLength() const
{
    return Py::Float(getBoundBoxPtr()->LengthZ());
}

Py::Float BoundBoxPy::getDiagonalLength() const
{
    if (!getBoundBoxPtr()->IsValid())
        throw Py::FloatingPointError("Cannot determine diagonal length of invalid bounding box");
    return Py::Float(getBoundBoxPtr()->CalcDiagonalLength());
}

PyObject *BoundBoxPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int BoundBoxPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


