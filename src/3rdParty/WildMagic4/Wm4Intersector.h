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
#include "Wm4LinComp.h"
#include "Wm4Vector2.h"
#include "Wm4Vector3.h"

namespace Wm4
{

template <class Real, class TVector>
class WM4_FOUNDATION_ITEM Intersector
{
public:
    // abstract base class
    virtual ~Intersector ();

    // Static intersection queries.  The default implementations return
    // 'false'.  The Find query produces a set of intersection.  The derived
    // class is responsible for providing access to that set, since the nature
    // of the set is dependent on the object types.
    virtual bool Test ();
    virtual bool Find ();

    // Dynamic intersection queries.  The default implementations return
    // 'false'.  The Find query produces a set of first contact.  The derived
    // class is responsible for providing access to that set, since the nature
    // of the set is dependent on the object types.
    virtual bool Test (Real fTMax, const TVector& rkVelocity0,
        const TVector& rkVelocity1);
    virtual bool Find (Real fTMax, const TVector& rkVelocity0,
        const TVector& rkVelocity1);

    // The time at which two objects are in first contact for the dynamic
    // intersection queries.
    Real GetContactTime () const;

    // information about the intersection set
    enum
    {
        IT_EMPTY = LinComp<Real>::CT_EMPTY,
        IT_POINT = LinComp<Real>::CT_POINT,
        IT_SEGMENT = LinComp<Real>::CT_SEGMENT,
        IT_RAY = LinComp<Real>::CT_RAY,
        IT_LINE = LinComp<Real>::CT_LINE,
        IT_POLYGON,
        IT_PLANE,
        IT_POLYHEDRON,
        IT_OTHER
    };
    int GetIntersectionType () const;

protected:
    Intersector ();

    Real m_fContactTime;
    int m_iIntersectionType;
};

typedef Intersector<float, Vector2<float> > Intersector2f;
typedef Intersector<float, Vector3<float> > Intersector3f;
typedef Intersector<double, Vector2<double> > Intersector2d;
typedef Intersector<double, Vector3<double> > Intersector3d;

}