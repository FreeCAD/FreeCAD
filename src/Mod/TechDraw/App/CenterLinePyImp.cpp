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

#include "DrawUtil.h"
#include "Cosmetic.h"
#include "CenterLinePy.h"
#include "CenterLinePy.cpp"

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string CenterLinePy::representation(void) const
{
    std::stringstream ss;
    ss << "<CenterLine object> at " << std::hex << this;
    return ss.str();
}

PyObject *CenterLinePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'CenterLine'.");
    return nullptr;
}

// constructor method
int CenterLinePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* CenterLinePy::clone(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TechDraw::CenterLine* geom = this->getCenterLinePtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = nullptr;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, nullptr);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create clone of CenterLine");
        return nullptr;
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
        return nullptr;

    TechDraw::CenterLine* geom = this->getCenterLinePtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = nullptr;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, nullptr);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of CenterLine");
        return nullptr;
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

Py::Object CenterLinePy::getFormat(void) const
{
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

    return Py::asObject(result);
}

void CenterLinePy::setFormat(Py::Object arg)
{
    PyObject* pTuple = arg.ptr();
    int style = 1;
    double weight = 0.50;
    double red = 0.0, green = 0.0, blue = 0.0, alpha = 0.0;
    App::Color c(red, blue, green, alpha);
    bool visible = 1; 
    
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
}

Py::String CenterLinePy::getTag(void) const
{
    std::string tmp = boost::uuids::to_string(getCenterLinePtr()->getTag());
    return Py::String(tmp);
}


Py::Long CenterLinePy::getType(void) const
{
    int tmp = getCenterLinePtr()->m_type;
    return Py::Long(tmp);
}

Py::Long CenterLinePy::getMode(void) const
{
    int tmp = getCenterLinePtr()->m_mode;
    return Py::Long(tmp);
}

void CenterLinePy::setMode(Py::Long arg)
{
    PyObject* p = arg.ptr();
    if (PyLong_Check(p)) {
        long int temp =  PyLong_AsLong(p);
        getCenterLinePtr()->m_mode = (int) temp;
    } else {
        std::string error = std::string("type must be 'Integer', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Float CenterLinePy::getHorizShift(void) const
{
    double shift = getCenterLinePtr()->getHShift();
    return  Py::asObject(PyFloat_FromDouble(shift));
}

void CenterLinePy::setHorizShift(Py::Float arg)
{
    PyObject* p = arg.ptr();
    if (PyFloat_Check(p)) {
        double hshift =  PyFloat_AsDouble(p);
        double vshift = getCenterLinePtr()->getVShift();
        getCenterLinePtr()->setShifts(hshift, vshift);
    } else {
        std::string error = std::string("type must be 'Float', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Float CenterLinePy::getVertShift(void) const
{
    double shift = getCenterLinePtr()->getVShift();
    return  Py::asObject(PyFloat_FromDouble(shift));
}

void CenterLinePy::setVertShift(Py::Float arg)
{
    PyObject* p = arg.ptr();
    if (PyFloat_Check(p)) {
        double vshift =  PyFloat_AsDouble(p);
        double hshift = getCenterLinePtr()->getHShift();
        getCenterLinePtr()->setShifts(hshift, vshift);
    } else {
        std::string error = std::string("type must be 'Float', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Float CenterLinePy::getRotation(void) const
{
    double rot = getCenterLinePtr()->getRotate();
    return  Py::asObject(PyFloat_FromDouble(rot));
}

void CenterLinePy::setRotation(Py::Float arg)
{
    PyObject* p = arg.ptr();
    if (PyFloat_Check(p)) {
        double rot =  PyFloat_AsDouble(p);
        getCenterLinePtr()->setRotate(rot);
    } else {
        std::string error = std::string("type must be 'Float', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Float CenterLinePy::getExtension(void) const
{
    double rot = getCenterLinePtr()->getExtend();
    return  Py::asObject(PyFloat_FromDouble(rot));
}

void CenterLinePy::setExtension(Py::Float arg)
{
    PyObject* p = arg.ptr();
    if (PyFloat_Check(p)) {
        double ext =  PyFloat_AsDouble(p);
        getCenterLinePtr()->setExtend(ext);
    } else {
        std::string error = std::string("type must be 'Float', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Boolean CenterLinePy::getFlip(void) const
{
    bool flip = getCenterLinePtr()->getFlip();
    return Py::Boolean(flip);
}

void CenterLinePy::setFlip(Py::Boolean arg)
{
    PyObject* p = arg.ptr();
    if (PyBool_Check(p)) {
        if (p == Py_True) {
            getCenterLinePtr()->setFlip(true);
        } else {
            getCenterLinePtr()->setFlip(false);
        }
    } else {
        std::string error = std::string("type must be 'Boolean', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object CenterLinePy::getEdges(void) const
{
//    Base::Console().Message("CLP::getEdges()\n");
    TechDraw::CenterLine* cl = this->getCenterLinePtr();

    std::vector<std::string> edges = cl->m_edges;
    int size = edges.size();

    Py::List result(size);

    for (auto& e: edges) {
        result.append(Py::asObject(PyUnicode_FromString(e.c_str())));
    }

    return result;
}

void CenterLinePy::setEdges(Py::Object arg)
{
//    Base::Console().Message("CLP::setEdges()\n");
    PyObject* pList = arg.ptr();
    
    TechDraw::CenterLine* cl = this->getCenterLinePtr();
    std::vector<std::string> temp;
    if (PyList_Check(pList)) {
        int tSize = (int) PyList_Size(pList);
        int i = 0;
        for ( ; i < tSize; i++) {
            PyObject* item = PyList_GetItem(pList, (Py_ssize_t) i);
            if (PyUnicode_Check(item)) {
                std::string s = PyUnicode_AsUTF8(item);       //py3 only!!!
                temp.push_back(s);
            }
        }
        cl->m_edges = temp;
    } else {
        Base::Console().Error("CLPI::setEdges - input not a list!\n");
    }
}
Py::Object CenterLinePy::getFaces(void) const
{
//    Base::Console().Message("CLP::getFaces()\n");
    TechDraw::CenterLine* cl = this->getCenterLinePtr();

    std::vector<std::string> faces = cl->m_faces;
    int size = faces.size();

    Py::List result(size);

    for (auto& f: faces) {
        result.append(Py::asObject(PyUnicode_FromString(f.c_str())));
    }

    return result;
}

void CenterLinePy::setFaces(Py::Object arg)
{
//    Base::Console().Message("CLP::setFaces()\n");
    PyObject* pList = arg.ptr();
    
    TechDraw::CenterLine* cl = this->getCenterLinePtr();
    std::vector<std::string> temp;
    if (PyList_Check(pList)) {
        int tSize = (int) PyList_Size(pList);
        int i = 0;
        for ( ; i < tSize; i++) {
            PyObject* item = PyList_GetItem(pList, (Py_ssize_t) i);
            if (PyUnicode_Check(item)) {
                std::string s = PyUnicode_AsUTF8(item);       //py3 only!!!
                temp.push_back(s);
            }
        }
        cl->m_faces = temp;
    } else {
        Base::Console().Error("CLPI::setFaces - input not a list!\n");
    }
}

Py::Object CenterLinePy::getPoints(void) const
{
//    Base::Console().Message("CLP::getPoints()\n");
    TechDraw::CenterLine* cl = this->getCenterLinePtr();

    std::vector<std::string> points = cl->m_verts;
    int size = points.size();

    Py::List result(size);

    for (auto& p: points) {
        result.append(Py::asObject(PyUnicode_FromString(p.c_str())));
    }

    return result;
}

void CenterLinePy::setPoints(Py::Object arg)
{
//    Base::Console().Message("CLP::setPoints()\n");
    PyObject* pList = arg.ptr();
    
    TechDraw::CenterLine* cl = this->getCenterLinePtr();
    std::vector<std::string> temp;
    if (PyList_Check(pList)) {
        int tSize = (int) PyList_Size(pList);
        int i = 0;
        for ( ; i < tSize; i++) {
             PyObject* item = PyList_GetItem(pList, (Py_ssize_t) i);
            if (PyUnicode_Check(item)) {
                std::string s = PyUnicode_AsUTF8(item);       //py3 only!!!
                temp.push_back(s);
            }
        }
        cl->m_verts = temp;
    } else {
        Base::Console().Error("CLPI::setPoints - input not a list!\n");
    }
}

PyObject *CenterLinePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int CenterLinePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
