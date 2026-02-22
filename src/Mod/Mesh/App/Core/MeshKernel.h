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

#include <cassert>
#include <iosfwd>

#include <Base/BoundBox.h>
#include <Base/Matrix.h>

#include "Helpers.h"


namespace Base
{
class Polygon2d;
class ViewProjMethod;
}  // namespace Base

namespace MeshCore
{

// forward declarations
class MeshFacetIterator;
class MeshPointIterator;
class MeshGeomFacet;
class MeshFacet;
class MeshFacetVisitor;
class MeshPointVisitor;
class MeshFacetGrid;


/**
 * The MeshKernel class is the basic class that holds the data points,
 * the edges and the facets describing a mesh object.
 *
 * The bounding box is calculated during the buildup of the data
 * structure and gets only re-caclulated after insertion of new facets
 * but not after removal of facets.
 *
 * This class provides only some rudimental querying methods.
 */
class MeshExport MeshKernel
{
public:
    /// Construction
    MeshKernel();
    /// Construction
    MeshKernel(const MeshKernel& rclMesh);
    MeshKernel(MeshKernel&& rclMesh);
    /// Destruction
    ~MeshKernel()
    {
        Clear();
    }

    /** @name I/O methods */
    //@{
    /// Binary streaming of data
    void Write(std::ostream& rclOut) const;
    void Read(std::istream& rclIn);
    //@}

    /** @name Querying */
    //@{
    /// Returns the number of facets
    unsigned long CountFacets() const
    {
        return static_cast<unsigned long>(_aclFacetArray.size());
    }
    /// Returns the number of edge
    unsigned long CountEdges() const;
    // Returns the number of points
    unsigned long CountPoints() const
    {
        return static_cast<unsigned long>(_aclPointArray.size());
    }
    /// Returns the number of required memory in bytes
    unsigned int GetMemSize() const
    {
        return static_cast<unsigned int>(
            _aclPointArray.size() * sizeof(MeshPoint) + _aclFacetArray.size() * sizeof(MeshFacet)
        );
    }
    /// Determines the bounding box
    const Base::BoundBox3f& GetBoundBox() const
    {
        return _clBoundBox;
    }

    /** Forces a recalculation of the bounding box. This method should be called after
     * the removal of points.or after a transformation of the data structure.
     */
    void RecalcBoundBox() const;

    /** Returns the point at the given index. This method is rather slow and should be
     * called occasionally only. For fast access the MeshPointIterator interfsce should
     * be used.
     */
    inline MeshPoint GetPoint(PointIndex ulIndex) const;

    /** Returns an array of the vertex normals of the mesh. A vertex normal gets calculated
     * by summarizing the normals of the associated facets.
     */
    std::vector<Base::Vector3f> CalcVertexNormals() const;
    std::vector<Base::Vector3f> GetFacetNormals(const std::vector<FacetIndex>&) const;

    /** Returns the facet at the given index. This method is rather slow and should be
     * called occasionally only. For fast access the MeshFacetIterator interface should
     * be used.
     */
    inline MeshGeomFacet GetFacet(FacetIndex ulIndex) const;
    inline MeshGeomFacet GetFacet(const MeshFacet& rclFacet) const;

    /** Returns the point indices of the given facet index. */
    inline void GetFacetPoints(
        FacetIndex ulFaIndex,
        PointIndex& rclP0,
        PointIndex& rclP1,
        PointIndex& rclP2
    ) const;
    /** Returns the point indices of the given facet index. */
    inline void SetFacetPoints(FacetIndex ulFaIndex, PointIndex rclP0, PointIndex rclP1, PointIndex rclP2);
    /** Returns the point indices of the given facet indices. */
    std::vector<PointIndex> GetFacetPoints(const std::vector<FacetIndex>&) const;
    /** Returns the facet indices that share the given point indices. */
    std::vector<FacetIndex> GetPointFacets(const std::vector<PointIndex>&) const;
    /** Returns the indices of the neighbour facets of the given facet index. */
    inline void GetFacetNeighbours(
        FacetIndex ulIndex,
        FacetIndex& rulNIdx0,
        FacetIndex& rulNIdx1,
        FacetIndex& rulNIdx2
    ) const;

    /** Determines all facets that are associated to this point. This method is very
     * slow and should be called occasionally only.
     */
    std::vector<FacetIndex> HasFacets(const MeshPointIterator& rclIter) const;

    /** Returns true if the data structure is valid. */
    bool IsValid() const
    {
        return _bValid;
    }

    /** Returns the array of all data points. */
    const MeshPointArray& GetPoints() const
    {
        return _aclPointArray;
    }
    /** Returns an array of points to the given indices. The indices
     * must not be out of range.
     */
    MeshPointArray GetPoints(const std::vector<PointIndex>&) const;

    /** Returns a modifier for the point array */
    MeshPointModifier ModifyPoints()
    {
        return MeshPointModifier(_aclPointArray);
    }

    /** Returns the array of all facets */
    const MeshFacetArray& GetFacets() const
    {
        return _aclFacetArray;
    }
    /** Returns an array of facets to the given indices. The indices
     * must not be out of range.
     */
    MeshFacetArray GetFacets(const std::vector<FacetIndex>&) const;

    /** Returns a modifier for the facet array */
    MeshFacetModifier ModifyFacets()
    {
        return MeshFacetModifier(_aclFacetArray);
    }

    /** Returns the array of all edges.
     *  Notice: The Edgelist will be temporary generated. Changes on the mesh
     * structure does not affect the Edgelist
     */
    void GetEdges(std::vector<MeshGeomEdge>&) const;
    //@}

    /** @name Evaluation */
    //@{
    /** Calculates the surface area of the mesh object. */
    float GetSurface() const;
    /** Calculates the surface area of the segment defined by \a aSegment. */
    float GetSurface(const std::vector<FacetIndex>& aSegment) const;
    /** Calculates the volume of the mesh object. Therefore the mesh must be a solid, if not 0
     * is returned.
     */
    float GetVolume() const;
    /** Checks whether the mesh has open edges. */
    bool HasOpenEdges() const;
    /** Checks whether the mesh has non.manifold edges. An edge is regarded as non-manifolds if it
     * shares more than two facets.
     */
    bool HasNonManifolds() const;
    /** Checks whether the mesh intersects itself. */
    bool HasSelfIntersections() const;
    //@}

    /** @name Facet visitors
     * The MeshKernel class provides different methods to visit "topologic connected" facets
     * to a given start facet. Two facets are regarded as "topologic connected" if they share
     * a common edge or a common point.
     * All methods expect a MeshFacetVisitor as argument that can decide to continue or to stop.
     * If there is no topologic neighbour facet any more being not marked as "VISIT" the algorithm
     * stops anyway.
     * @see MeshFacetVisitor, MeshOrientationVisitor, MeshSearchNeighbourFacetsVisitor
     * and MeshTopFacetVisitor.
     */
    //@{
    /**
     * This method visits all neighbour facets, i.e facets that share a common edge
     * starting from the facet associated to index \a ulStartFacet. All facets having set the VISIT
     * flag are ignored. Therefore the user have to set or unset this flag if needed.
     * All facets that get visited during this algorithm are marked as VISIT and the Visit() method
     * of the given MeshFacetVisitor gets invoked.
     * If there are no unvisited neighbours any more the algorithms returns immediately and returns
     * the number of visited facets.
     * \note For the start facet \a ulStartFacet MeshFacetVisitor::Visit() does not get invoked
     * though the facet gets marked as VISIT.
     */
    unsigned long VisitNeighbourFacets(MeshFacetVisitor& rclFVisitor, FacetIndex ulStartFacet) const;
    /**
     * Does basically the same as the method above unless the facets that share just a common point
     * are regared as neighbours.
     */
    unsigned long VisitNeighbourFacetsOverCorners(
        MeshFacetVisitor& rclFVisitor,
        FacetIndex ulStartFacet
    ) const;
    //@}

    /** @name Point visitors
     * The MeshKernel class provides a method to visit neighbour points to a given start point.
     * Two points are regarded as neighbours if they share an edge.
     * The method expects a MeshPointVisitor as argument that can decide to continue or to stop.
     * If there is no topologic neighbour point any more being not marked as "VISIT" the algorithm
     * stops anyway.
     */
    //@{
    /**
     * This method visits all neighbour points starting from the point associated to index \a
     * ulStartPoint. All points having set the VISIT flag are ignored. Therefore the user have to
     * set or unset this flag if needed before the algorithm starts. All points that get visited
     * during this algorithm are marked as VISIT and the Visit() method of the given
     * MeshPointVisitor gets invoked. If there are no unvisited neighbours any more the algorithms
     * returns immediately and returns the number of visited points. \note For the start facet \a
     * ulStartPoint MeshPointVisitor::Visit() does not get invoked though the point gets marked as
     * VISIT.
     */
    unsigned long VisitNeighbourPoints(MeshPointVisitor& rclPVisitor, PointIndex ulStartPoint) const;
    //@}

    /** @name Iterators
     * The iterator methods are provided for convenience. They return an iterator object that
     * points to the first element in the appropriate list.
     * \code
     * MeshKernel mesh = ...
     * // iterate over all facets
     * for ( MeshFacetIterator it = mesh.FacetIterator(); it.More(); it.Next() )
     * ...
     * \endcode
     * An iterator can also be used in the following way
     * \code
     * MeshKernel mesh = ...
     * // iterate over all facets
     * MeshFacetIterator it(mesh);
     * for (  it.Init(); it.More(); it.Next() )
     * ...
     * \endcode
     */
    //@{
    /** Returns an iterator object to go over all facets. */
    MeshFacetIterator FacetIterator() const;
    /** Returns an iterator object to go over all points. */
    MeshPointIterator PointIterator() const;
    //@}

    /** @name Modification */
    //@{
    /** Adds a single facet to the data structure. This method is very slow and should
     * be called occasionally only.
     */
    MeshKernel& operator+=(const MeshGeomFacet& rclSFacet);
    /** Adds a single facet to the data structure. This method is very slow and should
     * be called occasionally only. This does the same as the += operator above.
     */
    void AddFacet(const MeshGeomFacet& rclSFacet);
    /** Adds an array of facets to the data structure. This method keeps temporarily
     * set properties and flags.
     */
    MeshKernel& operator+=(const std::vector<MeshGeomFacet>& rclFAry);
    /** Adds an array of facets to the data structure. This method keeps temporarily
     * set properties and flags. This does the same as the += operator above.
     */
    void AddFacets(const std::vector<MeshGeomFacet>& rclFAry);
    /**
     * Adds an array of topologic facets to the data structure without inserting new points.
     * Facets which would create non-manifolds are not inserted.
     * The client programmer must make sure that the referenced point indices are correct and that
     * no geometric overlaps can be created. The method returns the total number of facets.
     * This method might be useful to close gaps or fill up holes in a mesh.
     * @note This method is quite expensive and should be rarely used.
     */
    unsigned long AddFacets(const std::vector<MeshFacet>& rclFAry, bool checkManifolds);
    /**
     * Adds new points and facets to the data structure. The client programmer must make sure
     * that all new points are referenced by the new facets.
     * All points in \a rclPAry get copied at the end of the internal point array to keep their
     * order. The point indices of the facets must be related to the internal point array, not the
     * passed array \a rclPAry.
     *
     * Example:
     * We have a mesh with p points and f facets where we want append new points and facets to.
     * Let's assume that the first facet of \a rclFAry references the 1st, 2nd and 3rd points
     * of \a rclPAry then its indices must be p, p+1, p+2 -- not 0,1,2. This is due to the fact
     * that facets of \a rclFAry can also reference point indices of the internal point array.
     * @note This method is quite expensive and should be rarely used.
     */
    unsigned long AddFacets(
        const std::vector<MeshFacet>& rclFAry,
        const std::vector<Base::Vector3f>& rclPAry,
        bool checkManifolds
    );
    /**
     * Adds all facets and referenced points to the underlying mesh structure. The client programmer
     * must be sure that both meshes don't have geometric overlaps, otherwise the resulting mesh
     * might be invalid, i.e. has self-intersections.
     * @note The method guarantees that the order of the arrays of the underlying mesh and of the
     * given array is kept.
     * @note Not all points of \a rKernel are necessarily appended to the underlying mesh but only
     * these points which are referenced by facets of \a rKernel.
     */
    void Merge(const MeshKernel& rKernel);
    /**
     * This method is provided for convenience that directly accepts the point and
     * facet arrays.
     * @note Not all points of \a rPoints are necessarily appended to the underlying
     * mesh but only these points which are referenced by facets of \a rFaces.
     */
    void Merge(const MeshPointArray& rPoints, const MeshFacetArray& rFaces);
    /** Deletes the facet the iterator points to. The deletion of a facet requires
     * the following steps:
     * \li Mark the neighbour index of all neighbour facets to the deleted facet as invalid
     * \li Adjust the indices of the neighbour facets of all facets.
     * \li If there is no neighbour facet check if the points can be deleted.
     * True is returned if the facet could be deleted.
     * @note This method is very slow and should only be called occasionally.
     * @note After deletion of the facet \a rclIter becomes invalid and must not
     * be used before setting to a new position.
     */
    bool DeleteFacet(const MeshFacetIterator& rclIter);
    /**
     * Does basically the same as the method above unless that the index of the facet is given.
     */
    bool DeleteFacet(FacetIndex ulInd);
    /** Removes several facets from the data structure.
     * @note This method overwrites the free usable property of each mesh point.
     * @note This method also removes points from the structure that are no longer
     * referenced by the facets.
     * @note This method is very slow and should only be called occasionally.
     */
    void DeleteFacets(const std::vector<FacetIndex>& raulFacets);
    /** Deletes the point the iterator points to. The deletion of a point requires the following
     * step: \li Find all associated facets to this point. \li Delete these facets. True is returned
     * if the point could be deleted.
     * @note This method is very slow and should only be called occasionally.
     * @note After deletion of the point \a rclIter becomes invalid and must not
     * be used before setting to a new position.
     */
    bool DeletePoint(const MeshPointIterator& rclIter);
    /**
     * Does basically the same as the method above unless that the index of the facet is given.
     */
    bool DeletePoint(PointIndex ulInd);
    /** Removes several points from the data structure.
     * @note This method overwrites the free usable property of each mesh point.
     */
    void DeletePoints(const std::vector<PointIndex>& raulPoints);
    /** Removes all as INVALID marked points and facets from the structure. */
    void RemoveInvalids();
    /** Rebuilds the neighbour indices for all facets. */
    void RebuildNeighbours();
    /** Removes unreferenced points or facets with invalid indices from the mesh. */
    void Cleanup();
    /** Clears the whole data structure. */
    void Clear();
    /** Replaces the current data structure with the structure built up of the array
     * of triangles given in \a rclFAry.
     */
    MeshKernel& operator=(const std::vector<MeshGeomFacet>& rclFAry);
    /** Assignment operator. */
    MeshKernel& operator=(const MeshKernel& rclMesh);
    MeshKernel& operator=(MeshKernel&& rclMesh);
    /** This allows one to assign the mesh structure directly. The caller must make sure that the
     * point indices are correctly set but the neighbourhood gets checked and corrected if \a
     * checkNeighbourHood is true.
     */
    void Assign(
        const MeshPointArray& rPoints,
        const MeshFacetArray& rFacets,
        bool checkNeighbourHood = false
    );
    /** This method does basically the same as Assign() unless that it swaps the content of both
     * arrays. These arrays may be empty after assigning to the kernel. This method is a convenient
     * way to build up the mesh structure from outside and assign to a mesh kernel without copying
     * the data. Especially for huge meshes this saves memory and increases speed.
     */
    void Adopt(MeshPointArray& rPoints, MeshFacetArray& rFacets, bool checkNeighbourHood = false);
    /// Swaps the content of this kernel and \a mesh
    void Swap(MeshKernel& mesh);
    /// Transform the data structure with the given transformation matrix.
    void operator*=(const Base::Matrix4D& rclMat);
    /** Transform the data structure with the given transformation matrix.
     * It does exactly the same as the '*=' operator.
     */
    void Transform(const Base::Matrix4D& rclMat);
    /** Moves the point at the given index along the vector \a rclTrans. */
    inline void MovePoint(PointIndex ulPtIndex, const Base::Vector3f& rclTrans);
    /** Sets the point at the given index to the new \a rPoint. */
    inline void SetPoint(PointIndex ulPtIndex, const Base::Vector3f& rPoint);
    /** Sets the point at the given index to the new \a rPoint. */
    inline void SetPoint(PointIndex ulPtIndex, float x, float y, float z);
    /** Smoothes the mesh kernel. */
    void Smooth(int iterations, float stepsize);
    /**
     * CheckFacets() is invoked within this method and all found facets get deleted from the mesh
     * structure. The facets to be deleted are returned with their geometric representation.
     * @see CheckFacets().
     */
    void CutFacets(
        const MeshFacetGrid& rclGrid,
        const Base::ViewProjMethod* pclP,
        const Base::Polygon2d& rclPoly,
        bool bCutInner,
        std::vector<MeshGeomFacet>& raclFacets
    );
    /**
     * Does basically the same as method above unless that the facets to be deleted are returned
     * with their index number in the facet array of the mesh structure.
     */
    void CutFacets(
        const MeshFacetGrid& grid,
        const Base::ViewProjMethod* proj,
        const Base::Polygon2d& poly,
        bool bInner,
        std::vector<FacetIndex>& cut
    );
    //@}

protected:
    /** Rebuilds the neighbour indices for subset of all facets from index \a index on. */
    void RebuildNeighbours(FacetIndex);
    /** Checks if this point is associated to no other facet and deletes if so.
     * The point indices of the facets get adjusted.
     * \a ulIndex is the index of the point to be deleted. \a ulFacetIndex is the index
     * of the quasi deleted facet and is ignored. If \a bOnlySetInvalid is true the point
     * doesn't get deleted but marked as invalid.
     */
    void ErasePoint(PointIndex ulIndex, FacetIndex ulFacetIndex, bool bOnlySetInvalid = false);

    /** Adjusts the facet's orierntation to the given normal direction. */
    inline void AdjustNormal(MeshFacet& rclFacet, const Base::Vector3f& rclNormal);
    /** Calculates the normal to the given facet. */
    inline Base::Vector3f GetNormal(const MeshFacet& rclFacet) const;
    /** Calculates the gravity point to the given facet. */
    inline Base::Vector3f GetGravityPoint(const MeshFacet& rclFacet) const;

private:
    MeshPointArray _aclPointArray;        /**< Holds the array of geometric points. */
    MeshFacetArray _aclFacetArray;        /**< Holds the array of facets. */
    mutable Base::BoundBox3f _clBoundBox; /**< The current calculated bounding box. */
    bool _bValid {true};                  /**< Current state of validality. */

    // friends
    friend class MeshPointIterator;
    friend class MeshFacetIterator;
    friend class MeshFastFacetIterator;
    friend class MeshAlgorithm;
    friend class MeshTopoAlgorithm;
    friend class MeshFixDuplicatePoints;
    friend class MeshBuilder;
    friend class MeshTrimming;
};

inline MeshPoint MeshKernel::GetPoint(PointIndex ulIndex) const
{
    assert(ulIndex < _aclPointArray.size());
    return _aclPointArray[ulIndex];
}

inline MeshGeomFacet MeshKernel::GetFacet(FacetIndex ulIndex) const
{
    assert(ulIndex < _aclFacetArray.size());

    const MeshFacet* pclF = &_aclFacetArray[ulIndex];
    MeshGeomFacet clFacet;

    clFacet._aclPoints[0] = _aclPointArray[pclF->_aulPoints[0]];
    clFacet._aclPoints[1] = _aclPointArray[pclF->_aulPoints[1]];
    clFacet._aclPoints[2] = _aclPointArray[pclF->_aulPoints[2]];
    clFacet._ulProp = pclF->_ulProp;
    clFacet._ucFlag = pclF->_ucFlag;
    clFacet.CalcNormal();
    return clFacet;
}

inline MeshGeomFacet MeshKernel::GetFacet(const MeshFacet& rclFacet) const
{
    assert(rclFacet._aulPoints[0] < _aclPointArray.size());
    assert(rclFacet._aulPoints[1] < _aclPointArray.size());
    assert(rclFacet._aulPoints[2] < _aclPointArray.size());

    MeshGeomFacet clFacet;
    clFacet._aclPoints[0] = _aclPointArray[rclFacet._aulPoints[0]];
    clFacet._aclPoints[1] = _aclPointArray[rclFacet._aulPoints[1]];
    clFacet._aclPoints[2] = _aclPointArray[rclFacet._aulPoints[2]];
    clFacet._ulProp = rclFacet._ulProp;
    clFacet._ucFlag = rclFacet._ucFlag;
    clFacet.CalcNormal();
    return clFacet;
}

inline void MeshKernel::GetFacetNeighbours(
    FacetIndex ulIndex,
    FacetIndex& rulNIdx0,
    FacetIndex& rulNIdx1,
    FacetIndex& rulNIdx2
) const
{
    assert(ulIndex < _aclFacetArray.size());

    rulNIdx0 = _aclFacetArray[ulIndex]._aulNeighbours[0];
    rulNIdx1 = _aclFacetArray[ulIndex]._aulNeighbours[1];
    rulNIdx2 = _aclFacetArray[ulIndex]._aulNeighbours[2];
}

inline void MeshKernel::MovePoint(PointIndex ulPtIndex, const Base::Vector3f& rclTrans)
{
    _aclPointArray[ulPtIndex] += rclTrans;
}

inline void MeshKernel::SetPoint(PointIndex ulPtIndex, const Base::Vector3f& rPoint)
{
    _aclPointArray[ulPtIndex] = rPoint;
}

inline void MeshKernel::SetPoint(PointIndex ulPtIndex, float x, float y, float z)
{
    _aclPointArray[ulPtIndex].Set(x, y, z);
}

inline void MeshKernel::AdjustNormal(MeshFacet& rclFacet, const Base::Vector3f& rclNormal)
{
    Base::Vector3f clN = (_aclPointArray[rclFacet._aulPoints[1]]
                          - _aclPointArray[rclFacet._aulPoints[0]])
        % (_aclPointArray[rclFacet._aulPoints[2]] - _aclPointArray[rclFacet._aulPoints[0]]);
    if ((clN * rclNormal) < 0.0F) {
        rclFacet.FlipNormal();
    }
}

inline Base::Vector3f MeshKernel::GetNormal(const MeshFacet& rclFacet) const
{
    Base::Vector3f clN = (_aclPointArray[rclFacet._aulPoints[1]]
                          - _aclPointArray[rclFacet._aulPoints[0]])
        % (_aclPointArray[rclFacet._aulPoints[2]] - _aclPointArray[rclFacet._aulPoints[0]]);
    clN.Normalize();
    return clN;
}

inline Base::Vector3f MeshKernel::GetGravityPoint(const MeshFacet& rclFacet) const
{
    const Base::Vector3f& p0 = _aclPointArray[rclFacet._aulPoints[0]];
    const Base::Vector3f& p1 = _aclPointArray[rclFacet._aulPoints[1]];
    const Base::Vector3f& p2 = _aclPointArray[rclFacet._aulPoints[2]];
    return Base::Vector3f(
        (p0.x + p1.x + p2.x) / 3.0F,
        (p0.y + p1.y + p2.y) / 3.0F,
        (p0.z + p1.z + p2.z) / 3.0F
    );
}

inline void MeshKernel::GetFacetPoints(
    FacetIndex ulFaIndex,
    PointIndex& rclP0,
    PointIndex& rclP1,
    PointIndex& rclP2
) const
{
    assert(ulFaIndex < _aclFacetArray.size());
    const MeshFacet& rclFacet = _aclFacetArray[ulFaIndex];
    rclP0 = rclFacet._aulPoints[0];
    rclP1 = rclFacet._aulPoints[1];
    rclP2 = rclFacet._aulPoints[2];
}

inline void MeshKernel::SetFacetPoints(
    FacetIndex ulFaIndex,
    PointIndex rclP0,
    PointIndex rclP1,
    PointIndex rclP2
)
{
    assert(ulFaIndex < _aclFacetArray.size());
    MeshFacet& rclFacet = _aclFacetArray[ulFaIndex];
    rclFacet._aulPoints[0] = rclP0;
    rclFacet._aulPoints[1] = rclP1;
    rclFacet._aulPoints[2] = rclP2;
}


}  // namespace MeshCore
