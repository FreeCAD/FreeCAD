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
#include <vector>
#endif

#include <Base/Matrix.h>
#include <Base/Sequencer.h>

#include "Algorithm.h"
#include "Approximation.h"
#include "Evaluation.h"
#include "Functional.h"
#include "Grid.h"
#include "Iterator.h"
#include "TopoAlgorithm.h"


using namespace MeshCore;


MeshOrientationVisitor::MeshOrientationVisitor() = default;

bool MeshOrientationVisitor::Visit(const MeshFacet& rclFacet,
                                   const MeshFacet& rclFrom,
                                   FacetIndex ulFInd,
                                   unsigned long ulLevel)
{
    (void)ulFInd;
    (void)ulLevel;
    if (!rclFrom.HasSameOrientation(rclFacet)) {
        _nonuniformOrientation = true;
        return false;
    }

    return true;
}

bool MeshOrientationVisitor::HasNonUnifomOrientedFacets() const
{
    return _nonuniformOrientation;
}

MeshOrientationCollector::MeshOrientationCollector(std::vector<FacetIndex>& aulIndices,
                                                   std::vector<FacetIndex>& aulComplement)
    : _aulIndices(aulIndices)
    , _aulComplement(aulComplement)
{}

bool MeshOrientationCollector::Visit(const MeshFacet& rclFacet,
                                     const MeshFacet& rclFrom,
                                     FacetIndex ulFInd,
                                     unsigned long ulLevel)
{
    (void)ulLevel;
    // different orientation of rclFacet and rclFrom
    if (!rclFacet.HasSameOrientation(rclFrom)) {
        // is not marked as false oriented
        if (!rclFrom.IsFlag(MeshFacet::TMP0)) {
            // mark this facet as false oriented
            rclFacet.SetFlag(MeshFacet::TMP0);
            _aulIndices.push_back(ulFInd);
        }
        else {
            _aulComplement.push_back(ulFInd);
        }
    }
    else {
        // same orientation but if the neighbour rclFrom is false oriented
        // then rclFrom is also false oriented
        if (rclFrom.IsFlag(MeshFacet::TMP0)) {
            // mark this facet as false oriented
            rclFacet.SetFlag(MeshFacet::TMP0);
            _aulIndices.push_back(ulFInd);
        }
        else {
            _aulComplement.push_back(ulFInd);
        }
    }

    return true;
}

MeshSameOrientationCollector::MeshSameOrientationCollector(std::vector<FacetIndex>& aulIndices)
    : _aulIndices(aulIndices)
{}

bool MeshSameOrientationCollector::Visit(const MeshFacet& rclFacet,
                                         const MeshFacet& rclFrom,
                                         FacetIndex ulFInd,
                                         unsigned long ulLevel)
{
    // different orientation of rclFacet and rclFrom
    (void)ulLevel;
    if (rclFacet.HasSameOrientation(rclFrom)) {
        _aulIndices.push_back(ulFInd);
    }

    return true;
}

// ----------------------------------------------------

MeshEvalOrientation::MeshEvalOrientation(const MeshKernel& rclM)
    : MeshEvaluation(rclM)
{}

bool MeshEvalOrientation::Evaluate()
{
    const MeshFacetArray& rFAry = _rclMesh.GetFacets();
    MeshFacetArray::_TConstIterator iBeg = rFAry.begin();
    MeshFacetArray::_TConstIterator iEnd = rFAry.end();
    for (MeshFacetArray::_TConstIterator it = iBeg; it != iEnd; ++it) {
        for (int i = 0; i < 3; i++) {
            if (it->_aulNeighbours[i] != FACET_INDEX_MAX) {
                const MeshFacet& rclFacet = iBeg[it->_aulNeighbours[i]];
                for (int j = 0; j < 3; j++) {
                    if (it->_aulPoints[i] == rclFacet._aulPoints[j]) {
                        if ((it->_aulPoints[(i + 1) % 3] == rclFacet._aulPoints[(j + 1) % 3])
                            || (it->_aulPoints[(i + 2) % 3] == rclFacet._aulPoints[(j + 2) % 3])) {
                            return false;  // adjacent face with wrong orientation
                        }
                    }
                }
            }
        }
    }

    return true;
}

unsigned long MeshEvalOrientation::HasFalsePositives(const std::vector<FacetIndex>& inds) const
{
    // All faces with wrong orientation (i.e. adjacent faces with a normal flip and their
    // neighbours) build a segment and are marked as TMP0. Now we check all border faces of the
    // segments with their correct neighbours if there was really a normal flip. If there is no
    // normal flip we have a false positive. False-positives can occur if the mesh structure has
    // some defects which let the region-grow algorithm fail to detect the faces with wrong
    // orientation.
    const MeshFacetArray& rFAry = _rclMesh.GetFacets();
    MeshFacetArray::_TConstIterator iBeg = rFAry.begin();
    for (FacetIndex it : inds) {
        const MeshFacet& f = iBeg[it];
        for (FacetIndex nbIndex : f._aulNeighbours) {
            if (nbIndex != FACET_INDEX_MAX) {
                const MeshFacet& n = iBeg[nbIndex];
                if (f.IsFlag(MeshFacet::TMP0) && !n.IsFlag(MeshFacet::TMP0)) {
                    for (int j = 0; j < 3; j++) {
                        if (f.HasSameOrientation(n)) {
                            // adjacent face with same orientation => false positive
                            return nbIndex;
                        }
                    }
                }
            }
        }
    }

    return FACET_INDEX_MAX;
}

std::vector<FacetIndex> MeshEvalOrientation::GetIndices() const
{
    FacetIndex ulStartFacet {}, ulVisited {};

    if (_rclMesh.CountFacets() == 0) {
        return {};
    }

    // reset VISIT flags
    MeshAlgorithm cAlg(_rclMesh);
    cAlg.ResetFacetFlag(MeshFacet::VISIT);
    cAlg.ResetFacetFlag(MeshFacet::TMP0);

    const MeshFacetArray& rFAry = _rclMesh.GetFacets();
    MeshFacetArray::_TConstIterator iTri = rFAry.begin();
    MeshFacetArray::_TConstIterator iBeg = rFAry.begin();
    MeshFacetArray::_TConstIterator iEnd = rFAry.end();

    ulStartFacet = 0;

    std::vector<FacetIndex> uIndices, uComplement;
    MeshOrientationCollector clHarmonizer(uIndices, uComplement);

    while (ulStartFacet != FACET_INDEX_MAX) {
        unsigned long wrongFacets = uIndices.size();

        uComplement.clear();
        uComplement.push_back(ulStartFacet);
        ulVisited = _rclMesh.VisitNeighbourFacets(clHarmonizer, ulStartFacet) + 1;

        // In the currently visited component we have found less than 40% as correct
        // oriented and the rest as false oriented. So, we decide that it should be the other
        // way round and swap the indices of this component.
        if (uComplement.size() < static_cast<unsigned long>(0.4f * static_cast<float>(ulVisited))) {
            uIndices.erase(uIndices.begin() + wrongFacets, uIndices.end());
            uIndices.insert(uIndices.end(), uComplement.begin(), uComplement.end());
        }

        // if the mesh consists of several topologic independent components
        // We can search from position 'iTri' on because all elements _before_ are already visited
        // what we know from the previous iteration.
        MeshIsNotFlag<MeshFacet> flag;
        iTri = std::find_if(iTri, iEnd, [flag](const MeshFacet& f) {
            return flag(f, MeshFacet::VISIT);
        });

        if (iTri < iEnd) {
            ulStartFacet = iTri - iBeg;
        }
        else {
            ulStartFacet = FACET_INDEX_MAX;
        }
    }

    // in some very rare cases where we have some strange artifacts in the mesh structure
    // we get false-positives. If we find some we check all 'invalid' faces again
    cAlg.ResetFacetFlag(MeshFacet::TMP0);
    cAlg.SetFacetsFlag(uIndices, MeshFacet::TMP0);
    ulStartFacet = HasFalsePositives(uIndices);
    while (ulStartFacet != FACET_INDEX_MAX) {
        cAlg.ResetFacetsFlag(uIndices, MeshFacet::VISIT);
        std::vector<FacetIndex> falsePos;
        MeshSameOrientationCollector coll(falsePos);
        _rclMesh.VisitNeighbourFacets(coll, ulStartFacet);

        std::sort(uIndices.begin(), uIndices.end());
        std::sort(falsePos.begin(), falsePos.end());

        std::vector<FacetIndex> diff;
        std::back_insert_iterator<std::vector<FacetIndex>> biit(diff);
        std::set_difference(uIndices.begin(),
                            uIndices.end(),
                            falsePos.begin(),
                            falsePos.end(),
                            biit);
        uIndices = diff;

        cAlg.ResetFacetFlag(MeshFacet::TMP0);
        cAlg.SetFacetsFlag(uIndices, MeshFacet::TMP0);
        FacetIndex current = ulStartFacet;
        ulStartFacet = HasFalsePositives(uIndices);
        if (current == ulStartFacet) {
            break;  // avoid an endless loop
        }
    }

    return uIndices;
}

MeshFixOrientation::MeshFixOrientation(MeshKernel& rclM)
    : MeshValidation(rclM)
{}

bool MeshFixOrientation::Fixup()
{
    MeshTopoAlgorithm(_rclMesh).HarmonizeNormals();
    return MeshEvalOrientation(_rclMesh).Evaluate();
}

// ----------------------------------------------------

MeshEvalSolid::MeshEvalSolid(const MeshKernel& rclM)
    : MeshEvaluation(rclM)
{}

bool MeshEvalSolid::Evaluate()
{
    std::vector<MeshGeomEdge> edges;
    _rclMesh.GetEdges(edges);
    for (const auto& it : edges) {
        if (it._bBorder) {
            return false;
        }
    }

    return true;
}

// ----------------------------------------------------

namespace MeshCore
{

struct Edge_Index
{
    PointIndex p0, p1;
    FacetIndex f;
};

struct Edge_Less
{
    bool operator()(const Edge_Index& x, const Edge_Index& y) const
    {
        if (x.p0 < y.p0) {
            return true;
        }
        else if (x.p0 > y.p0) {
            return false;
        }
        else if (x.p1 < y.p1) {
            return true;
        }
        else if (x.p1 > y.p1) {
            return false;
        }
        return false;
    }
};

}  // namespace MeshCore

bool MeshEvalTopology::Evaluate()
{
    // Using and sorting a vector seems to be faster and more memory-efficient
    // than a map.
    const MeshFacetArray& rclFAry = _rclMesh.GetFacets();
    std::vector<Edge_Index> edges;
    edges.reserve(3 * rclFAry.size());

    // build up an array of edges
    MeshFacetArray::_TConstIterator pI;
    Base::SequencerLauncher seq("Checking topology...", rclFAry.size());
    for (pI = rclFAry.begin(); pI != rclFAry.end(); ++pI) {
        for (int i = 0; i < 3; i++) {
            Edge_Index item {};
            item.p0 = std::min<PointIndex>(pI->_aulPoints[i], pI->_aulPoints[(i + 1) % 3]);
            item.p1 = std::max<PointIndex>(pI->_aulPoints[i], pI->_aulPoints[(i + 1) % 3]);
            item.f = pI - rclFAry.begin();
            edges.push_back(item);
        }

        seq.next();
    }

    // sort the edges
    std::sort(edges.begin(), edges.end(), Edge_Less());

    // search for non-manifold edges
    PointIndex p0 = POINT_INDEX_MAX, p1 = POINT_INDEX_MAX;
    nonManifoldList.clear();
    nonManifoldFacets.clear();

    int count = 0;
    std::vector<FacetIndex> facets;
    std::vector<Edge_Index>::iterator pE;
    for (pE = edges.begin(); pE != edges.end(); ++pE) {
        if (p0 == pE->p0 && p1 == pE->p1) {
            count++;
            facets.push_back(pE->f);
        }
        else {
            if (count > 2) {
                // Edge that is shared by more than 2 facets
                nonManifoldList.emplace_back(p0, p1);
                nonManifoldFacets.push_back(facets);
            }

            p0 = pE->p0;
            p1 = pE->p1;
            facets.clear();
            facets.push_back(pE->f);
            count = 1;
        }
    }

    return nonManifoldList.empty();
}

// generate indexed edge list which tangents non-manifolds
void MeshEvalTopology::GetFacetManifolds(std::vector<FacetIndex>& raclFacetIndList) const
{
    raclFacetIndList.clear();
    const MeshFacetArray& rclFAry = _rclMesh.GetFacets();
    MeshFacetArray::_TConstIterator pI;

    for (pI = rclFAry.begin(); pI != rclFAry.end(); ++pI) {
        for (int i = 0; i < 3; i++) {
            PointIndex ulPt0 = std::min<PointIndex>(pI->_aulPoints[i], pI->_aulPoints[(i + 1) % 3]);
            PointIndex ulPt1 = std::max<PointIndex>(pI->_aulPoints[i], pI->_aulPoints[(i + 1) % 3]);
            std::pair<PointIndex, PointIndex> edge = std::make_pair(ulPt0, ulPt1);

            if (std::find(nonManifoldList.begin(), nonManifoldList.end(), edge)
                != nonManifoldList.end()) {
                raclFacetIndList.push_back(pI - rclFAry.begin());
            }
        }
    }
}

unsigned long MeshEvalTopology::CountManifolds() const
{
    return nonManifoldList.size();
}

bool MeshFixTopology::Fixup()
{
#if 0
    MeshEvalTopology eval(_rclMesh);
    if (!eval.Evaluate()) {
        eval.GetFacetManifolds(deletedFaces);

        // remove duplicates
        std::sort(deletedFaces.begin(), deletedFaces.end());
        deletedFaces.erase(std::unique(deletedFaces.begin(), deletedFaces.end()), deletedFaces.end());

        _rclMesh.DeleteFacets(deletedFaces);
    }
#else
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();
    deletedFaces.reserve(3 * nonManifoldList.size());  // allocate some memory
    for (const auto& it : nonManifoldList) {
        std::vector<FacetIndex> non_mf;
        non_mf.reserve(it.size());
        for (FacetIndex jt : it) {
            // facet is only connected with one edge and there causes a non-manifold
            unsigned short numOpenEdges = rFaces[jt].CountOpenEdges();
            if (numOpenEdges == 2) {
                non_mf.push_back(jt);
            }
            else if (rFaces[jt].IsDegenerated()) {
                non_mf.push_back(jt);
            }
        }

        // are we able to repair the non-manifold edge by not removing all facets?
        if (it.size() - non_mf.size() == 2) {
            deletedFaces.insert(deletedFaces.end(), non_mf.begin(), non_mf.end());
        }
        else {
            deletedFaces.insert(deletedFaces.end(), it.begin(), it.end());
        }
    }

    if (!deletedFaces.empty()) {
        // remove duplicates
        std::sort(deletedFaces.begin(), deletedFaces.end());
        deletedFaces.erase(std::unique(deletedFaces.begin(), deletedFaces.end()),
                           deletedFaces.end());

        _rclMesh.DeleteFacets(deletedFaces);
        _rclMesh.RebuildNeighbours();
    }
#endif

    return true;
}

// ---------------------------------------------------------

bool MeshEvalPointManifolds::Evaluate()
{
    this->nonManifoldPoints.clear();
    this->facetsOfNonManifoldPoints.clear();

    MeshCore::MeshRefPointToPoints vv_it(_rclMesh);
    MeshCore::MeshRefPointToFacets vf_it(_rclMesh);

    unsigned long ctPoints = _rclMesh.CountPoints();
    for (PointIndex index = 0; index < ctPoints; index++) {
        // get the local neighbourhood of the point
        const std::set<FacetIndex>& nf = vf_it[index];
        const std::set<PointIndex>& np = vv_it[index];

        std::set<unsigned long>::size_type sp {}, sf {};
        sp = np.size();
        sf = nf.size();
        // for an inner point the number of adjacent points is equal to the number of shared faces
        // for a boundary point the number of adjacent points is higher by one than the number of
        // shared faces for a non-manifold point the number of adjacent points is higher by more
        // than one than the number of shared faces
        if (sp > sf + 1) {
            nonManifoldPoints.push_back(index);
            std::vector<FacetIndex> faces;
            faces.insert(faces.end(), nf.begin(), nf.end());
            this->facetsOfNonManifoldPoints.push_back(faces);
        }
    }

    return this->nonManifoldPoints.empty();
}

void MeshEvalPointManifolds::GetFacetIndices(std::vector<FacetIndex>& facets) const
{
    std::list<std::vector<FacetIndex>>::const_iterator it;
    for (it = facetsOfNonManifoldPoints.begin(); it != facetsOfNonManifoldPoints.end(); ++it) {
        facets.insert(facets.end(), it->begin(), it->end());
    }

    if (!facets.empty()) {
        // remove duplicates
        std::sort(facets.begin(), facets.end());
        facets.erase(std::unique(facets.begin(), facets.end()), facets.end());
    }
}

// ---------------------------------------------------------

bool MeshEvalSingleFacet::Evaluate()
{
    // get all non-manifolds
    (void)MeshEvalTopology::Evaluate();
    /*
      // for each (multiple) single linked facet there should
      // exist two valid facets sharing the same edge
      // so make facet 1 neighbour of facet 2 and vice versa
      const std::vector<MeshFacet>& rclFAry = _rclMesh.GetFacets();
      std::vector<MeshFacet>::const_iterator pI;

      std::vector<std::list<unsigned long> > aclMf = _aclManifoldList;
      _aclManifoldList.clear();

      std::map<std::pair<unsigned long, unsigned long>, std::list<unsigned long> > aclHits;
      std::map<std::pair<unsigned long, unsigned long>, std::list<unsigned long> >::iterator pEdge;

      // search for single links (a non-manifold edge and two open edges)
      //
      //
      // build edge <=> facet map
      for (pI = rclFAry.begin(); pI != rclFAry.end(); ++pI)
      {
        for (int i = 0; i < 3; i++)
        {
          unsigned long ulPt0 = std::min<unsigned long>(pI->_aulPoints[i], pI->_aulPoints[(i+1)%3]);
          unsigned long ulPt1 = std::max<unsigned long>(pI->_aulPoints[i], pI->_aulPoints[(i+1)%3]);
          aclHits[std::pair<unsigned long, unsigned long>(ulPt0, ulPt1)].push_front(pI -
      rclFAry.begin());
        }
      }

      // now search for single links
      for (std::vector<std::list<unsigned long> >::const_iterator pMF = aclMf.begin(); pMF !=
      aclMf.end(); ++pMF)
      {
        std::list<unsigned long> aulManifolds;
        for (std::list<unsigned long>::const_iterator pF = pMF->begin(); pF != pMF->end(); ++pF)
        {
          const MeshFacet& rclF = rclFAry[*pF];

          unsigned long ulCtNeighbours=0;
          for (int i = 0; i < 3; i++)
          {
            unsigned long ulPt0 = std::min<unsigned long>(rclF._aulPoints[i],
      rclF._aulPoints[(i+1)%3]); unsigned long ulPt1 = std::max<unsigned long>(rclF._aulPoints[i],
      rclF._aulPoints[(i+1)%3]); std::pair<unsigned long, unsigned long> clEdge(ulPt0, ulPt1);

            // number of facets sharing this edge
            ulCtNeighbours += aclHits[clEdge].size();
          }

          // single linked found
          if (ulCtNeighbours == pMF->size() + 2)
            aulManifolds.push_front(*pF);
        }

        if ( aulManifolds.size() > 0 )
          _aclManifoldList.push_back(aulManifolds);
      }
    */
    return (nonManifoldList.empty());
}

bool MeshFixSingleFacet::Fixup()
{
    std::vector<FacetIndex> aulInvalids;
    for (const auto& it : _raclManifoldList) {
        for (FacetIndex it2 : it) {
            aulInvalids.push_back(it2);
        }
    }

    _rclMesh.DeleteFacets(aulInvalids);
    return true;
}

// ----------------------------------------------------------------

bool MeshEvalSelfIntersection::Evaluate()
{
    // Contains bounding boxes for every facet
    std::vector<Base::BoundBox3f> boxes;

    // Splits the mesh using grid for speeding up the calculation
    MeshFacetGrid cMeshFacetGrid(_rclMesh);
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();
    MeshGridIterator clGridIter(cMeshFacetGrid);
    unsigned long ulGridX {}, ulGridY {}, ulGridZ {};
    cMeshFacetGrid.GetCtGrids(ulGridX, ulGridY, ulGridZ);

    MeshFacetIterator cMFI(_rclMesh);
    for (cMFI.Begin(); cMFI.More(); cMFI.Next()) {
        boxes.push_back((*cMFI).GetBoundBox());
    }

    // Calculates the intersections
    Base::SequencerLauncher seq("Checking for self-intersections...", ulGridX * ulGridY * ulGridZ);
    for (clGridIter.Init(); clGridIter.More(); clGridIter.Next()) {
        // Get the facet indices, belonging to the current grid unit
        std::vector<FacetIndex> aulGridElements;
        clGridIter.GetElements(aulGridElements);

        seq.next();
        if (aulGridElements.empty()) {
            continue;
        }

        MeshGeomFacet facet1, facet2;
        Base::Vector3f pt1, pt2;
        for (std::vector<FacetIndex>::iterator it = aulGridElements.begin();
             it != aulGridElements.end();
             ++it) {
            const Base::BoundBox3f& box1 = boxes[*it];
            cMFI.Set(*it);
            facet1 = *cMFI;
            const MeshFacet& rface1 = rFaces[*it];
            for (std::vector<FacetIndex>::iterator jt = it; jt != aulGridElements.end(); ++jt) {
                if (jt == it) {  // the identical facet
                    continue;
                }
                // If the facets share a common vertex we do not check for self-intersections
                // because they could but usually do not intersect each other and the algorithm
                // below would detect false-positives, otherwise
                const MeshFacet& rface2 = rFaces[*jt];
                if (rface1._aulPoints[0] == rface2._aulPoints[0]
                    || rface1._aulPoints[0] == rface2._aulPoints[1]
                    || rface1._aulPoints[0] == rface2._aulPoints[2]) {
                    continue;  // ignore facets sharing a common vertex
                }
                if (rface1._aulPoints[1] == rface2._aulPoints[0]
                    || rface1._aulPoints[1] == rface2._aulPoints[1]
                    || rface1._aulPoints[1] == rface2._aulPoints[2]) {
                    continue;  // ignore facets sharing a common vertex
                }
                if (rface1._aulPoints[2] == rface2._aulPoints[0]
                    || rface1._aulPoints[2] == rface2._aulPoints[1]
                    || rface1._aulPoints[2] == rface2._aulPoints[2]) {
                    continue;  // ignore facets sharing a common vertex
                }

                const Base::BoundBox3f& box2 = boxes[*jt];
                if (box1 && box2) {
                    cMFI.Set(*jt);
                    facet2 = *cMFI;
                    int ret = facet1.IntersectWithFacet(facet2, pt1, pt2);
                    if (ret == 2) {
                        // abort after the first detected self-intersection
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

void MeshEvalSelfIntersection::GetIntersections(
    const std::vector<std::pair<FacetIndex, FacetIndex>>& indices,
    std::vector<std::pair<Base::Vector3f, Base::Vector3f>>& intersection) const
{
    intersection.reserve(indices.size());
    MeshFacetIterator cMF1(_rclMesh);
    MeshFacetIterator cMF2(_rclMesh);

    Base::Vector3f pt1, pt2;
    std::vector<std::pair<FacetIndex, FacetIndex>>::const_iterator it;
    for (it = indices.begin(); it != indices.end(); ++it) {
        cMF1.Set(it->first);
        cMF2.Set(it->second);

        Base::BoundBox3f box1 = cMF1->GetBoundBox();
        Base::BoundBox3f box2 = cMF2->GetBoundBox();
        if (box1 && box2) {
            int ret = cMF1->IntersectWithFacet(*cMF2, pt1, pt2);
            if (ret == 2) {
                intersection.emplace_back(pt1, pt2);
            }
        }
    }
}

void MeshEvalSelfIntersection::GetIntersections(
    std::vector<std::pair<FacetIndex, FacetIndex>>& intersection) const
{
    // Contains bounding boxes for every facet
    std::vector<Base::BoundBox3f> boxes;
    // intersection.clear();

    // Splits the mesh using grid for speeding up the calculation
    MeshFacetGrid cMeshFacetGrid(_rclMesh);
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();
    MeshGridIterator clGridIter(cMeshFacetGrid);
    unsigned long ulGridX {}, ulGridY {}, ulGridZ {};
    cMeshFacetGrid.GetCtGrids(ulGridX, ulGridY, ulGridZ);

    MeshFacetIterator cMFI(_rclMesh);
    for (cMFI.Begin(); cMFI.More(); cMFI.Next()) {
        boxes.push_back((*cMFI).GetBoundBox());
    }

    // Calculates the intersections
    Base::SequencerLauncher seq("Checking for self-intersections...", ulGridX * ulGridY * ulGridZ);
    for (clGridIter.Init(); clGridIter.More(); clGridIter.Next()) {
        // Get the facet indices, belonging to the current grid unit
        std::vector<FacetIndex> aulGridElements;
        clGridIter.GetElements(aulGridElements);

        seq.next(true);
        if (aulGridElements.empty()) {
            continue;
        }

        MeshGeomFacet facet1, facet2;
        Base::Vector3f pt1, pt2;
        for (std::vector<FacetIndex>::iterator it = aulGridElements.begin();
             it != aulGridElements.end();
             ++it) {
            const Base::BoundBox3f& box1 = boxes[*it];
            cMFI.Set(*it);
            facet1 = *cMFI;
            const MeshFacet& rface1 = rFaces[*it];
            for (std::vector<FacetIndex>::iterator jt = it; jt != aulGridElements.end(); ++jt) {
                if (jt == it) {  // the identical facet
                    continue;
                }
                // If the facets share a common vertex we do not check for self-intersections
                // because they could but usually do not intersect each other and the algorithm
                // below would detect false-positives, otherwise
                const MeshFacet& rface2 = rFaces[*jt];
                if (rface1._aulPoints[0] == rface2._aulPoints[0]
                    || rface1._aulPoints[0] == rface2._aulPoints[1]
                    || rface1._aulPoints[0] == rface2._aulPoints[2]) {
                    continue;  // ignore facets sharing a common vertex
                }
                if (rface1._aulPoints[1] == rface2._aulPoints[0]
                    || rface1._aulPoints[1] == rface2._aulPoints[1]
                    || rface1._aulPoints[1] == rface2._aulPoints[2]) {
                    continue;  // ignore facets sharing a common vertex
                }
                if (rface1._aulPoints[2] == rface2._aulPoints[0]
                    || rface1._aulPoints[2] == rface2._aulPoints[1]
                    || rface1._aulPoints[2] == rface2._aulPoints[2]) {
                    continue;  // ignore facets sharing a common vertex
                }

                const Base::BoundBox3f& box2 = boxes[*jt];
                if (box1 && box2) {
                    cMFI.Set(*jt);
                    facet2 = *cMFI;
                    int ret = facet1.IntersectWithFacet(facet2, pt1, pt2);
                    if (ret == 2) {
                        intersection.emplace_back(*it, *jt);
                    }
                }
            }
        }
    }
}

std::vector<FacetIndex> MeshFixSelfIntersection::GetFacets() const
{
    std::vector<FacetIndex> indices;
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();
    for (const auto& it : selfIntersectons) {
        unsigned short numOpenEdges1 = rFaces[it.first].CountOpenEdges();
        unsigned short numOpenEdges2 = rFaces[it.second].CountOpenEdges();

        // often we have only single or border facets that intersect other facets
        // in this case remove only these facets and keep the other one
        if (numOpenEdges1 == 0 && numOpenEdges2 > 0) {
            indices.push_back(it.second);
        }
        else if (numOpenEdges1 > 0 && numOpenEdges2 == 0) {
            indices.push_back(it.first);
        }
        else {
            indices.push_back(it.first);
            indices.push_back(it.second);
        }
    }

    // remove duplicates
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

    return indices;
}

bool MeshFixSelfIntersection::Fixup()
{
    _rclMesh.DeleteFacets(GetFacets());
    return true;
}

// ----------------------------------------------------------------

bool MeshEvalNeighbourhood::Evaluate()
{
    // Note: If more than two facets are attached to the edge then we have a
    // non-manifold edge here.
    // This means that the neighbourhood cannot be valid, for sure. But we just
    // want to check whether the neighbourhood is valid for topologic correctly
    // edges and thus we ignore this case.
    // Non-manifolds are an own category of errors and are handled by the class
    // MeshEvalTopology.
    //
    // Using and sorting a vector seems to be faster and more memory-efficient
    // than a map.
    const MeshFacetArray& rclFAry = _rclMesh.GetFacets();
    std::vector<Edge_Index> edges;
    edges.reserve(3 * rclFAry.size());

    // build up an array of edges
    MeshFacetArray::_TConstIterator pI;
    Base::SequencerLauncher seq("Checking indices...", rclFAry.size());
    for (pI = rclFAry.begin(); pI != rclFAry.end(); ++pI) {
        for (int i = 0; i < 3; i++) {
            Edge_Index item {};
            item.p0 = std::min<PointIndex>(pI->_aulPoints[i], pI->_aulPoints[(i + 1) % 3]);
            item.p1 = std::max<PointIndex>(pI->_aulPoints[i], pI->_aulPoints[(i + 1) % 3]);
            item.f = pI - rclFAry.begin();
            edges.push_back(item);
        }

        seq.next();
    }

    // sort the edges
    std::sort(edges.begin(), edges.end(), Edge_Less());

    PointIndex p0 = POINT_INDEX_MAX, p1 = POINT_INDEX_MAX;
    PointIndex f0 = FACET_INDEX_MAX, f1 = FACET_INDEX_MAX;
    int count = 0;
    std::vector<Edge_Index>::iterator pE;
    for (pE = edges.begin(); pE != edges.end(); ++pE) {
        if (p0 == pE->p0 && p1 == pE->p1) {
            f1 = pE->f;
            count++;
        }
        else {
            // we handle only the cases for 1 and 2, for all higher
            // values we have a non-manifold that is ignored here
            if (count == 2) {
                const MeshFacet& rFace0 = rclFAry[f0];
                const MeshFacet& rFace1 = rclFAry[f1];
                unsigned short side0 = rFace0.Side(p0, p1);
                unsigned short side1 = rFace1.Side(p0, p1);
                // Check whether rFace0 and rFace1 reference each other as
                // neighbours
                if (rFace0._aulNeighbours[side0] != f1 || rFace1._aulNeighbours[side1] != f0) {
                    return false;
                }
            }
            else if (count == 1) {
                const MeshFacet& rFace = rclFAry[f0];
                unsigned short side = rFace.Side(p0, p1);
                // should be "open edge" but isn't marked as such
                if (rFace._aulNeighbours[side] != FACET_INDEX_MAX) {
                    return false;
                }
            }

            p0 = pE->p0;
            p1 = pE->p1;
            f0 = pE->f;
            count = 1;
        }
    }

    return true;
}

std::vector<FacetIndex> MeshEvalNeighbourhood::GetIndices() const
{
    std::vector<FacetIndex> inds;
    const MeshFacetArray& rclFAry = _rclMesh.GetFacets();
    std::vector<Edge_Index> edges;
    edges.reserve(3 * rclFAry.size());

    // build up an array of edges
    MeshFacetArray::_TConstIterator pI;
    Base::SequencerLauncher seq("Checking indices...", rclFAry.size());
    for (pI = rclFAry.begin(); pI != rclFAry.end(); ++pI) {
        for (int i = 0; i < 3; i++) {
            Edge_Index item {};
            item.p0 = std::min<PointIndex>(pI->_aulPoints[i], pI->_aulPoints[(i + 1) % 3]);
            item.p1 = std::max<PointIndex>(pI->_aulPoints[i], pI->_aulPoints[(i + 1) % 3]);
            item.f = pI - rclFAry.begin();
            edges.push_back(item);
        }

        seq.next();
    }

    // sort the edges
    std::sort(edges.begin(), edges.end(), Edge_Less());

    PointIndex p0 = POINT_INDEX_MAX, p1 = POINT_INDEX_MAX;
    PointIndex f0 = FACET_INDEX_MAX, f1 = FACET_INDEX_MAX;
    int count = 0;
    std::vector<Edge_Index>::iterator pE;
    for (pE = edges.begin(); pE != edges.end(); ++pE) {
        if (p0 == pE->p0 && p1 == pE->p1) {
            f1 = pE->f;
            count++;
        }
        else {
            // we handle only the cases for 1 and 2, for all higher
            // values we have a non-manifold that is ignored here
            if (count == 2) {
                const MeshFacet& rFace0 = rclFAry[f0];
                const MeshFacet& rFace1 = rclFAry[f1];
                unsigned short side0 = rFace0.Side(p0, p1);
                unsigned short side1 = rFace1.Side(p0, p1);
                // Check whether rFace0 and rFace1 reference each other as
                // neighbours
                if (rFace0._aulNeighbours[side0] != f1 || rFace1._aulNeighbours[side1] != f0) {
                    inds.push_back(f0);
                    inds.push_back(f1);
                }
            }
            else if (count == 1) {
                const MeshFacet& rFace = rclFAry[f0];
                unsigned short side = rFace.Side(p0, p1);
                // should be "open edge" but isn't marked as such
                if (rFace._aulNeighbours[side] != FACET_INDEX_MAX) {
                    inds.push_back(f0);
                }
            }

            p0 = pE->p0;
            p1 = pE->p1;
            f0 = pE->f;
            count = 1;
        }
    }

    // remove duplicates
    std::sort(inds.begin(), inds.end());
    inds.erase(std::unique(inds.begin(), inds.end()), inds.end());

    return inds;
}

bool MeshFixNeighbourhood::Fixup()
{
    _rclMesh.RebuildNeighbours();
    return true;
}

void MeshKernel::RebuildNeighbours(FacetIndex index)
{
    std::vector<Edge_Index> edges;
    edges.reserve(3 * (this->_aclFacetArray.size() - index));

    // build up an array of edges
    MeshFacetArray::_TConstIterator pI;
    MeshFacetArray::_TConstIterator pB = this->_aclFacetArray.begin();
    for (pI = pB + index; pI != this->_aclFacetArray.end(); ++pI) {
        for (int i = 0; i < 3; i++) {
            Edge_Index item {};
            item.p0 = std::min<PointIndex>(pI->_aulPoints[i], pI->_aulPoints[(i + 1) % 3]);
            item.p1 = std::max<PointIndex>(pI->_aulPoints[i], pI->_aulPoints[(i + 1) % 3]);
            item.f = pI - pB;
            edges.push_back(item);
        }
    }

    // sort the edges
    // std::sort(edges.begin(), edges.end(), Edge_Less());
    int threads = int(std::thread::hardware_concurrency());
    MeshCore::parallel_sort(edges.begin(), edges.end(), Edge_Less(), threads);

    PointIndex p0 = POINT_INDEX_MAX, p1 = POINT_INDEX_MAX;
    PointIndex f0 = FACET_INDEX_MAX, f1 = FACET_INDEX_MAX;
    int count = 0;
    std::vector<Edge_Index>::iterator pE;
    for (pE = edges.begin(); pE != edges.end(); ++pE) {
        if (p0 == pE->p0 && p1 == pE->p1) {
            f1 = pE->f;
            count++;
        }
        else {
            // we handle only the cases for 1 and 2, for all higher
            // values we have a non-manifold that is ignored here
            if (count == 2) {
                MeshFacet& rFace0 = this->_aclFacetArray[f0];
                MeshFacet& rFace1 = this->_aclFacetArray[f1];
                unsigned short side0 = rFace0.Side(p0, p1);
                unsigned short side1 = rFace1.Side(p0, p1);
                rFace0._aulNeighbours[side0] = f1;
                rFace1._aulNeighbours[side1] = f0;
            }
            else if (count == 1) {
                MeshFacet& rFace = this->_aclFacetArray[f0];
                unsigned short side = rFace.Side(p0, p1);
                rFace._aulNeighbours[side] = FACET_INDEX_MAX;
            }

            p0 = pE->p0;
            p1 = pE->p1;
            f0 = pE->f;
            count = 1;
        }
    }

    // we handle only the cases for 1 and 2, for all higher
    // values we have a non-manifold that is ignored here
    if (count == 2) {
        MeshFacet& rFace0 = this->_aclFacetArray[f0];
        MeshFacet& rFace1 = this->_aclFacetArray[f1];
        unsigned short side0 = rFace0.Side(p0, p1);
        unsigned short side1 = rFace1.Side(p0, p1);
        rFace0._aulNeighbours[side0] = f1;
        rFace1._aulNeighbours[side1] = f0;
    }
    else if (count == 1) {
        MeshFacet& rFace = this->_aclFacetArray[f0];
        unsigned short side = rFace.Side(p0, p1);
        rFace._aulNeighbours[side] = FACET_INDEX_MAX;
    }
}

void MeshKernel::RebuildNeighbours()
{
    // complete rebuild
    RebuildNeighbours(0);
}

// ----------------------------------------------------------------

MeshEigensystem::MeshEigensystem(const MeshKernel& rclB)
    : MeshEvaluation(rclB)
    , _cU(1.0f, 0.0f, 0.0f)
    , _cV(0.0f, 1.0f, 0.0f)
    , _cW(0.0f, 0.0f, 1.0f)
{
    // use the values of world coordinates as default
    Base::BoundBox3f box = _rclMesh.GetBoundBox();
    _fU = box.LengthX();
    _fV = box.LengthY();
    _fW = box.LengthZ();
}

Base::Matrix4D MeshEigensystem::Transform() const
{
    // x,y,c ... vectors
    // R,Q   ... matrices (R is orthonormal so its transposed(=inverse) is equal to Q)
    //
    // from local (x) to world (y,c) coordinates we have the equation
    // y = R * x  + c
    //     <==>
    // x = Q * y - Q * c
    Base::Matrix4D clTMat;
    // rotation part
    clTMat[0][0] = double(_cU.x);
    clTMat[0][1] = double(_cU.y);
    clTMat[0][2] = double(_cU.z);
    clTMat[0][3] = 0.0;
    clTMat[1][0] = double(_cV.x);
    clTMat[1][1] = double(_cV.y);
    clTMat[1][2] = double(_cV.z);
    clTMat[1][3] = 0.0;
    clTMat[2][0] = double(_cW.x);
    clTMat[2][1] = double(_cW.y);
    clTMat[2][2] = double(_cW.z);
    clTMat[2][3] = 0.0;
    clTMat[3][0] = 0.0;
    clTMat[3][1] = 0.0;
    clTMat[3][2] = 0.0;
    clTMat[3][3] = 1.0;

    Base::Vector3f c(_cC);
    c = clTMat * c;

    // translation part
    clTMat[0][3] = double(-c.x);
    clTMat[1][3] = double(-c.y);
    clTMat[2][3] = double(-c.z);

    return clTMat;
}

bool MeshEigensystem::Evaluate()
{
    CalculateLocalSystem();

    float xmin = 0.0f, xmax = 0.0f, ymin = 0.0f, ymax = 0.0f, zmin = 0.0f, zmax = 0.0f;

    Base::Vector3f clVect, clProj;
    float fH {};

    const MeshPointArray& aclPoints = _rclMesh.GetPoints();
    for (const auto& it : aclPoints) {
        // u-direction
        clVect = it - _cC;
        clProj.ProjectToLine(clVect, _cU);
        clVect = clVect + clProj;
        fH = clVect.Length();

        // point vectors in the same direction ?
        if ((clVect * _cU) < 0.0f) {
            fH = -fH;
        }

        xmax = std::max<float>(xmax, fH);
        xmin = std::min<float>(xmin, fH);

        // v-direction
        clVect = it - _cC;
        clProj.ProjectToLine(clVect, _cV);
        clVect = clVect + clProj;
        fH = clVect.Length();

        // point vectors in the same direction ?
        if ((clVect * _cV) < 0.0f) {
            fH = -fH;
        }

        ymax = std::max<float>(ymax, fH);
        ymin = std::min<float>(ymin, fH);

        // w-direction
        clVect = it - _cC;
        clProj.ProjectToLine(clVect, _cW);
        clVect = clVect + clProj;
        fH = clVect.Length();

        // point vectors in the same direction ?
        if ((clVect * _cW) < 0.0f) {
            fH = -fH;
        }

        zmax = std::max<float>(zmax, fH);
        zmin = std::min<float>(zmin, fH);
    }

    _fU = xmax - xmin;
    _fV = ymax - ymin;
    _fW = zmax - zmin;

    return false;  // to call Fixup() if needed
}

Base::Vector3f MeshEigensystem::GetBoundings() const
{
    return Base::Vector3f(_fU, _fV, _fW);
}

void MeshEigensystem::CalculateLocalSystem()
{
    // at least one facet is needed
    if (_rclMesh.CountFacets() < 1) {
        return;  // cannot continue calculation
    }

    const MeshPointArray& aclPoints = _rclMesh.GetPoints();
    MeshPointArray::_TConstIterator it;

    PlaneFit planeFit;
    for (it = aclPoints.begin(); it != aclPoints.end(); ++it) {
        planeFit.AddPoint(*it);
    }

    planeFit.Fit();
    _cC = planeFit.GetBase();
    _cU = planeFit.GetDirU();
    _cV = planeFit.GetDirV();
    _cW = planeFit.GetNormal();

    // set the sign for the vectors
    float fSumU {0.0F}, fSumV {0.0F}, fSumW {0.0F};
    for (it = aclPoints.begin(); it != aclPoints.end(); ++it) {
        float fU = _cU * (*it - _cC);
        float fV = _cV * (*it - _cC);
        float fW = _cW * (*it - _cC);
        fSumU += (fU > 0 ? fU * fU : -fU * fU);
        fSumV += (fV > 0 ? fV * fV : -fV * fV);
        fSumW += (fW > 0 ? fW * fW : -fW * fW);
    }

    // avoid ambiguities concerning directions
    if (fSumU < 0.0f) {
        _cU *= -1.0f;
    }
    if (fSumV < 0.0f) {
        _cV *= -1.0f;
    }
    if (fSumW < 0.0f) {
        _cW *= -1.0f;
    }

    if ((_cU % _cV) * _cW < 0.0f) {
        _cW = -_cW;  // make a right-handed system
    }
}
