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

#ifndef VISITOR_H
#define VISITOR_H

#include "MeshKernel.h"
#include <vector>


namespace MeshCore
{

class MeshFacet;
class MeshKernel;
class MeshFacetVisitor;
class MeshPoint;
class PlaneFit;

/**
 * Abstract base class for facet visitors.
 * The MeshFacetVisitor class can be used for the so called
 * "Region growing" algorithms.
 */
class MeshExport MeshFacetVisitor
{
public:
    /// Construction
    MeshFacetVisitor() = default;
    /// Denstruction
    virtual ~MeshFacetVisitor() = default;
    /** Needs to be implemented in sub-classes.
     * \a rclFacet is the currently visited facet with the index \a ulFInd, \a rclFrom
     * is the last visited facet and \a ulLevel indicates the ring number around the start facet.
     * If \a true is returned the next iteration is done if there are still facets to visit.
     * If \a false is returned the calling method stops immediately visiting further facets.
     */
    virtual bool Visit(const MeshFacet& rclFacet,
                       const MeshFacet& rclFrom,
                       FacetIndex ulFInd,
                       unsigned long ulLevel) = 0;

    /** Test before a facet will be flagged as VISIT, return false means: go on with
     * visiting the facets but not this one and set not the VISIT flag
     */
    virtual bool AllowVisit(const MeshFacet& rclFacet,
                            const MeshFacet& rclFrom,
                            FacetIndex ulFInd,
                            unsigned long ulLevel,
                            unsigned short neighbourIndex)
    {
        (void)rclFacet;
        (void)rclFrom;
        (void)ulFInd;
        (void)ulLevel;
        (void)neighbourIndex;
        return true;
    }
};

/**
 * Special mesh visitor that searches for facets within a given search radius.
 */
class MeshExport MeshSearchNeighbourFacetsVisitor: public MeshFacetVisitor
{
public:
    MeshSearchNeighbourFacetsVisitor(const MeshKernel& rclMesh,
                                     float fRadius,
                                     FacetIndex ulStartFacetIdx);
    ~MeshSearchNeighbourFacetsVisitor() override = default;
    /** Checks the facet if it lies inside the search radius. */
    inline bool Visit(const MeshFacet& rclFacet,
                      const MeshFacet& rclFrom,
                      FacetIndex ulFInd,
                      unsigned long ulLevel) override;
    /** Resets the VISIT flag of already visited facets. */
    inline std::vector<FacetIndex> GetAndReset();

protected:
    const MeshKernel& _rclMeshBase; /**< The mesh kernel. */
    Base::Vector3f _clCenter;       /**< Center. */
    float _fRadius;                 /**< Search radius. */
    unsigned long _ulCurrentLevel {0};
    bool _bFacetsFoundInCurrentLevel {false};
    std::vector<FacetIndex> _vecFacets; /**< Found facets. */
};

inline bool MeshSearchNeighbourFacetsVisitor::Visit(const MeshFacet& rclFacet,
                                                    const MeshFacet& rclFrom,
                                                    FacetIndex ulFInd,
                                                    unsigned long ulLevel)
{
    (void)rclFrom;
    if (ulLevel > _ulCurrentLevel) {
        if (!_bFacetsFoundInCurrentLevel) {
            return false;
        }
        _ulCurrentLevel = ulLevel;
        _bFacetsFoundInCurrentLevel = false;
    }

    for (PointIndex ptIndex : rclFacet._aulPoints) {
        if (Base::Distance(_clCenter, _rclMeshBase.GetPoint(ptIndex)) < _fRadius) {
            _vecFacets.push_back(ulFInd);
            _bFacetsFoundInCurrentLevel = true;
            return true;
        }
    }

    return true;
}

/**
 * The MeshTopFacetVisitor just collects the indices of all visited facets.
 */
class MeshExport MeshTopFacetVisitor: public MeshFacetVisitor
{
public:
    MeshTopFacetVisitor(std::vector<FacetIndex>& raulNB)
        : _raulNeighbours(raulNB)
    {}
    ~MeshTopFacetVisitor() override = default;
    /** Collects the facet indices. */
    bool Visit(const MeshFacet& rclFacet,
               const MeshFacet& rclFrom,
               FacetIndex ulFInd,
               unsigned long) override
    {
        (void)rclFacet;
        (void)rclFrom;
        _raulNeighbours.push_back(ulFInd);
        return true;
    }

protected:
    std::vector<FacetIndex>& _raulNeighbours; /**< Indices of all visited facets. */
};

// -------------------------------------------------------------------------

/**
 * The MeshPlaneVisitor collects all facets the are co-planar to the plane defined
 * by the start triangle.
 */
class MeshPlaneVisitor: public MeshFacetVisitor
{
public:
    MeshPlaneVisitor(const MeshKernel& mesh,
                     FacetIndex index,
                     float deviation,
                     std::vector<FacetIndex>& indices);
    ~MeshPlaneVisitor() override;
    bool AllowVisit(const MeshFacet& face,
                    const MeshFacet&,
                    FacetIndex,
                    unsigned long,
                    unsigned short neighbourIndex) override;
    bool Visit(const MeshFacet& face, const MeshFacet&, FacetIndex ulFInd, unsigned long) override;

protected:
    const MeshKernel& mesh;
    std::vector<FacetIndex>& indices;
    Base::Vector3f basepoint;
    Base::Vector3f normal;
    float max_deviation;
    PlaneFit* fitter;
};

// -------------------------------------------------------------------------

/**
 * Abstract base class for point visitors.
 */
class MeshExport MeshPointVisitor
{
public:
    /// Construction
    MeshPointVisitor() = default;
    /// Denstruction
    virtual ~MeshPointVisitor() = default;
    /** Needs to be implemented in sub-classes.
     * \a rclPoint is the currently visited point with the index \a ulPInd, \a rclFrom
     * is the last visited point  and \a ulLevel indicates the ring number around the start point.
     * If \a true is returned the next iteration is done if there are still point to visit. If
     * \a false is returned the calling method stops immediately visiting further points.
     */
    virtual bool Visit(const MeshPoint& rclPoint,
                       const MeshPoint& rclFrom,
                       FacetIndex ulPInd,
                       unsigned long ulLevel) = 0;
};

}  // namespace MeshCore

#endif  // VISITOR_H
