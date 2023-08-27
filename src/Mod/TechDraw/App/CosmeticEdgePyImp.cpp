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
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <boost/uuid/uuid_io.hpp>
#endif

#include <Base/PyWrapParseTupleAndKeywords.h>

#include "CosmeticEdgePy.h"
#include "CosmeticEdgePy.cpp"
#include "Cosmetic.h"
#include "DrawUtil.h"
#include "Geometry.h"


using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string CosmeticEdgePy::representation() const
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
        PyErr_SetString(PyExc_RuntimeError, "failed to create clone of CosmeticEdge");
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
        PyErr_SetString(PyExc_RuntimeError, "failed to create copy of CosmeticEdge");
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

void CosmeticEdgePy::setFormat(Py::Dict arg)
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

    TechDraw::LineFormat* format = &(this->getCosmeticEdgePtr()->m_format);
    format->m_style = style;
    format->m_weight = weight;
    format->m_color = DrawUtil::pyTupleToColor(pColor);
    format->m_visible = Base::asBoolean(visible);
}

Py::Dict CosmeticEdgePy::getFormat() const
{
    TechDraw::LineFormat* format= &(this->getCosmeticEdgePtr()->m_format);
    Py::Dict dict;

    dict.setItem("style", Py::Long(format->m_style));
    dict.setItem("weight", Py::Float(format->m_weight));
    dict.setItem("color", Py::Tuple(DrawUtil::colorToPyTuple(format->m_color), true));
    dict.setItem("visible", Py::Boolean(format->m_visible));

    return dict;
}

Py::String CosmeticEdgePy::getTag() const
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

Py::Vector CosmeticEdgePy::getStart() const
{
    Base::Vector3d point = getCosmeticEdgePtr()->permaStart;
    point = DrawUtil::invertY(point);
    return Py::Vector(point);
}

void CosmeticEdgePy::setStart(Py::Vector arg)
{
    Base::Vector3d pNew = static_cast<Base::Vector3d>(arg);

    pNew = DrawUtil::invertY(pNew);
    Base::Vector3d pEnd = getCosmeticEdgePtr()->permaEnd;
    gp_Pnt gp1(pNew.x, pNew.y, pNew.z);
    gp_Pnt gp2(pEnd.x, pEnd.y, pEnd.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
//    auto oldGeom = getCosmeticEdgePtr()->m_geometry;
    getCosmeticEdgePtr()->m_geometry = TechDraw::BaseGeom::baseFactory(e);
    getCosmeticEdgePtr()->permaStart = pNew;
//    delete oldGeom;
}

Py::Vector CosmeticEdgePy::getEnd() const
{
    Base::Vector3d point = getCosmeticEdgePtr()->permaEnd;
    point = DrawUtil::invertY(point);
    return Py::Vector(point);
}

void CosmeticEdgePy::setEnd(Py::Vector arg)
{
    Base::Vector3d pNew = static_cast<Base::Vector3d>(arg);

    pNew = DrawUtil::invertY(pNew);
    Base::Vector3d pStart = getCosmeticEdgePtr()->permaStart;
    gp_Pnt gp1(pNew.x, pNew.y, pNew.z);
    gp_Pnt gp2(pStart.x, pStart.y, pStart.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp2, gp1);
//    auto oldGeom = getCosmeticEdgePtr()->m_geometry;
    getCosmeticEdgePtr()->m_geometry = TechDraw::BaseGeom::baseFactory(e);
    getCosmeticEdgePtr()->permaEnd = pNew;
//    delete oldGeom;
}

Py::Float CosmeticEdgePy::getRadius() const
{
    TechDraw::GeomType gt = getCosmeticEdgePtr()->m_geometry->getGeomType();
    if ( (gt != TechDraw::GeomType::CIRCLE) &&
         (gt != TechDraw::GeomType::ARCOFCIRCLE) ) {
        throw Py::TypeError("Not a circle. Can not get radius");
    }
    double r = getCosmeticEdgePtr()->permaRadius;
    return Py::Float(r);
}

void CosmeticEdgePy::setRadius(Py::Float arg)
{
    TechDraw::GeomType gt = getCosmeticEdgePtr()->m_geometry->getGeomType();
    if ( (gt != TechDraw::GeomType::CIRCLE) &&
         (gt != TechDraw::GeomType::ARCOFCIRCLE) ) {
        throw Py::TypeError("Not a circle. Can not set radius");
    }

    double r = static_cast<double>(arg);
    getCosmeticEdgePtr()->permaRadius = r;
//    auto oldGeom = getCosmeticEdgePtr()->m_geometry;
    getCosmeticEdgePtr()->m_geometry =
                std::make_shared<TechDraw::Circle> (getCosmeticEdgePtr()->permaStart, r);
//    delete oldGeom;
}

Py::Vector CosmeticEdgePy::getCenter() const
{
    TechDraw::GeomType gt = getCosmeticEdgePtr()->m_geometry->getGeomType();
    if ( (gt != TechDraw::GeomType::CIRCLE) &&
         (gt != TechDraw::GeomType::ARCOFCIRCLE) ) {
        throw Py::TypeError("Not a circle. Can not get center");
    }
    Base::Vector3d point = getCosmeticEdgePtr()->permaStart;
    point = DrawUtil::invertY(point);
    return Py::Vector(point);
}

void CosmeticEdgePy::setCenter(Py::Vector arg)
{
    TechDraw::GeomType gt = getCosmeticEdgePtr()->m_geometry->getGeomType();
//    PyObject* p = arg.ptr();
    if ( (gt != TechDraw::GeomType::CIRCLE) &&
         (gt != TechDraw::GeomType::ARCOFCIRCLE) ) {
        throw Py::TypeError("Not a circle. Can not set center");
    }

    Base::Vector3d pNew = static_cast<Base::Vector3d>(arg);
    pNew = DrawUtil::invertY(pNew);
    auto oldGeom = getCosmeticEdgePtr()->m_geometry;
    TechDraw::CirclePtr oldCircle = std::dynamic_pointer_cast<TechDraw::Circle> (oldGeom);
    if (!oldCircle) {
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

