// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <functional>
#include <limits>

#include <Mod/Mesh/App/WildMagic4/Wm4DistVector3Triangle3.h>
#include <Mod/Mesh/App/WildMagic4/Wm4Sphere3.h>

#include "Algorithm.h"
#include "Iterator.h"
#include "MeshKernel.h"


namespace MeshCore
{

/**
 * The MeshSearchNeighbours class provides methods to get all points
 * in the neighbourhood of a given facet.
 */
class MeshSearchNeighbours
{
public:
    explicit MeshSearchNeighbours(const MeshKernel& rclM, float fSampleDistance = 1.0F);
    ~MeshSearchNeighbours() = default;
    /** Re-initilaizes internal structures. */
    void Reinit(float fSampleDistance);
    /** Collects all neighbour points from the facet (by index), the result are the points of the
     * facets lying inside a sphere of radius \a fDistance, center \a center of the original facet.
     * This method uses the MARKED flags.
     */
    unsigned long NeighboursFromFacet(
        FacetIndex ulFacetIdx,
        float fDistance,
        unsigned long ulMinPoints,
        std::vector<Base::Vector3f>& raclResultPoints
    );
    /** Searches for facets from the start facet, sample the neighbour facets and accumulates the
     * points. */
    unsigned long NeighboursFromSampledFacets(
        FacetIndex ulFacetIdx,
        float fDistance,
        std::vector<Base::Vector3f>& raclResultPoints
    );
    /** Searches for facets from the start facet. */
    unsigned long NeighboursFacetFromFacet(
        FacetIndex ulFacetIdx,
        float fDistance,
        std::vector<Base::Vector3f>& raclResultPoints,
        std::vector<FacetIndex>& raclResultFacets
    );

protected:
    /** Subsamples the mesh. */
    void SampleAllFacets();
    inline bool CheckDistToFacet(const MeshFacet& rclF);  // check distance to facet, add points
                                                          // inner radius
    bool AccumulateNeighbours(
        const MeshFacet& rclF,
        FacetIndex ulFIdx
    );  // accumulate the sample neighbours facet
    inline bool InnerPoint(const Base::Vector3f& rclPt) const;
    inline bool TriangleCutsSphere(const MeshFacet& rclF) const;
    bool ExpandRadius(unsigned long ulMinPoints);

    struct CDistRad
    {
        explicit CDistRad(const Base::Vector3f clCenter)
            : _clCenter(clCenter)
        {}
        bool operator()(const Base::Vector3f& rclPt1, const Base::Vector3f& rclPt2)
        {
            return Base::DistanceP2(_clCenter, rclPt1) < Base::DistanceP2(_clCenter, rclPt2);
        }
        Base::Vector3f _clCenter;
    };

private:
    const MeshKernel& _rclMesh;
    const MeshFacetArray& _rclFAry;
    const MeshPointArray& _rclPAry;
    MeshRefPointToFacets _clPt2Fa;
    float _fMaxDistanceP2 {0};                                   // square distance
    Base::Vector3f _clCenter;                                    // center points of start facet
    std::set<PointIndex> _aclResult;                             // result container (point indices)
    std::set<PointIndex> _aclOuter;                              // next searching points
    std::vector<Base::Vector3f> _aclPointsResult;                // result as vertex
    std::vector<std::vector<Base::Vector3f>> _aclSampledFacets;  // sample points from each facet
    float _fSampleDistance;  // distance between two sampled points
    Wm4::Sphere3<float> _akSphere;

public:
    MeshSearchNeighbours(const MeshSearchNeighbours&) = delete;
    MeshSearchNeighbours(MeshSearchNeighbours&&) = delete;
    void operator=(const MeshSearchNeighbours&) = delete;
    void operator=(MeshSearchNeighbours&&) = delete;
};

inline bool MeshSearchNeighbours::CheckDistToFacet(const MeshFacet& rclF)
{
    bool bFound = false;

    for (PointIndex ulPIdx : rclF._aulPoints) {
        if (!_rclPAry[ulPIdx].IsFlag(MeshPoint::MARKED)) {
            if (Base::DistanceP2(_clCenter, _rclPAry[ulPIdx]) < _fMaxDistanceP2) {
                bFound = true;
                {
                    _aclResult.insert(ulPIdx);
                    _rclPAry[ulPIdx].SetFlag(MeshPoint::MARKED);
                }
            }
            _aclOuter.insert(ulPIdx);
        }
    }

    return bFound;
}

inline bool MeshSearchNeighbours::InnerPoint(const Base::Vector3f& rclPt) const
{
    return Base::DistanceP2(_clCenter, rclPt) < _fMaxDistanceP2;
}

inline bool MeshSearchNeighbours::TriangleCutsSphere(const MeshFacet& rclF) const
{
    Base::Vector3f cP0 = _rclPAry[rclF._aulPoints[0]];
    Base::Vector3f cP1 = _rclPAry[rclF._aulPoints[1]];
    Base::Vector3f cP2 = _rclPAry[rclF._aulPoints[2]];

    Wm4::Vector3<float> akP0(cP0.x, cP0.y, cP0.z);
    Wm4::Vector3<float> akP1(cP1.x, cP1.y, cP1.z);
    Wm4::Vector3<float> akP2(cP2.x, cP2.y, cP2.z);

    Wm4::Triangle3<float> akTri(akP0, akP1, akP2);
    Wm4::DistVector3Triangle3<float> akDistVecTri(_akSphere.Center, akTri);

    float fSqrDist = akDistVecTri.GetSquared();
    float fRSqr = _akSphere.Radius * _akSphere.Radius;
    return fSqrDist < fRSqr;
}

class MeshFaceIterator
{
public:
    explicit MeshFaceIterator(const MeshKernel& mesh)
        : it(mesh)
    {}
    Base::Vector3f operator()(FacetIndex index)
    {
        it.Set(index);
        return it->GetGravityPoint();
    }

private:
    MeshFacetIterator it;
};

class MeshVertexIterator
{
public:
    explicit MeshVertexIterator(const MeshKernel& mesh)
        : it(mesh)
    {}
    Base::Vector3f operator()(PointIndex index)
    {
        it.Set(index);
        return Base::Vector3f(it->x, it->y, it->z);
    }

private:
    MeshPointIterator it;
};

template<class T>
class MeshNearestIndexToPlane
{
public:
    using Index = typename T::Index;
    MeshNearestIndexToPlane(const MeshKernel& mesh, const Base::Vector3f& b, const Base::Vector3f& n)
        : nearest_index(-1)
        , it(mesh)
        , base(b)
        , normal(n)
    {}
    void operator()(Index index)
    {
        float dist = (float)fabs(it(index).DistanceToPlane(base, normal));
        if (dist < nearest_dist) {
            nearest_dist = dist;
            nearest_index = index;
        }
    }

    // NOLINTBEGIN
    Index nearest_index;
    float nearest_dist {std::numeric_limits<float>::max()};
    // NOLINTEND

private:
    T it;
    Base::Vector3f base, normal;
};

}  // namespace MeshCore
