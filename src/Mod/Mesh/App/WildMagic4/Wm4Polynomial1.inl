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
Polynomial1<Real>::Polynomial1 (int iDegree)
{
    if (iDegree >= 0)
    {
        m_iDegree = iDegree;
        m_afCoeff = WM4_NEW Real[m_iDegree+1];
    }
    else
    {
        // default creation
        m_iDegree = -1;
        m_afCoeff = nullptr;
    }
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real>::Polynomial1 (const Polynomial1& rkPoly)
{
    m_iDegree = rkPoly.m_iDegree;
    m_afCoeff = WM4_NEW Real[m_iDegree+1];
    for (int i = 0; i <= m_iDegree; i++)
    {
        m_afCoeff[i] = rkPoly.m_afCoeff[i];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real>::~Polynomial1 ()
{
    WM4_DELETE[] m_afCoeff;
}
//----------------------------------------------------------------------------
template <class Real>
void Polynomial1<Real>::SetDegree (int iDegree)
{
    m_iDegree = iDegree;
    WM4_DELETE[] m_afCoeff;

    if (m_iDegree >= 0)
    {
        m_afCoeff = WM4_NEW Real[m_iDegree+1];
    }
    else
    {
        m_afCoeff = nullptr;
    }
}
//----------------------------------------------------------------------------
template <class Real>
int Polynomial1<Real>::GetDegree () const
{
    return m_iDegree;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real>::operator const Real* () const
{
    return m_afCoeff;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real>::operator Real* ()
{
    return m_afCoeff;
}
//----------------------------------------------------------------------------
template <class Real>
Real Polynomial1<Real>::operator[] (int i) const
{
    assert(0 <= i && i <= m_iDegree);
    return m_afCoeff[i];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Polynomial1<Real>::operator[] (int i)
{
    assert(0 <= i && i <= m_iDegree);
    return m_afCoeff[i];
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real>& Polynomial1<Real>::operator= (const Polynomial1& rkPoly)
{
    WM4_DELETE[] m_afCoeff;
    m_iDegree = rkPoly.m_iDegree;

    if (m_iDegree >= 0)
    {
        m_afCoeff = WM4_NEW Real[m_iDegree+1];
        for (int i = 0; i <= m_iDegree; i++)
        {
            m_afCoeff[i] = rkPoly.m_afCoeff[i];
        }
    }

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Real Polynomial1<Real>::operator() (Real fT) const
{
    assert(m_iDegree >= 0);

    Real fResult = m_afCoeff[m_iDegree];
    for (int i = m_iDegree-1; i >= 0; i--)
    {
        fResult *= fT;
        fResult += m_afCoeff[i];
    }
    return fResult;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real> Polynomial1<Real>::operator+ (const Polynomial1& rkPoly)
    const
{
    assert(m_iDegree >= 0 && rkPoly.m_iDegree >= 0);

    Polynomial1 kSum;
    int i;

    if (m_iDegree > rkPoly.m_iDegree)
    {
        kSum.SetDegree(m_iDegree);
        for (i = 0; i <= rkPoly.m_iDegree; i++)
        {
            kSum.m_afCoeff[i] = m_afCoeff[i] + rkPoly.m_afCoeff[i];
        }
        for (i = rkPoly.m_iDegree+1; i <= m_iDegree; i++)
        {
            kSum.m_afCoeff[i] = m_afCoeff[i];
        }

    }
    else
    {
        kSum.SetDegree(rkPoly.m_iDegree);
        for (i = 0; i <= m_iDegree; i++)
        {
            kSum.m_afCoeff[i] = m_afCoeff[i] + rkPoly.m_afCoeff[i];
        }
        for (i = m_iDegree+1; i <= rkPoly.m_iDegree; i++)
        {
            kSum.m_afCoeff[i] = rkPoly.m_afCoeff[i];
        }
    }

    return kSum;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real> Polynomial1<Real>::operator- (const Polynomial1& rkPoly)
    const
{
    assert(m_iDegree >= 0 && rkPoly.m_iDegree >= 0);

    Polynomial1 kDiff;
    int i;

    if (m_iDegree > rkPoly.m_iDegree)
    {
        kDiff.SetDegree(m_iDegree);
        for (i = 0; i <= rkPoly.m_iDegree; i++)
        {
            kDiff.m_afCoeff[i] = m_afCoeff[i] - rkPoly.m_afCoeff[i];
        }
        for (i = rkPoly.m_iDegree+1; i <= m_iDegree; i++)
        {
            kDiff.m_afCoeff[i] = m_afCoeff[i];
        }

    }
    else
    {
        kDiff.SetDegree(rkPoly.m_iDegree);
        for (i = 0; i <= m_iDegree; i++)
        {
            kDiff.m_afCoeff[i] = m_afCoeff[i] - rkPoly.m_afCoeff[i];
        }
        for (i = m_iDegree+1; i <= rkPoly.m_iDegree; i++)
        {
            kDiff.m_afCoeff[i] = -rkPoly.m_afCoeff[i];
        }
    }

    return kDiff;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real> Polynomial1<Real>::operator* (const Polynomial1& rkPoly)
    const
{
    assert(m_iDegree >= 0 && rkPoly.m_iDegree >= 0);

    Polynomial1 kProd(m_iDegree + rkPoly.m_iDegree);

    memset(kProd.m_afCoeff,0,(kProd.m_iDegree+1)*sizeof(Real));
    for (int i0 = 0; i0 <= m_iDegree; i0++)
    {
        for (int i1 = 0; i1 <= rkPoly.m_iDegree; i1++)
        {
            int i2 = i0 + i1;
            kProd.m_afCoeff[i2] += m_afCoeff[i0]*rkPoly.m_afCoeff[i1];
        }
    }

    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real> Polynomial1<Real>::operator+ (Real fScalar) const
{
    assert(m_iDegree >= 0);
    Polynomial1 kSum(m_iDegree);
    kSum.m_afCoeff[0] += fScalar;
    return kSum;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real> Polynomial1<Real>::operator- (Real fScalar) const
{
    assert(m_iDegree >= 0);
    Polynomial1 kDiff(m_iDegree);
    kDiff.m_afCoeff[0] -= fScalar;
    return kDiff;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real> Polynomial1<Real>::operator* (Real fScalar) const
{
    assert(m_iDegree >= 0);
    Polynomial1 kProd(m_iDegree);
    for (int i = 0; i <= m_iDegree; i++)
    {
        kProd.m_afCoeff[i] = fScalar*m_afCoeff[i];
    }
    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real> Polynomial1<Real>::operator/ (Real fScalar) const
{
    assert(m_iDegree >= 0);
    Polynomial1 kProd(m_iDegree);
    int i;

    if (fScalar != (Real)0.0)
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        for (i = 0; i <= m_iDegree; i++)
        {
            kProd.m_afCoeff[i] = fInvScalar*m_afCoeff[i];
        }
    }
    else
    {
        for (i = 0; i <= m_iDegree; i++)
        {
            kProd.m_afCoeff[i] = Math<Real>::MAX_REAL;
        }
    }

    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real> Polynomial1<Real>::operator- () const
{
    assert(m_iDegree >= 0);

    Polynomial1 kNeg(m_iDegree);
    for (int i = 0; i <= m_iDegree; i++)
    {
        kNeg.m_afCoeff[i] = -m_afCoeff[i];
    }

    return kNeg;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real> operator* (Real fScalar,
    const Polynomial1<Real>& rkPoly)
{
    assert(rkPoly.GetDegree() >= 0);

    Polynomial1<Real> kProd(rkPoly.GetDegree());
    for (int i = 0; i <= rkPoly.GetDegree(); i++)
    {
        kProd[i] = fScalar*rkPoly[i];
    }

    return kProd;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real>& Polynomial1<Real>::operator += (const Polynomial1& rkPoly)
{
    assert(m_iDegree >= 0);
    *this = *this + rkPoly;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real>& Polynomial1<Real>::operator -= (const Polynomial1& rkPoly)
{
    assert(m_iDegree >= 0);
    *this = *this - rkPoly;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real>& Polynomial1<Real>::operator *= (const Polynomial1& rkPoly)
{
    assert(m_iDegree >= 0);
    *this = (*this)*rkPoly;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real>& Polynomial1<Real>::operator += (Real fScalar)
{
    assert(m_iDegree >= 0);
    m_afCoeff[0] += fScalar;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real>& Polynomial1<Real>::operator -= (Real fScalar)
{
    assert(m_iDegree >= 0);
    m_afCoeff[0] -= fScalar;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real>& Polynomial1<Real>::operator *= (Real fScalar)
{
    assert(m_iDegree >= 0);
    *this = (*this)*fScalar;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real>& Polynomial1<Real>::operator /= (Real fScalar)
{
    assert(m_iDegree >= 0);
    *this = (*this)/fScalar;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real> Polynomial1<Real>::GetDerivative () const
{
    if (m_iDegree > 0)
    {
        Polynomial1 kDeriv(m_iDegree-1);
        for (int i0 = 0, i1 = 1; i0 < m_iDegree; i0++, i1++)
        {
            kDeriv.m_afCoeff[i0] = i1*m_afCoeff[i1];
        }
        return kDeriv;
    }
    else if (m_iDegree == 0)
    {
        Polynomial1 kConst(0);
        kConst.m_afCoeff[0] = (Real)0.0;
        return kConst;
    }
    return Polynomial1<Real>();  // invalid in, invalid out
}
//----------------------------------------------------------------------------
template <class Real>
Polynomial1<Real> Polynomial1<Real>::GetInversion () const
{
    Polynomial1 kInvPoly(m_iDegree);
    for (int i = 0; i <= m_iDegree; i++)
    {
        kInvPoly.m_afCoeff[i] = m_afCoeff[m_iDegree-i];
    }
    return kInvPoly;
}
//----------------------------------------------------------------------------
template <class Real>
void Polynomial1<Real>::Compress (Real fEpsilon)
{
    int i;
    for (i = m_iDegree; i >= 0; i--)
    {
        if (Math<Real>::FAbs(m_afCoeff[i]) <= fEpsilon)
        {
            m_iDegree--;
        }
        else
        {
            break;
        }
    }

    if (m_iDegree >= 0)
    {
        Real fInvLeading = ((Real)1.0)/m_afCoeff[m_iDegree];
        m_afCoeff[m_iDegree] = (Real)1.0;
        for (i = 0; i < m_iDegree; i++)
        {
            m_afCoeff[i] *= fInvLeading;
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
void Polynomial1<Real>::Divide (const Polynomial1& rkDiv, Polynomial1& rkQuot,
    Polynomial1& rkRem, Real fEpsilon) const
{
    int iQuotDegree = m_iDegree - rkDiv.m_iDegree;
    if (iQuotDegree >= 0)
    {
        rkQuot.SetDegree(iQuotDegree);

        // temporary storage for the remainder
        Polynomial1 kTmp = *this;

        // do the division (Euclidean algorithm)
        Real fInv = ((Real)1.0)/rkDiv[rkDiv.m_iDegree];
        for (int iQ = iQuotDegree; iQ >= 0; iQ--)
        {
            int iR = rkDiv.m_iDegree + iQ;
            rkQuot[iQ] = fInv*kTmp[iR];
            for (iR--; iR >= iQ; iR--)
            {
                kTmp[iR] -= rkQuot[iQ]*rkDiv[iR-iQ];
            }
        }

        // calculate the correct degree for the remainder
        int iRemDeg = rkDiv.m_iDegree - 1;
        while (iRemDeg > 0 && Math<Real>::FAbs(kTmp[iRemDeg]) < fEpsilon)
        {
            iRemDeg--;
        }

        if (iRemDeg == 0 && Math<Real>::FAbs(kTmp[0]) < fEpsilon)
        {
            kTmp[0] = (Real)0.0;
        }

        rkRem.SetDegree(iRemDeg);
        size_t uiSize = (iRemDeg+1)*sizeof(Real);
        System::Memcpy(rkRem.m_afCoeff,uiSize,kTmp.m_afCoeff,uiSize);
    }
    else
    {
        rkQuot.SetDegree(0);
        rkQuot[0] = (Real)0.0;
        rkRem = *this;
    }
}
//----------------------------------------------------------------------------
} //namespace Wm4
