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
#include "Wm4Vector2.h"

namespace Wm4
{

template <class Real>
class Box2
{
public:
    // A box has center C, axis directions U[0] and U[1] (both unit-length
    // vectors), and extents e[0] and e[1] (both nonnegative numbers).  A
    // point X = C+y[0]*U[0]+y[1]*U[1] is inside or on the box whenever
    // |y[i]| <= e[i] for all i.

    // construction
    Box2 ();  // uninitialized
    Box2 (const Vector2<Real>& rkCenter, const Vector2<Real>* akAxis,
        const Real* afExtent);
    Box2 (const Vector2<Real>& rkCenter, const Vector2<Real>& rkAxis0,
        const Vector2<Real>& rkAxis1, Real fExtent0, Real fExtent1);

    void ComputeVertices (Vector2<Real> akVertex[4]) const;

    Vector2<Real> Center;
    Vector2<Real> Axis[2];  // must be an orthonormal set of vectors
    Real Extent[2];         // must be nonnegative
};

#include "Wm4Box2.inl"

typedef Box2<float> Box2f;
typedef Box2<double> Box2d;

}