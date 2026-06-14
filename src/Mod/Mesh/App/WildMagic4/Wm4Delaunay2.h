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
// Version: 4.0.0 (2006/06/28)

#pragma once

#include "Wm4FoundationLIB.h"
#include "Wm4Delaunay1.h"
#include "Wm4DelTriangle.h"
#include "Wm4Query2.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM Delaunay2 : public Delaunay<Real>
{
public:
    // The input to the constructor is the array of vertices whose Delaunay
    // triangulation is required.  If you want Delaunay2 to delete the
    // vertices during destruction, set bOwner to 'true'.  Otherwise, you
    // own the vertices and must delete them yourself.
    //
    // You have a choice of speed versus accuracy.  The fastest choice is
    // Query::QT_INT64, but it gives up a lot of precision, scaling the points
    // to [0,2^{16}]^3.  The choice Query::QT_INTEGER gives up less precision,
    // scaling the points to [0,2^{20}]^3.  The choice Query::QT_RATIONAL uses
    // exact arithmetic, but is the slowest choice.  The choice Query::QT_REAL
    // uses floating-point arithmetic, but is not robust in all cases.

    Delaunay2 (int iVertexQuantity, Vector2<Real>* akVertex, Real fEpsilon,
        bool bOwner, Query::Type eQueryType);
    virtual ~Delaunay2 ();

    // The input vertex array.
    const Vector2<Real>* GetVertices () const;

    // The number of unique vertices processed.
    int GetUniqueVertexQuantity () const;

    // If GetDimension() returns 1, then the points lie on a line.  You must
    // create a Delaunay1 object using the function provided.
    const Vector2<Real>& GetLineOrigin () const;
    const Vector2<Real>& GetLineDirection () const;
    Delaunay1<Real>* GetDelaunay1 () const;

    // Locate those triangle edges that do not share other triangles.  The
    // returned quantity is the number of edges in the hull.  The returned
    // array has 2*quantity indices, each pair representing an edge.  The
    // edges are not ordered, but the pair of vertices for an edge is ordered
    // so that they conform to a counterclockwise traversal of the hull.  The
    // return value is 'true' iff the dimension is 2.
    bool GetHull (int& riEQuantity, int*& raiIndex);

    // Support for searching the triangulation for a triangle that contains
    // a point.  If there is a containing triangle, the returned value is a
    // triangle index i with 0 <= i < riTQuantity.  If there is not a
    // containing triangle, -1 is returned.
    int GetContainingTriangle (const Vector2<Real>& rkP) const;

    // If GetContainingTriangle returns a nonnegative value, the path of
    // triangles searched for the containing triangles is stored in an array.
    // The last index of the array is returned by GetPathLast; it is one
    // less than the number of array elements.  The array itself is returned
    // by GetPath.
    int GetPathLast () const;
    const int* GetPath () const;

    // If GetContainingTriangle returns -1, the path of triangles searched
    // may be obtained by GetPathLast and GetPath.  The input point is outside
    // an edge of the last triangle in the path.  This function returns the
    // vertex indices <v0,v1> of the edge, listed in counterclockwise order
    // relative to the convex hull of the data points.  The final output is
    // the index of the vertex v2 opposite the edge.  The return value of
    // the function is the index of the triple of vertex indices; the value
    // is 0, 1, or 2.
    int GetLastEdge (int& riV0, int& riV1, int& riV2) const;

    // Get the vertices for triangle i.  The function returns 'true' if i is
    // a valid triangle index, in which case the vertices are valid.
    // Otherwise, the function returns 'false' and the vertices are invalid.
    bool GetVertexSet (int i, Vector2<Real> akV[3]) const;

    // Get the vertex indices for triangle i.  The function returns 'true' if
    // i is a valid triangle index, in which case the vertices are valid.
    // Otherwise, the function returns 'false' and the vertices are invalid.
    bool GetIndexSet (int i, int aiIndex[3]) const;

    // Get the indices for triangles adjacent to triangle i.  The function
    // returns 'true' if i is a valid triangle index, in which case the
    // adjacencies are valid.  Otherwise, the function returns 'false' and
    // the adjacencies are invalid.
    bool GetAdjacentSet (int i, int aiAdjacent[3]) const;

    // Compute the barycentric coordinates of P with respect to triangle i.
    // The function returns 'true' if i is a valid triangle index, in which
    // case the coordinates are valid.  Otherwise, the function returns
    // 'false' and the coordinate array is invalid.
    bool GetBarycentricSet (int i, const Vector2<Real>& rkP, Real afBary[3])
        const;

    // Support for streaming to/from disk.
    Delaunay2 (const char* acFilename);
    bool Load (const char* acFilename);
    bool Save (const char* acFilename) const;

private:
    using Delaunay<Real>::m_eQueryType;
    using Delaunay<Real>::m_iVertexQuantity;
    using Delaunay<Real>::m_iDimension;
    using Delaunay<Real>::m_iSimplexQuantity;
    using Delaunay<Real>::m_aiIndex;
    using Delaunay<Real>::m_aiAdjacent;
    using Delaunay<Real>::m_fEpsilon;
    using Delaunay<Real>::m_bOwner;

    void Update (int i);
    DelTriangle<Real>* GetContainingTriangle (int i) const;
    void RemoveTriangles ();
    bool IsSupervertex (int i) const;

    // The input vertices.
    Vector2<Real>* m_akVertex;

    // The number of unique vertices processed.
    int m_iUniqueVertexQuantity;

    // The scaled input vertices with additional storage for the three
    // supertriangle vertices.  This array and supporting data structures
    // are for robust calculations.
    Vector2<Real>* m_akSVertex;
    Query2<Real>* m_pkQuery;
    Vector2<Real> m_kMin;
    Real m_fScale;

    // The indices for the three supertriangle vertices.
    int m_aiSV[3];

    // The current triangulation.
    std::set<DelTriangle<Real>*> m_kTriangle;

    // The line of containment if the dimension is 1.
    Vector2<Real> m_kLineOrigin, m_kLineDirection;

    // Store the path of tetrahedra visited in a GetContainingTetrahedron
    // function call.
    mutable int m_iPathLast;
    mutable int* m_aiPath;

    // If a query point is not in the convex hull of the input points, the
    // point is outside an edge of the last triangle in the search path.
    // These are the vertex indices for that edge.
    mutable int m_iLastEdgeV0, m_iLastEdgeV1;
    mutable int m_iLastEdgeOpposite, m_iLastEdgeOppositeIndex;
};

typedef Delaunay2<float> Delaunay2f;
typedef Delaunay2<double> Delaunay2d;

}