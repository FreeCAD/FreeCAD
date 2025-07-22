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
// Version: 4.0.2 (2006/08/19)

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>::Vector2 ()
{
    // uninitialized for performance in array construction
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>::Vector2 (Real fX, Real fY)
{
    m_afTuple[0] = fX;
    m_afTuple[1] = fY;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>::Vector2 (const Real* afTuple)
{
    m_afTuple[0] = afTuple[0];
    m_afTuple[1] = afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>::Vector2 (const Vector2& rkV)
{
    m_afTuple[0] = rkV.m_afTuple[0];
    m_afTuple[1] = rkV.m_afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>::operator const Real* () const
{
    return m_afTuple;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>::operator Real* ()
{
    return m_afTuple;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::operator[] (int i) const
{
    return m_afTuple[i];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Vector2<Real>::operator[] (int i)
{
    return m_afTuple[i];
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::X () const
{
    return m_afTuple[0];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Vector2<Real>::X ()
{
    return m_afTuple[0];
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::Y () const
{
    return m_afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Vector2<Real>::Y ()
{
    return m_afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>& Vector2<Real>::operator= (const Vector2& rkV)
{
    m_afTuple[0] = rkV.m_afTuple[0];
    m_afTuple[1] = rkV.m_afTuple[1];
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
int Vector2<Real>::CompareArrays (const Vector2& rkV) const
{
    return memcmp(m_afTuple,rkV.m_afTuple,2*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector2<Real>::operator== (const Vector2& rkV) const
{
    return CompareArrays(rkV) == 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector2<Real>::operator!= (const Vector2& rkV) const
{
    return CompareArrays(rkV) != 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector2<Real>::operator< (const Vector2& rkV) const
{
    return CompareArrays(rkV) < 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector2<Real>::operator<= (const Vector2& rkV) const
{
    return CompareArrays(rkV) <= 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector2<Real>::operator> (const Vector2& rkV) const
{
    return CompareArrays(rkV) > 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Vector2<Real>::operator>= (const Vector2& rkV) const
{
    return CompareArrays(rkV) >= 0;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::operator+ (const Vector2& rkV) const
{
    return Vector2(
        m_afTuple[0]+rkV.m_afTuple[0],
        m_afTuple[1]+rkV.m_afTuple[1]);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::operator- (const Vector2& rkV) const
{
    return Vector2(
        m_afTuple[0]-rkV.m_afTuple[0],
        m_afTuple[1]-rkV.m_afTuple[1]);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::operator* (Real fScalar) const
{
    return Vector2(
        fScalar*m_afTuple[0],
        fScalar*m_afTuple[1]);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::operator/ (Real fScalar) const
{
    Vector2 kQuot;

    if (fScalar != (Real)0.0)
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        kQuot.m_afTuple[0] = fInvScalar*m_afTuple[0];
        kQuot.m_afTuple[1] = fInvScalar*m_afTuple[1];
    }
    else
    {
        kQuot.m_afTuple[0] = Math<Real>::MAX_REAL;
        kQuot.m_afTuple[1] = Math<Real>::MAX_REAL;
    }

    return kQuot;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::operator- () const
{
    return Vector2(
        -m_afTuple[0],
        -m_afTuple[1]);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> operator* (Real fScalar, const Vector2<Real>& rkV)
{
    return Vector2<Real>(
        fScalar*rkV[0],
        fScalar*rkV[1]);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>& Vector2<Real>::operator+= (const Vector2& rkV)
{
    m_afTuple[0] += rkV.m_afTuple[0];
    m_afTuple[1] += rkV.m_afTuple[1];
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>& Vector2<Real>::operator-= (const Vector2& rkV)
{
    m_afTuple[0] -= rkV.m_afTuple[0];
    m_afTuple[1] -= rkV.m_afTuple[1];
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>& Vector2<Real>::operator*= (Real fScalar)
{
    m_afTuple[0] *= fScalar;
    m_afTuple[1] *= fScalar;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real>& Vector2<Real>::operator/= (Real fScalar)
{
    if (fScalar != (Real)0.0)
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        m_afTuple[0] *= fInvScalar;
        m_afTuple[1] *= fInvScalar;
    }
    else
    {
        m_afTuple[0] = Math<Real>::MAX_REAL;
        m_afTuple[1] = Math<Real>::MAX_REAL;
    }

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::Length () const
{
    return Math<Real>::Sqrt(
        m_afTuple[0]*m_afTuple[0] +
        m_afTuple[1]*m_afTuple[1]);
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::SquaredLength () const
{
    return
        m_afTuple[0]*m_afTuple[0] +
        m_afTuple[1]*m_afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::Dot (const Vector2& rkV) const
{
    return
        m_afTuple[0]*rkV.m_afTuple[0] +
        m_afTuple[1]*rkV.m_afTuple[1];
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::Normalize ()
{
    Real fLength = Length();

    if (fLength > Math<Real>::ZERO_TOLERANCE)
    {
        Real fInvLength = ((Real)1.0)/fLength;
        m_afTuple[0] *= fInvLength;
        m_afTuple[1] *= fInvLength;
    }
    else
    {
        fLength = (Real)0.0;
        m_afTuple[0] = (Real)0.0;
        m_afTuple[1] = (Real)0.0;
    }

    return fLength;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::Perp () const
{
    return Vector2(m_afTuple[1],-m_afTuple[0]);
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Vector2<Real>::UnitPerp () const
{
    Vector2 kPerp(m_afTuple[1],-m_afTuple[0]);
    kPerp.Normalize();
    return kPerp;
}
//----------------------------------------------------------------------------
template <class Real>
Real Vector2<Real>::DotPerp (const Vector2& rkV) const
{
    return m_afTuple[0]*rkV.m_afTuple[1] - m_afTuple[1]*rkV.m_afTuple[0];
}
//----------------------------------------------------------------------------
template <class Real>
void Vector2<Real>::GetBarycentrics (const Vector2& rkV0, const Vector2& rkV1,
    const Vector2& rkV2, Real afBary[3]) const
{
    // compute the vectors relative to V2 of the triangle
    Vector2 akDiff[3] =
    {
        rkV0 - rkV2,
        rkV1 - rkV2,
        *this - rkV2
    };

    // If the vertices have large magnitude, the linear system of equations
    // for computing barycentric coordinates can be ill-conditioned.  To avoid
    // this, uniformly scale the triangle edges to be of order 1.  The scaling
    // of all differences does not change the barycentric coordinates.
    Real fMax = (Real)0.0;
    int i;
    for (i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            Real fValue = Math<Real>::FAbs(akDiff[i][j]);
            if (fValue > fMax)
            {
                fMax = fValue;
            }
        }
    }

    // scale down only large data
    if (fMax > (Real)1.0)
    {
        Real fInvMax = ((Real)1.0)/fMax;
        for (i = 0; i < 3; i++)
        {
            akDiff[i] *= fInvMax;
        }
    }

    Real fDet = akDiff[0].DotPerp(akDiff[1]);
    if (Math<Real>::FAbs(fDet) > Math<Real>::ZERO_TOLERANCE)
    {
        Real fInvDet = ((Real)1.0)/fDet;
        afBary[0] = akDiff[2].DotPerp(akDiff[1])*fInvDet;
        afBary[1] = akDiff[0].DotPerp(akDiff[2])*fInvDet;
        afBary[2] = (Real)1.0 - afBary[0] - afBary[1];
    }
    else
    {
        // The triangle is a sliver.  Determine the longest edge and
        // compute barycentric coordinates with respect to that edge.
        Vector2 kE2 = rkV0 - rkV1;
        Real fMaxSqrLength = kE2.SquaredLength();
        int iMaxIndex = 2;
        Real fSqrLength = akDiff[1].SquaredLength();
        if (fSqrLength > fMaxSqrLength)
        {
            iMaxIndex = 1;
            fMaxSqrLength = fSqrLength;
        }
        fSqrLength = akDiff[0].SquaredLength();
        if (fSqrLength > fMaxSqrLength)
        {
            iMaxIndex = 0;
            fMaxSqrLength = fSqrLength;
        }

        if (fMaxSqrLength > Math<Real>::ZERO_TOLERANCE)
        {
            Real fInvSqrLength = ((Real)1.0)/fMaxSqrLength;
            if (iMaxIndex == 0)
            {
                // P-V2 = t(V0-V2)
                afBary[0] = akDiff[2].Dot(akDiff[0])*fInvSqrLength;
                afBary[1] = (Real)0.0;
                afBary[2] = (Real)1.0 - afBary[0];
            }
            else if (iMaxIndex == 1)
            {
                // P-V2 = t(V1-V2)
                afBary[0] = (Real)0.0;
                afBary[1] = akDiff[2].Dot(akDiff[1])*fInvSqrLength;
                afBary[2] = (Real)1.0 - afBary[1];
            }
            else
            {
                // P-V1 = t(V0-V1)
                akDiff[2] = *this - rkV1;
                afBary[0] = akDiff[2].Dot(kE2)*fInvSqrLength;
                afBary[1] = (Real)1.0 - afBary[0];
                afBary[2] = (Real)0.0;
            }
        }
        else
        {
            // triangle is a nearly a point, just return equal weights
            afBary[0] = ((Real)1.0)/(Real)3.0;
            afBary[1] = afBary[0];
            afBary[2] = afBary[0];
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Vector2<Real>::Orthonormalize (Vector2& rkU, Vector2& rkV)
{
    // If the input vectors are v0 and v1, then the Gram-Schmidt
    // orthonormalization produces vectors u0 and u1 as follows,
    //
    //   u0 = v0/|v0|
    //   u1 = (v1-(u0*v1)u0)/|v1-(u0*v1)u0|
    //
    // where |A| indicates length of vector A and A*B indicates dot
    // product of vectors A and B.

    // compute u0
    rkU.Normalize();

    // compute u1
    Real fDot0 = rkU.Dot(rkV); 
    rkV -= rkU*fDot0;
    rkV.Normalize();
}
//----------------------------------------------------------------------------
template <class Real>
void Vector2<Real>::GenerateOrthonormalBasis (Vector2& rkU, Vector2& rkV)
{
    rkV.Normalize();
    rkU = rkV.Perp();
}
//----------------------------------------------------------------------------
template <class Real>
void Vector2<Real>::ComputeExtremes (int iVQuantity, const Vector2* akPoint,
    Vector2& rkMin, Vector2& rkMax)
{
    assert(iVQuantity > 0 && akPoint);

    rkMin = akPoint[0];
    rkMax = rkMin;
    for (int i = 1; i < iVQuantity; i++)
    {
        const Vector2<Real>& rkPoint = akPoint[i];
        for (int j = 0; j < 2; j++)
        {
            if (rkPoint[j] < rkMin[j])
            {
                rkMin[j] = rkPoint[j];
            }
            else if (rkPoint[j] > rkMax[j])
            {
                rkMax[j] = rkPoint[j];
            }
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
std::ostream& operator<< (std::ostream& rkOStr, const Vector2<Real>& rkV)
{
     return rkOStr << rkV.X() << ' ' << rkV.Y();
}
//----------------------------------------------------------------------------
} //namespace Wm4
