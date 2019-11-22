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

#ifndef WM4INTRSEGMENT3PLANE3_H
#define WM4INTRSEGMENT3PLANE3_H

#include "Wm4FoundationLIB.h"
#include "Wm4Intersector.h"
#include "Wm4Segment3.h"
#include "Wm4Plane3.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM IntrSegment3Plane3
    : public Intersector<Real,Vector3<Real> >
{
public:
    IntrSegment3Plane3 (const Segment3<Real>& rkSegment,
        const Plane3<Real>& rkPlane);

    // object access
    const Segment3<Real>& GetSegment () const;
    const Plane3<Real>& GetPlane () const;

    // test-intersection query
    virtual bool Test ();

    // Find-intersection query.  The point of intersection is
    // P = origin + t*direction, with |t| <= e, where e is the segment
    // extent.
    virtual bool Find ();
    Real GetSegmentT () const;

private:
    using Intersector<Real,Vector3<Real> >::IT_EMPTY;
    using Intersector<Real,Vector3<Real> >::IT_POINT;
    using Intersector<Real,Vector3<Real> >::IT_SEGMENT;
    using Intersector<Real,Vector3<Real> >::m_iIntersectionType;

    // the objects to intersect
    const Segment3<Real>& m_rkSegment;
    const Plane3<Real>& m_rkPlane;

    // information about the intersection set
    Real m_fSegmentT;
};

typedef IntrSegment3Plane3<float> IntrSegment3Plane3f;
typedef IntrSegment3Plane3<double> IntrSegment3Plane3d;

}

#endif
