/***************************************************************************
 *   Copyright (c) 2019 WandererFan (wandererfan@gmail.com)                *
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

#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <Geom_Circle.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/GeometryPyCXX.h>
#include <Base/PyObjectBase.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

#include "DrawViewPart.h"
#include "GeometryObject.h"
#include "Cosmetic.h"
#include "DrawUtil.h"
#include "GeometryObject.h"

// inclusion of the generated files (generated out of DrawViewPartPy.xml)
#include <Mod/TechDraw/App/CosmeticVertexPy.h>
#include <Mod/TechDraw/App/CosmeticEdgePy.h>
#include <Mod/TechDraw/App/CenterLinePy.h>
#include <Mod/TechDraw/App/DrawViewPartPy.h>
#include <Mod/TechDraw/App/DrawViewPartPy.cpp>

using namespace TechDraw;

//TODO: errors to PyErrors

// returns a string which represents the object e.g. when printed in python
std::string DrawViewPartPy::representation(void) const
{
    return std::string("<DrawViewPart object>");
}

PyObject* DrawViewPartPy::clearCosmeticVertices(PyObject *args)
{
    (void) args;
    DrawViewPart* item = getDrawViewPartPtr();
    item->clearCosmeticVertexes();
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* DrawViewPartPy::clearCosmeticEdges(PyObject *args)
{
    (void) args;
    DrawViewPart* item = getDrawViewPartPtr();
    item->clearCosmeticEdges();
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* DrawViewPartPy::clearCenterLines(PyObject *args)
{
    (void) args;
    DrawViewPart* item = getDrawViewPartPtr();
    item->clearCenterLines();
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* DrawViewPartPy::clearGeomFormats(PyObject *args)
{
    (void) args;
    DrawViewPart* item = getDrawViewPartPtr();
    item->clearGeomFormats();
    Py_INCREF(Py_None);
    return Py_None;
}


PyObject* DrawViewPartPy::makeCosmeticVertex(PyObject *args)
{
    PyObject* pPnt1 = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &pPnt1)) {
        throw Py::TypeError("expected (vector)");
    }

    DrawViewPart* item = getDrawViewPartPtr();
//    Base::Vector3d pnt1 = DrawUtil::invertY(static_cast<Base::VectorPy*>(pPnt1)->value());
    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    int idx = item->addCosmeticVertex(pnt1);
    return PyLong_FromLong(idx);
}

PyObject* DrawViewPartPy::makeCosmeticLine(PyObject *args)
{
    PyObject* pPnt1 = nullptr;
    PyObject* pPnt2 = nullptr;
    int style = LineFormat::getDefEdgeStyle();
    double weight = LineFormat::getDefEdgeWidth();
    App::Color defCol = LineFormat::getDefEdgeColor();
    PyObject* pColor = nullptr;

    if (!PyArg_ParseTuple(args, "O!O!|idO", &(Base::VectorPy::Type), &pPnt1,
                                        &(Base::VectorPy::Type), &pPnt2,
                                        &style, &weight,
                                        &pColor)) {
        throw Py::TypeError("expected (vector, vector,[style,weight,color])");
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    //points inverted in addCosmeticEdge(p1, p2)
    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    Base::Vector3d pnt2 = static_cast<Base::VectorPy*>(pPnt2)->value();
    int idx = dvp->addCosmeticEdge(pnt1, pnt2);
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdgeByIndex(idx);
    if (ce != nullptr) {
        ce->m_format.m_style = style;
        ce->m_format.m_weight = weight;
        if (pColor == nullptr) {
            ce->m_format.m_color = defCol;
        } else {
            ce->m_format.m_color = DrawUtil::pyTupleToColor(pColor);
        }
    } else {
        std::string msg = "DVPPI:makeCosmeticLine - line creation failed";
        Base::Console().Message("%s\n",msg.c_str());
        throw Py::RuntimeError(msg);
    }
    return PyLong_FromLong(idx);
}

PyObject* DrawViewPartPy::makeCosmeticCircle(PyObject *args)
{
    PyObject* pPnt1 = nullptr;
    double radius = 5.0;
    double angle1 = 0.0;
    double angle2 = 360.0;
    int style = LineFormat::getDefEdgeStyle();
    double weight = LineFormat::getDefEdgeWidth();
    App::Color defCol = LineFormat::getDefEdgeColor();
    PyObject* pColor = nullptr;

    if (!PyArg_ParseTuple(args, "O!d|idO", &(Base::VectorPy::Type), &pPnt1,
                                        &radius,
                                        &style, &weight,
                                        &pColor)) {
        throw Py::TypeError("expected (vector, vector,[style,weight,color])");
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    Base::Vector3d pnt1 = DrawUtil::invertY(static_cast<Base::VectorPy*>(pPnt1)->value());
    gp_Pnt loc(pnt1.x, pnt1.y, pnt1.z);
    gp_Dir dir(0,0,1);
    gp_Ax1 axis(loc, dir);
    gp_Circ circle;
    circle.SetAxis(axis);
    circle.SetRadius(radius);

    Handle(Geom_Circle) hCircle = new Geom_Circle (circle);
    BRepBuilderAPI_MakeEdge aMakeEdge(hCircle, angle1*(M_PI/180), angle2*(M_PI/180));
    TopoDS_Edge edge = aMakeEdge.Edge();
    int idx = dvp->addCosmeticEdge(edge);
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdgeByIndex(idx);
    if (ce != nullptr) {
        ce->m_format.m_style = style;
        ce->m_format.m_weight = weight;
        if (pColor == nullptr) {
            ce->m_format.m_color = defCol;
        } else {
            ce->m_format.m_color = DrawUtil::pyTupleToColor(pColor);
        }
    } else {
        std::string msg = "DVPPI:makeCosmeticCircle - circle creation failed";
        Base::Console().Message("%s\n",msg.c_str());
        throw Py::RuntimeError(msg);
    }
    return PyLong_FromLong(idx);
}

PyObject* DrawViewPartPy::makeCosmeticCircleArc(PyObject *args)
{
    PyObject* pPnt1 = nullptr;
    double radius = 5.0;
    double angle1 = 0.0;
    double angle2 = 360.0;
    int style = LineFormat::getDefEdgeStyle();
    double weight = LineFormat::getDefEdgeWidth();
    App::Color defCol = LineFormat::getDefEdgeColor();
    PyObject* pColor = nullptr;

    if (!PyArg_ParseTuple(args, "O!ddd|idO", &(Base::VectorPy::Type), &pPnt1,
                                        &radius, &angle1, &angle2,
                                        &style, &weight, &pColor)) {
        throw Py::TypeError("expected (vector, radius, start, end,[style, weight, color])");
    }

    //from here on is almost duplicate of makeCosmeticCircle
    DrawViewPart* dvp = getDrawViewPartPtr();
    Base::Vector3d pnt1 = DrawUtil::invertY(static_cast<Base::VectorPy*>(pPnt1)->value());
    gp_Pnt loc(pnt1.x, pnt1.y, pnt1.z);
    gp_Dir dir(0,0,1);
    gp_Ax1 axis(loc, dir);
    gp_Circ circle;
    circle.SetAxis(axis);
    circle.SetRadius(radius);      //full circle @ right loc
    Handle(Geom_Circle) hCircle = new Geom_Circle (circle);
    BRepBuilderAPI_MakeEdge aMakeEdge(hCircle, -angle2*(M_PI/180), -angle1*(M_PI/180)); //hack!
    // right result, but ugly:
    // Qt angles are cw, OCC angles are CCW
    // Qt -y is up, OCC -y is down
    
    TopoDS_Edge edge = aMakeEdge.Edge();
    int idx = dvp->addCosmeticEdge(edge);
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdgeByIndex(idx);
    if (ce != nullptr) {
        ce->m_format.m_style = style;
        ce->m_format.m_weight = weight;
        if (pColor == nullptr) {
            ce->m_format.m_color = defCol;
        } else {
            ce->m_format.m_color = DrawUtil::pyTupleToColor(pColor);
        }
    } else {
        std::string msg = "DVPPI:makeCosmeticCircleArc - arc creation failed";
        Base::Console().Message("%s\n",msg.c_str());
        throw Py::RuntimeError(msg);
    }
    return PyLong_FromLong(idx);
}

PyObject* DrawViewPartPy::getCosmeticVertexByIndex(PyObject *args)
{
    PyObject* result = nullptr;
    int idx = -1;
    if (!PyArg_ParseTuple(args, "i", &idx)) {
        throw Py::TypeError("expected (index)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::CosmeticVertex* cv = dvp->getCosmeticVertexByIndex(idx);
    if (cv != nullptr) {
        result = new CosmeticVertexPy(new CosmeticVertex(cv));
    } else {
        result = Py_None;
    }
    return result;
}

PyObject* DrawViewPartPy::removeCosmeticVertex(PyObject *args)
{
    int idx = 0;
    if (!PyArg_ParseTuple(args, "i", &idx)) {
        throw Py::TypeError("expected (index)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();
    dvp->removeCosmeticVertex(idx);
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* DrawViewPartPy::getCosmeticEdgeByIndex(PyObject *args)
{
    int idx = 0;
    PyObject* result = Py_None;
    if (!PyArg_ParseTuple(args, "i", &idx)) {
        throw Py::TypeError("expected (index)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdgeByIndex(idx);
    if (ce != nullptr) {
        result = new CosmeticEdgePy(new CosmeticEdge(ce));
    } else {
        Base::Console().Error("DVPPI::getCosEdgebyIdx - edge %d not found\n", idx);
    }

    return result;
}

PyObject* DrawViewPartPy::getCosmeticEdgeByGeom(PyObject *args)
{
//    Base::Console().Message("DVPPI::getCosmeticEdgeByGeom()\n");
    int idx = 0;
    PyObject* result = Py_None;
    if (!PyArg_ParseTuple(args, "i", &idx)) {
        throw Py::TypeError("expected (index)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();
    
    TechDraw::BaseGeom* bg = dvp->getGeomByIndex(idx);
    if (bg == nullptr) {
        Base::Console().Error("DVPPI::getCEbyGeom - geom: %d not found\n",idx);
        return result;
    }
    int source = bg->source();
    int sourceIndex = bg->sourceIndex();
    if (source == 1) {           //cosmetic edge
        TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdgeByIndex(sourceIndex);
        if (ce != nullptr) {
            result = new CosmeticEdgePy(new CosmeticEdge(ce));
        } else {
            Base::Console().Error("DVPPI::getCosEdgebyGeom - edge %d not found\n", idx);
        }
    }
    return result;
}

PyObject* DrawViewPartPy::replaceCosmeticEdge(PyObject *args)
{
//    Base::Console().Message("DVPPI::replaceCosmeticEdge()\n");
    int idx = 0;
    PyObject* result = Py_None;
    PyObject* pCE;
    if (!PyArg_ParseTuple(args, "iO!", &idx, &(TechDraw::CosmeticEdgePy::Type), &pCE)) {
        throw Py::TypeError("expected (index, CosmeticEdge)");
    }
    TechDraw::CosmeticEdge* ce = static_cast<CosmeticEdgePy*>(pCE)->getCosmeticEdgePtr();
    DrawViewPart* dvp = getDrawViewPartPtr();
    dvp->replaceCosmeticEdge(idx, ce);

    return result;
}

PyObject* DrawViewPartPy::removeCosmeticEdge(PyObject *args)
{
//    Base::Console().Message("DVPPI::removeCosEdge()\n");
    int idx = 0;
    if (!PyArg_ParseTuple(args, "i", &idx)) {
        throw Py::TypeError("expected (index)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();
    dvp->removeCosmeticEdge(idx);
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* DrawViewPartPy::makeCenterLine(PyObject *args)
{
//    Base::Console().Message("DVPPI::makeCenterLine()\n");
    PyObject* pSubs;
    int mode = 0;
    std::vector<std::string> subs;

    if (!PyArg_ParseTuple(args, "Oi",&pSubs, &mode)) {
        throw Py::TypeError("expected (subNameList, mode)");
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    if (PyList_Check(pSubs)) {
        int size = PyList_Size(pSubs);
        int i = 0;
        for ( ; i < size; i++) {
            PyObject* po = PyList_GetItem(pSubs, i);
#if PY_MAJOR_VERSION >= 3
            if (PyUnicode_Check(po)) {
                std::string s = PyUnicode_AsUTF8(po);       //py3 only!!!
                subs.push_back(s);
            }
#else
            if (PyString_Check(po)) {
                std::string s = PyString_AsString(po);         //py2 only!!!
                subs.push_back(s);
            }
#endif
        }
    }

    CenterLine* cl = nullptr;
    int idx = -1;
    if (!subs.empty()) {
        cl = CenterLine::CenterLineBuilder(dvp,
                                           subs,
                                           mode);     //vert,horiz,align
        if (cl != nullptr) {
            idx = dvp->addCenterLine(cl);
        } else {
            std::string msg = "DVPPI:makeCenterLine - line creation failed";
            Base::Console().Message("%s\n",msg.c_str());
            throw Py::RuntimeError(msg);
        }
    }
    return PyLong_FromLong(idx);
}

PyObject* DrawViewPartPy::adjustCenterLine(PyObject *args)
{
//    Base::Console().Message("DVPPI::adjustCenterLine()\n");
    int idx = -1;
    double hShift = 0.0;
    double vShift = 0.0;
    double rotate = 0.0;
    double extend = 0.0;
    bool flip = false;

    if (!PyArg_ParseTuple(args, "idddd|p",&idx, &hShift, &vShift, &rotate, &extend, &flip)) {
        throw Py::TypeError("expected (index, hShift, vShift, rotate, extend [,flip])");
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    CenterLine* cl = dvp->getCenterLineByIndex(idx);
    if (cl != nullptr) {
        cl->m_hShift = hShift;
        cl->m_vShift = vShift;
        cl->m_rotate = rotate;
        cl->m_extendBy = extend;
        cl->m_flip2Line = flip;
    } else {
        std::string msg = "DVPPI:adjustCenterLine - CenterLine not found";
        Base::Console().Message("%s\n",msg.c_str());
        throw Py::RuntimeError(msg);
    }
    return Py_None;
}

PyObject* DrawViewPartPy::formatCenterLine(PyObject *args)
{
//    Base::Console().Message("DVPPI::formatCenterLine()\n");
    int idx = -1;
    int style = Qt::SolidLine;
    App::Color defColor = LineFormat::getDefEdgeColor();
    double weight = 0.5;
    int visible = 1;
    PyObject* pColor;

    if (!PyArg_ParseTuple(args, "iidOi",&idx, &style, &weight, &pColor, &visible)) {
        throw Py::TypeError("expected (index, style, weight, color, visible)");
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    CenterLine* cl = dvp->getCenterLineByIndex(idx);
    if (cl != nullptr) {
        cl->m_format.m_style = style;
        cl->m_format.m_weight = weight;
        if (pColor == nullptr) {
            cl->m_format.m_color = defColor;
        } else {
            cl->m_format.m_color = DrawUtil::pyTupleToColor(pColor);
        }
        cl->m_format.m_visible = visible;
    } else {
        std::string msg = "DVPPI:formatCenterLine - CenterLine not found";
        Base::Console().Message("%s\n",msg.c_str());
        throw Py::RuntimeError(msg);
    }
    return Py_None;
}

PyObject* DrawViewPartPy::formatGeometricEdge(PyObject *args)
{
//    Base::Console().Message("DVPPI::formatGeometricEdge()\n");
    int idx = -1;
    int style = Qt::SolidLine;
    App::Color color = LineFormat::getDefEdgeColor();
    double weight = 0.5;
    int visible = 1;
    PyObject* pColor;

    if (!PyArg_ParseTuple(args, "iidOi",&idx, &style, &weight, &pColor, &visible)) {
        throw Py::TypeError("expected (index, style, weight, color, visible)");
    }

    color = DrawUtil::pyTupleToColor(pColor);
    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::GeomFormat* gf = dvp->getGeomFormatByGeom(idx);
    if (gf != nullptr) {
        gf->m_format.m_style = style;
        gf->m_format.m_color = color;
        gf->m_format.m_weight = weight;
        gf->m_format.m_visible = visible;
    } else {
        TechDraw::LineFormat fmt(style,
                                 weight,
                                 color,
                                 visible);
        TechDraw::GeomFormat* newGF = new TechDraw::GeomFormat(idx,
                                                               fmt);
//                    int idx = 
        dvp->addGeomFormat(newGF);
    }
    return Py_None;
}


PyObject *DrawViewPartPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawViewPartPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

