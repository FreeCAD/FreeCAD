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
// Version: 4.0.1 (2006/08/22)

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::ACos (Real fValue)
{
    if (-(Real)1.0 < fValue)
    {
        if (fValue < (Real)1.0)
        {
            return (Real)acos((double)fValue);
        }
        else
        {
            return (Real)0.0;
        }
    }
    else
    {
        return PI;
    }
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::ASin (Real fValue)
{
    if (-(Real)1.0 < fValue)
    {
        if (fValue < (Real)1.0)
        {
            return (Real)asin((double)fValue);
        }
        else
        {
            return HALF_PI;
        }
    }
    else
    {
        return -HALF_PI;
    }
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::ATan (Real fValue)
{
    return (Real)atan((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::ATan2 (Real fY, Real fX)
{
    return (Real)atan2((double)fY,(double)fX);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Ceil (Real fValue)
{
    return (Real)ceil((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Cos (Real fValue)
{
    return (Real)cos((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Exp (Real fValue)
{
    return (Real)exp((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FAbs (Real fValue)
{
    return (Real)fabs((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Floor (Real fValue)
{
    return (Real)floor((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FMod (Real fX, Real fY)
{
    return (Real)fmod((double)fX,(double)fY);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::InvSqrt (Real fValue)
{
    return (Real)(1.0/sqrt((double)fValue));
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Log (Real fValue)
{
    return (Real)log((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Log2 (Real fValue)
{
    return Math<Real>::INV_LN_2 * (Real)log((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Log10 (Real fValue)
{
    return Math<Real>::INV_LN_10 * (Real)log((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Pow (Real fBase, Real fExponent)
{
    return (Real)pow((double)fBase,(double)fExponent);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Sin (Real fValue)
{
    return (Real)sin((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Sqr (Real fValue)
{
    return fValue*fValue;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Sqrt (Real fValue)
{
    return (Real)sqrt((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Tan (Real fValue)
{
    return (Real)tan((double)fValue);
}
//----------------------------------------------------------------------------
template <class Real>
int Math<Real>::Sign (int iValue)
{
    if (iValue > 0)
    {
        return 1;
    }

    if (iValue < 0)
    {
        return -1;
    }

    return 0;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::Sign (Real fValue)
{
    if (fValue > (Real)0.0)
    {
        return (Real)1.0;
    }

    if (fValue < (Real)0.0)
    {
        return -(Real)1.0;
    }

    return (Real)0.0;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::UnitRandom (unsigned int uiSeed )
{
    if (uiSeed > 0)
    {
        srand(uiSeed);
    }

    double dRatio = ((double)rand())/((double)(RAND_MAX));
    return (Real)dRatio;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::SymmetricRandom (unsigned int uiSeed)
{
    if (uiSeed > 0.0)
    {
        srand(uiSeed);
    }

    double dRatio = ((double)rand())/((double)(RAND_MAX));
    return (Real)(2.0*dRatio - 1.0);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::IntervalRandom (Real fMin, Real fMax, unsigned int uiSeed)
{
    if (uiSeed > 0)
    {
        srand(uiSeed);
    }

    double dRatio = ((double)rand())/((double)(RAND_MAX));
    return fMin+(fMax-fMin)*((Real)dRatio);
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastSin0 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = (Real)7.61e-03;
    fResult *= fASqr;
    fResult -= (Real)1.6605e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastSin1 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = -(Real)2.39e-08;
    fResult *= fASqr;
    fResult += (Real)2.7526e-06;
    fResult *= fASqr;
    fResult -= (Real)1.98409e-04;
    fResult *= fASqr;
    fResult += (Real)8.3333315e-03;
    fResult *= fASqr;
    fResult -= (Real)1.666666664e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastCos0 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = (Real)3.705e-02;
    fResult *= fASqr;
    fResult -= (Real)4.967e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastCos1 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = -(Real)2.605e-07;
    fResult *= fASqr;
    fResult += (Real)2.47609e-05;
    fResult *= fASqr;
    fResult -= (Real)1.3888397e-03;
    fResult *= fASqr;
    fResult += (Real)4.16666418e-02;
    fResult *= fASqr;
    fResult -= (Real)4.999999963e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastTan0 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = (Real)2.033e-01;
    fResult *= fASqr;
    fResult += (Real)3.1755e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastTan1 (Real fAngle)
{
    Real fASqr = fAngle*fAngle;
    Real fResult = (Real)9.5168091e-03;
    fResult *= fASqr;
    fResult += (Real)2.900525e-03;
    fResult *= fASqr;
    fResult += (Real)2.45650893e-02;
    fResult *= fASqr;
    fResult += (Real)5.33740603e-02;
    fResult *= fASqr;
    fResult += (Real)1.333923995e-01;
    fResult *= fASqr;
    fResult += (Real)3.333314036e-01;
    fResult *= fASqr;
    fResult += (Real)1.0;
    fResult *= fAngle;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvSin0 (Real fValue)
{
    Real fRoot = Math<Real>::Sqrt(((Real)1.0)-fValue);
    Real fResult = -(Real)0.0187293;
    fResult *= fValue;
    fResult += (Real)0.0742610;
    fResult *= fValue;
    fResult -= (Real)0.2121144;
    fResult *= fValue;
    fResult += (Real)1.5707288;
    fResult = HALF_PI - fRoot*fResult;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvSin1 (Real fValue)
{
    Real fRoot = Math<Real>::Sqrt(FAbs(((Real)1.0)-fValue));
    Real fResult = -(Real)0.0012624911;
    fResult *= fValue;
    fResult += (Real)0.0066700901;
    fResult *= fValue;
    fResult -= (Real)0.0170881256;
    fResult *= fValue;
    fResult += (Real)0.0308918810;
    fResult *= fValue;
    fResult -= (Real)0.0501743046;
    fResult *= fValue;
    fResult += (Real)0.0889789874;
    fResult *= fValue;
    fResult -= (Real)0.2145988016;
    fResult *= fValue;
    fResult += (Real)1.5707963050;
    fResult = HALF_PI - fRoot*fResult;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvCos0 (Real fValue)
{
    Real fRoot = Math<Real>::Sqrt(((Real)1.0)-fValue);
    Real fResult = -(Real)0.0187293;
    fResult *= fValue;
    fResult += (Real)0.0742610;
    fResult *= fValue;
    fResult -= (Real)0.2121144;
    fResult *= fValue;
    fResult += (Real)1.5707288;
    fResult *= fRoot;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvCos1 (Real fValue)
{
    Real fRoot = Math<Real>::Sqrt(FAbs(((Real)1.0)-fValue));
    Real fResult = -(Real)0.0012624911;
    fResult *= fValue;
    fResult += (Real)0.0066700901;
    fResult *= fValue;
    fResult -= (Real)0.0170881256;
    fResult *= fValue;
    fResult += (Real)0.0308918810;
    fResult *= fValue;
    fResult -= (Real)0.0501743046;
    fResult *= fValue;
    fResult += (Real)0.0889789874;
    fResult *= fValue;
    fResult -= (Real)0.2145988016;
    fResult *= fValue;
    fResult += (Real)1.5707963050;
    fResult *= fRoot;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvTan0 (Real fValue)
{
    Real fVSqr = fValue*fValue;
    Real fResult = (Real)0.0208351;
    fResult *= fVSqr;
    fResult -= (Real)0.085133;
    fResult *= fVSqr;
    fResult += (Real)0.180141;
    fResult *= fVSqr;
    fResult -= (Real)0.3302995;
    fResult *= fVSqr;
    fResult += (Real)0.999866;
    fResult *= fValue;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastInvTan1 (Real fValue)
{
    Real fVSqr = fValue*fValue;
    Real fResult = (Real)0.0028662257;
    fResult *= fVSqr;
    fResult -= (Real)0.0161657367;
    fResult *= fVSqr;
    fResult += (Real)0.0429096138;
    fResult *= fVSqr;
    fResult -= (Real)0.0752896400;
    fResult *= fVSqr;
    fResult += (Real)0.1065626393;
    fResult *= fVSqr;
    fResult -= (Real)0.1420889944;
    fResult *= fVSqr;
    fResult += (Real)0.1999355085;
    fResult *= fVSqr;
    fResult -= (Real)0.3333314528;
    fResult *= fVSqr;
    fResult += (Real)1.0;
    fResult *= fValue;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastNegExp0 (Real fValue)
{
    Real fResult = (Real)0.0038278;
    fResult *= fValue;
    fResult += (Real)0.0292732;
    fResult *= fValue;
    fResult += (Real)0.2507213;
    fResult *= fValue;
    fResult += (Real)1.0;
    fResult *= fResult;
    fResult *= fResult;
    fResult = ((Real)1.0)/fResult;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastNegExp1 (Real fValue)
{
    Real fResult = (Real)0.00026695;
    fResult *= fValue;
    fResult += (Real)0.00227723;
    fResult *= fValue;
    fResult += (Real)0.03158565;
    fResult *= fValue;
    fResult += (Real)0.24991035;
    fResult *= fValue;
    fResult += (Real)1.0;
    fResult *= fResult;
    fResult *= fResult;
    fResult = ((Real)1.0)/fResult;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastNegExp2 (Real fValue)
{
    Real fResult = (Real)0.000014876;
    fResult *= fValue;
    fResult += (Real)0.000127992;
    fResult *= fValue;
    fResult += (Real)0.002673255;
    fResult *= fValue;
    fResult += (Real)0.031198056;
    fResult *= fValue;
    fResult += (Real)0.250010936;
    fResult *= fValue;
    fResult += (Real)1.0;
    fResult *= fResult;
    fResult *= fResult;
    fResult = ((Real)1.0)/fResult;
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Real Math<Real>::FastNegExp3 (Real fValue)
{
    Real fResult = (Real)0.0000006906;
    fResult *= fValue;
    fResult += (Real)0.0000054302;
    fResult *= fValue;
    fResult += (Real)0.0001715620;
    fResult *= fValue;
    fResult += (Real)0.0025913712;
    fResult *= fValue;
    fResult += (Real)0.0312575832;
    fResult *= fValue;
    fResult += (Real)0.2499986842;
    fResult *= fValue;
    fResult += (Real)1.0;
    fResult *= fResult;
    fResult *= fResult;
    fResult = ((Real)1.0)/fResult;
    return fResult;
}
//----------------------------------------------------------------------------
} // namespace Wm4
