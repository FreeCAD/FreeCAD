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

#ifndef _PreComp_
# include <boost/uuid/uuid_io.hpp>
#endif

#include <Base/Console.h>
#include <Base/GeometryPyCXX.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

#include "Cosmetic.h"
#include "CosmeticVertex.h"
#include "CosmeticVertexPy.h"
#include "CosmeticVertexPy.cpp"
#include "DrawUtil.h"


using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string CosmeticVertexPy::representation() const
{
    return "<CosmeticVertex object>";
}

PyObject *CosmeticVertexPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'CosmeticVertex'.");
    return nullptr;
}

// constructor method
int CosmeticVertexPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* CosmeticVertexPy::clone(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TechDraw::CosmeticVertex* geom = this->getCosmeticVertexPtr();
//    geom->dump("CEPYI::clone");
    PyTypeObject* type = this->GetType();
    PyObject* cpy = nullptr;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, nullptr);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create clone of CosmeticVertex");
        return nullptr;
    }

    TechDraw::CosmeticVertexPy* geompy = static_cast<TechDraw::CosmeticVertexPy*>(cpy);
    // the PyMake function must have created the corresponding instance of the 'CosmeticVertex' subclass
    // so delete it now to avoid a memory leak
    if (geompy->_pcTwinPointer) {
        TechDraw::CosmeticVertex* clone = static_cast<TechDraw::CosmeticVertex*>(geompy->_pcTwinPointer);
        delete clone;
    }
    geompy->_pcTwinPointer = geom->clone();
    return cpy;
}

PyObject* CosmeticVertexPy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TechDraw::CosmeticVertex* geom = this->getCosmeticVertexPtr();
//    geom->dump("CEPYI::copy");
    PyTypeObject* type = this->GetType();
    PyObject* cpy = nullptr;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, nullptr);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of CosmeticVertex");
        return nullptr;
    }

    TechDraw::CosmeticVertexPy* geompy = static_cast<TechDraw::CosmeticVertexPy*>(cpy);
    // the PyMake function must have created the corresponding instance of the 'CosmeticVertex' subclass
    // so delete it now to avoid a memory leak
    if (geompy->_pcTwinPointer) {
        TechDraw::CosmeticVertex* copy = static_cast<TechDraw::CosmeticVertex*>(geompy->_pcTwinPointer);
        delete copy;
    }
    geompy->_pcTwinPointer = geom->copy();
    return cpy;
}

Py::String CosmeticVertexPy::getTag() const
{
    std::string tmp = boost::uuids::to_string(getCosmeticVertexPtr()->getTag());
    return Py::String(tmp);
}

Py::Object CosmeticVertexPy::getPoint() const
{
    Base::Vector3d point = getCosmeticVertexPtr()->permaPoint;
    point = DrawUtil::invertY(point);
    return Py::asObject(new Base::VectorPy(point));
}

void CosmeticVertexPy::setPoint(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d point = static_cast<Base::VectorPy*>(p)->value();
        getCosmeticVertexPtr()->permaPoint =
                DrawUtil::invertY(point);
    }
    else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        Base::Vector3d point = Base::getVectorFromTuple<double>(p);
        getCosmeticVertexPtr()->permaPoint =
                DrawUtil::invertY(point);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Boolean CosmeticVertexPy::getShow() const
{
    bool show = getCosmeticVertexPtr()->visible;
    return Py::Boolean(show);
}

void CosmeticVertexPy::setShow(Py::Boolean arg)
{
    PyObject* p = arg.ptr();
    if (PyBool_Check(p)) {
        if (p == Py_True) {
            getCosmeticVertexPtr()->visible = true;
        } else {
            getCosmeticVertexPtr()->visible = false;
        }
    }
}

Py::Object CosmeticVertexPy::getColor() const
{
    App::Color color = getCosmeticVertexPtr()->color;
    PyObject* pyColor = DrawUtil::colorToPyTuple(color);
    return Py::asObject(pyColor);
}

void CosmeticVertexPy::setColor(Py::Object arg)
{
    PyObject* pTuple = arg.ptr();
    double red = 0.0, green = 0.0, blue = 0.0, alpha = 0.0;
    App::Color c(red, green, blue, alpha);
    if (PyTuple_Check(pTuple)) {
        c = DrawUtil::pyTupleToColor(pTuple);
        CosmeticVertex* cv = getCosmeticVertexPtr();
        cv->color = c;
    } else {
        Base::Console().Error("CEPI::setColor - not a tuple!\n");
        std::string error = std::string("type must be 'tuple', not ");
        error += pTuple->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object CosmeticVertexPy::getSize() const
{
    CosmeticVertex* cv = getCosmeticVertexPtr();
    double size = cv->size;
    PyObject* pSize = PyFloat_FromDouble(size);
    return Py::asObject(pSize);
}

void CosmeticVertexPy::setSize(Py::Object arg)
{
    double size = 1.0;
    PyObject* p = arg.ptr();
    if (PyFloat_Check(p)) {
        size = PyFloat_AsDouble(p);
    } else if (PyLong_Check(p)) {
        size = (double) PyLong_AsLong(p);
    } else {
        throw Py::TypeError("expected (float)");
    }
    CosmeticVertex* cv = getCosmeticVertexPtr();
    cv->size = size;
}

Py::Object CosmeticVertexPy::getStyle() const
{
    CosmeticVertex* cv = getCosmeticVertexPtr();
    double style = cv->style;
    PyObject* pStyle = PyLong_FromLong((long) style);
    return Py::asObject(pStyle);
}

void CosmeticVertexPy::setStyle(Py::Object arg)
{
    int style = 1;
    PyObject* p = arg.ptr();
    if (PyLong_Check(p)) {
        style = (int) PyLong_AsLong(p);
    } else {
        throw Py::TypeError("expected (float)");
    }
    CosmeticVertex* cv = getCosmeticVertexPtr();
    cv->style = style;
}

PyObject *CosmeticVertexPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int CosmeticVertexPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
