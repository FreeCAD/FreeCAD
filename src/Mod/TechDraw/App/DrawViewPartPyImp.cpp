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


# include <BRepBuilderAPI_MakeVertex.hxx>
# include <gp_Pnt.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Shape.hxx>


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
using DU = DrawUtil;
// returns a string which represents the object e.g. when printed in python
std::string DrawViewPartPy::representation() const
{
    return {"<DrawViewPart object>"};
}
//TODO: gets & sets for geometry

PyObject* DrawViewPartPy::getVisibleEdges(PyObject *args)
{
    //NOLINTNEXTLINE
    PyObject* conventionalCoords = Py_False;    // false for gui display (+Y down), true for calculations (+Y up)
    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &conventionalCoords)) {
        throw Py::ValueError("Expected '[conventionalCoords=True/False] or None' ");
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    Py::List pEdgeList;
    std::vector<TechDraw::BaseGeomPtr> geoms = dvp->getEdgeGeometry();
    for (auto& g: geoms) {
        if (g->getHlrVisible()) {
            TopoDS_Edge occEdge = g->getOCCEdge();
            if (PyBool_Check(conventionalCoords) && conventionalCoords == Py_True) {
                TopoDS_Shape occShape = ShapeUtils::invertGeometry(occEdge);
                occEdge = TopoDS::Edge(occShape);
            }
            PyObject* pEdge = new Part::TopoShapeEdgePy(new Part::TopoShape(occEdge));
            pEdgeList.append(Py::asObject(pEdge));
        }
    }

    return Py::new_reference_to(pEdgeList);
}

PyObject* DrawViewPartPy::getHiddenEdges(PyObject *args)
{
    PyObject* conventionalCoords = Py_False;    // false for gui display (+Y down), true for calculations (+Y up)
    //NOLINTNEXTLINE
    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &conventionalCoords)) {
        throw Py::ValueError("Expected '[conventionalCoords=True/False] or None' ");
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    Py::List pEdgeList;
    std::vector<TechDraw::BaseGeomPtr> geoms = dvp->getEdgeGeometry();
    for (auto& g: geoms) {
        if (!g->getHlrVisible()) {
            TopoDS_Edge occEdge = g->getOCCEdge();
            if (PyBool_Check(conventionalCoords) && conventionalCoords == Py_True) {
                TopoDS_Shape occShape = ShapeUtils::invertGeometry(occEdge);
                occEdge = TopoDS::Edge(occShape);
            }
            PyObject* pEdge = new Part::TopoShapeEdgePy(new Part::TopoShape(occEdge));
            pEdgeList.append(Py::asObject(pEdge));
        }
    }

    return Py::new_reference_to(pEdgeList);
}

PyObject* DrawViewPartPy::getVisibleVertexes(PyObject *args)
{
    PyObject* conventionalCoords = Py_False;    // false for gui display (+Y down), true for calculations (+Y up)
    //NOLINTNEXTLINE
    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &conventionalCoords)) {
        throw Py::ValueError("Expected '[conventionalCoords=True/False] or None' ");
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    Py::List pVertexList;
    auto vertsAll = dvp->getVertexGeometry();
    for (auto& vert: vertsAll) {
        if (vert->getHlrVisible()) {
            Base::Vector3d vertPoint = vert->point();
            if (PyBool_Check(conventionalCoords) && conventionalCoords == Py_True) {
                vertPoint = DU::invertY(vertPoint);
            }
            PyObject* pVertex = new Base::VectorPy(new Base::Vector3d(vertPoint));
            pVertexList.append(Py::asObject(pVertex));
        }
    }

    return Py::new_reference_to(pVertexList);
}

PyObject* DrawViewPartPy::getHiddenVertexes(PyObject *args)
{
    PyObject* conventionalCoords = Py_False;    // false for gui display (+Y down), true for calculations (+Y up)
    //NOLINTNEXTLINE
    if (!PyArg_ParseTuple(args, "|O!", &PyBool_Type, &conventionalCoords)) {
        throw Py::ValueError("Expected '[conventionalCoords=True/False] or None' ");
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    Py::List pVertexList;
    auto vertsAll = dvp->getVertexGeometry();
    for (auto& vert: vertsAll) {
        if (!vert->getHlrVisible()) {
            Base::Vector3d vertPoint = vert->point();
            if (PyBool_Check(conventionalCoords) && conventionalCoords == Py_True) {
                vertPoint = DU::invertY(vertPoint);
            }
            PyObject* pVertex = new Base::VectorPy(new Base::Vector3d(vertPoint));
            pVertexList.append(Py::asObject(pVertex));
        }
    }

    return Py::new_reference_to(pVertexList);
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

PyObject* DrawViewPartPy::getGeometricCenter(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    Base::Vector3d pointOut = dvp->getCurrentCentroid();
    return new Base::VectorPy(new Base::Vector3d(pointOut));
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
    if (!dvp->hasGeometry()) {
        Base::Console().error("%s has no geometry yet. Can not add cosmetic vertex.\n", dvp->Label.getValue());
        Py_Return;
    }
    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    std::string id = dvp->addCosmeticVertex(pnt1);
    dvp->add1CVToGV(id);
    dvp->requestPaint();

    return PyUnicode_FromString(id.c_str());   //return tag for new CV
}

//! make a cosmetic vertex from a 3d point
PyObject* DrawViewPartPy::makeCosmeticVertex3d(PyObject *args)
{
    PyObject* pPnt1 = nullptr;
    if (!PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &pPnt1)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    if (!dvp->hasGeometry()) {
        Base::Console().error("%s has no geometry yet. Can not add cosmetic vertex.\n", dvp->Label.getValue());
        Py_Return;
    }
    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    Base::Vector3d centroid = dvp->getOriginalCentroid();
    // center the point
    pnt1 = pnt1 - centroid;
    // project but do not invert
    Base::Vector3d projected = dvp->projectPoint(pnt1);
    // this is a real world point, it is not scaled or rotated, so so it is in canonical form
    // add and invert the point.
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
    const char* id{};                      //unique tag
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
    const char* tag{};
    if (PyArg_ParseTuple(args, "s", &tag)) {
        dvp->removeCosmeticVertex(tag);
        dvp->refreshCVGeoms();
        dvp->requestPaint();
        Py_Return;
    }

    PyErr_Clear();
    PyObject* pCVToDelete = nullptr;
    if (PyArg_ParseTuple(args, "O!", &(TechDraw::CosmeticVertexPy::Type), &pCVToDelete)) {
        auto* cvPy = static_cast<TechDraw::CosmeticVertexPy*>(pCVToDelete);
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
        Py::Sequence sequence(pDelList);
        for (const auto& item : sequence) {
            if (!PyObject_TypeCheck(item.ptr(), &(TechDraw::CosmeticVertexPy::Type)))  {
                PyErr_Format(PyExc_TypeError ,"Types in sequence must be 'CosmeticVertex', not %s",
                    Py_TYPE(item.ptr())->tp_name);
                return nullptr;
            }
            auto* cvPy = static_cast<TechDraw::CosmeticVertexPy*>(item.ptr());
            TechDraw::CosmeticVertex* cv = cvPy->getCosmeticVertexPtr();
            dvp->removeCosmeticVertex(cv->getTagAsString());
        }
        dvp->refreshCVGeoms();
        dvp->requestPaint();

        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "Expected string, CosmeticVertex or sequence of CosmeticVertex");
    return nullptr;
}


//********* Cosmetic Line Routines *********************************************

PyObject* DrawViewPartPy::makeCosmeticLine(PyObject *args)
{
    // the input points are expected to use conventional coordinates (Y up) and need to be inverted
    // before building the line
    PyObject* pPnt1 = nullptr;
    PyObject* pPnt2 = nullptr;
    int style = LineFormat::getDefEdgeStyle();
    double weight = LineFormat::getDefEdgeWidth();
    Base::Color defCol = LineFormat::getDefEdgeColor();
    PyObject* pColor = nullptr;

    if (!PyArg_ParseTuple(args, "O!O!|idO!", &(Base::VectorPy::Type), &pPnt1,
                                        &(Base::VectorPy::Type), &pPnt2,
                                        &style, &weight,
                                        &PyTuple_Type, &pColor)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    if (!dvp->hasGeometry()) {
        Base::Console().error("%s has no geometry yet. Can not add cosmetic line.\n", dvp->Label.getValue());
        Py_Return;
    }

    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    Base::Vector3d pnt2 = static_cast<Base::VectorPy*>(pPnt2)->value();
    std::string newTag = dvp->addCosmeticEdge(DU::invertY(pnt1), DU::invertY(pnt2));
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
    if (ce) {
        ce->m_format.setStyle(style);
        ce->m_format.setWidth(weight);
        ce->m_format.setColor(pColor ? DrawUtil::pyTupleToColor(pColor) : defCol);
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
    // input points are expected to be conventional 3d points
    PyObject* pPnt1 = nullptr;
    PyObject* pPnt2 = nullptr;
    int style = LineFormat::getDefEdgeStyle();
    double weight = LineFormat::getDefEdgeWidth();
    Base::Color defCol = LineFormat::getDefEdgeColor();
    PyObject* pColor = nullptr;

    if (!PyArg_ParseTuple(args, "O!O!|idO!", &(Base::VectorPy::Type), &pPnt1,
                                        &(Base::VectorPy::Type), &pPnt2,
                                        &style, &weight,
                                        &PyTuple_Type, &pColor)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    if (!dvp->hasGeometry()) {
        Base::Console().error("%s has no geometry yet. Can not add cosmetic line.\n", dvp->Label.getValue());
        Py_Return;
    }
    Base::Vector3d centroid = dvp->getOriginalCentroid();

    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    pnt1 = pnt1 - centroid;
    pnt1 = dvp->projectPoint(pnt1);

    Base::Vector3d pnt2 = static_cast<Base::VectorPy*>(pPnt2)->value();
    pnt2 = pnt2 - centroid;
    pnt2 = dvp->projectPoint(pnt2);

    std::string newTag = dvp->addCosmeticEdge(pnt1, pnt2);
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
    if (ce) {
        ce->m_format.setStyle(style);
        ce->m_format.setWidth(weight);
        ce->m_format.setColor(pColor ? DrawUtil::pyTupleToColor(pColor) : defCol);
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
    constexpr double DefaultRadius{5.0};
    double radius = DefaultRadius;
    int style = LineFormat::getDefEdgeStyle();
    double weight = LineFormat::getDefEdgeWidth();
    Base::Color defCol = LineFormat::getDefEdgeColor();
    PyObject* pColor = nullptr;

    if (!PyArg_ParseTuple(args, "O!d|idO!", &(Base::VectorPy::Type), &pPnt1,
                                        &radius,
                                        &style, &weight,
                                        &PyTuple_Type, &pColor)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    if (!dvp->hasGeometry()) {
        Base::Console().error("%s has no geometry yet. Can not add cosmetic circle.\n", dvp->Label.getValue());
        Py_Return;
    }

    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    TechDraw::BaseGeomPtr bg = std::make_shared<TechDraw::Circle> (pnt1, radius);
    std::string newTag = dvp->addCosmeticEdge(bg->inverted());
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
    if (ce) {
        ce->permaRadius = radius;
        ce->m_format.setStyle(style);
        ce->m_format.setWidth(weight);
        ce->m_format.setColor(pColor ? DrawUtil::pyTupleToColor(pColor) : defCol);
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
    constexpr double DefaultRadius{5.0};
    constexpr double DegreesInCircle{360.0};
    double radius = DefaultRadius;
    double angle1 = 0.0;
    double angle2 = DegreesInCircle;
    int style = LineFormat::getDefEdgeStyle();
    double weight = LineFormat::getDefEdgeWidth();
    Base::Color defCol = LineFormat::getDefEdgeColor();
    PyObject* pColor = nullptr;

    if (!PyArg_ParseTuple(args, "O!ddd|idO!", &(Base::VectorPy::Type), &pPnt1,
                                        &radius, &angle1, &angle2,
                                        &style, &weight, &PyTuple_Type, &pColor)) {
        return nullptr;
    }

    //from here on is almost duplicate of makeCosmeticCircle
    DrawViewPart* dvp = getDrawViewPartPtr();
    if (!dvp->hasGeometry()) {
        Base::Console().error("%s has no geometry yet. Can not add cosmetic circle arc.\n", dvp->Label.getValue());
        Py_Return;
    }

    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    TechDraw::BaseGeomPtr bg = std::make_shared<TechDraw::AOC> (pnt1, radius, angle1, angle2);
    std::string newTag = dvp->addCosmeticEdge(bg->inverted());
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
    if (ce) {
        ce->permaRadius = radius;
        ce->m_format.setStyle(style);
        ce->m_format.setWidth(weight);
        if (!pColor){
            ce->m_format.setColor(defCol);
        }
        else {
            ce->m_format.setColor(DrawUtil::pyTupleToColor(pColor));
        }
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
    constexpr double DefaultRadius{5.0};
    double radius = DefaultRadius;
    int style = LineFormat::getDefEdgeStyle();
    double weight = LineFormat::getDefEdgeWidth();
    Base::Color defCol = LineFormat::getDefEdgeColor();
    PyObject* pColor = nullptr;

    if (!PyArg_ParseTuple(args, "O!d|idO!", &(Base::VectorPy::Type), &pPnt1,
                                        &radius,
                                        &style, &weight,
                                        &PyTuple_Type, &pColor)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    if (!dvp->hasGeometry()) {
        Base::Console().error("%s has no geometry yet. Can not add cosmetic circle.\n", dvp->Label.getValue());
        Py_Return;
    }

    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    // center, project and invert the 3d point
    Base::Vector3d centroid = dvp->getOriginalCentroid();
    pnt1 = DrawUtil::invertY(dvp->projectPoint(pnt1 - centroid));
    TechDraw::BaseGeomPtr bg = std::make_shared<TechDraw::Circle> (pnt1, radius);
    std::string newTag = dvp->addCosmeticEdge(bg->inverted());
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
    if (ce) {
        ce->permaRadius = radius;
        ce->m_format.setStyle(style);
        ce->m_format.setWidth(weight);
        ce->m_format.setColor(pColor ? DrawUtil::pyTupleToColor(pColor) : defCol);
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
    constexpr double DefaultRadius{5.0};
    double radius = DefaultRadius;
    double angle1 = 0.0;
    constexpr double DegreesInCircle{360.0};
    double angle2 = DegreesInCircle;
    int style = LineFormat::getDefEdgeStyle();
    double weight = LineFormat::getDefEdgeWidth();
    Base::Color defCol = LineFormat::getDefEdgeColor();
    PyObject* pColor = nullptr;

    if (!PyArg_ParseTuple(args, "O!ddd|idO!", &(Base::VectorPy::Type), &pPnt1,
                                        &radius, &angle1, &angle2,
                                        &style, &weight, &PyTuple_Type, &pColor)) {
        return nullptr;
    }

    //from here on is almost duplicate of makeCosmeticCircle
    DrawViewPart* dvp = getDrawViewPartPtr();
    if (!dvp->hasGeometry()) {
        Base::Console().error("%s has no geometry yet. Can not add cosmetic circle arc.\n", dvp->Label.getValue());
        Py_Return;
    }

    Base::Vector3d pnt1 = static_cast<Base::VectorPy*>(pPnt1)->value();
    // center, project and invert the 3d point
    Base::Vector3d centroid = dvp->getOriginalCentroid();
    pnt1 = DrawUtil::invertY(dvp->projectPoint(pnt1 - centroid));
    TechDraw::BaseGeomPtr bg = std::make_shared<TechDraw::AOC> (pnt1, radius, angle1, angle2);
    std::string newTag = dvp->addCosmeticEdge(bg->inverted());
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(newTag);
    if (ce) {
        ce->permaRadius = radius;
        ce->m_format.setStyle(style);
        ce->m_format.setWidth(weight);
        if (!pColor) {
            ce->m_format.setColor(defCol);
        }
        else {
            ce->m_format.setColor(DrawUtil::pyTupleToColor(pColor));
        }
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
    char* tag{};
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdge(tag);
    if (ce) {
        return ce->getPyObject();
    }

    PyErr_Format(PyExc_ValueError, "DVPPI::getCosmeticEdge - edge %s not found", tag);
    return nullptr;
}

PyObject* DrawViewPartPy::getCosmeticEdgeBySelection(PyObject *args)
{
    char* name{};
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();

    TechDraw::CosmeticEdge* ce = dvp->getCosmeticEdgeBySelection(name);
    if (ce) {
        return ce->getPyObject();
    }

    PyErr_Format(PyExc_ValueError, "DVPPI::getCosmeticEdgebySelection - edge for name %s not found", name);
    return nullptr;
}

PyObject* DrawViewPartPy::removeCosmeticEdge(PyObject *args)
{
    char* tag{};
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
    PyObject* pSubs{};
    CenterLine::Mode mode = CenterLine::Mode::VERTICAL;
    std::vector<std::string> subs;

    if (!PyArg_ParseTuple(args, "O!i", &PyList_Type, &pSubs, &mode)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    if (!dvp->hasGeometry()) {
        Base::Console().error("%s has no geometry yet. Can not add center line.\n", dvp->Label.getValue());
        Py_Return;
    }

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
    char* tag{};
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::CenterLine* cl = dvp->getCenterLine(tag);
    if (cl) {
        return  cl->getPyObject();
    }
    PyErr_Format(PyExc_ValueError, "DVPPI::getCenterLine - centerLine %s not found", tag);
    return nullptr;
}

PyObject* DrawViewPartPy::getCenterLineBySelection(PyObject *args)
{
    char* tag{};
    if (!PyArg_ParseTuple(args, "s", &tag)) {
        return nullptr;
    }

    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::CenterLine* cl = dvp->getCenterLineBySelection(tag);
    if (cl) {
        return cl->getPyObject();
    }
    PyErr_Format(PyExc_ValueError, "DVPPI::getCenterLinebySelection - centerLine for tag %s not found", tag);
    return nullptr;
}

PyObject* DrawViewPartPy::removeCenterLine(PyObject *args)
{
    char* tag{};
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
    int idx = -1;
    int style = Qt::SolidLine;
    Base::Color color = LineFormat::getDefEdgeColor();
    constexpr double DefaultWeight{0.5};
    double weight = DefaultWeight;
    int visible = 1;
    PyObject* pColor{};

    if (!PyArg_ParseTuple(args, "iidOi", &idx, &style, &weight, &pColor, &visible)) {
        return nullptr;
    }

    color = DrawUtil::pyTupleToColor(pColor);
    DrawViewPart* dvp = getDrawViewPartPtr();
    TechDraw::GeomFormat* gf = dvp->getGeomFormatBySelection(idx);
    if (gf) {
        gf->m_format.setStyle(style);
        gf->m_format.setColor(color);
        gf->m_format.setWidth(weight);
        gf->m_format.setVisible(visible);
    }
    else {
        TechDraw::LineFormat fmt(style, weight, color, visible);
        auto* newGF = new TechDraw::GeomFormat(idx, fmt);
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
    char* selName{};           //Selection routine name - "Edge0"
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
    const char* selName{};           //Selection routine name - "Vertex0"
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

