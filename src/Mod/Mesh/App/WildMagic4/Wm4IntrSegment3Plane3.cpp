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

#include "Wm4FoundationPCH.h"
#include "Wm4IntrSegment3Plane3.h"
#include "Wm4IntrLine3Plane3.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
IntrSegment3Plane3<Real>::IntrSegment3Plane3 (const Segment3<Real>& rkSegment,
    const Plane3<Real>& rkPlane)
    :
    m_rkSegment(rkSegment),
    m_rkPlane(rkPlane)
{
}
//----------------------------------------------------------------------------
template <class Real>
const Segment3<Real>& IntrSegment3Plane3<Real>::GetSegment () const
{
    return m_rkSegment;
}
//----------------------------------------------------------------------------
template <class Real>
const Plane3<Real>& IntrSegment3Plane3<Real>::GetPlane () const
{
    return m_rkPlane;
}
//----------------------------------------------------------------------------
template <class Real>
bool IntrSegment3Plane3<Real>::Test ()
{
    Vector3<Real> kP0 = m_rkSegment.GetNegEnd();
    Real fSDistance0 = m_rkPlane.DistanceTo(kP0);
    if (Math<Real>::FAbs(fSDistance0) <= Math<Real>::ZERO_TOLERANCE)
    {
        fSDistance0 = (Real)0.0;
    }

    Vector3<Real> kP1 = m_rkSegment.GetPosEnd();
    Real fSDistance1 = m_rkPlane.DistanceTo(kP1);
    if (Math<Real>::FAbs(fSDistance1) <= Math<Real>::ZERO_TOLERANCE)
    {
        fSDistance1 = (Real)0.0;
    }

    Real fProd = fSDistance0*fSDistance1;
    if (fProd < (Real)0.0)
    {
        // The segment passes through the plane.
        m_iIntersectionType = IT_POINT;
        return true;
    }

    if (fProd > (Real)0.0)
    {
        // The segment is on one side of the plane.
        m_iIntersectionType = IT_EMPTY;
        return false;
    }

    if (fSDistance0 != (Real)0.0 || fSDistance1 != (Real)0.0)
    {
        // A segment end point touches the plane.
        m_iIntersectionType = IT_POINT;
        return true;
    }

    // The segment is coincident with the plane.
    m_iIntersectionType = IT_SEGMENT;
    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool IntrSegment3Plane3<Real>::Find ()
{
    Line3<Real> kLine(m_rkSegment.Origin,m_rkSegment.Direction);
    IntrLine3Plane3<Real> kIntr(kLine,m_rkPlane);
    if (kIntr.Find())
    {
        // The line intersects the plane, but possibly at a point that is
        // not on the segment.
        m_iIntersectionType = kIntr.GetIntersectionType();
        m_fSegmentT = kIntr.GetLineT();
        return Math<Real>::FAbs(m_fSegmentT) <= m_rkSegment.Extent;
    }

    m_iIntersectionType = IT_EMPTY;
    return false;
}
//----------------------------------------------------------------------------
template <class Real>
Real IntrSegment3Plane3<Real>::GetSegmentT () const
{
    return m_fSegmentT;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class IntrSegment3Plane3<float>;

template WM4_FOUNDATION_ITEM
class IntrSegment3Plane3<double>;
//----------------------------------------------------------------------------
}
