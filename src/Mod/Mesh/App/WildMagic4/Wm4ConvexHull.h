// SPDX-License-Identifier: BSL-1.0

// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 4.10.0 (2009/11/18)

#pragma once

#include "Wm4FoundationLIB.h"
#include "Wm4System.h"
#include "Wm4Query.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM ConvexHull
{
public:
    // Abstract base class.
    virtual ~ConvexHull ();

    // Member accessors.  For notational purposes in this class documentation,
    // The number of vertices is VQ and the vertex array is V.
    int GetQueryType () const;
    int GetVertexQuantity () const;
    Real GetEpsilon () const;
    bool GetOwner () const;

    // The dimension of the result, call it d.  If n is the dimension of the
    // space of the input points, then 0 <= d <= n.
    int GetDimension () const;

    // The interpretations of the return values of these functions depends on
    // the dimension.  Generally, SQ = GetSimplexQuantity() is the number of
    // simplices in the mesh.  The array I is returned by GetIndices().
    int GetSimplexQuantity () const;
    const int* GetIndices () const;

    // Dimension d = 0.
    //   SQ = 1
    //   I  = null (use index zero for vertices)

    // Dimension d = 1.
    //   SQ = 2
    //   I = array of two indices
    // The segment has end points
    //   vertex[0] = V[I[2*i+0]]
    //   vertex[1] = V[I[2*i+1]]

    // Dimension d = 2.
    //   SQ = number of convex polygon edges
    //   I  = array of into V that represent the convex polygon edges
    //        (SQ total elements)
    // The i-th edge has vertices
    //   vertex[0] = V[I[2*i+0]]
    //   vertex[1] = V[I[2*i+1]]

    // Dimension d = 3.
    //   SQ = number of convex polyhedron triangular faces
    //   I  = array of 3-tuples of indices into V that represent the
    //        triangles (3*SQ total elements)
    // The i-th face has vertices
    //   vertex[0] = V[I[3*i+0]]
    //   vertex[1] = V[I[3*i+1]]
    //   vertex[2] = V[I[3*i+2]]

protected:
    // Abstract base class.  The number of vertices to be processed is
    // iVQuantity.  The value of fEpsilon is a tolerance used for determining
    // the intrinsic dimension of the input set of vertices.  Ownership of the
    // input points to the constructors for the derived classes may be
    // transferred to this class.  If you want the input vertices to be
    // deleted by this class, set bOwner to 'true'; otherwise, you own the
    // array and must delete it yourself.
    ConvexHull (int iVertexQuantity, Real fEpsilon, bool bOwner,
        Query::Type eQueryType);

    // Support for streaming to/from disk.
    bool Load (FILE* pkIFile);
    bool Save (FILE* pkOFile) const;

    Query::Type m_eQueryType;
    int m_iVertexQuantity;
    int m_iDimension;
    int m_iSimplexQuantity;
    int* m_aiIndex;
    Real m_fEpsilon;
    bool m_bOwner;
};

typedef ConvexHull<float> ConvexHullf;
typedef ConvexHull<double> ConvexHulld;

}