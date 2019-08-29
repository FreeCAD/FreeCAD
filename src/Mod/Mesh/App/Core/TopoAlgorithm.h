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


#ifndef MESH_TOPOALGORITHM_H
#define MESH_TOPOALGORITHM_H

#include <map>
#include <vector>

#include "Definitions.h"
#include "Iterator.h"
#include "MeshKernel.h"
#include "Elements.h"
#include "Visitor.h"
#include "Algorithm.h"

#include <Base/Vector3D.h>
#include <Base/Sequencer.h>

namespace MeshCore {
class AbstractPolygonTriangulator;

struct EdgeCollapse;

/**
 * The MeshTopoAlgorithm class provides several algorithms to manipulate a mesh.
 * It supports various mesh operations like inserting a new vertex, swapping the
 * common edge of two adjacent facets, split a facet, ...
 * @author Werner Mayer
 */
class MeshExport MeshTopoAlgorithm
{
public:
    // construction/destruction
    MeshTopoAlgorithm (MeshKernel &rclM);
    virtual ~MeshTopoAlgorithm (void);

public:
    /** @name Topological Operations */
    //@{
    /**
     * Inserts a new vertex in the given triangle so that is split into three
     * triangles. The given point must lie inside the triangle not outside or on
     * an edge.
     */
    bool InsertVertex(unsigned long ulFacetPos, const Base::Vector3f&  rclPoint);
    /**
     * This method is provided for convenience. It inserts a new vertex to the
     * mesh and tries to swap the common edges of the newly created facets with
     * their neighbours.
     * Just inserting a new vertex leads to very acute-angled triangles which
     * might be problematic for some algorithms. This method tries to swap the
     * edges to build more well-formed triangles.
     * @see InsertVertex(), ShouldSwapEdge(), SwapEdge().
     */
    bool InsertVertexAndSwapEdge(unsigned long ulFacetPos, const Base::Vector3f&  rclPoint,
                                 float fMaxAngle);
    /**
     * Swaps the common edge of two adjacent facets even if the operation might
     * be illegal. To be sure that this operation is legal, check either with
     * IsSwapEdgeLegal() or ShouldSwapEdge() before.
     * An illegal swap edge operation can produce non-manifolds, degenerated
     * facets or it might create a fold on the surface, i.e. geometric overlaps
     * of several triangles. 
     */
    void SwapEdge(unsigned long ulFacetPos, unsigned long ulNeighbour);
    /**
     * Splits the common edge of the two adjacent facets with index \a ulFacetPos
     * and \a ulNeighbour. The point \a rP must lie inside of one the given facets
     * are on the common edge. The two facets get broken into four facets, i.e.
     * that two new facets get created. If \a rP is coincident with a corner point
     * nothing happens.
     */
    bool SplitEdge(unsigned long ulFacetPos, unsigned long ulNeighbour, 
                   const Base::Vector3f& rP);
    /**
     * Splits the facet with index \a ulFacetPos on the edge side \a uSide into
     * two facets. This side must be an open edge otherwise nothing is done. The
     * point \a rP must be near to this edge and must not be coincident with any
     * corner vertices of the facet.
     */
    void SplitOpenEdge(unsigned long ulFacetPos, unsigned short uSide,
                       const Base::Vector3f& rP);
    /**
     * Splits the facet with index \a ulFacetPos into up to three facets. The points
     * \a rP1 and \a rP2 should lie on two different edges of the facet. This method
     * splits up the both neighbour facets as well.
     * If either \a rP1 or \a rP2 (probably due to a previous call of SplitFacet())
     * is coincident with a corner point then the facet is split into two facets.
     * If both points are coincident with corner points of this facet nothing is done.
     */
    void SplitFacet(unsigned long ulFacetPos, const Base::Vector3f& rP1,
                    const Base::Vector3f& rP2);
    /**
     * Collapse a vertex. At the moment only removing inner vertexes referenced
     * by three facets is supposrted.
     */
    bool CollapseVertex(const VertexCollapse& vc);
    /**
     * Checks whether a collapse edge operation is legal, that is fulfilled if none of the
     * adjacent facets flips its normal. If this operation is legal
     * true is returned, false is returned if this operation is illegal.
     */
    bool IsCollapseEdgeLegal(const EdgeCollapse& ec) const;
    /**
     * Collapses the common edge of two adjacent facets. This operation removes
     * one common point of the collapsed edge and the facets \a ulFacetPos and
     * \a ulNeighbour from the data structure.
     * @note If \a ulNeighbour is the neighbour facet on the i-th side of 
     * \a ulFacetPos then the i-th point is removed whereas i is 0, 1 or 2.
     * If the other common point should be removed then CollapseEdge()
     * should be invoked with swapped arguments of \a ulFacetPos and
     * \a ulNeighbour, i.e. CollapseEdge( \a ulNeighbour, \a ulFacetPos ).
     *
     * @note The client programmer must make sure that this is a legal operation.
     *
     * @note This method marks the facets and the point as 'invalid' but does not
     * remove them from the mesh structure, i.e. the mesh structure gets into an
     * inconsistent stage. To make the structure consistent again Cleanup() should
     * be called.
     * The reason why this cannot be done automatically is that it would become
     * quite slow if a lot of edges should be collapsed.
     *
     * @note While the mesh structure has invalid elements the client programmer
     * must take care not to use such elements.
     */
    bool CollapseEdge(unsigned long ulFacetPos, unsigned long ulNeighbour);
    /**
     * Convenience function that passes already all needed information.
     */
    bool CollapseEdge(const EdgeCollapse& ec);
    /**
     * Removes the facet with index \a ulFacetPos and all its neighbour facets.
     * The three vertices that are referenced by this facet are replaced by its
     * gravity point. 
     *
     * @note The client programmer must make sure that this is a legal operation.
     *
     * @note This method marks the facets and the point as 'invalid' but does not
     * remove them from the mesh structure, i.e. the mesh structure gets into an
     * inconsistent stage. To make the structure consistent again Cleanup() should
     * be called. 
     * The reason why this cannot be done automatically is that it would become
     * quite slow if a lot of facets should be collapsed.
     *
     * @note While the mesh structure has invalid elements the client programmer
     * must take care not to use such elements.
     */
    bool CollapseFacet(unsigned long ulFacetPos);
    //@}

    /** @name Topological Optimization */
    //@{
    /**
     * Tries to make a more beautiful mesh by swapping the common edge of two
     * adjacent facets where needed. 
     * \a fMaxAngle is the maximum allowed angle between the normals of two
     * adjacent facets to allow swapping the common edge. A too high value might
     * result into folds on the surface.
     * @note This is a high-level operation and tries to optimize the mesh as a whole.
     */
    void OptimizeTopology(float fMaxAngle);
    void OptimizeTopology();
    /**
     * Tries to make a more beautiful mesh by swapping the common edge of two
     * adjacent facets where needed. A swap is needed where two adjacent facets
     * don't fulfill the Delaunay condition.
     */
    void DelaunayFlip(float fMaxAngle);
    /**
     * Overloaded method DelaunayFlip that doesn't use ShouldSwapEdge to check for
     * legal swap edge.
     */
    int DelaunayFlip();
    /**
     * Tries to adjust the edges to the curvature direction with the minimum
     * absolute value of maximum and minimum curvature.
     * @note This is a high-level operation and tries to optimize the mesh as a
     * whole.
     */
    void AdjustEdgesToCurvatureDirection();
    //@}

    /**
     * Creates a new triangle with neighbour facet \a ulFacetPos and the vertex
     * \a rclPoint whereat it must lie outside the given facet.
     * @note The vertex \a rclPoint doesn't necessarily need to be a new vertex
     * it can already be part of another triangle but the client programmer must
     * make sure that no overlaps are created.
     * @note This operation might be useful to close gaps in a mesh.
     */
    bool SnapVertex(unsigned long ulFacetPos, const Base::Vector3f& rP);
    /**
     * Checks whether a swap edge operation is legal, that is fulfilled if the
     * two adjacent facets builds a convex polygon. If this operation is legal
     * true is returned, false is returned if this operation is illegal or if
     * \a ulFacetPos and \a ulNeighbour are not adjacent facets.
     */
    bool IsSwapEdgeLegal(unsigned long ulFacetPos, unsigned long ulNeighbour) const;
    /**
     * Checks whether the swap edge operation is legal and whether it makes
     * sense. This operation only makes sense if the maximum angle of both
     * facets is decreased and if the angle between the facet normals does
     * not exceed \a fMaxAngle.  
     */
    bool ShouldSwapEdge(unsigned long ulFacetPos, unsigned long ulNeighbour,
                        float fMaxAngle) const;
    /** Computes a value for the benefit of swapping the edge. */
    float SwapEdgeBenefit(unsigned long f, int e) const;
    /**
     * Removes all invalid marked elements from the mesh structure.
     */
    void Cleanup();
    /**
     * Removes the degenerated facet at position \a index from the mesh structure.
     * A facet is degenerated if its corner points are collinear.
     */
    void RemoveDegeneratedFacet(unsigned long index);
    /**
     * Removes the corrupted facet at position \a index from the mesh structure.
     * A facet is corrupted if the indices of its corner points are not all different.
     */
    void RemoveCorruptedFacet(unsigned long index);
    /**
     * Closes holes in the mesh that consists of up to \a length edges. In case a fit 
     * needs to be done then the points of the neighbours of \a level rings will be used.
     * Holes for which the triangulation failed are returned in \a aFailed.
     */
    void FillupHoles(unsigned long length, int level,
        AbstractPolygonTriangulator&,
        std::list<std::vector<unsigned long> >& aFailed);
    /**
     * This is an overloaded method provided for convenience. It takes as first argument
     * the boundaries which must be filled up.
     */
    void FillupHoles(int level, AbstractPolygonTriangulator&,
        const std::list<std::vector<unsigned long> >& aBorders,
        std::list<std::vector<unsigned long> >& aFailed);
    /**
     * Find holes which consists of up to \a length edges.
     */
    void FindHoles(unsigned long length,
        std::list<std::vector<unsigned long> >& aBorders) const;
    /**
     * Find topologic independent components with maximum \a count facets
     * and returns an array of the indices.
     */
    void FindComponents(unsigned long count, std::vector<unsigned long>& aInds);
    /**
     * Removes topologic independent components with maximum \a count facets.
     */
    void RemoveComponents(unsigned long count);
    /**
     * Harmonizes the normals.
     */
    void HarmonizeNormals (void);
    /** 
     * Flips the normals.
     */
    void FlipNormals (void);
    /**
     * Caching facility.
     */
    void BeginCache();
    void EndCache();

private:
    /**
     * Splits the neighbour facet of \a ulFacetPos on side \a uSide.
     */
    void SplitNeighbourFacet(unsigned long ulFacetPos, unsigned short uSide,
                             const Base::Vector3f rPoint);
    /**
     * Returns all facets that references the point index \a uPointPos. \a uFacetPos
     * is a facet that must reference this point and is added to the list as well.
     */
    std::vector<unsigned long> GetFacetsToPoint(unsigned long uFacetPos,
                                                unsigned long uPointPos) const;
    /** \internal */
    unsigned long GetOrAddIndex (const MeshPoint &rclPoint);

private:
    MeshKernel& _rclMesh;
    bool _needsCleanup;

    struct Vertex_Less  : public std::binary_function<const Base::Vector3f&,
                                                      const Base::Vector3f&, bool>
    {
        bool operator()(const Base::Vector3f& x, const Base::Vector3f& y) const;
    };

    // cache
    typedef std::map<Base::Vector3f,unsigned long,Vertex_Less> tCache;
    tCache* _cache;
};

/**
 * The MeshComponents class searches for topologic independent segments of the 
 * given mesh structure. 
 *
 * @author Werner Mayer
 */
class MeshExport MeshComponents
{
public:
    enum TMode {OverEdge, OverPoint};

    MeshComponents( const MeshKernel& rclMesh );
    ~MeshComponents();

    /**
     * Searches for 'isles' of the mesh. If \a tMode is \a OverEdge then facets
     * sharing the same edge are regarded as connected, if \a tMode is \a OverPoint
     * then facets sharing a common point are regarded as connected.
     */ 
    void SearchForComponents(TMode tMode, std::vector<std::vector<unsigned long> >& aclT) const;

    /**
     * Does basically the same as the method above escept that only the faces in
     * \a aSegment are regarded.
     */
    void SearchForComponents(TMode tMode, const std::vector<unsigned long>& aSegment,
                             std::vector<std::vector<unsigned long> >& aclT) const;

protected:
    // for sorting of elements
    struct CNofFacetsCompare : public std::binary_function<const std::vector<unsigned long>&, 
                                                           const std::vector<unsigned long>&, bool>
    {
        bool operator () (const std::vector<unsigned long> &rclC1, 
                          const std::vector<unsigned long> &rclC2)
        {
            return rclC1.size() > rclC2.size();
        }
    };
protected:
    const MeshKernel& _rclMesh;
};

} // namespace MeshCore

#endif // MESH_TOPOALGORITHM_H
