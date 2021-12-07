/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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
#include <BRepBuilderAPI_MakeVertex.hxx>
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

#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeEdgePy.h>
#include <Mod/Part/App/TopoShapeVertexPy.h>

#include "DrawViewPart.h"
#include "GeometryObject.h"
#include "Geometry.h"
#include "Cosmetic.h"
#include "CosmeticExtension.h"
#include "DrawUtil.h"

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
//TODO: gets & sets for geometry

PyObject* DrawViewPartPy::getVisibleEdges(PyObject *args)
{
    (void) args;
    DrawViewPart* dvp = getDrawViewPartPtr();
    Py::List pEdgeList;
    std::vector<TechDraw::BaseGeom*> geoms = dvp->getEdgeGeometry();
    for (auto& g: geoms) {
        if (g->hlrVisible) {
            PyObject* pEdge = new Part::TopoShapeEdgePy(new Part::TopoShape(g->occEdge));
            pEdgeList.append(Py::asObject(pEdge));
        }
    }

    return Py::new_reference_to(pEdgeList);
}

PyObject* DrawViewPartPy::getHiddenEdges(PyObject *args)
{
    (void) args;
    DrawViewPart* dvp = getDrawViewPartPtr();
    Py::List pEdgeList;
    std::vector<TechDraw::BaseGeom*> geoms = dvp->getEdgeGeometry();
    for (auto& g: geoms) {
        if (!g->hlrVisible) {
            PyObject* pEdge = new Part::TopoShapeEdgePy(new Part::TopoShape(g->occEdge));
            pEdgeList.append(Py::asObject(pEdge));
        }
    }

    return Py::new_reference_to(pEdgeList);
}

PyObject* DrawViewPartPy::requestPaint(PyObject *args)
{
    (void) args;
    DrawViewPart* item = getDrawViewPartPtr();
    item->requestPaint();
    Py_INCREF(Py_None);
    return Py_None;
}

// remove all cosmetics
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

//********* Cosmetic Vertex Routines *******************************************
PyObject* DrawViewPartPy::makeCosmeticVertex(PyObject *args)
{
    PyObject* pPnt1 = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &pPnt1)) {
        throw Py::TypeError("expected (vector)");
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    std::string dvpName = dvp->getNameInDocument();
    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    std::string id = dvp->addCosmeticVertex(pnt1);
    //int link =
    dvp->add1CVToGV(id);
    dvp->requestPaint();
    return PyUnicode_FromString(id.c_str());   //return tag for new CV
}

PyObject* DrawViewPartPy::makeCosmeticVertex3d(PyObject *args)
{
    PyObject* pPnt1 = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &pPnt1)) {
        throw Py::TypeError("expected (vector)");
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    Base::Vector3d centroid = dvp->getOriginalCentroid();
    pnt1 = pnt1 - centroid;
    Base::Vector3d projected = DrawUtil::invertY(dvp->projectPoint(pnt1));

    std::string id = dvp->addCosmeticVertex(projected);
    //int link =
    dvp->add1CVToGV(id);
    dvp->refreshCVGeoms();
    dvp->requestPaint();
    return PyUnicode_FromString(id.c_str());   //return tag for new CV
}

//get by unique tag
PyObject* DrawViewPartPy::getCosmeticVertex(PyObject *args)
{
    PyObject* result = nullptr;
    char* id;                      //unique tag
    if (!PyArg_ParseTuple(args, "s", &id)) {
        throw Py::TypeError("expected (string)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::CosmeticVertex* cv = dvp->getCosmeticVertex(id);
    if (cv != nullptr) {
        result = cv->getPyObject();
    } else {
        result = Py_None;
    }
    return result;
}

//get by selection name
PyObject* DrawViewPartPy::getCosmeticVertexBySelection(PyObject *args)
{
    PyObject* result = nullptr;
    char* selName;           //Selection routine name - "Vertex0"
    if (!PyArg_ParseTuple(args, "s", &selName)) {
        throw Py::TypeError("expected (string)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();

    TechDraw::CosmeticVertex* cv = dvp->getCosmeticVertexBySelection(selName);
    if (cv != nullptr) {
        result = cv->getPyObject();
    } else {
        result = Py_None;
    }
    return result;
}

PyObject* DrawViewPartPy::removeCosmeticVertex(PyObject *args)
{
    DrawViewPart* dvp = getDrawViewPartPtr();
    if (dvp == nullptr) {
        return Py_None;
    }

    char* tag;
    if (PyArg_ParseTuple(args, "s", &tag)) {
        dvp->removeCosmeticVertex(tag);
        dvp->refreshCVGeoms();
        dvp->requestPaint();
        return Py_None;
    }

    PyObject* pCVToDelete = nullptr;
    if (PyArg_ParseTuple(args, "O!", &(TechDraw::CosmeticVertexPy::Type), &pCVToDelete)) {
        TechDraw::CosmeticVertexPy* cvPy = static_cast<TechDraw::CosmeticVertexPy*>(pCVToDelete);
        TechDraw::CosmeticVertex* cv = cvPy->getCosmeticVertexPtr();
        dvp->removeCosmeticVertex(cv->getTagAsString());
        dvp->refreshCVGeoms();
        dvp->requestPaint();
        return Py_None;
    }

    PyObject* pDelList = nullptr;
    if (PyArg_ParseTuple(args, "O", &pDelList)) {
        if (PySequence_Check(pDelList))  {
            Py_ssize_t nSize = PySequence_Size(pDelList);
            for (Py_ssize_t i=0; i < nSize; i++) {
                PyObject* item = PySequence_GetItem(pDelList, i);
                if (!PyObject_TypeCheck(item, &(TechDraw::CosmeticVertexPy::Type)))  {
                    std::string error = std::string("types in list must be 'CosmeticVertex', not ");
                    error += item->ob_type->tp_name;
                    throw Base::TypeError(error);
                }
                TechDraw::CosmeticVertexPy* cvPy = static_cast<TechDraw::CosmeticVertexPy*>(item);
                TechDraw::CosmeticVertex* cv = cvPy->getCosmeticVertexPtr();
                dvp->removeCosmeticVertex(cv->getTagAsString());
            }
            dvp->refreshCVGeoms();
            dvp->requestPaint();
        }
    } else {
        throw Py::TypeError("expected (CosmeticVertex or [CosmeticVertex])");
    }
    return Py_None;
}

PyObject* DrawViewPartPy::replaceCosmeticVertex(PyObject *args)
{
    (void) args;
    Base::Console().Message("DVPP::replaceCosmeticVertex() - deprecated. do not use.\n");
    return PyBool_FromLong(0l);

}


//********* Cosmetic Line Routines *********************************************

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
    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    Base::Vector3d pnt2 = static_cast<Base::VectorPy*>(pPnt2)->value();
    std::string newTag = dvp->addCosmeticEdge(pnt1, pnt2);
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
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
    //int link =
    dvp->add1CEToGE(newTag);
    dvp->requestPaint();
    return PyUnicode_FromString(newTag.c_str());   //return tag for new CE
}

PyObject* DrawViewPartPy::makeCosmeticLine3D(PyObject *args)
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
    Base::Vector3d centroid = dvp->getOriginalCentroid();

    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    pnt1 = pnt1 - centroid;
    pnt1 = DrawUtil::invertY(dvp->projectPoint(pnt1));

    Base::Vector3d pnt2 = static_cast<Base::VectorPy*>(pPnt2)->value();
    pnt2 = pnt2 - centroid;
    pnt2 = DrawUtil::invertY(dvp->projectPoint(pnt2));

    std::string newTag = dvp->addCosmeticEdge(pnt1, pnt2);
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
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
    //int link =
    dvp->add1CEToGE(newTag);
    dvp->requestPaint();
    return PyUnicode_FromString(newTag.c_str());   //return tag for new CE
}
PyObject* DrawViewPartPy::makeCosmeticCircle(PyObject *args)
{
    PyObject* pPnt1 = nullptr;
    double radius = 5.0;
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
    TechDraw::BaseGeom* bg = new TechDraw::Circle(pnt1, radius);
    std::string newTag = dvp->addCosmeticEdge(bg);
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
    if (ce != nullptr) {
        ce->permaRadius = radius;
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
    //int link =
    dvp->add1CEToGE(newTag);
    dvp->requestPaint();
    return PyUnicode_FromString(newTag.c_str());   //return tag for new CE
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
    TechDraw::BaseGeom* bg = new TechDraw::AOC(pnt1, radius, angle1, angle2);
    std::string newTag = dvp->addCosmeticEdge(bg);
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
    if (ce != nullptr) {
        ce->permaRadius = radius;
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

    //int link =
    dvp->add1CEToGE(newTag);
    dvp->requestPaint();
    return PyUnicode_FromString(newTag.c_str());   //return tag for new CE
}

//********** Cosmetic Edge *****************************************************

PyObject* DrawViewPartPy::getCosmeticEdge(PyObject *args)
{
    char* tag;
    PyObject* result = Py_None;
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        throw Py::TypeError("expected (tag)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(tag);
    if (ce != nullptr) {
        result = ce->getPyObject();
    } else {
        Base::Console().Error("DVPPI::getCosmeticEdge - edge %s not found\n", tag);
    }

    return result;
}

PyObject* DrawViewPartPy::getCosmeticEdgeBySelection(PyObject *args)
{
//    Base::Console().Message("DVPPI::getCosmeticEdgeBySelection()\n");
    char* name;
    PyObject* result = Py_None;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        throw Py::TypeError("expected (name)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();

    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdgeBySelection(name);
    if (ce != nullptr) {
        result = ce->getPyObject();
    } else {
        Base::Console().Error("DVPPI::getCosmeticEdgebySelection - edge for name %s not found\n", name);
    }
    return result;
}

PyObject* DrawViewPartPy::replaceCosmeticEdge(PyObject *args)
{
    (void) args;
    Base::Console().Message("DVPP::replaceCosmeticEdge() - deprecated. do not use.\n");
    return PyBool_FromLong(0l);

//    Base::Console().Message("DVPPI::replaceCosmeticEdge()\n");
//    bool result = false;
//    PyObject* pNewCE;
//    if (!PyArg_ParseTuple(args, "O!", &(TechDraw::CosmeticEdgePy::Type), &pNewCE)) {
//        throw Py::TypeError("expected (CosmeticEdge)");
//    }
//    DrawViewPart* dvp = getDrawViewPartPtr();
//    TechDraw::CosmeticEdgePy* cePy = static_cast<TechDraw::CosmeticEdgePy*>(pNewCE);
//    TechDraw::CosmeticEdge* ce = cePy->getCosmeticEdgePtr();
//    if (ce != nullptr) {
//        result = dvp->replaceCosmeticEdge(ce);                 //<<<
//        dvp->refreshCEGeoms();
//        dvp->requestPaint();
//    }
//    return PyBool_FromLong((long) result);
}

PyObject* DrawViewPartPy::removeCosmeticEdge(PyObject *args)
{
//    Base::Console().Message("DVPPI::removeCosmeticEdge()\n");
    char* tag;
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        throw Py::TypeError("expected (tag)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();
    dvp->removeCosmeticEdge(tag);
    Py_INCREF(Py_None);
    return Py_None;
}

//********** Center Line *******************************************************

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
            if (PyUnicode_Check(po)) {
                std::string s = PyUnicode_AsUTF8(po);       //py3 only!!!
                subs.push_back(s);
            }
        }
    }
    CenterLine* cl = nullptr;
    std::string tag;
    if (!subs.empty()) {
        cl = CenterLine::CenterLineBuilder(dvp,
                                           subs,
                                           mode);     //vert,horiz,align
        if (cl != nullptr) {
            tag = dvp->addCenterLine(cl);
        } else {
            std::string msg = "DVPPI:makeCenterLine - line creation failed";
            Base::Console().Message("%s\n",msg.c_str());
            throw Py::RuntimeError(msg);
        }
    }
    //int link =
    dvp->add1CLToGE(tag);
    dvp->requestPaint();

    return PyUnicode_FromString(tag.c_str());   //return tag for new CV
}

PyObject* DrawViewPartPy::getCenterLine(PyObject *args)
{
    char* tag;
    PyObject* result = Py_None;
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        throw Py::TypeError("expected (tag)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::CenterLine* cl = dvp->getCenterLine(tag);
    if (cl != nullptr) {
        result = cl->getPyObject();
    } else {
        Base::Console().Error("DVPPI::getCenterLine - centerLine %s not found\n", tag);
    }

    return result;
}

PyObject* DrawViewPartPy::getCenterLineBySelection(PyObject *args)
{
//    Base::Console().Message("DVPPI::getCenterLineBySelection()\n");
    char* tag;
    PyObject* result = Py_None;
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        throw Py::TypeError("expected (name)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();

    TechDraw::CenterLine* cl = dvp->getCenterLineBySelection(tag);
    if (cl != nullptr) {
        result = cl->getPyObject();
    } else {
        Base::Console().Error("DVPPI::getCenterLinebySelection - centerLine for tag %s not found\n", tag);
    }
    return result;
}

PyObject* DrawViewPartPy::replaceCenterLine(PyObject *args)
{
    (void) args;
    Base::Console().Message("DVPP::replaceCenterLine() - deprecated. do not use.\n");
    return PyBool_FromLong(0l);

//    Base::Console().Message("DVPPI::replace CenterLine()\n");
//    PyObject* pNewCL;
//    if (!PyArg_ParseTuple(args, "O!", &(TechDraw::CenterLinePy::Type), &pNewCL)) {
//        throw Py::TypeError("expected (CenterLine)");
//    }
//    DrawViewPart* dvp = getDrawViewPartPtr();
//    TechDraw::CenterLinePy* clPy = static_cast<TechDraw::CenterLinePy*>(pNewCL);
//    TechDraw::CenterLine* cl = clPy->getCenterLinePtr();
//    bool result = dvp->replaceCenterLine(cl);
//    dvp->refreshCLGeoms();
//    dvp->requestPaint();
//    return PyBool_FromLong((long) result);
}

PyObject* DrawViewPartPy::removeCenterLine(PyObject *args)
{
//    Base::Console().Message("DVPPI::removeCenterLine()\n");
    char* tag;
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        throw Py::TypeError("expected (tag)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();
    dvp->removeCenterLine(tag);
    Py_INCREF(Py_None);
    return Py_None;
}

//********** Geometry Edge *****************************************************

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
    TechDraw::GeomFormat* gf = dvp->getGeomFormatBySelection(idx);
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

//------------------------------------------------------------------------------
PyObject* DrawViewPartPy::getEdgeByIndex(PyObject *args)
{
    int edgeIndex = 0;
    if (!PyArg_ParseTuple(args, "i", &edgeIndex)) {
        throw Py::TypeError("expected (edgeIndex)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();

    //this is scaled and +Yup
    //need unscaled and +Ydown
    TechDraw::BaseGeom* geom = dvp->getGeomByIndex(edgeIndex);
    if (geom == nullptr) {
        throw Py::ValueError("wrong edgeIndex");
    }

    TopoDS_Shape temp = TechDraw::mirrorShapeVec(geom->occEdge,
                                      Base::Vector3d(0.0, 0.0, 0.0),
                                      1.0 / dvp->getScale());

    TopoDS_Edge outEdge = TopoDS::Edge(temp);
    return new Part::TopoShapeEdgePy(new Part::TopoShape(outEdge));
}

PyObject* DrawViewPartPy::getVertexByIndex(PyObject *args)
{
    int vertexIndex = 0;
    if (!PyArg_ParseTuple(args, "i", &vertexIndex)) {
        throw Py::TypeError("expected (vertIndex)");
    }
    DrawViewPart* dvp = getDrawViewPartPtr();

    //this is scaled and +Yup
    //need unscaled and +Ydown
    TechDraw::VertexPtr vert = dvp->getProjVertexByIndex(vertexIndex);
    if (vert == nullptr) {
        throw Py::ValueError("wrong vertIndex");
    }
    Base::Vector3d point = DrawUtil::invertY(vert->point()) / dvp->getScale();

    gp_Pnt gPoint(point.x, point.y, point.z);
    BRepBuilderAPI_MakeVertex mkVertex(gPoint);
    TopoDS_Vertex outVertex = mkVertex.Vertex();
    return new Part::TopoShapeVertexPy(new Part::TopoShape(outVertex));
}

PyObject* DrawViewPartPy::getEdgeBySelection(PyObject *args)
{
    int edgeIndex = 0;
    char* selName;           //Selection routine name - "Edge0"
    if (!PyArg_ParseTuple(args, "s", &selName)) {
        throw Py::TypeError("expected (string)");
    }

    edgeIndex = DrawUtil::getIndexFromName(std::string(selName));
    DrawViewPart* dvp = getDrawViewPartPtr();

    //this is scaled and +Yup
    //need unscaled and +Ydown
    TechDraw::BaseGeom* geom = dvp->getGeomByIndex(edgeIndex);
    if (geom == nullptr) {
        throw Py::ValueError("wrong edgeIndex");
    }

    TopoDS_Shape temp = TechDraw::mirrorShapeVec(geom->occEdge,
                                      Base::Vector3d(0.0, 0.0, 0.0),
                                      1.0 / dvp->getScale());

    TopoDS_Edge outEdge = TopoDS::Edge(temp);
    return new Part::TopoShapeEdgePy(new Part::TopoShape(outEdge));
}

PyObject* DrawViewPartPy::getVertexBySelection(PyObject *args)
{
    int vertexIndex = 0;
    char* selName;           //Selection routine name - "Vertex0"
    if (!PyArg_ParseTuple(args, "s", &selName)) {
        throw Py::TypeError("expected (string)");
    }

    vertexIndex = DrawUtil::getIndexFromName(std::string(selName));
    DrawViewPart* dvp = getDrawViewPartPtr();

    //this is scaled and +Yup
    //need unscaled and +Ydown
    TechDraw::VertexPtr vert = dvp->getProjVertexByIndex(vertexIndex);
    if (vert == nullptr) {
        throw Py::ValueError("wrong vertIndex");
    }
    Base::Vector3d point = DrawUtil::invertY(vert->point()) / dvp->getScale();

    gp_Pnt gPoint(point.x, point.y, point.z);
    BRepBuilderAPI_MakeVertex mkVertex(gPoint);
    TopoDS_Vertex outVertex = mkVertex.Vertex();
    return new Part::TopoShapeVertexPy(new Part::TopoShape(outVertex));
}


//==============================================================================
PyObject *DrawViewPartPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawViewPartPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

