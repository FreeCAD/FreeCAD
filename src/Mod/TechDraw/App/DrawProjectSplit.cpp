/***************************************************************************
 *   Copyright (c) WandererFan            (wandererfan@gmail.com) 2016     *
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

#include <BRep_Tool.hxx>
#include <BRepGProp.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepLProp_CurveTool.hxx>
#include <BRepLProp_CLProps.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Geom_Curve.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GProp_GProps.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_XYZ.hxx>
#include <HLRBRep_Algo.hxx>
#include <HLRAlgo_Projector.hxx>
#include <HLRBRep_ShapeBounds.hxx>
#include <HLRBRep_HLRToShape.hxx>
#include <ShapeFix_ShapeTolerance.hxx>
#include <ShapeExtend_WireData.hxx>
#include <ShapeFix_Wire.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#endif

#include <limits>
#include <algorithm>
#include <cmath>
#include <GeomLib_Tool.hxx>

#include <App/Application.h>
#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>
#include <Mod/Part/App/PartFeature.h>

#include "DrawUtil.h"
#include "Geometry.h"
#include "GeometryObject.h"
#include "DrawProjectSplit.h"
#include "DrawHatch.h"
#include "EdgeWalker.h"


//#include <Mod/TechDraw/App/DrawProjectSplitPy.h>  // generated from DrawProjectSplitPy.xml

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawProjectSplit
//===========================================================================

DrawProjectSplit::DrawProjectSplit()
{
}

DrawProjectSplit::~DrawProjectSplit()
{
}

std::vector<TopoDS_Edge> DrawProjectSplit::getEdgesForWalker(TopoDS_Shape shape, double scale, Base::Vector3d direction)
{
    std::vector<TopoDS_Edge> result;
    if (shape.IsNull()) {
        return result;
    }

    BRepBuilderAPI_Copy BuilderCopy(shape);
    TopoDS_Shape copyShape = BuilderCopy.Shape();

    gp_Pnt inputCenter(0,0,0);
    TopoDS_Shape scaledShape;
    scaledShape = TechDrawGeometry::scaleShape(copyShape,
                                               scale);
    TechDrawGeometry::GeometryObject* go = buildGeometryObject(scaledShape,inputCenter,direction);
    result = getEdges(go);

    delete go;
    return result;
}


TechDrawGeometry::GeometryObject* DrawProjectSplit::buildGeometryObject(
                                           TopoDS_Shape shape, 
                                           gp_Pnt& inputCenter, 
                                           Base::Vector3d direction)
{
    TechDrawGeometry::GeometryObject* geometryObject = new TechDrawGeometry::GeometryObject("DrawProjectSplit");

    geometryObject->projectShape(shape,
                            inputCenter,
                            direction);
    geometryObject->extractGeometry(TechDrawGeometry::ecHARD,                   //always show the hard&outline visible lines
                                    true);
    geometryObject->extractGeometry(TechDrawGeometry::ecOUTLINE,
                                    true);
    return geometryObject;
}

//! get the projected edges with all their new intersections.
std::vector<TopoDS_Edge> DrawProjectSplit::getEdges(TechDrawGeometry::GeometryObject* geometryObject)
{
    const std::vector<TechDrawGeometry::BaseGeom*>& goEdges = geometryObject->getVisibleFaceEdges(true,true);
    std::vector<TechDrawGeometry::BaseGeom*>::const_iterator itEdge = goEdges.begin();
    std::vector<TopoDS_Edge> origEdges;
    for (;itEdge != goEdges.end(); itEdge++) {
        origEdges.push_back((*itEdge)->occEdge);
    }

    std::vector<TopoDS_Edge> faceEdges;
    std::vector<TopoDS_Edge> nonZero;
    for (auto& e:origEdges) {                            //drop any zero edges (shouldn't be any by now!!!)
        if (!DrawUtil::isZeroEdge(e)) {
            nonZero.push_back(e);
        } else {
            Base::Console().Message("INFO - DPS::extractFaces found ZeroEdge!\n");
        }
    }
    faceEdges = nonZero;
    origEdges = nonZero;

    //HLR algo does not provide all edge intersections for edge endpoints.
    //need to split long edges touched by Vertex of another edge
    std::vector<splitPoint> splits;
    std::vector<TopoDS_Edge>::iterator itOuter = origEdges.begin();
    int iOuter = 0;
    for (; itOuter != origEdges.end(); itOuter++, iOuter++) {
        TopoDS_Vertex v1 = TopExp::FirstVertex((*itOuter));
        TopoDS_Vertex v2 = TopExp::LastVertex((*itOuter));
        Bnd_Box sOuter;
        BRepBndLib::Add(*itOuter, sOuter);
        sOuter.SetGap(0.1);
        if (sOuter.IsVoid()) {
            Base::Console().Message("DPS::Extract Faces - outer Bnd_Box is void\n");
            continue;
        }
        if (DrawUtil::isZeroEdge(*itOuter)) {
            Base::Console().Message("DPS::extractFaces - outerEdge: %d is ZeroEdge\n",iOuter);   //this is not finding ZeroEdges
            continue;  //skip zero length edges. shouldn't happen ;)
        }
        int iInner = 0;
        std::vector<TopoDS_Edge>::iterator itInner = faceEdges.begin();
        for (; itInner != faceEdges.end(); itInner++,iInner++) {
            if (iInner == iOuter) {
                continue;
            }
            if (DrawUtil::isZeroEdge((*itInner))) {
                continue;  //skip zero length edges. shouldn't happen ;)
            }

            Bnd_Box sInner;
            BRepBndLib::Add(*itInner, sInner);
            sInner.SetGap(0.1);
            if (sInner.IsVoid()) {
                Base::Console().Log("INFO - DPS::Extract Faces - inner Bnd_Box is void\n");
                continue;
            }
            if (sOuter.IsOut(sInner)) {      //bboxes of edges don't intersect, don't bother
                continue;
            }

            double param = -1;
            if (isOnEdge((*itInner),v1,param,false)) {
                gp_Pnt pnt1 = BRep_Tool::Pnt(v1);
                splitPoint s1;
                s1.i = iInner;
                s1.v = Base::Vector3d(pnt1.X(),pnt1.Y(),pnt1.Z());
                s1.param = param;
                splits.push_back(s1);
            }
            if (isOnEdge((*itInner),v2,param,false)) {
                gp_Pnt pnt2 = BRep_Tool::Pnt(v2);
                splitPoint s2;
                s2.i = iInner;
                s2.v = Base::Vector3d(pnt2.X(),pnt2.Y(),pnt2.Z());
                s2.param = param;
                splits.push_back(s2);
            }
        } //inner loop
    }   //outer loop

    std::vector<splitPoint> sorted = sortSplits(splits,true);
    auto last = std::unique(sorted.begin(), sorted.end(), DrawProjectSplit::splitEqual);  //duplicates to back
    sorted.erase(last, sorted.end());                         //remove dupls
    std::vector<TopoDS_Edge> newEdges = splitEdges(faceEdges,sorted);

    if (newEdges.empty()) {
        Base::Console().Log("LOG - DPS::extractFaces - no newEdges\n");
    }
    return newEdges;
}


double DrawProjectSplit::simpleMinDist(TopoDS_Shape s1, TopoDS_Shape s2)
{
    Standard_Real minDist = -1;

    BRepExtrema_DistShapeShape extss(s1, s2);
    if (!extss.IsDone()) {
        Base::Console().Message("FE - BRepExtrema_DistShapeShape failed");
        return -1;
    }
    int count = extss.NbSolution();
    if (count != 0) {
        minDist = extss.Value();
    } else {
        minDist = -1;
    }
    return minDist;
}


//this routine is the big time consumer.  gets called many times (and is slow?))
//note param gets modified here
bool DrawProjectSplit::isOnEdge(TopoDS_Edge e, TopoDS_Vertex v, double& param, bool allowEnds)
{
    bool result = false;
    bool outOfBox = false;
    param = -2;

    //eliminate obvious cases
    Bnd_Box sBox;
    BRepBndLib::Add(e, sBox);
    sBox.SetGap(0.1);
    if (sBox.IsVoid()) {
        Base::Console().Message("DPS::isOnEdge - Bnd_Box is void\n");
    } else {
        gp_Pnt pt = BRep_Tool::Pnt(v);
        if (sBox.IsOut(pt)) {
            outOfBox = true;
        }
    }
    if (!outOfBox) {
            double dist = simpleMinDist(v,e);
            if (dist < 0.0) {
                Base::Console().Error("DPS::isOnEdge - simpleMinDist failed: %.3f\n",dist);
                result = false;
            } else if (dist < Precision::Confusion()) {
                const gp_Pnt pt = BRep_Tool::Pnt(v);                         //have to duplicate method 3 to get param
                BRepAdaptor_Curve adapt(e);
                const Handle_Geom_Curve c = adapt.Curve().Curve();
                double maxDist = 0.000001;     //magic number.  less than this gives false positives.
                //bool found =
                (void) GeomLib_Tool::Parameter(c,pt,maxDist,param);  //already know point it on curve
                result = true;
            }
            if (result) {
                TopoDS_Vertex v1 = TopExp::FirstVertex(e);
                TopoDS_Vertex v2 = TopExp::LastVertex(e);
                if (DrawUtil::isSamePoint(v,v1) || DrawUtil::isSamePoint(v,v2)) {
                    if (!allowEnds) {
                        result = false;
                    }
                }
            }
    } //!outofbox
    return result;
}


std::vector<TopoDS_Edge> DrawProjectSplit::splitEdges(std::vector<TopoDS_Edge> edges, std::vector<splitPoint> splits)
{
    std::vector<TopoDS_Edge> result;
    std::vector<TopoDS_Edge> newEdges;
    std::vector<splitPoint> edgeSplits;      //splits for current edge
    int iEdge = 0; //current edge index
    int iSplit = 0; //current splitindex
    int ii = 0;     //i value of current split
    int endEdge = edges.size();
    int endSplit = splits.size();
    int imax = std::numeric_limits<int>::max();

    while ((iEdge < endEdge) )  {
        if (iSplit < endSplit) {
            ii = splits[iSplit].i;
        } else {
            ii = imax;
        }
        if (ii == iEdge) {
            edgeSplits.push_back(splits[iSplit]);
            iSplit++;
        } else if (ii > iEdge) {
            if (!edgeSplits.empty()) {                          //save *iedge's splits
                newEdges = split1Edge(edges[iEdge],edgeSplits);
                result.insert(result.end(), newEdges.begin(), newEdges.end());
                edgeSplits.clear();
            } else {
                result.push_back(edges[iEdge]);                //save *iedge
            }
            iEdge++;                                           //next edge
        } else if (iEdge > ii) {
            iSplit++;
        }
    }

    if (!edgeSplits.empty()) {                                           //handle last batch
        newEdges = split1Edge(edges[iEdge],edgeSplits);
        result.insert(result.end(), newEdges.begin(), newEdges.end());
        edgeSplits.clear();
    }

    return result;
}


std::vector<TopoDS_Edge> DrawProjectSplit::split1Edge(TopoDS_Edge e, std::vector<splitPoint> splits)
{
    std::vector<TopoDS_Edge> result;
    if (splits.empty()) {
        return result;
    }

    BRepAdaptor_Curve adapt(e);
    Handle_Geom_Curve c = adapt.Curve().Curve();
    double first = BRepLProp_CurveTool::FirstParameter(adapt);
    double last = BRepLProp_CurveTool::LastParameter(adapt);
    if (first > last) {
        //TODO parms.reverse();
        Base::Console().Message("DPS::split1Edge - edge is backwards!\n");
        return result;
    }
    std::vector<double> parms;
    parms.push_back(first);
    for (auto& s:splits) {
        parms.push_back(s.param);
    }

    parms.push_back(last);
    std::vector<double>::iterator pfirst = parms.begin();
    auto parms2 = parms.begin() + 1;
    std::vector<double>::iterator psecond = parms2;
    std::vector<double>::iterator pstop = parms.end();
    for (; psecond != pstop; pfirst++,psecond++) {
        try {
            BRepBuilderAPI_MakeEdge mkEdge(c, *pfirst, *psecond);
            if (mkEdge.IsDone()) {
                TopoDS_Edge e1 = mkEdge.Edge();
                result.push_back(e1);
            }
        }
        catch (Standard_Failure) {
            Base::Console().Message("LOG - DPS::split1Edge failed building edge segment\n");
        }
    }
    return result;
}

std::vector<splitPoint> DrawProjectSplit::sortSplits(std::vector<splitPoint>& s, bool ascend)
{
    std::vector<splitPoint> sorted = s;
    std::sort(sorted.begin(), sorted.end(), DrawProjectSplit::splitCompare);
    if (ascend) {
        std::reverse(sorted.begin(),sorted.end());
    }
    return sorted;
}

//return true if p1 "is greater than" p2
/*static*/bool DrawProjectSplit::splitCompare(const splitPoint& p1, const splitPoint& p2)
{
    bool result = false;
    if (p1.i > p2.i) {
        result = true;
    } else if (p1.i < p2.i) {
        result = false;
    } else if (p1.param > p2.param) {
        result = true;
    } else if (p1.param < p2.param) {
        result = false;
    }
    return result;
}

//return true if p1 "is equal to" p2
/*static*/bool DrawProjectSplit::splitEqual(const splitPoint& p1, const splitPoint& p2)
{
    bool result = false;
    if ((p1.i == p2.i) &&
        (fabs(p1.param - p2.param) < Precision::Confusion())) {
        result = true;
    }
    return result;
}


