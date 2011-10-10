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
// Version: 4.0.1 (2006/12/02)

#include "Wm4FoundationPCH.h"
#include "Wm4DistSegment3Segment3.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
DistSegment3Segment3<Real>::DistSegment3Segment3 (
    const Segment3<Real>& rkSegment0, const Segment3<Real>& rkSegment1)
    :
    m_rkSegment0(rkSegment0),
    m_rkSegment1(rkSegment1)
{
}
//----------------------------------------------------------------------------
template <class Real>
const Segment3<Real>& DistSegment3Segment3<Real>::GetSegment0 () const
{
    return m_rkSegment0;
}
//----------------------------------------------------------------------------
template <class Real>
const Segment3<Real>& DistSegment3Segment3<Real>::GetSegment1 () const
{
    return m_rkSegment1;
}
//----------------------------------------------------------------------------
template <class Real>
Real DistSegment3Segment3<Real>::Get ()
{
    Real fSqrDist = GetSquared();
    return Math<Real>::Sqrt(fSqrDist);
}
//----------------------------------------------------------------------------
template <class Real>
Real DistSegment3Segment3<Real>::GetSquared ()
{
    Vector3<Real> kDiff = m_rkSegment0.Origin - m_rkSegment1.Origin;
    Real fA01 = -m_rkSegment0.Direction.Dot(m_rkSegment1.Direction);
    Real fB0 = kDiff.Dot(m_rkSegment0.Direction);
    Real fB1 = -kDiff.Dot(m_rkSegment1.Direction);
    Real fC = kDiff.SquaredLength();
    Real fDet = Math<Real>::FAbs((Real)1.0 - fA01*fA01);
    Real fS0, fS1, fSqrDist, fExtDet0, fExtDet1, fTmpS0, fTmpS1;

    if (fDet >= Math<Real>::ZERO_TOLERANCE)
    {
        // segments are not parallel
        fS0 = fA01*fB1-fB0;
        fS1 = fA01*fB0-fB1;
        fExtDet0 = m_rkSegment0.Extent*fDet;
        fExtDet1 = m_rkSegment1.Extent*fDet;

        if (fS0 >= -fExtDet0)
        {
            if (fS0 <= fExtDet0)
            {
                if (fS1 >= -fExtDet1)
                {
                    if (fS1 <= fExtDet1)  // region 0 (interior)
                    {
                        // minimum at two interior points of 3D lines
                        Real fInvDet = ((Real)1.0)/fDet;
                        fS0 *= fInvDet;
                        fS1 *= fInvDet;
                        fSqrDist = fS0*(fS0+fA01*fS1+((Real)2.0)*fB0) +
                            fS1*(fA01*fS0+fS1+((Real)2.0)*fB1)+fC;
                    }
                    else  // region 3 (side)
                    {
                        fS1 = m_rkSegment1.Extent;
                        fTmpS0 = -(fA01*fS1+fB0);
                        if (fTmpS0 < -m_rkSegment0.Extent)
                        {
                            fS0 = -m_rkSegment0.Extent;
                            fSqrDist = fS0*(fS0-((Real)2.0)*fTmpS0) +
                                fS1*(fS1+((Real)2.0)*fB1)+fC;
                        }
                        else if (fTmpS0 <= m_rkSegment0.Extent)
                        {
                            fS0 = fTmpS0;
                            fSqrDist = -fS0*fS0+fS1*(fS1+((Real)2.0)*fB1)+fC;
                        }
                        else
                        {
                            fS0 = m_rkSegment0.Extent;
                            fSqrDist = fS0*(fS0-((Real)2.0)*fTmpS0) +
                                fS1*(fS1+((Real)2.0)*fB1)+fC;
                        }
                    }
                }
                else  // region 7 (side)
                {
                    fS1 = -m_rkSegment1.Extent;
                    fTmpS0 = -(fA01*fS1+fB0);
                    if (fTmpS0 < -m_rkSegment0.Extent)
                    {
                        fS0 = -m_rkSegment0.Extent;
                        fSqrDist = fS0*(fS0-((Real)2.0)*fTmpS0) +
                            fS1*(fS1+((Real)2.0)*fB1)+fC;
                    }
                    else if (fTmpS0 <= m_rkSegment0.Extent)
                    {
                        fS0 = fTmpS0;
                        fSqrDist = -fS0*fS0+fS1*(fS1+((Real)2.0)*fB1)+fC;
                    }
                    else
                    {
                        fS0 = m_rkSegment0.Extent;
                        fSqrDist = fS0*(fS0-((Real)2.0)*fTmpS0) +
                            fS1*(fS1+((Real)2.0)*fB1)+fC;
                    }
                }
            }
            else
            {
                if (fS1 >= -fExtDet1)
                {
                    if (fS1 <= fExtDet1)  // region 1 (side)
                    {
                        fS0 = m_rkSegment0.Extent;
                        fTmpS1 = -(fA01*fS0+fB1);
                        if (fTmpS1 < -m_rkSegment1.Extent)
                        {
                            fS1 = -m_rkSegment1.Extent;
                            fSqrDist = fS1*(fS1-((Real)2.0)*fTmpS1) +
                                fS0*(fS0+((Real)2.0)*fB0)+fC;
                        }
                        else if (fTmpS1 <= m_rkSegment1.Extent)
                        {
                            fS1 = fTmpS1;
                            fSqrDist = -fS1*fS1+fS0*(fS0+((Real)2.0)*fB0)+fC;
                        }
                        else
                        {
                            fS1 = m_rkSegment1.Extent;
                            fSqrDist = fS1*(fS1-((Real)2.0)*fTmpS1) +
                                fS0*(fS0+((Real)2.0)*fB0)+fC;
                        }
                    }
                    else  // region 2 (corner)
                    {
                        fS1 = m_rkSegment1.Extent;
                        fTmpS0 = -(fA01*fS1+fB0);
                        if (fTmpS0 < -m_rkSegment0.Extent)
                        {
                            fS0 = -m_rkSegment0.Extent;
                            fSqrDist = fS0*(fS0-((Real)2.0)*fTmpS0) +
                                fS1*(fS1+((Real)2.0)*fB1)+fC;
                        }
                        else if (fTmpS0 <= m_rkSegment0.Extent)
                        {
                            fS0 = fTmpS0;
                            fSqrDist = -fS0*fS0+fS1*(fS1+((Real)2.0)*fB1)+fC;
                        }
                        else
                        {
                            fS0 = m_rkSegment0.Extent;
                            fTmpS1 = -(fA01*fS0+fB1);
                            if (fTmpS1 < -m_rkSegment1.Extent)
                            {
                                fS1 = -m_rkSegment1.Extent;
                                fSqrDist = fS1*(fS1-((Real)2.0)*fTmpS1) +
                                    fS0*(fS0+((Real)2.0)*fB0)+fC;
                            }
                            else if (fTmpS1 <= m_rkSegment1.Extent)
                            {
                                fS1 = fTmpS1;
                                fSqrDist = -fS1*fS1+fS0*(fS0+((Real)2.0)*fB0)
                                    + fC;
                            }
                            else
                            {
                                fS1 = m_rkSegment1.Extent;
                                fSqrDist = fS1*(fS1-((Real)2.0)*fTmpS1) +
                                    fS0*(fS0+((Real)2.0)*fB0)+fC;
                            }
                        }
                    }
                }
                else  // region 8 (corner)
                {
                    fS1 = -m_rkSegment1.Extent;
                    fTmpS0 = -(fA01*fS1+fB0);
                    if (fTmpS0 < -m_rkSegment0.Extent)
                    {
                        fS0 = -m_rkSegment0.Extent;
                        fSqrDist = fS0*(fS0-((Real)2.0)*fTmpS0) +
                            fS1*(fS1+((Real)2.0)*fB1)+fC;
                    }
                    else if (fTmpS0 <= m_rkSegment0.Extent)
                    {
                        fS0 = fTmpS0;
                        fSqrDist = -fS0*fS0+fS1*(fS1+((Real)2.0)*fB1)+fC;
                    }
                    else
                    {
                        fS0 = m_rkSegment0.Extent;
                        fTmpS1 = -(fA01*fS0+fB1);
                        if (fTmpS1 > m_rkSegment1.Extent)
                        {
                            fS1 = m_rkSegment1.Extent;
                            fSqrDist = fS1*(fS1-((Real)2.0)*fTmpS1) +
                                fS0*(fS0+((Real)2.0)*fB0)+fC;
                        }
                        else if (fTmpS1 >= -m_rkSegment1.Extent)
                        {
                            fS1 = fTmpS1;
                            fSqrDist = -fS1*fS1+fS0*(fS0+((Real)2.0)*fB0)
                                + fC;
                        }
                        else
                        {
                            fS1 = -m_rkSegment1.Extent;
                            fSqrDist = fS1*(fS1-((Real)2.0)*fTmpS1) +
                                fS0*(fS0+((Real)2.0)*fB0)+fC;
                        }
                    }
                }
            }
        }
        else 
        {
            if (fS1 >= -fExtDet1)
            {
                if (fS1 <= fExtDet1)  // region 5 (side)
                {
                    fS0 = -m_rkSegment0.Extent;
                    fTmpS1 = -(fA01*fS0+fB1);
                    if (fTmpS1 < -m_rkSegment1.Extent)
                    {
                        fS1 = -m_rkSegment1.Extent;
                        fSqrDist = fS1*(fS1-((Real)2.0)*fTmpS1) +
                            fS0*(fS0+((Real)2.0)*fB0)+fC;
                    }
                    else if (fTmpS1 <= m_rkSegment1.Extent)
                    {
                        fS1 = fTmpS1;
                        fSqrDist = -fS1*fS1+fS0*(fS0+((Real)2.0)*fB0)+fC;
                    }
                    else
                    {
                        fS1 = m_rkSegment1.Extent;
                        fSqrDist = fS1*(fS1-((Real)2.0)*fTmpS1) +
                            fS0*(fS0+((Real)2.0)*fB0)+fC;
                    }
                }
                else  // region 4 (corner)
                {
                    fS1 = m_rkSegment1.Extent;
                    fTmpS0 = -(fA01*fS1+fB0);
                    if (fTmpS0 > m_rkSegment0.Extent)
                    {
                        fS0 = m_rkSegment0.Extent;
                        fSqrDist = fS0*(fS0-((Real)2.0)*fTmpS0) +
                            fS1*(fS1+((Real)2.0)*fB1)+fC;
                    }
                    else if (fTmpS0 >= -m_rkSegment0.Extent)
                    {
                        fS0 = fTmpS0;
                        fSqrDist = -fS0*fS0+fS1*(fS1+((Real)2.0)*fB1)+fC;
                    }
                    else
                    {
                        fS0 = -m_rkSegment0.Extent;
                        fTmpS1 = -(fA01*fS0+fB1);
                        if (fTmpS1 < -m_rkSegment1.Extent)
                        {
                            fS1 = -m_rkSegment1.Extent;
                            fSqrDist = fS1*(fS1-((Real)2.0)*fTmpS1) +
                                fS0*(fS0+((Real)2.0)*fB0)+fC;
                        }
                        else if (fTmpS1 <= m_rkSegment1.Extent)
                        {
                            fS1 = fTmpS1;
                            fSqrDist = -fS1*fS1+fS0*(fS0+((Real)2.0)*fB0)
                                + fC;
                        }
                        else
                        {
                            fS1 = m_rkSegment1.Extent;
                            fSqrDist = fS1*(fS1-((Real)2.0)*fTmpS1) +
                                fS0*(fS0+((Real)2.0)*fB0)+fC;
                        }
                    }
                }
            }
            else   // region 6 (corner)
            {
                fS1 = -m_rkSegment1.Extent;
                fTmpS0 = -(fA01*fS1+fB0);
                if (fTmpS0 > m_rkSegment0.Extent)
                {
                    fS0 = m_rkSegment0.Extent;
                    fSqrDist = fS0*(fS0-((Real)2.0)*fTmpS0) +
                        fS1*(fS1+((Real)2.0)*fB1)+fC;
                }
                else if (fTmpS0 >= -m_rkSegment0.Extent)
                {
                    fS0 = fTmpS0;
                    fSqrDist = -fS0*fS0+fS1*(fS1+((Real)2.0)*fB1)+fC;
                }
                else
                {
                    fS0 = -m_rkSegment0.Extent;
                    fTmpS1 = -(fA01*fS0+fB1);
                    if (fTmpS1 < -m_rkSegment1.Extent)
                    {
                        fS1 = -m_rkSegment1.Extent;
                        fSqrDist = fS1*(fS1-((Real)2.0)*fTmpS1) +
                            fS0*(fS0+((Real)2.0)*fB0)+fC;
                    }
                    else if (fTmpS1 <= m_rkSegment1.Extent)
                    {
                        fS1 = fTmpS1;
                        fSqrDist = -fS1*fS1+fS0*(fS0+((Real)2.0)*fB0)
                            + fC;
                    }
                    else
                    {
                        fS1 = m_rkSegment1.Extent;
                        fSqrDist = fS1*(fS1-((Real)2.0)*fTmpS1) +
                            fS0*(fS0+((Real)2.0)*fB0)+fC;
                    }
                }
            }
        }
    }
    else
    {
        // The segments are parallel.  The average b0 term is designed to
        // ensure symmetry of the function.  That is, dist(seg0,seg1) and
        // dist(seg1,seg0) should produce the same number.
        Real fE0pE1 = m_rkSegment0.Extent + m_rkSegment1.Extent;
        Real fSign = (fA01 > (Real)0.0 ? (Real)-1.0 : (Real)1.0);
        Real fB0Avr = ((Real)0.5)*(fB0 - fSign*fB1);
        Real fLambda = -fB0Avr;
        if (fLambda < -fE0pE1)
        {
            fLambda = -fE0pE1;
        }
        else if (fLambda > fE0pE1)
        {
            fLambda = fE0pE1;
        }

        fS1 = -fSign*fLambda*m_rkSegment1.Extent/fE0pE1;
        fS0 = fLambda + fSign*fS1;
        fSqrDist = fLambda*(fLambda + ((Real)2.0)*fB0Avr) + fC;
    }

    m_kClosestPoint0 = m_rkSegment0.Origin + fS0*m_rkSegment0.Direction;
    m_kClosestPoint1 = m_rkSegment1.Origin + fS1*m_rkSegment1.Direction;
    m_fSegment0Parameter = fS0;
    m_fSegment1Parameter = fS1;
    return Math<Real>::FAbs(fSqrDist);
}
//----------------------------------------------------------------------------
template <class Real>
Real DistSegment3Segment3<Real>::Get (Real fS1,
    const Vector3<Real>& rkVelocity0, const Vector3<Real>& rkVelocity1)
{
    Vector3<Real> kMOrigin0 = m_rkSegment0.Origin + fS1*rkVelocity0;
    Vector3<Real> kMOrigin1 = m_rkSegment1.Origin + fS1*rkVelocity1;
    Segment3<Real> kMSegment0(kMOrigin0,m_rkSegment0.Direction,
        m_rkSegment0.Extent);
    Segment3<Real> kMSegment1(kMOrigin1,m_rkSegment1.Direction,
        m_rkSegment1.Extent);
    return DistSegment3Segment3<Real>(kMSegment0,kMSegment1).Get();
}
//----------------------------------------------------------------------------
template <class Real>
Real DistSegment3Segment3<Real>::GetSquared (Real fS1,
    const Vector3<Real>& rkVelocity0, const Vector3<Real>& rkVelocity1)
{
    Vector3<Real> kMOrigin0 = m_rkSegment0.Origin + fS1*rkVelocity0;
    Vector3<Real> kMOrigin1 = m_rkSegment1.Origin + fS1*rkVelocity1;
    Segment3<Real> kMSegment0(kMOrigin0,m_rkSegment0.Direction,
        m_rkSegment0.Extent);
    Segment3<Real> kMSegment1(kMOrigin1,m_rkSegment1.Direction,
        m_rkSegment1.Extent);
    return DistSegment3Segment3<Real>(kMSegment0,kMSegment1).GetSquared();
}
//----------------------------------------------------------------------------
template <class Real>
Real DistSegment3Segment3<Real>::GetSegment0Parameter () const
{
    return m_fSegment0Parameter;
}
//----------------------------------------------------------------------------
template <class Real>
Real DistSegment3Segment3<Real>::GetSegment1Parameter () const
{
    return m_fSegment1Parameter;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class DistSegment3Segment3<float>;

template WM4_FOUNDATION_ITEM
class DistSegment3Segment3<double>;
//----------------------------------------------------------------------------
}
