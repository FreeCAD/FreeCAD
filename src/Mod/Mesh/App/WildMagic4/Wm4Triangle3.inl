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
Triangle3<Real>::Triangle3 ()
{
    // uninitialized
}
//----------------------------------------------------------------------------
template <class Real>
Triangle3<Real>::Triangle3 (const Vector3<Real>& rkV0,
    const Vector3<Real>& rkV1, const Vector3<Real>& rkV2)
{
    V[0] = rkV0;
    V[1] = rkV1;
    V[2] = rkV2;
}
//----------------------------------------------------------------------------
template <class Real>
Triangle3<Real>::Triangle3 (const Vector3<Real> akV[3])
{
    for (int i = 0; i < 3; i++)
    {
        V[i] = akV[i];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Real Triangle3<Real>::DistanceTo (const Vector3<Real>& rkQ) const
{
    Vector3<Real> kDiff = V[0] - rkQ;
    Vector3<Real> kE0 = V[1] - V[0], kE1 = V[2] - V[0];
    Real fA00 = kE0.SquaredLength();
    Real fA01 = kE0.Dot(kE1);
    Real fA11 = kE1.SquaredLength();
    Real fB0 = kDiff.Dot(kE0);
    Real fB1 = kDiff.Dot(kE1);
    Real fC = kDiff.SquaredLength();
    Real fDet = Math<Real>::FAbs(fA00*fA11-fA01*fA01);
    Real fS = fA01*fB1-fA11*fB0;
    Real fT = fA01*fB0-fA00*fB1;
    Real fSqrDist;

    if (fS + fT <= fDet)
    {
        if (fS < (Real)0.0)
        {
            if (fT < (Real)0.0)  // region 4
            {
                if (fB0 < (Real)0.0)
                {
                    if (-fB0 >= fA00)
                    {
                        fSqrDist = fA00+((Real)2.0)*fB0+fC;
                    }
                    else
                    {
                        fSqrDist = fC-fB0*fB0/fA00;
                    }
                }
                else
                {
                    if (fB1 >= (Real)0.0)
                    {
                        fSqrDist = fC;
                    }
                    else if (-fB1 >= fA11)
                    {
                        fSqrDist = fA11+((Real)2.0)*fB1+fC;
                    }
                    else
                    {
                        fSqrDist = fC-fB1*fB1/fA11;
                    }
                }
            }
            else  // region 3
            {
                if (fB1 >= (Real)0.0)
                {
                    fSqrDist = fC;
                }
                else if (-fB1 >= fA11)
                {
                    fSqrDist = fA11+((Real)2.0)*fB1+fC;
                }
                else
                {
                    fSqrDist = fC-fB1*fB1/fA11;
                }
            }
        }
        else if (fT < (Real)0.0)  // region 5
        {
            if (fB0 >= (Real)0.0)
            {
                fSqrDist = fC;
            }
            else if (-fB0 >= fA00)
            {
                fSqrDist = fA00+((Real)2.0)*fB0+fC;
            }
            else
            {
                fSqrDist = fC-fB0*fB0/fA00;
            }
        }
        else  // region 0
        {
            // minimum at interior point
            Real fInvDet = ((Real)1.0)/fDet;
            fS *= fInvDet;
            fT *= fInvDet;
            fSqrDist = fS*(fA00*fS+fA01*fT+((Real)2.0)*fB0) +
                fT*(fA01*fS+fA11*fT+((Real)2.0)*fB1)+fC;
        }
    }
    else
    {
        Real fTmp0, fTmp1, fNumer, fDenom;

        if (fS < (Real)0.0)  // region 2
        {
            fTmp0 = fA01 + fB0;
            fTmp1 = fA11 + fB1;
            if (fTmp1 > fTmp0)
            {
                fNumer = fTmp1 - fTmp0;
                fDenom = fA00-2.0f*fA01+fA11;
                if (fNumer >= fDenom)
                {
                    fSqrDist = fA00+((Real)2.0)*fB0+fC;
                }
                else
                {
                    fS = fNumer/fDenom;
                    fT = (Real)1.0 - fS;
                    fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) +
                        fT*(fA01*fS+fA11*fT+((Real)2.0)*fB1)+fC;
                }
            }
            else
            {
                if (fTmp1 <= (Real)0.0)
                {
                    fSqrDist = fA11+((Real)2.0)*fB1+fC;
                }
                else if (fB1 >= (Real)0.0)
                {
                    fSqrDist = fC;
                }
                else
                {
                    fSqrDist = fC-fB1*fB1/fA11;
                }
            }
        }
        else if (fT < (Real)0.0)  // region 6
        {
            fTmp0 = fA01 + fB1;
            fTmp1 = fA00 + fB0;
            if (fTmp1 > fTmp0)
            {
                fNumer = fTmp1 - fTmp0;
                fDenom = fA00-((Real)2.0)*fA01+fA11;
                if (fNumer >= fDenom)
                {
                    fT = (Real)1.0;
                    fS = (Real)0.0;
                    fSqrDist = fA11+((Real)2.0)*fB1+fC;
                }
                else
                {
                    fT = fNumer/fDenom;
                    fS = (Real)1.0 - fT;
                    fSqrDist = fS*(fA00*fS+fA01*fT+((Real)2.0)*fB0) +
                        fT*(fA01*fS+fA11*fT+((Real)2.0)*fB1)+fC;
                }
            }
            else
            {
                if (fTmp1 <= (Real)0.0)
                {
                    fSqrDist = fA00+((Real)2.0)*fB0+fC;
                }
                else if (fB0 >= (Real)0.0)
                {
                    fSqrDist = fC;
                }
                else
                {
                    fSqrDist = fC-fB0*fB0/fA00;
                }
            }
        }
        else  // region 1
        {
            fNumer = fA11 + fB1 - fA01 - fB0;
            if (fNumer <= (Real)0.0)
            {
                fSqrDist = fA11+((Real)2.0)*fB1+fC;
            }
            else
            {
                fDenom = fA00-2.0f*fA01+fA11;
                if (fNumer >= fDenom)
                {
                    fSqrDist = fA00+((Real)2.0)*fB0+fC;
                }
                else
                {
                    fS = fNumer/fDenom;
                    fT = (Real)1.0 - fS;
                    fSqrDist = fS*(fA00*fS+fA01*fT+((Real)2.0)*fB0) +
                        fT*(fA01*fS+fA11*fT+((Real)2.0)*fB1)+fC;
                }
            }
        }
    }

    return Math<Real>::Sqrt(Math<Real>::FAbs(fSqrDist));
}
//----------------------------------------------------------------------------
} //namespace Wm4
