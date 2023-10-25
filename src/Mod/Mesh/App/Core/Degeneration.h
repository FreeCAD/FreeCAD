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

#ifndef MESH_DEGENERATION_H
#define MESH_DEGENERATION_H

#include <string>
#include <vector>

#include "Definitions.h"
#include "Evaluation.h"


namespace MeshCore
{

class MeshKernel;
class MeshGeomFacet;
class MeshFacetIterator;

/**
 * The MeshEvalInvalids class searches for as 'Invalid' marked facets and points.
 * Basically this comes from a not properly implemented algorithm that marks facets or points
 * as 'Invalid' without removing them from the mesh kernel.
 * @see MeshFixInvalids
 * @author Werner Mayer
 */
class MeshExport MeshEvalInvalids: public MeshEvaluation
{
public:
    /**
     * Construction.
     */
    explicit MeshEvalInvalids(const MeshKernel& rclM)
        : MeshEvaluation(rclM)
    {}
    /**
     * Searches for as 'Invalid' marked points or facets.
     */
    bool Evaluate() override;
    /**
     * Returns the indices of all invalid facets or facets whose points are invalid.
     */
    std::vector<FacetIndex> GetIndices() const;
};

/**
 * The MeshFixInvalids class deletes all elements that are marked as 'Invalid'.
 * @see MeshEvalInvalids
 * @author Werner Mayer
 */
class MeshExport MeshFixInvalids: public MeshValidation
{
public:
    /**
     * Construction.
     */
    explicit MeshFixInvalids(MeshKernel& rclM)
        : MeshValidation(rclM)
    {}
    /**
     * Remove invalid elements.
     */
    bool Fixup() override;
};

/**
 * The MeshEvalDuplicatePoints class searches for duplicated points.
 * A point is regarded as duplicated if the distances between x, y and z coordinates of two points
 * is less than an epsilon (defined by MeshDefinitions::_fMinPointDistanceD1, default
 * value=1.0e-5f).
 * @see MeshFixDuplicatePoints
 * @see MeshEvalDegeneratedFacets
 * @author Werner Mayer
 */
class MeshExport MeshEvalDuplicatePoints: public MeshEvaluation
{
public:
    /**
     * Construction.
     */
    explicit MeshEvalDuplicatePoints(const MeshKernel& rclM)
        : MeshEvaluation(rclM)
    {}
    /**
     * Merges points to one if the distance between them is less than the global \a
     * MeshDefinitions::_fMinPointDistanceD1.
     */
    bool Evaluate() override;
    /**
     * Returns the indices of all duplicated points.
     */
    std::vector<PointIndex> GetIndices() const;
};

/**
 * The MeshFixDuplicatePoints class merges duplicated points.
 * @see MeshEvalDuplicatePoints
 * @author Werner Mayer
 */
class MeshExport MeshFixDuplicatePoints: public MeshValidation
{
public:
    /**
     * Construction.
     */
    explicit MeshFixDuplicatePoints(MeshKernel& rclM)
        : MeshValidation(rclM)
    {}
    /**
     * Merges duplicated points.
     */
    bool Fixup() override;
};

/**
 * The MeshEvalNaNPoints class searches for points with a coordinate that is NaN.
 * @see MeshFixNaNPoints
 * @author Werner Mayer
 */
class MeshExport MeshEvalNaNPoints: public MeshEvaluation
{
public:
    /**
     * Construction.
     */
    explicit MeshEvalNaNPoints(const MeshKernel& rclM)
        : MeshEvaluation(rclM)
    {}
    /**
     * Returns false if a point with NaN coordinate is found.
     */
    bool Evaluate() override;
    /**
     * Returns the indices of all NaN points.
     */
    std::vector<PointIndex> GetIndices() const;
};

/**
 * The MeshFixNaNPoints class removes all points with a coordinate that is NaN.
 * @see MeshEvalNaNPoints
 * @author Werner Mayer
 */
class MeshExport MeshFixNaNPoints: public MeshValidation
{
public:
    /**
     * Construction.
     */
    explicit MeshFixNaNPoints(MeshKernel& rclM)
        : MeshValidation(rclM)
    {}
    /**
     * Merges duplicated points.
     */
    bool Fixup() override;
};

/**
 * The MeshEvalDuplicateFacets class searches for duplicated facets.
 * A facet is regarded as duplicated if all its point indices refer to the same location in the
 * point array of the mesh kernel. The actual geometric points are not taken into consideration.
 * @see MeshFixDuplicateFacets
 * @author Werner Mayer
 */
class MeshExport MeshEvalDuplicateFacets: public MeshEvaluation
{
public:
    /**
     * Construction.
     */
    explicit MeshEvalDuplicateFacets(const MeshKernel& rclM)
        : MeshEvaluation(rclM)
    {}
    /**
     * Searches for duplicated facets.
     */
    bool Evaluate() override;
    /**
     * Returns the indices of all duplicated facets.
     */
    std::vector<FacetIndex> GetIndices() const;
};

/**
 * The MeshFixDuplicateFacets class removes duplicated facets from the mesh structure.
 * @see MeshEvalDuplicateFacets
 * @author Werner Mayer
 */
class MeshExport MeshFixDuplicateFacets: public MeshValidation
{
public:
    /**
     * Construction.
     */
    explicit MeshFixDuplicateFacets(MeshKernel& rclM)
        : MeshValidation(rclM)
    {}
    /**
     * Removes duplicated facets.
     */
    bool Fixup() override;
};

/**
 * The MeshEvalInternalFacets class identifies internal facets of a volume mesh.
 * @author Werner Mayer
 */
class MeshExport MeshEvalInternalFacets: public MeshEvaluation
{
public:
    /**
     * Construction.
     */
    explicit MeshEvalInternalFacets(const MeshKernel& rclM)
        : MeshEvaluation(rclM)
    {}
    /**
     * Identify internal facets.
     */
    bool Evaluate() override;
    /**
     * Return the indices.
     */
    const std::vector<FacetIndex>& GetIndices() const
    {
        return _indices;
    }

private:
    std::vector<FacetIndex> _indices;
};

/**
 * The MeshEvalDegeneratedFacets class searches for degenerated facets. A facet is degenerated
 * either if its points are collinear, i.e. they lie on a line or two points are coincident. In the
 * latter case these points are duplicated. If a facet refers to at least two equal point indices
 * then the facet is also regarded is 'corrupt'.
 * @see MeshEvalCorruptedFacets
 * @see MeshEvalDuplicatePoints
 * @see MeshFixDegeneratedFacets
 * @author Werner Mayer
 */
class MeshExport MeshEvalDegeneratedFacets: public MeshEvaluation
{
public:
    /**
     * Construction.
     */
    MeshEvalDegeneratedFacets(const MeshKernel& rclM, float fEps)
        : MeshEvaluation(rclM)
        , fEpsilon(fEps)
    {}
    /**
     * Searches degenerated facets.
     */
    bool Evaluate() override;
    /**
     * Returns the number of facets with an edge smaller than \a fMinEdgeLength.
     */
    unsigned long CountEdgeTooSmall(float fMinEdgeLength) const;
    /**
     * Returns the indices of all corrupt facets.
     */
    std::vector<FacetIndex> GetIndices() const;

private:
    float fEpsilon;
};

/**
 * The MeshFixDegeneratedFacets class tries to fix degenerations by removing the concerning facets.
 * @see MeshEvalDegeneratedFacets
 * @author Werner Mayer
 */
class MeshExport MeshFixDegeneratedFacets: public MeshValidation
{
public:
    /**
     * Construction.
     */
    MeshFixDegeneratedFacets(MeshKernel& rclM, float fEps)
        : MeshValidation(rclM)
        , fEpsilon(fEps)
    {}
    /**
     * Removes degenerated facets.
     */
    bool Fixup() override;

private:
    float fEpsilon;
};

/**
 * The MeshRemoveNeedles class tries to fix degenerations by removing needles.
 * Needles are triangles where its longest edge is much longer than its shortest edge.
 * https://graphics.uni-bielefeld.de/publications/vmv01.pdf
 * @see MeshFixDegeneratedFacets
 * @see MeshFixCaps
 * @author Werner Mayer
 */
class MeshExport MeshRemoveNeedles: public MeshValidation
{
public:
    /**
     * Construction. The \arg fMinEdgeLen must be in the range of 0.0 and 0.25.
     * It defines the amount of perimeter of a triangle for which the shortest
     * edge is considered for removal.
     */
    explicit MeshRemoveNeedles(MeshKernel& rclM, float fMinEdgeLen = 0.05f)
        : MeshValidation(rclM)
        , fMinEdgeLength(std::min(fMinEdgeLen, 0.25f))
    {}
    /**
     * Removes all facets with an edge smaller than \a fMinEdgeLength without leaving holes or gaps
     * in the mesh.
     */
    bool Fixup() override;

private:
    float fMinEdgeLength;
};

/**
 * The MeshFixCaps class tries to fix degenerations by swapping the common edge of a cap
 * and its neighbour.
 * Caps are triangles with one angle close to 180 degree. The definitions of caps and needles
 * are not mutually exclusive but here we only consider triangles that are caps but no needles.
 * https://graphics.uni-bielefeld.de/publications/vmv01.pdf
 * @see MeshFixDegeneratedFacets
 * @see MeshRemoveNeedles
 * @author Werner Mayer
 */
class MeshExport MeshFixCaps: public MeshValidation
{
public:
    /**
     * Construction. The \arg fFactor must be in the range of 0.0 and 0.5.
     */
    explicit MeshFixCaps(MeshKernel& rclM,
                         float fMaxAng = 2.61f,
                         float fFactor = 0.25f)  // ~150 degree
        : MeshValidation(rclM)
        , fMaxAngle(fMaxAng)
        , fSplitFactor(fFactor)
    {}
    /**
     */
    bool Fixup() override;

private:
    float fMaxAngle;
    float fSplitFactor;
};

/**
 * The MeshEvalDeformedFacets class searches for deformed facets. A facet is regarded as deformed
 * if an angle is < 30 deg or > 120 deg.
 * @see MeshFixDegeneratedFacets
 * @author Werner Mayer
 */
class MeshExport MeshEvalDeformedFacets: public MeshEvaluation
{
public:
    /**
     * Construction.
     */
    MeshEvalDeformedFacets(const MeshKernel& rclM, float fMinAngle, float fMaxAngle)
        : MeshEvaluation(rclM)
        , fMinAngle(fMinAngle)
        , fMaxAngle(fMaxAngle)
    {}
    /**
     * Searches deformed facets.
     */
    bool Evaluate() override;
    /**
     * Returns the indices of deformed facets.
     */
    std::vector<FacetIndex> GetIndices() const;

private:
    float fMinAngle; /**< If an angle of a facet is lower than fMinAngle it's considered as
                        deformed. */
    float fMaxAngle; /**< If an angle of a facet is higher than fMaxAngle it's considered as
                        deformed. */
};

/**
 * The MeshFixDeformedFacets class tries to fix deformed facets by swapping the common edge with one
 * of their neighbours.
 * @note Degenerated facets are also deformed facet but this algorithm tries to fix deformed facets
 * that or not degenerated. The removal of degenerated facets is done by @ref
 * MeshFixDegeneratedFacets.
 * @see MeshEvalDeformedFacets
 * @author Werner Mayer
 */
class MeshExport MeshFixDeformedFacets: public MeshValidation
{
public:
    /**
     * Construction.
     */
    MeshFixDeformedFacets(MeshKernel& rclM,
                          float fMinAngle,
                          float fMaxAngle,
                          float fSwapAngle,
                          float fEps)
        : MeshValidation(rclM)
        , fMinAngle(fMinAngle)
        , fMaxAngle(fMaxAngle)
        , fMaxSwapAngle(fSwapAngle)
        , fEpsilon(fEps)
    {}
    /**
     * Removes deformed facets.
     */
    bool Fixup() override;

private:
    float fMinAngle;     /**< If an angle of a facet is lower than fMinAngle it's considered as
                            deformed. */
    float fMaxAngle;     /**< If an angle of a facet is higher than fMaxAngle it's considered as
                            deformed. */
    float fMaxSwapAngle; /**< A swap edge is only allowed if the angle of both normals doesn't
                            exceed fMaxSwapAngle */
    float fEpsilon;
};

/**
 * The MeshFixMergeFacets class removes vertexes which have three adjacent vertexes and is
 * referenced by three facets. Usually all the three facets that reference this vertex are not
 * well-formed. If the number of adjacent vertexes is equal to the number of adjacent facets the
 * affected vertex never lies on the boundary and thus it's safe to delete and replace the three
 * facets with a single facet. Effectively this algorithm does the opposite of \ref
 * MeshTopoAlgorithm::InsertVertex
 * @author Werner Mayer
 */
class MeshExport MeshFixMergeFacets: public MeshValidation
{
public:
    /**
     * Construction.
     */
    explicit MeshFixMergeFacets(MeshKernel& rclM)
        : MeshValidation(rclM)
    {}
    /**
     * Removes deformed facets.
     */
    bool Fixup() override;
};

/**
 * If an adjacent point (A) of a point (P) can be projected onto a triangle shared
 * by (P) but not by (A) then we have a local dent. The topology is not affected.
 */
class MeshExport MeshEvalDentsOnSurface: public MeshEvaluation
{
public:
    explicit MeshEvalDentsOnSurface(const MeshKernel& rclM)
        : MeshEvaluation(rclM)
    {}

    bool Evaluate() override;
    std::vector<FacetIndex> GetIndices() const;

private:
    std::vector<FacetIndex> indices;
};

class MeshExport MeshFixDentsOnSurface: public MeshValidation
{
public:
    explicit MeshFixDentsOnSurface(MeshKernel& rclM)
        : MeshValidation(rclM)
    {}

    bool Fixup() override;
};

/**
 * If the angle between the adjacent triangles of a triangle is lower then 90 deg
 * but the angles between both of these adjacent triangles is higher than 90 deg
 * we have a fold. The topology is not affected but the geometry is broken.
 */
class MeshExport MeshEvalFoldsOnSurface: public MeshEvaluation
{
public:
    explicit MeshEvalFoldsOnSurface(const MeshKernel& rclM)
        : MeshEvaluation(rclM)
    {}

    bool Evaluate() override;
    std::vector<FacetIndex> GetIndices() const;

private:
    std::vector<FacetIndex> indices;
};

/**
 * Considers a boundary triangle with two open edges and an angle higher than
 * 60 deg with its adjacent triangle as a boundary fold.
 * The topology is not affected there but such triangles can lead to problems
 * on some hole-filling algorithms.
 */
class MeshExport MeshEvalFoldsOnBoundary: public MeshEvaluation
{
public:
    explicit MeshEvalFoldsOnBoundary(const MeshKernel& rclM)
        : MeshEvaluation(rclM)
    {}

    bool Evaluate() override;
    std::vector<FacetIndex> GetIndices() const;

private:
    std::vector<FacetIndex> indices;
};

class MeshExport MeshFixFoldsOnBoundary: public MeshValidation
{
public:
    explicit MeshFixFoldsOnBoundary(MeshKernel& rclM)
        : MeshValidation(rclM)
    {}

    bool Fixup() override;
};

/**
 * Considers two adjacent triangles with an angle higher than 120 deg of their
 * normals as a fold-over. The topology is not affected there.
 */
class MeshExport MeshEvalFoldOversOnSurface: public MeshEvaluation
{
public:
    explicit MeshEvalFoldOversOnSurface(const MeshKernel& rclM)
        : MeshEvaluation(rclM)
    {}

    bool Evaluate() override;
    std::vector<FacetIndex> GetIndices() const
    {
        return this->indices;
    }

private:
    std::vector<FacetIndex> indices;
};

/**
 * The MeshEvalBorderFacet class removes facets whose all three vertices are
 * part of a boundary.
 * @see MeshEvalSingleFacet
 */
class MeshExport MeshEvalBorderFacet: public MeshEvaluation
{
public:
    MeshEvalBorderFacet(const MeshKernel& rclB, std::vector<FacetIndex>& f)
        : MeshEvaluation(rclB)
        , _facets(f)
    {}
    bool Evaluate() override;

private:
    std::vector<FacetIndex>& _facets;
};

// ----------------------------------------------------

/**
 * The MeshEvalRangeFacet class checks whether a facet points to neighbour
 * facets that are out of range. All errors detected by this class would also
 * be implicitly found by MeshEvalNeighbourhood. However, MeshEvalRangeFacet
 * is used for a very fast search while MeshEvalNeighbourhood needs much more
 * time because it can detect more errors.
 * @see MeshFixRangeFacet
 * @author Werner Mayer
 */
class MeshExport MeshEvalRangeFacet: public MeshEvaluation
{
public:
    /**
     * Construction.
     */
    explicit MeshEvalRangeFacet(const MeshKernel& rclM)
        : MeshEvaluation(rclM)
    {}
    /**
     * Searches for facets that has neighbour facet indices out of range.
     */
    bool Evaluate() override;
    /**
     * Returns the indices of all facets with invalid neighbour indices.
     */
    std::vector<FacetIndex> GetIndices() const;
};

/**
 * The MeshFixRangeFacet class fixes facets with invalid neighbour indices.
 * @see MeshEvalRangeFacet
 * @author Werner Mayer
 */
class MeshExport MeshFixRangeFacet: public MeshValidation
{
public:
    /**
     * Construction.
     */
    explicit MeshFixRangeFacet(MeshKernel& rclM)
        : MeshValidation(rclM)
    {}
    /**
     * Fixes facets with neighbour indices out of range.
     */
    bool Fixup() override;
};

/**
 * The MeshEvalRangePoint class searches for facets that has point indices out of range.
 * @see MeshFixRangePoint
 * @author Werner Mayer
 */
class MeshExport MeshEvalRangePoint: public MeshEvaluation
{
public:
    /**
     * Construction.
     */
    explicit MeshEvalRangePoint(const MeshKernel& rclM)
        : MeshEvaluation(rclM)
    {}
    /**
     * Searches for facets that has point indices out of range.
     */
    bool Evaluate() override;
    /**
     * Returns the indices of all facets with invalid point indices.
     */
    std::vector<PointIndex> GetIndices() const;
};

/**
 * The MeshFixRangePoint class fixes the facets with point indices out of range.
 * @see MeshFixRangePoint
 * @author Werner Mayer
 */
class MeshExport MeshFixRangePoint: public MeshValidation
{
public:
    /**
     * Construction.
     */
    explicit MeshFixRangePoint(MeshKernel& rclM)
        : MeshValidation(rclM)
    {}
    /**
     * Fixes facets with point indices out of range.
     */
    bool Fixup() override;
};

/**
 * The MeshEvalCorruptedFacets class searches for facets with several equal point
 * indices.
 * @see MeshFixCorruptedFacets
 * @author Werner Mayer
 */
class MeshExport MeshEvalCorruptedFacets: public MeshEvaluation
{
public:
    /**
     * Construction.
     */
    explicit MeshEvalCorruptedFacets(const MeshKernel& rclM)
        : MeshEvaluation(rclM)
    {}
    /**
     * Searches for corrupted facets.
     */
    bool Evaluate() override;
    /**
     * Returns the indices of all corrupt facets.
     */
    std::vector<FacetIndex> GetIndices() const;
};

/**
 * The MeshFixCorruptedFacets class fixes corrupted facets by removing them from the mesh
 * structure.
 * @see MeshEvalCorruptedFacets
 * @author Werner Mayer
 */
class MeshExport MeshFixCorruptedFacets: public MeshValidation
{
public:
    /**
     * Construction.
     */
    explicit MeshFixCorruptedFacets(MeshKernel& rclM)
        : MeshValidation(rclM)
    {}
    /**
     * Removes corrupted facets.
     */
    bool Fixup() override;
};

/**
 * The MeshEvalPointOnEdge class searches for points that lie on or close to an edge of a triangle.
 * @see MeshFixPointOnEdge
 * @author Werner Mayer
 */
class MeshExport MeshEvalPointOnEdge: public MeshEvaluation
{
public:
    /**
     * Construction.
     */
    explicit MeshEvalPointOnEdge(const MeshKernel& rclM)
        : MeshEvaluation(rclM)
    {}
    /**
     * Searches for points that lie on edge of triangle.
     */
    bool Evaluate() override;
    /**
     * Returns the indices of all points on edge.
     */
    std::vector<PointIndex> GetPointIndices() const;
    /**
     * Returns the indices of all facets with an open edge on that a point lies.
     */
    std::vector<FacetIndex> GetFacetIndices() const;

private:
    std::vector<PointIndex> pointsIndices;
    std::vector<FacetIndex> facetsIndices;
};

/**
 * The MeshFixPointOnEdge class removes points that lie on or close to an edge of a triangle.
 * @see MeshEvalPointOnEdge
 * @author Werner Mayer
 */
class MeshExport MeshFixPointOnEdge: public MeshValidation
{
public:
    /**
     * Construction.
     */
    explicit MeshFixPointOnEdge(MeshKernel& rclM, bool fill = false)
        : MeshValidation(rclM)
        , fillBoundary(fill)
    {}
    /**
     * Removes points that lie on edges of triangles.
     */
    bool Fixup() override;

private:
    void MarkBoundaries(const std::vector<FacetIndex>& facetsIndices);
    void FindBoundaries(std::list<std::vector<PointIndex>>& borderList);
    void FillBoundaries(const std::list<std::vector<PointIndex>>& borderList);

private:
    bool fillBoundary;
};

}  // namespace MeshCore

#endif  // MESH_DEGENERATION_H
