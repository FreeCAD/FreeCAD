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


#ifndef MESHALGORITHM_H
#define MESHALGORITHM_H

#include <set>
#include <vector>
#include <map>

#include "MeshKernel.h"
#include "Elements.h"
#include <Base/Vector3D.h>

// forward declarations

namespace Base{
  class ViewProjMethod;
  class Polygon2d;
}

namespace MeshCore {

class MeshGeomFacet;
class MeshGeomEdge;
class MeshKernel;
class MeshFacetGrid;
class MeshFacetArray;
class MeshRefPointToFacets;
class AbstractPolygonTriangulator;

/**
 * The MeshAlgorithm class provides algorithms base on meshes.
 */
class MeshExport MeshAlgorithm
{
public:
  /// Construction
  MeshAlgorithm (const MeshKernel &rclM) : _rclMesh(rclM) { }
  /// Destruction
  ~MeshAlgorithm (void) { }

public:
  /**
   * Searches for the nearest facet to the ray defined by
   * (\a rclPt, \a rclDir).
   * The point \a rclRes holds the intersection point with the ray and the
   * nearest facet with index \a rulFacet.
   * \note This method tests all facets so it should only be used
   * occasionally.
   */
  bool NearestFacetOnRay (const Base::Vector3f &rclPt, const Base::Vector3f &rclDir, Base::Vector3f &rclRes,
                          unsigned long &rulFacet) const;
  /**
   * Searches for the nearest facet to the ray defined by
   * (\a rclPt, \a rclDir).
   * The point \a rclRes holds the intersection point with the ray and the
   * nearest facet with index \a rulFacet.
   * \note This method is optimized by using a grid. So this method can be
   * used for a lot of tests.
   */
  bool NearestFacetOnRay (const Base::Vector3f &rclPt, const Base::Vector3f &rclDir, const MeshFacetGrid &rclGrid,
                          Base::Vector3f &rclRes, unsigned long &rulFacet) const;
  /**
   * Searches for the nearest facet to the ray defined by
   * (\a rclPt, \a rclDir).
   * The point \a rclRes holds the intersection point with the ray and the
   * nearest facet with index \a rulFacet.
   * \note This method tests all facets taken from \a raulFacets instead of
   * the attached mesh. So the caller must ensure that the indices are valid
   * facets.
   */
  bool NearestFacetOnRay (const Base::Vector3f &rclPt, const Base::Vector3f &rclDir, const std::vector<unsigned long> &raulFacets,
                          Base::Vector3f &rclRes, unsigned long &rulFacet) const;
  /**
   * Searches for the nearest facet to the ray defined by (\a rclPt, \a  rclDir). The point \a rclRes holds
   * the intersection point with the ray and the nearest facet with index \a rulFacet.
   * More a search radius around the ray of \a fMaxSearchArea is defined.
   * \note This method is optimized by using a grid. So this method can be used for a lot of tests.
   */
  bool NearestFacetOnRay (const Base::Vector3f &rclPt, const Base::Vector3f &rclDir, float fMaxSearchArea,
                          const MeshFacetGrid &rclGrid, Base::Vector3f &rclRes, unsigned long &rulFacet) const;
  /**
   * Searches for the first facet of the grid element (\a rclGrid) in that the point \a rclPt lies into which is a distance not
   * higher than \a fMaxDistance. Of no such facet is found \a rulFacet is undefined and false is returned, otherwise true.
   * \note If the point \a rclPt is outside of the grid \a rclGrid nothing is done.
   */
  bool FirstFacetToVertex(const Base::Vector3f &rclPt, float fMaxDistance, const MeshFacetGrid &rclGrid, unsigned long &rulFacet) const;
  /**
   * Checks from the viewpoint \a rcView if the vertex \a rcVertex is visible or it is hidden by a facet. 
   * If the vertex is visible true is returned, false otherwise.
   */
  bool IsVertexVisible (const Base::Vector3f &rcVertex, const Base::Vector3f &rcView, const MeshFacetGrid &rclGrid ) const;
  /**
   * Calculates the average length of edges.
   */
  float GetAverageEdgeLength() const;
  /**
   * Calculates the gravity point of the mesh.
   */
  Base::Vector3f GetGravityPoint() const;
  /**
   * Returns all boundaries of the mesh.
   */
  void GetMeshBorders (std::list<std::vector<Base::Vector3f> > &rclBorders) const;
  /**
   * Returns all boundaries of the mesh. This method does basically the same as above unless that it returns the point indices
   * of the boundaries.
   */
  void GetMeshBorders (std::list<std::vector<unsigned long> > &rclBorders) const;
  /**
   * Returns all boundaries of a subset the mesh defined by \a raulInd.
   */
  void GetFacetBorders (const std::vector<unsigned long> &raulInd, std::list<std::vector<Base::Vector3f> > &rclBorders) const;
  /**
   * Returns all boundaries of a subset the mesh defined by \a raulInd. This method does basically the same as above unless 
   * that it returns the point indices of the boundaries.
   * If \a ignoreOrientation is false (the default) we may get a broken boundary curve if the mesh has facets
   * with wrong orientation. However, if \a ignoreOrientation is true we may get a boundary curve with wrong
   * orientation even if the mesh is topologically correct. You should let the default value unless you exactly
   * know what you do.
   */
  void GetFacetBorders (const std::vector<unsigned long> &raulInd, std::list<std::vector<unsigned long> > &rclBorders,
                        bool ignoreOrientation = false) const;
  /**
   * Returns the boundary of the mesh to the facet \a uFacet. If this facet does not have an open edge the returned
   * boundary is empty.
   */
  void GetMeshBorder(unsigned long uFacet, std::list<unsigned long>& rBorder) const;
  /**
   * Boundaries that consist of several loops must be split in several independent boundaries
   * to perform e.g. a polygon triangulation algorithm on them.
   */
  void SplitBoundaryLoops( std::list<std::vector<unsigned long> >& aBorders );
  /**
   * Fills up the single boundary if it is a hole with high quality triangles and a maximum area of \a fMaxArea.
   * The triangulation information is stored in \a rFaces and \a rPoints.
   * To speed up the calculations the optional parameter \a pStructure can be specified that holds a facet-to-points
   * structure of the underlying mesh.
   * If the boundary is not a hole or the algorithm failed false is returned, otherwise true.
   * @note \a boundary contains the point indices of the mesh data structure. The first and last index must therefore be equal.
   * @note \a rPoints contains the geometric points of the triangulation. The number of points can be the same as or exceed
   * the number of boundary indices but it cannot be lower. 
   * @note If the number of geometric points exceeds the number of boundary indices then the triangulation algorithm has 
   * introduced new points which are added to the end of \a rPoints.
   */
  bool FillupHole(const std::vector<unsigned long>& boundary,
                  AbstractPolygonTriangulator& cTria,
                  MeshFacetArray& rFaces, MeshPointArray& rPoints,
                  int level, const MeshRefPointToFacets* pP2FStructure=0) const;
  /** Sets to all facets in \a raulInds the properties in raulProps. 
   * \note Both arrays must have the same size.
   */
  void SetFacetsProperty(const std::vector<unsigned long> &raulInds, const std::vector<unsigned long> &raulProps) const;
  /** Sets to all facets the flag \a tF. */
  void SetFacetFlag (MeshFacet::TFlagType tF) const;
  /** Sets to all points the flag \a tF. */
  void SetPointFlag (MeshPoint::TFlagType tF) const;
  /** Resets of all facets the flag \a tF. */
  void ResetFacetFlag (MeshFacet::TFlagType tF) const;
  /** Resets of all points the flag \a tF. */
  void ResetPointFlag (MeshPoint::TFlagType tF) const;
  /** Sets to all facets in \a raulInds the flag \a tF. */
  void SetFacetsFlag (const std::vector<unsigned long> &raulInds, MeshFacet::TFlagType tF) const;
  /** Sets to all points in \a raulInds the flag \a tF. */
  void SetPointsFlag (const std::vector<unsigned long> &raulInds, MeshPoint::TFlagType tF) const;
  /** Gets all facets in \a raulInds with the flag \a tF. */
  void GetFacetsFlag (std::vector<unsigned long> &raulInds, MeshFacet::TFlagType tF) const;
  /** Gets all points in \a raulInds with the flag \a tF. */
  void GetPointsFlag (std::vector<unsigned long> &raulInds, MeshPoint::TFlagType tF) const;
  /** Resets from all facets in \a raulInds the flag \a tF. */
  void ResetFacetsFlag (const std::vector<unsigned long> &raulInds, MeshFacet::TFlagType tF) const;
  /** Resets from all points in \a raulInds the flag \a tF. */
  void ResetPointsFlag (const std::vector<unsigned long> &raulInds, MeshPoint::TFlagType tF) const;
  /** Count all facets with the flag \a tF. */
  unsigned long CountFacetFlag (MeshFacet::TFlagType tF) const;
  /** Count all points with the flag \a tF. */
  unsigned long CountPointFlag (MeshPoint::TFlagType tF) const;
  /** Returns all geometric points from the facets in \a rvecIndices. */
  void PointsFromFacetsIndices (const std::vector<unsigned long> &rvecIndices, std::vector<Base::Vector3f> &rvecPoints) const;
  /**
   * Returns the indices of all facets that have at least one point that lies inside the tool mesh. The direction
   * \a dir is used to try to foraminate the facets of the tool mesh and counts the number of foraminated facets.
   * If this number is odd the considered point lies inside otherwise outside.
   * @note The tool mesh must be a valid solid.
   * @note It's not tested if \a rToolMesh is a valid solid. In case it is not the result is undefined.
   */
  void GetFacetsFromToolMesh( const MeshKernel& rToolMesh, const Base::Vector3f& rcDir, std::vector<unsigned long> &raclCutted ) const;
  /**
   * Does basically the same as method above except it uses a mesh grid to speed up the computation.
   */
  void GetFacetsFromToolMesh( const MeshKernel& rToolMesh, const Base::Vector3f& rcDir, const MeshFacetGrid& rGrid, std::vector<unsigned long> &raclCutted ) const;
  /** 
   * Checks whether the bounding box \a rBox is surrounded by the attached mesh which must be a solid.
   * The direction \a rcDir is used to try to foraminate the facets of the tool mesh and counts the number of foraminated facets.
   *  1 is returned if the box is completely inside the mesh
   *  0 is returned if the box is partially inside (i.e. intersects) the mesh
   * -1 is returned if the box is completely outside the mesh. This could also mean that the mesh is surrounded by \a rBox.
   */
  int Surround( const Base::BoundBox3f& rBox, const Base::Vector3f& rcDir );
  /**
   * Projects the determined facets through projection with \a pclProj into the 2D plane and checks for
   * intersection with the polygon.
   * If \a bInner is \a true than all facets with at least one corner inside the polygon get deleted. If \a
   * bInner is \a false then all facets with at least one corner outside the polygon get deleted.
   * This algorithm is optimized by using a grid.
   */
  void CheckFacets (const MeshFacetGrid &rclGrid, const Base::ViewProjMethod* pclProj, const Base::Polygon2d& rclPoly,
                    bool bInner, std::vector<unsigned long> &rclRes) const;
  /**
   * Does the same as the above method unless that it doesn't use a grid.
   */
  void CheckFacets (const Base::ViewProjMethod* pclProj, const Base::Polygon2d& rclPoly,
                    bool bInner, std::vector<unsigned long> &rclRes) const;
  /**
   * Determines all facets of the given array \a raclFacetIndices that lie at the edge or that
   * have at least neighbour facet that is not inside the array. The resulting array \a raclResultIndices
   * is not be deleted before the algorithm starts. \a usLevel indicates how often the algorithm is 
   * repeated.
   */
  void CheckBorderFacets (const std::vector<unsigned long> &raclFacetIndices, 
                          std::vector<unsigned long> &raclResultIndices, unsigned short usLevel = 1) const;
  /**
   * Invokes CheckBorderFacets() to get all border facets of \a raclFacetIndices. Then the content of
   * \a raclFacetIndices is replaced by all facets that can be deleted.
   * \note The mesh structure is not modified by this method. This is in the responsibility of the user.
   */
  void CutBorderFacets (std::vector<unsigned long> &raclFacetIndices, unsigned short usLevel = 1) const;
  /** Returns the number of border edges */
  unsigned long CountBorderEdges() const;
  /**
   * Determines all border points as indices of the facets in \a raclFacetIndices. The points are unsorted.
   */
  void GetBorderPoints (const std::vector<unsigned long> &raclFacetIndices, std::set<unsigned long> &raclResultPointsIndices) const;
  /** Computes the surface of the mesh. */
  float Surface (void) const;
  /** Subsamples the mesh with point distance \a fDist and stores the points in \a rclPoints. */
  void SubSampleByDist  (float fDist, std::vector<Base::Vector3f> &rclPoints) const;
  /**
   * Subsamples the mesh to produce around \a ulCtPoints. \a ulCtPoints should be greater
   * than 5 * number of facets.
   */
  void SubSampleByCount (unsigned long ulCtPoints, std::vector<Base::Vector3f> &rclPoints) const;
  /** Returns only the points of the mesh without actually sampling the data. */
  void SubSampleAllPoints(std::vector<Base::Vector3f> &rclPoints) const;
  /**
   * Searches for all facets that intersect the "search tube" with radius \a r around the polyline. 
   */
  void SearchFacetsFromPolyline (const std::vector<Base::Vector3f> &rclPolyline, float fRadius,
                                 const MeshFacetGrid& rclGrid, std::vector<unsigned long> &rclResultFacetsIndices) const;
  /** Projects a point directly to the mesh (means nearest facet), the result is the facet index and
   * the foraminate point, use second version with grid for more performance.
   */
  bool NearestPointFromPoint (const Base::Vector3f &rclPt, unsigned long &rclResFacetIndex, Base::Vector3f &rclResPoint) const;
  bool NearestPointFromPoint (const Base::Vector3f &rclPt, const MeshFacetGrid& rclGrid,
                              unsigned long &rclResFacetIndex, Base::Vector3f &rclResPoint) const;
  bool NearestPointFromPoint (const Base::Vector3f &rclPt, const MeshFacetGrid& rclGrid, float fMaxSearchArea,
                              unsigned long &rclResFacetIndex, Base::Vector3f &rclResPoint) const;
  /** Cuts the mesh with a plane. The result is a list of polylines. */
  bool CutWithPlane (const Base::Vector3f &clBase, const Base::Vector3f &clNormal, const MeshFacetGrid &rclGrid,
                     std::list<std::vector<Base::Vector3f> > &rclResult, float fMinEps = 1.0e-2f, bool bConnectPolygons = false) const;
  /** 
   * Gets all facets that cut the plane (N,d) and that lie between the two points left and right. 
   * The plane is defined by it normalized normal and the signed distance to the origin.
   */
  void GetFacetsFromPlane (const MeshFacetGrid &rclGrid, const Base::Vector3f& clNormal, float dist, 
      const Base::Vector3f &rclLeft, const Base::Vector3f &rclRight, std::vector<unsigned long> &rclRes) const;

  /** Returns true if the distance from the \a rclPt to the facet \a ulFacetIdx is less than \a fMaxDistance.
   * If this restriction is met \a rfDistance is set to the actual distance, otherwise false is returned.
   */
  bool Distance (const Base::Vector3f &rclPt, unsigned long ulFacetIdx, float fMaxDistance, float &rfDistance) const;
  /**
   * Calculates the minimum grid length so that not more elements than \a maxElements will be created when the grid gets
   * built up. The minimum grid length must be at least \a fLength.
   */
  float CalculateMinimumGridLength(float fLength, const Base::BoundBox3f& rBBox, unsigned long maxElements) const;
   
protected:
  /** Helper method to connect the intersection points to polylines. */
  bool ConnectLines (std::list<std::pair<Base::Vector3f, Base::Vector3f> > &rclLines, std::list<std::vector<Base::Vector3f> >&rclPolylines,
                    float fMinEps) const;
  bool ConnectPolygons(std::list<std::vector<Base::Vector3f> > &clPolyList, std::list<std::pair<Base::Vector3f,
                       Base::Vector3f> > &rclLines) const;
  /** Searches the nearest facet in \a raulFacets to the ray (\a rclPt, \a rclDir). */
  bool RayNearestField (const Base::Vector3f &rclPt, const Base::Vector3f &rclDir, const std::vector<unsigned long> &raulFacets,
                        Base::Vector3f &rclRes, unsigned long &rulFacet, float fMaxAngle = Mathf::PI) const;
  /** 
   * Splits the boundary \a rBound in several loops and append this loops to the list of borders.
   */
  void SplitBoundaryLoops( const std::vector<unsigned long>& rBound, std::list<std::vector<unsigned long> >& aBorders );

protected:
  const MeshKernel      &_rclMesh; /**< The mesh kernel. */
};

class MeshExport MeshCollector
{
public:
    MeshCollector(){}
    virtual ~MeshCollector(){}
    virtual void Append(const MeshCore::MeshKernel&, unsigned long index) = 0;
};

class MeshExport PointCollector : public MeshCollector
{
public:
    PointCollector(std::vector<unsigned long>& ind) : indices(ind){}
    virtual ~PointCollector(){}
    virtual void Append(const MeshCore::MeshKernel& kernel, unsigned long index)
    {
        unsigned long ulP1, ulP2, ulP3;
        kernel.GetFacetPoints(index, ulP1, ulP2, ulP3);
        indices.push_back(ulP1);
        indices.push_back(ulP2);
        indices.push_back(ulP3);
    }

private:
    std::vector<unsigned long>& indices;
};

class MeshExport FacetCollector : public MeshCollector
{
public:
    FacetCollector(std::vector<unsigned long>& ind) : indices(ind){}
    virtual ~FacetCollector(){}
    void Append(const MeshCore::MeshKernel&, unsigned long index)
    {
        indices.push_back(index);
    }

private:
    std::vector<unsigned long>& indices;
};

/**
 * The MeshRefPointToFacets builds up a structure to have access to all facets indexing
 * a point.
 * \note If the underlying mesh kernel gets changed this structure becomes invalid and must
 * be rebuilt.
 */
class MeshExport MeshRefPointToFacets
{
public:
    /// Construction
    MeshRefPointToFacets (const MeshKernel &rclM) : _rclMesh(rclM) 
    { Rebuild(); }
    /// Destruction
    ~MeshRefPointToFacets (void)
    { }

    /// Rebuilds up data structure
    void Rebuild (void);
    const std::set<unsigned long>& operator[] (unsigned long) const;
    MeshFacetArray::_TConstIterator GetFacet (unsigned long) const;
    std::set<unsigned long> NeighbourPoints(const std::vector<unsigned long>& , int level) const;
    std::set<unsigned long> NeighbourPoints(unsigned long) const;
    void Neighbours (unsigned long ulFacetInd, float fMaxDist, MeshCollector& collect) const;
    Base::Vector3f GetNormal(unsigned long) const;
    void AddNeighbour(unsigned long, unsigned long);
    void RemoveNeighbour(unsigned long, unsigned long);
    void RemoveFacet(unsigned long);

protected:
    void SearchNeighbours(const MeshFacetArray& rFacets, unsigned long index, const Base::Vector3f &rclCenter, 
        float fMaxDist, std::set<unsigned long> &visit, MeshCollector& collect) const;

protected:
    const MeshKernel  &_rclMesh; /**< The mesh kernel. */
    std::vector<std::set<unsigned long> > _map;
};

/**
 * The MeshRefFacetToFacets builds up a structure to have access to all facets sharing 
 * at least one same point.
 * \note If the underlying mesh kernel gets changed this structure becomes invalid and must
 * be rebuilt.
 */
class MeshExport MeshRefFacetToFacets
{
public:
    /// Construction
    MeshRefFacetToFacets (const MeshKernel &rclM) : _rclMesh(rclM)
    { Rebuild(); }
    /// Destruction
    ~MeshRefFacetToFacets (void)
    { }
    /// Rebuilds up data structure
    void Rebuild (void);

    /// Returns a set of facets sharing one or more points with the facet with
    /// index \a ulFacetIndex.
    const std::set<unsigned long>& operator[] (unsigned long) const;

protected:
    const MeshKernel  &_rclMesh; /**< The mesh kernel. */
    std::vector<std::set<unsigned long> > _map;
};

/**
 * The MeshRefPointToPoints builds up a structure to have access to all neighbour points  
 * of a point. Two points are neighbours if there is an edge indexing both points.
 * \note If the underlying mesh kernel gets changed this structure becomes invalid and must
 * be rebuilt.
 */
class MeshExport MeshRefPointToPoints
{
public:
    /// Construction
    MeshRefPointToPoints (const MeshKernel &rclM) : _rclMesh(rclM) 
    { Rebuild(); }
    /// Destruction
    ~MeshRefPointToPoints (void)
    { }

    /// Rebuilds up data structure
    void Rebuild (void);
    const std::set<unsigned long>& operator[] (unsigned long) const;
    Base::Vector3f GetNormal(unsigned long) const;
    float GetAverageEdgeLength(unsigned long) const;
    void AddNeighbour(unsigned long, unsigned long);
    void RemoveNeighbour(unsigned long, unsigned long);

protected:
    const MeshKernel  &_rclMesh; /**< The mesh kernel. */
    std::vector<std::set<unsigned long> > _map;
};

/**
 * The MeshRefEdgeToFacets builds up a structure to have access to all facets 
 * of an edge. On a manifold mesh an edge has one or two facets associated.
 * \note If the underlying mesh kernel gets changed this structure becomes invalid and must
 * be rebuilt.
 */
class MeshExport MeshRefEdgeToFacets
{
public:
    /// Construction
    MeshRefEdgeToFacets (const MeshKernel &rclM) : _rclMesh(rclM) 
    { Rebuild(); }
    /// Destruction
    ~MeshRefEdgeToFacets (void)
    { }

    /// Rebuilds up data structure
    void Rebuild (void);
    const std::pair<unsigned long, unsigned long>& operator[] (const MeshEdge&) const;

protected:
    class EdgeOrder {
    public:
        bool operator () (const MeshEdge &e1, const MeshEdge &e2) const
        {
            if (e1.first < e2.first)
                return true;
            else if (e1.first > e2.first)
                return false;
            else if (e1.second < e2.second)
                return true;
            else
                return false;
        }
    };
    typedef std::pair<unsigned long, unsigned long> MeshFacetPair;
    const MeshKernel  &_rclMesh; /**< The mesh kernel. */
    std::map<MeshEdge, MeshFacetPair, EdgeOrder> _map;
};

/**
 * The MeshRefNormalToPoints builds up a structure to have access to the normal of a vertex.
 * \note If the underlying mesh kernel gets changed this structure becomes invalid and must
 * be rebuilt.
 */
class MeshExport MeshRefNormalToPoints
{
public:
    /// Construction
    MeshRefNormalToPoints (const MeshKernel &rclM) : _rclMesh(rclM) 
    { Rebuild(); }
    /// Destruction
    ~MeshRefNormalToPoints (void)
    { }

    /// Rebuilds up data structure
    void Rebuild (void);
    const Base::Vector3f& operator[] (unsigned long) const;

protected:
    const MeshKernel  &_rclMesh; /**< The mesh kernel. */
    std::vector<Base::Vector3f> _norm;
};

}; // namespace MeshCore 

#endif  // MESH_ALGORITHM_H 
