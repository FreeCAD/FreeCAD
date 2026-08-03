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
#include "Wm4Plane3.h"

namespace Wm4
{

// Least-squares fit of a plane to (x,y,f(x,y)) data by using distance
// measurements in the z-direction.  The resulting plane is represented by
// z = A*x + B*y + C.  The return value is 'false' if the 3x3 coefficient
// matrix in the linear system that defines A, B, and C is (nearly) singular.
// In this case, A, B, and C are returned as MAX_REAL.
template <class Real> WM4_FOUNDATION_ITEM
bool HeightPlaneFit3 (int iQuantity, const Vector3<Real>* akPoint,
    Real& rfA, Real& rfB, Real& rfC);

// Least-squares fit of a plane to (x,y,z) data by using distance measurements
// orthogonal to the proposed plane.
template <class Real> WM4_FOUNDATION_ITEM
Plane3<Real> OrthogonalPlaneFit3 (int iQuantity,
    const Vector3<Real>* akPoint);

}