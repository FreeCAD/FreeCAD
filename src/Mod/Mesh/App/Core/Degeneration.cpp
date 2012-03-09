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
# include <map>
#endif

#include "Degeneration.h"
#include "Definitions.h"
#include "Iterator.h"
#include "Helpers.h"
#include "MeshKernel.h"
#include "Algorithm.h"
#include "Info.h"
#include "Grid.h"
#include "TopoAlgorithm.h"

#include <Base/Sequencer.h>

using namespace MeshCore;

bool MeshEvalInvalids::Evaluate()
{
  const MeshFacetArray& rFaces = _rclMesh.GetFacets();
  for ( MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it )
  {
    if ( !it->IsValid() )
      return false;
  }

  const MeshPointArray& rPoints = _rclMesh.GetPoints();
  for ( MeshPointArray::_TConstIterator jt = rPoints.begin(); jt != rPoints.end(); ++jt )
  {
    if ( !jt->IsValid() )
      return false;
  }

  return true;
}

std::vector<unsigned long> MeshEvalInvalids::GetIndices() const
{
  std::vector<unsigned long> aInds;
  const MeshFacetArray& rFaces = _rclMesh.GetFacets();
  const MeshPointArray& rPoints = _rclMesh.GetPoints();
  unsigned long ind=0;
  for ( MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it, ind++ )
  {
    if ( !it->IsValid() )
      aInds.push_back(ind);
    else if ( !rPoints[it->_aulPoints[0]].IsValid() )
      aInds.push_back(ind);
    else if ( !rPoints[it->_aulPoints[1]].IsValid() )
      aInds.push_back(ind);
    else if ( !rPoints[it->_aulPoints[2]].IsValid() )
      aInds.push_back(ind);
  }

  return aInds;
}

bool MeshFixInvalids::Fixup()
{
  _rclMesh.RemoveInvalids();
  return true;
}

// ----------------------------------------------------------------------

namespace MeshCore {

typedef MeshPointArray::_TConstIterator VertexIterator;
/*
 * When building up a mesh then usually the class MeshBuilder is used. This
 * class uses internally a std::set<MeshPoint> which uses the '<' operator of
 * MeshPoint to sort the points. Thus to be consistent (and avoid using the
 * '==' operator of MeshPoint) we use the same operator when comparing the
 * points in the function object.
 */
struct Vertex_EqualTo  : public std::binary_function<const VertexIterator&,
                                                     const VertexIterator&, bool>
{
    bool operator()(const VertexIterator& x,
                    const VertexIterator& y) const
    {
        if ( (*x) < (*y) )
            return false;
        else if ( (*y) < (*x) )
            return false;
        return true;
    }
};

struct Vertex_Less  : public std::binary_function<const VertexIterator&,
                                                  const VertexIterator&, bool>
{
    bool operator()(const VertexIterator& x,
                    const VertexIterator& y) const
    {
        return (*x) < (*y);
    }
};

}

bool MeshEvalDuplicatePoints::Evaluate()
{
    // get an const iterator to each vertex and sort them in ascending order by
    // their (x,y,z) coordinates
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    std::vector<VertexIterator> vertices;
    vertices.reserve(rPoints.size());
    for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it) {
        vertices.push_back(it);
    }

    // if there are two adjacent vertices which have the same coordinates
    std::sort(vertices.begin(), vertices.end(), Vertex_Less());
    if (std::adjacent_find(vertices.begin(), vertices.end(), Vertex_EqualTo()) < vertices.end() )
        return false;
    return true;
}

std::vector<unsigned long> MeshEvalDuplicatePoints::GetIndices() const
{
    //Note: We must neither use map or set to get duplicated indices because
    //the sort algorithms deliver different results compared to std::sort of
    //a vector.
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    std::vector<VertexIterator> vertices;
    vertices.reserve(rPoints.size());
    for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it) {
        vertices.push_back(it);
    }

    // if there are two adjacent vertices which have the same coordinates
    std::vector<unsigned long> aInds;
    Vertex_EqualTo pred;
    std::sort(vertices.begin(), vertices.end(), Vertex_Less());

    std::vector<VertexIterator>::iterator vt = vertices.begin();
    while (vt < vertices.end()) {
        // get first item which adjacent element has the same vertex
        vt = std::adjacent_find(vt, vertices.end(), pred);
        if (vt < vertices.end()) {
            vt++;
            aInds.push_back(*vt - rPoints.begin());
        }
    }

    return aInds;
}

bool MeshFixDuplicatePoints::Fixup()
{
    //Note: We must neither use map or set to get duplicated indices because
    //the sort algorithms deliver different results compared to std::sort of
    //a vector.
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    std::vector<VertexIterator> vertices;
    vertices.reserve(rPoints.size());
    for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it) {
        vertices.push_back(it);
    }

    // get the indices of adjacent vertices which have the same coordinates
    std::vector<unsigned long> aInds;
    std::sort(vertices.begin(), vertices.end(), Vertex_Less());

    Vertex_EqualTo pred;
    std::vector<VertexIterator>::iterator next = vertices.begin();
    std::map<unsigned long, unsigned long> mapPointIndex;
    std::vector<unsigned long> pointIndices;
    while (next < vertices.end()) {
        next = std::adjacent_find(next, vertices.end(), pred);
        if (next < vertices.end()) {
            std::vector<VertexIterator>::iterator first = next;
            unsigned long first_index = *first - rPoints.begin();
            next++;
            while (next < vertices.end() && pred(*first, *next)) {
                unsigned long next_index = *next - rPoints.begin();
                mapPointIndex[next_index] = first_index;
                pointIndices.push_back(next_index);
                next++;
            }
        }
    }

    // now set all facets to the correct index
    MeshFacetArray& rFacets = _rclMesh._aclFacetArray;
    for (MeshFacetArray::_TIterator it = rFacets.begin(); it != rFacets.end(); ++it) {
        for (int i=0; i<3; i++) {
            std::map<unsigned long, unsigned long>::iterator pt = mapPointIndex.find(it->_aulPoints[i]);
            if (pt != mapPointIndex.end())
                it->_aulPoints[i] = pt->second;
        }
    }

    // remove invalid indices
    _rclMesh.DeletePoints(pointIndices);
    _rclMesh.RebuildNeighbours();
    
    return true;
}

// ----------------------------------------------------------------------

namespace MeshCore {

typedef MeshFacetArray::_TConstIterator FaceIterator;
/*
 * The facet with the lowset index is regarded as 'less'.
 */
struct MeshFacet_Less  : public std::binary_function<const FaceIterator&, 
                                                     const FaceIterator&, bool>
{
    bool operator()(const FaceIterator& x, 
                    const FaceIterator& y) const
    {
        unsigned long tmp;
        unsigned long x0 = x->_aulPoints[0];
        unsigned long x1 = x->_aulPoints[1];
        unsigned long x2 = x->_aulPoints[2];
        unsigned long y0 = y->_aulPoints[0];
        unsigned long y1 = y->_aulPoints[1];
        unsigned long y2 = y->_aulPoints[2];

        if (x0 > x1)
        { tmp = x0; x0 = x1; x1 = tmp; }
        if (x0 > x2)
        { tmp = x0; x0 = x2; x2 = tmp; }
        if (x1 > x2)
        { tmp = x1; x1 = x2; x2 = tmp; }
        if (y0 > y1)
        { tmp = y0; y0 = y1; y1 = tmp; }
        if (y0 > y2)
        { tmp = y0; y0 = y2; y2 = tmp; }
        if (y1 > y2)
        { tmp = y1; y1 = y2; y2 = tmp; }

        if      (x0 < y0)  return true;
        else if (x0 > y0)  return false;
        else if (x1 < y1)  return true;
        else if (x1 > y1)  return false;
        else if (x2 < y2)  return true;
        else               return false;
    }
};

}

/*
 * Two facets are equal if all its three point indices refer to the same
 * location in the point array of the mesh kernel they belong to.
 */
struct MeshFacet_EqualTo  : public std::binary_function<const FaceIterator&, 
                                                        const FaceIterator&, bool>
{
    bool operator()(const FaceIterator& x,
                    const FaceIterator& y) const
    {
        for (int i=0; i<3; i++ ) {
            if (x->_aulPoints[0] == y->_aulPoints[i]) {
                if (x->_aulPoints[1] == y->_aulPoints[(i+1)%3] && 
                    x->_aulPoints[2] == y->_aulPoints[(i+2)%3])
                    return true;
                else if (x->_aulPoints[1] == y->_aulPoints[(i+2)%3] &&
                     x->_aulPoints[2] == y->_aulPoints[(i+1)%3])
                    return true;
            }
        }

        return false;
    }
};

bool MeshEvalDuplicateFacets::Evaluate()
{
  std::set<FaceIterator, MeshFacet_Less> aFaces;
  const MeshFacetArray& rFaces = _rclMesh.GetFacets();
  for (MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it)
  {
    std::pair<std::set<FaceIterator, MeshFacet_Less>::iterator, bool>
        pI = aFaces.insert(it);
    if (!pI.second)
        return false;
  }

  return true;
}

std::vector<unsigned long> MeshEvalDuplicateFacets::GetIndices() const
{
#if 1
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();
    std::vector<FaceIterator> faces;
    faces.reserve(rFacets.size());
    for (MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it) {
        faces.push_back(it);
    }

    // if there are two adjacent faces which references the same vertices
    std::vector<unsigned long> aInds;
    MeshFacet_EqualTo pred;
    std::sort(faces.begin(), faces.end(), MeshFacet_Less());

    std::vector<FaceIterator>::iterator ft = faces.begin();
    while (ft < faces.end()) {
        // get first item which adjacent element has the same face
        ft = std::adjacent_find(ft, faces.end(), pred);
        if (ft < faces.end()) {
            ft++;
            aInds.push_back(*ft - rFacets.begin());
        }
    }

    return aInds;
#else
  std::vector<unsigned long> aInds;
  const MeshFacetArray& rFaces = _rclMesh.GetFacets();
  unsigned long uIndex=0;

  // get all facets
  std::set<FaceIterator, MeshFacet_Less > aFaceSet;
  for (MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it, uIndex++)
  {
    std::pair<std::set<FaceIterator, MeshFacet_Less>::iterator, bool>
        pI = aFaceSet.insert(it);
    if (!pI.second)
      aInds.push_back(uIndex);
  }

  return aInds;
#endif
}

bool MeshFixDuplicateFacets::Fixup()
{
    unsigned long uIndex=0;
    std::vector<unsigned long> aRemoveFaces;
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();

    // get all facets
    std::set<FaceIterator, MeshFacet_Less > aFaceSet;
    for (MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it, uIndex++) {
        std::pair<std::set<FaceIterator, MeshFacet_Less>::iterator, bool>
        pI = aFaceSet.insert(it);
        if (!pI.second)
            aRemoveFaces.push_back(uIndex);
    }

    _rclMesh.DeleteFacets(aRemoveFaces);
    _rclMesh.RebuildNeighbours(); // needs to be done here

    return true;
}

// ----------------------------------------------------------------------

bool MeshEvalInternalFacets::Evaluate()
{
    _indices.clear();
    unsigned long uIndex=0;
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();

    // get all facets
    std::set<FaceIterator, MeshFacet_Less > aFaceSet;
    MeshFacetArray::_TConstIterator first = rFaces.begin();
    for (MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it, uIndex++) {
        std::pair<std::set<FaceIterator, MeshFacet_Less>::iterator, bool>
        pI = aFaceSet.insert(it);
        if (!pI.second) {
            // collect both elements
            _indices.push_back(*pI.first - first);
            _indices.push_back(uIndex);
        }
    }

    return _indices.empty();
}

// ----------------------------------------------------------------------

bool MeshEvalDegeneratedFacets::Evaluate()
{
  MeshFacetIterator it(_rclMesh);
  for ( it.Init(); it.More(); it.Next() )
  {
    if ( it->IsDegenerated() )
      return false;
  }

  return true;
}

unsigned long MeshEvalDegeneratedFacets::CountEdgeTooSmall (float fMinEdgeLength) const
{
  MeshFacetIterator  clFIter(_rclMesh);   
  unsigned long k = 0;

  while (clFIter.EndReached() == false)
  {
    for ( int i = 0; i < 3; i++)
    {
      if (Base::Distance(clFIter->_aclPoints[i], clFIter->_aclPoints[(i+1)%3]) < fMinEdgeLength)
        k++;
    }
    ++clFIter;
  }

  return k;
}

std::vector<unsigned long> MeshEvalDegeneratedFacets::GetIndices() const
{
  std::vector<unsigned long> aInds;
  MeshFacetIterator it(_rclMesh);
  for ( it.Init(); it.More(); it.Next() )
  {
    if ( it->IsDegenerated() )
      aInds.push_back(it.Position());
  }

  return aInds;
}

bool MeshFixDegeneratedFacets::Fixup()
{
  MeshTopoAlgorithm cTopAlg(_rclMesh);

  MeshFacetIterator it(_rclMesh);
  for ( it.Init(); it.More(); it.Next() )
  {
    if ( it->IsDegenerated() )
    {
      unsigned long uCt = _rclMesh.CountFacets();
      unsigned long uId = it.Position();
      cTopAlg.RemoveDegeneratedFacet(uId);
      if ( uCt != _rclMesh.CountFacets() )
      {
        // due to a modification of the array the iterator became invalid
        it.Set(uId-1);
      }
    }
  }

  return true;
}

unsigned long MeshFixDegeneratedFacets::RemoveEdgeTooSmall (float fMinEdgeLength, float fMinEdgeAngle)
{
    unsigned long ulCtLastLoop, ulCtFacets = _rclMesh.CountFacets();

    MeshFacetArray &rclFAry = _rclMesh._aclFacetArray;
    MeshPointArray &rclPAry = _rclMesh._aclPointArray;
    MeshFacetArray::_TConstIterator f_beg = rclFAry.begin();

    // repeat until no facet van be removed
    do {
        MeshRefPointToFacets  clPt2Facets(_rclMesh);

        rclFAry.ResetInvalid();
        rclPAry.ResetInvalid();
        rclPAry.ResetFlag(MeshPoint::VISIT);

        std::set<std::pair<unsigned long, unsigned long> > aclPtDelList;

        MeshFacetIterator clFIter(_rclMesh), clFN0(_rclMesh), clFN1(_rclMesh), clFN2(_rclMesh);
        for (clFIter.Init(); clFIter.More(); clFIter.Next()) {
            MeshGeomFacet clSFacet = *clFIter;
            Base::Vector3f clP0  = clSFacet._aclPoints[0];
            Base::Vector3f clP1  = clSFacet._aclPoints[1];
            Base::Vector3f clP2  = clSFacet._aclPoints[2];
            Base::Vector3f clE01 = clP1 - clP0;
            Base::Vector3f clE12 = clP2 - clP1;
            Base::Vector3f clE20 = clP2 - clP0;
            MeshFacet clFacet = clFIter.GetIndices();
            unsigned long    ulP0 = clFacet._aulPoints[0];
            unsigned long    ulP1 = clFacet._aulPoints[1];
            unsigned long    ulP2 = clFacet._aulPoints[2];

            if ((Base::Distance(clP0, clP1) < fMinEdgeLength) ||
                (clE20.GetAngle(-clE12) < fMinEdgeAngle)) {
                // delete point P1 on P0
                aclPtDelList.insert(std::make_pair
                    (std::min<unsigned long>(ulP1, ulP0), std::max<unsigned long>(ulP1, ulP0)));
            }
            else if ((Base::Distance(clP1, clP2) < fMinEdgeLength) ||
                    (clE01.GetAngle(-clE20) < fMinEdgeAngle)) {
                // delete point P2 on P1
                aclPtDelList.insert(std::make_pair
                    (std::min<unsigned long>(ulP2, ulP1), std::max<unsigned long>(ulP2, ulP1)));
            }
            else if ((Base::Distance(clP2, clP0) < fMinEdgeLength) ||
                    (clE12.GetAngle(-clE01) < fMinEdgeAngle)) {
                // delete point P0 on P2
                aclPtDelList.insert(std::make_pair
                    (std::min<unsigned long>(ulP0, ulP2), std::max<unsigned long>(ulP0, ulP2)));
            }
        }

        // remove points, fix indices
        for (std::set<std::pair<unsigned long, unsigned long> >::iterator pI = aclPtDelList.begin();
            pI != aclPtDelList.end(); pI++) {
            // one of the point pairs is already processed
            if ((rclPAry[pI->first].IsFlag(MeshPoint::VISIT) == true) ||
                (rclPAry[pI->second].IsFlag(MeshPoint::VISIT) == true))
                continue;

            rclPAry[pI->first].SetFlag(MeshPoint::VISIT);
            rclPAry[pI->second].SetFlag(MeshPoint::VISIT);
            rclPAry[pI->second].SetInvalid();

            // Redirect all point-indices to the new neighbour point of all facets referencing the
            // deleted point
            const std::set<unsigned long>& faces = clPt2Facets[pI->second];
            for (std::set<unsigned long>::const_iterator pF = faces.begin(); pF != faces.end(); ++pF) {
                const MeshFacet &rclF = f_beg[*pF];

                for (int i = 0; i < 3; i++) {
//                  if (rclF._aulPoints[i] == pI->second)
//                      rclF._aulPoints[i] = pI->first;
                }

                // Delete facets with two identical corners
                if ((rclF._aulPoints[0] == rclF._aulPoints[1]) ||
                    (rclF._aulPoints[0] == rclF._aulPoints[2]) ||
                    (rclF._aulPoints[1] == rclF._aulPoints[2])) {
                    rclF.SetInvalid();
                }
            }
        }

        ulCtLastLoop = _rclMesh.CountFacets();
        _rclMesh.RemoveInvalids();
    }
    while (ulCtLastLoop > _rclMesh.CountFacets());

    _rclMesh.RebuildNeighbours();

    return ulCtFacets - _rclMesh.CountFacets();
}

// ----------------------------------------------------------------------

bool MeshEvalDeformedFacets::Evaluate()
{
  MeshFacetIterator it(_rclMesh);
  for ( it.Init(); it.More(); it.Next() )
  {
    if ( it->IsDeformed() )
      return false;
  }

  return true;
}

std::vector<unsigned long> MeshEvalDeformedFacets::GetIndices() const
{
  std::vector<unsigned long> aInds;
  MeshFacetIterator it(_rclMesh);
  for ( it.Init(); it.More(); it.Next() )
  {
    if ( it->IsDeformed() )
      aInds.push_back(it.Position());
  }

  return aInds;
}

bool MeshFixDeformedFacets::Fixup()
{
  Base::Vector3f u,v;
  MeshTopoAlgorithm cTopAlg(_rclMesh);

  MeshFacetIterator it(_rclMesh);
  for ( it.Init(); it.More(); it.Next() )
  {
    // possibly deformed but not degenerated
    if ( !it->IsDegenerated() )
    {
      // store the angles to avoid to compute twice
      float fCosAngles[3];
      bool done=false;

      // first check for angle > 120�: in this case we swap with the opposite edge
      for (int i=0; i<3; i++)
      {
        u = it->_aclPoints[(i+1)%3]-it->_aclPoints[i];
        v = it->_aclPoints[(i+2)%3]-it->_aclPoints[i];
        u.Normalize();
        v.Normalize();

        float fCosAngle = u * v;
        fCosAngles[i] = fCosAngle;

        if (fCosAngle < -0.5f) {
          const MeshFacet& face = it.GetReference();
          unsigned long uNeighbour = face._aulNeighbours[(i+1)%3];
          if (uNeighbour!=ULONG_MAX && cTopAlg.ShouldSwapEdge(it.Position(), uNeighbour, fMaxAngle)) {
            cTopAlg.SwapEdge(it.Position(), uNeighbour);
            done = true;
          }
          break;
        }
      }

      // we have swapped already
      if (done)
        continue;

      // now check for angle < 30�: in this case we swap with one of the edges the corner is part of
      for (int j=0; j<3; j++)
      {
        float fCosAngle = fCosAngles[j];
        if (fCosAngle > 0.86f) {
          const MeshFacet& face = it.GetReference();

          unsigned long uNeighbour = face._aulNeighbours[j];
          if (uNeighbour!=ULONG_MAX && cTopAlg.ShouldSwapEdge(it.Position(), uNeighbour, fMaxAngle)) {
            cTopAlg.SwapEdge(it.Position(), uNeighbour);
            break;
          }
          uNeighbour = face._aulNeighbours[(j+2)%3];
          if (uNeighbour!=ULONG_MAX && cTopAlg.ShouldSwapEdge(it.Position(), uNeighbour, fMaxAngle)) {
            cTopAlg.SwapEdge(it.Position(), uNeighbour);
            break;
          }
        }
      }
    }
  }

  return true;
}

// ----------------------------------------------------------------------

bool MeshEvalFoldsOnSurface::Evaluate()
{
    this->indices.clear();
    MeshRefPointToFacets  clPt2Facets(_rclMesh);
    const MeshPointArray& rPntAry = _rclMesh.GetPoints();
    MeshFacetArray::_TConstIterator f_beg = _rclMesh.GetFacets().begin();

    MeshGeomFacet rTriangle;
    Base::Vector3f tmp;
    unsigned long ctPoints = _rclMesh.CountPoints();
    for (unsigned long index=0; index < ctPoints; index++) {
        std::vector<unsigned long> point;
        point.push_back(index);

        // get the local neighbourhood of the point
        std::set<unsigned long> nb = clPt2Facets.NeighbourPoints(point,1);
        const std::set<unsigned long>& faces = clPt2Facets[index];

        for (std::set<unsigned long>::iterator pt = nb.begin(); pt != nb.end(); ++pt) {
            const MeshPoint& mp = rPntAry[*pt];
            for (std::set<unsigned long>::const_iterator
                ft = faces.begin(); ft != faces.end(); ++ft) {
                    // the point must not be part of the facet we test
                    if (f_beg[*ft]._aulPoints[0] == *pt)
                        continue;
                    if (f_beg[*ft]._aulPoints[1] == *pt)
                        continue;
                    if (f_beg[*ft]._aulPoints[2] == *pt)
                        continue;
                    // is the point projectable onto the facet?
                    rTriangle = _rclMesh.GetFacet(f_beg[*ft]);
                    if (rTriangle.IntersectWithLine(mp,rTriangle.GetNormal(),tmp)) {
                        const std::set<unsigned long>& f = clPt2Facets[*pt];
                        this->indices.insert(this->indices.end(), f.begin(), f.end());
                        break;
                    }
            }
        }
    }

    // remove duplicates
    std::sort(this->indices.begin(), this->indices.end());
    this->indices.erase(std::unique(this->indices.begin(),
                        this->indices.end()), this->indices.end());

    return this->indices.empty();
}

std::vector<unsigned long> MeshEvalFoldsOnSurface::GetIndices() const
{
    return this->indices;
}

bool MeshFixFoldsOnSurface::Fixup()
{
    MeshEvalFoldsOnSurface eval(_rclMesh);
    if (!eval.Evaluate()) {
        std::vector<unsigned long> inds = eval.GetIndices();
        _rclMesh.DeleteFacets(inds);
    }

    return true;
}

// ----------------------------------------------------------------------

bool MeshEvalFoldsOnBoundary::Evaluate()
{
    // remove all boundary facets with two open edges and where
    // the angle to the neighbour is more than 60 degree
    this->indices.clear();
    const MeshFacetArray& rFacAry = _rclMesh.GetFacets();
    for (MeshFacetArray::_TConstIterator it = rFacAry.begin(); it != rFacAry.end(); ++it) {
        if (it->CountOpenEdges() == 2) {
            for (int i=0; i<3; i++) {
                if (it->_aulNeighbours[i] != ULONG_MAX) {
                    MeshGeomFacet f1 = _rclMesh.GetFacet(*it);
                    MeshGeomFacet f2 = _rclMesh.GetFacet(it->_aulNeighbours[i]);
                    float cos_angle = f1.GetNormal() * f2.GetNormal();
                    if (cos_angle <= 0.5f) // ~ 60 degree
                        indices.push_back(it-rFacAry.begin());
                }
            }
        }
    }

    return this->indices.empty();
}

std::vector<unsigned long> MeshEvalFoldsOnBoundary::GetIndices() const
{
    return this->indices;
}

bool MeshFixFoldsOnBoundary::Fixup()
{
    MeshEvalFoldsOnBoundary eval(_rclMesh);
    if (!eval.Evaluate()) {
        std::vector<unsigned long> inds = eval.GetIndices();
        _rclMesh.DeleteFacets(inds);
    }

    return true;
}

// ----------------------------------------------------------------------

bool MeshEvalFoldOversOnSurface::Evaluate()
{
    this->indices.clear();
    const MeshCore::MeshFacetArray& facets = _rclMesh.GetFacets();
    MeshCore::MeshFacetArray::_TConstIterator f_it,
        f_beg = facets.begin(), f_end = facets.end();

    Base::Vector3f n1, n2;
    for (f_it = facets.begin(); f_it != f_end; ++f_it) {
        for (int i=0; i<3; i++) {
            unsigned long index1 = f_it->_aulNeighbours[i];
            unsigned long index2 = f_it->_aulNeighbours[(i+1)%3];
            if (index1 != ULONG_MAX && index2 != ULONG_MAX) {
                // if the topology is correct but the normals flip from
                // two neighbours we have a fold
                if (f_it->HasSameOrientation(f_beg[index1]) &&
                    f_it->HasSameOrientation(f_beg[index2])) {
                    n1 = _rclMesh.GetFacet(index1).GetNormal();
                    n2 = _rclMesh.GetFacet(index2).GetNormal();
                    if (n1 * n2 < -0.5f) { // angle > 120 deg
                        this->indices.push_back(f_it-f_beg);
                        break;
                    }
                }
            }
        }
    }

    return this->indices.empty();
}

// ----------------------------------------------------------------

bool MeshEvalBorderFacet::Evaluate()
{
    const MeshCore::MeshFacetArray& facets = _rclMesh.GetFacets();
    MeshCore::MeshFacetArray::_TConstIterator f_it,
        f_beg = facets.begin(), f_end = facets.end();
    MeshCore::MeshRefPointToPoints vv_it(_rclMesh);
    MeshCore::MeshRefPointToFacets vf_it(_rclMesh);

    for (f_it = facets.begin(); f_it != f_end; ++f_it) {
        bool ok = true;
        for (int i=0; i<3; i++) {
            unsigned long index = f_it->_aulPoints[i];
            if (vv_it[index].size() == vf_it[index].size()) {
                ok = false;
                break;
            }
        }

        if (ok)
            _facets.push_back(f_it-f_beg);
    }

    return _facets.empty();
}

// ----------------------------------------------------------------------

bool MeshEvalRangeFacet::Evaluate()
{
  const MeshFacetArray& rFaces = _rclMesh.GetFacets();
  unsigned long ulCtFacets = rFaces.size();

  for ( MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it ) {
    for ( int i = 0; i < 3; i++ ) {
      if ((it->_aulNeighbours[i] >= ulCtFacets) && (it->_aulNeighbours[i] < ULONG_MAX)) {
        return false;
      }
    }
  }

  return true;
}

std::vector<unsigned long> MeshEvalRangeFacet::GetIndices() const
{
  std::vector<unsigned long> aInds;
  const MeshFacetArray& rFaces = _rclMesh.GetFacets();
  unsigned long ulCtFacets = rFaces.size();

  unsigned long ind=0;
  for ( MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it, ind++ )
  {
    for ( int i = 0; i < 3; i++ ) {
      if ((it->_aulNeighbours[i] >= ulCtFacets) && (it->_aulNeighbours[i] < ULONG_MAX)) {
        aInds.push_back(ind);
        break;
      }
    }
  }

  return aInds;
}

bool MeshFixRangeFacet::Fixup()
{
  return false;
}

// ----------------------------------------------------------------------

bool MeshEvalRangePoint::Evaluate()
{
  const MeshFacetArray& rFaces = _rclMesh.GetFacets();
  unsigned long ulCtPoints = _rclMesh.CountPoints();

  for ( MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it ) {
    if (std::find_if(it->_aulPoints, it->_aulPoints + 3, std::bind2nd(std::greater_equal<unsigned long>(), ulCtPoints)) < it->_aulPoints + 3)
      return false;
  }

  return true;
}

std::vector<unsigned long> MeshEvalRangePoint::GetIndices() const
{
  std::vector<unsigned long> aInds;
  const MeshFacetArray& rFaces = _rclMesh.GetFacets();
  unsigned long ulCtPoints = _rclMesh.CountPoints();

  unsigned long ind=0;
  for ( MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it, ind++ )
  {
    if (std::find_if(it->_aulPoints, it->_aulPoints + 3, std::bind2nd(std::greater_equal<unsigned long>(), ulCtPoints)) < it->_aulPoints + 3)
      aInds.push_back(ind);
  }

  return aInds;
}

bool MeshFixRangePoint::Fixup()
{
  return false;
}

// ----------------------------------------------------------------------

bool MeshEvalCorruptedFacets::Evaluate()
{
  const MeshFacetArray& rFaces = _rclMesh.GetFacets();

  for ( MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it ) {
    // dupicated point indices
    if ((it->_aulPoints[0] == it->_aulPoints[1]) || 
        (it->_aulPoints[1] == it->_aulPoints[2]) || 
        (it->_aulPoints[2] == it->_aulPoints[0]))
      return false;
  }

  return true;
}

std::vector<unsigned long> MeshEvalCorruptedFacets::GetIndices() const
{
  std::vector<unsigned long> aInds;
  const MeshFacetArray& rFaces = _rclMesh.GetFacets();
  unsigned long ind=0;

  for ( MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it, ind++ ) {
    if ((it->_aulPoints[0] == it->_aulPoints[1]) || 
        (it->_aulPoints[1] == it->_aulPoints[2]) || 
        (it->_aulPoints[2] == it->_aulPoints[0]))
      aInds.push_back(ind);
  }

  return aInds;
}

bool MeshFixCorruptedFacets::Fixup()
{
  MeshTopoAlgorithm cTopAlg(_rclMesh);

  MeshFacetIterator it(_rclMesh);
  for ( it.Init(); it.More(); it.Next() )
  {
    if ( it->Area() <= FLOAT_EPS )
    {
      unsigned long uId = it.Position();
      cTopAlg.RemoveCorruptedFacet(uId);
      // due to a modification of the array the iterator became invalid
      it.Set(uId-1);
    }
  }

  return true;
}

