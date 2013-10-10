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
#include <algorithm>

#include "Trim.h"
#include "Grid.h"
#include "Iterator.h"
#include <Base/Sequencer.h>

using namespace MeshCore;

MeshTrimming::MeshTrimming(MeshKernel &rclM, const Base::ViewProjMethod* pclProj, 
                           const Base::Polygon2D& rclPoly)
  : myMesh(rclM), myInner(true), myProj(pclProj), myPoly(rclPoly)
{
}

MeshTrimming::~MeshTrimming()
{
}

void MeshTrimming::SetInnerOrOuter(TMode tMode)
{
    switch (tMode)
    {
    case INNER:
        myInner = true;
        break;
    case OUTER:
        myInner = false;
        break;
    }
}

void MeshTrimming::CheckFacets(const MeshFacetGrid& rclGrid, std::vector<unsigned long> &raulFacets) const
{
    std::vector<unsigned long>::iterator it;
    MeshFacetIterator clIter(myMesh, 0);

    // cut inner: use grid to accelerate search
    if (myInner) {
        Base::BoundBox3f clBBox3d;
        Base::BoundBox2D clViewBBox, clPolyBBox;
        std::vector<unsigned long> aulAllElements;

        // BBox of polygon
        clPolyBBox = myPoly.CalcBoundBox();
        MeshGridIterator clGridIter(rclGrid);
        // traverse all BBoxes
        for (clGridIter.Init(); clGridIter.More(); clGridIter.Next()) {
            clBBox3d = clGridIter.GetBoundBox();
            clViewBBox = clBBox3d.ProjectBox(myProj);
            if (clViewBBox || clPolyBBox) {
                // save all elements in AllElements 
                clGridIter.GetElements(aulAllElements);
            }
        }
    
        // remove double elements 
        std::sort(aulAllElements.begin(), aulAllElements.end());
        aulAllElements.erase(std::unique(aulAllElements.begin(), aulAllElements.end()), aulAllElements.end());

        Base::SequencerLauncher seq("Check facets for intersection...", aulAllElements.size());

        for (it = aulAllElements.begin(); it != aulAllElements.end(); it++) {
            MeshGeomFacet clFacet = myMesh.GetFacet(*it);
            if (HasIntersection(clFacet))
                raulFacets.push_back(*it);
            seq.next();
        }
    }
    // cut outer
    else {
        Base::SequencerLauncher seq("Check facets for intersection...", myMesh.CountFacets());
        for (clIter.Init(); clIter.More(); clIter.Next()) {
            if (HasIntersection(*clIter))
                raulFacets.push_back(clIter.Position());
            seq.next();
        }
    }
}

bool MeshTrimming::HasIntersection(const MeshGeomFacet& rclFacet) const
{
    int i;
    unsigned long j;
    Base::Polygon2D clPoly;
    Base::Line2D clFacLine, clPolyLine;
    Base::Vector2D S;
    // is corner of facet inside the polygon
    for (i=0; i<3; i++) {
        Base::Vector3f clPt2d = myProj->operator ()(rclFacet._aclPoints[i]);
        if (myPoly.Contains(Base::Vector2D(clPt2d.x, clPt2d.y)) == myInner)
            return true;
        else
            clPoly.Add(Base::Vector2D(clPt2d.x, clPt2d.y));
    }

    // is corner of polygon inside the facet
    for (j=0; j<myPoly.GetCtVectors(); j++) {
        if (clPoly.Contains(myPoly[j]))
            return true;
    }
  
    // check for other intersections
    for (j=0; j<myPoly.GetCtVectors(); j++) {
        clPolyLine.clV1 = myPoly[j];
        clPolyLine.clV2 = myPoly[(j+1)%myPoly.GetCtVectors()];

        for (i=0; i<3; i++) {
            clFacLine.clV1 = clPoly[i];
            clFacLine.clV2 = clPoly[(i+1)%3];

            if (clPolyLine.IntersectAndContain(clFacLine, S))
                return true;
        }
    }

    // no intersection
    return false;
}

bool MeshTrimming::PolygonContainsCompleteFacet(bool bInner, unsigned long ulIndex) const
{
    const MeshFacet &rclFacet = myMesh._aclFacetArray[ulIndex];
    for (int i=0; i<3; i++) {
        const MeshPoint &rclFacPt = myMesh._aclPointArray[rclFacet._aulPoints[i]];
        Base::Vector3f clPt = (*myProj)(rclFacPt);
        if (myPoly.Contains(Base::Vector2D(clPt.x, clPt.y)) != bInner)
            return false;
    }

    return true;
}

bool MeshTrimming::IsPolygonPointInFacet(unsigned long ulIndex, Base::Vector3f& clPoint)
{
    Base::Vector2D A, B, C, P;
    float u,v,w, fDetPAC, fDetPBC, fDetPAB, fDetABC;
    Base::Polygon2D clFacPoly;
    const MeshGeomFacet &rclFacet = myMesh.GetFacet(ulIndex);
  
    for (int i=0; i<3; i++) {
        Base::Vector3f clPt = (*myProj)(rclFacet._aclPoints[i]);
        clFacPoly.Add(Base::Vector2D(clPt.x, clPt.y));
    }

    A = clFacPoly[0];
    B = clFacPoly[1];
    C = clFacPoly[2];
    fDetABC = (float)(A.fX*B.fY+A.fY*C.fX+B.fX*C.fY-(B.fY*C.fX+A.fY*B.fX+A.fX*C.fY));

    for (unsigned long j=0; j<myPoly.GetCtVectors(); j++) {
        // facet contains a polygon point -> calculate the corresponding 3d-point
        if (clFacPoly.Contains(myPoly[j])) {
            P = myPoly[j];
            fDetPAC = (float)(A.fX*P.fY+A.fY*C.fX+P.fX*C.fY-(P.fY*C.fX+A.fY*P.fX+A.fX*C.fY));
            fDetPBC = (float)(P.fX*B.fY+P.fY*C.fX+B.fX*C.fY-(B.fY*C.fX+P.fY*B.fX+P.fX*C.fY));
            fDetPAB = (float)(A.fX*B.fY+A.fY*P.fX+B.fX*P.fY-(B.fY*P.fX+A.fY*B.fX+A.fX*P.fY));
            u = fDetPBC / fDetABC;
            v = fDetPAC / fDetABC;
            w = fDetPAB / fDetABC;
      
            // point is on edge or no valid convex combination
            if (u == 0.0f || v == 0.0f || w == 0.0f || fabs(u+v+w-1.0f) >= 0.001)
                return false;
            // 3d point
            clPoint = u*rclFacet._aclPoints[0]+v*rclFacet._aclPoints[1]+w*rclFacet._aclPoints[2];

            return true;
        }
    }

    return false;
}

bool MeshTrimming::GetIntersectionPointsOfPolygonAndFacet(unsigned long ulIndex, int& iSide, std::vector<Base::Vector3f>& raclPoints) const
{
    MeshGeomFacet clFac(myMesh.GetFacet(ulIndex));
    Base::Vector2D S;
    Base::Line2D clFacLine, clPolyLine;
    int iIntersections=0;
    int iIntsctWithEdge0=0, iIntsctWithEdge1=0, iIntsctWithEdge2=0;
    // Edge with no intersection
    iSide = -1;

    for (unsigned long i=0; i<myPoly.GetCtVectors(); i++) {
        // totally only four intersections allowed
        if (iIntersections == 4)
            break;

        Base::Vector2D P3(myPoly[i]), P4(myPoly[(i+1)%myPoly.GetCtVectors()]);
        clPolyLine.clV1 = P3;
        clPolyLine.clV2 = P4;

        for (int j=0; j<3; j++) {
            Base::Vector3f clP1((*myProj)(clFac._aclPoints[j])); 
            Base::Vector3f clP2((*myProj)(clFac._aclPoints[(j+1)%3]));
            Base::Vector2D P1(clP1.x, clP1.y);
            Base::Vector2D P2(clP2.x, clP2.y);
            clFacLine.clV1 = P1;
            clFacLine.clV2 = P2;

            if (clPolyLine.Intersect(clFacLine, S)) {
                bool bPushBack=true;
                float fP1P2 = (float)(P2-P1).Length();
                float fSP1  = (float)(P1-S).Length();
                float fSP2  = (float)(P2-S).Length();

                float fP3P4 = (float)(P4-P3).Length();
                float fSP3  = (float)(P3-S).Length();
                float fSP4  = (float)(P4-S).Length();
                // compute propotion of length
                float l = fSP1 / fP1P2;
                float m = fSP2 / fP1P2;

                float r = fSP3 / fP3P4;
                float s = fSP4 / fP3P4;
 
                // is intersection point convex combination?
                if ((fabs(l+m-1.0f) < 0.001) && (fabs(r+s-1.0f) < 0.001)) {
#ifdef _DEBUG
                    Base::Vector3f clIntersection(m*clFac._aclPoints[j]+l*clFac._aclPoints[(j+1)%3]);
#endif
                    iIntersections++;
              
                    // only two intersections points per edge allowed
                    if (j == 0) {
                        if (iIntsctWithEdge0 == 2)
                            bPushBack = false;
                        else
                            iIntsctWithEdge0++;
                    }
                    else if (j == 1) {
                        if (iIntsctWithEdge1 == 2)
                            bPushBack = false;
                        else
                            iIntsctWithEdge1++;
                    }
                    else {
                        if (iIntsctWithEdge2 == 2)
                            bPushBack = false;
                        else
                            iIntsctWithEdge2++;
                    }

                    if (bPushBack == true)
                        raclPoints.push_back(m*clFac._aclPoints[j]+l*clFac._aclPoints[(j+1)%3]);
                }
            }
        }
    }

    // check for rotating facet later
    if (iIntsctWithEdge0 == 0)
        iSide = 0;
    else if (iIntsctWithEdge1 == 0)
        iSide = 1;
    else if (iIntsctWithEdge2 == 0)
        iSide = 2;

    // further check (for rotating the facet)
    if (iIntsctWithEdge0 == 0 && iIntsctWithEdge1 == 0)
        iSide = 1;
    else if (iIntsctWithEdge0 == 0 && iIntsctWithEdge2 == 0)
        iSide = 0;
    else if (iIntsctWithEdge1 == 0 && iIntsctWithEdge2 == 0)
        iSide = 2;

    // and last another check
    if (iIntsctWithEdge0 * iIntsctWithEdge1 * iIntsctWithEdge2 > 0) {
        if (iIntsctWithEdge0 == 2)
            iSide = 2;
        else if (iIntsctWithEdge1 == 2)
            iSide = 0;
        else if (iIntsctWithEdge2 == 2)
            iSide = 1;
    }

    return iIntersections > 0;
}

void MeshTrimming::AdjustFacet(MeshFacet& facet, int iInd)
{
    unsigned long tmp;

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

bool MeshTrimming::CreateFacets(unsigned long ulFacetPos, int iSide, const std::vector<Base::Vector3f>& raclPoints, std::vector<MeshGeomFacet>& aclNewFacets)
{
    MeshGeomFacet clFac;

    // no valid triangulation possible
    if (iSide == -1)
        return false;

    // two points found
    if (raclPoints.size() == 2) {
        MeshFacet& facet = myMesh._aclFacetArray[ulFacetPos];
        AdjustFacet(facet, iSide);
        Base::Vector3f clP1(raclPoints[0]), clP2(raclPoints[1]);
    
        if (iSide == 1) {
            // swap P1 and P2
            clP1 = raclPoints[1];
            clP2 = raclPoints[0];
        }

        // check which facets can be inserted
        int iCtPts=0;
        Base::Vector3f clFacPnt;
        Base::Vector2D clProjPnt;
        for (int i=0; i<3; i++) {
            clFacPnt = (*myProj)(myMesh._aclPointArray[facet._aulPoints[i]]);
            clProjPnt = Base::Vector2D(clFacPnt.x, clFacPnt.y);
            if (myPoly.Contains(clProjPnt) == myInner)
                ++iCtPts;
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
        else
            return false;
    }
    // four points found
    else if (raclPoints.size() == 4) {
        MeshFacet& facet = myMesh._aclFacetArray[ulFacetPos];
        AdjustFacet(facet, iSide);

        MeshFacet clOrg(myMesh._aclFacetArray[ulFacetPos]);
        clFac = myMesh.GetFacet(ulFacetPos);
        // intersection points
        Base::Vector3f clP1(raclPoints[0]), clP2(raclPoints[1]), clP3(raclPoints[2]), clP4(raclPoints[3]);

        // check which facets can be inserted
        int iCtPts=0;
        Base::Vector3f clFacPnt;
        Base::Vector2D clProjPnt;
        for (int i=0; i<3; i++) {
            clFacPnt = (*myProj)(myMesh._aclPointArray[facet._aulPoints[i]]);
            clProjPnt = Base::Vector2D(clFacPnt.x, clFacPnt.y);
            if (myPoly.Contains(clProjPnt) == myInner)
                ++iCtPts;
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

            if ((clP1-clFac._aclPoints[1]).Length() > (clP3-clFac._aclPoints[1]).Length()) {
                // swap P1 and P3
                Base::Vector3f tmp(clP1);
                clP1 = clP3;
                clP3 = tmp;
            }
            if ((clP2-clFac._aclPoints[0]).Length() > (clP4-clFac._aclPoints[0]).Length()) {
                // swap P2 and P4
                Base::Vector3f tmp(clP2);
                clP2 = clP4;
                clP4 = tmp;
            }
        }
        else {
            if (iSide == 0) {
                Base::Vector3f clNormal(clFac.GetNormal());
                MeshGeomFacet clTmpFac; clTmpFac._aclPoints[0] = clFac._aclPoints[1];
                clTmpFac._aclPoints[1] = clP2; clTmpFac._aclPoints[2] = clP1;
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
                if ((clP2-clFac._aclPoints[1]).Length() > (clP4-clFac._aclPoints[1]).Length()) {
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
                if ((clP1-clFac._aclPoints[1]).Length() > (clP3-clFac._aclPoints[1]).Length()) {
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
    else
        return false;

    return true;
}

bool MeshTrimming::CreateFacets(unsigned long ulFacetPos, int iSide, const std::vector<Base::Vector3f>& raclPoints, Base::Vector3f& clP3,
                                std::vector<MeshGeomFacet>& aclNewFacets)
{
    // no valid triangulation possible
    if (iSide == -1 || raclPoints.size() < 2)
        return false;

    Base::Vector3f clP1(raclPoints[0]);
    Base::Vector3f clP2(raclPoints[1]);

    MeshFacet& facet = myMesh._aclFacetArray[ulFacetPos];
    AdjustFacet(facet, iSide);

    MeshGeomFacet clFac;

    Base::Vector3f pnt = myMesh._aclPointArray[facet._aulPoints[1]];
    Base::Vector3f dir = myMesh._aclPointArray[facet._aulPoints[2]] - 
                         myMesh._aclPointArray[facet._aulPoints[1]];

    float fDistEdgeP1 = clP1.DistanceToLineSegment(
        myMesh._aclPointArray[facet._aulPoints[1]],
        myMesh._aclPointArray[facet._aulPoints[2]]).Length();
    float fDistEdgeP2 = clP2.DistanceToLineSegment(
        myMesh._aclPointArray[facet._aulPoints[1]],
        myMesh._aclPointArray[facet._aulPoints[2]]).Length();

    // swap P1 and P2
    if (fDistEdgeP2 < fDistEdgeP1) {
        Base::Vector3f tmp(clP1);
        clP1 = clP2;
        clP2 = tmp;
    }

    // check which facets should be inserted
    int iCtPts=0;
    Base::Vector3f clFacPnt;
    Base::Vector2D clProjPnt;
    for (int i=0; i<3; i++) {
        clFacPnt = (*myProj)(myMesh._aclPointArray[facet._aulPoints[i]]);
        clProjPnt = Base::Vector2D(clFacPnt.x, clFacPnt.y);
        if (myPoly.Contains(clProjPnt) == myInner)
            ++iCtPts;
    }
    if (iCtPts == 3) {
        clFac = myMesh.GetFacet(ulFacetPos);
        if ((clP1-clFac._aclPoints[1]).Length() > (clP2-clFac._aclPoints[1]).Length()) {
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
        if ((clP1-clFac._aclPoints[1]).Length() > (clP2-clFac._aclPoints[1]).Length()) {
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

void MeshTrimming::TrimFacets(const std::vector<unsigned long>& raulFacets, std::vector<MeshGeomFacet>& aclNewFacets)
{
    Base::Vector3f clP;
    std::vector<Base::Vector3f> clIntsct;
    int iSide;

    Base::SequencerLauncher seq("trimming facets...", raulFacets.size());
    for (std::vector<unsigned long>::const_iterator it=raulFacets.begin(); it!=raulFacets.end(); it++) {
        clIntsct.clear();
        if (IsPolygonPointInFacet(*it, clP) == false) {
            // facet must be trimmed
            if (PolygonContainsCompleteFacet(myInner, *it) == false) {
                // generate new facets
                if (GetIntersectionPointsOfPolygonAndFacet(*it, iSide, clIntsct))
                    CreateFacets(*it, iSide, clIntsct, myTriangles);
            }
        }
        // facet contains a polygon point
        else {
            // generate new facets
            if (GetIntersectionPointsOfPolygonAndFacet(*it, iSide, clIntsct))
                CreateFacets(*it, iSide, clIntsct, clP, myTriangles);
        }
        seq.next();
    }

    aclNewFacets = myTriangles;
}
