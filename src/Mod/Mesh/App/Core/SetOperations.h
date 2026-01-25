// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2005 Berthold Grupp                                     *
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

#include <list>
#include <map>
#include <set>

#include <Base/Builder3D.h>

#include "Iterator.h"
#include "MeshKernel.h"
#include "Visitor.h"


// forward declarations

namespace MeshCore
{

class MeshGeomFacet;
class MeshGeomEdge;
class MeshBuilder;
class MeshKernel;
class MeshFacetGrid;
class MeshFacetArray;
class MeshFacetIterator;

/**
 * The MeshAlgorithm class provides algorithms base on meshes.
 */
class MeshExport SetOperations
{
public:
    enum OperationType
    {
        Union,
        Intersect,
        Difference,
        Inner,
        Outer
    };

    /// Construction
    SetOperations(
        const MeshKernel& cutMesh1,
        const MeshKernel& cutMesh2,
        MeshKernel& result,
        OperationType opType,
        float minDistanceToPoint = 1e-5F
    );

public:
    /** Cut this mesh with another one. The result is a list of polylines
     * If the distance of the polyline to one of the points is less than minDistanceToPoint the
     * polyline goes direct to the point
     */
    void Do();

private:
    const MeshKernel& _cutMesh0;  /** Mesh for set operations source 1 */
    const MeshKernel& _cutMesh1;  /** Mesh for set operations source 2 */
    MeshKernel& _resultMesh;      /** Result mesh */
    OperationType _operationType; /** Set Operation Type */
    float _minDistanceToPoint;    /** Minimal distance to facet corner points */

private:
    // Helper class cutting edge to its two attached facets
    class Edge
    {
    public:
        MeshPoint pt1, pt2;  // edge

        Edge() = default;

        Edge(MeshPoint p1, MeshPoint p2)
        {
            if (p1 < p2) {
                pt1 = p1;
                pt2 = p2;
            }
            else {
                pt2 = p1;
                pt1 = p2;
            }
        }

        bool operator==(const Edge& edge) const
        {
            return (pt1 == edge.pt1) && (pt2 == edge.pt2);
        }

        bool operator<(const Edge& edge) const
        {
            return (pt1 == edge.pt1) ? (pt2 < edge.pt2) : (pt1 < edge.pt1);
        }
    };

    class EdgeInfo
    {
    public:
        int fcounter[2] {};          // counter of facets attacted to the edge
        MeshGeomFacet facets[2][2];  // Geom-Facets attached to the edge
        FacetIndex facet[2] {};      // underlying Facet-Index
    };

    // class CollectFacetVisitor : public MeshFacetVisitor
    //{
    //   public:
    //     std::vector<unsigned long> &_facets;
    //     MeshKernel                 &_mesh;
    //     std::map<Edge, EdgeInfo>   &_edges;
    //     int                         _side;
    //     float                       _mult;
    //     int                         _addFacets; // 0: add facets to the result 1: do not add
    //     facets to the result Base::Builder3D& _builder;

    //    CollectFacetVisitor (MeshKernel& mesh, std::vector<unsigned long>& facets, std::map<Edge,
    //    EdgeInfo>& edges, int side, float mult, Base::Builder3D& builder); bool Visit (MeshFacet
    //    &rclFacet, const MeshFacet &rclFrom, unsigned long ulFInd, unsigned long ulLevel); bool
    //    AllowVisit (MeshFacet& rclFacet, MeshFacet& rclFrom, unsigned long ulFInd, unsigned long
    //    ulLevel, unsigned short neighbourIndex);
    //};

    class CollectFacetVisitor: public MeshFacetVisitor
    {
    public:
        std::vector<FacetIndex>& _facets;
        const MeshKernel& _mesh;
        std::map<Edge, EdgeInfo>& _edges;
        int _side;
        float _mult;
        int _addFacets {-1};  // 0: add facets to the result 1: do not add facets to the result
        Base::Builder3D& _builder;

        CollectFacetVisitor(
            const MeshKernel& mesh,
            std::vector<FacetIndex>& facets,
            std::map<Edge, EdgeInfo>& edges,
            int side,
            float mult,
            Base::Builder3D& builder
        );
        bool Visit(
            const MeshFacet& rclFacet,
            const MeshFacet& rclFrom,
            FacetIndex ulFInd,
            unsigned long ulLevel
        ) override;
        bool AllowVisit(
            const MeshFacet& rclFacet,
            const MeshFacet& rclFrom,
            FacetIndex ulFInd,
            unsigned long ulLevel,
            unsigned short neighbourIndex
        ) override;
    };

    /** all points from cut */
    std::set<MeshPoint> _cutPoints;
    /** all edges */
    std::map<Edge, EdgeInfo> _edges;
    /** map from facet index to its cut points (mesh 1 and mesh 2) Key: Facet-Index  Value: List of
     * iterators of set<MeshPoint> */
    std::map<FacetIndex, std::list<std::set<MeshPoint>::iterator>> _facet2points[2];
    /** Facets collected from region growing */
    std::vector<MeshGeomFacet> _facetsOf[2];

    std::vector<MeshGeomFacet> _newMeshFacets[2];

    /** Cut mesh 1 with mesh 2 */
    void Cut(std::set<FacetIndex>& facetsCuttingEdge0, std::set<FacetIndex>& facetsCuttingEdge1);
    /** Trianglute each facets cut with its cutting points */
    void TriangulateMesh(const MeshKernel& cutMesh, int side);
    /** search facets for adding (with region growing) */
    void CollectFacets(int side, float mult);
    /** close gap in the mesh */
    void CloseGaps(MeshBuilder& meshBuilder);

    /** visual debugger */
    Base::Builder3D _builder;
};

/*!
  Determine the intersections between two meshes.
*/
class MeshExport MeshIntersection
{
public:
    struct Tuple
    {
        Base::Vector3f p1, p2;
        FacetIndex f1, f2;
    };
    struct Triple
    {
        Base::Vector3f p;
        FacetIndex f1, f2;
    };
    struct Pair
    {
        Base::Vector3f p;
        FacetIndex f;
    };

    MeshIntersection(const MeshKernel& m1, const MeshKernel& m2, float dist)
        : kernel1(m1)
        , kernel2(m2)
        , minDistance(dist)
    {}

    bool hasIntersection() const;
    void getIntersection(std::list<Tuple>&) const;
    /*!
      From an unsorted list of intersection points make a list of sorted intersection points. If
      parameter \a onlyclosed is set to true then only closed intersection curves are taken and all
      other curves are filtered out.
     */
    void connectLines(bool onlyclosed, const std::list<Tuple>&, std::list<std::list<Triple>>&);

private:
    static bool testIntersection(const MeshKernel& k1, const MeshKernel& k2);

private:
    const MeshKernel& kernel1;
    const MeshKernel& kernel2;
    float minDistance;
};


}  // namespace MeshCore
