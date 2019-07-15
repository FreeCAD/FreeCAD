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

//# include <boost/uuid/uuid_io.hpp>
#endif

#include "DrawUtil.h"
#include "Cosmetic.h"
#include "CenterLinePy.h"
#include "CenterLinePy.cpp"

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string CenterLinePy::representation(void) const
{
    return "<CenterLine object>";
}

PyObject *CenterLinePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'CenterLine'.");
    return 0;
}

// constructor method
int CenterLinePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* CenterLinePy::clone(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    TechDraw::CenterLine* geom = this->getCenterLinePtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = 0;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, 0);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create clone of CenterLine");
        return 0;
    }

    TechDraw::CenterLinePy* geompy = static_cast<TechDraw::CenterLinePy*>(cpy);
    // the PyMake function must have created the corresponding instance of the 'CenterLine' subclass
    // so delete it now to avoid a memory leak
    if (geompy->_pcTwinPointer) {
        TechDraw::CenterLine* clone = static_cast<TechDraw::CenterLine*>(geompy->_pcTwinPointer);
        delete clone;
    }
    geompy->_pcTwinPointer = geom->clone();
    return cpy;
}

PyObject* CenterLinePy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    TechDraw::CenterLine* geom = this->getCenterLinePtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = 0;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, 0);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of CenterLine");
        return 0;
    }

    TechDraw::CenterLinePy* geompy = static_cast<TechDraw::CenterLinePy*>(cpy);
    // the PyMake function must have created the corresponding instance of the 'CenterLine' subclass
    // so delete it now to avoid a memory leak
    if (geompy->_pcTwinPointer) {
        TechDraw::CenterLine* copy = static_cast<TechDraw::CenterLine*>(geompy->_pcTwinPointer);
        delete copy;
    }
    geompy->_pcTwinPointer = geom->copy();
    return cpy;
}

PyObject* CenterLinePy::setFormat(PyObject* args)
{
//    Base::Console().Message("CLPI::setFormat()\n");
    PyObject* pTuple;
    int style = 1;
    double weight = 0.50;
    double red = 0.0, green = 0.0, blue = 0.0, alpha = 0.0;
    App::Color c(red, blue, green, alpha);
    bool visible = 1; 
    if (!PyArg_ParseTuple(args, "O", &pTuple)) {
      return NULL;
    }
    
    TechDraw::CenterLine* cl = this->getCenterLinePtr();
    if (PyTuple_Check(pTuple)) {
        int tSize = (int) PyTuple_Size(pTuple);
        if (tSize > 3) {
            PyObject* pStyle = PyTuple_GetItem(pTuple,0);
            style = (int) PyLong_AsLong(pStyle);
            PyObject* pWeight = PyTuple_GetItem(pTuple,1);
            weight = PyFloat_AsDouble(pWeight);
            PyObject* pColor = PyTuple_GetItem(pTuple,2);
            c = DrawUtil::pyTupleToColor(pColor);
            PyObject* pVisible = PyTuple_GetItem(pTuple,3);
            visible = (bool) PyLong_AsLong(pVisible);

            cl->m_format.m_style = style;
            cl->m_format.m_weight = weight;
            cl->m_format.m_color = c;
            cl->m_format.m_visible = visible;
        }
    } else {
        Base::Console().Error("CLPI::setFormat - not a tuple!\n");
    }
    
    return Py_None;
}

PyObject* CenterLinePy::getFormat(PyObject *args)
{
    (void) args;
//    Base::Console().Message("CLPI::getFormat()\n");
    TechDraw::CenterLine* cl = this->getCenterLinePtr();

    PyObject* pStyle = PyLong_FromLong((long) cl->m_format.m_style);
    PyObject* pWeight = PyFloat_FromDouble(cl->m_format.m_weight);
    PyObject* pColor = DrawUtil::colorToPyTuple(cl->m_format.m_color);
    PyObject* pVisible = PyBool_FromLong((long) cl->m_format.m_visible);

    PyObject* result = PyTuple_New(4);

    PyTuple_SET_ITEM(result, 0, pStyle);
    PyTuple_SET_ITEM(result, 1, pWeight);
    PyTuple_SET_ITEM(result, 2, pColor);
    PyTuple_SET_ITEM(result, 3, pVisible);

    return result;
}

PyObject *CenterLinePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int CenterLinePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
