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
# include <cstdlib>
# include <sstream>

# include <Bnd_Box.hxx>
# include <BRepBndLib.hxx>
# include <BRep_Builder.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <Geom_Line.hxx>
# include <gp_Pln.hxx>
# include <TopoDS_Edge.hxx>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Vector3D.h>

#include "DrawDimHelper.h"
#include "Cosmetic.h"
#include "DrawPage.h"
#include "DrawUtil.h"
#include "DrawViewDimension.h"
#include "DrawViewDimExtent.h"
#include "DrawViewPart.h"
#include "Geometry.h"
#include "GeometryObject.h"


#define HORIZONTAL 0
#define VERTICAL 1
#define LENGTH 2

using namespace TechDraw;

void DrawDimHelper::makeExtentDim(DrawViewPart* dvp, std::vector<std::string> edgeNames,
                                  int direction)
{
    //    Base::Console().Message("DDH::makeExtentDim() - dvp: %s edgeNames: %d\n",
    //                            dvp->Label.getValue(), edgeNames.size());
    if (!dvp) {
        return;
    }

    std::string dimType = "DistanceX";
    int dimNum = 0;
    if (direction == VERTICAL) {
        dimType = "DistanceY";
        dimNum = 1;
    }

    TechDraw::DrawPage* page = dvp->findParentPage();
    std::string pageName = page->getNameInDocument();

    App::Document* doc = dvp->getDocument();
    std::string dimName = doc->getUniqueObjectName("DimExtent");
    Base::Interpreter().runStringArg(
        "App.activeDocument().addObject('TechDraw::DrawViewDimExtent', '%s')", dimName.c_str());
        Base::Interpreter().runStringArg(
            "App.activeDocument().%s.translateLabel('DrawViewDimExtent', 'DimExtent', '%s')",
              dimName.c_str(), dimName.c_str());    Base::Interpreter().runStringArg(
        "App.activeDocument().%s.Type = '%s'", dimName.c_str(), dimType.c_str());
    Base::Interpreter().runStringArg(
        "App.activeDocument().%s.DirExtent = %d", dimName.c_str(), dimNum);

    TechDraw::DrawViewDimExtent* dimExt =
        dynamic_cast<TechDraw::DrawViewDimExtent*>(doc->getObject(dimName.c_str()));
    if (!dimExt) {
        throw Base::TypeError("Dim extent not found");
    }
    dimExt->Source.setValue(dvp, edgeNames);
    ReferenceVector newRefs;
    if (edgeNames.empty()) {
        ReferenceEntry emptyRef(dvp, std::string());
        newRefs.push_back(emptyRef);
    }
    else {
        for (auto& edge : edgeNames) {
            ReferenceEntry ref(dvp, edge);
            newRefs.push_back(ref);
        }
    }
    dimExt->setReferences2d(newRefs);

    Base::Interpreter().runStringArg("App.activeDocument().%s.addView(App.activeDocument().%s)",
                                     pageName.c_str(),
                                     dimName.c_str());

    dimExt->recomputeFeature();
}

void DrawDimHelper::makeExtentDim3d(DrawViewPart* dvp, ReferenceVector references, int direction)
{
    //    Base::Console().Message("DDH::makeExtentDim3d() - dvp: %s references: %d\n",
    //                            dvp->Label.getValue(), references.size());
    if (!dvp) {
        return;
    }

    std::string dimType = "DistanceX";
    int dimNum = 0;
    if (direction == VERTICAL) {
        dimType = "DistanceY";
        dimNum = 1;
    }

    TechDraw::DrawPage* page = dvp->findParentPage();
    std::string pageName = page->getNameInDocument();

    App::Document* doc = dvp->getDocument();
    std::string dimName = doc->getUniqueObjectName("DimExtent");
    Base::Interpreter().runStringArg(
        "App.activeDocument().addObject('TechDraw::DrawViewDimExtent', '%s')", dimName.c_str());
        Base::Interpreter().runStringArg(
            "App.activeDocument().%s.translateLabel('DrawViewDimExtent', 'DimExtent', '%s')",
              dimName.c_str(), dimName.c_str());    Base::Interpreter().runStringArg(
        "App.activeDocument().%s.Type = '%s'", dimName.c_str(), dimType.c_str());
    Base::Interpreter().runStringArg(
        "App.activeDocument().%s.DirExtent = %d", dimName.c_str(), dimNum);

    TechDraw::DrawViewDimExtent* dimExt =
        dynamic_cast<TechDraw::DrawViewDimExtent*>(doc->getObject(dimName.c_str()));
    if (!dimExt) {
        throw Base::TypeError("Dim extent not found");
    }

    dimExt->Source.setValue(dvp);

    std::vector<App::DocumentObject*> objs3d;
    std::vector<std::string> subs3d;
    for (auto& ref : references) {
        objs3d.push_back(ref.getObject());
        subs3d.push_back(ref.getSubName());
    }
    dimExt->Source3d.setValues(objs3d, subs3d);

    ReferenceVector newRefs2d;
    ReferenceEntry emptyRef(dvp, std::string());
    newRefs2d.push_back(emptyRef);
    dimExt->setReferences2d(newRefs2d);

    dimExt->setReferences3d(references);

    Base::Interpreter().runStringArg("App.activeDocument().%s.addView(App.activeDocument().%s)",
                                     pageName.c_str(),
                                     dimName.c_str());

    dimExt->recomputeFeature();
}
std::pair<Base::Vector3d, Base::Vector3d>
DrawDimHelper::minMax(DrawViewPart* dvp, std::vector<std::string> edgeNames, int direction)
{
    //    Base::Console().Message("DDH::minMax() - edgeName: %d\n", edgeNames.size());
    std::pair<Base::Vector3d, Base::Vector3d> result;
    Base::Vector3d refMin;
    Base::Vector3d refMax;

    gp_Ax3 projAx3;// OXYZ
    gp_Pln projPlane(projAx3);

    BaseGeomPtrVector edgeGeomList;
    if (!edgeNames.empty() && !edgeNames.front().empty()) {
        //we have edge names and the first one isn't null
        for (auto& n : edgeNames) {
            std::string geomType = DrawUtil::getGeomTypeFromName(n);
            if (geomType == "Edge") {
                int i = DrawUtil::getIndexFromName(n);
                BaseGeomPtr bg = dvp->getGeomByIndex(i);
                if (bg) {
                    edgeGeomList.push_back(bg);
                }
            }
        }
    }
    else {
        for (auto& edge : dvp->getEdgeGeometry()) {
            if (!edge->getCosmetic()) {
                // skip cosmetic edges
                edgeGeomList.push_back(edge);
            }
        }
    }

    if (edgeGeomList.empty()) {
        return result;
    }

    Bnd_Box edgeBbx;
    edgeBbx.SetGap(1.0);//make the box a bit bigger

    std::vector<TopoDS_Edge> inEdges;
    for (auto& bg : edgeGeomList) {
        inEdges.push_back(bg->getOCCEdge());
        BRepBndLib::Add(bg->getOCCEdge(), edgeBbx);
    }

    double minX, minY, minZ, maxX, maxY, maxZ;
    edgeBbx.Get(minX, minY, minZ, maxX, maxY, maxZ);
    double xMid = (maxX + minX) / 2.0;
    double yMid = (maxY + minY) / 2.0;

    gp_Pnt rightMid(maxX, yMid, 0.0);
    gp_Pnt leftMid(minX, yMid, 0.0);
    gp_Pnt topMid(xMid, maxY, 0.0);
    gp_Pnt bottomMid(xMid, minY, 0.0);

    gp_Dir xDir(1.0, 0.0, 0.0);
    gp_Dir yDir(0.0, 1.0, 0.0);

    if (direction == HORIZONTAL) {
        Handle(Geom_Line) lineLeft = new Geom_Line(leftMid, yDir);
        BRepBuilderAPI_MakeEdge mkEdgeLeft(lineLeft);
        TopoDS_Edge edgeLeft = mkEdgeLeft.Edge();
        gp_Pnt leftPoint = findClosestPoint(inEdges, edgeLeft);
        Handle(Geom_Line) lineRight = new Geom_Line(rightMid, yDir);
        BRepBuilderAPI_MakeEdge mkEdgeRight(lineRight);
        TopoDS_Edge edgeRight = mkEdgeRight.Edge();
        gp_Pnt rightPoint = findClosestPoint(inEdges, edgeRight);

        refMin = Base::Vector3d(leftPoint.X(), leftPoint.Y(), 0.0);
        refMax = Base::Vector3d(rightPoint.X(), rightPoint.Y(), 0.0);
    }
    else if (direction == VERTICAL) {
        Handle(Geom_Line) lineBottom = new Geom_Line(bottomMid, xDir);
        BRepBuilderAPI_MakeEdge mkEdgeBottom(lineBottom);
        TopoDS_Edge edgeBottom = mkEdgeBottom.Edge();
        gp_Pnt bottomPoint = findClosestPoint(inEdges, edgeBottom);
        Handle(Geom_Line) lineTop = new Geom_Line(topMid, xDir);
        BRepBuilderAPI_MakeEdge mkEdgeTop(lineTop);
        TopoDS_Edge edgeTop = mkEdgeTop.Edge();
        gp_Pnt topPoint = findClosestPoint(inEdges, edgeTop);
        refMin = Base::Vector3d(bottomPoint.X(), bottomPoint.Y(), 0.0);
        refMax = Base::Vector3d(topPoint.X(), topPoint.Y(), 0.0);
    }

    result.first = refMin;
    result.second = refMax;
    return result;
}

//given list of curves, find the closest point from any curve to a boundary
//computation intensive for a cosmetic result.
gp_Pnt DrawDimHelper::findClosestPoint(std::vector<TopoDS_Edge> inEdges, TopoDS_Edge& boundary)
{
    //    Base::Console().Message("DDH::findClosestPoint() - edges: %d\n", inEdges.size());
    //
    //find an extent point that is actually on one of the curves
    double minDistance(std::numeric_limits<float>::max());
    gp_Pnt nearPoint;
    for (auto& edge : inEdges) {
        BRepExtrema_DistShapeShape extss(edge, boundary);
        if (!extss.IsDone()) {
            Base::Console().Warning(
                "DDH::findClosestPoint - BRepExtrema_DistShapeShape failed - 1\n");
            continue;
        }
        if (extss.NbSolution() == 0) {
            Base::Console().Warning(
                "DDH::findClosestPoint - BRepExtrema_DistShapeShape failed - 2\n");
            continue;
        }
        if (extss.Value() < minDistance) {
            minDistance = extss.Value();
            nearPoint = extss.PointOnShape1(1);
        }
    }
    return nearPoint;
}

std::pair<Base::Vector3d, Base::Vector3d>
DrawDimHelper::minMax3d(DrawViewPart* dvp, ReferenceVector references, int direction)
{
    //    Base::Console().Message("DDH::minMax3d() - references: %d\n", references.size());
    std::pair<Base::Vector3d, Base::Vector3d> result;
    Base::Vector3d refMin;
    Base::Vector3d refMax;

    gp_Ax3 projAx3;//OXYZ
    gp_Pln projPlane(projAx3);

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    for (auto& ref : references) {
        builder.Add(comp, ref.getGeometry());
    }
    Base::Vector3d centroid = dvp->getOriginalCentroid();
    TopoDS_Shape centeredShape =//this result is a throw away. We will work with comp.
        DrawViewPart::centerScaleRotate(dvp, comp, centroid);

    //project the selected 3d shapes in the dvp's coord system
    TechDraw::GeometryObjectPtr go(
        std::make_shared<TechDraw::GeometryObject>(std::string(), nullptr));
    go->setIsoCount(0);
    go->isPerspective(false);
    go->usePolygonHLR(false);
    go->projectShape(comp, dvp->getProjectionCS());
    auto edges = go->getEdgeGeometry();

    if (edges.empty()) {
        return result;
    }

    Bnd_Box shapeBbx;
    shapeBbx.SetGap(1.0);//make the box a bit bigger

    std::vector<TopoDS_Edge> inEdges;
    for (auto& bg : edges) {
        inEdges.push_back(bg->getOCCEdge());
        BRepBndLib::Add(bg->getOCCEdge(), shapeBbx);
    }

    //from here on this is the same as 2d method
    double minX, minY, minZ, maxX, maxY, maxZ;
    shapeBbx.Get(minX, minY, minZ, maxX, maxY, maxZ);
    double xMid = (maxX + minX) / 2.0;
    double yMid = (maxY + minY) / 2.0;

    gp_Pnt rightMid(maxX, yMid, 0.0);
    gp_Pnt leftMid(minX, yMid, 0.0);
    gp_Pnt topMid(xMid, maxY, 0.0);
    gp_Pnt bottomMid(xMid, minY, 0.0);

    gp_Dir xDir(1.0, 0.0, 0.0);
    gp_Dir yDir(0.0, 1.0, 0.0);

    if (direction == HORIZONTAL) {
        Handle(Geom_Line) lineLeft = new Geom_Line(leftMid, yDir);
        BRepBuilderAPI_MakeEdge mkEdgeLeft(lineLeft);
        TopoDS_Edge edgeLeft = mkEdgeLeft.Edge();
        gp_Pnt leftPoint = findClosestPoint(inEdges, edgeLeft);
        Handle(Geom_Line) lineRight = new Geom_Line(rightMid, yDir);
        BRepBuilderAPI_MakeEdge mkEdgeRight(lineRight);
        TopoDS_Edge edgeRight = mkEdgeRight.Edge();
        gp_Pnt rightPoint = findClosestPoint(inEdges, edgeRight);

        refMin = Base::Vector3d(leftPoint.X(), leftPoint.Y(), 0.0);
        refMax = Base::Vector3d(rightPoint.X(), rightPoint.Y(), 0.0);
    }
    else if (direction == VERTICAL) {
        Handle(Geom_Line) lineBottom = new Geom_Line(bottomMid, xDir);
        BRepBuilderAPI_MakeEdge mkEdgeBottom(lineBottom);
        TopoDS_Edge edgeBottom = mkEdgeBottom.Edge();
        gp_Pnt bottomPoint = findClosestPoint(inEdges, edgeBottom);
        Handle(Geom_Line) lineTop = new Geom_Line(topMid, xDir);
        BRepBuilderAPI_MakeEdge mkEdgeTop(lineTop);
        TopoDS_Edge edgeTop = mkEdgeTop.Edge();
        gp_Pnt topPoint = findClosestPoint(inEdges, edgeTop);
        refMin = Base::Vector3d(bottomPoint.X(), bottomPoint.Y(), 0.0);
        refMax = Base::Vector3d(topPoint.X(), topPoint.Y(), 0.0);
    }

    result.first = refMin;
    result.second = refMax;
    return result;
}

DrawViewDimension*
DrawDimHelper::makeDistDim(DrawViewPart* dvp, std::string dimType,
                           Base::Vector3d inMin,//is this scaled or unscaled??
                           Base::Vector3d inMax,//expects scaled from makeExtentDim
                           bool extent)
{
    //    Base::Console().Message("DDH::makeDistDim() - inMin: %s inMax: %s\n",
    //                            DrawUtil::formatVector(inMin).c_str(),
    //                            DrawUtil::formatVector(inMax).c_str());
    TechDraw::DrawPage* page = dvp->findParentPage();
    std::string pageName = page->getNameInDocument();

    TechDraw::DrawViewDimension* dim = nullptr;
    App::Document* doc = dvp->getDocument();
    std::string dimName = doc->getUniqueObjectName("Dimension");
    if (extent) {
        dimName = doc->getUniqueObjectName("DimExtent");
    }

    std::vector<TechDraw::VertexPtr> gVerts = dvp->getVertexGeometry();

    Base::Vector3d cleanMin = DrawUtil::invertY(inMin);
    std::string tag1 = dvp->addCosmeticVertex(cleanMin);
    int iGV1 = dvp->add1CVToGV(tag1);

    Base::Vector3d cleanMax = DrawUtil::invertY(inMax);
    std::string tag2 = dvp->addCosmeticVertex(cleanMax);
    int iGV2 = dvp->add1CVToGV(tag2);

    gVerts = dvp->getVertexGeometry();
    std::vector<App::DocumentObject*> objs;
    std::vector<std::string> subs;

    std::stringstream ss;
    ss << "Vertex" << iGV1;
    std::string vertexName = ss.str();
    subs.push_back(vertexName);
    objs.push_back(dvp);

    ss.clear();
    ss.str(std::string());
    ss << "Vertex" << iGV2;
    vertexName = ss.str();
    subs.push_back(vertexName);
    objs.push_back(dvp);

    if (extent) {
        Base::Interpreter().runStringArg(
            "App.activeDocument().addObject('TechDraw::DrawViewDimExtent', '%s')", dimName.c_str());
        Base::Interpreter().runStringArg(
            "App.activeDocument().%s.translateLabel('DrawViewDimExtent', 'DimExtent', '%s')",
              dimName.c_str(), dimName.c_str());
    }
    else {
        Base::Interpreter().runStringArg(
            "App.activeDocument().addObject('TechDraw::DrawViewDimension', '%s')", dimName.c_str());
        Base::Interpreter().runStringArg(
            "App.activeDocument().%s.translateLabel('DrawViewDimimension', 'Dimension', '%s')",
              dimName.c_str(), dimName.c_str());
    }

    Base::Interpreter().runStringArg(
        "App.activeDocument().%s.Type = '%s'", dimName.c_str(), dimType.c_str());

    dim = dynamic_cast<TechDraw::DrawViewDimension*>(doc->getObject(dimName.c_str()));
    if (!dim) {
        throw Base::TypeError("DDH::makeDistDim - dim not found\n");
    }
    dim->References2D.setValues(objs, subs);

    Base::Interpreter().runStringArg("App.activeDocument().%s.addView(App.activeDocument().%s)",
                                     pageName.c_str(),
                                     dimName.c_str());

    dvp->requestPaint();
    return dim;
}
