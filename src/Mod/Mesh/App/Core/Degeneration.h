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

#include <bitset>
#include <string>
#include <vector>

#include "Evaluation.h"
#include "Definitions.h"

namespace MeshCore {

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
class MeshExport MeshEvalInvalids : public MeshEvaluation
{
public:
  /**
   * Construction.
   */
  MeshEvalInvalids (const MeshKernel &rclM) : MeshEvaluation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshEvalInvalids () { }
  /**
   * Searches for as 'Invalid' marked points or facets.
   */
  bool Evaluate ();
  /**
   * Returns the indices of all invalid facets or facets whose points are invalid.
   */
  std::vector<unsigned long> GetIndices() const;
};

/**
 * The MeshFixInvalids class deletes all elements that are marked as 'Invalid'.
 * @see MeshEvalInvalids
 * @author Werner Mayer
 */
class MeshExport MeshFixInvalids : public MeshValidation
{
public:
  /**
   * Construction.
   */
  MeshFixInvalids (MeshKernel &rclM) : MeshValidation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshFixInvalids () { }
  /**
   * Remove invalid elements.
   */
  bool Fixup ();
};

/**
 * The MeshEvalDuplicatePoints class searches for duplicated points.
 * A point is regarded as duplicated if the distances between x, y and z coordinates of two points is
 * less than an epsilon (defined by MeshDefinitions::_fMinPointDistanceD1, default value=1.0e-5f).
 * @see MeshFixDuplicatePoints
 * @see MeshEvalDegeneratedFacets
 * @author Werner Mayer
 */
class MeshExport MeshEvalDuplicatePoints : public MeshEvaluation
{
public:
  /**
   * Construction.
   */
  MeshEvalDuplicatePoints (const MeshKernel &rclM) : MeshEvaluation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshEvalDuplicatePoints () { }
  /**
   * Merges points to one if the distance between them is less than the global \a MeshDefinitions::_fMinPointDistanceD1.
   */
  bool Evaluate ();
  /**
   * Returns the indices of all duplicated points.
   */
  std::vector<unsigned long> GetIndices() const;
};

/**
 * The MeshFixDuplicatePoints class merges duplicated points.
 * @see MeshEvalDuplicatePoints
 * @author Werner Mayer
 */
class MeshExport MeshFixDuplicatePoints : public MeshValidation
{
public:
  /**
   * Construction.
   */
  MeshFixDuplicatePoints (MeshKernel &rclM) : MeshValidation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshFixDuplicatePoints () { }
  /**
   * Merges duplicated points.
   */
  bool Fixup ();
};

/**
 * The MeshEvalNaNPoints class searches for points with a coordinate that is NaN.
 * @see MeshFixNaNPoints
 * @author Werner Mayer
 */
class MeshExport MeshEvalNaNPoints : public MeshEvaluation
{
public:
  /**
   * Construction.
   */
  MeshEvalNaNPoints (const MeshKernel &rclM) : MeshEvaluation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshEvalNaNPoints () { }
  /**
   * Returns false if a point with NaN coordinate is found.
   */
  bool Evaluate ();
  /**
   * Returns the indices of all NaN points.
   */
  std::vector<unsigned long> GetIndices() const;
};

/**
 * The MeshFixNaNPoints class removes all points with a coordinate that is NaN.
 * @see MeshEvalNaNPoints
 * @author Werner Mayer
 */
class MeshExport MeshFixNaNPoints : public MeshValidation
{
public:
  /**
   * Construction.
   */
  MeshFixNaNPoints (MeshKernel &rclM) : MeshValidation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshFixNaNPoints () { }
  /**
   * Merges duplicated points.
   */
  bool Fixup ();
};

/**
 * The MeshEvalDuplicateFacets class searches for duplicated facets.
 * A facet is regarded as duplicated if all its point indices refer to the same location in the point array of the mesh kernel.
 * The actual geometric points are not taken into consideration.
 * @see MeshFixDuplicateFacets
 * @author Werner Mayer
 */
class MeshExport MeshEvalDuplicateFacets : public MeshEvaluation
{
public:
  /**
   * Construction.
   */
  MeshEvalDuplicateFacets (const MeshKernel &rclM) : MeshEvaluation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshEvalDuplicateFacets () { }
  /**
   * Searches for duplicated facets.
   */
  bool Evaluate ();
  /**
   * Returns the indices of all duplicated facets.
   */
  std::vector<unsigned long> GetIndices() const;
};

/**
 * The MeshFixDuplicateFacets class removes duplicated facets from the mesh structure.
 * @see MeshEvalDuplicateFacets
 * @author Werner Mayer
 */
class MeshExport MeshFixDuplicateFacets : public MeshValidation
{
public:
  /**
   * Construction.
   */
  MeshFixDuplicateFacets (MeshKernel &rclM) : MeshValidation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshFixDuplicateFacets () { }
  /**
   * Removes duplicated facets.
   */
  bool Fixup ();
};

/**
 * The MeshEvalInternalFacets class identifies internal facets of a volume mesh.
 * @author Werner Mayer
 */
class MeshExport MeshEvalInternalFacets : public MeshEvaluation
{
public:
  /**
   * Construction.
   */
  MeshEvalInternalFacets (const MeshKernel &rclM) : MeshEvaluation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshEvalInternalFacets () { }
  /**
   * Identify internal facets.
   */
  bool Evaluate ();
  /**
   * Return the indices.
   */
  const std::vector<unsigned long>& GetIndices() const
  { return _indices; }

private:
    std::vector<unsigned long> _indices;
};

/**
 * The MeshEvalDegeneratedFacets class searches for degenerated facets. A facet is degenerated either if its points
 * are collinear, i.e. they lie on a line or two points are coincident. In the latter case these points are duplicated.
 * If a facet refers to at least two equal point indices then the facet is also regarded is 'corrupt'.
 * @see MeshEvalCorruptedFacets
 * @see MeshEvalDuplicatePoints
 * @see MeshFixDegeneratedFacets
 * @author Werner Mayer
 */
class MeshExport MeshEvalDegeneratedFacets : public MeshEvaluation
{
public:
  /**
   * Construction.
   */
  MeshEvalDegeneratedFacets (const MeshKernel &rclM, float fEps)
      : MeshEvaluation(rclM), fEpsilon(fEps) {}
  /**
   * Destruction.
   */
  ~MeshEvalDegeneratedFacets () { }
  /**
   * Searches degenerated facets.
   */
  bool Evaluate ();
  /**
   * Returns the number of facets with an edge smaller than \a fMinEdgeLength.
   */
  unsigned long CountEdgeTooSmall (float fMinEdgeLength) const;
  /**
   * Returns the indices of all corrupt facets.
   */
  std::vector<unsigned long> GetIndices() const;

private:
  float fEpsilon;
};

/**
 * The MeshFixDegeneratedFacets class tries to fix degenerations by removing the concerning facets.
 * @see MeshEvalDegeneratedFacets
 * @author Werner Mayer
 */
class MeshExport MeshFixDegeneratedFacets : public MeshValidation
{
public:
  /**
   * Construction.
   */
  MeshFixDegeneratedFacets (MeshKernel &rclM, float fEps)
      : MeshValidation(rclM), fEpsilon(fEps) { }
  /**
   * Destruction.
   */
  ~MeshFixDegeneratedFacets () { }
  /**
   * Removes degenerated facets.
   */
  bool Fixup ();
  /**
   * Removes all facets with an edge smaller than \a fMinEdgeLength without leaving holes or gaps
   * in the mesh. Returns the number of removed facets.
   */
  unsigned long RemoveEdgeTooSmall (float fMinEdgeLength = MeshDefinitions::_fMinPointDistance,
                                    float fMinEdgeAngle  = MeshDefinitions::_fMinEdgeAngle);
private:
  float fEpsilon;
};

/**
 * The MeshEvalDeformedFacets class searches for deformed facets. A facet is regarded as deformed
 * if an angle is < 30 deg or > 120 deg.
 * @see MeshFixDegeneratedFacets
 * @author Werner Mayer
 */
class MeshExport MeshEvalDeformedFacets : public MeshEvaluation
{
public:
  /**
   * Construction.
   */
  MeshEvalDeformedFacets (const MeshKernel &rclM, float fMinAngle, float fMaxAngle)
      : MeshEvaluation(rclM), fMinAngle(fMinAngle), fMaxAngle(fMaxAngle) { }
  /**
   * Destruction.
   */
  ~MeshEvalDeformedFacets () { }
  /**
   * Searches deformed facets.
   */
  bool Evaluate ();
  /**
   * Returns the indices of deformed facets.
   */
  std::vector<unsigned long> GetIndices() const;

private:
  float fMinAngle; /**< If an angle of a facet is lower than fMinAngle it's considered as deformed. */
  float fMaxAngle; /**< If an angle of a facet is higher than fMaxAngle it's considered as deformed. */
};

/**
 * The MeshFixDeformedFacets class tries to fix deformed facets by swapping the common edge with one of their neighbours.
 * @note Degenerated facets are also deformed facet but this algorithm tries to fix deformed facets that or not degenerated.
 * The removal of degenerated facets is done by @ref MeshFixDegeneratedFacets.
 * @see MeshEvalDeformedFacets
 * @author Werner Mayer
 */
class MeshExport MeshFixDeformedFacets : public MeshValidation
{
public:
  /**
   * Construction.
   */
  MeshFixDeformedFacets (MeshKernel &rclM, float fMinAngle, float fMaxAngle, float fSwapAngle, float fEps)
      : MeshValidation(rclM), fMinAngle(fMinAngle), fMaxAngle(fMaxAngle),
        fMaxSwapAngle(fSwapAngle), fEpsilon(fEps) { }
  /**
   * Destruction.
   */
  ~MeshFixDeformedFacets () { }
  /**
   * Removes deformed facets.
   */
  bool Fixup ();

private:
  float fMinAngle; /**< If an angle of a facet is lower than fMinAngle it's considered as deformed. */
  float fMaxAngle; /**< If an angle of a facet is higher than fMaxAngle it's considered as deformed. */
  float fMaxSwapAngle; /**< A swap edge is only allowed if the angle of both normals doesn't exceed fMaxSwapAngle */
  float fEpsilon;
};

/**
 * The MeshFixMergeFacets class removes vertexes which have three adjacent vertexes and is referenced by three facets.
 * Usually all the three facets that reference this vertex are not well-formed. If the number of adjacent vertexes
 * is equal to the number of adjacent facets the affected vertex never lies on the boundary and thus it's safe to delete
 * and replace the three facets with a single facet.
 * Effectively this algorithm does the opposite of \ref MeshTopoAlgorithm::InsertVertex
 * @author Werner Mayer
 */
class MeshExport MeshFixMergeFacets : public MeshValidation
{
public:
  /**
   * Construction.
   */
  MeshFixMergeFacets (MeshKernel &rclM)
      : MeshValidation(rclM) { }
  /**
   * Destruction.
   */
  ~MeshFixMergeFacets () { }
  /**
   * Removes deformed facets.
   */
  bool Fixup ();
};

/**
 * If an adjacent point (A) of a point (P) can be projected onto a triangle shared
 * by (P) but not by (A) then we have a local dent. The topology is not affected.
 */
class MeshExport MeshEvalDentsOnSurface : public MeshEvaluation
{
public:
    MeshEvalDentsOnSurface (const MeshKernel &rclM) : MeshEvaluation( rclM ) { }
    ~MeshEvalDentsOnSurface() {}

    bool Evaluate();
    std::vector<unsigned long> GetIndices() const;

private:
    std::vector<unsigned long> indices;
};

class MeshExport MeshFixDentsOnSurface : public MeshValidation
{
public:
    MeshFixDentsOnSurface (MeshKernel &rclM) : MeshValidation( rclM ) { }
    ~MeshFixDentsOnSurface() {}

    bool Fixup();
};

/**
 * If the angle between the adjacent triangles of a triangle is lower then 90 deg
 * but the angles between both of these adjacent triangles is higher than 90 deg
 * we have a fold. The topology is not affected but the geometry is broken.
 */
class MeshExport MeshEvalFoldsOnSurface : public MeshEvaluation
{
public:
    MeshEvalFoldsOnSurface (const MeshKernel &rclM) : MeshEvaluation( rclM ) { }
    ~MeshEvalFoldsOnSurface() {}

    bool Evaluate();
    std::vector<unsigned long> GetIndices() const;

private:
    std::vector<unsigned long> indices;
};

/**
 * Considers a boundary triangle with two open edges and an angle higher than
 * 60 deg with its adjacent triangle as a boundary fold.
 * The topology is not affected there but such triangles can lead to problems
 * on some hole-filling algorithms.
 */
class MeshExport MeshEvalFoldsOnBoundary : public MeshEvaluation
{
public:
    MeshEvalFoldsOnBoundary (const MeshKernel &rclM) : MeshEvaluation( rclM ) { }
    ~MeshEvalFoldsOnBoundary() {}

    bool Evaluate();
    std::vector<unsigned long> GetIndices() const;

private:
    std::vector<unsigned long> indices;
};

class MeshExport MeshFixFoldsOnBoundary : public MeshValidation
{
public:
    MeshFixFoldsOnBoundary (MeshKernel &rclM) : MeshValidation( rclM ) { }
    ~MeshFixFoldsOnBoundary() {}

    bool Fixup();
};

/**
 * Considers two adjacent triangles with an angle higher than 120 deg of their
 * normals as a fold-over. The topology is not affected there.
 */
class MeshExport MeshEvalFoldOversOnSurface : public MeshEvaluation
{
public:
    MeshEvalFoldOversOnSurface (const MeshKernel &rclM) : MeshEvaluation( rclM ) { }
    ~MeshEvalFoldOversOnSurface() {}

    bool Evaluate();
    std::vector<unsigned long> GetIndices() const
    { return this->indices; }

private:
    std::vector<unsigned long> indices;
};

/**
 * The MeshEvalBorderFacet class removes facets whose all three vertices are
 * part of a boundary.
 * @see MeshEvalSingleFacet
 */
class MeshExport MeshEvalBorderFacet : public MeshEvaluation
{
public:
  MeshEvalBorderFacet (const MeshKernel &rclB, std::vector<unsigned long>& f)
    : MeshEvaluation(rclB), _facets(f) {}
  virtual ~MeshEvalBorderFacet () {}
  bool Evaluate();

protected:
    std::vector<unsigned long>& _facets;
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
class MeshExport MeshEvalRangeFacet : public MeshEvaluation
{
public:
  /**
   * Construction.
   */
  MeshEvalRangeFacet (const MeshKernel &rclM) : MeshEvaluation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshEvalRangeFacet () { }
  /**
   * Searches for facets that has neighbour facet indices out of range.
   */
  bool Evaluate ();
  /**
   * Returns the indices of all facets with invalid neighbour indices.
   */
  std::vector<unsigned long> GetIndices() const;
};

/**
 * The MeshFixRangeFacet class fixes facets with invalid neighbour indices.
 * @see MeshEvalRangeFacet
 * @author Werner Mayer
 */
class MeshExport MeshFixRangeFacet : public MeshValidation
{
public:
  /**
   * Construction.
   */
  MeshFixRangeFacet (MeshKernel &rclM) : MeshValidation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshFixRangeFacet () { }
  /**
   * Fixes facets with neighbour indices out of range.
   */
  bool Fixup ();
};

/**
 * The MeshEvalRangePoint class searches for facets that has point indices out of range.
 * @see MeshFixRangePoint
 * @author Werner Mayer
 */
class MeshExport MeshEvalRangePoint : public MeshEvaluation
{
public:
  /**
   * Construction.
   */
  MeshEvalRangePoint (const MeshKernel &rclM) : MeshEvaluation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshEvalRangePoint () { }
  /**
   * Searches for facets that has point indices out of range.
   */
  bool Evaluate ();
  /**
   * Returns the indices of all facets with invalid point indices.
   */
  std::vector<unsigned long> GetIndices() const;
};

/**
 * The MeshFixRangePoint class fixes the facets with point indices out of range.
 * @see MeshFixRangePoint
 * @author Werner Mayer
 */
class MeshExport MeshFixRangePoint : public MeshValidation
{
public:
  /**
   * Construction.
   */
  MeshFixRangePoint (MeshKernel &rclM) : MeshValidation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshFixRangePoint () { }
  /**
   * Fixes facets with point indices out of range.
   */
  bool Fixup ();
};

/**
 * The MeshEvalCorruptedFacets class searches for facets with several equal point
 * indices.
 * @see MeshFixCorruptedFacets
 * @author Werner Mayer
 */
class MeshExport MeshEvalCorruptedFacets : public MeshEvaluation
{
public:
  /**
   * Construction.
   */
  MeshEvalCorruptedFacets (const MeshKernel &rclM) : MeshEvaluation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshEvalCorruptedFacets () { }
  /**
   * Searches for corrupted facets.
   */
  bool Evaluate ();
  /**
   * Returns the indices of all corrupt facets.
   */
  std::vector<unsigned long> GetIndices() const;
};

/**
 * The MeshFixCorruptedFacets class fixes corrupted facets by removing them from the mesh
 * structure.
 * @see MeshEvalCorruptedFacets
 * @author Werner Mayer
 */
class MeshExport MeshFixCorruptedFacets : public MeshValidation
{
public:
  /**
   * Construction.
   */
  MeshFixCorruptedFacets (MeshKernel &rclM) : MeshValidation( rclM ) { }
  /**
   * Destruction.
   */
  ~MeshFixCorruptedFacets () { }
  /**
   * Removes corrupted facets.
   */
  bool Fixup ();
};

} // namespace MeshCore

#endif // MESH_DEGENERATION_H
