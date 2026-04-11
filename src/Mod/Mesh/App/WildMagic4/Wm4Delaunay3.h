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
#include "Wm4Delaunay2.h"
#include "Wm4DelTetrahedron.h"
#include "Wm4Query3.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM Delaunay3 : public Delaunay<Real>
{
public:
    // The input to the constructor is the array of vertices whose Delaunay
    // tetrahedralization is required.  If you want Delaunay3 to delete the
    // vertices during destruction, set bOwner to 'true'.  Otherwise, you
    // own the vertices and must delete them yourself.  Before using this
    // class, you should "clean" your input points by removing duplicates.
    //
    // You have a choice of speed versus accuracy.  The fastest choice is
    // Query::QT_INT64, but it gives up a lot of precision, scaling the points
    // to [0,2^{10}]^3.  The choice Query::QT_INTEGER gives up less precision,
    // scaling the points to [0,2^{20}]^3.  The choice Query::QT_RATIONAL uses
    // exact arithmetic, but is the slowest choice.  The choice Query::QT_REAL
    // uses floating-point arithmetic, but is not robust in all cases.

    Delaunay3 (int iVertexQuantity, Vector3<Real>* akVertex, Real fEpsilon,
        bool bOwner, Query::Type eQueryType);
    virtual ~Delaunay3 ();

    // The input vertex array.
    const Vector3<Real>* GetVertices () const;

    // The number of unique vertices processed.
    int GetUniqueVertexQuantity () const;

    // If GetDimension() returns 1, then the points lie on a line.  You must
    // create a Delaunay1 object using the function provided.
    const Vector3<Real>& GetLineOrigin () const;
    const Vector3<Real>& GetLineDirection () const;
    Delaunay1<Real>* GetDelaunay1 () const;

    // If GetDimension() returns 2, then the points lie on a plane.  The plane
    // has two direction vectors (inputs 0 or 1).  You must create a Delaunay2
    // object using the function provided.
    const Vector3<Real>& GetPlaneOrigin () const;
    const Vector3<Real>& GetPlaneDirection (int i) const;
    Delaunay2<Real>* GetDelaunay2 () const;

    // Locate those tetrahedra faces that do not share other tetrahedra.
    // The returned quantity is the number of triangles in the hull.  The
    // returned array has 3*quantity indices, each triple representing a
    // triangle.  The triangles are counterclockwise ordered when viewed
    // from outside the hull.  The return value is 'true' iff the dimension
    // is 3.
    bool GetHull (int& riTQuantity, int*& raiIndex) const;

    // Support for searching the tetrahedralization for a tetrahedron that
    // contains a point.  If there is a containing tetrahedron, the returned
    // value is a tetrahedron index i with 0 <= i < riTQuantity.  If there is
    // not a containing tetrahedron, -1 is returned.
    int GetContainingTetrahedron (const Vector3<Real>& rkP) const;

    // If GetContainingTetrahedron returns a nonnegative value, the path of
    // tetrahedra searched for the containing tetrahedra is stored in an
    // array.  The last index of the array is returned by GetPathLast; it is
    // one less than the number of array elements.  The array itself is
    // returned by GetPath.
    int GetPathLast () const;
    const int* GetPath () const;

    // If GetContainingTetrahedron returns -1, the path of tetrahedra
    // searched may be obtained by GetPathLast and GetPath.  The input point
    // is outside a face of the last tetrahedron in the path.  This function
    // returns the vertex indices <v0,v1,v2> of the face, listed in
    // counterclockwise order relative to the convex hull of the data points
    // as viewed by an outside observer.  The final output is the index of the
    // vertex v3 opposite the face.  The return value of the function is the
    // index of the quadruple of vertex indices; the value is 0, 1, 2, or 3.
    int GetLastFace (int& riV0, int& riV1, int& riV2, int& riV3) const;

    // Get the vertices for tetrahedron i.  The function returns 'true' if i
    // is a valid tetrahedron index, in which case the vertices are valid.
    // Otherwise, the function returns 'false' and the vertices are invalid.
    bool GetVertexSet (int i, Vector3<Real> akV[4]) const;

    // Get the vertex indices for tetrahedron i.  The function returns 'true'
    // if i is a valid tetrahedron index, in which case the vertices are
    // valid.  Otherwise, the function returns 'false' and the vertices are
    // invalid.
    bool GetIndexSet (int i, int aiIndex[4]) const;

    // Get the indices for tetrahedra adjacent to tetrahedron i.  The function
    // returns 'true' if i is a valid tetrahedron index, in which case the
    // adjacencies are valid.  Otherwise, the function returns 'false' and
    // the adjacencies are invalid.
    bool GetAdjacentSet (int i, int aiAdjacent[4]) const;

    // Compute the barycentric coordinates of P with respect to tetrahedron i.
    // The function returns 'true' if i is a valid tetrahedron index, in which
    // case the coordinates are valid.  Otherwise, the function returns
    // 'false' and the coordinate array is invalid.
    bool GetBarycentricSet (int i, const Vector3<Real>& rkP, Real afBary[4])
        const;

    // Support for streaming to/from disk.
    Delaunay3 (const char* acFilename);
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
    DelTetrahedron<Real>* GetContainingTetrahedron (int i) const;
    void RemoveTetrahedra ();
    bool IsSupervertex (int i) const;
    bool SharesFace (int i, DelTetrahedron<Real>* pkFace,
        DelTetrahedron<Real>* pkAdj);

    // The input vertices.
    Vector3<Real>* m_akVertex;

    // The number of unique vertices processed.
    int m_iUniqueVertexQuantity;

    // The scaled input vertices with additional storage for the four
    // supertetrahedron vertices.  This array and supporting data structures
    // are for robust calculations.
    Vector3<Real>* m_akSVertex;
    Query3<Real>* m_pkQuery;
    Vector3<Real> m_kMin;
    Real m_fScale;

    // The indices for the three supertetrahedron vertices.
    int m_aiSV[4];

    // The current tetrahedralization.
    std::set<DelTetrahedron<Real>*> m_kTetrahedron;

    // The line of containment if the dimension is 1.
    Vector3<Real> m_kLineOrigin, m_kLineDirection;

    // The plane of containment if the dimension is 2.
    Vector3<Real> m_kPlaneOrigin, m_akPlaneDirection[2];

    // Store the path of tetrahedra visited in a GetContainingTetrahedron
    // function call.
    mutable int m_iPathLast;
    mutable int* m_aiPath;

    // If a query point is not in the convex hull of the input points, the
    // point is outside a face of the last tetrahedron in the search path.
    // These are the vertex indices for that face.
    mutable int m_iLastFaceV0, m_iLastFaceV1, m_iLastFaceV2;
    mutable int m_iLastFaceOpposite, m_iLastFaceOppositeIndex;
};

typedef Delaunay3<float> Delaunay3f;
typedef Delaunay3<double> Delaunay3d;

}