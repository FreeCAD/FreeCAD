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
#include "Wm4Vector3.h"

namespace Wm4
{

template <class Real>
class Box3
{
public:
    // A box has center C, axis directions U[0], U[1], and U[2] (all
    // unit-length vectors), and extents e[0], e[1], and e[2] (all nonnegative
    // numbers).  A point X = C+y[0]*U[0]+y[1]*U[1]+y[2]*U[2] is inside or
    // on the box whenever |y[i]| <= e[i] for all i.

    // construction
    Box3 ();  // uninitialized
    Box3 (const Vector3<Real>& rkCenter, const Vector3<Real>* akAxis,
        const Real* afExtent);
    Box3 (const Vector3<Real>& rkCenter, const Vector3<Real>& rkAxis0,
        const Vector3<Real>& rkAxis1, const Vector3<Real>& rkAxis2,
        Real fExtent0, Real fExtent1, Real fExtent2);

    void ComputeVertices (Vector3<Real> akVertex[8]) const;

    Vector3<Real> Center;
    Vector3<Real> Axis[3];  // must be an orthonormal set of vectors
    Real Extent[3];         // must be nonnegative
};

} //namespace Wm4

#include "Wm4Box3.inl"

namespace Wm4
{
typedef Box3<float> Box3f;
typedef Box3<double> Box3d;
}