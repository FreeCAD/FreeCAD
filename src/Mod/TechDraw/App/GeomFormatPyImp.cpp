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

#include "GeomFormatPy.h"
#include "GeomFormatPy.cpp"


using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string GeomFormatPy::representation(void) const
{
    return "<GeomFormat object>";
}

PyObject *GeomFormatPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'GeomFormat'.");
    return nullptr;
}

// constructor method
int GeomFormatPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* GeomFormatPy::clone(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TechDraw::GeomFormat* geom = this->getGeomFormatPtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = nullptr;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, nullptr);
    if (!cpy) {
        PyErr_SetString(PyExc_RuntimeError, "failed to create clone of GeomFormat");
        return nullptr;
    }

    TechDraw::GeomFormatPy* geompy = static_cast<TechDraw::GeomFormatPy*>(cpy);
    // the PyMake function must have created the corresponding instance of the 'GeomFormat' subclass
    // so delete it now to avoid a memory leak
    if (geompy->_pcTwinPointer) {
        TechDraw::GeomFormat* clone = static_cast<TechDraw::GeomFormat*>(geompy->_pcTwinPointer);
        delete clone;
    }
    geompy->_pcTwinPointer = geom->clone();
    return cpy;
}

PyObject* GeomFormatPy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TechDraw::GeomFormat* geom = this->getGeomFormatPtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = nullptr;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, nullptr);
    if (!cpy) {
        PyErr_SetString(PyExc_RuntimeError, "failed to create copy of GeomFormat");
        return nullptr;
    }

    TechDraw::GeomFormatPy* geompy = static_cast<TechDraw::GeomFormatPy*>(cpy);
    // the PyMake function must have created the corresponding instance of the 'GeomFormat' subclass
    // so delete it now to avoid a memory leak
    if (geompy->_pcTwinPointer) {
        TechDraw::GeomFormat* copy = static_cast<TechDraw::GeomFormat*>(geompy->_pcTwinPointer);
        delete copy;
    }
    geompy->_pcTwinPointer = geom->copy();
    return cpy;
}

Py::String GeomFormatPy::getTag(void) const
{
    std::string tmp = boost::uuids::to_string(getGeomFormatPtr()->getTag());
    return Py::String(tmp);
}

PyObject *GeomFormatPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int GeomFormatPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
