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

#include "Tools.h"


using namespace MeshCore;

MeshSearchNeighbours::MeshSearchNeighbours(const MeshKernel& rclM, float fSampleDistance)
    : _rclMesh(rclM)
    , _rclFAry(rclM.GetFacets())
    , _rclPAry(rclM.GetPoints())
    , _clPt2Fa(rclM)
    , _fSampleDistance(fSampleDistance)
{
    MeshAlgorithm(_rclMesh).ResetFacetFlag(MeshFacet::MARKED);
    MeshAlgorithm(_rclMesh).ResetPointFlag(MeshPoint::MARKED);
}

void MeshSearchNeighbours::Reinit(float fSampleDistance)
{
    _fSampleDistance = fSampleDistance;
    MeshAlgorithm(_rclMesh).ResetFacetFlag(MeshFacet::MARKED);
    MeshAlgorithm(_rclMesh).ResetPointFlag(MeshPoint::MARKED);
}

unsigned long
MeshSearchNeighbours::NeighboursFromFacet(FacetIndex ulFacetIdx,
                                          float fDistance,
                                          unsigned long ulMinPoints,
                                          std::vector<Base::Vector3f>& raclResultPoints)
{
    bool bAddPoints = false;

    _fMaxDistanceP2 = fDistance * fDistance;
    _clCenter = _rclMesh.GetFacet(ulFacetIdx).GetGravityPoint();

    unsigned long ulVisited = 1;
    std::vector<MeshFacetArray::_TConstIterator> aclTestedFacet;

    _aclResult.clear();
    _aclOuter.clear();

    // add start facet
    bool bFound = CheckDistToFacet(_rclFAry[ulFacetIdx]);
    _rclFAry[ulFacetIdx].SetFlag(MeshFacet::MARKED);
    aclTestedFacet.push_back(_rclFAry.begin() + ulFacetIdx);

    if (!bFound && (_aclResult.size() < ulMinPoints)) {
        bAddPoints = true;
        bFound = ExpandRadius(ulMinPoints);
    }

    int nCtExpandRadius = 0;
    // search neighbours, add not marked facets, test distance, add outer points
    MeshFacetArray::_TConstIterator f_beg = _rclFAry.begin();
    while (bFound && (nCtExpandRadius < 10)) {
        bFound = false;

        std::set<PointIndex> aclTmp;
        aclTmp.swap(_aclOuter);
        for (PointIndex pI : aclTmp) {
            const std::set<FacetIndex>& rclISet = _clPt2Fa[pI];
            // search all facets hanging on this point
            for (FacetIndex pJ : rclISet) {
                const MeshFacet& rclF = f_beg[pJ];

                if (!rclF.IsFlag(MeshFacet::MARKED)) {
                    bool bLF = CheckDistToFacet(rclF);
                    bFound = bFound || bLF;
                    rclF.SetFlag(MeshFacet::MARKED);
                    aclTestedFacet.push_back(f_beg + pJ);
                }
            }
            ulVisited += rclISet.size();
        }

        // too few points inside radius found -> expand radius
        if (!bFound && (_aclResult.size() < ulMinPoints)) {
            nCtExpandRadius++;
            bAddPoints = true;
            bFound = ExpandRadius(ulMinPoints);
        }
        else {
            nCtExpandRadius = 0;
        }
    }

    // reset marked facets, points
    for (auto& pF : aclTestedFacet) {
        pF->ResetFlag(MeshFacet::MARKED);
    }
    for (PointIndex pR : _aclResult) {
        _rclPAry[pR].ResetFlag(MeshPoint::MARKED);
    }


    // copy points in result container
    raclResultPoints.resize(_aclResult.size());
    size_t i = 0;
    for (std::set<PointIndex>::iterator pI = _aclResult.begin(); pI != _aclResult.end();
         ++pI, i++) {
        raclResultPoints[i] = _rclPAry[*pI];
    }

    if (bAddPoints) {
        // sort points, remove points lying furthest from center
        std::sort(raclResultPoints.begin(), raclResultPoints.end(), CDistRad(_clCenter));
        raclResultPoints.erase(raclResultPoints.begin() + ulMinPoints, raclResultPoints.end());
    }

    return ulVisited;
}

void MeshSearchNeighbours::SampleAllFacets()
{
    if (_aclSampledFacets.size() == _rclMesh.CountFacets()) {
        return;  // already sampled, do nothing
    }

    _aclSampledFacets.resize(_rclMesh.CountFacets());
    MeshFacetIterator clFIter(_rclMesh);
    size_t i = 0;
    for (clFIter.Init(); clFIter.More(); clFIter.Next(), i++) {
        std::vector<Base::Vector3f> clPoints;
        clFIter->SubSample(_fSampleDistance, clPoints);
        _aclSampledFacets[i].resize(clPoints.size());
        std::copy(clPoints.begin(), clPoints.end(), _aclSampledFacets[i].begin());
    }
}

unsigned long
MeshSearchNeighbours::NeighboursFromSampledFacets(FacetIndex ulFacetIdx,
                                                  float fDistance,
                                                  std::vector<Base::Vector3f>& raclResultPoints)
{
    SampleAllFacets();

    _fMaxDistanceP2 = fDistance * fDistance;
    _clCenter = _rclMesh.GetFacet(ulFacetIdx).GetGravityPoint();

    _akSphere.Center = Wm4::Vector3<float>(_clCenter.x, _clCenter.y, _clCenter.z);
    _akSphere.Radius = fDistance;

    unsigned long ulVisited = 1;
    std::vector<MeshFacetArray::_TConstIterator> aclTestedFacet;

    _aclResult.clear();
    _aclOuter.clear();
    _aclPointsResult.clear();

    // add start facet
    bool bFound = AccumulateNeighbours(_rclFAry[ulFacetIdx], ulFacetIdx);
    _rclFAry[ulFacetIdx].SetFlag(MeshFacet::MARKED);

    // search neighbours, add not marked facets, test distance, add outer points
    MeshFacetArray::_TConstIterator f_beg = _rclFAry.begin();
    while (bFound) {
        bFound = false;

        std::set<PointIndex> aclTmp;
        aclTmp.swap(_aclOuter);
        for (PointIndex pI : aclTmp) {
            const std::set<FacetIndex>& rclISet = _clPt2Fa[pI];
            // search all facets hanging on this point
            for (FacetIndex pJ : rclISet) {
                const MeshFacet& rclF = f_beg[pJ];

                if (!rclF.IsFlag(MeshFacet::MARKED)) {
                    bool bLF = AccumulateNeighbours(rclF, pJ);
                    bFound = bFound || bLF;
                    rclF.SetFlag(MeshFacet::MARKED);
                    aclTestedFacet.push_back(f_beg + pJ);
                }
            }
            ulVisited += rclISet.size();
        }
    }

    // reset marked facets
    for (auto& pF : aclTestedFacet) {
        pF->ResetFlag(MeshFacet::MARKED);
    }

    // copy points in result container
    raclResultPoints.resize(_aclPointsResult.size());
    std::copy(_aclPointsResult.begin(), _aclPointsResult.end(), raclResultPoints.begin());

    // facet points
    for (PointIndex pI : _aclResult) {
        if (InnerPoint(_rclPAry[pI])) {
            raclResultPoints.push_back(_rclPAry[pI]);
        }
    }

    return ulVisited;
}

bool MeshSearchNeighbours::AccumulateNeighbours(const MeshFacet& rclF, FacetIndex ulFIdx)
{
    int k = 0;

    for (PointIndex ulPIdx : rclF._aulPoints) {
        _aclOuter.insert(ulPIdx);
        _aclResult.insert(ulPIdx);

        if (Base::DistanceP2(_clCenter, _rclPAry[ulPIdx]) < _fMaxDistanceP2) {
            k++;
        }
    }

    bool bFound = false;
    if (k == 3) {  // add all sample points
        _aclPointsResult.insert(_aclPointsResult.end(),
                                _aclSampledFacets[ulFIdx].begin(),
                                _aclSampledFacets[ulFIdx].end());
        bFound = true;
    }
    else {  // add points inner radius
        bFound = TriangleCutsSphere(rclF);

        if (bFound) {
            const std::vector<Base::Vector3f>& rclT = _aclSampledFacets[ulFIdx];
            std::vector<Base::Vector3f> clTmp;
            clTmp.reserve(rclT.size());
            for (const auto& pI : rclT) {
                if (InnerPoint(pI)) {
                    clTmp.push_back(pI);
                }
            }
            _aclPointsResult.insert(_aclPointsResult.end(), clTmp.begin(), clTmp.end());
        }
    }

    return bFound;
}

bool MeshSearchNeighbours::ExpandRadius(unsigned long ulMinPoints)
{
    // add facets from current level
    _aclResult.insert(_aclOuter.begin(), _aclOuter.end());
    for (PointIndex pI : _aclOuter) {
        _rclPAry[pI].SetFlag(MeshPoint::MARKED);
    }

    if (_aclResult.size() < ulMinPoints) {
        _fMaxDistanceP2 *= float(ulMinPoints) / float(_aclResult.size());
        return true;
    }
    else {
        return false;
    }
}

unsigned long
MeshSearchNeighbours::NeighboursFacetFromFacet(FacetIndex ulFacetIdx,
                                               float fDistance,
                                               std::vector<Base::Vector3f>& raclResultPoints,
                                               std::vector<FacetIndex>& raclResultFacets)
{
    std::set<FacetIndex> aulFacetSet;

    _fMaxDistanceP2 = fDistance * fDistance;
    _clCenter = _rclMesh.GetFacet(ulFacetIdx).GetGravityPoint();

    unsigned long ulVisited = 1;
    std::vector<MeshFacetArray::_TConstIterator> aclTestedFacet;

    _aclResult.clear();
    _aclOuter.clear();

    // add start facet
    bool bFound = CheckDistToFacet(_rclFAry[ulFacetIdx]);
    _rclFAry[ulFacetIdx].SetFlag(MeshFacet::MARKED);
    aclTestedFacet.push_back(_rclFAry.begin() + ulFacetIdx);

    aulFacetSet.insert(ulFacetIdx);

    // search neighbours, add not marked facets, test distance, add outer points
    MeshFacetArray::_TConstIterator f_beg = _rclFAry.begin();
    while (bFound) {
        bFound = false;

        std::set<PointIndex> aclTmp;
        aclTmp.swap(_aclOuter);
        for (PointIndex pI : aclTmp) {
            const std::set<FacetIndex>& rclISet = _clPt2Fa[pI];
            // search all facets hanging on this point
            for (FacetIndex pJ : rclISet) {
                const MeshFacet& rclF = f_beg[pJ];

                for (PointIndex ptIndex : rclF._aulPoints) {
                    if (Base::DistanceP2(_clCenter, _rclPAry[ptIndex]) < _fMaxDistanceP2) {
                        aulFacetSet.insert(pJ);
                        break;
                    }
                }

                if (!rclF.IsFlag(MeshFacet::MARKED)) {
                    bool bLF = CheckDistToFacet(rclF);

                    bFound = bFound || bLF;
                    rclF.SetFlag(MeshFacet::MARKED);
                    aclTestedFacet.push_back(f_beg + pJ);
                }
            }
            ulVisited += rclISet.size();
        }
    }

    // reset marked facets, points
    for (auto& pF : aclTestedFacet) {
        pF->ResetFlag(MeshFacet::MARKED);
    }
    for (PointIndex pR : _aclResult) {
        _rclPAry[pR].ResetFlag(MeshPoint::MARKED);
    }

    // copy points in result container
    raclResultPoints.resize(_aclResult.size());
    size_t i = 0;
    for (std::set<PointIndex>::iterator pI = _aclResult.begin(); pI != _aclResult.end();
         ++pI, i++) {
        raclResultPoints[i] = _rclPAry[*pI];
    }

    // copy facets in result container
    raclResultFacets.insert(raclResultFacets.begin(), aulFacetSet.begin(), aulFacetSet.end());

    return ulVisited;
}
