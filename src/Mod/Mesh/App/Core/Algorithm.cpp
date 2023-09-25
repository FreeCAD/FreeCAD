/***************************************************************************
 *   Copyright (c) 2005 Imetric 3D GmbH                                    *
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

#include <Base/Console.h>
#include <Base/Sequencer.h>

#include "Algorithm.h"
#include "Approximation.h"
#include "Elements.h"
#include "Grid.h"
#include "Iterator.h"
#include "Triangulation.h"


using namespace MeshCore;
using Base::BoundBox2d;
using Base::BoundBox3f;
using Base::Polygon2d;


bool MeshAlgorithm::IsVertexVisible(const Base::Vector3f& rcVertex,
                                    const Base::Vector3f& rcView,
                                    const MeshFacetGrid& rclGrid) const
{
    const float fMaxDistance = 0.001F;
    Base::Vector3f cDirection = rcVertex - rcView;
    float fDistance = cDirection.Length();
    Base::Vector3f cIntsct;
    FacetIndex uInd {};

    // search for the nearest facet to rcView in direction to rcVertex
    if (NearestFacetOnRay(rcView, cDirection, /*1.2f*fDistance,*/ rclGrid, cIntsct, uInd)) {
        // now check if the facet overlays the point
        float fLen = Base::Distance(rcView, cIntsct);
        if (fLen < fDistance) {
            // is it the same point?
            if (Base::Distance(rcVertex, cIntsct) > fMaxDistance) {
                // ok facet overlays the vertex
                return false;
            }
        }
    }

    return true;  // no facet between the two points
}

bool MeshAlgorithm::NearestFacetOnRay(const Base::Vector3f& rclPt,
                                      const Base::Vector3f& rclDir,
                                      Base::Vector3f& rclRes,
                                      FacetIndex& rulFacet) const
{
    return NearestFacetOnRay(rclPt, rclDir, Mathf::PI, rclRes, rulFacet);
}

bool MeshAlgorithm::NearestFacetOnRay(const Base::Vector3f& rclPt,
                                      const Base::Vector3f& rclDir,
                                      float fMaxAngle,
                                      Base::Vector3f& rclRes,
                                      FacetIndex& rulFacet) const
{
    Base::Vector3f clProj;
    Base::Vector3f clRes;
    bool bSol = false;
    FacetIndex ulInd = 0;

    // slow execution with no grid
    MeshFacetIterator clFIter(_rclMesh);
    for (clFIter.Init(); clFIter.More(); clFIter.Next()) {
        if (clFIter->Foraminate(rclPt, rclDir, clRes, fMaxAngle)) {
            if (!bSol) {
                // first solution
                bSol = true;
                clProj = clRes;
                ulInd = clFIter.Position();
            }
            else {
                // is closer to the point
                if ((clRes - rclPt).Length() < (clProj - rclPt).Length()) {
                    clProj = clRes;
                    ulInd = clFIter.Position();
                }
            }
        }
    }

    if (bSol) {
        rclRes = clProj;
        rulFacet = ulInd;
    }

    return bSol;
}

bool MeshAlgorithm::NearestFacetOnRay(const Base::Vector3f& rclPt,
                                      const Base::Vector3f& rclDir,
                                      const MeshFacetGrid& rclGrid,
                                      Base::Vector3f& rclRes,
                                      FacetIndex& rulFacet) const
{
    std::vector<FacetIndex> aulFacets;
    MeshGridIterator clGridIter(rclGrid);

    if (clGridIter.InitOnRay(rclPt, rclDir, aulFacets)) {
        if (!RayNearestField(rclPt, rclDir, aulFacets, rclRes, rulFacet)) {
            aulFacets.clear();
            while (clGridIter.NextOnRay(aulFacets)) {
                if (RayNearestField(rclPt, rclDir, aulFacets, rclRes, rulFacet)) {
                    return true;
                }
            }
        }
        else {
            return true;
        }
    }

    return false;
}

bool MeshAlgorithm::NearestFacetOnRay(const Base::Vector3f& rclPt,
                                      const Base::Vector3f& rclDir,
                                      float fMaxSearchArea,
                                      const MeshFacetGrid& rclGrid,
                                      Base::Vector3f& rclRes,
                                      FacetIndex& rulFacet) const
{
    const float fMaxAngle = 1.75F;
    std::vector<FacetIndex> aulFacets;
    MeshGridIterator clGridIter(rclGrid);

    if (clGridIter.InitOnRay(rclPt, rclDir, fMaxSearchArea, aulFacets)) {
        if (!RayNearestField(rclPt, rclDir, aulFacets, rclRes, rulFacet, fMaxAngle)) {
            aulFacets.clear();
            while (clGridIter.NextOnRay(aulFacets)) {
                if (RayNearestField(rclPt, rclDir, aulFacets, rclRes, rulFacet, fMaxAngle)) {
                    return true;
                }
            }
        }
        else {
            return true;
        }
    }

    return false;
}

bool MeshAlgorithm::NearestFacetOnRay(const Base::Vector3f& rclPt,
                                      const Base::Vector3f& rclDir,
                                      const std::vector<FacetIndex>& raulFacets,
                                      Base::Vector3f& rclRes,
                                      FacetIndex& rulFacet) const
{
    Base::Vector3f clProj;
    Base::Vector3f clRes;
    bool bSol = false;
    FacetIndex ulInd = 0;

    for (FacetIndex index : raulFacets) {
        MeshGeomFacet rclSFacet = _rclMesh.GetFacet(index);
        if (rclSFacet.Foraminate(rclPt, rclDir, clRes)) {
            if (!bSol) {  // first solution
                bSol = true;
                clProj = clRes;
                ulInd = index;
            }
            else {  // is closer to the point
                if ((clRes - rclPt).Length() < (clProj - rclPt).Length()) {
                    clProj = clRes;
                    ulInd = index;
                }
            }
        }
    }

    if (bSol) {
        rclRes = clProj;
        rulFacet = ulInd;
    }

    return bSol;
}

bool MeshAlgorithm::RayNearestField(const Base::Vector3f& rclPt,
                                    const Base::Vector3f& rclDir,
                                    const std::vector<FacetIndex>& raulFacets,
                                    Base::Vector3f& rclRes,
                                    FacetIndex& rulFacet,
                                    float /*fMaxAngle*/) const
{
    Base::Vector3f clProj, clRes;
    bool bSol = false;
    FacetIndex ulInd = 0;

    for (FacetIndex index : raulFacets) {
        if (_rclMesh.GetFacet(index).Foraminate(rclPt, rclDir, clRes /*, fMaxAngle*/)) {
            if (!bSol) {  // first solution
                bSol = true;
                clProj = clRes;
                ulInd = index;
            }
            else {  // is closer to the point
                if ((clRes - rclPt).Length() < (clProj - rclPt).Length()) {
                    clProj = clRes;
                    ulInd = index;
                }
            }
        }
    }

    if (bSol) {
        rclRes = clProj;
        rulFacet = ulInd;
    }

    return bSol;
}

bool MeshAlgorithm::FirstFacetToVertex(const Base::Vector3f& rPt,
                                       float fMaxDistance,
                                       const MeshFacetGrid& rGrid,
                                       FacetIndex& uIndex) const
{
    const float fEps = 0.001f;

    bool found = false;
    std::vector<FacetIndex> facets;

    // get the facets of the grid the point lies into
    rGrid.GetElements(rPt, facets);

    // Check all facets inside the grid if the point is part of it
    for (FacetIndex facet : facets) {
        MeshGeomFacet cFacet = this->_rclMesh.GetFacet(facet);
        if (cFacet.IsPointOfFace(rPt, fMaxDistance)) {
            found = true;
            uIndex = facet;
            break;
        }
        else {
            // if not then check the distance to the border of the triangle
            Base::Vector3f res;
            float fDist {};
            unsigned short uSide {};
            cFacet.ProjectPointToPlane(rPt, res);
            cFacet.NearestEdgeToPoint(res, fDist, uSide);
            if (fDist < fEps) {
                found = true;
                uIndex = facet;
                break;
            }
        }
    }

    return found;
}

float MeshAlgorithm::GetAverageEdgeLength() const
{
    float fLen = 0.0f;
    MeshFacetIterator cF(_rclMesh);
    for (cF.Init(); cF.More(); cF.Next()) {
        for (int i = 0; i < 3; i++) {
            fLen += Base::Distance(cF->_aclPoints[i], cF->_aclPoints[(i + 1) % 3]);
        }
    }

    fLen = fLen / (3.0f * _rclMesh.CountFacets());
    return fLen;
}

float MeshAlgorithm::GetMinimumEdgeLength() const
{
    float fLen = FLOAT_MAX;
    MeshFacetIterator cF(_rclMesh);
    for (cF.Init(); cF.More(); cF.Next()) {
        for (int i = 0; i < 3; i++) {
            fLen = std::min(fLen, Base::Distance(cF->_aclPoints[i], cF->_aclPoints[(i + 1) % 3]));
        }
    }

    return fLen;
}

float MeshAlgorithm::GetMaximumEdgeLength() const
{
    float fLen = 0.0f;
    MeshFacetIterator cF(_rclMesh);
    for (cF.Init(); cF.More(); cF.Next()) {
        for (int i = 0; i < 3; i++) {
            fLen = std::max(fLen, Base::Distance(cF->_aclPoints[i], cF->_aclPoints[(i + 1) % 3]));
        }
    }

    return fLen;
}

Base::Vector3f MeshAlgorithm::GetGravityPoint() const
{
    Base::Vector3f center;
    MeshPointIterator cP(_rclMesh);
    for (cP.Init(); cP.More(); cP.Next()) {
        center += *cP;
    }

    return center / static_cast<float>(_rclMesh.CountPoints());
}

void MeshAlgorithm::GetMeshBorders(std::list<std::vector<Base::Vector3f>>& rclBorders) const
{
    std::vector<FacetIndex> aulAllFacets(_rclMesh.CountFacets());
    FacetIndex k = 0;
    for (FacetIndex& index : aulAllFacets) {
        index = k++;
    }

    GetFacetBorders(aulAllFacets, rclBorders);
}

void MeshAlgorithm::GetMeshBorders(std::list<std::vector<PointIndex>>& rclBorders) const
{
    std::vector<FacetIndex> aulAllFacets(_rclMesh.CountFacets());
    FacetIndex k = 0;
    for (FacetIndex& index : aulAllFacets) {
        index = k++;
    }

    GetFacetBorders(aulAllFacets, rclBorders, true);
}

void MeshAlgorithm::GetFacetBorders(const std::vector<FacetIndex>& raulInd,
                                    std::list<std::vector<Base::Vector3f>>& rclBorders) const
{
    const MeshPointArray& rclPAry = _rclMesh._aclPointArray;
    std::list<std::vector<PointIndex>> aulBorders;

    GetFacetBorders(raulInd, aulBorders, true);
    for (const auto& border : aulBorders) {
        std::vector<Base::Vector3f> boundary;
        boundary.reserve(border.size());

        for (PointIndex jt : border) {
            boundary.push_back(rclPAry[jt]);
        }

        rclBorders.push_back(boundary);
    }
}

void MeshAlgorithm::GetFacetBorders(const std::vector<FacetIndex>& raulInd,
                                    std::list<std::vector<PointIndex>>& rclBorders,
                                    bool ignoreOrientation) const
{
    const MeshFacetArray& rclFAry = _rclMesh._aclFacetArray;

    // mark all facets that are in the indices list
    ResetFacetFlag(MeshFacet::VISIT);
    for (FacetIndex it : raulInd) {
        rclFAry[it].SetFlag(MeshFacet::VISIT);
    }

    // collect all boundary edges (unsorted)
    std::list<std::pair<PointIndex, PointIndex>> aclEdges;
    for (FacetIndex it : raulInd) {
        const MeshFacet& rclFacet = rclFAry[it];
        for (unsigned short i = 0; i < 3; i++) {
            FacetIndex ulNB = rclFacet._aulNeighbours[i];
            if (ulNB != FACET_INDEX_MAX) {
                if (rclFAry[ulNB].IsFlag(MeshFacet::VISIT)) {
                    continue;
                }
            }

            aclEdges.push_back(rclFacet.GetEdge(i));
        }
    }

    if (aclEdges.empty()) {
        return;  // no borders found (=> solid)
    }

    // search for edges in the unsorted list
    PointIndex ulFirst {}, ulLast {};
    std::list<PointIndex> clBorder;
    ulFirst = aclEdges.begin()->first;
    ulLast = aclEdges.begin()->second;

    aclEdges.erase(aclEdges.begin());
    clBorder.push_back(ulFirst);
    clBorder.push_back(ulLast);

    while (!aclEdges.empty()) {
        // get adjacent edge
        std::list<std::pair<PointIndex, PointIndex>>::iterator pEI;
        for (pEI = aclEdges.begin(); pEI != aclEdges.end(); ++pEI) {
            if (pEI->first == ulLast) {
                ulLast = pEI->second;
                clBorder.push_back(ulLast);
                aclEdges.erase(pEI);
                pEI = aclEdges.begin();
                break;
            }
            else if (pEI->second == ulFirst) {
                ulFirst = pEI->first;
                clBorder.push_front(ulFirst);
                aclEdges.erase(pEI);
                pEI = aclEdges.begin();
                break;
            }
            // Note: Using this might result into boundaries with wrong orientation.
            // But if the mesh has some facets with wrong orientation we might get
            // broken boundary curves.
            else if (pEI->second == ulLast && ignoreOrientation) {
                ulLast = pEI->first;
                clBorder.push_back(ulLast);
                aclEdges.erase(pEI);
                pEI = aclEdges.begin();
                break;
            }
            else if (pEI->first == ulFirst && ignoreOrientation) {
                ulFirst = pEI->second;
                clBorder.push_front(ulFirst);
                aclEdges.erase(pEI);
                pEI = aclEdges.begin();
                break;
            }
        }

        // Note: Calling erase on list iterators doesn't force a re-allocation and
        // thus doesn't invalidate the iterator itself, only the referenced object
        if ((pEI == aclEdges.end()) || aclEdges.empty() || (ulLast == ulFirst)) {
            // no further edge found or closed polyline, respectively
            rclBorders.emplace_back(clBorder.begin(), clBorder.end());
            clBorder.clear();

            if (!aclEdges.empty()) {
                // start new boundary
                ulFirst = aclEdges.begin()->first;
                ulLast = aclEdges.begin()->second;
                aclEdges.erase(aclEdges.begin());
                clBorder.push_back(ulFirst);
                clBorder.push_back(ulLast);
            }
        }
    }
}

void MeshAlgorithm::GetFacetBorder(FacetIndex uFacet, std::list<PointIndex>& rBorder) const
{
    const MeshFacetArray& rFAry = _rclMesh._aclFacetArray;
    std::list<std::pair<PointIndex, PointIndex>> openEdges;
    if (uFacet >= rFAry.size()) {
        return;
    }
    // add the open edge to the beginning of the list
    MeshFacetArray::_TConstIterator face = rFAry.begin() + uFacet;
    for (unsigned short i = 0; i < 3; i++) {
        if (face->_aulNeighbours[i] == FACET_INDEX_MAX) {
            openEdges.push_back(face->GetEdge(i));
        }
    }

    if (openEdges.empty()) {
        return;  // facet is not a border facet
    }

    for (MeshFacetArray::_TConstIterator it = rFAry.begin(); it != rFAry.end(); ++it) {
        if (it == face) {
            continue;
        }
        for (unsigned short i = 0; i < 3; i++) {
            if (it->_aulNeighbours[i] == FACET_INDEX_MAX) {
                openEdges.push_back(it->GetEdge(i));
            }
        }
    }

    SplitBoundaryFromOpenEdges(openEdges, rBorder);
}

void MeshAlgorithm::GetFacetsBorders(const std::vector<FacetIndex>& uFacets,
                                     std::list<std::vector<PointIndex>>& rBorders) const
{
    ResetFacetFlag(MeshFacet::TMP0);
    SetFacetsFlag(uFacets, MeshFacet::TMP0);
    ResetPointFlag(MeshPoint::TMP0);

    const MeshFacetArray& rFAry = _rclMesh._aclFacetArray;
    const MeshPointArray& rPAry = _rclMesh._aclPointArray;
    std::list<std::pair<PointIndex, PointIndex>> openEdges;

    // add the open edge to the beginning of the list
    for (auto it : uFacets) {
        const MeshFacet& face = rFAry[it];
        for (int i = 0; i < 3; i++) {
            if (face._aulNeighbours[i] == FACET_INDEX_MAX) {
                std::pair<PointIndex, PointIndex> openEdge = face.GetEdge(i);
                openEdges.push_back(openEdge);
                // mark all points of open edges of the given facets
                rPAry[openEdge.first].SetFlag(MeshPoint::TMP0);
                rPAry[openEdge.second].SetFlag(MeshPoint::TMP0);
            }
        }
    }

    if (openEdges.empty()) {
        return;  // none of the facets are border facets
    }

    for (const auto& it : rFAry) {
        if (it.IsFlag(MeshFacet::TMP0)) {
            continue;
        }
        for (int i = 0; i < 3; i++) {
            if (it._aulNeighbours[i] == FACET_INDEX_MAX) {
                openEdges.push_back(it.GetEdge(i));
            }
        }
    }

    // if the first element is not an edge of "uFacets" then give up
    while (!openEdges.empty()) {
        PointIndex first = openEdges.begin()->first;
        PointIndex second = openEdges.begin()->second;
        if (!rPAry[first].IsFlag(MeshPoint::TMP0)) {
            break;
        }
        if (!rPAry[second].IsFlag(MeshPoint::TMP0)) {
            break;
        }

        std::list<PointIndex> boundary;
        SplitBoundaryFromOpenEdges(openEdges, boundary);
        rBorders.emplace_back(boundary.begin(), boundary.end());
    }
}

void MeshAlgorithm::SplitBoundaryFromOpenEdges(
    std::list<std::pair<PointIndex, PointIndex>>& openEdges,
    std::list<PointIndex>& boundary) const
{
    // Start with the edge that is associated to uFacet
    if (openEdges.empty()) {
        return;
    }

    PointIndex ulFirst = openEdges.begin()->first;
    PointIndex ulLast = openEdges.begin()->second;

    openEdges.erase(openEdges.begin());
    boundary.push_back(ulFirst);
    boundary.push_back(ulLast);

    while (ulLast != ulFirst) {
        // find adjacent edge
        std::list<std::pair<PointIndex, PointIndex>>::iterator pEI;
        for (pEI = openEdges.begin(); pEI != openEdges.end(); ++pEI) {
            if (pEI->first == ulLast) {
                ulLast = pEI->second;
                boundary.push_back(ulLast);
                openEdges.erase(pEI);
                pEI = openEdges.begin();
                break;
            }
            else if (pEI->second == ulFirst) {
                ulFirst = pEI->first;
                boundary.push_front(ulFirst);
                openEdges.erase(pEI);
                pEI = openEdges.begin();
                break;
            }
        }

        // cannot close the border
        if (pEI == openEdges.end()) {
            break;
        }
    }
}

void MeshAlgorithm::SplitBoundaryLoops(std::list<std::vector<PointIndex>>& aBorders)
{
    // Count the number of open edges for each point
    std::map<PointIndex, int> openPointDegree;
    for (const auto& jt : _rclMesh._aclFacetArray) {
        for (int i = 0; i < 3; i++) {
            if (jt._aulNeighbours[i] == FACET_INDEX_MAX) {
                openPointDegree[jt._aulPoints[i]]++;
                openPointDegree[jt._aulPoints[(i + 1) % 3]]++;
            }
        }
    }

    // go through all boundaries and split them if needed
    std::list<std::vector<PointIndex>> aSplitBorders;
    for (const auto& aBorder : aBorders) {
        bool split = false;
        for (auto jt : aBorder) {
            // two (or more) boundaries meet in one non-manifold point
            if (openPointDegree[jt] > 2) {
                split = true;
                break;
            }
        }

        if (!split) {
            aSplitBorders.push_back(aBorder);
        }
        else {
            SplitBoundaryLoops(aBorder, aSplitBorders);
        }
    }

    aBorders = aSplitBorders;
}

void MeshAlgorithm::SplitBoundaryLoops(const std::vector<PointIndex>& rBound,
                                       std::list<std::vector<PointIndex>>& aBorders)
{
    std::map<PointIndex, int> aPtDegree;
    std::vector<PointIndex> cBound;
    for (PointIndex it : rBound) {
        int deg = (aPtDegree[it]++);
        if (deg > 0) {
            for (std::vector<PointIndex>::iterator jt = cBound.begin(); jt != cBound.end(); ++jt) {
                if (*jt == it) {
                    std::vector<PointIndex> cBoundLoop;
                    cBoundLoop.insert(cBoundLoop.end(), jt, cBound.end());
                    cBoundLoop.push_back(it);
                    cBound.erase(jt, cBound.end());
                    aBorders.push_back(cBoundLoop);
                    (aPtDegree[it]--);
                    break;
                }
            }
        }

        cBound.push_back(it);
    }
}

bool MeshAlgorithm::FillupHole(const std::vector<PointIndex>& boundary,
                               AbstractPolygonTriangulator& cTria,
                               MeshFacetArray& rFaces,
                               MeshPointArray& rPoints,
                               int level,
                               const MeshRefPointToFacets* pP2FStructure) const
{
    if (boundary.front() == boundary.back()) {
        // first and last vertex are identical
        if (boundary.size() < 4) {
            return false;  // something strange
        }
    }
    else if (boundary.size() < 3) {
        return false;  // something strange
    }

    // Get a facet as reference coordinate system
    MeshGeomFacet rTriangle;
    MeshFacet rFace;
    PointIndex refPoint0 = *(boundary.begin());
    PointIndex refPoint1 = *(boundary.begin() + 1);
    if (pP2FStructure) {
        const std::set<FacetIndex>& ring1 = (*pP2FStructure)[refPoint0];
        const std::set<FacetIndex>& ring2 = (*pP2FStructure)[refPoint1];
        std::vector<FacetIndex> f_int;
        std::set_intersection(ring1.begin(),
                              ring1.end(),
                              ring2.begin(),
                              ring2.end(),
                              std::back_insert_iterator<std::vector<FacetIndex>>(f_int));
        if (f_int.size() != 1) {
            return false;  // error, this must be an open edge!
        }

        rFace = _rclMesh._aclFacetArray[f_int.front()];
        rTriangle = _rclMesh.GetFacet(rFace);
    }
    else {
        bool ready = false;
        for (MeshFacetArray::_TConstIterator it = _rclMesh._aclFacetArray.begin();
             it != _rclMesh._aclFacetArray.end();
             ++it) {
            for (int i = 0; i < 3; i++) {
                if (((it->_aulPoints[i] == refPoint0) && (it->_aulPoints[(i + 1) % 3] == refPoint1))
                    || ((it->_aulPoints[i] == refPoint1)
                        && (it->_aulPoints[(i + 1) % 3] == refPoint0))) {
                    rFace = *it;
                    rTriangle = _rclMesh.GetFacet(*it);
                    ready = true;
                    break;
                }
            }

            if (ready) {
                break;
            }
        }
    }

    // add points to the polygon
    std::vector<Base::Vector3f> polygon;
    for (PointIndex jt : boundary) {
        polygon.push_back(_rclMesh._aclPointArray[jt]);
        rPoints.push_back(_rclMesh._aclPointArray[jt]);
    }

    // remove the last added point if it is duplicated
    std::vector<PointIndex> bounds = boundary;
    if (boundary.front() == boundary.back()) {
        bounds.pop_back();
        polygon.pop_back();
        rPoints.pop_back();
    }

    // There is no easy way to check whether the boundary is interior (a hole) or exterior before
    // performing the triangulation. Afterwards we can compare the normals of the created triangles
    // with the z-direction of our local coordinate system. If the scalar product is positive it was
    // a hole, otherwise not.
    cTria.SetPolygon(polygon);
    cTria.SetIndices(bounds);

    std::vector<Base::Vector3f> surf_pts = cTria.GetPolygon();
    if (pP2FStructure && level > 0) {
        std::set<PointIndex> index = pP2FStructure->NeighbourPoints(boundary, level);
        for (PointIndex it : index) {
            Base::Vector3f pt(_rclMesh._aclPointArray[it]);
            surf_pts.push_back(pt);
        }
    }

    if (cTria.TriangulatePolygon()) {
        // if we have enough points then we fit a surface through the points and project
        // the added points onto this surface
        cTria.PostProcessing(surf_pts);
        // get the facets and add the additional points to the array
        rFaces.insert(rFaces.end(), cTria.GetFacets().begin(), cTria.GetFacets().end());
        std::vector<Base::Vector3f> newVertices = cTria.AddedPoints();
        for (const auto& vertex : newVertices) {
            rPoints.push_back(vertex);
        }

        // Unfortunately, some algorithms do not care about the orientation of the polygon so we
        // cannot rely on the normal criterion to decide whether it's a hole or not.
        //
        std::vector<MeshFacet> faces = cTria.GetFacets();

        // Special case handling for a hole with three edges: the resulting facet might be
        // coincident with the reference facet
        if (faces.size() == 1) {
            MeshFacet first = faces.front();
            if (cTria.NeedsReindexing()) {
                first._aulPoints[0] = boundary[first._aulPoints[0]];
                first._aulPoints[1] = boundary[first._aulPoints[1]];
                first._aulPoints[2] = boundary[first._aulPoints[2]];
            }
            if (first.IsEqual(rFace)) {
                rFaces.clear();
                rPoints.clear();
                cTria.Discard();
                return false;
            }
        }

        // Get the new neighbour to our reference facet
        MeshFacet facet;
        unsigned short ref_side = rFace.Side(refPoint0, refPoint1);
        unsigned short tri_side = USHRT_MAX;
        if (cTria.NeedsReindexing()) {
            // the referenced indices of the polyline
            refPoint0 = 0;
            refPoint1 = 1;
        }
        if (ref_side < USHRT_MAX) {
            for (const auto& face : faces) {
                tri_side = face.Side(refPoint0, refPoint1);
                if (tri_side < USHRT_MAX) {
                    facet = face;
                    break;
                }
            }
        }

        // in case the reference facet has not an open edge print a log message
        if (ref_side == USHRT_MAX || tri_side == USHRT_MAX) {
            Base::Console().Log(
                "MeshAlgorithm::FillupHole: Expected open edge for facet <%d, %d, %d>\n",
                rFace._aulPoints[0],
                rFace._aulPoints[1],
                rFace._aulPoints[2]);
            rFaces.clear();
            rPoints.clear();
            cTria.Discard();
            return false;
        }

#if 1
        MeshGeomFacet triangle;
        triangle = cTria.GetTriangle(rPoints, facet);

        TriangulationVerifier* verifier = cTria.GetVerifier();
        if (!verifier) {
            return true;
        }

        // Now we have two adjacent triangles which we check for overlaps.
        // Therefore we build a separation plane that must separate the two diametrically opposed
        // points.
        Base::Vector3f planeNormal = rTriangle.GetNormal()
            % (rTriangle._aclPoints[(ref_side + 1) % 3] - rTriangle._aclPoints[ref_side]);
        Base::Vector3f planeBase = rTriangle._aclPoints[ref_side % 3];
        Base::Vector3f ref_point = rTriangle._aclPoints[(ref_side + 2) % 3];
        Base::Vector3f tri_point = triangle._aclPoints[(tri_side + 2) % 3];

        if (!verifier->Accept(planeNormal, planeBase, ref_point, tri_point)) {
            rFaces.clear();
            rPoints.clear();
            cTria.Discard();
            return false;
        }

        // we know to have filled a polygon, now check for the orientation
        if (verifier->MustFlip(triangle.GetNormal(), rTriangle.GetNormal())) {
            for (auto& rFace : rFaces) {
                rFace.FlipNormal();
            }
        }
#endif

        return true;
    }

    return false;
}

void MeshAlgorithm::SetFacetsProperty(const std::vector<FacetIndex>& raulInds,
                                      const std::vector<unsigned long>& raulProps) const
{
    if (raulInds.size() != raulProps.size()) {
        return;
    }

    std::vector<unsigned long>::const_iterator iP = raulProps.begin();
    for (std::vector<FacetIndex>::const_iterator i = raulInds.begin(); i != raulInds.end();
         ++i, ++iP) {
        _rclMesh._aclFacetArray[*i].SetProperty(*iP);
    }
}

void MeshAlgorithm::SetFacetsFlag(const std::vector<FacetIndex>& raulInds,
                                  MeshFacet::TFlagType tF) const
{
    for (FacetIndex it : raulInds) {
        _rclMesh._aclFacetArray[it].SetFlag(tF);
    }
}

void MeshAlgorithm::SetPointsFlag(const std::vector<FacetIndex>& raulInds,
                                  MeshPoint::TFlagType tF) const
{
    for (PointIndex it : raulInds) {
        _rclMesh._aclPointArray[it].SetFlag(tF);
    }
}

void MeshAlgorithm::GetFacetsFlag(std::vector<FacetIndex>& raulInds, MeshFacet::TFlagType tF) const
{
    raulInds.reserve(raulInds.size() + CountFacetFlag(tF));
    MeshFacetArray::_TConstIterator beg = _rclMesh._aclFacetArray.begin();
    MeshFacetArray::_TConstIterator end = _rclMesh._aclFacetArray.end();
    for (MeshFacetArray::_TConstIterator it = beg; it != end; ++it) {
        if (it->IsFlag(tF)) {
            raulInds.push_back(it - beg);
        }
    }
}

void MeshAlgorithm::GetPointsFlag(std::vector<PointIndex>& raulInds, MeshPoint::TFlagType tF) const
{
    raulInds.reserve(raulInds.size() + CountPointFlag(tF));
    MeshPointArray::_TConstIterator beg = _rclMesh._aclPointArray.begin();
    MeshPointArray::_TConstIterator end = _rclMesh._aclPointArray.end();
    for (MeshPointArray::_TConstIterator it = beg; it != end; ++it) {
        if (it->IsFlag(tF)) {
            raulInds.push_back(it - beg);
        }
    }
}

void MeshAlgorithm::ResetFacetsFlag(const std::vector<FacetIndex>& raulInds,
                                    MeshFacet::TFlagType tF) const
{
    for (FacetIndex it : raulInds) {
        _rclMesh._aclFacetArray[it].ResetFlag(tF);
    }
}

void MeshAlgorithm::ResetPointsFlag(const std::vector<FacetIndex>& raulInds,
                                    MeshPoint::TFlagType tF) const
{
    for (PointIndex it : raulInds) {
        _rclMesh._aclPointArray[it].ResetFlag(tF);
    }
}

void MeshAlgorithm::SetFacetFlag(MeshFacet::TFlagType tF) const
{
    _rclMesh._aclFacetArray.SetFlag(tF);
}

void MeshAlgorithm::SetPointFlag(MeshPoint::TFlagType tF) const
{
    _rclMesh._aclPointArray.SetFlag(tF);
}

void MeshAlgorithm::ResetFacetFlag(MeshFacet::TFlagType tF) const
{
    _rclMesh._aclFacetArray.ResetFlag(tF);
}

void MeshAlgorithm::ResetPointFlag(MeshPoint::TFlagType tF) const
{
    _rclMesh._aclPointArray.ResetFlag(tF);
}

unsigned long MeshAlgorithm::CountFacetFlag(MeshFacet::TFlagType tF) const
{
    MeshIsFlag<MeshFacet> flag;
    return std::count_if(_rclMesh._aclFacetArray.begin(),
                         _rclMesh._aclFacetArray.end(),
                         [flag, tF](const MeshFacet& f) {
                             return flag(f, tF);
                         });
}

unsigned long MeshAlgorithm::CountPointFlag(MeshPoint::TFlagType tF) const
{
    MeshIsFlag<MeshPoint> flag;
    return std::count_if(_rclMesh._aclPointArray.begin(),
                         _rclMesh._aclPointArray.end(),
                         [flag, tF](const MeshPoint& f) {
                             return flag(f, tF);
                         });
}

void MeshAlgorithm::GetFacetsFromToolMesh(const MeshKernel& rToolMesh,
                                          const Base::Vector3f& rcDir,
                                          std::vector<FacetIndex>& raclCutted) const
{
    MeshFacetIterator cFIt(_rclMesh);
    MeshFacetIterator cTIt(rToolMesh);

    BoundBox3f cBB = rToolMesh.GetBoundBox();

    Base::SequencerLauncher seq("Check facets...", _rclMesh.CountFacets());

    // check all facets
    Base::Vector3f tmp;
    for (cFIt.Init(); cFIt.More(); cFIt.Next()) {
        // check each point of each facet
        for (const auto& pnt : cFIt->_aclPoints) {
            // at least the point must be inside the bounding box of the tool mesh
            if (cBB.IsInBox(pnt)) {
                // should not cause performance problems since the tool mesh is usually rather
                // lightweight
                int ct = 0;
                for (cTIt.Init(); cTIt.More(); cTIt.Next()) {
                    if (cTIt->IsPointOfFace(pnt, MeshPoint::epsilon())) {
                        ct = 1;
                        break;  // the point lies on the tool mesh
                    }
                    else if (cTIt->Foraminate(pnt, rcDir, tmp)) {
                        // check if the intersection point lies in direction rcDir of the considered
                        // point
                        if ((tmp - pnt) * rcDir > 0) {
                            ct++;
                        }
                    }
                }

                // odd number => point is inside the tool mesh
                if (ct % 2 == 1) {
                    raclCutted.push_back(cFIt.Position());
                    break;
                }
            }
        }

        seq.next();
    }
}

void MeshAlgorithm::GetFacetsFromToolMesh(const MeshKernel& rToolMesh,
                                          const Base::Vector3f& rcDir,
                                          const MeshFacetGrid& rGrid,
                                          std::vector<FacetIndex>& raclCutted) const
{
    // iterator over grid structure
    MeshGridIterator clGridIter(rGrid);
    BoundBox3f cBB = rToolMesh.GetBoundBox();
    Base::Vector3f tmp;

    MeshFacetIterator cFIt(_rclMesh);
    MeshFacetIterator cTIt(rToolMesh);
    MeshAlgorithm cToolAlg(rToolMesh);

    // To speed up the algorithm we use the grid built up from the associated mesh. For each grid
    // element we check whether it lies completely inside or outside the toolmesh or even intersects
    // with the toolmesh. So we can reduce the number of facets with further tests dramatically.
    // If the grid box is outside the toolmesh all the facets inside can be skipped. If the grid
    // box is inside the toolmesh all facets are stored with no further tests because they must
    // also lie inside the toolmesh. Finally, if the grid box intersects with the toolmesh we must
    // also check for each whether it intersects with the toolmesh as well.
    std::vector<FacetIndex> aulInds;
    for (clGridIter.Init(); clGridIter.More(); clGridIter.Next()) {
        int ret = cToolAlg.Surround(clGridIter.GetBoundBox(), rcDir);

        // the box is completely inside the toolmesh
        if (ret == 1) {
            // these facets can be removed without more checks
            clGridIter.GetElements(raclCutted);
        }
        // the box intersects with toolmesh
        else if (ret == 0) {
            // these facets must be tested for intersections with the toolmesh
            clGridIter.GetElements(aulInds);
        }
        // the box is outside the toolmesh but this could still mean that the triangles
        // inside the grid intersect with the toolmesh
        else if (ret == -1) {
            // these facets must be tested for intersections with the toolmesh
            clGridIter.GetElements(aulInds);
        }
    }

    // remove duplicates
    std::sort(aulInds.begin(), aulInds.end());
    aulInds.erase(std::unique(aulInds.begin(), aulInds.end()), aulInds.end());
    std::sort(raclCutted.begin(), raclCutted.end());
    raclCutted.erase(std::unique(raclCutted.begin(), raclCutted.end()), raclCutted.end());

    Base::SequencerLauncher seq("Check facets...", aulInds.size());

    // check all facets
    for (FacetIndex it : aulInds) {
        cFIt.Set(it);

        // check each point of each facet
        for (auto point : cFIt->_aclPoints) {
            // at least the point must be inside the bounding box of the tool mesh
            if (cBB.IsInBox(point)) {
                // should not cause performance problems since the tool mesh is usually rather
                // lightweight
                int ct = 0;
                for (cTIt.Init(); cTIt.More(); cTIt.Next()) {
                    if (cTIt->IsPointOfFace(point, MeshPoint::epsilon())) {
                        ct = 1;
                        break;  // the point lies on the tool mesh
                    }
                    else if (cTIt->Foraminate(point, rcDir, tmp)) {
                        // check if the intersection point lies in direction rcDir of the considered
                        // point
                        if ((tmp - point) * rcDir > 0) {
                            ct++;
                        }
                    }
                }

                // odd number => point is inside the tool mesh
                if (ct % 2 == 1) {
                    raclCutted.push_back(cFIt.Position());
                    break;
                }
            }
        }

        seq.next();
    }

    // remove duplicates
    std::sort(raclCutted.begin(), raclCutted.end());
    raclCutted.erase(std::unique(raclCutted.begin(), raclCutted.end()), raclCutted.end());
}

int MeshAlgorithm::Surround(const Base::BoundBox3f& rBox, const Base::Vector3f& rcDir)
{
    Base::Vector3f pt1, pt2, tmp;
    const BoundBox3f& cBB = _rclMesh.GetBoundBox();

    // at least both boxes intersect
    if (cBB && rBox) {
        // check for intersections with the actual mesh
        Base::Vector3f cCorner[8] = {Base::Vector3f(rBox.MinX, rBox.MinY, rBox.MinZ),
                                     Base::Vector3f(rBox.MaxX, rBox.MinY, rBox.MinZ),
                                     Base::Vector3f(rBox.MaxX, rBox.MaxY, rBox.MinZ),
                                     Base::Vector3f(rBox.MinX, rBox.MaxY, rBox.MinZ),
                                     Base::Vector3f(rBox.MinX, rBox.MinY, rBox.MaxZ),
                                     Base::Vector3f(rBox.MaxX, rBox.MinY, rBox.MaxZ),
                                     Base::Vector3f(rBox.MaxX, rBox.MaxY, rBox.MaxZ),
                                     Base::Vector3f(rBox.MinX, rBox.MaxY, rBox.MaxZ)};

        MeshFacetIterator cTIt(_rclMesh);

        // triangulation of the box
        int triangles[36] = {0, 1, 2, 0, 2, 3, 0, 1, 5, 0, 5, 4, 0, 4, 7, 0, 7, 3,
                             6, 7, 4, 6, 4, 5, 6, 2, 3, 6, 3, 7, 6, 1, 2, 6, 5, 1};

        std::vector<MeshGeomFacet> cFacet(12);
        int id = 0;
        for (size_t ii = 0; ii < 12; ii++) {
            cFacet[ii]._aclPoints[0] = cCorner[triangles[id++]];
            cFacet[ii]._aclPoints[1] = cCorner[triangles[id++]];
            cFacet[ii]._aclPoints[2] = cCorner[triangles[id++]];
        }

        // check for intersections of the box with the mesh
        for (const auto& it : cFacet) {
            for (cTIt.Init(); cTIt.More(); cTIt.Next()) {
                int ret = cTIt->IntersectWithFacet(it, pt1, pt2);

                // the box intersects the mesh?
                if (ret != 0) {
                    return 0;  // => no more investigations required
                }
            }
        }

        // Now we know that the box doesn't intersect with the mesh. This means that either the box
        // is completely inside or outside the mesh. To check this we test one point of the box
        // whether it is inside or outside.
        int ct = 0;
        for (cTIt.Init(); cTIt.More(); cTIt.Next()) {
            if (cTIt->IsPointOfFace(cCorner[0], MeshPoint::epsilon())) {
                ct = 1;
                break;  // the point lies on the tool mesh
            }
            else if (cTIt->Foraminate(cCorner[0], rcDir, tmp)) {
                // check if the intersection point lies in direction rcDir of the considered point
                if ((tmp - cCorner[0]) * rcDir > 0) {
                    ct++;
                }
            }
        }

        // odd number => point (i.e. the box) is inside the mesh, even number => point is outside
        // the mesh
        return (ct % 2 == 1) ? 1 : -1;
    }

    // no intersection the box is outside the mesh
    return -1;
}

void MeshAlgorithm::CheckFacets(const MeshFacetGrid& rclGrid,
                                const Base::ViewProjMethod* pclProj,
                                const Base::Polygon2d& rclPoly,
                                bool bInner,
                                std::vector<FacetIndex>& raulFacets) const
{
    std::vector<FacetIndex>::iterator it;
    MeshFacetIterator clIter(_rclMesh, 0);
    Base::Vector3f clPt2d;
    Base::Vector3f clGravityOfFacet;
    bool bNoPointInside {};
    // Cache current view projection matrix since calls to Coin's projection are expensive
    Base::ViewProjMatrix fixedProj(pclProj->getComposedProjectionMatrix());
    // Precompute the polygon's bounding box
    Base::BoundBox2d clPolyBBox = rclPoly.CalcBoundBox();

    // if true use grid on mesh to speed up search
    if (bInner) {
        BoundBox3f clBBox3d;
        BoundBox2d clViewBBox;
        std::vector<FacetIndex> aulAllElements;
        // iterator for the bounding box grids
        MeshGridIterator clGridIter(rclGrid);
        for (clGridIter.Init(); clGridIter.More(); clGridIter.Next()) {
            clBBox3d = clGridIter.GetBoundBox();
            clViewBBox = clBBox3d.ProjectBox(&fixedProj);
            if (clViewBBox.Intersect(clPolyBBox)) {
                // collect all elements in aulAllElements
                clGridIter.GetElements(aulAllElements);
            }
        }

        // remove duplicates
        std::sort(aulAllElements.begin(), aulAllElements.end());
        aulAllElements.erase(std::unique(aulAllElements.begin(), aulAllElements.end()),
                             aulAllElements.end());

        Base::SequencerLauncher seq("Check facets", aulAllElements.size());

        for (it = aulAllElements.begin(); it != aulAllElements.end(); ++it) {
            bNoPointInside = true;
            clGravityOfFacet.Set(0.0f, 0.0f, 0.0f);
            MeshGeomFacet rclFacet = _rclMesh.GetFacet(*it);
            for (const auto& pnt : rclFacet._aclPoints) {
                clPt2d = fixedProj(pnt);
                clGravityOfFacet += clPt2d;
                if (clPolyBBox.Contains(Base::Vector2d(clPt2d.x, clPt2d.y))
                    && rclPoly.Contains(Base::Vector2d(clPt2d.x, clPt2d.y))) {
                    raulFacets.push_back(*it);
                    bNoPointInside = false;
                    break;
                }
            }

            // if no facet point is inside the polygon then check also the gravity
            if (bNoPointInside) {
                clGravityOfFacet *= 1.0f / 3.0f;

                if (clPolyBBox.Contains(Base::Vector2d(clGravityOfFacet.x, clGravityOfFacet.y))
                    && rclPoly.Contains(Base::Vector2d(clGravityOfFacet.x, clGravityOfFacet.y))) {
                    raulFacets.push_back(*it);
                }
            }

            seq.next();
        }
    }
    // When cutting triangles outside then go through all elements
    else {
        Base::SequencerLauncher seq("Check facets", _rclMesh.CountFacets());
        for (clIter.Init(); clIter.More(); clIter.Next()) {
            for (const auto& pnt : clIter->_aclPoints) {
                clPt2d = fixedProj(pnt);
                if ((clPolyBBox.Contains(Base::Vector2d(clPt2d.x, clPt2d.y))
                     && !rclPoly.Contains(Base::Vector2d(clPt2d.x, clPt2d.y)))) {
                    raulFacets.push_back(clIter.Position());
                    break;
                }
            }
            seq.next();
        }
    }
}

void MeshAlgorithm::CheckFacets(const Base::ViewProjMethod* pclProj,
                                const Base::Polygon2d& rclPoly,
                                bool bInner,
                                std::vector<FacetIndex>& raulFacets) const
{
    const MeshPointArray& p = _rclMesh.GetPoints();
    const MeshFacetArray& f = _rclMesh.GetFacets();
    Base::Vector3f pt2d;
    // Use a bounding box to reduce number of call to Polygon::Contains
    Base::BoundBox2d bb = rclPoly.CalcBoundBox();
    // Precompute the screen projection matrix as Coin's projection function is expensive
    Base::ViewProjMatrix fixedProj(pclProj->getComposedProjectionMatrix());

    FacetIndex index = 0;
    for (MeshFacetArray::_TConstIterator it = f.begin(); it != f.end(); ++it, ++index) {
        for (PointIndex ptIndex : it->_aulPoints) {
            pt2d = fixedProj(p[ptIndex]);

            // First check whether the point is in the bounding box of the polygon
            if ((bb.Contains(Base::Vector2d(pt2d.x, pt2d.y))
                 && rclPoly.Contains(Base::Vector2d(pt2d.x, pt2d.y)))
                ^ !bInner) {
                raulFacets.push_back(index);
                break;
            }
        }
    }
}

float MeshAlgorithm::Surface() const
{
    float fTotal = 0.0f;
    MeshFacetIterator clFIter(_rclMesh);

    for (clFIter.Init(); clFIter.More(); clFIter.Next()) {
        fTotal += clFIter->Area();
    }

    return fTotal;
}

void MeshAlgorithm::SubSampleByDist(float fDist, std::vector<Base::Vector3f>& rclPoints) const
{
    rclPoints.clear();
    MeshFacetIterator clFIter(_rclMesh);
    for (clFIter.Init(); clFIter.More(); clFIter.Next()) {
        size_t k = rclPoints.size();
        clFIter->SubSample(fDist, rclPoints);
        if (rclPoints.size() == k) {
            rclPoints.push_back(clFIter->GetGravityPoint());  // min. add middle point
        }
    }
}

void MeshAlgorithm::SubSampleAllPoints(std::vector<Base::Vector3f>& rclPoints) const
{
    rclPoints.clear();

    // Add all Points
    //
    MeshPointIterator clPIter(_rclMesh);
    for (clPIter.Init(); clPIter.More(); clPIter.Next()) {
        rclPoints.push_back(*clPIter);
    }
}

void MeshAlgorithm::SubSampleByCount(unsigned long ulCtPoints,
                                     std::vector<Base::Vector3f>& rclPoints) const
{
    float fDist = float(sqrt(Surface() / float(ulCtPoints)));
    SubSampleByDist(fDist, rclPoints);
}

void MeshAlgorithm::SearchFacetsFromPolyline(const std::vector<Base::Vector3f>& rclPolyline,
                                             float fRadius,
                                             const MeshFacetGrid& rclGrid,
                                             std::vector<FacetIndex>& rclResultFacetsIndices) const
{
    rclResultFacetsIndices.clear();
    if (rclPolyline.size() < 3) {
        return;  // no polygon defined
    }

    std::set<FacetIndex> aclFacets;
    for (std::vector<Base::Vector3f>::const_iterator pV = rclPolyline.begin();
         pV < (rclPolyline.end() - 1);
         ++pV) {
        const Base::Vector3f &rclP0 = *pV, &rclP1 = *(pV + 1);

        // BB eines Polyline-Segments
        BoundBox3f clSegmBB(rclP0.x, rclP0.y, rclP0.z, rclP0.x, rclP0.y, rclP0.z);
        clSegmBB.Add(rclP1);
        clSegmBB.Enlarge(fRadius);  // BB um Suchradius vergroessern

        std::vector<FacetIndex> aclBBFacets;
        unsigned long k = rclGrid.Inside(clSegmBB, aclBBFacets, false);
        for (unsigned long i = 0; i < k; i++) {
            if (_rclMesh.GetFacet(aclBBFacets[i]).DistanceToLineSegment(rclP0, rclP1) < fRadius) {
                aclFacets.insert(aclBBFacets[i]);
            }
        }
    }

    rclResultFacetsIndices.insert(rclResultFacetsIndices.begin(),
                                  aclFacets.begin(),
                                  aclFacets.end());
}

void MeshAlgorithm::CutBorderFacets(std::vector<FacetIndex>& raclFacetIndices,
                                    unsigned short usLevel) const
{
    std::vector<FacetIndex> aclToDelete;

    CheckBorderFacets(raclFacetIndices, aclToDelete, usLevel);

    // alle gefunden "Rand"-Facetsindizes" aus dem Array loeschen
    std::vector<FacetIndex> aclResult;
    std::set<FacetIndex> aclTmp(aclToDelete.begin(), aclToDelete.end());

    for (FacetIndex facetIndex : raclFacetIndices) {
        if (aclTmp.find(facetIndex) == aclTmp.end()) {
            aclResult.push_back(facetIndex);
        }
    }

    raclFacetIndices = aclResult;
}

unsigned long MeshAlgorithm::CountBorderEdges() const
{
    unsigned long cnt = 0;
    const MeshFacetArray& rclFAry = _rclMesh._aclFacetArray;
    MeshFacetArray::_TConstIterator end = rclFAry.end();
    for (MeshFacetArray::_TConstIterator it = rclFAry.begin(); it != end; ++it) {
        for (FacetIndex facetIndex : it->_aulNeighbours) {
            if (facetIndex == FACET_INDEX_MAX) {
                cnt++;
            }
        }
    }

    return cnt;
}

void MeshAlgorithm::CheckBorderFacets(const std::vector<FacetIndex>& raclFacetIndices,
                                      std::vector<FacetIndex>& raclResultIndices,
                                      unsigned short usLevel) const
{
    ResetFacetFlag(MeshFacet::TMP0);
    SetFacetsFlag(raclFacetIndices, MeshFacet::TMP0);

    const MeshFacetArray& rclFAry = _rclMesh._aclFacetArray;

    for (unsigned short usL = 0; usL < usLevel; usL++) {
        for (FacetIndex facetIndex : raclFacetIndices) {
            for (FacetIndex ulNB : rclFAry[facetIndex]._aulNeighbours) {
                if (ulNB == FACET_INDEX_MAX) {
                    raclResultIndices.push_back(facetIndex);
                    rclFAry[facetIndex].ResetFlag(MeshFacet::TMP0);
                    continue;
                }
                if (!rclFAry[ulNB].IsFlag(MeshFacet::TMP0)) {
                    raclResultIndices.push_back(facetIndex);
                    rclFAry[facetIndex].ResetFlag(MeshFacet::TMP0);
                    continue;
                }
            }
        }
    }
}

void MeshAlgorithm::GetBorderPoints(const std::vector<FacetIndex>& raclFacetIndices,
                                    std::set<PointIndex>& raclResultPointsIndices) const
{
    ResetFacetFlag(MeshFacet::TMP0);
    SetFacetsFlag(raclFacetIndices, MeshFacet::TMP0);

    const MeshFacetArray& rclFAry = _rclMesh._aclFacetArray;

    for (FacetIndex facetIndex : raclFacetIndices) {
        for (int i = 0; i < 3; i++) {
            const MeshFacet& rclFacet = rclFAry[facetIndex];
            FacetIndex ulNB = rclFacet._aulNeighbours[i];
            if (ulNB == FACET_INDEX_MAX) {
                raclResultPointsIndices.insert(rclFacet._aulPoints[i]);
                raclResultPointsIndices.insert(rclFacet._aulPoints[(i + 1) % 3]);
                continue;
            }
            if (!rclFAry[ulNB].IsFlag(MeshFacet::TMP0)) {
                raclResultPointsIndices.insert(rclFacet._aulPoints[i]);
                raclResultPointsIndices.insert(rclFacet._aulPoints[(i + 1) % 3]);
                continue;
            }
        }
    }
}

bool MeshAlgorithm::NearestPointFromPoint(const Base::Vector3f& rclPt,
                                          FacetIndex& rclResFacetIndex,
                                          Base::Vector3f& rclResPoint) const
{
    if (_rclMesh.CountFacets() == 0) {
        return false;
    }

    // calc each facet
    float fMinDist = FLOAT_MAX;
    FacetIndex ulInd = FACET_INDEX_MAX;
    MeshFacetIterator pF(_rclMesh);
    for (pF.Init(); pF.More(); pF.Next()) {
        float fDist = pF->DistanceToPoint(rclPt);
        if (fDist < fMinDist) {
            fMinDist = fDist;
            ulInd = pF.Position();
        }
    }

    MeshGeomFacet rclSFacet = _rclMesh.GetFacet(ulInd);
    rclSFacet.DistanceToPoint(rclPt, rclResPoint);
    rclResFacetIndex = ulInd;

    return true;
}

bool MeshAlgorithm::NearestPointFromPoint(const Base::Vector3f& rclPt,
                                          const MeshFacetGrid& rclGrid,
                                          FacetIndex& rclResFacetIndex,
                                          Base::Vector3f& rclResPoint) const
{
    FacetIndex ulInd = rclGrid.SearchNearestFromPoint(rclPt);

    if (ulInd == FACET_INDEX_MAX) {
        return false;
    }

    MeshGeomFacet rclSFacet = _rclMesh.GetFacet(ulInd);
    rclSFacet.DistanceToPoint(rclPt, rclResPoint);
    rclResFacetIndex = ulInd;

    return true;
}

bool MeshAlgorithm::NearestPointFromPoint(const Base::Vector3f& rclPt,
                                          const MeshFacetGrid& rclGrid,
                                          float fMaxSearchArea,
                                          FacetIndex& rclResFacetIndex,
                                          Base::Vector3f& rclResPoint) const
{
    FacetIndex ulInd = rclGrid.SearchNearestFromPoint(rclPt, fMaxSearchArea);

    if (ulInd == FACET_INDEX_MAX) {
        return false;  // no facets inside BoundingBox
    }

    MeshGeomFacet rclSFacet = _rclMesh.GetFacet(ulInd);
    rclSFacet.DistanceToPoint(rclPt, rclResPoint);
    rclResFacetIndex = ulInd;

    return true;
}

bool MeshAlgorithm::CutWithPlane(const Base::Vector3f& clBase,
                                 const Base::Vector3f& clNormal,
                                 const MeshFacetGrid& rclGrid,
                                 std::list<std::vector<Base::Vector3f>>& rclResult,
                                 float fMinEps,
                                 bool bConnectPolygons) const
{
    std::vector<FacetIndex> aulFacets;

    // Search grid
    MeshGridIterator clGridIter(rclGrid);
    for (clGridIter.Init(); clGridIter.More(); clGridIter.Next()) {
        // if Gridvoxel intersects the plane: pick up all facets for cutting
        if (clGridIter.GetBoundBox().IsCutPlane(clBase, clNormal)) {
            clGridIter.GetElements(aulFacets);
        }
    }

    // remove multiple triangles
    std::sort(aulFacets.begin(), aulFacets.end());
    aulFacets.erase(std::unique(aulFacets.begin(), aulFacets.end()), aulFacets.end());

    // intersect all facets with plane
    std::list<std::pair<Base::Vector3f, Base::Vector3f>>
        clTempPoly;  // Field with intersection lines (unsorted, not chained)

    for (FacetIndex facetIndex : aulFacets) {
        Base::Vector3f clE1, clE2;
        const MeshGeomFacet clF(_rclMesh.GetFacet(facetIndex));

        // Cut the facet and store the cutting path
        if (clF.IntersectWithPlane(clBase, clNormal, clE1, clE2)) {
            clTempPoly.emplace_back(clE1, clE2);
        }
    }

    if (bConnectPolygons) {
        // std::list<std::pair<Base::Vector3f, Base::Vector3f> > rclTempLines;
        std::list<std::pair<Base::Vector3f, Base::Vector3f>> rclResultLines(clTempPoly.begin(),
                                                                            clTempPoly.end());
        std::list<std::vector<Base::Vector3f>> tempList;
        ConnectLines(clTempPoly, tempList, fMinEps);
        ConnectPolygons(tempList, clTempPoly);

        for (auto& iter : clTempPoly) {
            rclResultLines.push_front(iter);
        }

        return ConnectLines(rclResultLines, rclResult, fMinEps);
    }

    return ConnectLines(clTempPoly, rclResult, fMinEps);
}

bool MeshAlgorithm::ConnectLines(std::list<std::pair<Base::Vector3f, Base::Vector3f>>& rclLines,
                                 std::list<std::vector<Base::Vector3f>>& rclPolylines,
                                 float fMinEps) const
{
    using TCIter = std::list<std::pair<Base::Vector3f, Base::Vector3f>>::iterator;

    // square search radius
    // const float fMinEps = 1.0e-2f; // := 10 micrometer distance
    fMinEps = fMinEps * fMinEps;

    // remove all lines whose distance is smaller than epsilon
    std::list<TCIter> _clToDelete;
    float fToDelDist = fMinEps / 10.0f;
    for (TCIter pF = rclLines.begin(); pF != rclLines.end(); ++pF) {
        if (Base::DistanceP2(pF->first, pF->second) < fToDelDist) {
            _clToDelete.push_back(pF);
        }
    }

    for (auto& pI : _clToDelete) {
        rclLines.erase(pI);
    }

    while (!rclLines.empty()) {
        TCIter pF;

        // new polyline
        std::list<Base::Vector3f> clPoly;

        // add first line and delete from the list
        Base::Vector3f clFront = rclLines.begin()->first;  // current start point of the polyline
        Base::Vector3f clEnd = rclLines.begin()->second;   // current end point of the polyline
        clPoly.push_back(clFront);
        clPoly.push_back(clEnd);
        rclLines.erase(rclLines.begin());

        // search for the next line on the begin/end of the polyline and add it
        TCIter pFront, pEnd;
        bool bFoundLine {};
        do {
            float fFrontMin = fMinEps, fEndMin = fMinEps;
            bool bFrontFirst = false, bEndFirst = false;

            pFront = rclLines.end();
            pEnd = rclLines.end();
            bFoundLine = false;

            for (pF = rclLines.begin(); pF != rclLines.end(); ++pF) {
                if (Base::DistanceP2(clFront, pF->first) < fFrontMin) {
                    fFrontMin = Base::DistanceP2(clFront, pF->first);
                    pFront = pF;
                    bFrontFirst = true;
                }
                else if (Base::DistanceP2(clEnd, pF->first) < fEndMin) {
                    fEndMin = Base::DistanceP2(clEnd, pF->first);
                    pEnd = pF;
                    bEndFirst = true;
                }
                else if (Base::DistanceP2(clFront, pF->second) < fFrontMin) {
                    fFrontMin = Base::DistanceP2(clFront, pF->second);
                    pFront = pF;
                    bFrontFirst = false;
                }
                else if (Base::DistanceP2(clEnd, pF->second) < fEndMin) {
                    fEndMin = Base::DistanceP2(clEnd, pF->second);
                    pEnd = pF;
                    bEndFirst = false;
                }
            }

            if (pFront != rclLines.end()) {
                bFoundLine = true;
                if (bFrontFirst) {
                    clPoly.push_front(pFront->second);
                    clFront = pFront->second;
                }
                else {
                    clPoly.push_front(pFront->first);
                    clFront = pFront->first;
                }

                rclLines.erase(pFront);
            }

            if (pEnd != rclLines.end()) {
                bFoundLine = true;
                if (bEndFirst) {
                    clPoly.push_back(pEnd->second);
                    clEnd = pEnd->second;
                }
                else {
                    clPoly.push_back(pEnd->first);
                    clEnd = pEnd->first;
                }

                rclLines.erase(pEnd);
            }
        } while (bFoundLine);

        rclPolylines.emplace_back(clPoly.begin(), clPoly.end());
    }

    // remove all polylines with too few length
    using TPIter = std::list<std::vector<Base::Vector3f>>::iterator;
    std::list<TPIter> _clPolyToDelete;
    for (TPIter pJ = rclPolylines.begin(); pJ != rclPolylines.end(); ++pJ) {
        if (pJ->size() == 2) {  // only one line segment
            if (Base::DistanceP2(*pJ->begin(), *(pJ->begin() + 1)) <= fMinEps) {
                _clPolyToDelete.push_back(pJ);
            }
        }
    }

    for (auto& pK : _clPolyToDelete) {
        rclPolylines.erase(pK);
    }

    return true;
}

bool MeshAlgorithm::ConnectPolygons(
    std::list<std::vector<Base::Vector3f>>& clPolyList,
    std::list<std::pair<Base::Vector3f, Base::Vector3f>>& rclLines) const
{

    for (std::list<std::vector<Base::Vector3f>>::iterator OutIter = clPolyList.begin();
         OutIter != clPolyList.end();
         ++OutIter) {
        if (OutIter->empty()) {
            continue;
        }
        std::pair<Base::Vector3f, Base::Vector3f> currentSort;
        float fDist = Base::Distance(OutIter->front(), OutIter->back());
        currentSort.first = OutIter->front();
        currentSort.second = OutIter->back();

        for (std::list<std::vector<Base::Vector3f>>::iterator InnerIter = clPolyList.begin();
             InnerIter != clPolyList.end();
             ++InnerIter) {
            if (OutIter == InnerIter) {
                continue;
            }

            if (Base::Distance(OutIter->front(), InnerIter->front()) < fDist) {
                currentSort.second = InnerIter->front();
                fDist = Base::Distance(OutIter->front(), InnerIter->front());
            }

            if (Base::Distance(OutIter->front(), InnerIter->back()) < fDist) {
                currentSort.second = InnerIter->back();
                fDist = Base::Distance(OutIter->front(), InnerIter->back());
            }
        }

        rclLines.push_front(currentSort);
    }

    return true;
}

void MeshAlgorithm::GetFacetsFromPlane(const MeshFacetGrid& rclGrid,
                                       const Base::Vector3f& clNormal,
                                       float d,
                                       const Base::Vector3f& rclLeft,
                                       const Base::Vector3f& rclRight,
                                       std::vector<FacetIndex>& rclRes) const
{
    std::vector<FacetIndex> aulFacets;

    Base::Vector3f clBase = d * clNormal;

    Base::Vector3f clPtNormal(rclLeft - rclRight);
    clPtNormal.Normalize();

    // search grid
    MeshGridIterator clGridIter(rclGrid);
    for (clGridIter.Init(); clGridIter.More(); clGridIter.Next()) {
        // add facets from grid if the plane if cut the grid-voxel
        if (clGridIter.GetBoundBox().IsCutPlane(clBase, clNormal)) {
            clGridIter.GetElements(aulFacets);
        }
    }

    // testing facet against planes
    for (FacetIndex facetIndex : aulFacets) {
        MeshGeomFacet clSFacet = _rclMesh.GetFacet(facetIndex);
        if (clSFacet.IntersectWithPlane(clBase, clNormal)) {
            bool bInner = false;
            for (int i = 0; (i < 3) && !bInner; i++) {
                Base::Vector3f clPt = clSFacet._aclPoints[i];
                if ((clPt.DistanceToPlane(rclLeft, clPtNormal) <= 0.0f)
                    && (clPt.DistanceToPlane(rclRight, clPtNormal) >= 0.0f)) {
                    bInner = true;
                }
            }

            if (bInner) {
                rclRes.push_back(facetIndex);
            }
        }
    }
}

void MeshAlgorithm::PointsFromFacetsIndices(const std::vector<FacetIndex>& rvecIndices,
                                            std::vector<Base::Vector3f>& rvecPoints) const
{
    const MeshFacetArray& rclFAry = _rclMesh._aclFacetArray;
    const MeshPointArray& rclPAry = _rclMesh._aclPointArray;

    std::set<PointIndex> setPoints;

    for (FacetIndex facetIndex : rvecIndices) {
        for (PointIndex pointIndex : rclFAry[facetIndex]._aulPoints) {
            setPoints.insert(pointIndex);
        }
    }

    rvecPoints.clear();
    for (PointIndex pointIndex : setPoints) {
        rvecPoints.push_back(rclPAry[pointIndex]);
    }
}

bool MeshAlgorithm::Distance(const Base::Vector3f& rclPt,
                             FacetIndex ulFacetIdx,
                             float fMaxDistance,
                             float& rfDistance) const
{
    const MeshFacetArray& rclFAry = _rclMesh._aclFacetArray;
    const MeshPointArray& rclPAry = _rclMesh._aclPointArray;
    const PointIndex* pulIdx = rclFAry[ulFacetIdx]._aulPoints;

    BoundBox3f clBB;
    clBB.Add(rclPAry[*(pulIdx++)]);
    clBB.Add(rclPAry[*(pulIdx++)]);
    clBB.Add(rclPAry[*pulIdx]);
    clBB.Enlarge(fMaxDistance);

    if (!clBB.IsInBox(rclPt)) {
        return false;
    }

    rfDistance = _rclMesh.GetFacet(ulFacetIdx).DistanceToPoint(rclPt);

    return rfDistance < fMaxDistance;
}

float MeshAlgorithm::CalculateMinimumGridLength(float fLength,
                                                const Base::BoundBox3f& rBBox,
                                                unsigned long maxElements) const
{
    // Max. limit of grid elements
    float fMaxGridElements = static_cast<float>(maxElements);

    // estimate the minimum allowed grid length
    float fMinGridLen = static_cast<float>(
        pow((rBBox.LengthX() * rBBox.LengthY() * rBBox.LengthZ() / fMaxGridElements), 0.3333f));
    return std::max<float>(fMinGridLen, fLength);
}

// ----------------------------------------------------

void MeshRefPointToFacets::Rebuild()
{
    _map.clear();

    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();
    _map.resize(rPoints.size());

    MeshFacetArray::_TConstIterator pFBegin = rFacets.begin();
    for (MeshFacetArray::_TConstIterator pFIter = rFacets.begin(); pFIter != rFacets.end();
         ++pFIter) {
        _map[pFIter->_aulPoints[0]].insert(pFIter - pFBegin);
        _map[pFIter->_aulPoints[1]].insert(pFIter - pFBegin);
        _map[pFIter->_aulPoints[2]].insert(pFIter - pFBegin);
    }
}

Base::Vector3f MeshRefPointToFacets::GetNormal(PointIndex pos) const
{
    const std::set<FacetIndex>& n = _map[pos];
    Base::Vector3f normal;
    MeshGeomFacet f;
    for (FacetIndex it : n) {
        f = _rclMesh.GetFacet(it);
        normal += f.Area() * f.GetNormal();
    }

    normal.Normalize();
    return normal;
}

std::set<PointIndex> MeshRefPointToFacets::NeighbourPoints(const std::vector<PointIndex>& pt,
                                                           int level) const
{
    std::set<PointIndex> cp, nb, lp;
    cp.insert(pt.begin(), pt.end());
    lp.insert(pt.begin(), pt.end());
    MeshFacetArray::_TConstIterator f_it = _rclMesh.GetFacets().begin();
    for (int i = 0; i < level; i++) {
        std::set<PointIndex> cur;
        for (PointIndex it : lp) {
            const std::set<FacetIndex>& ft = (*this)[it];
            for (FacetIndex jt : ft) {
                for (PointIndex index : f_it[jt]._aulPoints) {
                    if (cp.find(index) == cp.end() && nb.find(index) == nb.end()) {
                        nb.insert(index);
                        cur.insert(index);
                    }
                }
            }
        }

        lp = cur;
        if (lp.empty()) {
            break;
        }
    }
    return nb;
}

std::set<PointIndex> MeshRefPointToFacets::NeighbourPoints(PointIndex pos) const
{
    std::set<PointIndex> p;
    const std::set<FacetIndex>& vf = _map[pos];
    for (FacetIndex it : vf) {
        PointIndex p1 {}, p2 {}, p3 {};
        _rclMesh.GetFacetPoints(it, p1, p2, p3);
        if (p1 != pos) {
            p.insert(p1);
        }
        if (p2 != pos) {
            p.insert(p2);
        }
        if (p3 != pos) {
            p.insert(p3);
        }
    }

    return p;
}

void MeshRefPointToFacets::Neighbours(FacetIndex ulFacetInd,
                                      float fMaxDist,
                                      MeshCollector& collect) const
{
    std::set<FacetIndex> visited;
    Base::Vector3f clCenter = _rclMesh.GetFacet(ulFacetInd).GetGravityPoint();

    const MeshFacetArray& rFacets = _rclMesh.GetFacets();
    SearchNeighbours(rFacets, ulFacetInd, clCenter, fMaxDist * fMaxDist, visited, collect);
}

void MeshRefPointToFacets::SearchNeighbours(const MeshFacetArray& rFacets,
                                            FacetIndex index,
                                            const Base::Vector3f& rclCenter,
                                            float fMaxDist2,
                                            std::set<FacetIndex>& visited,
                                            MeshCollector& collect) const
{
    if (visited.find(index) != visited.end()) {
        return;
    }

    const MeshFacet& face = rFacets[index];
    if (Base::DistanceP2(rclCenter, _rclMesh.GetFacet(face).GetGravityPoint()) > fMaxDist2) {
        return;
    }

    visited.insert(index);
    collect.Append(_rclMesh, index);
    for (PointIndex ptIndex : face._aulPoints) {
        const std::set<FacetIndex>& f = (*this)[ptIndex];

        for (FacetIndex j : f) {
            SearchNeighbours(rFacets, j, rclCenter, fMaxDist2, visited, collect);
        }
    }
}

MeshFacetArray::_TConstIterator MeshRefPointToFacets::GetFacet(FacetIndex index) const
{
    return _rclMesh.GetFacets().begin() + index;
}

const std::set<FacetIndex>& MeshRefPointToFacets::operator[](PointIndex pos) const
{
    return _map[pos];
}

std::vector<FacetIndex> MeshRefPointToFacets::GetIndices(PointIndex pos1, PointIndex pos2) const
{
    std::vector<FacetIndex> intersection;
    std::back_insert_iterator<std::vector<FacetIndex>> result(intersection);
    const std::set<FacetIndex>& set1 = _map[pos1];
    const std::set<FacetIndex>& set2 = _map[pos2];
    std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(), result);
    return intersection;
}

std::vector<FacetIndex>
MeshRefPointToFacets::GetIndices(PointIndex pos1, PointIndex pos2, PointIndex pos3) const
{
    std::vector<FacetIndex> intersection;
    std::back_insert_iterator<std::vector<FacetIndex>> result(intersection);
    std::vector<FacetIndex> set1 = GetIndices(pos1, pos2);
    const std::set<FacetIndex>& set2 = _map[pos3];
    std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(), result);
    return intersection;
}

void MeshRefPointToFacets::AddNeighbour(PointIndex pos, FacetIndex facet)
{
    _map[pos].insert(facet);
}

void MeshRefPointToFacets::RemoveNeighbour(PointIndex pos, FacetIndex facet)
{
    _map[pos].erase(facet);
}

void MeshRefPointToFacets::RemoveFacet(FacetIndex facetIndex)
{
    PointIndex p0 {}, p1 {}, p2 {};
    _rclMesh.GetFacetPoints(facetIndex, p0, p1, p2);

    _map[p0].erase(facetIndex);
    _map[p1].erase(facetIndex);
    _map[p2].erase(facetIndex);
}

//----------------------------------------------------------------------------

void MeshRefFacetToFacets::Rebuild()
{
    _map.clear();

    const MeshFacetArray& rFacets = _rclMesh.GetFacets();
    _map.resize(rFacets.size());

    MeshRefPointToFacets vertexFace(_rclMesh);
    MeshFacetArray::_TConstIterator pFBegin = rFacets.begin();
    for (MeshFacetArray::_TConstIterator pFIter = pFBegin; pFIter != rFacets.end(); ++pFIter) {
        for (PointIndex ptIndex : pFIter->_aulPoints) {
            const std::set<FacetIndex>& faces = vertexFace[ptIndex];
            for (FacetIndex face : faces) {
                _map[pFIter - pFBegin].insert(face);
            }
        }
    }
}

const std::set<FacetIndex>& MeshRefFacetToFacets::operator[](FacetIndex pos) const
{
    return _map[pos];
}

std::vector<FacetIndex> MeshRefFacetToFacets::GetIndices(FacetIndex pos1, FacetIndex pos2) const
{
    std::vector<FacetIndex> intersection;
    std::back_insert_iterator<std::vector<FacetIndex>> result(intersection);
    const std::set<FacetIndex>& set1 = _map[pos1];
    const std::set<FacetIndex>& set2 = _map[pos2];
    std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(), result);
    return intersection;
}

//----------------------------------------------------------------------------

void MeshRefPointToPoints::Rebuild()
{
    _map.clear();

    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    _map.resize(rPoints.size());

    const MeshFacetArray& rFacets = _rclMesh.GetFacets();
    for (const auto& rFacet : rFacets) {
        PointIndex ulP0 = rFacet._aulPoints[0];
        PointIndex ulP1 = rFacet._aulPoints[1];
        PointIndex ulP2 = rFacet._aulPoints[2];

        _map[ulP0].insert(ulP1);
        _map[ulP0].insert(ulP2);
        _map[ulP1].insert(ulP0);
        _map[ulP1].insert(ulP2);
        _map[ulP2].insert(ulP0);
        _map[ulP2].insert(ulP1);
    }
}

Base::Vector3f MeshRefPointToPoints::GetNormal(PointIndex pos) const
{
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    MeshCore::PlaneFit pf;
    pf.AddPoint(rPoints[pos]);
    MeshCore::MeshPoint center = rPoints[pos];
    const std::set<PointIndex>& cv = _map[pos];
    for (PointIndex cv_it : cv) {
        pf.AddPoint(rPoints[cv_it]);
        center += rPoints[cv_it];
    }

    pf.Fit();

    Base::Vector3f normal = pf.GetNormal();
    normal.Normalize();
    return normal;
}

float MeshRefPointToPoints::GetAverageEdgeLength(PointIndex index) const
{
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    float len = 0.0f;
    const std::set<PointIndex>& n = (*this)[index];
    const Base::Vector3f& p = rPoints[index];
    for (PointIndex it : n) {
        len += Base::Distance(p, rPoints[it]);
    }
    return (len / n.size());
}

const std::set<PointIndex>& MeshRefPointToPoints::operator[](PointIndex pos) const
{
    return _map[pos];
}

void MeshRefPointToPoints::AddNeighbour(PointIndex pos, PointIndex facet)
{
    _map[pos].insert(facet);
}

void MeshRefPointToPoints::RemoveNeighbour(PointIndex pos, PointIndex facet)
{
    _map[pos].erase(facet);
}

//----------------------------------------------------------------------------

void MeshRefEdgeToFacets::Rebuild()
{
    _map.clear();

    const MeshFacetArray& rFacets = _rclMesh.GetFacets();
    FacetIndex index = 0;
    for (MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it, ++index) {
        for (int i = 0; i < 3; i++) {
            MeshEdge e;
            e.first = it->_aulPoints[i];
            e.second = it->_aulPoints[(i + 1) % 3];
            std::map<MeshEdge, MeshFacetPair, EdgeOrder>::iterator jt = _map.find(e);
            if (jt == _map.end()) {
                _map[e].first = index;
                _map[e].second = FACET_INDEX_MAX;
            }
            else {
                _map[e].second = index;
            }
        }
    }
}

const std::pair<FacetIndex, FacetIndex>& MeshRefEdgeToFacets::operator[](const MeshEdge& edge) const
{
    return _map.find(edge)->second;
}

//----------------------------------------------------------------------------

void MeshRefNormalToPoints::Rebuild()
{
    _norm.clear();

    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    _norm.resize(rPoints.size());

    const MeshFacetArray& rFacets = _rclMesh.GetFacets();
    for (const auto& rFacet : rFacets) {
        const MeshPoint& p0 = rPoints[rFacet._aulPoints[0]];
        const MeshPoint& p1 = rPoints[rFacet._aulPoints[1]];
        const MeshPoint& p2 = rPoints[rFacet._aulPoints[2]];
        float l2p01 = Base::DistanceP2(p0, p1);
        float l2p12 = Base::DistanceP2(p1, p2);
        float l2p20 = Base::DistanceP2(p2, p0);

        Base::Vector3f facenormal = _rclMesh.GetFacet(rFacet).GetNormal();
        _norm[rFacet._aulPoints[0]] += facenormal * (1.0f / (l2p01 * l2p20));
        _norm[rFacet._aulPoints[1]] += facenormal * (1.0f / (l2p12 * l2p01));
        _norm[rFacet._aulPoints[2]] += facenormal * (1.0f / (l2p20 * l2p12));
    }
    for (auto& it : _norm) {
        it.Normalize();
    }
}

const Base::Vector3f& MeshRefNormalToPoints::operator[](PointIndex pos) const
{
    return _norm[pos];
}
