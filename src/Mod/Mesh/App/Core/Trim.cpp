/***************************************************************************
 *   Copyright (c) 2012 Imetric 3D GmbH                                    *
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
#include <algorithm>
#endif

#include <Base/Sequencer.h>

#include "Grid.h"
#include "Iterator.h"
#include "Trim.h"


using namespace MeshCore;

MeshTrimming::MeshTrimming(MeshKernel& rclM,
                           const Base::ViewProjMethod* pclProj,
                           const Base::Polygon2d& rclPoly)
    : myMesh(rclM)
    , myProj(pclProj)
    , myPoly(rclPoly)
{}

void MeshTrimming::SetInnerOrOuter(TMode tMode)
{
    switch (tMode) {
        case INNER:
            myInner = true;
            break;
        case OUTER:
            myInner = false;
            break;
    }
}

void MeshTrimming::CheckFacets(const MeshFacetGrid& rclGrid,
                               std::vector<FacetIndex>& raulFacets) const
{
    std::vector<FacetIndex>::iterator it;
    MeshFacetIterator clIter(myMesh, 0);

    // cut inner: use grid to accelerate search
    if (myInner) {
        Base::BoundBox3f clBBox3d;
        Base::BoundBox2d clViewBBox, clPolyBBox;
        std::vector<FacetIndex> aulAllElements;

        // BBox of polygon
        clPolyBBox = myPoly.CalcBoundBox();
        MeshGridIterator clGridIter(rclGrid);
        // traverse all BBoxes
        for (clGridIter.Init(); clGridIter.More(); clGridIter.Next()) {
            clBBox3d = clGridIter.GetBoundBox();
            clViewBBox = clBBox3d.ProjectBox(myProj);
            if (clViewBBox.Intersect(clPolyBBox)) {
                // save all elements in AllElements
                clGridIter.GetElements(aulAllElements);
            }
        }

        // remove double elements
        std::sort(aulAllElements.begin(), aulAllElements.end());
        aulAllElements.erase(std::unique(aulAllElements.begin(), aulAllElements.end()),
                             aulAllElements.end());

        Base::SequencerLauncher seq("Check facets for intersection...", aulAllElements.size());

        for (it = aulAllElements.begin(); it != aulAllElements.end(); ++it) {
            MeshGeomFacet clFacet = myMesh.GetFacet(*it);
            if (HasIntersection(clFacet)) {
                raulFacets.push_back(*it);
            }
            seq.next();
        }
    }
    // cut outer
    else {
        Base::SequencerLauncher seq("Check facets for intersection...", myMesh.CountFacets());
        for (clIter.Init(); clIter.More(); clIter.Next()) {
            if (HasIntersection(*clIter)) {
                raulFacets.push_back(clIter.Position());
            }
            seq.next();
        }
    }
}

bool MeshTrimming::HasIntersection(const MeshGeomFacet& rclFacet) const
{
    Base::Polygon2d clPoly;
    Base::Line2d clFacLine, clPolyLine;
    Base::Vector2d S;
    // is corner of facet inside the polygon
    for (auto pnt : rclFacet._aclPoints) {
        Base::Vector3f clPt2d = myProj->operator()(pnt);
        if (myPoly.Contains(Base::Vector2d(clPt2d.x, clPt2d.y)) == myInner) {
            return true;
        }
        else {
            clPoly.Add(Base::Vector2d(clPt2d.x, clPt2d.y));
        }
    }

    // is corner of polygon inside the facet
    for (size_t j = 0; j < myPoly.GetCtVectors(); j++) {
        if (clPoly.Contains(myPoly[j])) {
            return true;
        }
    }

    // check for other intersections
    for (size_t j = 0; j < myPoly.GetCtVectors(); j++) {
        clPolyLine.clV1 = myPoly[j];
        clPolyLine.clV2 = myPoly[(j + 1) % myPoly.GetCtVectors()];

        for (size_t i = 0; i < 3; i++) {
            clFacLine.clV1 = clPoly[i];
            clFacLine.clV2 = clPoly[(i + 1) % 3];

            if (clPolyLine.IntersectAndContain(clFacLine, S)) {
                return true;
            }
        }
    }

    // no intersection
    return false;
}

bool MeshTrimming::PolygonContainsCompleteFacet(bool bInner, FacetIndex ulIndex) const
{
    const MeshFacet& rclFacet = myMesh._aclFacetArray[ulIndex];
    for (PointIndex ptIndex : rclFacet._aulPoints) {
        const MeshPoint& rclFacPt = myMesh._aclPointArray[ptIndex];
        Base::Vector3f clPt = (*myProj)(rclFacPt);
        if (myPoly.Contains(Base::Vector2d(clPt.x, clPt.y)) != bInner) {
            return false;
        }
    }

    return true;
}

bool MeshTrimming::IsPolygonPointInFacet(FacetIndex ulIndex, Base::Vector3f& clPoint)
{
    Base::Vector2d A, B, C, P;
    float u {}, v {}, w {}, fDetPAC {}, fDetPBC {}, fDetPAB {}, fDetABC {};
    Base::Polygon2d clFacPoly;
    const MeshGeomFacet& rclFacet = myMesh.GetFacet(ulIndex);

    for (auto pnt : rclFacet._aclPoints) {
        Base::Vector3f clPt = (*myProj)(pnt);
        clFacPoly.Add(Base::Vector2d(clPt.x, clPt.y));
    }

    A = clFacPoly[0];
    B = clFacPoly[1];
    C = clFacPoly[2];
    fDetABC =
        static_cast<float>(A.x * B.y + A.y * C.x + B.x * C.y - (B.y * C.x + A.y * B.x + A.x * C.y));

    for (size_t j = 0; j < myPoly.GetCtVectors(); j++) {
        // facet contains a polygon point -> calculate the corresponding 3d-point
        if (clFacPoly.Contains(myPoly[j])) {
            P = myPoly[j];
            fDetPAC = static_cast<float>(A.x * P.y + A.y * C.x + P.x * C.y
                                         - (P.y * C.x + A.y * P.x + A.x * C.y));
            fDetPBC = static_cast<float>(P.x * B.y + P.y * C.x + B.x * C.y
                                         - (B.y * C.x + P.y * B.x + P.x * C.y));
            fDetPAB = static_cast<float>(A.x * B.y + A.y * P.x + B.x * P.y
                                         - (B.y * P.x + A.y * B.x + A.x * P.y));
            u = fDetPBC / fDetABC;
            v = fDetPAC / fDetABC;
            w = fDetPAB / fDetABC;

            // point is on edge or no valid convex combination
            if (u == 0.0f || v == 0.0f || w == 0.0f || fabs(u + v + w - 1.0f) >= 0.001f) {
                return false;
            }
            // 3d point
            clPoint = u * rclFacet._aclPoints[0] + v * rclFacet._aclPoints[1]
                + w * rclFacet._aclPoints[2];

            return true;
        }
    }

    return false;
}

bool MeshTrimming::GetIntersectionPointsOfPolygonAndFacet(
    FacetIndex ulIndex,
    int& iSide,
    std::vector<Base::Vector3f>& raclPoints) const
{
    MeshGeomFacet clFac(myMesh.GetFacet(ulIndex));
    Base::Vector2d S;
    Base::Line2d clFacLine, clPolyLine;
    int iIntersections = 0;
    int iIntsctWithEdge0 = 0, iIntsctWithEdge1 = 0, iIntsctWithEdge2 = 0;

    // Edge with no intersection
    iSide = -1;

    for (size_t i = 0; i < myPoly.GetCtVectors(); i++) {
        // totally only four intersections allowed
        if (iIntersections == 4) {
            break;
        }

        Base::Vector2d P3(myPoly[i]), P4(myPoly[(i + 1) % myPoly.GetCtVectors()]);
        clPolyLine.clV1 = P3;
        clPolyLine.clV2 = P4;

        for (int j = 0; j < 3; j++) {
            Base::Vector3f clP1((*myProj)(clFac._aclPoints[j]));
            Base::Vector3f clP2((*myProj)(clFac._aclPoints[(j + 1) % 3]));
            Base::Vector2d P1(clP1.x, clP1.y);
            Base::Vector2d P2(clP2.x, clP2.y);
            clFacLine.clV1 = P1;
            clFacLine.clV2 = P2;

            if (clPolyLine.Intersect(P1, double(MESH_MIN_PT_DIST))) {
                // do not pick up corner points
                iIntersections++;
            }
            else if (clPolyLine.Intersect(P2, double(MESH_MIN_PT_DIST))) {
                // do not pick up corner points
                iIntersections++;
            }
            else if (clPolyLine.Intersect(clFacLine, S)) {
                bool bPushBack = true;
                float fP1P2 = static_cast<float>((P2 - P1).Length());
                float fSP1 = static_cast<float>((P1 - S).Length());
                float fSP2 = static_cast<float>((P2 - S).Length());

                float fP3P4 = static_cast<float>((P4 - P3).Length());
                float fSP3 = static_cast<float>((P3 - S).Length());
                float fSP4 = static_cast<float>((P4 - S).Length());
                // compute proportion of length
                float l = fSP1 / fP1P2;
                float m = fSP2 / fP1P2;

                float r = fSP3 / fP3P4;
                float s = fSP4 / fP3P4;

                // is intersection point convex combination?
                if ((fabs(l + m - 1.0f) < 0.001f) && (fabs(r + s - 1.0f) < 0.001f)) {
                    Base::Vector3f clIntersection(m * clFac._aclPoints[j]
                                                  + l * clFac._aclPoints[(j + 1) % 3]);

                    iIntersections++;

                    // only two intersections points per edge allowed
                    if (j == 0) {
                        if (iIntsctWithEdge0 == 2) {
                            bPushBack = false;
                        }
                        else {
                            iIntsctWithEdge0++;
                        }
                    }
                    else if (j == 1) {
                        if (iIntsctWithEdge1 == 2) {
                            bPushBack = false;
                        }
                        else {
                            iIntsctWithEdge1++;
                        }
                    }
                    else {
                        if (iIntsctWithEdge2 == 2) {
                            bPushBack = false;
                        }
                        else {
                            iIntsctWithEdge2++;
                        }
                    }

                    if (bPushBack) {
                        raclPoints.push_back(clIntersection);
                    }
                }
            }
        }
    }

    // check for rotating facet later
    if (iIntsctWithEdge0 == 0) {
        iSide = 0;
    }
    else if (iIntsctWithEdge1 == 0) {
        iSide = 1;
    }
    else if (iIntsctWithEdge2 == 0) {
        iSide = 2;
    }

    // further check (for rotating the facet)
    if (iIntsctWithEdge0 == 0 && iIntsctWithEdge1 == 0) {
        iSide = 1;
    }
    else if (iIntsctWithEdge0 == 0 && iIntsctWithEdge2 == 0) {
        iSide = 0;
    }
    else if (iIntsctWithEdge1 == 0 && iIntsctWithEdge2 == 0) {
        iSide = 2;
    }

    // and last another check
    if (iIntsctWithEdge0 * iIntsctWithEdge1 * iIntsctWithEdge2 > 0) {
        if (iIntsctWithEdge0 == 2) {
            iSide = 2;
        }
        else if (iIntsctWithEdge1 == 2) {
            iSide = 0;
        }
        else if (iIntsctWithEdge2 == 2) {
            iSide = 1;
        }
    }

    return iIntersections > 0;
}

void MeshTrimming::AdjustFacet(MeshFacet& facet, int iInd)
{
    unsigned long tmp {};

    if (iInd == 1) {
        tmp = facet._aulPoints[0];
        facet._aulPoints[0] = facet._aulPoints[1];
        facet._aulPoints[1] = facet._aulPoints[2];
        facet._aulPoints[2] = tmp;
        tmp = facet._aulNeighbours[0];
        facet._aulNeighbours[0] = facet._aulNeighbours[1];
        facet._aulNeighbours[1] = facet._aulNeighbours[2];
        facet._aulNeighbours[2] = tmp;
    }
    else if (iInd == 2) {
        tmp = facet._aulPoints[0];
        facet._aulPoints[0] = facet._aulPoints[2];
        facet._aulPoints[2] = facet._aulPoints[1];
        facet._aulPoints[1] = tmp;
        tmp = facet._aulNeighbours[0];
        facet._aulNeighbours[0] = facet._aulNeighbours[2];
        facet._aulNeighbours[2] = facet._aulNeighbours[1];
        facet._aulNeighbours[1] = tmp;
    }
}

bool MeshTrimming::CreateFacets(FacetIndex ulFacetPos,
                                int iSide,
                                const std::vector<Base::Vector3f>& raclPoints,
                                std::vector<MeshGeomFacet>& aclNewFacets)
{
    MeshGeomFacet clFac;

    // no valid triangulation possible
    if (iSide == -1) {
        return false;
    }

    // no intersection point found => triangle is only touched at a corner point
    if (raclPoints.empty()) {
        MeshFacet& facet = myMesh._aclFacetArray[ulFacetPos];
        int iCtPtsIn = 0;
        int iCtPtsOn = 0;
        Base::Vector3f clFacPnt;
        Base::Vector2d clProjPnt;
        for (PointIndex ptIndex : facet._aulPoints) {
            clFacPnt = (*myProj)(myMesh._aclPointArray[ptIndex]);
            clProjPnt = Base::Vector2d(clFacPnt.x, clFacPnt.y);
            if (myPoly.Intersect(clProjPnt, double(MESH_MIN_PT_DIST))) {
                ++iCtPtsOn;
            }
            else if (myPoly.Contains(clProjPnt) == myInner) {
                ++iCtPtsIn;
            }
        }

        // in this case we can use the original triangle
        if (iCtPtsIn != (3 - iCtPtsOn)) {
            aclNewFacets.push_back(myMesh.GetFacet(ulFacetPos));
        }
    }
    // one intersection point found => triangle is also touched at a corner point
    else if (raclPoints.size() == 1) {
        Base::Vector3f clP(raclPoints[0]);
        clP = ((*myProj)(clP));
        Base::Vector2d P(clP.x, clP.y);
        MeshGeomFacet clFac(myMesh.GetFacet(ulFacetPos));

        // determine the edge containing the intersection point
        Base::Line2d clFacLine;
        for (int j = 0; j < 3; j++) {
            Base::Vector3f clP1((*myProj)(clFac._aclPoints[j]));
            Base::Vector3f clP2((*myProj)(clFac._aclPoints[(j + 1) % 3]));
            Base::Vector2d P1(clP1.x, clP1.y);
            Base::Vector2d P2(clP2.x, clP2.y);
            clFacLine.clV1 = P1;
            clFacLine.clV2 = P2;

            if (clFacLine.Intersect(P, double(MESH_MIN_PT_DIST))) {
                if (myPoly.Contains(P1) == myInner) {
                    MeshGeomFacet clNew;
                    clNew._aclPoints[0] = raclPoints[0];
                    clNew._aclPoints[1] = clFac._aclPoints[(j + 1) % 3];
                    clNew._aclPoints[2] = clFac._aclPoints[(j + 2) % 3];
                    aclNewFacets.push_back(clNew);
                    break;
                }
                else if (myPoly.Contains(P2) == myInner) {
                    MeshGeomFacet clNew;
                    clNew._aclPoints[0] = raclPoints[0];
                    clNew._aclPoints[1] = clFac._aclPoints[(j + 2) % 3];
                    clNew._aclPoints[2] = clFac._aclPoints[j];
                    aclNewFacets.push_back(clNew);
                    break;
                }
            }
        }
    }
    // two intersection points found
    else if (raclPoints.size() == 2) {
        MeshFacet& facet = myMesh._aclFacetArray[ulFacetPos];
        AdjustFacet(facet, iSide);
        Base::Vector3f clP1(raclPoints[0]), clP2(raclPoints[1]);

        if (iSide == 1) {
            // swap P1 and P2
            clP1 = raclPoints[1];
            clP2 = raclPoints[0];
        }

        // check which facets can be inserted
        int iCtPts = 0;
        Base::Vector3f clFacPnt;
        Base::Vector2d clProjPnt;
        for (PointIndex ptIndex : facet._aulPoints) {
            clFacPnt = (*myProj)(myMesh._aclPointArray[ptIndex]);
            clProjPnt = Base::Vector2d(clFacPnt.x, clFacPnt.y);
            if (myPoly.Contains(clProjPnt) == myInner) {
                ++iCtPts;
            }
        }

        if (iCtPts == 2) {
            // erstes Dreieck
            clFac._aclPoints[0] = clP1;
            clFac._aclPoints[1] = myMesh._aclPointArray[facet._aulPoints[2]];
            clFac._aclPoints[2] = clP2;
            aclNewFacets.push_back(clFac);
        }
        else if (iCtPts == 1) {
            // erstes Dreieck
            clFac._aclPoints[0] = myMesh._aclPointArray[facet._aulPoints[0]];
            clFac._aclPoints[1] = myMesh._aclPointArray[facet._aulPoints[1]];
            clFac._aclPoints[2] = clP2;
            aclNewFacets.push_back(clFac);
            // zweites Dreieck
            clFac._aclPoints[0] = myMesh._aclPointArray[facet._aulPoints[1]];
            clFac._aclPoints[1] = clP1;
            clFac._aclPoints[2] = clP2;
            aclNewFacets.push_back(clFac);
        }
        else {
            return false;
        }
    }
    // four intersection points found
    else if (raclPoints.size() == 4) {
        MeshFacet& facet = myMesh._aclFacetArray[ulFacetPos];
        AdjustFacet(facet, iSide);

        clFac = myMesh.GetFacet(ulFacetPos);
        // intersection points
        Base::Vector3f clP1(raclPoints[0]), clP2(raclPoints[1]), clP3(raclPoints[2]),
            clP4(raclPoints[3]);

        // check which facets can be inserted
        int iCtPts = 0;
        Base::Vector3f clFacPnt;
        Base::Vector2d clProjPnt;
        for (PointIndex ptIndex : facet._aulPoints) {
            clFacPnt = (*myProj)(myMesh._aclPointArray[ptIndex]);
            clProjPnt = Base::Vector2d(clFacPnt.x, clFacPnt.y);
            if (myPoly.Contains(clProjPnt) == myInner) {
                ++iCtPts;
            }
        }

        // sort the intersection points in a certain order
        if (iCtPts == 0 || iCtPts == 3) {
            if (iSide == 1) {
                // swap the points
                clP1 = clP2;
                clP2 = raclPoints[0];
                clP3 = clP4;
                clP4 = raclPoints[2];
            }

            if ((clP1 - clFac._aclPoints[1]).Length() > (clP3 - clFac._aclPoints[1]).Length()) {
                // swap P1 and P3
                Base::Vector3f tmp(clP1);
                clP1 = clP3;
                clP3 = tmp;
            }
            if ((clP2 - clFac._aclPoints[0]).Length() > (clP4 - clFac._aclPoints[0]).Length()) {
                // swap P2 and P4
                Base::Vector3f tmp(clP2);
                clP2 = clP4;
                clP4 = tmp;
            }
        }
        else {
            if (iSide == 0) {
                Base::Vector3f clNormal(clFac.GetNormal());
                MeshGeomFacet clTmpFac;
                clTmpFac._aclPoints[0] = clFac._aclPoints[1];
                clTmpFac._aclPoints[1] = clP2;
                clTmpFac._aclPoints[2] = clP1;
                if (clTmpFac.GetNormal() * clNormal > 0) {
                    Base::Vector3f tmp(clP1);
                    clP1 = clP2;
                    clP2 = tmp;
                }
                else {
                    Base::Vector3f tmp(clP1);
                    clP1 = clP4;
                    clP4 = clP2;
                    clP2 = clP3;
                    clP3 = tmp;
                }
            }
            else if (iSide == 1) {
                if ((clP2 - clFac._aclPoints[1]).Length() > (clP4 - clFac._aclPoints[1]).Length()) {
                    Base::Vector3f tmp(clP1);
                    clP1 = clP4;
                    clP4 = tmp;
                    tmp = clP2;
                    clP2 = clP3;
                    clP3 = tmp;
                }
                else {
                    Base::Vector3f tmp(clP1);
                    clP1 = clP2;
                    clP2 = tmp;
                    tmp = clP3;
                    clP3 = clP4;
                    clP4 = tmp;
                }
            }
            else {
                if ((clP1 - clFac._aclPoints[1]).Length() > (clP3 - clFac._aclPoints[1]).Length()) {
                    Base::Vector3f tmp(clP1);
                    clP1 = clP3;
                    clP3 = tmp;
                    tmp = clP2;
                    clP2 = clP4;
                    clP4 = tmp;
                }
            }
        }

        // now create the new facets
        if (iCtPts == 0) {
            // insert first facet
            clFac._aclPoints[0] = myMesh._aclPointArray[facet._aulPoints[0]];
            clFac._aclPoints[1] = myMesh._aclPointArray[facet._aulPoints[1]];
            clFac._aclPoints[2] = clP1;
            aclNewFacets.push_back(clFac);
            // insert second facet
            clFac._aclPoints[0] = myMesh._aclPointArray[facet._aulPoints[0]];
            clFac._aclPoints[1] = clP1;
            clFac._aclPoints[2] = clP2;
            aclNewFacets.push_back(clFac);
            // finally insert third facet
            clFac._aclPoints[0] = myMesh._aclPointArray[facet._aulPoints[2]];
            clFac._aclPoints[1] = clP4;
            clFac._aclPoints[2] = clP3;
            aclNewFacets.push_back(clFac);
        }
        else if (iCtPts == 1) {
            // insert first facet
            clFac._aclPoints[0] = clP1;
            clFac._aclPoints[1] = clP2;
            clFac._aclPoints[2] = myMesh._aclPointArray[facet._aulPoints[1]];
            aclNewFacets.push_back(clFac);
            // finally insert second facet
            clFac._aclPoints[0] = clP4;
            clFac._aclPoints[1] = clP3;
            clFac._aclPoints[2] = myMesh._aclPointArray[facet._aulPoints[2]];
            aclNewFacets.push_back(clFac);
        }
        else if (iCtPts == 2) {
            // insert first facet
            clFac._aclPoints[0] = myMesh._aclPointArray[facet._aulPoints[0]];
            clFac._aclPoints[1] = clP2;
            clFac._aclPoints[2] = clP4;
            aclNewFacets.push_back(clFac);
            // insert second facet
            clFac._aclPoints[0] = clP1;
            clFac._aclPoints[1] = clP4;
            clFac._aclPoints[2] = clP2;
            aclNewFacets.push_back(clFac);
            // finally insert third facet
            clFac._aclPoints[0] = clP1;
            clFac._aclPoints[1] = clP3;
            clFac._aclPoints[2] = clP4;
            aclNewFacets.push_back(clFac);
        }
        else {
            // insert first facet
            clFac._aclPoints[0] = clP1;
            clFac._aclPoints[1] = clP3;
            clFac._aclPoints[2] = clP4;
            aclNewFacets.push_back(clFac);
            // finally insert second facet
            clFac._aclPoints[0] = clP1;
            clFac._aclPoints[1] = clP4;
            clFac._aclPoints[2] = clP2;
            aclNewFacets.push_back(clFac);
        }
    }
    else {
        return false;
    }

    return true;
}

bool MeshTrimming::CreateFacets(FacetIndex ulFacetPos,
                                int iSide,
                                const std::vector<Base::Vector3f>& raclPoints,
                                Base::Vector3f& clP3,
                                std::vector<MeshGeomFacet>& aclNewFacets)
{
    // no valid triangulation possible
    if (iSide == -1 || raclPoints.size() < 2) {
        return false;
    }

    Base::Vector3f clP1(raclPoints[0]);
    Base::Vector3f clP2(raclPoints[1]);

    MeshFacet& facet = myMesh._aclFacetArray[ulFacetPos];
    AdjustFacet(facet, iSide);

    MeshGeomFacet clFac;

    float fDistEdgeP1 = clP1.DistanceToLineSegment(myMesh._aclPointArray[facet._aulPoints[1]],
                                                   myMesh._aclPointArray[facet._aulPoints[2]])
                            .Length();
    float fDistEdgeP2 = clP2.DistanceToLineSegment(myMesh._aclPointArray[facet._aulPoints[1]],
                                                   myMesh._aclPointArray[facet._aulPoints[2]])
                            .Length();

    // swap P1 and P2
    if (fDistEdgeP2 < fDistEdgeP1) {
        Base::Vector3f tmp(clP1);
        clP1 = clP2;
        clP2 = tmp;
    }

    // check which facets should be inserted
    int iCtPts = 0;
    Base::Vector3f clFacPnt;
    Base::Vector2d clProjPnt;
    for (PointIndex ptIndex : facet._aulPoints) {
        clFacPnt = (*myProj)(myMesh._aclPointArray[ptIndex]);
        clProjPnt = Base::Vector2d(clFacPnt.x, clFacPnt.y);
        if (myPoly.Contains(clProjPnt) == myInner) {
            ++iCtPts;
        }
    }
    if (iCtPts == 3) {
        clFac = myMesh.GetFacet(ulFacetPos);
        if ((clP1 - clFac._aclPoints[1]).Length() > (clP2 - clFac._aclPoints[1]).Length()) {
            Base::Vector3f tmp(clP1);
            clP1 = clP2;
            clP2 = tmp;
        }
        // only one facet
        clFac._aclPoints[0] = clP1;
        clFac._aclPoints[1] = clP2;
        clFac._aclPoints[2] = clP3;
        aclNewFacets.push_back(clFac);
    }
    else if (iCtPts == 2) {
        // first facet
        clFac._aclPoints[0] = clP1;
        clFac._aclPoints[1] = myMesh._aclPointArray[facet._aulPoints[2]];
        clFac._aclPoints[2] = clP3;
        aclNewFacets.push_back(clFac);
        // second facet
        clFac._aclPoints[0] = myMesh._aclPointArray[facet._aulPoints[2]];
        clFac._aclPoints[1] = clP2;
        clFac._aclPoints[2] = clP3;
        aclNewFacets.push_back(clFac);
    }
    else if (iCtPts == 1) {
        // first facet
        clFac._aclPoints[0] = myMesh._aclPointArray[facet._aulPoints[0]];
        clFac._aclPoints[1] = myMesh._aclPointArray[facet._aulPoints[1]];
        clFac._aclPoints[2] = clP3;
        aclNewFacets.push_back(clFac);
        // second facet
        clFac._aclPoints[0] = myMesh._aclPointArray[facet._aulPoints[1]];
        clFac._aclPoints[1] = clP1;
        clFac._aclPoints[2] = clP3;
        aclNewFacets.push_back(clFac);
        // third facet
        clFac._aclPoints[0] = myMesh._aclPointArray[facet._aulPoints[0]];
        clFac._aclPoints[1] = clP3;
        clFac._aclPoints[2] = clP2;
        aclNewFacets.push_back(clFac);
    }
    else if (iCtPts == 0) {
        clFac = myMesh.GetFacet(ulFacetPos);
        if ((clP1 - clFac._aclPoints[1]).Length() > (clP2 - clFac._aclPoints[1]).Length()) {
            Base::Vector3f tmp(clP1);
            clP1 = clP2;
            clP2 = tmp;
        }
        // first facet
        clFac._aclPoints[0] = myMesh._aclPointArray[facet._aulPoints[2]];
        clFac._aclPoints[1] = clP3;
        clFac._aclPoints[2] = clP2;
        aclNewFacets.push_back(clFac);
        // second facet
        clFac._aclPoints[0] = myMesh._aclPointArray[facet._aulPoints[2]];
        clFac._aclPoints[1] = myMesh._aclPointArray[facet._aulPoints[0]];
        clFac._aclPoints[2] = clP3;
        aclNewFacets.push_back(clFac);
        // third facet
        clFac._aclPoints[0] = myMesh._aclPointArray[facet._aulPoints[0]];
        clFac._aclPoints[1] = myMesh._aclPointArray[facet._aulPoints[1]];
        clFac._aclPoints[2] = clP3;
        aclNewFacets.push_back(clFac);
        // and finally fourth facet
        clFac._aclPoints[0] = clP3;
        clFac._aclPoints[1] = myMesh._aclPointArray[facet._aulPoints[1]];
        clFac._aclPoints[2] = clP1;
        aclNewFacets.push_back(clFac);
    }

    return true;
}

void MeshTrimming::TrimFacets(const std::vector<FacetIndex>& raulFacets,
                              std::vector<MeshGeomFacet>& aclNewFacets)
{
    Base::Vector3f clP;
    std::vector<Base::Vector3f> clIntsct;
    int iSide {};

    Base::SequencerLauncher seq("trimming facets...", raulFacets.size());
    for (FacetIndex index : raulFacets) {
        clIntsct.clear();
        if (!IsPolygonPointInFacet(index, clP)) {
            // facet must be trimmed
            if (!PolygonContainsCompleteFacet(myInner, index)) {
                // generate new facets
                if (GetIntersectionPointsOfPolygonAndFacet(index, iSide, clIntsct)) {
                    CreateFacets(index, iSide, clIntsct, myTriangles);
                }
            }
        }
        // facet contains a polygon point
        else {
            // generate new facets
            if (GetIntersectionPointsOfPolygonAndFacet(index, iSide, clIntsct)) {
                CreateFacets(index, iSide, clIntsct, clP, myTriangles);
            }
        }
        seq.next();
    }

    aclNewFacets = myTriangles;
}
