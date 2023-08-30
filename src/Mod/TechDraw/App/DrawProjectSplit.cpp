/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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
# include <algorithm>
# include <limits>
# include <sstream>
#include <Bnd_Box.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepLProp_CurveTool.hxx>
#include <Geom_Curve.hxx>
#include <GeomLib_Tool.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#endif

#include <Base/Console.h>
#include <Base/Parameter.h>

#include "DrawProjectSplit.h"
#include "DrawUtil.h"
#include "Geometry.h"
#include "GeometryObject.h"
#include "ShapeUtils.h"


using namespace TechDraw;

//===========================================================================
// DrawProjectSplit
//===========================================================================

DrawProjectSplit::DrawProjectSplit()
{
}

DrawProjectSplit::~DrawProjectSplit()
{
}

//make a projection of shape and return the edges
//used by python outline routines
std::vector<TopoDS_Edge> DrawProjectSplit::getEdgesForWalker(TopoDS_Shape shape, double scale, Base::Vector3d direction)
{
    std::vector<TopoDS_Edge> edgesIn;
    if (shape.IsNull()) {
        return edgesIn;
    }

    BRepBuilderAPI_Copy BuilderCopy(shape);
    TopoDS_Shape copyShape = BuilderCopy.Shape();

    gp_Pnt inputCenter(0, 0, 0);
    TopoDS_Shape scaledShape;
    scaledShape = ShapeUtils::scaleShape(copyShape,
                                       scale);
    gp_Ax2 viewAxis = ShapeUtils::legacyViewAxis1(Base::Vector3d(0.0, 0.0, 0.0), direction, false);
    TechDraw::GeometryObjectPtr go = buildGeometryObject(scaledShape, viewAxis);
    const std::vector<TechDraw::BaseGeomPtr>& goEdges = go->getVisibleFaceEdges(false, false);
    for (auto& e: goEdges){
        edgesIn.push_back(e->getOCCEdge());
    }

    std::vector<TopoDS_Edge> nonZero;
    for (auto& e: edgesIn) {                            //drop any zero edges (shouldn't be any by now!!!)
        if (!DrawUtil::isZeroEdge(e, 2.0 * EWTOLERANCE)) {
            nonZero.push_back(e);
        } else {
            Base::Console().Message("DPS::getEdgesForWalker found ZeroEdge!\n");
        }
    }

    return nonZero;
}

//project the shape using viewAxis (coordinate system) and return a geometry object
TechDraw::GeometryObjectPtr DrawProjectSplit::buildGeometryObject(TopoDS_Shape shape,
                                                                        const gp_Ax2& viewAxis)
{
    TechDraw::GeometryObjectPtr geometryObject(std::make_shared<TechDraw::GeometryObject>("DrawProjectSplit", nullptr));

    if (geometryObject->usePolygonHLR()){
        geometryObject->projectShapeWithPolygonAlgo(shape, viewAxis);
    }
    else{
        //note that this runs in the main thread, unlike DrawViewPart
        geometryObject->projectShape(shape, viewAxis);
    }
    return geometryObject;
}

//! get the projected edges with all their new intersections.
std::vector<TopoDS_Edge> DrawProjectSplit::getEdges(TechDraw::GeometryObject* geometryObject)
{
    const std::vector<TechDraw::BaseGeomPtr>& goEdges = geometryObject->getVisibleFaceEdges(true, true);
    std::vector<TechDraw::BaseGeomPtr>::const_iterator itEdge = goEdges.begin();
    std::vector<TopoDS_Edge> origEdges;
    for (;itEdge != goEdges.end(); itEdge++) {
        origEdges.push_back((*itEdge)->getOCCEdge());
    }

    std::vector<TopoDS_Edge> faceEdges;
    std::vector<TopoDS_Edge> nonZero;
    for (auto& e:origEdges) {                            //drop any zero edges (shouldn't be any by now!!!)
        if (!DrawUtil::isZeroEdge(e, 2.0 * EWTOLERANCE)) {
            nonZero.push_back(e);
        }
    }
    faceEdges = nonZero;
    origEdges = nonZero;

    //HLR algo does not provide all edge intersections for edge endpoints.
    //need to split long edges touched by Vertex of another edge
    std::vector<splitPoint> splits;
    std::vector<TopoDS_Edge>::iterator itOuter = origEdges.begin();
    int iOuter = 0;
    for (; itOuter != origEdges.end(); ++itOuter, iOuter++) {
        TopoDS_Vertex v1 = TopExp::FirstVertex((*itOuter));
        TopoDS_Vertex v2 = TopExp::LastVertex((*itOuter));
        Bnd_Box sOuter;
        BRepBndLib::AddOptimal(*itOuter, sOuter);
        sOuter.SetGap(0.1);
        if (sOuter.IsVoid()) {
            continue;
        }
        if (DrawUtil::isZeroEdge(*itOuter)) {
            continue;  //skip zero length edges. shouldn't happen ;)
        }
        int iInner = 0;
        std::vector<TopoDS_Edge>::iterator itInner = faceEdges.begin();
        for (; itInner != faceEdges.end(); ++itInner, iInner++) {
            if (iInner == iOuter) {
                continue;
            }
            if (DrawUtil::isZeroEdge((*itInner))) {
                continue;  //skip zero length edges. shouldn't happen ;)
            }

            Bnd_Box sInner;
            BRepBndLib::AddOptimal(*itInner, sInner);
            sInner.SetGap(0.1);
            if (sInner.IsVoid()) {
                continue;
            }
            if (sOuter.IsOut(sInner)) {      //bboxes of edges don't intersect, don't bother
                continue;
            }

            double param = -1;
            if (isOnEdge((*itInner), v1, param, false)) {
                gp_Pnt pnt1 = BRep_Tool::Pnt(v1);
                splitPoint s1;
                s1.i = iInner;
                s1.v = Base::Vector3d(pnt1.X(), pnt1.Y(), pnt1.Z());
                s1.param = param;
                splits.push_back(s1);
            }
            if (isOnEdge((*itInner), v2, param, false)) {
                gp_Pnt pnt2 = BRep_Tool::Pnt(v2);
                splitPoint s2;
                s2.i = iInner;
                s2.v = Base::Vector3d(pnt2.X(), pnt2.Y(), pnt2.Z());
                s2.param = param;
                splits.push_back(s2);
            }
        } //inner loop
    }   //outer loop

    std::vector<splitPoint> sorted = sortSplits(splits, true);
    auto last = std::unique(sorted.begin(), sorted.end(), DrawProjectSplit::splitEqual);  //duplicates to back
    sorted.erase(last, sorted.end());                         //remove dupls
    std::vector<TopoDS_Edge> newEdges = splitEdges(faceEdges, sorted);

    if (!newEdges.empty()) {
        newEdges = removeDuplicateEdges(newEdges);
    }
    return newEdges;
}


//this routine is the big time consumer.  gets called many times (and is slow?))
//note param gets modified here
bool DrawProjectSplit::isOnEdge(TopoDS_Edge e, TopoDS_Vertex v, double& param, bool allowEnds)
{
    param = -2;

    //eliminate obvious cases
    Bnd_Box sBox;
    BRepBndLib::AddOptimal(e, sBox);
    sBox.SetGap(0.1);
    if (!sBox.IsVoid()) {
        gp_Pnt pt = BRep_Tool::Pnt(v);
        if (sBox.IsOut(pt)) {
            return false;  // Out of box
        }
    }

    double dist = DrawUtil::simpleMinDist(v, e);
    if (dist < 0.0) {
        Base::Console().Error("DPS::isOnEdge - simpleMinDist failed: %.3f\n", dist);
        return false;
    } else if (dist < Precision::Confusion()) {
        const gp_Pnt pt = BRep_Tool::Pnt(v);                         //have to duplicate method 3 to get param
        BRepAdaptor_Curve adapt(e);
        const Handle(Geom_Curve) c = adapt.Curve().Curve();
        double maxDist = 0.000001;     //magic number.  less than this gives false positives.
        //bool found =
        (void) GeomLib_Tool::Parameter(c, pt, maxDist, param);  //already know point it on curve

        TopoDS_Vertex v1 = TopExp::FirstVertex(e);
        TopoDS_Vertex v2 = TopExp::LastVertex(e);
        if (DrawUtil::isSamePoint(v, v1) || DrawUtil::isSamePoint(v, v2)) {
            if (!allowEnds) {
                return false;
            }
        }
        return true;
    }

    return false;
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

    while (iEdge < endEdge)  {
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
                newEdges = split1Edge(edges[iEdge], edgeSplits);
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
        newEdges = split1Edge(edges[iEdge], edgeSplits);
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
    Handle(Geom_Curve) c = adapt.Curve().Curve();
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
    for (; psecond != pstop; ++pfirst, ++psecond) {
        try {
            BRepBuilderAPI_MakeEdge mkEdge(c, *pfirst, *psecond);
            if (mkEdge.IsDone()) {
                TopoDS_Edge e1 = mkEdge.Edge();
                result.push_back(e1);
            }
        }
        catch (Standard_Failure&) {
            Base::Console().Message("DPS::split1Edge failed building edge segment\n");
        }
    }
    return result;
}

std::vector<splitPoint> DrawProjectSplit::sortSplits(std::vector<splitPoint>& s, bool ascend)
{
    std::vector<splitPoint> sorted = s;
    std::sort(sorted.begin(), sorted.end(), DrawProjectSplit::splitCompare);
    if (ascend) {
        std::reverse(sorted.begin(), sorted.end());
    }
    return sorted;
}

//return true if p1 "is greater than" p2
/*static*/bool DrawProjectSplit::splitCompare(const splitPoint& p1, const splitPoint& p2)
{
    if (p1.i > p2.i) {
        return true;
    } else if (p1.i < p2.i) {
        return false;
    } else if (p1.param > p2.param) {
        return true;
    } else if (p1.param < p2.param) {
        return false;
    }
    return false;
}

//return true if p1 "is equal to" p2
/*static*/bool DrawProjectSplit::splitEqual(const splitPoint& p1, const splitPoint& p2)
{
    if (p1.i == p2.i &&
        fabs(p1.param - p2.param) < Precision::Confusion()) {
        return true;
    }
    return false;
}

std::vector<TopoDS_Edge> DrawProjectSplit::removeDuplicateEdges(std::vector<TopoDS_Edge>& inEdges)
{
    std::vector<TopoDS_Edge> result;
    std::vector<edgeSortItem> temp;

    unsigned int idx = 0;
    for (auto& e: inEdges) {
        edgeSortItem item;
        TopoDS_Vertex v1 = TopExp::FirstVertex(e);
        TopoDS_Vertex v2 = TopExp::LastVertex(e);
        item.start = DrawUtil::vertex2Vector(v1);
        item.end   = DrawUtil::vertex2Vector(v2);
        item.startAngle = DrawUtil::angleWithX(e, v1);
        item.endAngle = DrawUtil::angleWithX(e, v2);
        //catch reverse-duplicates
        if (DrawUtil::vectorLess(item.end, item.start)) {
             Base::Vector3d vTemp = item.start;
             item.start  = item.end;
             item.end    = vTemp;
             double aTemp = item.startAngle;
             item.startAngle = item.endAngle;
             item.endAngle = aTemp;
        }
        item.idx = idx;
        temp.push_back(item);
        idx++;
    }

    std::vector<edgeSortItem> sorted = sortEdges(temp, true);
    auto last = std::unique(sorted.begin(), sorted.end(), edgeSortItem::edgeEqual);  //duplicates to back
    sorted.erase(last, sorted.end());                         //remove dupls

    for (const auto& e: sorted) {
        if (e.idx < inEdges.size()) {
            result.push_back(inEdges.at(e.idx));
        } else {
            Base::Console().Message("ERROR - DPS::removeDuplicateEdges - access: %d inEdges: %d\n", e.idx, inEdges.size());
            //TODO: throw index error
        }
    }
    return result;
}

std::vector<edgeSortItem> DrawProjectSplit::sortEdges(std::vector<edgeSortItem>& e, bool ascend)
{
    std::vector<edgeSortItem> sorted = e;
    std::sort(sorted.begin(), sorted.end(), edgeSortItem::edgeLess);
    if (ascend) {
        std::reverse(sorted.begin(), sorted.end());
    }
    return sorted;
}


//*************************
//* edgeSortItem Methods
//*************************
std::string edgeSortItem::dump()
{
    std::string result;
    std::stringstream builder;
    builder << "edgeSortItem - s: " << DrawUtil::formatVector(start)  << " e: " << DrawUtil::formatVector(end) <<
                              " sa: " << startAngle * 180.0/M_PI << " ea: " << endAngle* 180.0/M_PI << " idx: " << idx;
    return builder.str();
}


//true if "e1 < e2" - for sorting
/*static*/bool edgeSortItem::edgeLess(const edgeSortItem& e1, const edgeSortItem& e2)
{
    if (!((e1.start - e2.start).Length() < Precision::Confusion())) {  //e1 != e2
        if (DrawUtil::vectorLess(e1.start, e2.start)) {
            return true;
        }
    } else if (!DrawUtil::fpCompare(e1.startAngle, e2.startAngle)) {
        if (e1.startAngle < e2.startAngle) {
            return true;
        }
    } else if (!DrawUtil::fpCompare(e1.endAngle, e2.endAngle)) {
        if (e1.endAngle < e2.endAngle) {
            return true;
        }
    } else if (e1.idx < e2.idx) {
        return true;
    }
    return false;
}

//true if "e1 = e2" - for sorting/unique test
/*static*/bool edgeSortItem::edgeEqual(const edgeSortItem& e1, const edgeSortItem& e2)
{
    double startDif = (e1.start - e2.start).Length();
    double endDif   = (e1.end   - e2.end).Length();
    if (
        startDif < Precision::Confusion() &&
        endDif   < Precision::Confusion() &&
        DrawUtil::fpCompare(e1.startAngle, e2.startAngle) &&
        DrawUtil::fpCompare(e1.endAngle, e2.endAngle)
    ) {
        return true;
    }
    return false;
}

//*****************************************
// routines for revised face finding approach
//*****************************************

//clean an unstructured group of edges so they can be connected into sensible closed faces.
// Warning: uses loose tolerances to create connections between edges
std::vector<TopoDS_Edge> DrawProjectSplit::scrubEdges(const std::vector<TechDraw::BaseGeomPtr>& origEdges,
                                                      std::vector<TopoDS_Edge> &closedEdges)
{
//    Base::Console().Message("DPS::scrubEdges() - BaseGeom in: %d\n", origEdges.size());
    //make a copy of the input edges so the loose tolerances of face finding are
    //not applied to the real edge geometry.  See TopoDS_Shape::TShape().
    std::vector<TopoDS_Edge> copyEdges;
    bool copyGeometry = true;
    bool copyMesh = false;
    for (const auto& tdEdge: origEdges) {
        if (!DrawUtil::isZeroEdge(tdEdge->getOCCEdge(), 2.0 * EWTOLERANCE)) {
            BRepBuilderAPI_Copy copier(tdEdge->getOCCEdge(), copyGeometry, copyMesh);
            copyEdges.push_back(TopoDS::Edge(copier.Shape()));
        }
    }
    return scrubEdges(copyEdges, closedEdges);
}

//origEdges should be a copy of the original edges since the underlying TShape will be modified.
std::vector<TopoDS_Edge> DrawProjectSplit::scrubEdges(std::vector<TopoDS_Edge>& origEdges,
                                                      std::vector<TopoDS_Edge> &closedEdges)
{
//    Base::Console().Message("DPS::scrubEdges() - TopoDS_Edges in: %d\n", origEdges.size());

    if (origEdges.empty()) {
        //how did this happen? if Scale is zero, all the edges will be zero length,
        //but Scale property has constraint, so this shouldn't happen!
//        Base::Console().Message("DPS::scrubEdges(2) - origEdges is empty\n");     //debug
        return std::vector<TopoDS_Edge>();
    }
    //HLR usually delivers overlapping edges. We need to refine edge overlaps
    //into non-overlapping pieces
    std::vector<TopoDS_Edge> noOverlaps;
    noOverlaps = DrawProjectSplit::removeOverlapEdges(origEdges);

    //HLR algo does not provide all edge intersections.
    //need to split edges at intersection points.
    std::vector<TopoDS_Edge> splitEdges;
    splitEdges = DrawProjectSplit::splitIntersectingEdges(noOverlaps);

    //separate any closed edges (ex circle) from the edge pile so as not to confuse
    //the edge walker later. Closed edges are added back in the caller after
    //EdgeWalker finds the faces using the open edges
    std::vector<TopoDS_Edge> openEdges;
    for (auto& edge : splitEdges) {
        if (BRep_Tool::IsClosed(edge)) {
            closedEdges.push_back(edge);
        } else {
            openEdges.push_back(edge);
        }
    }

    //find all the unique vertices and count how many edges terminate at each, then
    //remove edges that can't be part of a closed region since they are not connected at both ends
    vertexMap verts = DrawProjectSplit::getUniqueVertexes(openEdges);
    return DrawProjectSplit::pruneUnconnected(verts, openEdges);
}

//extract a map of unique vertexes based on start and end point of each edge in
//the input vector and count the usage of each unique vertex
vertexMap DrawProjectSplit::getUniqueVertexes(std::vector<TopoDS_Edge> inEdges)
{
    vertexMap verts;
    //count the occurrences of each vertex in the pile
    for (auto& edge: inEdges) {
        gp_Pnt p = BRep_Tool::Pnt(TopExp::FirstVertex(edge));
        Base::Vector3d v0(p.X(), p.Y(), p.Z());
        vertexMap::iterator it0(verts.find(v0));
        if (it0 != verts.end()) {
            it0->second++;
        } else {
            verts[v0] = 1;
        }
        p = BRep_Tool::Pnt(TopExp::LastVertex(edge));
        Base::Vector3d v1(p.X(), p.Y(), p.Z());
        vertexMap::iterator it1(verts.find(v1));
        if (it1 != verts.end()) {
            it1->second++;
        } else {
            verts[v1] = 1;
        }
    }
    return verts;
}

//if an edge is not connected at both ends, then it can't be part of a face boundary
//and unconnected edges may confuse up the edge walker.
//note: closed edges have been removed by this point for later handling
std::vector<TopoDS_Edge> DrawProjectSplit::pruneUnconnected(vertexMap verts,
                                                            std::vector<TopoDS_Edge> edges)
{
//    Base::Console().Message("DPS::pruneUnconnected() - edges in: %d\n", edges.size());
    //check if edge ends are used at least twice => edge is joined to another edge
    std::vector<TopoDS_Edge> newPile;
    std::vector<TopoDS_Edge> deadEnds;
    for (auto& edge: edges) {
        gp_Pnt p = BRep_Tool::Pnt(TopExp::FirstVertex(edge));
        Base::Vector3d v0(p.X(), p.Y(), p.Z());
        int count0 = 0;
        vertexMap::iterator it0(verts.find(v0));
        if (it0 != verts.end()) {
            count0 = it0->second;
        }
        p = BRep_Tool::Pnt(TopExp::LastVertex(edge));
        Base::Vector3d v1(p.X(), p.Y(), p.Z());
        int count1 = 0;
        vertexMap::iterator it1(verts.find(v1));
        if (it1 != verts.end()) {
            count1 = it1->second;
        }
        if ((count0 > 1) && (count1 > 1)) {           //connected at both ends
            newPile.push_back(edge);
        } else if ((count0 == 1) && (count1 == 1)) {
            //completely disconnected edge. just drop it.
            continue;
        } else {
            //only connected at 1 end
            deadEnds.push_back(edge);    //could separate dead ends here
        }
    }

    return newPile;
}

bool DrawProjectSplit::sameEndPoints(const TopoDS_Edge &e1, const TopoDS_Edge &e2)
{
    TopoDS_Vertex first1 = TopExp::FirstVertex(e1);
    TopoDS_Vertex last1 = TopExp::LastVertex(e1);
    TopoDS_Vertex first2 = TopExp::FirstVertex(e2);
    TopoDS_Vertex last2 = TopExp::LastVertex(e2);

    if (DrawUtil::vertexEqual(first1, first2) &&
        DrawUtil::vertexEqual(last1, last2) ) {
        return true;
    }
    else if (DrawUtil::vertexEqual(first1, last2) &&
             DrawUtil::vertexEqual(last1, first2) ) {
        return true;
    }

    return false;
}

#define e0ISSUBSET 0
#define e1ISSUBSET 1
#define EDGEOVERLAP 2
#define NOTASUBSET 3

//eliminate edges that overlap another edge
std::vector<TopoDS_Edge> DrawProjectSplit::removeOverlapEdges(const std::vector<TopoDS_Edge> &inEdges)
{
//    Base::Console().Message("DPS::removeOverlapEdges() - %d edges in\n", inEdges.size());
    std::vector<TopoDS_Edge> outEdges;
    std::vector<TopoDS_Edge> overlapEdges;
    std::vector<bool> skipThisEdge(inEdges.size(), false);
    int edgeCount = inEdges.size();
    int ie0 = 0;
    for (; ie0 < edgeCount; ie0++) {
        if (skipThisEdge.at(ie0)) {
            continue;
        }
        int ie1 = ie0 + 1;
        for (; ie1 < edgeCount; ie1++) {
            if (skipThisEdge.at(ie1)) {
                continue;
            }
            int rc = isSubset(inEdges.at(ie0), inEdges.at(ie1));
            if (rc == e0ISSUBSET) {
                skipThisEdge.at(ie0) = true;
                break;      //stop checking ie0
            } else if (rc == e1ISSUBSET) {
                skipThisEdge.at(ie1) = true;
            } else if (rc == EDGEOVERLAP) {
                skipThisEdge.at(ie0) = true;
                skipThisEdge.at(ie1) = true;
                std::vector<TopoDS_Edge> olap = fuseEdges(inEdges.at(ie0), inEdges.at(ie1));
                if (!olap.empty()) {
                    overlapEdges.insert(overlapEdges.end(), olap.begin(), olap.end());
                }
                break;      //stop checking ie0
            }
        } //inner loop
    } //outer loop

    int iOut = 0;
    for (auto& e: inEdges) {
        if (!skipThisEdge.at(iOut)) {
            outEdges.push_back(e);
        }
        iOut++;
    }

    if (!overlapEdges.empty()) {
        outEdges.insert(outEdges.end(), overlapEdges.begin(), overlapEdges.end());
    }

//    Base::Console().Message("DPS::removeOverlapEdges() - %d edges out\n", outEdges.size());

    return outEdges;
}

//determine if edge0 & edge1 are superimposed, and classify the type of overlap
int DrawProjectSplit::isSubset(const TopoDS_Edge &edge0, const TopoDS_Edge &edge1)
{
    if (!boxesIntersect(edge0, edge1)) {
        return NOTASUBSET;      //boxes don't intersect, so edges do not overlap
    }

    //bboxes of edges intersect
    BRepAlgoAPI_Common anOp;
    anOp.SetFuzzyValue (FUZZYADJUST * EWTOLERANCE);
    TopTools_ListOfShape anArg1, anArg2;
    anArg1.Append (edge0);
    anArg2.Append (edge1);
    anOp.SetArguments (anArg1);
    anOp.SetTools (anArg2);
    anOp.Build();
    TopoDS_Shape aRes = anOp.Shape();   //always a compound
    if (aRes.IsNull()) {
        return NOTASUBSET;      //no common segment
    }
    std::vector<TopoDS_Edge> commonEdgeList;
    TopExp_Explorer edges(aRes, TopAbs_EDGE);
    for (int i = 1; edges.More(); edges.Next(), i++) {
        commonEdgeList.push_back(TopoDS::Edge(edges.Current()));
    }
    if (commonEdgeList.empty()) {
        return NOTASUBSET;
    }
    //we're only going to deal with the situation where the common of the edges
    //is a single edge.  A really odd pair of edges could have >1 edge in their
    //common.
    TopoDS_Edge commonEdge = commonEdgeList.at(0);
    if (sameEndPoints(edge1, commonEdge)) {
        return e1ISSUBSET;  //e1 is a subset of e0
    }
    if (sameEndPoints(edge0, commonEdge)) {
        return e0ISSUBSET;  //e0 is a subset of e1
    }
    // edge0 is not a subset of edge1, nor is edge1 a subset of edge0, but they have a common segment
    return EDGEOVERLAP;
}

//edge0 and edge1 overlap, so we need to make 3 edges - part of edge0, common segment, part of edge1
std::vector<TopoDS_Edge> DrawProjectSplit::fuseEdges(const TopoDS_Edge &edge0, const TopoDS_Edge &edge1)
{
    std::vector<TopoDS_Edge> edgeList;
    BRepAlgoAPI_Fuse anOp;
    anOp.SetFuzzyValue (FUZZYADJUST * EWTOLERANCE);
    TopTools_ListOfShape anArg1, anArg2;
    anArg1.Append (edge0);
    anArg2.Append (edge1);
    anOp.SetArguments (anArg1);
    anOp.SetTools (anArg2);
    anOp.Build();
    TopoDS_Shape aRes = anOp.Shape();    //always a compound
    if (aRes.IsNull()) {
        return edgeList;     //empty result
    }
    TopExp_Explorer edges(aRes, TopAbs_EDGE);
    for (int i = 1; edges.More(); edges.Next(), i++) {
        edgeList.push_back(TopoDS::Edge(edges.Current()));
    }
    return edgeList;
}

//split edges that intersect into pieces.
std::vector<TopoDS_Edge> DrawProjectSplit::splitIntersectingEdges(std::vector<TopoDS_Edge>& inEdges)
{
//    Base::Console().Message("DPS::splitIntersectingEdges() - edges in: %d\n", inEdges.size());
    std::vector<TopoDS_Edge> outEdges;
    std::vector<bool> skipThisEdge(inEdges.size(), false);
    int edgeCount = inEdges.size();
    int iEdge0 = 0;
    for (; iEdge0 < edgeCount; iEdge0++) {  //all but last one
        if (skipThisEdge.at(iEdge0)) {
            continue;
        }
        int iEdge1 = iEdge0 + 1;
        bool outerEdgeSplit = false;
        for (; iEdge1 < edgeCount; iEdge1++) {
            if (skipThisEdge.at(iEdge1)) {
                continue;
            }

            if (boxesIntersect(inEdges.at(iEdge0), inEdges.at(iEdge1))) {
                std::vector<TopoDS_Edge> intersectEdges = fuseEdges(inEdges.at(iEdge0), inEdges.at(iEdge1));
                if (intersectEdges.empty()) {
                    //don't think this can happen. fusion of disjoint edges is 2 edges.
                    //maybe an error?
                    continue;   //next inner edge
                }

                if (intersectEdges.size() == 1) {
                    //one edge is a subset of the other.
                    if (sameEndPoints(inEdges.at(iEdge0), intersectEdges.front())) {
                        //we got the outer edge back so mark the inner edge
                        skipThisEdge.at(iEdge1) = true;
                    } else if (sameEndPoints(inEdges.at(iEdge1), intersectEdges.front())) {
                        //we got the inner edge back so mark the outer edge and go to the next outer edge
                        skipThisEdge.at(iEdge0) = true;
                        break;          //next outer edge
                    } else {
                        //not sure what this means?  bad geometry?
                    }

                } else if (intersectEdges.size() == 2) {
                    //got the input edges back, so no intersection. carry on with next inner edge
                    continue;    //next inner edge

                } else if (intersectEdges.size() == 3) {
                    //we have split 1 edge at a vertex of the other edge
                    //check if outer edge is the one split
                    bool innerEdgeSplit = false;
                    for (auto& interEdge : intersectEdges) {
                        if (!sameEndPoints(inEdges.at(iEdge0), interEdge) &&
                            !sameEndPoints(inEdges.at(iEdge1), interEdge)) {
                            //interEdge does not match either outer or inner edge,
                            //so this is a piece of the split edge and we need to add it
                            //to end of list
                            inEdges.push_back(interEdge);
                            skipThisEdge.push_back(false);
                            edgeCount++;
                         }
                        if (sameEndPoints(inEdges.at(iEdge0), interEdge)) {
                            //outer edge is in output, so it was not split.
                            //therefore the inner edge was split and we should skip it in the future
                            //the two pieces of the split edge will have been added to edgesToKeep
                            //in the previous if
                            innerEdgeSplit = true;
                            skipThisEdge.at(iEdge1) = true;
                        } else if (sameEndPoints(inEdges.at(iEdge1), interEdge)) {
                            //inner edge is in output, so it was not split.
                            //therefore the outer edge was split and we should skip it in the future.
                            outerEdgeSplit = true;
                            skipThisEdge.at(iEdge0) = true;
                        }
                    }
                    if (!innerEdgeSplit && !outerEdgeSplit) {
                        //neither edge found in output, so this was a partial overlap, so
                        //both edges are replaced by the 3 split pieces
                        //Q: why does this happen if we have run pruneOverlaps before this???
                        skipThisEdge.at(iEdge0) = true;
                        skipThisEdge.at(iEdge1) = true;
                        outerEdgeSplit = true;
                    }
                    if (outerEdgeSplit) {
                        //we can't use the outer edge any more, so we should exit the inner loop
                        break;
                    }

                } else if (intersectEdges.size() == 4) {
                    //we have split both edges at a single intersection
                    skipThisEdge.at(iEdge0) = true;
                    skipThisEdge.at(iEdge1) = true;
                    inEdges.insert(inEdges.end(), intersectEdges.begin(), intersectEdges.end());
                    skipThisEdge.insert(skipThisEdge.end(), { false, false, false, false});
                    edgeCount += 4;
                    outerEdgeSplit = true;
                    break;

                } else {
                    //this means multiple intersections of the 2 edges. we don't handle that yet.
                    continue;  //next inner edge?
                }

            } else {
                //bboxes of edges do not intersect, so edges do not intersect
            }
        }  //inner loop boundary

        if (!outerEdgeSplit) {
            //outer edge[iEdge0] was not split, so add it to the output and mark it as used
            outEdges.push_back(inEdges.at(iEdge0));
            skipThisEdge.at(iEdge0) = true;        //superfluous?
        }
    }  //outer loop boundary

    if (!skipThisEdge.back()) {
        //last entry has not been split, so add it to output
        outEdges.push_back(inEdges.back());
    }

    return outEdges;
}

bool DrawProjectSplit::boxesIntersect(const TopoDS_Edge &edge0, const TopoDS_Edge &edge1)
{
    Bnd_Box box0, box1;
    BRepBndLib::Add(edge0, box0);
    box0.SetGap(0.1);           //generous
    BRepBndLib::Add(edge1, box1);
    box1.SetGap(0.1);
    if (box0.IsOut(box1)) {
        return false;      //boxes don't intersect
    }
    return true;
}

//this is an aid to debugging and isn't used in normal processing.
void DrawProjectSplit::dumpVertexMap(vertexMap verts)
{
    Base::Console().Message("DPS::dumpVertexMap - %d verts\n", verts.size());
    int iVert = 0;
    for (auto& item : verts) {
        Base::Console().Message("%d: %s - %d\n",iVert,
                                DrawUtil::formatVector(item.first).c_str(), item.second);
        iVert++;
    }
}
