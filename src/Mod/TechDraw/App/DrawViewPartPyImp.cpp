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

#ifndef _PreComp_
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <gp_Pnt.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Shape.hxx>
#endif

#include <Base/Console.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeEdgePy.h>
#include <Mod/Part/App/TopoShapeVertexPy.h>

#include "CenterLine.h"
#include "DrawViewPart.h"
#include "Geometry.h"
#include "GeometryObject.h"
#include "Cosmetic.h"
#include "DrawUtil.h"

// inclusion of the generated files (generated out of DrawViewPartPy.xml)
#include <Mod/TechDraw/App/CosmeticVertexPy.h>
#include <Mod/TechDraw/App/DrawViewPartPy.h>
#include <Mod/TechDraw/App/DrawViewPartPy.cpp>


using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawViewPartPy::representation() const
{
    return std::string("<DrawViewPart object>");
}
//TODO: gets & sets for geometry

PyObject* DrawViewPartPy::getVisibleEdges(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    Py::List pEdgeList;
    std::vector<TechDraw::BaseGeomPtr> geoms = dvp->getEdgeGeometry();
    for (auto& g: geoms) {
        if (g->getHlrVisible()) {
            PyObject* pEdge = new Part::TopoShapeEdgePy(new Part::TopoShape(g->getOCCEdge()));
            pEdgeList.append(Py::asObject(pEdge));
        }
    }

    return Py::new_reference_to(pEdgeList);
}

PyObject* DrawViewPartPy::getHiddenEdges(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    Py::List pEdgeList;
    std::vector<TechDraw::BaseGeomPtr> geoms = dvp->getEdgeGeometry();
    for (auto& g: geoms) {
        if (!g->getHlrVisible()) {
            PyObject* pEdge = new Part::TopoShapeEdgePy(new Part::TopoShape(g->getOCCEdge()));
            pEdgeList.append(Py::asObject(pEdge));
        }
    }

    return Py::new_reference_to(pEdgeList);
}

PyObject* DrawViewPartPy::requestPaint(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawViewPart* item = getDrawViewPartPtr();
    item->requestPaint();

    Py_Return;
}

// remove all cosmetics
PyObject* DrawViewPartPy::clearCosmeticVertices(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawViewPart* item = getDrawViewPartPtr();
    item->clearCosmeticVertexes();

    Py_Return;
}

PyObject* DrawViewPartPy::clearCosmeticEdges(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawViewPart* item = getDrawViewPartPtr();
    item->clearCosmeticEdges();

    Py_Return;
}

PyObject* DrawViewPartPy::clearCenterLines(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawViewPart* item = getDrawViewPartPtr();
    item->clearCenterLines();

    Py_Return;
}

PyObject* DrawViewPartPy::clearGeomFormats(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawViewPart* item = getDrawViewPartPtr();
    item->clearGeomFormats();

    Py_Return;
}

//********* Cosmetic Vertex Routines *******************************************
PyObject* DrawViewPartPy::makeCosmeticVertex(PyObject *args)
{
    PyObject* pPnt1 = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &pPnt1)) {
        return nullptr;
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
        return nullptr;
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
    const char* id;                      //unique tag
    if (!PyArg_ParseTuple(args, "s", &id)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::CosmeticVertex* cv = dvp->getCosmeticVertex(id);
    if (cv) {
        return cv->getPyObject();
    }

    Py_Return;
}

//get by selection name
PyObject* DrawViewPartPy::getCosmeticVertexBySelection(PyObject *args)
{
    const char* selName;           //Selection routine name - "Vertex0"
    if (!PyArg_ParseTuple(args, "s", &selName)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::CosmeticVertex* cv = dvp->getCosmeticVertexBySelection(selName);
    if (cv) {
        return cv->getPyObject();
    }

    Py_Return;
}

PyObject* DrawViewPartPy::removeCosmeticVertex(PyObject *args)
{
    DrawViewPart* dvp = getDrawViewPartPtr();
    const char* tag;
    if (PyArg_ParseTuple(args, "s", &tag)) {
        dvp->removeCosmeticVertex(tag);
        dvp->refreshCVGeoms();
        dvp->requestPaint();
        Py_Return;
    }

    PyErr_Clear();
    PyObject* pCVToDelete = nullptr;
    if (PyArg_ParseTuple(args, "O!", &(TechDraw::CosmeticVertexPy::Type), &pCVToDelete)) {
        TechDraw::CosmeticVertexPy* cvPy = static_cast<TechDraw::CosmeticVertexPy*>(pCVToDelete);
        TechDraw::CosmeticVertex* cv = cvPy->getCosmeticVertexPtr();
        dvp->removeCosmeticVertex(cv->getTagAsString());
        dvp->refreshCVGeoms();
        dvp->requestPaint();
        Py_Return;
    }

    PyErr_Clear();
    PyObject* pDelList = nullptr;
    if (!PyArg_ParseTuple(args, "O", &pDelList)) {
        return nullptr;
    }

    if (PySequence_Check(pDelList))  {
        Py_ssize_t nSize = PySequence_Size(pDelList);
        for (Py_ssize_t i=0; i < nSize; i++) {
            PyObject* item = PySequence_GetItem(pDelList, i);
            if (!PyObject_TypeCheck(item, &(TechDraw::CosmeticVertexPy::Type)))  {
                PyErr_Format(PyExc_TypeError ,"Types in sequence must be 'CosmeticVertex', not %s",
                    Py_TYPE(item)->tp_name);
                return nullptr;
            }
            TechDraw::CosmeticVertexPy* cvPy = static_cast<TechDraw::CosmeticVertexPy*>(item);
            TechDraw::CosmeticVertex* cv = cvPy->getCosmeticVertexPtr();
            dvp->removeCosmeticVertex(cv->getTagAsString());
        }
        dvp->refreshCVGeoms();
        dvp->requestPaint();

        Py_Return;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "Expected string, CosmeticVertex or sequence of CosmeticVertex");
        return nullptr;
    }
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

    if (!PyArg_ParseTuple(args, "O!O!|idO!", &(Base::VectorPy::Type), &pPnt1,
                                        &(Base::VectorPy::Type), &pPnt2,
                                        &style, &weight,
                                        &PyTuple_Type, &pColor)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    Base::Vector3d pnt2 = static_cast<Base::VectorPy*>(pPnt2)->value();
    std::string newTag = dvp->addCosmeticEdge(pnt1, pnt2);
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
    if (ce) {
        ce->m_format.m_style = style;
        ce->m_format.m_weight = weight;
        ce->m_format.m_color = pColor ? DrawUtil::pyTupleToColor(pColor) : defCol;
    }
    else {
        PyErr_SetString(PyExc_RuntimeError, "DVPPI:makeCosmeticLine - line creation failed");
        return nullptr;
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

    if (!PyArg_ParseTuple(args, "O!O!|idO!", &(Base::VectorPy::Type), &pPnt1,
                                        &(Base::VectorPy::Type), &pPnt2,
                                        &style, &weight,
                                        &PyTuple_Type, &pColor)) {
        return nullptr;
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
    if (ce) {
        ce->m_format.m_style = style;
        ce->m_format.m_weight = weight;
        ce->m_format.m_color = pColor ? DrawUtil::pyTupleToColor(pColor) : defCol;
    }
    else {
        PyErr_SetString(PyExc_RuntimeError, "DVPPI:makeCosmeticLine - line creation failed");
        return nullptr;
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

    if (!PyArg_ParseTuple(args, "O!d|idO!", &(Base::VectorPy::Type), &pPnt1,
                                        &radius,
                                        &style, &weight,
                                        &PyTuple_Type, &pColor)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    TechDraw::BaseGeomPtr bg = std::make_shared<TechDraw::Circle> (pnt1, radius);
    std::string newTag = dvp->addCosmeticEdge(bg->inverted());
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
    if (ce) {
        ce->permaRadius = radius;
        ce->m_format.m_style = style;
        ce->m_format.m_weight = weight;
        ce->m_format.m_color = pColor ? DrawUtil::pyTupleToColor(pColor) : defCol;
    }
    else {
        PyErr_SetString(PyExc_RuntimeError, "DVPPI:makeCosmeticCircle - circle creation failed");
        return nullptr;
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

    if (!PyArg_ParseTuple(args, "O!ddd|idO!", &(Base::VectorPy::Type), &pPnt1,
                                        &radius, &angle1, &angle2,
                                        &style, &weight, &PyTuple_Type, &pColor)) {
        return nullptr;
    }

    //from here on is almost duplicate of makeCosmeticCircle
    DrawViewPart* dvp = getDrawViewPartPtr();
    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    TechDraw::BaseGeomPtr bg = std::make_shared<TechDraw::AOC> (pnt1, radius, angle1, angle2);
    std::string newTag = dvp->addCosmeticEdge(bg->inverted());
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
    if (ce) {
        ce->permaRadius = radius;
        ce->m_format.m_style = style;
        ce->m_format.m_weight = weight;
        if (!pColor)
            ce->m_format.m_color = defCol;
        else
            ce->m_format.m_color = DrawUtil::pyTupleToColor(pColor);
    }
    else {
        PyErr_SetString(PyExc_RuntimeError, "DVPPI:makeCosmeticCircleArc - arc creation failed");
        return nullptr;
    }

    //int link =
    dvp->add1CEToGE(newTag);
    dvp->requestPaint();

    return PyUnicode_FromString(newTag.c_str());   //return tag for new CE
}

PyObject* DrawViewPartPy::makeCosmeticCircle3d(PyObject *args)
{
    PyObject* pPnt1 = nullptr;
    double radius = 5.0;
    int style = LineFormat::getDefEdgeStyle();
    double weight = LineFormat::getDefEdgeWidth();
    App::Color defCol = LineFormat::getDefEdgeColor();
    PyObject* pColor = nullptr;

    if (!PyArg_ParseTuple(args, "O!d|idO!", &(Base::VectorPy::Type), &pPnt1,
                                        &radius,
                                        &style, &weight,
                                        &PyTuple_Type, &pColor)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    // center, project and invert the 3d point
    Base::Vector3d centroid = dvp->getOriginalCentroid();
    pnt1 = DrawUtil::invertY(dvp->projectPoint(pnt1 - centroid));
    TechDraw::BaseGeomPtr bg = std::make_shared<TechDraw::Circle> (pnt1, radius);
    std::string newTag = dvp->addCosmeticEdge(bg->inverted());
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
    if (ce) {
        ce->permaRadius = radius;
        ce->m_format.m_style = style;
        ce->m_format.m_weight = weight;
        ce->m_format.m_color = pColor ? DrawUtil::pyTupleToColor(pColor) : defCol;
    }
    else {
        PyErr_SetString(PyExc_RuntimeError, "DVPPI:makeCosmeticCircle - circle creation failed");
        return nullptr;
    }
    //int link =
    dvp->add1CEToGE(newTag);
    dvp->requestPaint();

    return PyUnicode_FromString(newTag.c_str());   //return tag for new CE
}

PyObject* DrawViewPartPy::makeCosmeticCircleArc3d(PyObject *args)
{
    PyObject* pPnt1 = nullptr;
    double radius = 5.0;
    double angle1 = 0.0;
    double angle2 = 360.0;
    int style = LineFormat::getDefEdgeStyle();
    double weight = LineFormat::getDefEdgeWidth();
    App::Color defCol = LineFormat::getDefEdgeColor();
    PyObject* pColor = nullptr;

    if (!PyArg_ParseTuple(args, "O!ddd|idO!", &(Base::VectorPy::Type), &pPnt1,
                                        &radius, &angle1, &angle2,
                                        &style, &weight, &PyTuple_Type, &pColor)) {
        return nullptr;
    }

    //from here on is almost duplicate of makeCosmeticCircle
    DrawViewPart* dvp = getDrawViewPartPtr();
    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    // center, project and invert the 3d point
    Base::Vector3d centroid = dvp->getOriginalCentroid();
    pnt1 = DrawUtil::invertY(dvp->projectPoint(pnt1 - centroid));
    TechDraw::BaseGeomPtr bg = std::make_shared<TechDraw::AOC> (pnt1, radius, angle1, angle2);
    std::string newTag = dvp->addCosmeticEdge(bg->inverted());
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
    if (ce) {
        ce->permaRadius = radius;
        ce->m_format.m_style = style;
        ce->m_format.m_weight = weight;
        if (!pColor)
            ce->m_format.m_color = defCol;
        else
            ce->m_format.m_color = DrawUtil::pyTupleToColor(pColor);
    }
    else {
        PyErr_SetString(PyExc_RuntimeError, "DVPPI:makeCosmeticCircleArc - arc creation failed");
        return nullptr;
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
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(tag);
    if (ce) {
        return ce->getPyObject();
    }
    else {
        PyErr_Format(PyExc_ValueError, "DVPPI::getCosmeticEdge - edge %s not found", tag);
        return nullptr;
    }
}

PyObject* DrawViewPartPy::getCosmeticEdgeBySelection(PyObject *args)
{
//    Base::Console().Message("DVPPI::getCosmeticEdgeBySelection()\n");
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();

    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdgeBySelection(name);
    if (ce) {
        return ce->getPyObject();
    }
    else {
        PyErr_Format(PyExc_ValueError, "DVPPI::getCosmeticEdgebySelection - edge for name %s not found", name);
        return nullptr;
    }
}

PyObject* DrawViewPartPy::removeCosmeticEdge(PyObject *args)
{
//    Base::Console().Message("DVPPI::removeCosmeticEdge()\n");
    char* tag;
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    dvp->removeCosmeticEdge(tag);

    Py_Return;
}

//********** Center Line *******************************************************

PyObject* DrawViewPartPy::makeCenterLine(PyObject *args)
{
//    Base::Console().Message("DVPPI::makeCenterLine()\n");
    PyObject* pSubs;
    int mode = 0;
    std::vector<std::string> subs;

    if (!PyArg_ParseTuple(args, "O!i", &PyList_Type, &pSubs, &mode)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    int size = PyList_Size(pSubs);
    int i = 0;
    for ( ; i < size; i++) {
        PyObject* po = PyList_GetItem(pSubs, i);
        if (PyUnicode_Check(po)) {
            std::string s = PyUnicode_AsUTF8(po);
            subs.push_back(s);
        }
        else {
            PyErr_SetString(PyExc_TypeError, "Expected list of string");
            return nullptr;
        }
    }

    CenterLine* cl = nullptr;
    std::string tag;
    if (!subs.empty()) {
        cl = CenterLine::CenterLineBuilder(dvp, subs, mode);     //vert, horiz, align
        if (cl) {
            tag = dvp->addCenterLine(cl);
        }
        else {
            PyErr_SetString(PyExc_RuntimeError, "DVPPI:makeCenterLine - line creation failed");
            return nullptr;
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
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::CenterLine* cl = dvp->getCenterLine(tag);
    if (cl) {
        return  cl->getPyObject();
    }
    else {
        PyErr_Format(PyExc_ValueError, "DVPPI::getCenterLine - centerLine %s not found", tag);
        return nullptr;
    }
}

PyObject* DrawViewPartPy::getCenterLineBySelection(PyObject *args)
{
//    Base::Console().Message("DVPPI::getCenterLineBySelection()\n");
    char* tag;
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::CenterLine* cl = dvp->getCenterLineBySelection(tag);
    if (cl) {
        return cl->getPyObject();
    }
    else {
        PyErr_Format(PyExc_ValueError, "DVPPI::getCenterLinebySelection - centerLine for tag %s not found", tag);
        return nullptr;
    }
}

PyObject* DrawViewPartPy::removeCenterLine(PyObject *args)
{
//    Base::Console().Message("DVPPI::removeCenterLine()\n");
    char* tag;
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    dvp->removeCenterLine(tag);

    Py_Return;
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

    if (!PyArg_ParseTuple(args, "iidOi", &idx, &style, &weight, &pColor, &visible)) {
        return nullptr;
    }

    color = DrawUtil::pyTupleToColor(pColor);
    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::GeomFormat* gf = dvp->getGeomFormatBySelection(idx);
    if (gf) {
        gf->m_format.m_style = style;
        gf->m_format.m_color = color;
        gf->m_format.m_weight = weight;
        gf->m_format.m_visible = visible;
    }
    else {
        TechDraw::LineFormat fmt(style, weight, color, visible);
        TechDraw::GeomFormat* newGF = new TechDraw::GeomFormat(idx, fmt);
//                    int idx =
        dvp->addGeomFormat(newGF);
    }

    Py_Return;
}

//------------------------------------------------------------------------------
PyObject* DrawViewPartPy::getEdgeByIndex(PyObject *args)
{
    int edgeIndex = 0;
    if (!PyArg_ParseTuple(args, "i", &edgeIndex)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();

    //this is scaled and +Yup
    //need unscaled and +Ydown
    TechDraw::BaseGeomPtr geom = dvp->getGeomByIndex(edgeIndex);
    if (!geom) {
        PyErr_SetString(PyExc_ValueError, "Wrong edge index");
        return nullptr;
    }

    TopoDS_Shape temp = ShapeUtils::mirrorShapeVec(geom->getOCCEdge(),
                                      Base::Vector3d(0.0, 0.0, 0.0),
                                      1.0 / dvp->getScale());

    TopoDS_Edge outEdge = TopoDS::Edge(temp);

    return new Part::TopoShapeEdgePy(new Part::TopoShape(outEdge));
}

PyObject* DrawViewPartPy::getVertexByIndex(PyObject *args)
{
    int vertexIndex = 0;
    if (!PyArg_ParseTuple(args, "i", &vertexIndex)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();

    //this is scaled and +Yup
    //need unscaled and +Ydown
    TechDraw::VertexPtr vert = dvp->getProjVertexByIndex(vertexIndex);
    if (!vert) {
        PyErr_SetString(PyExc_ValueError, "Wrong vertex index");
        return nullptr;
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
        return nullptr;
    }

    edgeIndex = DrawUtil::getIndexFromName(std::string(selName));
    DrawViewPart* dvp = getDrawViewPartPtr();

    //this is scaled and +Yup
    //need unscaled and +Ydown
    TechDraw::BaseGeomPtr geom = dvp->getGeomByIndex(edgeIndex);
    if (!geom) {
        PyErr_SetString(PyExc_ValueError, "Wrong edge index");
        return nullptr;
    }

    TopoDS_Shape temp = ShapeUtils::mirrorShapeVec(geom->getOCCEdge(),
                                      Base::Vector3d(0.0, 0.0, 0.0),
                                      1.0 / dvp->getScale());

    TopoDS_Edge outEdge = TopoDS::Edge(temp);

    return new Part::TopoShapeEdgePy(new Part::TopoShape(outEdge));
}

PyObject* DrawViewPartPy::getVertexBySelection(PyObject *args)
{
    int vertexIndex = 0;
    const char* selName;           //Selection routine name - "Vertex0"
    if (!PyArg_ParseTuple(args, "s", &selName)) {
        return nullptr;
    }

    vertexIndex = DrawUtil::getIndexFromName(std::string(selName));
    DrawViewPart* dvp = getDrawViewPartPtr();

    //this is scaled and +Yup
    //need unscaled and +Ydown
    TechDraw::VertexPtr vert = dvp->getProjVertexByIndex(vertexIndex);
    if (!vert) {
        PyErr_SetString(PyExc_ValueError, "Wrong vertex index");
        return nullptr;
    }

    Base::Vector3d point = DrawUtil::invertY(vert->point()) / dvp->getScale();
    gp_Pnt gPoint(point.x, point.y, point.z);
    BRepBuilderAPI_MakeVertex mkVertex(gPoint);
    TopoDS_Vertex outVertex = mkVertex.Vertex();

    return new Part::TopoShapeVertexPy(new Part::TopoShape(outVertex));
}

PyObject* DrawViewPartPy::projectPoint(PyObject *args)
{
    PyObject* pPoint = nullptr;
    PyObject* pInvert = Py_False;
    if (!PyArg_ParseTuple(args, "O!|O!", &(Base::VectorPy::Type), &pPoint, &PyBool_Type, &pInvert)) {
        return nullptr;
    }

    bool invert = Base::asBoolean(pInvert);

    DrawViewPart* dvp = getDrawViewPartPtr();
    Base::Vector3d projection = dvp->projectPoint(static_cast<Base::VectorPy*>(pPoint)->value(), invert);

    return new Base::VectorPy(new Base::Vector3d(projection));
}

//==============================================================================
PyObject *DrawViewPartPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int DrawViewPartPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

