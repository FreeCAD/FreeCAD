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
#include "Wm4Distance.h"
#include "Wm4Segment3.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM DistSegment3Segment3
    : public Distance<Real,Vector3<Real> >
{
public:
    DistSegment3Segment3 (const Segment3<Real>& rkSegment0,
        const Segment3<Real>& rkSegment1);

    // object access
    const Segment3<Real>& GetSegment0 () const;
    const Segment3<Real>& GetSegment1 () const;

    // static distance queries
    virtual Real Get ();
    virtual Real GetSquared ();

    // function calculations for dynamic distance queries
    virtual Real Get (Real fT, const Vector3<Real>& rkVelocity0,
        const Vector3<Real>& rkVelocity1);
    virtual Real GetSquared (Real fT, const Vector3<Real>& rkVelocity0,
        const Vector3<Real>& rkVelocity1);

    // Information about the closest points.
    Real GetSegment0Parameter () const;
    Real GetSegment1Parameter () const;

private:
    using Distance<Real,Vector3<Real> >::m_kClosestPoint0;
    using Distance<Real,Vector3<Real> >::m_kClosestPoint1;

    const Segment3<Real>& m_rkSegment0;
    const Segment3<Real>& m_rkSegment1;

    // Information about the closest points.
    Real m_fSegment0Parameter;  // closest0 = seg0.origin+param*seg0.direction
    Real m_fSegment1Parameter;  // closest1 = seg1.origin+param*seg1.direction
};

typedef DistSegment3Segment3<float> DistSegment3Segment3f;
typedef DistSegment3Segment3<double> DistSegment3Segment3d;

}