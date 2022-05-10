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
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <gp_Circ.hxx>
#include <Geom_Circle.hxx>


#include <App/Material.h>

#include <Base/Console.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include "DrawUtil.h"
#include "Geometry.h"
#include "Cosmetic.h"
#include "CosmeticEdgePy.h"
#include "CosmeticEdgePy.cpp"

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string CosmeticEdgePy::representation(void) const
{
    std::stringstream ss;
    ss << "<CosmeticEdge object> at " << std::hex << this;
    return ss.str();
//    return "<CosmeticEdge object>";
}

PyObject *CosmeticEdgePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'CosmeticEdge'.");
    return nullptr;
}

// constructor method
int CosmeticEdgePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

//From Part::GeometryPy.cpp
PyObject* CosmeticEdgePy::clone(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TechDraw::CosmeticEdge* geom = this->getCosmeticEdgePtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = nullptr;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, nullptr);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create clone of CosmeticEdge");
        return nullptr;
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
        return nullptr;

    TechDraw::CosmeticEdge* geom = this->getCosmeticEdgePtr();
    PyTypeObject* type = this->GetType();
    PyObject* cpy = nullptr;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, nullptr);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of CosmeticEdge");
        return nullptr;
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

void CosmeticEdgePy::setFormat(Py::Object arg)
{
    PyObject* pTuple = arg.ptr();
    int style = 1;
    double weight = 0.50;
    double red = 0.0, green = 0.0, blue = 0.0, alpha = 0.0;
    App::Color c(red, blue, green, alpha);
    bool visible = 1; 
    
    TechDraw::CosmeticEdge* ce = this->getCosmeticEdgePtr();
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

            ce->m_format.m_style = style;
            ce->m_format.m_weight = weight;
            ce->m_format.m_color = c;
            ce->m_format.m_visible = visible;
        }
    } else {
        Base::Console().Error("CEPI::setFormat - not a tuple!\n");
    }
}

Py::Object CosmeticEdgePy::getFormat(void) const
{
    TechDraw::CosmeticEdge* ce = this->getCosmeticEdgePtr();

    PyObject* pStyle = PyLong_FromLong((long) ce->m_format.m_style);
    PyObject* pWeight = PyFloat_FromDouble(ce->m_format.m_weight);
    PyObject* pColor = DrawUtil::colorToPyTuple(ce->m_format.m_color);
    PyObject* pVisible = PyBool_FromLong((long) ce->m_format.m_visible);

    PyObject* result = PyTuple_New(4);

    PyTuple_SET_ITEM(result, 0, pStyle);
    PyTuple_SET_ITEM(result, 1, pWeight);
    PyTuple_SET_ITEM(result, 2, pColor);
    PyTuple_SET_ITEM(result, 3, pVisible);

    return Py::asObject(result);
}

Py::String CosmeticEdgePy::getTag(void) const
{
    std::string tmp = boost::uuids::to_string(getCosmeticEdgePtr()->getTag());
    return Py::String(tmp);
}

//Py::String CosmeticEdgePy::getOwner(void) const
//{
////    std::string tmp = boost::uuids::to_string(getCosmeticEdgePtr()->getOwner());
//    std::string tmp = "not implemented yet";
//    return Py::String(tmp);
//}

//TODO: make BaseGeom py-aware or convert TD geometry to ??Part::Geometry2d?? or other
//      py-aware class.
//Py::Object CosmeticEdgePy::getGeometry(void) const
//{
////    TechDraw::BaseGeomPtr bg = getCosmeticEdgePtr()->m_geometry;
//    Base::Console().Message("Not implemented yet");
//    return Py::asObject(Py_None);
//}

//void CosmeticEdgePy::setGeometry(Py::Object arg)
//{
//    Base::Console().Message("Not implemented yet");
//    PyObject* p = arg.ptr();
//    if (PyObject_TypeCheck(p, &(TechDraw::BaseGeomPy::Type))) {
//        //TODO
//    } else {
//        std::string error = std::string("type must be 'BaseGeom', not ");
//        error += p->ob_type->tp_name;
//        throw Py::TypeError(error);
//    }
//}

Py::Object CosmeticEdgePy::getStart(void) const
{
    Base::Vector3d point = getCosmeticEdgePtr()->permaStart;
    point = DrawUtil::invertY(point);
    return Py::asObject(new Base::VectorPy(point));
}

void CosmeticEdgePy::setStart(Py::Object arg)
{
    PyObject* p = arg.ptr();
    Base::Vector3d pNew;
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        pNew = static_cast<Base::VectorPy*>(p)->value();
    }
    else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        pNew = Base::getVectorFromTuple<double>(p);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    pNew = DrawUtil::invertY(pNew);
    Base::Vector3d pEnd = getCosmeticEdgePtr()->permaEnd;
    gp_Pnt gp1(pNew.x,pNew.y,pNew.z);
    gp_Pnt gp2(pEnd.x,pEnd.y,pEnd.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
//    auto oldGeom = getCosmeticEdgePtr()->m_geometry;
    getCosmeticEdgePtr()->m_geometry = TechDraw::BaseGeom::baseFactory(e);
    getCosmeticEdgePtr()->permaStart = pNew;
//    delete oldGeom;
}

Py::Object CosmeticEdgePy::getEnd(void) const
{
    Base::Vector3d point = getCosmeticEdgePtr()->permaEnd;
    point = DrawUtil::invertY(point);
    return Py::asObject(new Base::VectorPy(point));
}

void CosmeticEdgePy::setEnd(Py::Object arg)
{
    PyObject* p = arg.ptr();
    Base::Vector3d pNew;
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        pNew = static_cast<Base::VectorPy*>(p)->value();
    }
    else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        pNew = Base::getVectorFromTuple<double>(p);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    pNew = DrawUtil::invertY(pNew);
    Base::Vector3d pStart = getCosmeticEdgePtr()->permaStart;
    gp_Pnt gp1(pNew.x,pNew.y,pNew.z);
    gp_Pnt gp2(pStart.x,pStart.y,pStart.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp2, gp1);
//    auto oldGeom = getCosmeticEdgePtr()->m_geometry;
    getCosmeticEdgePtr()->m_geometry = TechDraw::BaseGeom::baseFactory(e);
    getCosmeticEdgePtr()->permaEnd = pNew;
//    delete oldGeom;
}

Py::Object CosmeticEdgePy::getRadius(void) const
{
    TechDraw::GeomType gt = getCosmeticEdgePtr()->m_geometry->geomType;
    if ( (gt != TechDraw::GeomType::CIRCLE) &&
         (gt != TechDraw::GeomType::ARCOFCIRCLE) ) {
        std::string error = "not a circle. Can not set radius";
        throw Py::TypeError(error);
    }
    double r = getCosmeticEdgePtr()->permaRadius;
    return Py::asObject(PyFloat_FromDouble(r));
}

void CosmeticEdgePy::setRadius(Py::Object arg)
{
    TechDraw::GeomType gt = getCosmeticEdgePtr()->m_geometry->geomType;
    PyObject* p = arg.ptr();
    if ( (gt != TechDraw::GeomType::CIRCLE) &&
         (gt != TechDraw::GeomType::ARCOFCIRCLE) ) {
        std::string error = std::string(p->ob_type->tp_name);
        error += " is not a circle. Can not set radius";
        throw Py::TypeError(error);
    }

    double r;
    if (PyObject_TypeCheck(p, &PyFloat_Type)) {
        r = PyFloat_AsDouble(p);
    }
    else if (PyObject_TypeCheck(p, &PyLong_Type)) {
        r = (double) PyLong_AsLong(p);
    }
    else {
        std::string error = std::string("type must be 'Float' or 'Int', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    getCosmeticEdgePtr()->permaRadius = r;
//    auto oldGeom = getCosmeticEdgePtr()->m_geometry;
    getCosmeticEdgePtr()->m_geometry =
                std::make_shared<TechDraw::Circle> (getCosmeticEdgePtr()->permaStart, r);
//    delete oldGeom;
}

Py::Object CosmeticEdgePy::getCenter(void) const
{
    TechDraw::GeomType gt = getCosmeticEdgePtr()->m_geometry->geomType;
    if ( (gt != TechDraw::GeomType::CIRCLE) &&
         (gt != TechDraw::GeomType::ARCOFCIRCLE) ) {
        std::string error = "not a circle. Can not get center";
        throw Py::TypeError(error);
    }
    Base::Vector3d point = getCosmeticEdgePtr()->permaStart;
    point = DrawUtil::invertY(point);
    return Py::asObject(new Base::VectorPy(point));
}

void CosmeticEdgePy::setCenter(Py::Object arg)
{
    TechDraw::GeomType gt = getCosmeticEdgePtr()->m_geometry->geomType;
    PyObject* p = arg.ptr();
    if ( (gt != TechDraw::GeomType::CIRCLE) &&
         (gt != TechDraw::GeomType::ARCOFCIRCLE) ) {
        std::string error = std::string(p->ob_type->tp_name);
        error += " is not a circle. Can not set center";
        throw Py::TypeError(error);
    }

//    PyObject* p = arg.ptr();
    Base::Vector3d pNew;
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        pNew = static_cast<Base::VectorPy*>(p)->value();
    }
    else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        pNew = Base::getVectorFromTuple<double>(p);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    pNew = DrawUtil::invertY(pNew);
    auto oldGeom = getCosmeticEdgePtr()->m_geometry;
    TechDraw::CirclePtr oldCircle = std::dynamic_pointer_cast<TechDraw::Circle> (oldGeom);
    if (oldCircle == nullptr) {
        throw Py::TypeError("Edge geometry is not a circle");
    }

    getCosmeticEdgePtr()->permaStart = pNew;
    getCosmeticEdgePtr()->permaEnd = pNew;
    getCosmeticEdgePtr()->permaRadius = oldCircle->radius;
    getCosmeticEdgePtr()->m_geometry =
            std::make_shared<TechDraw::Circle> (getCosmeticEdgePtr()->permaStart, oldCircle->radius);
//    delete oldGeom;
}

PyObject *CosmeticEdgePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int CosmeticEdgePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

