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
#include "Wm4Sphere3.h"

namespace Wm4
{

// Least-squares fit of a sphere to a set of points.  Successful fit is
// indicated by return value of 'true'.  If return value is false, number of
// iterations was exceeded.  Try increasing the maximum number of iterations.
//
// If bInitialCenterIsAverage is set to 'true', the initial guess for the
// sphere center is the average of the data points.  If the data points are
// clustered along a solid angle, SphereFit3 is very slow to converge.  If
// bInitialCenterIsAverage is set to 'false', the initial guess for the
// sphere center is computed using a least-squares estimate of the
// coefficients for a quadratic equation that represents a sphere.  This
// approach tends to converge rapidly.

template <class Real> WM4_FOUNDATION_ITEM
bool SphereFit3 (int iQuantity, const Vector3<Real>* akPoint,
    int iMaxIterations, Sphere3<Real>& rkSphere,
    bool bInitialCenterIsAverage);

}