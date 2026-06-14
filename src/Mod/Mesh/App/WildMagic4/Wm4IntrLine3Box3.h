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
#include "Wm4Intersector.h"
#include "Wm4Line3.h"
#include "Wm4Box3.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM IntrLine3Box3
    : public Intersector<Real,Vector3<Real> >
{
public:
    IntrLine3Box3 (const Line3<Real>& rkLine, const Box3<Real>& rkBox);

    // object access
    const Line3<Real>& GetLine () const;
    const Box3<Real>& GetBox () const;

    // static intersection queries
    virtual bool Test ();
    virtual bool Find ();

    // the intersection set
    int GetQuantity () const;
    const Vector3<Real>& GetPoint (int i) const;

private:
    using Intersector<Real,Vector3<Real> >::IT_EMPTY;
    using Intersector<Real,Vector3<Real> >::IT_POINT;
    using Intersector<Real,Vector3<Real> >::IT_SEGMENT;
    using Intersector<Real,Vector3<Real> >::m_iIntersectionType;

    static bool Clip (Real fDenom, Real fNumer, Real& rfT0, Real& rfT1);

    // the objects to intersect
    const Line3<Real>& m_rkLine;
    const Box3<Real>& m_rkBox;

    // information about the intersection set
    int m_iQuantity;
    Vector3<Real> m_akPoint[2];

// internal use (shared by IntrRay3Box3 and IntrSegment3Box3)
public:
    static bool DoClipping (Real fT0, Real fT1, const Vector3<Real>& rkOrigin,
        const Vector3<Real>& rkDirection, const Box3<Real>& rkBox,
        bool bSolid, int& riQuantity, Vector3<Real> akPoint[2],
        int& riIntrType);
};

typedef IntrLine3Box3<float> IntrLine3Box3f;
typedef IntrLine3Box3<double> IntrLine3Box3d;

}