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

#endif

#include "Cosmetic.h"
#include "CosmeticEdgePy.h"
#include "CosmeticEdgePy.cpp"

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string CosmeticEdgePy::representation(void) const
{
    return "<CosmeticEdge object>";
}

PyObject *CosmeticEdgePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'CosmeticEdge'.");
    return 0;
}

// constructor method
int CosmeticEdgePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* CosmeticEdgePy::clone(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    TechDraw::CosmeticEdge* geom = this->getCosmeticEdgePtr();
    geom->dump("CEPYI::clone");
    PyTypeObject* type = this->GetType();
    PyObject* cpy = 0;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, 0);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create clone of CosmeticEdge");
        return 0;
    }

    TechDraw::CosmeticEdgePy* geompy = static_cast<TechDraw::CosmeticEdgePy*>(cpy);
    // the PyMake function must have created the corresponding instance of the 'CosmeticEdge' subclass
    // so delete it now to avoid a memory leak
    if (geompy->_pcTwinPointer) {
        TechDraw::CosmeticEdge* clone = static_cast<TechDraw::CosmeticEdge*>(geompy->_pcTwinPointer);
        delete clone;
    }
    geompy->_pcTwinPointer = geom->clone();
    return cpy;
}

PyObject* CosmeticEdgePy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    TechDraw::CosmeticEdge* geom = this->getCosmeticEdgePtr();
    geom->dump("CEPYI::copy");
    PyTypeObject* type = this->GetType();
    PyObject* cpy = 0;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, 0);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of CosmeticEdge");
        return 0;
    }

    TechDraw::CosmeticEdgePy* geompy = static_cast<TechDraw::CosmeticEdgePy*>(cpy);
    // the PyMake function must have created the corresponding instance of the 'CosmeticEdge' subclass
    // so delete it now to avoid a memory leak
    if (geompy->_pcTwinPointer) {
        TechDraw::CosmeticEdge* copy = static_cast<TechDraw::CosmeticEdge*>(geompy->_pcTwinPointer);
        delete copy;
    }
    geompy->_pcTwinPointer = geom->copy();
    return cpy;
}

PyObject *CosmeticEdgePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int CosmeticEdgePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
