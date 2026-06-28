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

// Least-squares fit of a finite cylinder to (x,y,z) data.  The cylinder has
// center C, unit-length axis direction U, radius r, and height h.  The end
// disks of the cylinder are at C+(h/2)*U and C-(h/2)*U.  A point X is on the
// cylinder wall if (X-C)^T*(I-U*U^T)*(X-C) = r^2.  A point X is on the end
// disk at C+(h/2)*U if Dot(U,X-C) = h/2 and (X-C)^T*(I-U*U^T)*(X-C) <= r^2.
// A point X is on the end disk at C-(h/2)*U if Dot(U,X-C) = -h/2 and
// (X-C)^T*(I-U*U^T)*(X-C) <= r^2.
  
// The inputs are the quantity of points and the point array.  The outputs
// are the center C, unit-length axis direction U, radius R, and height H.
// You can supply initial guesses for C and U.  In this case you need to set
// bInputsAreInitialGuess to 'true'.  Otherwise set it to 'false' and the
// function will select C and U by first fitting the data with a least-squares
// line.  The return function value is the error for the least-squares fit,
// e >= 0.  If all the points lie exactly on a cylinder, then e = 0.
//
// You can examine the error e and iterate the calls yourself.  The outputs
// C, U, R, and H can be fed back into the function call as initial guesses.
//
// Real fError0 = CylinderFit3<Real>(iQuantity,akPoint,kC,kU,fR,fH,false);
// for (i = 1; i <= iMax; i++)
// {
//     Real fError1 = CylinderFit3<Real>(iQuantity,akPoint,kC,kU,fR,fH,true);
//     if ( fError1 not changed much from fError0 )
//         break;
// }

#include "Wm4FoundationLIB.h"
#include "Wm4Vector3.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM CylinderFit3
{
public:
    CylinderFit3 (int iQuantity, const Vector3<Real>* akPoint,
        Vector3<Real>& rkC, Vector3<Real>& rkU, Real& rfR, Real& rfH,
        bool bInputsAreInitialGuess);

    // return the error value
    operator Real ();

private:
    static Real UpdateInvRSqr (int iQuantity, const Vector3<Real>* akPoint,
        const Vector3<Real>& rkC, const Vector3<Real>& rkU, Real& rfInvRSqr);

    static Real UpdateDirection (int iQuantity, const Vector3<Real>* akPoint,
        const Vector3<Real>& rkC, Vector3<Real>& rkU, Real& rfInvRSqr);

    static Real UpdateCenter (int iQuantity, const Vector3<Real>* akPoint,
        Vector3<Real>& rkC, const Vector3<Real>& rkU, const Real& rfInvRSqr);

    Real m_fError;
};

typedef CylinderFit3<float> CylinderFit3f;
typedef CylinderFit3<double> CylinderFit3d;

}