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

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
Plane3<Real>::Plane3 ()
{
    // uninitialized
}
//----------------------------------------------------------------------------
template <class Real>
Plane3<Real>::Plane3 (const Plane3& rkPlane)
    :
    Normal(rkPlane.Normal)
{
    Constant = rkPlane.Constant;
}
//----------------------------------------------------------------------------
template <class Real>
Plane3<Real>::Plane3 (const Vector3<Real>& rkNormal, Real fConstant)
    :
    Normal(rkNormal)
{
    Constant = fConstant;
}
//----------------------------------------------------------------------------
template <class Real>
Plane3<Real>::Plane3 (const Vector3<Real>& rkNormal, const Vector3<Real>& rkP)
    :
    Normal(rkNormal)
{
    Constant = rkNormal.Dot(rkP);
}
//----------------------------------------------------------------------------
template <class Real>
Plane3<Real>::Plane3 (const Vector3<Real>& rkP0, const Vector3<Real>& rkP1,
    const Vector3<Real>& rkP2)
{
    Vector3<Real> kEdge1 = rkP1 - rkP0;
    Vector3<Real> kEdge2 = rkP2 - rkP0;
    Normal = kEdge1.UnitCross(kEdge2);
    Constant = Normal.Dot(rkP0);
}
//----------------------------------------------------------------------------
template <class Real>
Plane3<Real>& Plane3<Real>::operator= (const Plane3& rkPlane)
{
    Normal = rkPlane.Normal;
    Constant = rkPlane.Constant;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Real Plane3<Real>::DistanceTo (const Vector3<Real>& rkP) const
{
    return Normal.Dot(rkP) - Constant;
}
//----------------------------------------------------------------------------
template <class Real>
int Plane3<Real>::WhichSide (const Vector3<Real>& rkQ) const
{
    Real fDistance = DistanceTo(rkQ);

    if (fDistance < (Real)0.0)
    {
        return -1;
    }

    if (fDistance > (Real)0.0)
    {
        return +1;
    }

    return 0;
}
//----------------------------------------------------------------------------
} //namespace Wm4

