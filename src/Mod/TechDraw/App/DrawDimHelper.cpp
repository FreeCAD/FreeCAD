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
#include "Geometry.h"
#include "Cosmetic.h"
#include "DrawPage.h"
#include "DrawUtil.h"
#include "DrawViewDimension.h"
#include "DrawViewDimExtent.h"
#include "DrawViewPart.h"


#define HORIZONTAL 0
#define VERTICAL 1
#define LENGTH 2

using namespace TechDraw;

//All this OCC math is being done on edges(&vertices) that have been through the center/scale/mirror process.

//TODO: this needs to be exposed to Python
void DrawDimHelper::makeExtentDim(DrawViewPart* dvp,
                                  std::vector<std::string> edgeNames,
                                  int direction)
{
//    Base::Console().Message("DDH::makeExtentDim() - dvp: %s edgeNames: %d\n",
//                            dvp->Label.getValue(), edgeNames.size());
    if (!dvp) {
//        Base::Console().Message("DDH::makeExtentDim - dvp: %X\n", dvp);
        return;
    }

    std::string dimType = "DistanceX";
    int dimNum = 0;
    if (direction == VERTICAL) {
        dimType = "DistanceY";
        dimNum = 1;
    }

    std::pair<Base::Vector3d, Base::Vector3d> endPoints = minMax(dvp,
                                                                 edgeNames,
                                                                 direction);
    Base::Vector3d refMin = endPoints.first / dvp->getScale();     //unscale from geometry
    Base::Vector3d refMax = endPoints.second / dvp->getScale();

    //pause recomputes
    dvp->getDocument()->setStatus(App::Document::Status::SkipRecompute, true);

    DrawViewDimension* distDim = makeDistDim(dvp, dimType, refMin, refMax, true);
    std::string dimName = distDim->getNameInDocument();
    Base::Interpreter().runStringArg("App.activeDocument().%s.DirExtent = %d",
                                     dimName.c_str(), dimNum);
    DrawViewDimExtent* extDim = dynamic_cast<DrawViewDimExtent*>(distDim);
    extDim->Source.setValue(dvp, edgeNames);

    std::vector<std::string> subElements      = extDim->References2D.getSubValues();
    std::vector<std::string> cvTags;
    std::string tag0;
    std::string tag1;
    TechDraw::VertexPtr v0;
    TechDraw::VertexPtr v1;
    if (subElements.size() > 1) {
        int idx0 = DrawUtil::getIndexFromName(subElements[0]);
        int idx1 = DrawUtil::getIndexFromName(subElements[1]);
        v0 = dvp->getProjVertexByIndex(idx0);
        v1 = dvp->getProjVertexByIndex(idx1);
        if (v0 && !v0->cosmeticTag.empty()) {
            tag0 = v0->cosmeticTag;
        }
        if (v1 && !v1->cosmeticTag.empty()) {
            tag1 = v1->cosmeticTag;
        }
        cvTags.push_back(tag0);
        cvTags.push_back(tag1);
        extDim->CosmeticTags.setValues(cvTags);
    }

    //continue recomputes
    dvp->getDocument()->setStatus(App::Document::Status::SkipRecompute, false);
    extDim->recomputeFeature();
}

std::pair<Base::Vector3d, Base::Vector3d> DrawDimHelper::minMax(DrawViewPart* dvp,
                                  std::vector<std::string> edgeNames,
                                  int direction)
{
//    Base::Console().Message("DDH::minMax()\n");
    std::pair<Base::Vector3d, Base::Vector3d> result;
    Base::Vector3d refMin;
    Base::Vector3d refMax;

    gp_Pnt stdOrg(0.0, 0.0, 0.0);
    gp_Dir stdZ(0.0, 0.0, 1.0);
    gp_Dir stdX(1.0, 0.0, 0.0);
    gp_Ax3 projAx3(stdOrg, stdZ, stdX);
    gp_Pln projPlane(projAx3);                     // OZX

    BaseGeomPtrVector bgList;
    if (!edgeNames.empty()) {
        for (auto& n: edgeNames) {
            if (!n.empty()) {
                std::string geomType = DrawUtil::getGeomTypeFromName(n);
                if (!n.empty() && (geomType == "Edge")) {
                    int i = DrawUtil::getIndexFromName(n);
                    BaseGeomPtr bg = dvp->getGeomByIndex(i);
                    if (bg) {
                        bgList.push_back(bg);
                    }
                }
            }
        }
    }

    BaseGeomPtrVector selEdges = bgList;
    if (selEdges.empty()) {
        selEdges = dvp->getEdgeGeometry();                  //do the whole View
    }

    Bnd_Box edgeBbx;
    edgeBbx.SetGap(1.0);     //make the box a bit bigger

    std::vector<TopoDS_Edge> inEdges;
    for (auto& bg: selEdges) {
        inEdges.push_back(bg->occEdge);
        BRepBndLib::Add(bg->occEdge, edgeBbx);
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
        gp_Pnt leftPoint = findClosestPoint(inEdges,
                                            edgeLeft);
        Handle(Geom_Line) lineRight = new Geom_Line(rightMid, yDir);
        BRepBuilderAPI_MakeEdge mkEdgeRight(lineRight);
        TopoDS_Edge edgeRight = mkEdgeRight.Edge();
        gp_Pnt rightPoint = findClosestPoint(inEdges,
                                             edgeRight);

        refMin = Base::Vector3d(leftPoint.X(), leftPoint.Y(), 0.0);
        refMax = Base::Vector3d(rightPoint.X(), rightPoint.Y(), 0.0);

    } else if (direction == VERTICAL) {
        Handle(Geom_Line) lineBottom = new Geom_Line(bottomMid, xDir);
        BRepBuilderAPI_MakeEdge mkEdgeBottom(lineBottom);
        TopoDS_Edge edgeBottom = mkEdgeBottom.Edge();
        gp_Pnt bottomPoint = findClosestPoint(inEdges,
                                              edgeBottom);
        Handle(Geom_Line) lineTop = new Geom_Line(topMid, xDir);
        BRepBuilderAPI_MakeEdge mkEdgeTop(lineTop);
        TopoDS_Edge edgeTop = mkEdgeTop.Edge();
        gp_Pnt topPoint = findClosestPoint(inEdges,
                                           edgeTop);
        refMin = Base::Vector3d(bottomPoint.X(), bottomPoint.Y(), 0.0);
        refMax = Base::Vector3d(topPoint.X(), topPoint.Y(), 0.0);
    }

    result.first = refMin;
    result.second = refMax;
    return result;
}

//given list of curves, find the closest point from any curve to a boundary
//computation intensive for a cosmetic result.
gp_Pnt DrawDimHelper::findClosestPoint(std::vector<TopoDS_Edge> inEdges,
                                       TopoDS_Edge& boundary)
{
//    Base::Console().Message("DDH::findClosestPoint() - edges: %d\n", inEdges.size());
//
//find an extent point that is actually on one of the curves
    double minDistance(std::numeric_limits<float>::max());
    gp_Pnt nearPoint;
    for (auto& edge : inEdges) {
        BRepExtrema_DistShapeShape extss(edge, boundary);
        if (!extss.IsDone()) {
            Base::Console().Warning("DDH::findClosestPoint - BRepExtrema_DistShapeShape failed - 1\n");
            continue;
        }
        if (extss.NbSolution() == 0) {
            Base::Console().Warning("DDH::findClosestPoint - BRepExtrema_DistShapeShape failed - 2\n");
            continue;
        }
        if (extss.Value() < minDistance) {
            minDistance = extss.Value();
            nearPoint = extss.PointOnShape1(1);
        }
    }
    return nearPoint;
}

DrawViewDimension* DrawDimHelper::makeDistDim(DrawViewPart* dvp,
                                              std::string dimType,
                                              Base::Vector3d inMin,      //is this scaled or unscaled??
                                              Base::Vector3d inMax,      //expects scaled from makeExtentDim
                                              bool extent)
{
//    Base::Console().Message("DDH::makeDistDim() - inMin: %s inMax: %s\n",
//                            DrawUtil::formatVector(inMin).c_str(),
//                            DrawUtil::formatVector(inMax).c_str());
    TechDraw::DrawPage* page = dvp->findParentPage();
    std::string pageName = page->getNameInDocument();

    TechDraw::DrawViewDimension *dim = nullptr;
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
    std::vector<App::DocumentObject *> objs;
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
        Base::Interpreter().runStringArg("App.activeDocument().addObject('TechDraw::DrawViewDimExtent', '%s')",
                                         dimName.c_str());
    } else {

        Base::Interpreter().runStringArg("App.activeDocument().addObject('TechDraw::DrawViewDimension', '%s')",
                                         dimName.c_str());
    }

    Base::Interpreter().runStringArg("App.activeDocument().%s.Type = '%s'",
                                     dimName.c_str(), dimType.c_str());

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(doc->getObject(dimName.c_str()));
    if (!dim) {
        throw Base::TypeError("DDH::makeDistDim - dim not found\n");
    }
    dim->References2D.setValues(objs, subs);

    Base::Interpreter().runStringArg("App.activeDocument().%s.addView(App.activeDocument().%s)",
                                     pageName.c_str(), dimName.c_str());


    dvp->requestPaint();
    return dim;
}
