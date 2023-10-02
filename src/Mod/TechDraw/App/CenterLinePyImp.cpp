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
#include <Base/PyWrapParseTupleAndKeywords.h>

#include "CenterLinePy.h"
#include "DrawUtil.h"


using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string CenterLinePy::representation() const
{
    std::stringstream sStream;
    sStream << "<CenterLine object> at " << std::hex << this;
    return sStream.str();
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
        PyErr_SetString(PyExc_RuntimeError, "failed to create clone of CenterLine");
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
        PyErr_SetString(PyExc_RuntimeError, "failed to create copy of CenterLine");
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

Py::Dict CenterLinePy::getFormat() const
{
    TechDraw::LineFormat* format= &(this->getCenterLinePtr()->m_format);
    Py::Dict dict;

    dict.setItem("style", Py::Long(format->m_style));
    dict.setItem("weight", Py::Float(format->m_weight));
    dict.setItem("color", Py::Tuple(DrawUtil::colorToPyTuple(format->m_color), true));
    dict.setItem("visible", Py::Boolean(format->m_visible));

    return dict;
}

void CenterLinePy::setFormat(Py::Dict arg)
{
    Py::Tuple dummy;
    Py::TupleN color(Py::Float(0.0), Py::Float(0.0), Py::Float(0.0), Py::Float(0.0));
    int style = 1;
    double weight = 0.5;
    PyObject* pColor = color.ptr();
    PyObject* visible = Py_True;
    static const std::array<const char *, 5> kw{"style", "weight", "color", "visible", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(dummy.ptr(), arg.ptr(), "|idO!O!", kw,
        &style, &weight, &PyTuple_Type, &pColor, &PyBool_Type, &visible)) {
        throw Py::ValueError("Expected {'style':int, 'weight':float, 'color':tuple, 'visible':bool} dict");
    }

    TechDraw::LineFormat* format = &(this->getCenterLinePtr()->m_format);
    format->m_style = style;
    format->m_weight = weight;
    format->m_color = DrawUtil::pyTupleToColor(pColor);
    format->m_visible = Base::asBoolean(visible);
}

Py::String CenterLinePy::getTag() const
{
    std::string tmp = boost::uuids::to_string(getCenterLinePtr()->getTag());
    return Py::String(tmp);
}


Py::Long CenterLinePy::getType() const
{
    int tmp = getCenterLinePtr()->m_type;
    return Py::Long(tmp);
}

Py::Long CenterLinePy::getMode() const
{
    int tmp = getCenterLinePtr()->m_mode;
    return Py::Long(tmp);
}

void CenterLinePy::setMode(Py::Long arg)
{
    int temp = static_cast<int>(arg);
    getCenterLinePtr()->m_mode = temp;
}

Py::Float CenterLinePy::getHorizShift() const
{
    double shift = getCenterLinePtr()->getHShift();
    return  Py::Float(shift);
}

void CenterLinePy::setHorizShift(Py::Float arg)
{
    double hshift =  static_cast<double>(arg);
    double vshift = getCenterLinePtr()->getVShift();
    getCenterLinePtr()->setShifts(hshift, vshift);
}

Py::Float CenterLinePy::getVertShift() const
{
    double shift = getCenterLinePtr()->getVShift();
    return  Py::Float(shift);
}

void CenterLinePy::setVertShift(Py::Float arg)
{
    double vshift =  static_cast<double>(arg);
    double hshift = getCenterLinePtr()->getHShift();
    getCenterLinePtr()->setShifts(hshift, vshift);
}

Py::Float CenterLinePy::getRotation() const
{
    double rot = getCenterLinePtr()->getRotate();
    return  Py::Float(rot);
}

void CenterLinePy::setRotation(Py::Float arg)
{
    double rot = static_cast<double>(arg);
    getCenterLinePtr()->setRotate(rot);
}

Py::Float CenterLinePy::getExtension() const
{
    double rot = getCenterLinePtr()->getExtend();
    return  Py::Float(rot);
}

void CenterLinePy::setExtension(Py::Float arg)
{
    double ext = static_cast<double>(arg);
    getCenterLinePtr()->setExtend(ext);
}

Py::Boolean CenterLinePy::getFlip() const
{
    bool flip = getCenterLinePtr()->getFlip();
    return Py::Boolean(flip);
}

void CenterLinePy::setFlip(Py::Boolean arg)
{
    PyObject* pObj = arg.ptr();
    if (PyBool_Check(pObj)) {
        getCenterLinePtr()->setFlip(Base::asBoolean(pObj));
    }
    else {
        std::string error = "Type must be bool, not " + std::string(Py_TYPE(pObj)->tp_name);
        throw Py::TypeError(error);
    }
}

// Helper functions to set/get geometries

static std::vector<std::string> setGeom(const Py::List& arg)
{
    std::vector<std::string> temp;
    for (const auto& it: arg) {
        if (PyUnicode_Check(it.ptr())) {
            temp.push_back(PyUnicode_AsUTF8(it.ptr()));
        }
        else {
            std::string error = "Type in list must be str, not " + std::string(Py_TYPE(it.ptr())->tp_name);
            throw Py::TypeError(error);
        }
    }
    return  temp;
}

static Py::List getGeom(const std::vector<std::string>& arg)
{
    Py::List result;

    for (const auto& name : arg) {
        result.append(Py::String(name));
    }

    return result;
}

Py::List CenterLinePy::getEdges() const
{
    return getGeom(this->getCenterLinePtr()->m_edges);
}

void CenterLinePy::setEdges(Py::List arg)
{
    TechDraw::CenterLine* cLine= this->getCenterLinePtr();
    cLine->m_edges = setGeom(arg);
}

Py::List CenterLinePy::getFaces() const
{
    return getGeom(this->getCenterLinePtr()->m_faces);
}

void CenterLinePy::setFaces(Py::List arg)
{
    TechDraw::CenterLine* cLine= this->getCenterLinePtr();
    cLine->m_faces = setGeom(arg);
}

Py::List CenterLinePy::getPoints() const
{
    return getGeom(this->getCenterLinePtr()->m_verts);
}

void CenterLinePy::setPoints(Py::List arg)
{
    TechDraw::CenterLine* cLine= this->getCenterLinePtr();
    cLine->m_verts = setGeom(arg);
}

PyObject *CenterLinePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int CenterLinePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

#include "CenterLinePy.cpp"
