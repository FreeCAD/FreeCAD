// SPDX-License-Identifier: LGPL-2.1-or-later

// Wild Magic Source Code
// David Eberly
// http://www.geometrictools.com
// Copyright (c) 1998-2007
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or (at
// your option) any later version.  The license is available for reading at
// either of the locations:
//     http://www.gnu.org/copyleft/lgpl.html
//     http://www.geometrictools.com/License/WildMagicLicense.pdf
// The license applies to versions 0 through 4 of Wild Magic.
//
// Version: 4.0.2 (2006/08/30)

#pragma once

#include "Wm4FoundationLIB.h"
#include "Wm4Query2.h"
#include "Wm4Vector2.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM TriangulateEC
{
public:
    // This class implements triangulation of polygons using ear clipping.
    // The method is O(n^2) for n input points.  There are five constructors.
    // The query type and epsilon parameters are discussed later.  In all
    // cases, the output is
    //
    //    triangles:
    //        An array of 3*T indices representing T triangles.  Each triple
    //        (i0,i1,i2) corresponds to the triangle (P[i0],P[i1],P[i2]),
    //        where P is the 'positions' input.  These triangles are all
    //        counterclockwise ordered.
    //
    // TriangulateEC(positions,queryType,epsilon,triangles)
    //    positions:
    //        An array of n vertex positions for a simple polygon.  The
    //        polygon is (P[0],P[1],...,P[n-1]).
    //               
    // TriangulateEC(positions,queryType,epsilon,polygon,triangles)
    //    positions:
    //        An array of vertex positions, not necessarily the exact set of
    //        positions for the polygon vertices.
    //    polygon:
    //        An array of n indices into 'positions'.  If the array is
    //        (I[0],I[1],...,I[n-1]), the polygon vertices are
    //        (P[I[0]],P[I[1]],...,P[I[n-1]]).
    //               
    // TriangulateEC(positions,queryType,epsilon,outerPolygon,innerPolygon,
    //               triangles)
    //    positions:
    //        An array of vertex positions, not necessarily the exact set of
    //        positions for the polygon vertices.
    //    outerPolygon:
    //        An array of n indices into 'positions' for the outer polygon.
    //        If the array is (I[0],I[1],...,I[n-1]), the outer polygon
    //        vertices are (P[I[0]],P[I[1]],...,P[I[n-1]]).
    //    innerPolygon:
    //        An array of m indices into 'positions' for the inner polygon.
    //        The inner polygon must be strictly inside the outer polygon.
    //        If the array is (J[0],J[1],...,J[m-1]), the inner polygon
    //        vertices are (P[J[0]],P[J[1]],...,P[J[m-1]]).
    //               
    // TriangulateEC(positions,queryType,epsilon,outerPolygon,innerPolygons,
    //               triangles)
    //    positions:
    //        An array of vertex positions, not necessarily the exact set of
    //        positions for the polygon vertices.
    //    outerPolygon:
    //        An array of n indices into 'positions' for the outer polygon.
    //        If the array is (I[0],I[1],...,I[n-1]), the outer polygon
    //        vertices are (P[I[0]],P[I[1]],...,P[I[n-1]]).
    //    innerPolygons:
    //        An array of arrays of indices, each index array representing
    //        an inner polygon.  The inner polygons must be nonoverlapping and
    //        strictly contained in the outer polygon.  If innerPolygons[i]
    //        is the array (J[0],J[1],...,J[m-1]), the inner polygon
    //        vertices are (P[J[0]],P[J[1]],...,P[J[m-1]]).
    //               
    // TriangulateEC(positions,queryType,epsilon,tree,triangles)
    //    positions:
    //        An array of vertex positions, not necessarily the exact set of
    //        positions for the polygon vertices.
    //    tree:
    //        A hierarchy of nested polygons.  The root node corresponds to
    //        the outermost outer polygon.  The child nodes of the root
    //        correspond to inner polygons contained by the outer polygon.
    //        Each inner polygon may itself contain outer polygons, and
    //        those outer polygons may themselves contain inner polygons.
    //
    // You have a choice of speed versus accuracy.  The fastest choice is
    // Query::QT_INT64, but it gives up a lot of precision, scaling the points
    // to [0,2^{20}]^3.  The choice Query::QT_INTEGER gives up less precision,
    // scaling the points to [0,2^{24}]^3.  The choice Query::QT_RATIONAL uses
    // exact arithmetic, but is the slowest choice.  The choice Query::QT_REAL
    // uses floating-point arithmetic, but is not robust in all cases.  The
    // choice Query::QT_FILTERED uses floating-point arithmetic to compute
    // determinants whose signs are of interest.  If the floating-point value
    // is nearly zero, the determinant is recomputed using exact rational
    // arithmetic in order to correctly classify the sign.  The hope is to
    // have an exact result computed faster than with all rational arithmetic.
    // The value fEpsilon is used only for the Query::QT_FILTERED case and
    // should be in [0,1].  If 0, the method defaults to all exact rational
    // arithmetic.  If 1, the method defaults to all floating-point
    // arithmetic.  Generally, if M is the maximum absolute value of a
    // determinant and if d is the current determinant value computed as a
    // floating-point quantity, the recalculation with rational arithmetic
    // occurs when |d| < epsilon*M.

    // Convenient typedefs.
    typedef std::vector<Vector2<Real> > Positions;
    typedef std::vector<int> Indices;
    typedef std::vector<Indices*> IndicesArray;
    typedef std::map<int,int> IndexMap;

    // The input rkPosition represents an array of vertices for a simple
    // polygon. The vertices are rkPosition[0] through rkPosition[n-1], where
    // n = rkPolygon.size(), and are listed in counterclockwise order.
    TriangulateEC (const Positions& rkPositions, Query::Type eQueryType,
        Real fEpsilon, Indices& rkTriangles);

    // The input rkPositions represents an array of vertices that contains the
    // vertices of a simple polygon.  The input rkPolygon represents a simple
    // polygon whose vertices are rkPositions[rkPolygon[0]] through
    // rkPositions[rkPolygon[m-1]], where m = rkPolygon.size(), and are listed
    // in counterclockwise ordered.
    TriangulateEC (const Positions& rkPositions, Query::Type eQueryType,
        Real fEpsilon, const Indices& rkPolygon, Indices& rkTriangles);

    // The input rkPositions is a shared array of vertices that contains the
    // vertices for two simple polygons, an outer polygon and an inner
    // polygon.  The inner polygon must be strictly inside the outer polygon.
    // The input rkOuter represents the outer polygon whose vertices are
    // rkPositions[rkOuter[0]] through rkPositions[rkOuter[n-1]], where
    // n = rkOuter.size(), and are listed in counterclockwise order.  The
    // input rkInner represents the inner polygon whose vertices are
    // rkPositions[rkInner[0]] through rkPositions[rkInner[m-1]], where
    // m = rkInner.size(), and are listed in clockwise order.
    TriangulateEC (const Positions& rkPositions, Query::Type eQueryType,
        Real fEpsilon, const Indices& rkOuter, const Indices& rkInner,
        Indices& rkTriangles);

    // The input rkPositions is a shared array of vertices that contains the
    // vertices for multiple simple polygons, an outer polygon and one or more
    // inner polygons.  The inner polygons must be nonoverlapping and strictly
    // inside the outer polygon.  The input rkOuter represents the outer
    // polygon whose vertices are rkPositions[rkOuter[0]] through
    // rkPositions[rkOuter[n-1]], where n = rkOuter.size(), and are listed in
    // counterclockwise order.  The input element rkInners[i] represents the
    // i-th inner polygon whose vertices are rkPositions[rkInners[i][0]]
    // through rkPositions[rkInners[i][m-1]], where m = rkInners[i].size(),
    // and are listed in clockwise order.
    TriangulateEC (const Positions& rkPositions, Query::Type eQueryType,
        Real fEpsilon, const Indices& rkOuter, const IndicesArray& rkInners,
        Indices& rkTriangles);

    // A tree of nested polygons.  The root node corresponds to an outer
    // polygon.  The children of the root correspond to inner polygons, which
    // are nonoverlapping polygons strictly contained in the outer polygon.
    // Each inner polygon may itself contain an outer polygon, thus leading
    // to a hierarchy of polygons.  The outer polygons have vertices listed
    // in counterclockwise order.  The inner polygons have vertices listed in
    // clockwise order.
    class WM4_FOUNDATION_ITEM Tree
    {
    public:
        Indices Polygon;
        std::vector<Tree*> Child;
    };

    // The input rkPositions is a shared array of vertices that contains the
    // vertices for multiple simple polygons in a tree of polygons.
    TriangulateEC (const Positions& rkPositions, Query::Type eQueryType,
        Real fEpsilon, const Tree* pkTree, Indices& rkTriangles);

    ~TriangulateEC ();

    // Helper function to delete Tree objects.  Call this only if all tree
    // nodes were dynamically allocated.
    static void Delete (Tree*& rpkRoot);

private:
    // Create the query object and scaled positions to be used during
    // triangulation.  Extra elements are required when triangulating polygons
    // with holes.  These are preallocated to avoid dynamic resizing during
    // the triangulation.
    void InitializePositions (const Positions& rkPositions,
        Query::Type eQueryType, Real fEpsilon, int iExtraElements);

    // Create the vertex objects that store the various lists required by the
    // ear-clipping algorithm.
    void InitializeVertices (int iVQuantity, const int* aiIndex,
        Indices& rkTriangles);

    // Apply ear clipping to the input polygon.  Polygons with holes are
    // preprocessed to obtain an index array that is nearly a simple polygon.
    // This outer polygon has a pair of coincident edges per inner polygon.
    void DoEarClipping (int iVQuantity, const int* aiIndex,
        Indices& rkTriangles);

    // This function is used to help determine a pair of visible vertices
    // for the purpose of triangulating polygons with holes.  The query is
    // point-in-triangle, but is encapsulated here to use the same type of
    // query object that the user specified in the constructors.
    int TriangleQuery (const Vector2<Real>& rkPosition,
        Query::Type eQueryType, Real fEpsilon,
        const Vector2<Real> akTriangle[3]) const;

    // Given an outer polygon that contains an inner polygon, this function
    // determines a pair of visible vertices and inserts two coincident edges
    // to generate a nearly simple polygon.
    void CombinePolygons (Query::Type eQueryType, Real fEpsilon,
        int iNextElement, const Indices& rkOuter, const Indices& rkInner,
        IndexMap& rkMap, Indices& rkCombined);

    // Two extra elements are needed in the position array per outer-inners
    // polygon.  This function computes the total number of extra elements
    // needed for the input tree.  This number is passed to the function
    // InitializePositions.
    static int GetExtraElements (const Tree* pkTree);

    // Given an outer polygon that contains a set of nonoverlapping inner
    // polygons, this function determines pairs of visible vertices and
    // inserts coincident edges to generate a nearly simple polygon.  It
    // repeatedly calls CombinePolygons for each inner polygon of the outer
    // polygon.
    void ProcessOuterAndInners (Query::Type eQueryType, Real fEpsilon,
        const Indices& rkOuter, const IndicesArray& rkInners,
        int& rkNextElement, IndexMap& rkMap, Indices& rkCombined);

    // The insertion of coincident edges to obtain a nearly simple polygon
    // requires duplication of vertices in order that the ear-clipping
    // algorithm work correctly.  After the triangulation, the indices of
    // the duplicated vertices are converted to the original indices.
    void RemapIndices (const IndexMap& rkMap, Indices& rkTriangles) const;

    // Doubly linked lists for storing specially tagged vertices.
    class WM4_FOUNDATION_ITEM Vertex
    {
    public:
        Vertex ()
        {
            Index = -1;
            IsConvex = false;
            IsEar = false;
            VPrev = -1;
            VNext = -1;
            SPrev = -1;
            SNext = -1;
            EPrev = -1;
            ENext = -1;
        }

        int Index;  // index of vertex in position array
        bool IsConvex, IsEar;
        int VPrev, VNext; // vertex links for polygon
        int SPrev, SNext; // convex/reflex vertex links (disjoint lists)
        int EPrev, ENext; // ear links
    };

    Vertex& V (int i);
    bool IsConvex (int i);
    bool IsEar (int i);
    void InsertAfterC (int i);   // insert convex vertex
    void InsertAfterR (int i);   // insert reflex vertesx
    void InsertEndE (int i);     // insert ear at end of list
    void InsertAfterE (int i);   // insert ear after efirst
    void InsertBeforeE (int i);  // insert ear before efirst
    void RemoveV (int i);        // remove vertex
    int  RemoveE (int i);        // remove ear at i
    void RemoveR (int i);        // remove reflex vertex

    // The doubly linked list.
    std::vector<Vertex> m_kVertex;
    int m_iCFirst, m_iCLast;  // linear list of convex vertices
    int m_iRFirst, m_iRLast;  // linear list of reflex vertices
    int m_iEFirst, m_iELast;  // cyclical list of ears

    // For robust determinant calculation.
    Query2<Real>* m_pkQuery;
    std::vector<Vector2<Real> >m_kSPositions;
};

}