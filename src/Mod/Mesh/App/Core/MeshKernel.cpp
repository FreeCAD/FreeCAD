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
# include <stdexcept>
# include <map>
# include <queue>
#endif

#include <Base/Exception.h>
#include <Base/Sequencer.h>
#include <Base/Stream.h>
#include <Base/Swap.h>

#include "Algorithm.h"
#include "Approximation.h"
#include "Helpers.h"
#include "MeshKernel.h"
#include "Iterator.h"
#include "Evaluation.h"
#include "Builder.h"
#include "Smoothing.h"
#include "MeshIO.h"

using namespace MeshCore;

MeshKernel::MeshKernel (void)
: _bValid(true)
{
    _clBoundBox.SetVoid();
}

MeshKernel::MeshKernel (const MeshKernel &rclMesh)
{
    *this = rclMesh;
}

MeshKernel& MeshKernel::operator = (const MeshKernel &rclMesh)
{
    if (this != &rclMesh) { // must be a different instance
        this->_aclPointArray  = rclMesh._aclPointArray;
        this->_aclFacetArray  = rclMesh._aclFacetArray;
        this->_clBoundBox     = rclMesh._clBoundBox;
        this->_bValid         = rclMesh._bValid;
    }
    return *this;
}

MeshKernel& MeshKernel::operator = (const std::vector<MeshGeomFacet> &rclFAry)
{
    MeshBuilder builder(*this);
    builder.Initialize(rclFAry.size());

    for (std::vector<MeshGeomFacet>::const_iterator it = rclFAry.begin(); it != rclFAry.end(); ++it)
        builder.AddFacet(*it);

    builder.Finish();

    return *this;
}

void MeshKernel::Assign(const MeshPointArray& rPoints, const MeshFacetArray& rFacets, bool checkNeighbourHood)
{
    _aclPointArray = rPoints;
    _aclFacetArray = rFacets;
    RecalcBoundBox();
    if (checkNeighbourHood)
        RebuildNeighbours();
}

void MeshKernel::Adopt(MeshPointArray& rPoints, MeshFacetArray& rFacets, bool checkNeighbourHood)
{
    _aclPointArray.swap(rPoints);
    _aclFacetArray.swap(rFacets);
    RecalcBoundBox();
    if (checkNeighbourHood)
        RebuildNeighbours();
}

void MeshKernel::Swap(MeshKernel& mesh)
{
    this->_aclPointArray.swap(mesh._aclPointArray);
    this->_aclFacetArray.swap(mesh._aclFacetArray);
    this->_clBoundBox = mesh._clBoundBox;
}

MeshKernel& MeshKernel::operator += (const MeshGeomFacet &rclSFacet)
{
    this->AddFacet(rclSFacet);
    return *this;
}

void MeshKernel::AddFacet(const MeshGeomFacet &rclSFacet)
{
    unsigned long i;
    MeshFacet clFacet;

    // set corner points
    for (i = 0; i < 3; i++) {
        _clBoundBox.Add(rclSFacet._aclPoints[i]);
        clFacet._aulPoints[i] = _aclPointArray.GetOrAddIndex(rclSFacet._aclPoints[i]);
    }

    // adjust orientation to normal
    AdjustNormal(clFacet, rclSFacet.GetNormal());

    unsigned long ulCt = _aclFacetArray.size();

    // set neighbourhood
    unsigned long ulP0 = clFacet._aulPoints[0];
    unsigned long ulP1 = clFacet._aulPoints[1];
    unsigned long ulP2 = clFacet._aulPoints[2];
    unsigned long ulCC = 0;
    for (TMeshFacetArray::iterator pF = _aclFacetArray.begin(); pF != _aclFacetArray.end(); ++pF, ulCC++) {
        for (int i=0; i<3;i++) {
            unsigned long ulP = pF->_aulPoints[i];
            unsigned long ulQ = pF->_aulPoints[(i+1)%3];
            if (ulQ == ulP0 && ulP == ulP1) {
                clFacet._aulNeighbours[0] = ulCC;
                pF->_aulNeighbours[i] = ulCt;
            }
            else if (ulQ == ulP1 && ulP == ulP2) {
                clFacet._aulNeighbours[1] = ulCC;
                pF->_aulNeighbours[i] = ulCt;
            }
            else if (ulQ == ulP2 && ulP == ulP0) {
                clFacet._aulNeighbours[2] = ulCC;
                pF->_aulNeighbours[i] = ulCt;
            }
        }
    }

    // insert facet into array
    _aclFacetArray.push_back(clFacet);
}

MeshKernel& MeshKernel::operator += (const std::vector<MeshGeomFacet> &rclFAry)
{
    this->AddFacets(rclFAry);
    return *this;
}

void MeshKernel::AddFacets(const std::vector<MeshGeomFacet> &rclFAry)
{
    // Create a temp. kernel to get the topology of the passed triangles
    // and merge them with this kernel. This keeps properties and flags 
    // of this mesh.
    MeshKernel tmp;
    tmp = rclFAry;
    Merge(tmp);
}

unsigned long MeshKernel::AddFacets(const std::vector<MeshFacet> &rclFAry,
                                    bool checkManifolds)
{
    // Build map of edges of the referencing facets we want to append
#ifdef FC_DEBUG
    unsigned long countPoints = CountPoints();
#endif

    // if the manifold check shouldn't be done then just add all faces
    if (!checkManifolds) {
        unsigned long countFacets = CountFacets();
        unsigned long countValid = rclFAry.size();
        _aclFacetArray.reserve(countFacets + countValid);

        // just add all faces now
        for (std::vector<MeshFacet>::const_iterator pF = rclFAry.begin(); pF != rclFAry.end(); ++pF) {
            _aclFacetArray.push_back(*pF);
        }

        RebuildNeighbours(countFacets);
        return _aclFacetArray.size();
    }

    this->_aclPointArray.ResetInvalid();
    unsigned long k = CountFacets();
    std::map<std::pair<unsigned long, unsigned long>, std::list<unsigned long> > edgeMap;
    for (std::vector<MeshFacet>::const_iterator pF = rclFAry.begin(); pF != rclFAry.end(); ++pF, k++) {
        // reset INVALID flag for all candidates
        pF->ResetFlag(MeshFacet::INVALID);
        for (int i=0; i<3; i++) {
#ifdef FC_DEBUG
            assert( pF->_aulPoints[i] < countPoints );
#endif
            this->_aclPointArray[pF->_aulPoints[i]].SetFlag(MeshPoint::INVALID);
            unsigned long ulT0 = pF->_aulPoints[i];
            unsigned long ulT1 = pF->_aulPoints[(i+1)%3];
            unsigned long ulP0 = std::min<unsigned long>(ulT0, ulT1);
            unsigned long ulP1 = std::max<unsigned long>(ulT0, ulT1);
            edgeMap[std::make_pair(ulP0, ulP1)].push_front(k);
        }
    }

    // Check for the above edges in the current facet array
    k=0;
    for (MeshFacetArray::_TIterator pF = _aclFacetArray.begin(); pF != _aclFacetArray.end(); ++pF, k++) {
        // if none of the points references one of the edges ignore the facet
        if (!this->_aclPointArray[pF->_aulPoints[0]].IsFlag(MeshPoint::INVALID) &&
            !this->_aclPointArray[pF->_aulPoints[1]].IsFlag(MeshPoint::INVALID) &&
            !this->_aclPointArray[pF->_aulPoints[2]].IsFlag(MeshPoint::INVALID))
            continue;
        for (int i=0; i<3; i++) {
            unsigned long ulT0 = pF->_aulPoints[i];
            unsigned long ulT1 = pF->_aulPoints[(i+1)%3];
            unsigned long ulP0 = std::min<unsigned long>(ulT0, ulT1);
            unsigned long ulP1 = std::max<unsigned long>(ulT0, ulT1);
            std::pair<unsigned long, unsigned long> edge = std::make_pair(ulP0, ulP1);
            std::map<std::pair<unsigned long, unsigned long>, std::list<unsigned long> >::iterator pI = edgeMap.find(edge);
            // Does the current facet share the same edge?
            if (pI != edgeMap.end()) {
                pI->second.push_front(k);
            }
        }
    }

    this->_aclPointArray.ResetInvalid();

    // Now let's see for which edges we might get manifolds, if so we don't add the corresponding candidates
    unsigned long countFacets = CountFacets();
    std::map<std::pair<unsigned long, unsigned long>, std::list<unsigned long> >::iterator pE;
    for (pE = edgeMap.begin(); pE != edgeMap.end(); ++pE)
    {
        if (pE->second.size() > 2)
        {
            for (std::list<unsigned long>::iterator it = pE->second.begin(); it != pE->second.end(); ++it)
            {
                if (*it >= countFacets) {
                    // this is a candidate
                    unsigned long index = *it - countFacets;
                    rclFAry[index].SetFlag(MeshFacet::INVALID);
                }
            }
        }
    }

    // Do not insert directly to the data structure because we should get the correct size of new
    // facets, otherwise std::vector reallocates too much memory which can't be freed so easily
    unsigned long countValid = std::count_if(rclFAry.begin(), rclFAry.end(),
        std::bind2nd(MeshIsNotFlag<MeshFacet>(), MeshFacet::INVALID));
    _aclFacetArray.reserve( _aclFacetArray.size() + countValid );
    // now start inserting the facets to the data structure and set the correct neighbourhood as well
    unsigned long startIndex = CountFacets();
    for (std::vector<MeshFacet>::const_iterator pF = rclFAry.begin(); pF != rclFAry.end(); ++pF) {
        if (!pF->IsFlag(MeshFacet::INVALID)) {
            _aclFacetArray.push_back(*pF);
            pF->SetProperty(startIndex++);
        }
    }

    // resolve neighbours
    for (pE = edgeMap.begin(); pE != edgeMap.end(); ++pE)
    {
        unsigned long ulP0 = pE->first.first;
        unsigned long ulP1 = pE->first.second;
        if (pE->second.size() == 1)  // border facet
        {
            unsigned long ulF0 = pE->second.front();
            if (ulF0 >= countFacets) {
                ulF0 -= countFacets;
                std::vector<MeshFacet>::const_iterator pF = rclFAry.begin() + ulF0;
                if (!pF->IsFlag(MeshFacet::INVALID))
                    ulF0 = pF->_ulProp;
                else
                    ulF0 = ULONG_MAX;
            }

            if (ulF0 != ULONG_MAX) {
                unsigned short usSide =  _aclFacetArray[ulF0].Side(ulP0, ulP1);
                assert(usSide != USHRT_MAX);
                _aclFacetArray[ulF0]._aulNeighbours[usSide] = ULONG_MAX;
            }
        }
        else if (pE->second.size() == 2)  // normal facet with neighbour
        {
            // we must check if both facets are part of the mesh now 
            unsigned long ulF0 = pE->second.front();
            if (ulF0 >= countFacets) {
                ulF0 -= countFacets;
                std::vector<MeshFacet>::const_iterator pF = rclFAry.begin() + ulF0;
                if (!pF->IsFlag(MeshFacet::INVALID))
                    ulF0 = pF->_ulProp;
                else
                    ulF0 = ULONG_MAX;
            }
            unsigned long ulF1 = pE->second.back();
            if (ulF1 >= countFacets) {
                ulF1 -= countFacets;
                std::vector<MeshFacet>::const_iterator pF = rclFAry.begin() + ulF1;
                if (!pF->IsFlag(MeshFacet::INVALID))
                    ulF1 = pF->_ulProp;
                else
                    ulF1 = ULONG_MAX;
            }
            
            if (ulF0 != ULONG_MAX) {
                unsigned short usSide = _aclFacetArray[ulF0].Side(ulP0, ulP1);
                assert(usSide != USHRT_MAX);
                _aclFacetArray[ulF0]._aulNeighbours[usSide] = ulF1;
            }

            if (ulF1 != ULONG_MAX) {
                unsigned short usSide = _aclFacetArray[ulF1].Side(ulP0, ulP1);
                assert(usSide != USHRT_MAX);
                _aclFacetArray[ulF1]._aulNeighbours[usSide] = ulF0;
            }
        }
    }

    return _aclFacetArray.size();
}

unsigned long MeshKernel::AddFacets(const std::vector<MeshFacet> &rclFAry,
                                    const std::vector<Base::Vector3f>& rclPAry,
                                    bool checkManifolds)
{
    for (std::vector<Base::Vector3f>::const_iterator it = rclPAry.begin(); it != rclPAry.end(); ++it)
        _clBoundBox.Add(*it);
    this->_aclPointArray.insert(this->_aclPointArray.end(), rclPAry.begin(), rclPAry.end());
    return this->AddFacets(rclFAry, checkManifolds);
}

void MeshKernel::Merge(const MeshKernel& rKernel)
{
    if (this != &rKernel) {
        const MeshPointArray& rPoints = rKernel._aclPointArray;
        const MeshFacetArray& rFacets  = rKernel._aclFacetArray;
        Merge(rPoints, rFacets);
    }
}

void MeshKernel::Merge(const MeshPointArray& rPoints, const MeshFacetArray& rFaces)
{
    if (rPoints.empty() || rFaces.empty())
        return; // nothing to do
    std::vector<unsigned long> increments(rPoints.size());

    unsigned long countFacets = this->_aclFacetArray.size();
    // Reserve the additional memory to append the new facets
    this->_aclFacetArray.reserve(this->_aclFacetArray.size() + rFaces.size());

    // Copy the new faces immediately to the facet array
    MeshFacet face;
    for(MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it) {
        face = *it;
        for (int i=0; i<3; i++) {
            increments[it->_aulPoints[i]]++;
        }

        // append to the facet array
        this->_aclFacetArray.push_back(face);
    }

    unsigned long countNewPoints = std::count_if(increments.begin(), increments.end(),
                                   std::bind2nd(std::greater<unsigned long>(), 0));
    // Reserve the additional memory to append the new points
    unsigned long index = this->_aclPointArray.size();
    this->_aclPointArray.reserve(this->_aclPointArray.size() + countNewPoints);
    
    // Now we can start inserting the points and adjust the point indices of the faces
    for (std::vector<unsigned long>::iterator it = increments.begin(); it != increments.end(); ++it) {
        if (*it > 0) {
            // set the index of the point array
            *it = index++;
            const MeshPoint& rPt = rPoints[it-increments.begin()];
            this->_aclPointArray.push_back(rPt);
            _clBoundBox.Add(rPt);
        }
    }

    for (MeshFacetArray::_TIterator pF = this->_aclFacetArray.begin()+countFacets;
        pF != this->_aclFacetArray.end(); ++pF) {
        for (int i=0; i<3; i++) {
            pF->_aulPoints[i] = increments[pF->_aulPoints[i]];
        }
    }

    // Since rFaces could consist of a subset of the actual facet array the
    // neighbour indices could be totally wrong so they must be rebuilt from
    // scratch. Fortunately, this needs only to be done for the newly inserted
    // facets -- not for all
    RebuildNeighbours(countFacets);
}

void MeshKernel::Cleanup()
{
    MeshCleanup meshCleanup(_aclPointArray, _aclFacetArray);
    meshCleanup.RemoveInvalids();
}

void MeshKernel::Clear (void)
{
    _aclPointArray.clear();
    _aclFacetArray.clear();

    // release memory
    MeshPointArray().swap(_aclPointArray);
    MeshFacetArray().swap(_aclFacetArray);

    _clBoundBox.SetVoid();
}

bool MeshKernel::DeleteFacet (const MeshFacetIterator &rclIter)
{
    unsigned long i, j, ulNFacet, ulInd;

    if (rclIter._clIter >= _aclFacetArray.end())
        return false;

    // index of the facet to delete
    ulInd = rclIter._clIter - _aclFacetArray.begin(); 

    // invalidate neighbour indices of the neighbour facet to this facet
    for (i = 0; i < 3; i++) {
        ulNFacet = rclIter._clIter->_aulNeighbours[i];
        if (ulNFacet != ULONG_MAX) {
            for (j = 0; j < 3; j++) {
                if (_aclFacetArray[ulNFacet]._aulNeighbours[j] == ulInd) {
                    _aclFacetArray[ulNFacet]._aulNeighbours[j] = ULONG_MAX;
                    break;
                }
            }
        }
    }

    // erase corner point if needed
    for (i = 0; i < 3; i++) {
        if ((rclIter._clIter->_aulNeighbours[i] == ULONG_MAX) &&
            (rclIter._clIter->_aulNeighbours[(i+1)%3] == ULONG_MAX)) {
            // no neighbours, possibly delete point
            ErasePoint(rclIter._clIter->_aulPoints[(i+1)%3], ulInd);
        }
    }

    // remove facet from array
    _aclFacetArray.Erase(_aclFacetArray.begin() + rclIter.Position());

    return true;
}

bool MeshKernel::DeleteFacet (unsigned long ulInd)
{
    if (ulInd >= _aclFacetArray.size())
        return false;

    MeshFacetIterator clIter(*this);
    clIter.Set(ulInd);

    return DeleteFacet(clIter);
}

void MeshKernel::DeleteFacets (const std::vector<unsigned long> &raulFacets)
{
    _aclPointArray.SetProperty(0);

    // number of referencing facets per point
    for (MeshFacetArray::_TConstIterator pF = _aclFacetArray.begin(); pF != _aclFacetArray.end(); ++pF) {
        _aclPointArray[pF->_aulPoints[0]]._ulProp++;
        _aclPointArray[pF->_aulPoints[1]]._ulProp++;
        _aclPointArray[pF->_aulPoints[2]]._ulProp++;
    }

    // invalidate facet and adjust number of point references
    _aclFacetArray.ResetInvalid();
    for (std::vector<unsigned long>::const_iterator pI = raulFacets.begin(); pI != raulFacets.end(); ++pI) {
        MeshFacet &rclFacet = _aclFacetArray[*pI];
        rclFacet.SetInvalid();
        _aclPointArray[rclFacet._aulPoints[0]]._ulProp--;
        _aclPointArray[rclFacet._aulPoints[1]]._ulProp--;
        _aclPointArray[rclFacet._aulPoints[2]]._ulProp--;
    }

    // invalidate all unreferenced points
    _aclPointArray.ResetInvalid();
    for (MeshPointArray::_TIterator pP = _aclPointArray.begin(); pP != _aclPointArray.end(); ++pP) {
        if (pP->_ulProp == 0)
            pP->SetInvalid();
    }

    RemoveInvalids();
    RecalcBoundBox();
}

bool MeshKernel::DeletePoint (unsigned long ulInd)
{
    if (ulInd >= _aclPointArray.size())
        return false;

    MeshPointIterator clIter(*this);
    clIter.Set(ulInd);

    return DeletePoint(clIter);
}

bool MeshKernel::DeletePoint (const MeshPointIterator &rclIter)
{
    MeshFacetIterator pFIter(*this), pFEnd(*this);
    std::vector<MeshFacetIterator>  clToDel; 
    unsigned long i, ulInd;

    // index of the point to delete
    ulInd = rclIter._clIter - _aclPointArray.begin(); 
 
    pFIter.Begin();
    pFEnd.End();

    // check corner points of all facets
    while (pFIter < pFEnd) {
        for (i = 0; i < 3; i++) {
            if (ulInd == pFIter._clIter->_aulPoints[i])
                clToDel.push_back(pFIter);
        }
        ++pFIter;
    }

    // iterators (facets) sort by index
    std::sort(clToDel.begin(), clToDel.end());

    // delete each facet separately (from back to front to avoid to
    // invalidate the iterators)
    for (i = clToDel.size(); i > 0; i--)
        DeleteFacet(clToDel[i-1]); 
    return true;
}

void MeshKernel::DeletePoints (const std::vector<unsigned long> &raulPoints)
{
    _aclPointArray.ResetInvalid();
    for (std::vector<unsigned long>::const_iterator pI = raulPoints.begin(); pI != raulPoints.end(); ++pI)
        _aclPointArray[*pI].SetInvalid();

    // delete facets if at least one corner point is invalid
    _aclPointArray.SetProperty(0);
    for (MeshFacetArray::_TIterator pF = _aclFacetArray.begin(); pF != _aclFacetArray.end(); ++pF) {
        MeshPoint &rclP0 = _aclPointArray[pF->_aulPoints[0]];
        MeshPoint &rclP1 = _aclPointArray[pF->_aulPoints[1]];
        MeshPoint &rclP2 = _aclPointArray[pF->_aulPoints[2]];

        if ((rclP0.IsValid() == false) || (rclP1.IsValid() == false) || (rclP2.IsValid() == false)) {
            pF->SetInvalid();
        }
        else {
            pF->ResetInvalid();
            rclP0._ulProp++;
            rclP1._ulProp++;
            rclP2._ulProp++;
        }
    }

    // invalidate all unreferenced points to delete them
    for (MeshPointArray::_TIterator pP = _aclPointArray.begin(); pP != _aclPointArray.end(); ++pP) {
        if (pP->_ulProp == 0)
            pP->SetInvalid();
    }

    RemoveInvalids();
    RecalcBoundBox();
}

void MeshKernel::ErasePoint (unsigned long ulIndex, unsigned long ulFacetIndex, bool bOnlySetInvalid)
{
    unsigned long  i;
    std::vector<MeshFacet>::iterator pFIter, pFEnd, pFNot;

    pFIter = _aclFacetArray.begin();
    pFNot  = _aclFacetArray.begin() + ulFacetIndex;
    pFEnd  = _aclFacetArray.end();
 
    // check all facets
    while (pFIter < pFNot) {
        for (i = 0; i < 3; i++) {
            if (pFIter->_aulPoints[i] == ulIndex)
                return; // point still referenced ==> do not delete
        }
        ++pFIter;
    }

    ++pFIter;
    while (pFIter < pFEnd) {
        for (i = 0; i < 3; i++) {
            if (pFIter->_aulPoints[i] == ulIndex)
                return; // point still referenced ==> do not delete
        }
        ++pFIter;
    }


    if (bOnlySetInvalid == false) {
        // completely remove point
        _aclPointArray.erase(_aclPointArray.begin() + ulIndex);

        // correct point indices of the facets
        pFIter = _aclFacetArray.begin();
        while (pFIter < pFEnd) {
            for (i = 0; i < 3; i++) {
                if (pFIter->_aulPoints[i] > ulIndex)
                    pFIter->_aulPoints[i]--;
            }
            ++pFIter;
        }
    }
    else // only invalidate
        _aclPointArray[ulIndex].SetInvalid();
}

void MeshKernel::RemoveInvalids ()
{
    std::vector<unsigned long> aulDecrements;
    std::vector<unsigned long>::iterator pDIter;
    unsigned long ulDec, i, k;
    MeshPointArray::_TIterator pPIter, pPEnd;
    MeshFacetArray::_TIterator pFIter, pFEnd;

    // generate array of decrements
    aulDecrements.resize(_aclPointArray.size());
    pDIter = aulDecrements.begin();
    ulDec  = 0;
    pPEnd  = _aclPointArray.end();
    for (pPIter = _aclPointArray.begin(); pPIter != pPEnd; ++pPIter) {
        *pDIter++ = ulDec;
        if (pPIter->IsValid() == false)
            ulDec++;
    }

    // correct point indices of the facets
    pFEnd  = _aclFacetArray.end();
    for (pFIter = _aclFacetArray.begin(); pFIter != pFEnd; ++pFIter) {
        if (pFIter->IsValid() == true) {
            pFIter->_aulPoints[0] -= aulDecrements[pFIter->_aulPoints[0]];
            pFIter->_aulPoints[1] -= aulDecrements[pFIter->_aulPoints[1]];
            pFIter->_aulPoints[2] -= aulDecrements[pFIter->_aulPoints[2]];
        }
    }

    // delete point, number of valid points
    unsigned long ulNewPts = std::count_if(_aclPointArray.begin(), _aclPointArray.end(),
        std::mem_fun_ref(&MeshPoint::IsValid));
    // tmp. point array
    MeshPointArray  aclTempPt(ulNewPts);
    MeshPointArray::_TIterator pPTemp = aclTempPt.begin();
    pPEnd = _aclPointArray.end();
    for (pPIter = _aclPointArray.begin(); pPIter != pPEnd; ++pPIter) {
        if (pPIter->IsValid() == true)
            *pPTemp++ = *pPIter;
    }

    // free memory
    //_aclPointArray = aclTempPt;
    //aclTempPt.clear();
    _aclPointArray.swap(aclTempPt);
    MeshPointArray().swap(aclTempPt);

    // generate array of facet decrements
    aulDecrements.resize(_aclFacetArray.size());
    pDIter = aulDecrements.begin();
    ulDec  = 0;
    pFEnd  = _aclFacetArray.end();
    for (pFIter = _aclFacetArray.begin(); pFIter != pFEnd; ++pFIter, ++pDIter) {
        *pDIter = ulDec;
        if (pFIter->IsValid() == false)
            ulDec++;
    }

    // correct neighbour indices of the facets
    pFEnd = _aclFacetArray.end();
    for (pFIter = _aclFacetArray.begin(); pFIter != pFEnd; ++pFIter) {
        if (pFIter->IsValid() == true) {
            for (i = 0; i < 3; i++) {
                k = pFIter->_aulNeighbours[i];
                if (k != ULONG_MAX) {
                    if (_aclFacetArray[k].IsValid() == true)
                        pFIter->_aulNeighbours[i] -= aulDecrements[k];
                    else
                        pFIter->_aulNeighbours[i] = ULONG_MAX;
                }
            }
        }
    }

    // delete facets, number of valid facets
    unsigned long ulDelFacets = std::count_if(_aclFacetArray.begin(), _aclFacetArray.end(),
        std::mem_fun_ref(&MeshFacet::IsValid));
    MeshFacetArray aclFArray(ulDelFacets);
    MeshFacetArray::_TIterator pFTemp = aclFArray.begin();  
    pFEnd  = _aclFacetArray.end();
    for (pFIter = _aclFacetArray.begin(); pFIter != pFEnd; ++pFIter) {
        if (pFIter->IsValid() == true)
            *pFTemp++ = *pFIter;
    }

    // free memory
    //_aclFacetArray = aclFArray;
    _aclFacetArray.swap(aclFArray);
}

void MeshKernel::CutFacets(const MeshFacetGrid& rclGrid, const Base::ViewProjMethod* pclProj, 
                           const Base::Polygon2d& rclPoly, bool bCutInner, std::vector<MeshGeomFacet> &raclFacets)
{
    std::vector<unsigned long> aulFacets;

    MeshAlgorithm(*this).CheckFacets(rclGrid, pclProj, rclPoly, bCutInner, aulFacets );

    for (std::vector<unsigned long>::iterator i = aulFacets.begin(); i != aulFacets.end(); ++i)
        raclFacets.push_back(GetFacet(*i));

    DeleteFacets(aulFacets);
}

void MeshKernel::CutFacets(const MeshFacetGrid& rclGrid, const Base::ViewProjMethod* pclProj,
                           const Base::Polygon2d& rclPoly, bool bInner, std::vector<unsigned long> &raclCutted)
{
    MeshAlgorithm(*this).CheckFacets(rclGrid, pclProj, rclPoly, bInner, raclCutted);
    DeleteFacets(raclCutted);
}

std::vector<unsigned long> MeshKernel::GetFacetPoints(const std::vector<unsigned long>& facets) const
{
    std::vector<unsigned long> points;
    for (std::vector<unsigned long>::const_iterator it = facets.begin(); it != facets.end(); ++it) {
        unsigned long p0, p1, p2;
        GetFacetPoints(*it, p0, p1, p2);
        points.push_back(p0);
        points.push_back(p1);
        points.push_back(p2);
  }

    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    return points;
}

std::vector<unsigned long> MeshKernel::HasFacets (const MeshPointIterator &rclIter) const
{
    unsigned long i, ulPtInd = rclIter.Position();
    std::vector<MeshFacet>::const_iterator  pFIter = _aclFacetArray.begin();
    std::vector<MeshFacet>::const_iterator  pFBegin = _aclFacetArray.begin();
    std::vector<MeshFacet>::const_iterator  pFEnd = _aclFacetArray.end();
    std::vector<unsigned long> aulBelongs;

      while (pFIter < pFEnd) {
        for (i = 0; i < 3; i++) {
            if (pFIter->_aulPoints[i] == ulPtInd) {
                aulBelongs.push_back(pFIter - pFBegin);
                break;
            }
        }
        ++pFIter;
    }

    return aulBelongs;
}

MeshPointArray MeshKernel::GetPoints(const std::vector<unsigned long>& indices) const
{
    MeshPointArray ary;
    ary.reserve(indices.size());
    for (std::vector<unsigned long>::const_iterator it = indices.begin(); it != indices.end(); ++it)
        ary.push_back(this->_aclPointArray[*it]);
    return ary;
}

MeshFacetArray MeshKernel::GetFacets(const std::vector<unsigned long>& indices) const
{
    MeshFacetArray ary;
    ary.reserve(indices.size());
    for (std::vector<unsigned long>::const_iterator it = indices.begin(); it != indices.end(); ++it)
        ary.push_back(this->_aclFacetArray[*it]);
    return ary;
}

void MeshKernel::Write (std::ostream &rclOut) const 
{
    if (!rclOut || rclOut.bad())
        return;

    Base::OutputStream str(rclOut);

    // Write a header with a "magic number" and a version
    str << (uint32_t)0xA0B0C0D0;
    str << (uint32_t)0x010000;

    char szInfo[257]; // needs an additional byte for zero-termination
    strcpy(szInfo, "MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-"
                   "MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-"
                   "MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-MESH-"
                   "MESH-MESH-MESH-\n");
    rclOut.write(szInfo, 256);

    // write the number of points and facets
    str << (uint32_t)CountPoints() << (uint32_t)CountFacets();

    // write the data
    for (MeshPointArray::_TConstIterator it = _aclPointArray.begin(); it != _aclPointArray.end(); ++it) {
        str << it->x << it->y << it->z;
    }

    for (MeshFacetArray::_TConstIterator it = _aclFacetArray.begin(); it != _aclFacetArray.end(); ++it) {
        str << (uint32_t)it->_aulPoints[0] 
            << (uint32_t)it->_aulPoints[1] 
            << (uint32_t)it->_aulPoints[2];
        str << (uint32_t)it->_aulNeighbours[0] 
            << (uint32_t)it->_aulNeighbours[1] 
            << (uint32_t)it->_aulNeighbours[2];
    }

    str << _clBoundBox.MinX << _clBoundBox.MaxX;
    str << _clBoundBox.MinY << _clBoundBox.MaxY;
    str << _clBoundBox.MinZ << _clBoundBox.MaxZ;
}

void MeshKernel::Read (std::istream &rclIn)
{
    if (!rclIn || rclIn.bad())
        return;

    // get header
    Base::InputStream str(rclIn);

    // Read the header with a "magic number" and a version
    uint32_t magic, version, swap_magic, swap_version;
    str >> magic >> version;
    swap_magic = magic; Base::SwapEndian(swap_magic);
    swap_version = version; Base::SwapEndian(swap_version);
    uint32_t open_edge = 0xffffffff; // value to mark an open edge

    // is it the new or old format?
    bool new_format = false;
    if (magic == 0xA0B0C0D0 && version == 0x010000) {
        new_format = true;
    }
    else if (swap_magic == 0xA0B0C0D0 && swap_version == 0x010000) {
        new_format = true;
        str.setByteOrder(Base::Stream::BigEndian);
    }

    if (new_format) {
        char szInfo[256];
        rclIn.read(szInfo, 256);

        // read the number of points and facets
        uint32_t uCtPts=0, uCtFts=0;
        str >> uCtPts >> uCtFts;

        try {
            // read the data
            MeshPointArray pointArray;
            pointArray.resize(uCtPts);
            for (MeshPointArray::_TIterator it = pointArray.begin(); it != pointArray.end(); ++it) {
                str >> it->x >> it->y >> it->z;
            }
          
            MeshFacetArray facetArray;
            facetArray.resize(uCtFts);

            uint32_t v1, v2, v3;
            for (MeshFacetArray::_TIterator it = facetArray.begin(); it != facetArray.end(); ++it) {
                str >> v1 >> v2 >> v3;

                // make sure to have valid indices
                if (v1 >= uCtPts || v2 >= uCtPts || v3 >= uCtPts)
                    throw Base::BadFormatError("Invalid data structure");

                it->_aulPoints[0] = v1;
                it->_aulPoints[1] = v2;
                it->_aulPoints[2] = v3;

                // On systems where an 'unsigned long' is a 64-bit value
                // the empty neighbour must be explicitly set to 'ULONG_MAX'
                // because in algorithms this value is always used to check
                // for open edges.
                str >> v1 >> v2 >> v3;

                // make sure to have valid indices
                if (v1 >= uCtFts && v1 < open_edge)
                    throw Base::BadFormatError("Invalid data structure");
                if (v2 >= uCtFts && v2 < open_edge)
                    throw Base::BadFormatError("Invalid data structure");
                if (v3 >= uCtFts && v3 < open_edge)
                    throw Base::BadFormatError("Invalid data structure");

                if (v1 < open_edge)
                    it->_aulNeighbours[0] = v1;
                else
                    it->_aulNeighbours[0] = ULONG_MAX;

                if (v2 < open_edge)
                    it->_aulNeighbours[1] = v2;
                else
                    it->_aulNeighbours[1] = ULONG_MAX;

                if (v3 < open_edge)
                    it->_aulNeighbours[2] = v3;
                else
                    it->_aulNeighbours[2] = ULONG_MAX;
            }

            str >> _clBoundBox.MinX >> _clBoundBox.MaxX;
            str >> _clBoundBox.MinY >> _clBoundBox.MaxY;
            str >> _clBoundBox.MinZ >> _clBoundBox.MaxZ;

            // If we reach this block no exception occurred and we can safely assign the mesh
            _aclPointArray.swap(pointArray);
            _aclFacetArray.swap(facetArray);
        }
        catch (std::exception&) {
            // Special handling of std::length_error
            throw Base::BadFormatError("Reading from stream failed");
        }
    }
    else {
        // The old formats
        unsigned long uCtPts=magic, uCtFts=version;
        MeshPointArray pointArray;
        MeshFacetArray facetArray;

        float ratio = 0;
        if (uCtPts > 0) {
            ratio = static_cast<float>(uCtFts) / static_cast<float>(uCtPts);
        }

        // without edge array
        if (ratio < 2.5) {
            // the stored mesh kernel might be empty
            if (uCtPts > 0) {
                pointArray.resize(uCtPts);
                rclIn.read((char*)&(pointArray[0]), uCtPts*sizeof(MeshPoint));
            }
            if (uCtFts > 0) {
                facetArray.resize(uCtFts);
                rclIn.read((char*)&(facetArray[0]), uCtFts*sizeof(MeshFacet));
            }
            rclIn.read((char*)&_clBoundBox, sizeof(Base::BoundBox3f));
        }
        else {
            // with edge array
            unsigned long uCtEdges=uCtFts;
            str >> magic;
            uCtFts = magic;
            pointArray.resize(uCtPts);
            for (MeshPointArray::_TIterator it = pointArray.begin(); it != pointArray.end(); ++it) {
                str >> it->x >> it->y >> it->z;
            }
            uint32_t dummy;
            for (unsigned long i=0; i<uCtEdges; i++) {
                str >> dummy;
            }
            uint32_t v1, v2, v3;
            facetArray.resize(uCtFts);
            for (MeshFacetArray::_TIterator it = facetArray.begin(); it != facetArray.end(); ++it) {
                str >> v1 >> v2 >> v3;
                it->_aulNeighbours[0] = v1;
                it->_aulNeighbours[1] = v2;
                it->_aulNeighbours[2] = v3;
                str >> v1 >> v2 >> v3;
                it->_aulPoints[0] = v1;
                it->_aulPoints[1] = v2;
                it->_aulPoints[2] = v3;
                str >> it->_ucFlag;
            }

            str >> _clBoundBox.MinX
                >> _clBoundBox.MinY
                >> _clBoundBox.MinZ
                >> _clBoundBox.MaxX
                >> _clBoundBox.MaxY
                >> _clBoundBox.MaxZ;
        }

        for (auto it = facetArray.begin(); it != facetArray.end(); ++it) {
            for (int i=0; i<3; i++) {
                if (it->_aulPoints[i] >= uCtPts)
                    throw Base::BadFormatError("Invalid data structure");
                if (it->_aulNeighbours[i] < ULONG_MAX && it->_aulNeighbours[i] >= uCtFts)
                    throw Base::BadFormatError("Invalid data structure");
            }
        }

        _aclPointArray.swap(pointArray);
        _aclFacetArray.swap(facetArray);
    }
}

void MeshKernel::operator *= (const Base::Matrix4D &rclMat)
{
    this->Transform(rclMat);
}

void MeshKernel::Transform (const Base::Matrix4D &rclMat)
{
    MeshPointArray::_TIterator  clPIter = _aclPointArray.begin(), clPEIter = _aclPointArray.end();
    Base::Matrix4D clMatrix(rclMat);

    _clBoundBox.SetVoid();
    while (clPIter < clPEIter) {
        *clPIter *= clMatrix;
        _clBoundBox.Add(*clPIter);
        clPIter++;
    }
}

void MeshKernel::Smooth(int iterations, float stepsize)
{
    (void)stepsize;
    LaplaceSmoothing(*this).Smooth(iterations);
}

void MeshKernel::RecalcBoundBox (void)
{
    _clBoundBox.SetVoid();
    for (MeshPointArray::_TConstIterator pI = _aclPointArray.begin(); pI != _aclPointArray.end(); pI++)
        _clBoundBox.Add(*pI);
}

std::vector<Base::Vector3f> MeshKernel::CalcVertexNormals() const
{
    std::vector<Base::Vector3f> normals;

    normals.resize(CountPoints());

    unsigned long p1,p2,p3;
    unsigned int ct = CountFacets();
    for (unsigned int pFIter = 0;pFIter < ct; pFIter++) {
        GetFacetPoints(pFIter,p1,p2,p3);
        Base::Vector3f Norm = (GetPoint(p2)-GetPoint(p1)) % (GetPoint(p3)-GetPoint(p1));

        normals[p1] += Norm;
        normals[p2] += Norm;
        normals[p3] += Norm;
    }

    return normals;
}

// Evaluation
float MeshKernel::GetSurface() const
{
    float fSurface = 0.0;
    MeshFacetIterator cIter(*this);
    for (cIter.Init(); cIter.More(); cIter.Next())
        fSurface += cIter->Area();

    return fSurface;
}

float MeshKernel::GetSurface( const std::vector<unsigned long>& aSegment ) const
{
    float fSurface = 0.0;
    MeshFacetIterator cIter(*this);

    for (std::vector<unsigned long>::const_iterator it = aSegment.begin(); it != aSegment.end(); ++it) {
        cIter.Set(*it);
        fSurface += cIter->Area();
    }

    return fSurface;
}

float MeshKernel::GetVolume() const
{
    //MeshEvalSolid cSolid(*this);
    //if ( !cSolid.Evaluate() )
    //    return 0.0f; // no solid

    float fVolume = 0.0;
    MeshFacetIterator cIter(*this);
    Base::Vector3f p1,p2,p3;
    for (cIter.Init(); cIter.More(); cIter.Next()) {
        const MeshGeomFacet& rclF = *cIter;
        p1 = rclF._aclPoints[0];
        p2 = rclF._aclPoints[1];
        p3 = rclF._aclPoints[2];

        fVolume += (-p3.x*p2.y*p1.z + p2.x*p3.y*p1.z + p3.x*p1.y*p2.z - p1.x*p3.y*p2.z - p2.x*p1.y*p3.z + p1.x*p2.y*p3.z);
    }

    fVolume /= 6.0;
    fVolume = (float)fabs(fVolume);

    return fVolume;
}

bool MeshKernel::HasOpenEdges() const
{
    MeshEvalSolid eval(*this);
    return !eval.Evaluate();
}

bool MeshKernel::HasNonManifolds() const
{
    MeshEvalTopology eval(*this);
    return !eval.Evaluate();
}

bool MeshKernel::HasSelfIntersections() const
{
    MeshEvalSelfIntersection eval(*this);
    return !eval.Evaluate();
}

// Iterators
MeshFacetIterator MeshKernel::FacetIterator() const
{
    MeshFacetIterator it(*this);
    it.Begin(); return it;
}

MeshPointIterator MeshKernel::PointIterator() const
{
    MeshPointIterator it(*this);
    it.Begin(); return it;
}

void MeshKernel::GetEdges (std::vector<MeshGeomEdge>& edges) const
{
    std::set<MeshBuilder::Edge> tmp;

    for (MeshFacetArray::_TConstIterator it = _aclFacetArray.begin(); it != _aclFacetArray.end(); ++it) {
        for (int i = 0; i < 3; i++) {
            tmp.insert(MeshBuilder::Edge(it->_aulPoints[i], it->_aulPoints[(i+1)%3], it->_aulNeighbours[i]));
        }
    }

    edges.reserve(tmp.size());
    for (std::set<MeshBuilder::Edge>::iterator it2 = tmp.begin(); it2 != tmp.end(); ++it2) {
        MeshGeomEdge edge;
        edge._aclPoints[0] = this->_aclPointArray[it2->pt1];
        edge._aclPoints[1] = this->_aclPointArray[it2->pt2];
        edge._bBorder = it2->facetIdx == ULONG_MAX;

        edges.push_back(edge);
    }
}

unsigned long MeshKernel::CountEdges (void) const
{
    unsigned long openEdges = 0, closedEdges = 0;

    for (MeshFacetArray::_TConstIterator it = _aclFacetArray.begin(); it != _aclFacetArray.end(); ++it) {
        for (int i = 0; i < 3; i++) {
            if (it->_aulNeighbours[i] == ULONG_MAX)
                openEdges++;
            else
                closedEdges++;
        }
    }

    return (openEdges + (closedEdges / 2));
}

