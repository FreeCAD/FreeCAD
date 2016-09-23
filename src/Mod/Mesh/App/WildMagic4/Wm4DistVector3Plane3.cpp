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
#include "Wm4DistVector3Plane3.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
DistVector3Plane3<Real>::DistVector3Plane3 (const Vector3<Real>& rkVector,
    const Plane3<Real>& rkPlane)
    :
    m_rkVector(rkVector),
    m_rkPlane(rkPlane)
{
}
//----------------------------------------------------------------------------
template <class Real>
const Vector3<Real>& DistVector3Plane3<Real>::GetVector () const
{
    return m_rkVector;
}
//----------------------------------------------------------------------------
template <class Real>
const Plane3<Real>& DistVector3Plane3<Real>::GetPlane () const
{
    return m_rkPlane;
}
//----------------------------------------------------------------------------
template <class Real>
Real DistVector3Plane3<Real>::Get ()
{
    Real fSigned = m_rkPlane.Normal.Dot(m_rkVector) - m_rkPlane.Constant;
    m_kClosestPoint0 = m_rkVector;
    m_kClosestPoint1 = m_rkVector - fSigned*m_rkPlane.Normal;
    return Math<Real>::FAbs(fSigned);
}
//----------------------------------------------------------------------------
template <class Real>
Real DistVector3Plane3<Real>::GetSquared ()
{
    Real fSigned = m_rkPlane.Normal.Dot(m_rkVector) - m_rkPlane.Constant;
    m_kClosestPoint0 = m_rkVector;
    m_kClosestPoint1 = m_rkVector - fSigned*m_rkPlane.Normal;
    return fSigned*fSigned;
}
//----------------------------------------------------------------------------
template <class Real>
Real DistVector3Plane3<Real>::Get (Real fT, const Vector3<Real>& rkVelocity0,
    const Vector3<Real>& rkVelocity1)
{
    (void)rkVelocity1;
    Vector3<Real> kMVector = m_rkVector + fT*rkVelocity0;
    Real fMConstant = m_rkPlane.Constant +
        fT*m_rkPlane.Normal.Dot(rkVelocity0);
    Plane3<Real> kMPlane(m_rkPlane.Normal,fMConstant);
    return DistVector3Plane3<Real>(kMVector,kMPlane).Get();
}
//----------------------------------------------------------------------------
template <class Real>
Real DistVector3Plane3<Real>::GetSquared (Real fT,
    const Vector3<Real>& rkVelocity0, const Vector3<Real>& rkVelocity1)
{
    (void)rkVelocity1;
    Vector3<Real> kMVector = m_rkVector + fT*rkVelocity0;
    Real fMConstant = m_rkPlane.Constant +
        fT*m_rkPlane.Normal.Dot(rkVelocity0);
    Plane3<Real> kMPlane(m_rkPlane.Normal,fMConstant);
    return DistVector3Plane3<Real>(kMVector,kMPlane).GetSquared();
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class DistVector3Plane3<float>;

template WM4_FOUNDATION_ITEM
class DistVector3Plane3<double>;
//----------------------------------------------------------------------------
}
