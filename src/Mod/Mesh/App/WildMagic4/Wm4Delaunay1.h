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

// A fancy class to sort a collection of real-valued numbers, but this
// provides some convenience for Delaunay2 and Delaunay3 when the input point
// set has intrinsic dimension smaller than the containing space.  The
// interface of Delaunay1 is also the model for those of Delaunay2 and
// Delaunay3.

#include "Wm4FoundationLIB.h"
#include "Wm4Delaunay.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM Delaunay1 : public Delaunay<Real>
{
public:
    // The input to the constructor is the array of vertices you want to sort.
    // If you want Delaunay1 to delete the array during destruction, set
    // bOwner to 'true'.  Otherwise, you own the array and must delete it
    // yourself.  TO DO:  The query type is currently ignored by this class.
    // Add support for the various types later.
    Delaunay1 (int iVertexQuantity, Real* afVertex, Real fEpsilon,
        bool bOwner, Query::Type eQueryType);
    virtual ~Delaunay1 ();

    // The input vertex array.
    const Real* GetVertices () const;

    // The functions listed here are valid only for dimension 1.

    // The convex hull of the vertices is an interval.  This function returns
    // the indices of the vertices that form the interval.  The return value
    // is 'true' iff the dimension is 1.
    bool GetHull (int aiIndex[2]);

    // Support for searching the sorted vertices for a interval that contains
    // P.  If there is a containing interval, the returned value is a index i
    // into the GetIndices() array with 0 <= i < GetSimplexQuantity().  If
    // there is not a containing segment, -1 is returned.
    int GetContainingSegment (const Real fP) const;

    // Get the vertices for segment i.  The function returns 'true' if i is a
    // valid segment index, in which case the vertices are valid.  Otherwise,
    // the function returns 'false' and the vertices are invalid.
    bool GetVertexSet (int i, Real afV[2]) const;

    // Get the vertex indices for segment i.  The function returns 'true' if
    // i is a valid segment index, in which case the vertices are valid.
    // Otherwise, the function returns 'false' and the vertices are invalid.
    bool GetIndexSet (int i, int aiIndex[2]) const;

    // Get the indices for segments adjacent to segment i.  The function
    // returns 'true' if i is a valid segment index, in which case the
    // adjacencies are valid.  Otherwise, the function returns 'false' and
    // the adjacencies are invalid.
    bool GetAdjacentSet (int i, int aiAdjacent[2]) const;

    // Compute the barycentric coordinates of P with respect to segment i.
    // The function returns 'true' if i is a valid segment index, in which
    // case the coordinates are valid.  Otherwise, the function returns
    // 'false' and the coordinate array is invalid.
    bool GetBarycentricSet (int i, const Real fP, Real afBary[2]) const;

    // Support for streaming to/from disk.
    Delaunay1 (const char* acFilename);
    bool Load (const char* acFilename);
    bool Save (const char* acFilename) const;

private:
    using Delaunay<Real>::m_iVertexQuantity;
    using Delaunay<Real>::m_iDimension;
    using Delaunay<Real>::m_iSimplexQuantity;
    using Delaunay<Real>::m_aiIndex;
    using Delaunay<Real>::m_aiAdjacent;
    using Delaunay<Real>::m_fEpsilon;
    using Delaunay<Real>::m_bOwner;

    Real* m_afVertex;

    class WM4_FOUNDATION_ITEM SortedVertex
    {
    public:
        Real Value;
        int Index;

        bool operator< (const SortedVertex& rkProj) const
        {
            return Value < rkProj.Value;
        }
    };
};

typedef Delaunay1<float> Delaunay1f;
typedef Delaunay1<double> Delaunay1d;

}