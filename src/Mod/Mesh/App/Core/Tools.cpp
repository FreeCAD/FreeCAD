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
# include <algorithm>
#endif

#include "Tools.h"
#include "Iterator.h"


using namespace MeshCore;

MeshSearchNeighbours::MeshSearchNeighbours (const MeshKernel &rclM, float fSampleDistance)
: _rclMesh(rclM),
  _rclFAry(rclM.GetFacets()),
  _rclPAry(rclM.GetPoints()),
  _clPt2Fa(rclM),
  _fSampleDistance(fSampleDistance)
{
    MeshAlgorithm(_rclMesh).ResetFacetFlag(MeshFacet::MARKED);
    MeshAlgorithm(_rclMesh).ResetPointFlag(MeshPoint::MARKED);
}

void MeshSearchNeighbours::Reinit (float fSampleDistance)
{
    _fSampleDistance = fSampleDistance;
    MeshAlgorithm(_rclMesh).ResetFacetFlag(MeshFacet::MARKED);
    MeshAlgorithm(_rclMesh).ResetPointFlag(MeshPoint::MARKED);
}

unsigned long MeshSearchNeighbours::NeighboursFromFacet (unsigned long ulFacetIdx, float fDistance, unsigned long ulMinPoints, std::vector<Base::Vector3f> &raclResultPoints)
{
    bool bAddPoints = false;

    _fMaxDistanceP2 = fDistance * fDistance;
    _clCenter       = _rclMesh.GetFacet(ulFacetIdx).GetGravityPoint();

    unsigned long ulVisited = 1;
    std::vector<MeshFacetArray::_TConstIterator>  aclTestedFacet;

    _aclResult.clear();
    _aclOuter.clear();

    // add start facet
    bool bFound = CheckDistToFacet(_rclFAry[ulFacetIdx]);
    _rclFAry[ulFacetIdx].SetFlag(MeshFacet::MARKED);
    aclTestedFacet.push_back(_rclFAry.begin() + ulFacetIdx);

    if ((bFound == false) && (_aclResult.size() < ulMinPoints)) {
        bAddPoints = true;
        bFound = ExpandRadius(ulMinPoints);
    }

    int nCtExpandRadius = 0; 
    // search neighbours, add not marked facets, test distance, add outer points
    MeshFacetArray::_TConstIterator f_beg = _rclFAry.begin();
    while ((bFound == true) && (nCtExpandRadius < 10)) {
        bFound = false;

        std::set<unsigned long> aclTmp;
        aclTmp.swap(_aclOuter);
        for (std::set<unsigned long>::iterator pI = aclTmp.begin(); pI != aclTmp.end(); pI++) {
            const std::set<unsigned long> &rclISet = _clPt2Fa[*pI]; 
            // search all facets hanging on this point
            for (std::set<unsigned long>::const_iterator pJ = rclISet.begin(); pJ != rclISet.end(); pJ++) {
                const MeshFacet &rclF = f_beg[*pJ];

                if (rclF.IsFlag(MeshFacet::MARKED) == false) {
                    bool bLF = CheckDistToFacet(rclF);
                    bFound = bFound || bLF;
                    rclF.SetFlag(MeshFacet::MARKED);
                    aclTestedFacet.push_back(f_beg+*pJ);
                }
            }
            ulVisited += rclISet.size();
        }

        // too few points inside radius found -> expand radius
        if ((bFound == false) && (_aclResult.size() < ulMinPoints)) {
            nCtExpandRadius++;
            bAddPoints = true;
            bFound = ExpandRadius(ulMinPoints);
        }
        else
            nCtExpandRadius = 0;
    }

    // reset marked facets, points
    for (std::vector<MeshFacetArray::_TConstIterator>::iterator pF = aclTestedFacet.begin();
        pF != aclTestedFacet.end(); ++pF)
        (*pF)->ResetFlag(MeshFacet::MARKED);
    for (std::set<unsigned long>::iterator pR = _aclResult.begin(); pR != _aclResult.end(); ++pR)
        _rclPAry[*pR].ResetFlag(MeshPoint::MARKED);


    // copy points in result container
    raclResultPoints.resize(_aclResult.size());
    int i = 0;
    for (std::set<unsigned long>::iterator pI = _aclResult.begin(); pI != _aclResult.end(); pI++, i++)
        raclResultPoints[i] = _rclPAry[*pI];

    if (bAddPoints == true) {
        // sort points, remove points lying furthest from center
        std::sort(raclResultPoints.begin(), raclResultPoints.end(), CDistRad(_clCenter));
        raclResultPoints.erase(raclResultPoints.begin() + ulMinPoints, raclResultPoints.end());
    }

    return ulVisited;
}

void MeshSearchNeighbours::SampleAllFacets (void)
{
    if (_aclSampledFacets.size() == _rclMesh.CountFacets())
        return; // already sampled, do nothing

    _aclSampledFacets.resize(_rclMesh.CountFacets());
    MeshFacetIterator clFIter(_rclMesh);
    int i = 0;
    for (clFIter.Init(); clFIter.More(); clFIter.Next(), i++) {
        std::vector<Base::Vector3f> clPoints;
        clFIter->SubSample(_fSampleDistance, clPoints);
        _aclSampledFacets[i].resize(clPoints.size());
        std::copy(clPoints.begin(), clPoints.end(), _aclSampledFacets[i].begin());
    }
}

unsigned long MeshSearchNeighbours::NeighboursFromSampledFacets (unsigned long ulFacetIdx, float fDistance, std::vector<Base::Vector3f> &raclResultPoints)
{
    SampleAllFacets();

    _fMaxDistanceP2 = fDistance * fDistance;
    _clCenter       = _rclMesh.GetFacet(ulFacetIdx).GetGravityPoint();

    _akSphere.Center = Wm4::Vector3<float>(_clCenter.x, _clCenter.y, _clCenter.z);
    _akSphere.Radius = fDistance;

    unsigned long ulVisited = 1;
    std::vector<MeshFacetArray::_TConstIterator>  aclTestedFacet;

    _aclResult.clear();
    _aclOuter.clear();
    _aclPointsResult.clear();

    // add start facet
    bool bFound = AccumulateNeighbours(_rclFAry[ulFacetIdx], ulFacetIdx);
    _rclFAry[ulFacetIdx].SetFlag(MeshFacet::MARKED);

    // search neighbours, add not marked facets, test distance, add outer points
    MeshFacetArray::_TConstIterator f_beg = _rclFAry.begin();
    while (bFound == true) {
        bFound = false;

        std::set<unsigned long> aclTmp;
        aclTmp.swap(_aclOuter);
        for (std::set<unsigned long>::iterator pI = aclTmp.begin(); pI != aclTmp.end(); pI++) {
            const std::set<unsigned long> &rclISet = _clPt2Fa[*pI]; 
            // search all facets hanging on this point
            for (std::set<unsigned long>::const_iterator pJ = rclISet.begin(); pJ != rclISet.end(); pJ++) {
                const MeshFacet &rclF = f_beg[*pJ];

                if (rclF.IsFlag(MeshFacet::MARKED) == false) {
                    bool bLF = AccumulateNeighbours(rclF, *pJ);
                    bFound = bFound || bLF;
                    rclF.SetFlag(MeshFacet::MARKED);
                    aclTestedFacet.push_back(f_beg+*pJ);
                }
            }
            ulVisited += rclISet.size();
        }
    }

    // reset marked facets
    for (std::vector<MeshFacetArray::_TConstIterator>::iterator pF = aclTestedFacet.begin(); pF != aclTestedFacet.end(); pF++)
        (*pF)->ResetFlag(MeshFacet::MARKED);

    // copy points in result container
    raclResultPoints.resize(_aclPointsResult.size());
    std::copy(_aclPointsResult.begin(), _aclPointsResult.end(), raclResultPoints.begin());

    // facet points
    for (std::set<unsigned long>::iterator pI = _aclResult.begin(); pI != _aclResult.end(); pI++) {
        if (InnerPoint(_rclPAry[*pI]) == true)
            raclResultPoints.push_back(_rclPAry[*pI]);
    }

    return ulVisited;
}

bool MeshSearchNeighbours::AccumulateNeighbours (const MeshFacet &rclF, unsigned long ulFIdx)
{
    int  k = 0;

    for (int i = 0; i < 3; i++) {
        unsigned long ulPIdx = rclF._aulPoints[i];
        _aclOuter.insert(ulPIdx);
        _aclResult.insert(ulPIdx);

        if (Base::DistanceP2(_clCenter, _rclPAry[ulPIdx]) < _fMaxDistanceP2)
            k++;
    }

    bool bFound = false;
    if (k == 3) {  // add all sample points
        _aclPointsResult.insert(_aclPointsResult.end(), _aclSampledFacets[ulFIdx].begin(), _aclSampledFacets[ulFIdx].end());
        bFound = true;
    }
    else {  // add points inner radius
        bFound = TriangleCutsSphere(rclF);

        if (bFound == true) {
            std::vector<Base::Vector3f> &rclT = _aclSampledFacets[ulFIdx];
            std::vector<Base::Vector3f> clTmp;
            clTmp.reserve(rclT.size());
            for (std::vector<Base::Vector3f>::iterator pI = rclT.begin(); pI != rclT.end(); pI++) {
                if (InnerPoint(*pI) == true)
                    clTmp.push_back(*pI);
            }
            _aclPointsResult.insert(_aclPointsResult.end(), clTmp.begin(), clTmp.end());
        }
    }

    return bFound;
}

bool MeshSearchNeighbours::ExpandRadius (unsigned long ulMinPoints)
{
    // add facets from current level
    _aclResult.insert(_aclOuter.begin(), _aclOuter.end());
    for (std::set<unsigned long>::iterator pI = _aclOuter.begin(); pI != _aclOuter.end(); pI++)
        _rclPAry[*pI].SetFlag(MeshPoint::MARKED);

    if (_aclResult.size() < ulMinPoints) {
        _fMaxDistanceP2 *= float(ulMinPoints) / float(_aclResult.size());
        return true;
    }
    else
        return false;
}

unsigned long MeshSearchNeighbours::NeighboursFacetFromFacet (unsigned long ulFacetIdx, float fDistance, std::vector<Base::Vector3f> &raclResultPoints, std::vector<unsigned long> &raclResultFacets)
{
    std::set<unsigned long> aulFacetSet;

    _fMaxDistanceP2 = fDistance * fDistance;
    _clCenter       = _rclMesh.GetFacet(ulFacetIdx).GetGravityPoint();

    unsigned long            ulVisited = 1;
    std::vector<MeshFacetArray::_TConstIterator>  aclTestedFacet;

    _aclResult.clear();
    _aclOuter.clear();

    // add start facet
    bool bFound = CheckDistToFacet(_rclFAry[ulFacetIdx]);
    _rclFAry[ulFacetIdx].SetFlag(MeshFacet::MARKED);
    aclTestedFacet.push_back(_rclFAry.begin() + ulFacetIdx);

    aulFacetSet.insert(ulFacetIdx);

    // search neighbours, add not marked facets, test distance, add outer points
    MeshFacetArray::_TConstIterator f_beg = _rclFAry.begin();
    while (bFound == true) {
        bFound = false;

        std::set<unsigned long> aclTmp;
        aclTmp.swap(_aclOuter);
        for (std::set<unsigned long>::iterator pI = aclTmp.begin(); pI != aclTmp.end(); pI++) {
            const std::set<unsigned long> &rclISet = _clPt2Fa[*pI]; 
            // search all facets hanging on this point
            for (std::set<unsigned long>::const_iterator pJ = rclISet.begin(); pJ != rclISet.end(); pJ++) {
                const MeshFacet &rclF = f_beg[*pJ];

                for (int i = 0; i < 3; i++) {
                    if (Base::DistanceP2(_clCenter, _rclPAry[rclF._aulPoints[i]]) < _fMaxDistanceP2) {
                        aulFacetSet.insert(*pJ);
                        break;
                    }
                }

                if (rclF.IsFlag(MeshFacet::MARKED) == false) {
                    bool bLF = CheckDistToFacet(rclF);

                    bFound = bFound || bLF;
                    rclF.SetFlag(MeshFacet::MARKED);
                    aclTestedFacet.push_back(f_beg+*pJ);
                }
            }
            ulVisited += rclISet.size();
        }
    }

    // reset marked facets, points
    for (std::vector<MeshFacetArray::_TConstIterator>::iterator pF = aclTestedFacet.begin(); pF != aclTestedFacet.end(); pF++)
        (*pF)->ResetFlag(MeshFacet::MARKED);
    for (std::set<unsigned long>::iterator pR = _aclResult.begin(); pR != _aclResult.end(); pR++)
        _rclPAry[*pR].ResetFlag(MeshPoint::MARKED);

    // copy points in result container
    raclResultPoints.resize(_aclResult.size());
    int i = 0;
    for (std::set<unsigned long>::iterator pI = _aclResult.begin(); pI != _aclResult.end(); pI++, i++)
        raclResultPoints[i] = _rclPAry[*pI];

    // copy facets in result container
    raclResultFacets.insert(raclResultFacets.begin(), aulFacetSet.begin(), aulFacetSet.end());

    return ulVisited;
}
