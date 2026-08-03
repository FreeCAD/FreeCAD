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
#include "Wm4Vector2.h"

namespace Wm4
{

template <class Real>
class Triangle2
{
public:
    // The triangle is represented as an array of three vertices, V0, V1,
    // and V2.

    // construction
    Triangle2 ();  // uninitialized
    Triangle2 (const Vector2<Real>& rkV0, const Vector2<Real>& rkV1,
        const Vector2<Real>& rkV2);
    Triangle2 (const Vector2<Real> akV[3]);

    // distance from the point Q to the triangle
    Real DistanceTo (const Vector2<Real>& rkQ) const;

    Vector2<Real> V[3];
};

}

#include "Wm4Triangle2.inl"

namespace Wm4
{
typedef Triangle2<float> Triangle2f;
typedef Triangle2<double> Triangle2d;

}