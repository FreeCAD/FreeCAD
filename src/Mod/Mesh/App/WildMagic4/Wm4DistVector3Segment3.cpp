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
#include "Wm4DistVector3Segment3.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
DistVector3Segment3<Real>::DistVector3Segment3 (const Vector3<Real>& rkVector,
    const Segment3<Real>& rkSegment)
    :
    m_rkVector(rkVector),
    m_rkSegment(rkSegment)
{
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& DistVector3Segment3<Real>::GetVector () const
{
    return m_rkVector;
}
//----------------------------------------------------------------------------
template <class Real>
const Segment3<Real>& DistVector3Segment3<Real>::GetSegment () const
{
    return m_rkSegment;
}
//----------------------------------------------------------------------------
template <class Real>
Real DistVector3Segment3<Real>::Get ()
{
    Real fSqrDist = GetSquared();
    return Math<Real>::Sqrt(fSqrDist);
}
//----------------------------------------------------------------------------
template <class Real>
Real DistVector3Segment3<Real>::GetSquared ()
{
    Vector3<Real> kDiff = m_rkVector - m_rkSegment.Origin;
    m_fSegmentParameter = m_rkSegment.Direction.Dot(kDiff);

    if (-m_rkSegment.Extent < m_fSegmentParameter)
    {
        if (m_fSegmentParameter < m_rkSegment.Extent)
        {
            m_kClosestPoint1 = m_rkSegment.Origin +
                m_fSegmentParameter*m_rkSegment.Direction;
        }
        else
        {
            m_kClosestPoint1 = m_rkSegment.Origin +
                m_rkSegment.Extent*m_rkSegment.Direction;
        }
    }
    else
    {
        m_kClosestPoint1 = m_rkSegment.Origin -
            m_rkSegment.Extent*m_rkSegment.Direction;
    }

    m_kClosestPoint0 = m_rkVector;
    kDiff = m_kClosestPoint1 - m_kClosestPoint0;
    return kDiff.SquaredLength();
}
//----------------------------------------------------------------------------
template <class Real>
Real DistVector3Segment3<Real>::Get (Real fT,
    const Vector3<Real>& rkVelocity0, const Vector3<Real>& rkVelocity1)
{
    Vector3<Real> kMVector = m_rkVector + fT*rkVelocity0;
    Vector3<Real> kMOrigin = m_rkSegment.Origin + fT*rkVelocity1;
    Segment3<Real> kMSegment(kMOrigin,m_rkSegment.Direction,
        m_rkSegment.Extent);
    return DistVector3Segment3<Real>(kMVector,kMSegment).Get();
}
//----------------------------------------------------------------------------
template <class Real>
Real DistVector3Segment3<Real>::GetSquared (Real fT,
    const Vector3<Real>& rkVelocity0, const Vector3<Real>& rkVelocity1)
{
    Vector3<Real> kMVector = m_rkVector + fT*rkVelocity0;
    Vector3<Real> kMOrigin = m_rkSegment.Origin + fT*rkVelocity1;
    Segment3<Real> kMSegment(kMOrigin,m_rkSegment.Direction,
        m_rkSegment.Extent);
    return DistVector3Segment3<Real>(kMVector,kMSegment).GetSquared();
}
//----------------------------------------------------------------------------
template <class Real>
Real DistVector3Segment3<Real>::GetSegmentParameter () const
{
    return m_fSegmentParameter;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class DistVector3Segment3<float>;

template WM4_FOUNDATION_ITEM
class DistVector3Segment3<double>;
//----------------------------------------------------------------------------
}
