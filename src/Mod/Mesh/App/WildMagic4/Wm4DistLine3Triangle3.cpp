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
// Version: 4.0.1 (2006/07/23)

#include "Wm4FoundationPCH.h"
#include "Wm4DistLine3Triangle3.h"
#include "Wm4DistLine3Segment3.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
DistLine3Triangle3<Real>::DistLine3Triangle3 (const Line3<Real>& rkLine,
    const Triangle3<Real>& rkTriangle)
    :
    m_rkLine(rkLine),
    m_rkTriangle(rkTriangle)
{
}
//----------------------------------------------------------------------------
template <class Real>
const Line3<Real>& DistLine3Triangle3<Real>::GetLine () const
{
    return m_rkLine;
}
//----------------------------------------------------------------------------
template <class Real>
const Triangle3<Real>& DistLine3Triangle3<Real>::GetTriangle () const
{
    return m_rkTriangle;
}
//----------------------------------------------------------------------------
template <class Real>
Real DistLine3Triangle3<Real>::Get ()
{
    Real fSqrDist = GetSquared();
    return Math<Real>::Sqrt(fSqrDist);
}
//----------------------------------------------------------------------------
template <class Real>
Real DistLine3Triangle3<Real>::GetSquared ()
{
    // Test if line intersects triangle.  If so, the squared distance is zero.
    Vector3<Real> kEdge0 = m_rkTriangle.V[1] - m_rkTriangle.V[0];
    Vector3<Real> kEdge1 = m_rkTriangle.V[2] - m_rkTriangle.V[0];
    Vector3<Real> kNormal = kEdge0.UnitCross(kEdge1);
    Real fNdD = kNormal.Dot(m_rkLine.Direction);
    if (Math<Real>::FAbs(fNdD) > Math<Real>::ZERO_TOLERANCE)
    {
        // The line and triangle are not parallel, so the line intersects
        // the plane of the triangle.
        Vector3<Real> kDiff = m_rkLine.Origin - m_rkTriangle.V[0];
        Vector3<Real>& rkD = (Vector3<Real>&) m_rkLine.Direction;
        Vector3<Real> kU, kV;
        Vector3<Real>::GenerateComplementBasis(kU,kV,rkD);
        Real fUdE0 = kU.Dot(kEdge0);
        Real fUdE1 = kU.Dot(kEdge0);
        Real fUdDiff = kU.Dot(kDiff);
        Real fVdE0 = kV.Dot(kEdge0);
        Real fVdE1 = kV.Dot(kEdge0);
        Real fVdDiff = kV.Dot(kDiff);
        Real fInvDet = ((Real)1.0)/(fUdE0*fVdE1 - fUdE1*fVdE0);

        // Barycentric coordinates for the point of intersection.
        Real fB1 = (fVdE1*fUdDiff - fUdE1*fVdDiff)*fInvDet;
        Real fB2 = (fUdE0*fVdDiff - fVdE0*fUdDiff)*fInvDet;
        Real fB0 = (Real)1.0 - fB1 - fB2;

        if (fB0 >= (Real)0.0 && fB1 >= (Real)0.0 && fB2 >= (Real)0.0)
        {
            // Line parameter for the point of intersection.
            Real fDdE0 = rkD.Dot(kEdge0);
            Real fDdE1 = rkD.Dot(kEdge1);
            Real fDdDiff = m_rkLine.Direction.Dot(kDiff);
            m_fLineParameter = fB1*fDdE0 + fB2*fDdE1 - fDdDiff;

            // Barycentric coordinates for the point of intersection.
            m_afTriangleBary[0] = fB0;
            m_afTriangleBary[1] = fB1;
            m_afTriangleBary[2] = fB2;

            // The intersection point is inside or on the triangle.
            m_kClosestPoint0 = m_rkLine.Origin +
                m_fLineParameter*m_rkLine.Direction;

            m_kClosestPoint1 = m_rkTriangle.V[0] + fB1*kEdge0 + fB2*kEdge1;
            return (Real)0.0;
        }
    }

    // Either (1) the line is not parallel to the triangle and the point of
    // intersection of the line and the plane of the triangle is outside the
    // triangle or (2) the line and triangle are parallel.  Regardless, the
    // closest point on the triangle is on an edge of the triangle.  Compare
    // the line to all three edges of the triangle.
    Real fSqrDist = Math<Real>::MAX_REAL;
    for (int i0 = 2, i1 = 0; i1 < 3; i0 = i1++)
    {
        Segment3<Real> kSeg;
        kSeg.Origin = ((Real)0.5)*(m_rkTriangle.V[i0] + m_rkTriangle.V[i1]);
        kSeg.Direction = m_rkTriangle.V[i1] - m_rkTriangle.V[i0];
        kSeg.Extent = ((Real)0.5)*kSeg.Direction.Normalize();
        DistLine3Segment3<Real> kLSDist(m_rkLine,kSeg);
        Real fSqrDistTmp = kLSDist.GetSquared();
        if (fSqrDistTmp < fSqrDist)
        {
            m_kClosestPoint0 = kLSDist.GetClosestPoint0();
            m_kClosestPoint1 = kLSDist.GetClosestPoint1();
            fSqrDist = fSqrDistTmp;

            m_fLineParameter = kLSDist.GetLineParameter();
            Real fRatio = kLSDist.GetSegmentParameter()/kSeg.Extent;
            m_afTriangleBary[i0] = ((Real)0.5)*((Real)1.0 - fRatio);
            m_afTriangleBary[i1] = (Real)1.0 - m_afTriangleBary[i0];
            m_afTriangleBary[3-i0-i1] = (Real)0.0;
        }
    }
    return fSqrDist;
}
//----------------------------------------------------------------------------
template <class Real>
Real DistLine3Triangle3<Real>::Get (Real fT, const Vector3<Real>& rkVelocity0,
    const Vector3<Real>& rkVelocity1)
{
    Vector3<Real> kMOrigin = m_rkLine.Origin + fT*rkVelocity0;
    Vector3<Real> kMV0 = m_rkTriangle.V[0] + fT*rkVelocity1;
    Vector3<Real> kMV1 = m_rkTriangle.V[1] + fT*rkVelocity1;
    Vector3<Real> kMV2 = m_rkTriangle.V[2] + fT*rkVelocity1;
    Line3<Real> kMLine(kMOrigin,m_rkLine.Direction);
    Triangle3<Real> kMTriangle(kMV0,kMV1,kMV2);
    return DistLine3Triangle3<Real>(kMLine,kMTriangle).Get();
}
//----------------------------------------------------------------------------
template <class Real>
Real DistLine3Triangle3<Real>::GetSquared (Real fT,
    const Vector3<Real>& rkVelocity0, const Vector3<Real>& rkVelocity1)
{
    Vector3<Real> kMOrigin = m_rkLine.Origin + fT*rkVelocity0;
    Vector3<Real> kMV0 = m_rkTriangle.V[0] + fT*rkVelocity1;
    Vector3<Real> kMV1 = m_rkTriangle.V[1] + fT*rkVelocity1;
    Vector3<Real> kMV2 = m_rkTriangle.V[2] + fT*rkVelocity1;
    Line3<Real> kMLine(kMOrigin,m_rkLine.Direction);
    Triangle3<Real> kMTriangle(kMV0,kMV1,kMV2);
    return DistLine3Triangle3<Real>(kMLine,kMTriangle).GetSquared();
}
//----------------------------------------------------------------------------
template <class Real>
Real DistLine3Triangle3<Real>::GetLineParameter () const
{
    return m_fLineParameter;
}
//----------------------------------------------------------------------------
template <class Real>
Real DistLine3Triangle3<Real>::GetTriangleBary (int i) const
{
    assert(0 <= i && i < 3);
    return m_afTriangleBary[i];
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class DistLine3Triangle3<float>;

template WM4_FOUNDATION_ITEM
class DistLine3Triangle3<double>;
//----------------------------------------------------------------------------
}
