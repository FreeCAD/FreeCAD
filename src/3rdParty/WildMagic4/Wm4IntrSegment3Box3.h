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
#include "Wm4Segment3.h"
#include "Wm4Box3.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM IntrSegment3Box3
    : public Intersector<Real,Vector3<Real> >
{
public:
    IntrSegment3Box3 (const Segment3<Real>& rkSegment,
        const Box3<Real>& rkBox, bool bSolid);

    // object access
    const Segment3<Real>& GetSegment () const;
    const Box3<Real>& GetBox () const;

    // static intersection queries
    virtual bool Test ();
    virtual bool Find ();

    // the intersection set
    int GetQuantity () const;
    const Vector3<Real>& GetPoint (int i) const;

private:
    using Intersector<Real,Vector3<Real> >::m_iIntersectionType;

    // the objects to intersect
    const Segment3<Real>& m_rkSegment;
    const Box3<Real>& m_rkBox;
    bool m_bSolid;

    // information about the intersection set
    int m_iQuantity;
    Vector3<Real> m_akPoint[2];
};

typedef IntrSegment3Box3<float> IntrSegment3Box3f;
typedef IntrSegment3Box3<double> IntrSegment3Box3d;

}