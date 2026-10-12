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
#include "Wm4Vector3.h"

namespace Wm4
{

template <class Real, class TVector>
class WM4_FOUNDATION_ITEM Distance
{
public:
    // abstract base class
    virtual ~Distance ();

    // static distance queries
    virtual Real Get () = 0;     // distance
    virtual Real GetSquared () = 0;  // squared distance

    // function calculations for dynamic distance queries
    virtual Real Get (Real fT, const TVector& rkVelocity0,
        const TVector& rkVelocity1) = 0;
    virtual Real GetSquared (Real fT, const TVector& rkVelocity0,
        const TVector& rkVelocity1) = 0;

    // Derivative calculations for dynamic distance queries.  The defaults
    // use finite difference estimates
    //   f'(t) = (f(t+h)-f(t-h))/(2*h)
    // where h = DifferenceStep.  A derived class may override these and
    // provide implementations of exact formulas that do not require h.
    virtual Real GetDerivative (Real fT, const TVector& rkVelocity0,
        const TVector& rkVelocity1);
    virtual Real GetDerivativeSquared (Real fT, const TVector& rkVelocity0,
        const TVector& rkVelocity1);

    // Dynamic distance queries.  The function computes the smallest distance
    // between the two objects over the time interval [tmin,tmax].
    virtual Real Get (Real fTMin, Real fTMax, const TVector& rkVelocity0,
        const TVector& rkVelocity1);
    virtual Real GetSquared (Real fTMin, Real fTMax,
        const TVector& rkVelocity0, const TVector& rkVelocity1);

    // for Newton's method and inverse parabolic interpolation
    int MaximumIterations;  // default = 8
    Real ZeroThreshold;     // default = Math<Real>::ZERO_TOLERANCE

    // for derivative approximations
    void SetDifferenceStep (Real fDifferenceStep);  // default = 1e-03
    Real GetDifferenceStep () const;

    // The time at which minimum distance occurs for the dynamic queries.
    Real GetContactTime () const;

    // Closest points on the two objects.  These are valid for static or
    // dynamic queries.  The set of closest points on a single object need
    // not be a single point.  In this case, the Boolean member functions
    // return 'true'.  A derived class may support querying for the full
    // contact set.
    const TVector& GetClosestPoint0 () const;
    const TVector& GetClosestPoint1 () const;
    bool HasMultipleClosestPoints0 () const;
    bool HasMultipleClosestPoints1 () const;

protected:
    Distance ();

    Real m_fContactTime;
    TVector m_kClosestPoint0;
    TVector m_kClosestPoint1;
    bool m_bHasMultipleClosestPoints0;
    bool m_bHasMultipleClosestPoints1;
    Real m_fDifferenceStep, m_fInvTwoDifferenceStep;
};

typedef Distance<float,Vector2f> Distance2f;
typedef Distance<float,Vector3f> Distance3f;
typedef Distance<double,Vector2d> Distance2d;
typedef Distance<double,Vector3d> Distance3d;

}