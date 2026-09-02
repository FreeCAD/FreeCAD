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
#include "Wm4System.h"

namespace Wm4
{

template <int N, class Real>
class UniqueVerticesTriangles
{
public:
    // Construction and destruction.  A vertex is an N-tuple of Real values,
    // usually starting with position and optionally followed by attributes
    // such as normal vector, colors, and texture coordinates.
    //
    // TO DO:  Allow the user to specify an epsilon e > 0 so that vertices V0
    // and V1 are considered to be the same when |V1-V0| <= e.  The current
    // code uses e = 0.
    
    // Triangle soup.  The input vertex array consists of triples of vertices,
    // each triple representing a triangle.  The array akInVertex must have
    // 3*iTQuantity tuples.  The caller is responsible for deleting the input
    // vertex array if it was dynamically allocated.  An array rakOutVertex of
    // riOutVQuantity unique vertices and an array aiOutIndex of
    // riOutTQuantity unique index triples are computed; raiOutIndex has
    // 3*iTQuantity elements.  The indices are relative to the array of unique
    // vertices and each index triple represents a triangle.  The output
    // arrays are dynamically allocated.  The caller is responsible for
    // deleting them.
    UniqueVerticesTriangles (int iTQuantity,
        const TTuple<N,Real>* akInVertex, int& riOutVQuantity,
        TTuple<N,Real>*& rakOutVertex, int*& raiOutIndex);

    // Indexed triangles.  The input vertex array consists of all vertices
    // referenced by the input index array.  The array akInVertex must have
    // iInVQuantity tuples.  The array aiInIndex must have 3*iTQuantity
    // elements.  The caller is responsible for deleting the input arrays if
    // they were dynamically allocated.  An array rakOutVertex of
    // riOutVQuantity unique vertices and an array aiOutIndex of iTQuantity
    // unique index triples are computed; raiOutIndex has 3*iTQuantity
    // elements.  The indices are relative to the array of unique
    // vertices and each index triple represents a triangle.  The output
    // arrays are dynamically allocated.  The caller is responsible for
    // deleting them.
    UniqueVerticesTriangles (int iInVQuantity,
        const TTuple<N,Real>* akInVertex, int iTQuantity,
        const int* aiInIndex, int& riOutVQuantity,
        TTuple<N,Real>*& rakOutVertex, int*& raiOutIndex);

    ~UniqueVerticesTriangles ();

    // The input vertices have indices 0 <= i < VINum.  The output vertices
    // have indices 0 <= j < VONum.  The construction leads to a mapping of
    // input indices i to output indices j.  Duplicate vertices have different
    // input indices but the same output index.  The following function gives
    // you access to the mapping.  If the input index is invalid (i < 0 or
    // i >= VINum), the return value is -1.
    int GetOutputIndexFor (int iInputIndex) const;

private:
    void ConstructUniqueVertices (int iInVQuantity,
        const TTuple<N,Real>* akInVertex, int& raiOutVQuantity,
        TTuple<N,Real>*& rakOutVertex);

    int m_iInVQuantity, m_iOutVQuantity;
    int* m_aiInToOutMapping;
};

}

#include "Wm4UniqueVerticesTriangles.inl"