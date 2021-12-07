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
# include <sstream>
# include <cstring>
# include <cstdlib>
#include <cmath>
#include <float.h>
#include <string>
# include <exception>

#include <Precision.hxx>
#include <Bnd_Box2d.hxx>
#include <BndLib_Add2dCurve.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <Extrema_ExtCC2d.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2dAPI_ProjectPointOnCurve.hxx>
#include <GeomAPI.hxx>
#include <Geom_Curve.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Pln.hxx>
#include <TopoDS_Edge.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>
#include <Base/Vector3D.h>

#include "Geometry.h"
#include "DrawUtil.h"
#include "Cosmetic.h"
#include "DrawPage.h"
#include "DrawViewPart.h"
#include "DrawViewDimension.h"
#include "DrawViewDimExtent.h"

#include "DrawDimHelper.h"

#define HORIZONTAL 0
#define VERTICAL 1
#define LENGTH 2

using namespace TechDraw;

hTrimCurve::hTrimCurve(Handle(Geom2d_Curve) hCurveIn,
                        double parm1,
                        double parm2) :
    hCurve(hCurveIn),
    first(parm1),
    last(parm2)
{
    //just a convenient struct for now.
}

//All this OCC math is being done on edges(&vertices) that have been through the center/scale/mirror process.

//TODO: this needs to be exposed to Python
void DrawDimHelper::makeExtentDim(DrawViewPart* dvp,
                                  std::vector<std::string> edgeNames,
                                  int direction)
{
//    Base::Console().Message("DDH::makeExtentDim() - dvp: %s edgeNames: %d\n",
//                            dvp->Label.getValue(), edgeNames.size());
    if (dvp == nullptr) {
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
        if ( (v0 != nullptr) &&
             (!v0->cosmeticTag.empty()) ) {
            tag0 = v0->cosmeticTag;
        }
        if ( (v1 != nullptr) &&
             (!v1->cosmeticTag.empty()) ) {
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

    std::vector<BaseGeom*> bgList;
    if (!edgeNames.empty()) {
        for (auto& n: edgeNames) {
            if (!n.empty()) {
                std::string geomType = DrawUtil::getGeomTypeFromName(n);
                if (!n.empty() && (geomType == "Edge")) {
                    int i = DrawUtil::getIndexFromName(n);
                    BaseGeom* bg = dvp->getGeomByIndex(i);
                    if (bg != nullptr) {
                        bgList.push_back(bg);
                    }
                }
            }
        }
    }

    std::vector<BaseGeom*> selEdges = bgList;
    if (selEdges.empty()) {
        selEdges = dvp->getEdgeGeometry();                  //do the whole View
    }

    Bnd_Box edgeBbx;
    edgeBbx.SetGap(0.0);

    std::vector<Handle(Geom_Curve)> selCurves;
    std::vector<hTrimCurve> hTCurve2dList;
    for (auto& bg: selEdges) {
        TopoDS_Edge e = bg->occEdge;
        BRepBndLib::Add(e, edgeBbx);
        double first = 0.0;
        double last = 0.0;
        Handle(Geom_Curve) hCurve = BRep_Tool::Curve(e, first, last);
        Handle(Geom2d_Curve) hCurve2 = GeomAPI::To2d (hCurve, projPlane);
        hTrimCurve temp(hCurve2, first, last);
        hTCurve2dList.push_back(temp);
    }

    //can't use Bnd_Box2d here as BndLib_Add2dCurve::Add adds the poles of splines to the box.
    //poles are not necessarily on the curve! 3d Bnd_Box does it properly. 
    //this has to be the bbx of the selected edges, not the dvp!!!
    double minX, minY, minZ, maxX, maxY, maxZ;
    edgeBbx.Get(minX, minY, minZ, maxX, maxY, maxZ);
    double xMid = (maxX + minX) / 2.0;
    double yMid = (maxY + minY) / 2.0;

    gp_Pnt2d rightMid(maxX, yMid);
    gp_Pnt2d leftMid(minX, yMid);
    gp_Pnt2d topMid(xMid, maxY);
    gp_Pnt2d bottomMid(xMid, minY);

    gp_Dir2d xDir(1.0, 0.0);
    gp_Dir2d yDir(0.0, 1.0);

    if (direction == HORIZONTAL) {
        Handle(Geom2d_Line) boundaryLeft  = new Geom2d_Line(leftMid, yDir);
        gp_Pnt2d leftPoint = findClosestPoint(hTCurve2dList,
                                              boundaryLeft);
        Handle(Geom2d_Line) boundaryRight  = new Geom2d_Line(rightMid, yDir);
        gp_Pnt2d rightPoint = findClosestPoint(hTCurve2dList,
                                               boundaryRight);

        refMin = Base::Vector3d(leftPoint.X(), leftPoint.Y(), 0.0);
        refMax = Base::Vector3d(rightPoint.X(), rightPoint.Y(), 0.0);

    } else if (direction == VERTICAL) {
        Handle(Geom2d_Line) boundaryBottom = new Geom2d_Line(bottomMid, xDir);
        gp_Pnt2d bottomPoint = findClosestPoint(hTCurve2dList,
                                                boundaryBottom);
        Handle(Geom2d_Line) boundaryTop = new Geom2d_Line(topMid, xDir);
        gp_Pnt2d topPoint = findClosestPoint(hTCurve2dList,
                                             boundaryTop);
        refMin = Base::Vector3d(bottomPoint.X(),bottomPoint.Y(), 0.0);
        refMax = Base::Vector3d(topPoint.X(), topPoint.Y(), 0.0);
    }

    result.first = refMin;
    result.second = refMax;
    return result;
}

//given list of curves, find the closest point from any curve to a boundary
//computation intensive for a cosmetic result.
gp_Pnt2d DrawDimHelper::findClosestPoint(std::vector<hTrimCurve> hTCurve2dList,
                                         Handle(Geom2d_Curve) boundary)
{
//    Base::Console().Message("DDH::findClosestPoint() - curves: %d\n", hTCurve2dList.size());
//
//find an extent point that is actually on one of the curves
    gp_Pnt2d result(-1.0, -1.0);
    Geom2dAdaptor_Curve aBoundary(boundary);

    double globalNearDist = FLT_MAX;
    gp_Pnt2d globalNearPoint;
    bool found = false;
    for (auto& hCurve2d : hTCurve2dList) {
        Geom2dAdaptor_Curve aCurve(hCurve2d.hCurve,
                                   hCurve2d.first,
                                   hCurve2d.last);
        Extrema_ExtCC2d mkExtr(aBoundary, aCurve);
        if (mkExtr.IsDone()) {
            double nearDist = FLT_MAX;
            int nearIdx = 0;
            gp_Pnt2d nearPoint;
            if (mkExtr.NbExt() > 0) {
                int stop = mkExtr.NbExt();
                int i = 1;                      //this is OCC start (1) not conventional start (0)
                for ( ; i <= stop; i++) {
                    double dist = mkExtr.SquareDistance(i);
                    if (dist < nearDist) {
                        found = true;
                        nearDist = dist;
                        nearIdx = i;
                        Extrema_POnCurv2d p1, p2;
                        mkExtr.Points(nearIdx, p1, p2);
                        nearPoint = p2.Value();
                    }
                }
            } else {                             //no extrema? - might be a line parallel to boundary
                if (mkExtr.IsParallel()) {
                    //get midpoint of aCurve
                    double mid   = (hCurve2d.first + hCurve2d.last) / 2.0;
                    gp_Pnt2d midPoint = hCurve2d.hCurve->Value(mid);
                    //get distance midpoint to boundary => nearDist
                    Geom2dAPI_ProjectPointOnCurve mkProj(midPoint, boundary);
                    double dist = mkProj.LowerDistance() * mkProj.LowerDistance();
                    if (dist < nearDist) {
                        found = true;
                        nearDist = dist;
                        nearPoint = mkProj.NearestPoint();
                    }
                } else {  //skew and no extrema?
                    gp_Pnt2d pFirst = hCurve2d.hCurve->Value(hCurve2d.first);
                    Geom2dAPI_ProjectPointOnCurve mkFirst(pFirst, boundary);
                    double dist1 = mkFirst.LowerDistance() * mkFirst.LowerDistance();
                    gp_Pnt2d pLast  = hCurve2d.hCurve->Value(hCurve2d.last);
                    Geom2dAPI_ProjectPointOnCurve mkLast(pLast, boundary);
                    double dist2 = mkLast.LowerDistance() * mkLast.LowerDistance();
                    if (dist1 < dist2) {
                        if (dist1 < nearDist) {
                            found = true;
                            nearDist = dist1;
                            nearPoint = mkFirst.NearestPoint();
                        }
                    } else {
                        if (dist2 < nearDist) {
                            found = true;
                            nearDist = dist2;
                            nearPoint = mkLast.NearestPoint();
                        }
                    }
                }
            }
            if (nearDist < globalNearDist) {
                globalNearDist = nearDist;
                globalNearPoint = nearPoint;
            }
        }
    }
    if (found) {
        result = globalNearPoint;
    }

    return result;
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

    TechDraw::DrawViewDimension *dim = 0;
    App::Document* doc = dvp->getDocument();
    std::string dimName = doc->getUniqueObjectName("Dimension");
    if (extent) {
        dimName = doc->getUniqueObjectName("DimExtent");
    }

    Base::Vector3d cleanMin = DrawUtil::invertY(inMin);
    std::string tag1 = dvp->addCosmeticVertex(cleanMin);
    int iGV1 = dvp->add1CVToGV(tag1);
    
    Base::Vector3d cleanMax = DrawUtil::invertY(inMax);
    std::string tag2 = dvp->addCosmeticVertex(cleanMax);
    int iGV2 = dvp->add1CVToGV(tag2);

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
        Base::Interpreter().runStringArg("App.activeDocument().addObject('TechDraw::DrawViewDimExtent','%s')",
                                         dimName.c_str());
    } else {

        Base::Interpreter().runStringArg("App.activeDocument().addObject('TechDraw::DrawViewDimension','%s')",
                                         dimName.c_str());
    }

    Base::Interpreter().runStringArg("App.activeDocument().%s.Type = '%s'",
                                     dimName.c_str(), dimType.c_str());

    Base::Interpreter().runStringArg("App.activeDocument().%s.addView(App.activeDocument().%s)",
                                     pageName.c_str(),dimName.c_str());

    dim = dynamic_cast<TechDraw::DrawViewDimension *>(doc->getObject(dimName.c_str()));
    if (!dim) {
        throw Base::TypeError("DDH::makeDistDim - dim not found\n");
    }

    dim->References2D.setValues(objs, subs);

    dvp->requestPaint();
    dim->recomputeFeature();

    return dim;
}
