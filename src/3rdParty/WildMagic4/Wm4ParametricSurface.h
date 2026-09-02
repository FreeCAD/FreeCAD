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
#include "Wm4Surface.h"
#include "Wm4Vector3.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM ParametricSurface : public Surface<Real>
{
public:
    // abstract base class
    virtual ~ParametricSurface ();

    // The parametric domain is either rectangular or triangular.  Valid (u,v)
    // values for a rectangular domain satisfy
    //   umin <= u <= umax,  vmin <= v <= vmax
    // Valid (u,v) values for a triangular domain satisfy
    //   umin <= u <= umax,  vmin <= v <= vmax,
    //   (vmax-vmin)*(u-umin)+(umax-umin)*(v-vmax) <= 0
    Real GetUMin () const;
    Real GetUMax () const;
    Real GetVMin () const;
    Real GetVMax () const;
    bool IsRectangular () const;

    // position and derivatives up to second order
    virtual Vector3<Real> P (Real fU, Real fV) const = 0;
    virtual Vector3<Real> PU (Real fU, Real fV) const = 0;
    virtual Vector3<Real> PV (Real fU, Real fV) const = 0;
    virtual Vector3<Real> PUU (Real fU, Real fV) const = 0;
    virtual Vector3<Real> PUV (Real fU, Real fV) const = 0;
    virtual Vector3<Real> PVV (Real fU, Real fV) const = 0;

    // Compute a coordinate frame.  The set {T0,T1,N} is a right-handed
    // orthonormal set.
    void GetFrame (Real fU, Real fV, Vector3<Real>& rkPosition,
        Vector3<Real>& rkTangent0, Vector3<Real>& rkTangent1,
        Vector3<Real>& rkNormal) const;

    // Differential geometric quantities.  The returned scalars are the
    // principal curvatures and the returned vectors are the corresponding
    // principal directions.
    void ComputePrincipalCurvatureInfo (Real fU, Real fV, Real& rfCurv0,
        Real& rfCurv1, Vector3<Real>& rkDir0, Vector3<Real>& rkDir1);

protected:
    ParametricSurface (Real fUMin, Real fUMax, Real fVMin, Real fVMax,
        bool bRectangular);

    Real m_fUMin, m_fUMax, m_fVMin, m_fVMax;
    bool m_bRectangular;
};

typedef ParametricSurface<float> ParametricSurfacef;
typedef ParametricSurface<double> ParametricSurfaced;

}