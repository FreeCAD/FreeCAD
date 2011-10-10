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
#include "Wm4PolynomialRoots.h"

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
PolynomialRoots<Real>::PolynomialRoots (Real fEpsilon)
{
    assert(fEpsilon >= (Real)0.0);
    m_fEpsilon = fEpsilon;
    m_iMaxIterations = 128;
    m_iCount = 0;
    m_iMaxRoot = 4;  // default support for degree <= 4
    m_afRoot = WM4_NEW Real[m_iMaxRoot];
}
//----------------------------------------------------------------------------
template <class Real>
PolynomialRoots<Real>::~PolynomialRoots ()
{
    WM4_DELETE[] m_afRoot;
}
//----------------------------------------------------------------------------
template <class Real>
int PolynomialRoots<Real>::GetCount () const
{
    return m_iCount;
}
//----------------------------------------------------------------------------
template <class Real>
const Real* PolynomialRoots<Real>::GetRoots () const
{
    return m_afRoot;
}
//----------------------------------------------------------------------------
template <class Real>
Real PolynomialRoots<Real>::GetRoot (int i) const
{
    assert(0 <= i && i < m_iCount);
    if (0 <= i && i < m_iCount)
    {
        return m_afRoot[i];
    }

    return Math<Real>::MAX_REAL;
}
//----------------------------------------------------------------------------
template <class Real>
Real& PolynomialRoots<Real>::Epsilon ()
{
    return m_fEpsilon;
}
//----------------------------------------------------------------------------
template <class Real>
Real PolynomialRoots<Real>::Epsilon () const
{
    return m_fEpsilon;
}
//----------------------------------------------------------------------------
template <class Real>
int& PolynomialRoots<Real>::MaxIterations ()
{
    return m_iMaxIterations;
}
//----------------------------------------------------------------------------
template <class Real>
int PolynomialRoots<Real>::MaxIterations () const
{
    return m_iMaxIterations;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// degree 1
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::FindA (Real fC0, Real fC1)
{
    if (Math<Real>::FAbs(fC1) >= m_fEpsilon)
    {
        m_afRoot[0] = -fC0/fC1;
        m_iCount = 1;
        return true;
    }

    m_iCount = 0;
    return false;
}
//----------------------------------------------------------------------------
template <class Real>
Real PolynomialRoots<Real>::GetBound (Real fC0, Real fC1)
{
    if (Math<Real>::FAbs(fC1) <= m_fEpsilon)
    {
        // polynomial is constant, return invalid bound
        return -(Real)1.0;
    }

    Real fInvC1 = ((Real)1.0)/fC1;
    Real fMax = Math<Real>::FAbs(fC0)*fInvC1;
    return (Real)1.0 + fMax;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// degree 2
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::FindA (Real fC0, Real fC1, Real fC2)
{
    if (Math<Real>::FAbs(fC2) <= m_fEpsilon)
    {
        // polynomial is linear
        return FindA(fC0,fC1);
    }

    Real fDiscr = fC1*fC1 - 4.0f*fC0*fC2;
    if (Math<Real>::FAbs(fDiscr) <= m_fEpsilon)
    {
        fDiscr = (Real)0.0;
    }

    if (fDiscr < (Real)0.0)
    {
        m_iCount = 0;
        return false;
    }

    Real fTmp = ((Real)0.5)/fC2;

    if (fDiscr > (Real)0.0)
    {
        fDiscr = Math<Real>::Sqrt(fDiscr);
        m_afRoot[0] = fTmp*(-fC1 - fDiscr);
        m_afRoot[1] = fTmp*(-fC1 + fDiscr);
        m_iCount = 2;
    }
    else
    {
        m_afRoot[0] = -fTmp*fC1;
        m_iCount = 1;
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
Real PolynomialRoots<Real>::GetBound (Real fC0, Real fC1, Real fC2)
{
    if (Math<Real>::FAbs(fC2) <= m_fEpsilon)
    {
        // polynomial is linear
        return (FindA(fC0,fC1) ? m_afRoot[0] : Math<Real>::MAX_REAL);
    }

    Real fInvC2 = ((Real)1.0)/fC2;
    Real fTmp0 = Math<Real>::FAbs(fC0)*fInvC2;
    Real fTmp1 = Math<Real>::FAbs(fC1)*fInvC2;
    Real fMax = ( fTmp0 >= fTmp1 ? fTmp0 : fTmp1 );
    return (Real)1.0 + fMax;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// degree 3
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::FindA (Real fC0, Real fC1, Real fC2, Real fC3)
{
    if (Math<Real>::FAbs(fC3) <= m_fEpsilon)
    {
        // polynomial is quadratic
        return FindA(fC0,fC1,fC2);
    }

    // make polynomial monic, x^3+c2*x^2+c1*x+c0
    Real fInvC3 = ((Real)1.0)/fC3;
    fC0 *= fInvC3;
    fC1 *= fInvC3;
    fC2 *= fInvC3;

    // convert to y^3+a*y+b = 0 by x = y-c2/3
    const Real fThird = (Real)1.0/(Real)3.0;
    const Real fTwentySeventh = (Real)1.0/(Real)27.0;
    Real fOffset = fThird*fC2;
    Real fA = fC1 - fC2*fOffset;
    Real fB = fC0+fC2*(((Real)2.0)*fC2*fC2-((Real)9.0)*fC1)*fTwentySeventh;
    Real fHalfB = ((Real)0.5)*fB;

    Real fDiscr = fHalfB*fHalfB + fA*fA*fA*fTwentySeventh;
    if (Math<Real>::FAbs(fDiscr) <= m_fEpsilon)
    {
        fDiscr = (Real)0.0;
    }

    if (fDiscr > (Real)0.0)  // 1 real, 2 complex roots
    {
        fDiscr = Math<Real>::Sqrt(fDiscr);
        Real fTemp = -fHalfB + fDiscr;
        if (fTemp >= (Real)0.0)
        {
            m_afRoot[0] = Math<Real>::Pow(fTemp,fThird);
        }
        else
        {
            m_afRoot[0] = -Math<Real>::Pow(-fTemp,fThird);
        }
        fTemp = -fHalfB - fDiscr;
        if ( fTemp >= (Real)0.0 )
        {
            m_afRoot[0] += Math<Real>::Pow(fTemp,fThird);
        }
        else
        {
            m_afRoot[0] -= Math<Real>::Pow(-fTemp,fThird);
        }
        m_afRoot[0] -= fOffset;
        m_iCount = 1;
    }
    else if (fDiscr < (Real)0.0) 
    {
        const Real fSqrt3 = Math<Real>::Sqrt((Real)3.0);
        Real fDist = Math<Real>::Sqrt(-fThird*fA);
        Real fAngle = fThird*Math<Real>::ATan2(Math<Real>::Sqrt(-fDiscr),
            -fHalfB);
        Real fCos = Math<Real>::Cos(fAngle);
        Real fSin = Math<Real>::Sin(fAngle);
        m_afRoot[0] = ((Real)2.0)*fDist*fCos-fOffset;
        m_afRoot[1] = -fDist*(fCos+fSqrt3*fSin)-fOffset;
        m_afRoot[2] = -fDist*(fCos-fSqrt3*fSin)-fOffset;
        m_iCount = 3;
    }
    else 
    {
        Real fTemp;
        if (fHalfB >= (Real)0.0)
        {
            fTemp = -Math<Real>::Pow(fHalfB,fThird);
        }
        else
        {
            fTemp = Math<Real>::Pow(-fHalfB,fThird);
        }
        m_afRoot[0] = ((Real)2.0)*fTemp-fOffset;
        m_afRoot[1] = -fTemp-fOffset;
        m_afRoot[2] = m_afRoot[1];
        m_iCount = 3;
    }

    return true;
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::FindE (Real fC0, Real fC1, Real fC2, Real fC3,
    bool bDoBalancing)
{
    if (Math<Real>::FAbs(fC3) <= m_fEpsilon)
    {
        // polynomial is quadratic
        return FindA(fC0,fC1,fC2);
    }

    // make polynomial monic, x^3+c2*x^2+c1*x+c0
    Real fInvC3 = ((Real)1.0)/fC3;
    fC0 *= fInvC3;
    fC1 *= fInvC3;
    fC2 *= fInvC3;

    // construct the 3-by-3 companion matrix
    GMatrix<Real> kMat(3,3);  // initialized to zero
    kMat[1][0] = (Real)1.0;
    kMat[2][1] = (Real)1.0;
    kMat[0][2] = -fC0;
    kMat[1][2] = -fC1;
    kMat[2][2] = -fC2;

    if (bDoBalancing)
    {
        BalanceCompanion3(kMat);
    }

    return QRIteration3(kMat);
}
//----------------------------------------------------------------------------
template <class Real>
Real PolynomialRoots<Real>::GetBound (Real fC0, Real fC1, Real fC2, Real fC3)
{
    if (Math<Real>::FAbs(fC3) <= m_fEpsilon)
    {
        // polynomial is quadratic
        return GetBound(fC0,fC1,fC2);
    }

    Real fInvC3 = ((Real)1.0)/fC3;
    Real fMax = Math<Real>::FAbs(fC0)*fInvC3;

    Real fTmp = Math<Real>::FAbs(fC1)*fInvC3;
    if (fTmp > fMax)
    {
        fMax = fTmp;
    }

    fTmp = Math<Real>::FAbs(fC2)*fInvC3;
    if (fTmp > fMax)
    {
        fMax = fTmp;
    }

    return (Real)1.0 + fMax;
}
//----------------------------------------------------------------------------
template <class Real>
Real PolynomialRoots<Real>::SpecialCubic (Real fA, Real fB, Real fC)
{
    // Solve A*r^3 + B*r = C where A > 0 and B > 0.
    //
    // Let r = D*sinh(u) where D = sqrt(4*B/(3*A)).  Then
    // sinh(3*u) = 4*[sinh(u)]^3+3*sinh(u) = E where E = 4*C/(A*D^3).
    // sinh(3*u) = E has solution u = (1/3)*log(E+sqrt(E^2+1)).  This
    // leads to sinh(u) = ((E+sqrt(E^2+1))^{1/3}-(E+sqrt(E^2+1))^{-1/3})/2.
    // Therefore,  r = D*((E+sqrt(E^2+1))^{1/3}-(E+sqrt(E^2+1))^{-1/3})/2.

    const Real fThird = (Real)1.0/(Real)3.0;
    Real fD = Math<Real>::Sqrt(((Real)4.0)*fThird*fB/fA);
    Real fE = ((Real)4.0)*fC/(fA*fD*fD*fD);
    Real fF = Math<Real>::Pow(fE+Math<Real>::Sqrt(fE*fE+(Real)1.0),fThird);
    Real fRoot = ((Real)0.5)*fD*(fF-((Real)1.0)/fF);
    return fRoot;
}
//----------------------------------------------------------------------------
template <class Real>
Real PolynomialRoots<Real>::GetRowNorm (int iRow, GMatrix<Real>& rkMat)
{
    Real fNorm = Math<Real>::FAbs(rkMat[iRow][0]);
    for (int iCol = 1; iCol < rkMat.GetColumns(); iCol++)
    {
        Real fAbs = Math<Real>::FAbs(rkMat[iRow][iCol]);
        if (fAbs > fNorm)
        {
            fNorm = fAbs;
        }
    }
    return fNorm;
}
//----------------------------------------------------------------------------
template <class Real>
Real PolynomialRoots<Real>::GetColNorm (int iCol, GMatrix<Real>& rkMat)
{
    Real fNorm = Math<Real>::FAbs(rkMat[0][iCol]);
    for (int iRow = 1; iRow < rkMat.GetRows(); iRow++)
    {
        Real fAbs = Math<Real>::FAbs(rkMat[iRow][iCol]);
        if (fAbs > fNorm)
        {
            fNorm = fAbs;
        }
    }
    return fNorm;
}
//----------------------------------------------------------------------------
template <class Real>
void PolynomialRoots<Real>::ScaleRow (int iRow, Real fScale,
    GMatrix<Real>& rkMat)
{
    for (int iCol = 0; iCol < rkMat.GetColumns(); iCol++)
    {
        rkMat[iRow][iCol] *= fScale;
    }
}
//----------------------------------------------------------------------------
template <class Real>
void PolynomialRoots<Real>::ScaleCol (int iCol, Real fScale,
    GMatrix<Real>& rkMat)
{
    for (int iRow = 0; iRow < rkMat.GetRows(); iRow++)
    {
        rkMat[iRow][iCol] *= fScale;
    }
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::IsBalanced3 (GMatrix<Real>& rkMat)
{
    const Real fTolerance = (Real)0.001;
    for (int i = 0; i < 3; i++)
    {
        Real fRowNorm = GetRowNorm(i,rkMat);
        Real fColNorm = GetColNorm(i,rkMat);
        Real fTest = Math<Real>::FAbs((Real)1.0 - fColNorm/fRowNorm);
        if (fTest > fTolerance)
        {
            return false;
        }
    }
    return true;
}
//----------------------------------------------------------------------------
template <class Real>
void PolynomialRoots<Real>::Balance3 (GMatrix<Real>& rkMat)
{
    const int iMax = 16;
    int i;
    for (i = 0; i < iMax; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            Real fRowNorm = GetRowNorm(j,rkMat);
            Real fColNorm = GetColNorm(j,rkMat);
            Real fScale = Math<Real>::Sqrt(fColNorm/fRowNorm);
            Real fInvScale = ((Real)1.0)/fScale;
            ScaleRow(j,fScale,rkMat);
            ScaleCol(j,fInvScale,rkMat);
        }

        if (IsBalanced3(rkMat))
        {
            break;
        }
    }
    assert(i < iMax);
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::IsBalancedCompanion3 (Real fA10, Real fA21,
    Real fA02, Real fA12, Real fA22)
{
    const Real fTolerance = (Real)0.001;

    // row/col 0
    Real fRowNorm = fA02;
    Real fColNorm = fA10;
    Real fTest = Math<Real>::FAbs((Real)1.0 - fColNorm/fRowNorm);
    if (fTest > fTolerance)
    {
        return false;
    }

    // row/col 1
    fRowNorm = (fA10 >= fA12 ? fA10 : fA12);
    fColNorm = fA21;
    fTest = Math<Real>::FAbs((Real)1.0 - fColNorm/fRowNorm);
    if (fTest > fTolerance)
    {
        return false;
    }

    // row/col 2
    fRowNorm = (fA21 >= fA22 ? fA21 : fA22);
    fColNorm = (fA02 >= fA12 ? fA02 : fA12);
    if (fA22 > fColNorm)
    {
        fColNorm = fA22;
    }
    fTest = Math<Real>::FAbs((Real)1.0 - fColNorm/fRowNorm);
    return fTest <= fTolerance;
}
//----------------------------------------------------------------------------
template <class Real>
void PolynomialRoots<Real>::BalanceCompanion3 (GMatrix<Real>& rkMat)
{
    Real fA10 = Math<Real>::FAbs(rkMat[1][0]);
    Real fA21 = Math<Real>::FAbs(rkMat[2][1]);
    Real fA02 = Math<Real>::FAbs(rkMat[0][2]);
    Real fA12 = Math<Real>::FAbs(rkMat[1][2]);
    Real fA22 = Math<Real>::FAbs(rkMat[2][2]);
    Real fRowNorm, fColNorm, fScale, fInvScale;

    const int iMax = 16;
    int i;
    for (i = 0; i < iMax; i++)
    {
        // balance row/col 0
        fRowNorm = fA02;
        fColNorm = fA10;
        fScale = Math<Real>::Sqrt(fColNorm/fRowNorm);
        fA02 *= fScale;
        fA10 = fA02;

        // balance row/col 1
        fRowNorm = (fA10 >= fA12 ? fA10 : fA12);
        fColNorm = fA21;
        fScale = Math<Real>::Sqrt(fColNorm/fRowNorm);
        fInvScale = ((Real)1.0)/fScale;
        fA10 *= fScale;
        fA12 *= fScale;
        fA21 *= fInvScale;

        // balance row/col 2
        fRowNorm = (fA21 >= fA22 ? fA21 : fA22);
        fColNorm = (fA02 >= fA12 ? fA02 : fA12);
        if (fA22 > fColNorm)
        {
            fColNorm = fA22;
        }
        fScale = Math<Real>::Sqrt(fColNorm/fRowNorm);
        fInvScale = ((Real)1.0)/fScale;
        fA21 *= fScale;
        fA02 *= fInvScale;
        fA12 *= fInvScale;

        if (IsBalancedCompanion3(fA10,fA21,fA02,fA12,fA22))
        {
            break;
        }
    }
    assert(i < iMax);

    rkMat[1][0] = (rkMat[1][0] >= (Real)0.0 ? fA10 : -fA10);
    rkMat[2][1] = (rkMat[2][1] >= (Real)0.0 ? fA21 : -fA21);
    rkMat[0][2] = (rkMat[0][2] >= (Real)0.0 ? fA02 : -fA02);
    rkMat[1][2] = (rkMat[1][2] >= (Real)0.0 ? fA12 : -fA12);
    rkMat[2][2] = (rkMat[2][2] >= (Real)0.0 ? fA22 : -fA22);
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::QRIteration3 (GMatrix<Real>& rkMat)
{
    GVector<Real> kW(3);
    Real fRHS, fTrace, fDet;
    for (int i = 0; i < m_iMaxIterations; i++)
    {
        fRHS = m_fEpsilon*(Math<Real>::FAbs(rkMat[0][0]) +
            Math<Real>::FAbs(rkMat[1][1]));

        if (Math<Real>::FAbs(rkMat[1][0]) <= fRHS)
        {
            // mat[0][0] is a root, solve the quadratic for the submatrix
            fTrace = rkMat[1][1] + rkMat[2][2];
            fDet = rkMat[1][1]*rkMat[2][2] - rkMat[1][2]*rkMat[2][1];
            FindA(fDet,-fTrace,(Real)1.0);
            m_afRoot[m_iCount++] = rkMat[0][0];
            return true;
        }

        fRHS = m_fEpsilon*(Math<Real>::FAbs(rkMat[1][1]) +
            Math<Real>::FAbs(rkMat[2][2]));

        if (Math<Real>::FAbs(rkMat[2][1]) <= fRHS)
        {
            // mat[2][2] is a root, solve the quadratic for the submatrix
            fTrace = rkMat[0][0] + rkMat[1][1];
            fDet = rkMat[0][0]*rkMat[1][1] - rkMat[0][1]*rkMat[1][0];
            FindA(fDet,-fTrace,(Real)1.0);
            m_afRoot[m_iCount++] = rkMat[2][2];
            return true;
        }

        FrancisQRStep(rkMat,kW);
    }

    // TO DO: In theory, cubic polynomials always have one real-valued root,
    // but if the maximum iterations were exceeded, what to do?  Some
    // experiments show that when the polynomial nearly has a double root,
    // the convergence of the algorithm is slow.  Maybe a random perturbation
    // to "kick" the system a bit might work?
    //
    // If you want to trap exceeding the maximum iterations, uncomment the
    // 'assert' line of code.
    //
    // assert( false );

    // For now, zero out the smallest subdiagonal entry to decouple the
    // matrix.
    if (Math<Real>::FAbs(rkMat[1][0]) <= Math<Real>::FAbs(rkMat[2][1]))
    {
        // mat[0][0] is a root, solve the quadratic for the submatrix
        fTrace = rkMat[1][1] + rkMat[2][2];
        fDet = rkMat[1][1]*rkMat[2][2] - rkMat[1][2]*rkMat[2][1];
        FindA(fDet,-fTrace,(Real)1.0);
        m_afRoot[m_iCount++] = rkMat[0][0];
    }
    else
    {
        // mat[2][2] is a root, solve the quadratic for the submatrix
        fTrace = rkMat[0][0] + rkMat[1][1];
        fDet = rkMat[0][0]*rkMat[1][1] - rkMat[0][1]*rkMat[1][0];
        FindA(fDet,-fTrace,(Real)1.0);
        m_afRoot[m_iCount++] = rkMat[2][2];
    }

    return true;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// degree 4
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::FindA (Real fC0, Real fC1, Real fC2, Real fC3,
    Real fC4)
{
    if (Math<Real>::FAbs(fC4) <= m_fEpsilon)
    {
        // polynomial is cubic
        return FindA(fC0,fC1,fC2,fC3);
    }

    // make polynomial monic, x^4+c3*x^3+c2*x^2+c1*x+c0
    Real fInvC4 = ((Real)1.0)/fC4;
    fC0 *= fInvC4;
    fC1 *= fInvC4;
    fC2 *= fInvC4;
    fC3 *= fInvC4;

    // reduction to resolvent cubic polynomial y^3+r2*y^2+r1*y+r0 = 0
    Real fR0 = -fC3*fC3*fC0 + ((Real)4.0)*fC2*fC0 - fC1*fC1;
    Real fR1 = fC3*fC1 - ((Real)4.0)*fC0;
    Real fR2 = -fC2;
    FindA(fR0,fR1,fR2,(Real)1.0);  // always produces at least one root
    Real fY = m_afRoot[0];

    m_iCount = 0;
    Real fDiscr = ((Real)0.25)*fC3*fC3 - fC2 + fY;
    if (Math<Real>::FAbs(fDiscr) <= m_fEpsilon)
    {
        fDiscr = (Real)0.0;
    }

    if (fDiscr > (Real)0.0) 
    {
        Real fR = Math<Real>::Sqrt(fDiscr);
        Real fT1 = ((Real)0.75)*fC3*fC3 - fR*fR - ((Real)2.0)*fC2;
        Real fT2 = (((Real)4.0)*fC3*fC2 - ((Real)8.0)*fC1 - fC3*fC3*fC3) /
            (((Real)4.0)*fR);

        Real fTplus = fT1+fT2;
        Real fTminus = fT1-fT2;
        if (Math<Real>::FAbs(fTplus) <= m_fEpsilon) 
        {
            fTplus = (Real)0.0;
        }
        if (Math<Real>::FAbs(fTminus) <= m_fEpsilon) 
        {
            fTminus = (Real)0.0;
        }

        if (fTplus >= (Real)0.0)
        {
            Real fD = Math<Real>::Sqrt(fTplus);
            m_afRoot[0] = -((Real)0.25)*fC3+((Real)0.5)*(fR+fD);
            m_afRoot[1] = -((Real)0.25)*fC3+((Real)0.5)*(fR-fD);
            m_iCount += 2;
        }
        if (fTminus >= (Real)0.0)
        {
            Real fE = Math<Real>::Sqrt(fTminus);
            m_afRoot[m_iCount++] = -((Real)0.25)*fC3+((Real)0.5)*(fE-fR);
            m_afRoot[m_iCount++] = -((Real)0.25)*fC3-((Real)0.5)*(fE+fR);
        }
    }
    else if (fDiscr < (Real)0.0)
    {
        m_iCount = 0;
    }
    else
    {
        Real fT2 = fY*fY-((Real)4.0)*fC0;
        if (fT2 >= -m_fEpsilon) 
        {
            if (fT2 < (Real)0.0) // round to zero
            {
                fT2 = (Real)0.0;
            }
            fT2 = ((Real)2.0)*Math<Real>::Sqrt(fT2);
            Real fT1 = ((Real)0.75)*fC3*fC3 - ((Real)2.0)*fC2;
            if (fT1+fT2 >= m_fEpsilon) 
            {
                Real fD = Math<Real>::Sqrt(fT1+fT2);
                m_afRoot[0] = -((Real)0.25)*fC3+((Real)0.5)*fD;
                m_afRoot[1] = -((Real)0.25)*fC3-((Real)0.5)*fD;
                m_iCount += 2;
            }
            if (fT1-fT2 >= m_fEpsilon) 
            {
                Real fE = Math<Real>::Sqrt(fT1-fT2);
                m_afRoot[m_iCount++] = -((Real)0.25)*fC3+((Real)0.5)*fE;
                m_afRoot[m_iCount++] = -((Real)0.25)*fC3-((Real)0.5)*fE;
            }
        }
    }

    return m_iCount > 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::FindE (Real fC0, Real fC1, Real fC2, Real fC3,
    Real fC4, bool bDoBalancing)
{
    if (Math<Real>::FAbs(fC4) <= m_fEpsilon)
    {
        // polynomial is cubic
        return FindA(fC0,fC1,fC2,fC3);
    }

    // make polynomial monic, x^4+c3*x^3+c2*x^2+c1*x+c0
    Real fInvC4 = ((Real)1.0)/fC4;
    fC0 *= fInvC4;
    fC1 *= fInvC4;
    fC2 *= fInvC4;
    fC3 *= fInvC4;

    // construct the 4-by-4 companion matrix
    GMatrix<Real> kMat(4,4);  // initialized to zero
    kMat[1][0] = (Real)1.0;
    kMat[2][1] = (Real)1.0;
    kMat[3][2] = (Real)1.0;
    kMat[0][3] = -fC0;
    kMat[1][3] = -fC1;
    kMat[2][3] = -fC2;
    kMat[3][3] = -fC3;

    if (bDoBalancing)
    {
        BalanceCompanion4(kMat);
    }

    return QRIteration4(kMat);
}
//----------------------------------------------------------------------------
template <class Real>
Real PolynomialRoots<Real>::GetBound (Real fC0, Real fC1, Real fC2, Real fC3,
    Real fC4)
{
    if (Math<Real>::FAbs(fC4) <= m_fEpsilon)
    {
        // polynomial is cubic
        return GetBound(fC0,fC1,fC2,fC3);
    }

    Real fInvC4 = ((Real)1.0)/fC4;
    Real fMax = Math<Real>::FAbs(fC0)*fInvC4;

    Real fTmp = Math<Real>::FAbs(fC1)*fInvC4;
    if (fTmp > fMax)
    {
        fMax = fTmp;
    }

    fTmp = Math<Real>::FAbs(fC2)*fInvC4;
    if (fTmp > fMax)
    {
        fMax = fTmp;
    }

    fTmp = Math<Real>::FAbs(fC3)*fInvC4;
    if (fTmp > fMax)
    {
        fMax = fTmp;
    }

    return (Real)1.0 + fMax;
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::IsBalancedCompanion4 (Real fA10, Real fA21,
    Real fA32, Real fA03, Real fA13, Real fA23, Real fA33)
{
    const Real fTolerance = (Real)0.001;

    // row/col 0
    Real fRowNorm = fA03;
    Real fColNorm = fA10;
    Real fTest = Math<Real>::FAbs(1.0f - fColNorm/fRowNorm);
    if (fTest > fTolerance)
    {
        return false;
    }

    // row/col 1
    fRowNorm = (fA10 >= fA13 ? fA10 : fA13);
    fColNorm = fA21;
    fTest = Math<Real>::FAbs(1.0f - fColNorm/fRowNorm);
    if (fTest > fTolerance)
    {
        return false;
    }

    // row/col 2
    fRowNorm = (fA21 >= fA23 ? fA21 : fA23);
    fColNorm = fA32;
    fTest = Math<Real>::FAbs(1.0f - fColNorm/fRowNorm);
    if (fTest > fTolerance)
    {
        return false;
    }

    // row/col 3
    fRowNorm = (fA32 >= fA33 ? fA32 : fA33);
    fColNorm = (fA03 >= fA13 ? fA03 : fA13);
    if (fA23 > fColNorm)
    {
        fColNorm = fA23;
    }
    if (fA33 > fColNorm)
    {
        fColNorm = fA33;
    }
    fTest = Math<Real>::FAbs(1.0f - fColNorm/fRowNorm);
    return fTest <= fTolerance;
}
//----------------------------------------------------------------------------
template <class Real>
void PolynomialRoots<Real>::BalanceCompanion4 (GMatrix<Real>& rkMat)
{
    Real fA10 = Math<Real>::FAbs(rkMat[1][0]);
    Real fA21 = Math<Real>::FAbs(rkMat[2][1]);
    Real fA32 = Math<Real>::FAbs(rkMat[3][2]);
    Real fA03 = Math<Real>::FAbs(rkMat[0][3]);
    Real fA13 = Math<Real>::FAbs(rkMat[1][3]);
    Real fA23 = Math<Real>::FAbs(rkMat[2][3]);
    Real fA33 = Math<Real>::FAbs(rkMat[3][3]);
    Real fRowNorm, fColNorm, fScale, fInvScale;

    const int iMax = 16;
    int i;
    for (i = 0; i < iMax; i++)
    {
        // balance row/col 0
        fRowNorm = fA03;
        fColNorm = fA10;
        fScale = Math<Real>::Sqrt(fColNorm/fRowNorm);
        fA03 *= fScale;
        fA10 = fA03;

        // balance row/col 1
        fRowNorm = (fA10 >= fA13 ? fA10 : fA13);
        fColNorm = fA21;
        fScale = Math<Real>::Sqrt(fColNorm/fRowNorm);
        fInvScale = ((Real)1.0)/fScale;
        fA10 *= fScale;
        fA13 *= fScale;
        fA21 *= fInvScale;

        // balance row/col 2
        fRowNorm = (fA21 >= fA23 ? fA21 : fA23);
        fColNorm = fA32;
        fScale = Math<Real>::Sqrt(fColNorm/fRowNorm);
        fInvScale = ((Real)1.0)/fScale;
        fA21 *= fScale;
        fA23 *= fScale;
        fA32 *= fInvScale;

        // balance row/col 3
        fRowNorm = (fA32 >= fA33 ? fA32 : fA33);
        fColNorm = (fA03 >= fA13 ? fA03 : fA13);
        if (fA23 > fColNorm)
        {
            fColNorm = fA23;
        }
        if (fA33 > fColNorm)
        {
            fColNorm = fA33;
        }
        fScale = Math<Real>::Sqrt(fColNorm/fRowNorm);
        fInvScale = ((Real)1.0)/fScale;
        fA32 *= fScale;
        fA03 *= fInvScale;
        fA13 *= fInvScale;
        fA23 *= fInvScale;

        if (IsBalancedCompanion4(fA10,fA21,fA32,fA03,fA13,fA23,fA33))
        {
            break;
        }
    }
    assert(i < iMax);

    rkMat[1][0] = (rkMat[1][0] >= (Real)0.0 ? fA10 : -fA10);
    rkMat[2][1] = (rkMat[2][1] >= (Real)0.0 ? fA21 : -fA21);
    rkMat[3][2] = (rkMat[3][2] >= (Real)0.0 ? fA32 : -fA32);
    rkMat[0][3] = (rkMat[0][3] >= (Real)0.0 ? fA03 : -fA03);
    rkMat[1][3] = (rkMat[1][3] >= (Real)0.0 ? fA13 : -fA13);
    rkMat[2][3] = (rkMat[2][3] >= (Real)0.0 ? fA23 : -fA23);
    rkMat[3][3] = (rkMat[3][3] >= (Real)0.0 ? fA33 : -fA33);
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::QRIteration4 (GMatrix<Real>& rkMat)
{
    GVector<Real> kW(4);
    GMatrix<Real> kMS(3,3);
    Real fRHS, fTrace, fDet, afSaveRoot[2];
    int i, j, iSaveCount;
    for (i = 0; i < m_iMaxIterations; i++)
    {
        fRHS = m_fEpsilon*(Math<Real>::FAbs(rkMat[0][0]) +
            Math<Real>::FAbs(rkMat[1][1]));

        if (Math<Real>::FAbs(rkMat[1][0]) <= fRHS)
        {
            // mat[0][0] is a root, reduce the 3-by-3 submatrix
            // TO DO:  Avoid the copy and pass row/column offsets to the
            // FrancisQR method.
            kMS[0][0] = rkMat[1][1];
            kMS[0][1] = rkMat[1][2];
            kMS[0][2] = rkMat[1][3];
            kMS[1][0] = rkMat[2][1];
            kMS[1][1] = rkMat[2][2];
            kMS[1][2] = rkMat[2][3];
            kMS[2][0] = rkMat[3][1];
            kMS[2][1] = rkMat[3][2];
            kMS[2][2] = rkMat[3][3];
            QRIteration3(kMS);
            m_afRoot[m_iCount++] = rkMat[0][0];
            return true;
        }

        fRHS = m_fEpsilon*(Math<Real>::FAbs(rkMat[1][1]) +
            Math<Real>::FAbs(rkMat[2][2]));

        if (Math<Real>::FAbs(rkMat[2][1]) <= fRHS)
        {
            // The matrix is decoupled into two 2-by-2 blocks.  Solve the
            // quadratics for the blocks.
            fTrace = rkMat[0][0] + rkMat[1][1];
            fDet = rkMat[0][0]*rkMat[1][1] - rkMat[0][1]*rkMat[1][0];
            FindA(fDet,-fTrace,(Real)1.0);
            iSaveCount = m_iCount;
            for (j = 0; j < iSaveCount; j++)
            {
                afSaveRoot[j] = m_afRoot[j];
            }

            fTrace = rkMat[2][2] + rkMat[3][3];
            fDet = rkMat[2][2]*rkMat[3][3] - rkMat[2][3]*rkMat[3][2];
            FindA(fDet,-fTrace,(Real)1.0);
            for (j = 0; j < iSaveCount; j++)
            {
                m_afRoot[m_iCount++] = afSaveRoot[j];
            }
            return m_iCount > 0;
        }

        fRHS = m_fEpsilon*(Math<Real>::FAbs(rkMat[2][2]) +
            Math<Real>::FAbs(rkMat[3][3]));

        if (Math<Real>::FAbs(rkMat[3][2]) <= fRHS)
        {
            // mat[3][3] is a root, reduce the 3-by-3 submatrix
            // TO DO:  Avoid the copy and pass row/column offsets to the
            // FrancisQR method.
            kMS[0][0] = rkMat[0][0];
            kMS[0][1] = rkMat[0][1];
            kMS[0][2] = rkMat[0][2];
            kMS[1][0] = rkMat[1][0];
            kMS[1][1] = rkMat[1][1];
            kMS[1][2] = rkMat[1][2];
            kMS[2][0] = rkMat[2][0];
            kMS[2][1] = rkMat[2][1];
            kMS[2][2] = rkMat[2][2];
            QRIteration3(kMS);
            m_afRoot[m_iCount++] = rkMat[3][3];
            return true;
        }

        FrancisQRStep(rkMat,kW);
    }

    // TO DO:  What to do if the maximum iterations were exceeded?  Maybe a
    // random perturbation to "kick" the system a bit might work?
    //
    // If you want to trap exceeding the maximum iterations, uncomment the
    // 'assert' line of code.
    //
    // assert( false );

    // For now, decouple the matrix using the smallest subdiagonal entry.
    i = 0;
    Real fMin = Math<Real>::FAbs(rkMat[1][0]);
    Real fAbs = Math<Real>::FAbs(rkMat[2][1]);
    if (fAbs < fMin)
    {
        fMin = fAbs;
        i = 1;
    }
    fAbs = Math<Real>::FAbs(rkMat[3][2]);
    if (fAbs < fMin)
    {
        fMin = fAbs;
        i = 2;
    }

    if (i == 0)
    {
        // mat[0][0] is a root, reduce the 3-by-3 submatrix
        // TO DO:  Avoid the copy and pass row/column offsets to the
        // FrancisQR method.
        kMS[0][0] = rkMat[1][1];
        kMS[0][1] = rkMat[1][2];
        kMS[0][2] = rkMat[1][3];
        kMS[1][0] = rkMat[2][1];
        kMS[1][1] = rkMat[2][2];
        kMS[1][2] = rkMat[2][3];
        kMS[2][0] = rkMat[3][1];
        kMS[2][1] = rkMat[3][2];
        kMS[2][2] = rkMat[3][3];
        QRIteration3(kMS);
        m_afRoot[m_iCount++] = rkMat[0][0];
    }
    else if (i == 1)
    {
        // The matrix is decoupled into two 2-by-2 blocks.  Solve the
        // quadratics for the blocks.
        fTrace = rkMat[0][0] + rkMat[1][1];
        fDet = rkMat[0][0]*rkMat[1][1] - rkMat[0][1]*rkMat[1][0];
        FindA(fDet,-fTrace,(Real)1.0);
        iSaveCount = m_iCount;
        for (j = 0; j < iSaveCount; j++)
        {
            afSaveRoot[j] = m_afRoot[j];
        }

        fTrace = rkMat[2][2] + rkMat[3][3];
        fDet = rkMat[2][2]*rkMat[3][3] - rkMat[2][3]*rkMat[3][2];
        FindA(fDet,-fTrace,(Real)1.0);
        for (j = 0; j < iSaveCount; j++)
        {
            m_afRoot[m_iCount++] = afSaveRoot[j];
        }
    }
    else  // i == 2
    {
        // mat[3][3] is a root, reduce the 3-by-3 submatrix
        // TO DO:  Avoid the copy and pass row/column offsets to the
        // FrancisQR method.
        kMS[0][0] = rkMat[0][0];
        kMS[0][1] = rkMat[0][1];
        kMS[0][2] = rkMat[0][2];
        kMS[1][0] = rkMat[1][0];
        kMS[1][1] = rkMat[1][1];
        kMS[1][2] = rkMat[1][2];
        kMS[2][0] = rkMat[2][0];
        kMS[2][1] = rkMat[2][1];
        kMS[2][2] = rkMat[2][2];
        QRIteration3(kMS);
        m_afRoot[m_iCount++] = rkMat[3][3];
    }

    return m_iCount > 0;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// degree N
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::FindB (const Polynomial1<Real>& rkPoly,
    int iDigits)
{
    Real fBound = GetBound(rkPoly);
    return FindB(rkPoly,-fBound,fBound,iDigits);
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::FindN (const Polynomial1<Real>&, int)
{
    // TO DO:  Implement this.
    assert(false);
    return false;
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::FindE (const Polynomial1<Real>&, bool)
{
    // TO DO:  Implement this.
    assert(false);
    return false;
}
//----------------------------------------------------------------------------
template <class Real>
Real PolynomialRoots<Real>::GetBound (const Polynomial1<Real>& rkPoly)
{
    Polynomial1<Real> kCPoly = rkPoly;
    kCPoly.Compress(m_fEpsilon);
    int iDegree = kCPoly.GetDegree();
    if (iDegree < 1)
    {
        // polynomial is constant, return invalid bound
        return -(Real)1.0;
    }

    Real fInvCDeg = ((Real)1.0)/kCPoly[iDegree];
    Real fMax = (Real)0.0;
    for (int i = 0; i < iDegree; i++)
    {
        Real fTmp = Math<Real>::FAbs(kCPoly[i])*fInvCDeg;
        if (fTmp > fMax)
        {
            fMax = fTmp;
        }
    }

    return (Real)1.0 + fMax;
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::FindB (const Polynomial1<Real>& rkPoly,
    Real fXMin, Real fXMax, int iDigits)
{
    // reallocate root array if necessary
    if (rkPoly.GetDegree() > m_iMaxRoot)
    {
        m_iMaxRoot = rkPoly.GetDegree();
        WM4_DELETE[] m_afRoot;
        m_afRoot = WM4_NEW Real[m_iMaxRoot];
    }

    Real fRoot;
    if (rkPoly.GetDegree() == 1)
    {
        if (Bisection(rkPoly,fXMin,fXMax,iDigits,fRoot))
        {
            m_iCount = 1;
            m_afRoot[0] = fRoot;
            return true;
        }
        m_iCount = 0;
        return false;
    }

    // get roots of derivative polynomial
    Polynomial1<Real> kDeriv = rkPoly.GetDerivative();
    FindB(kDeriv,fXMin,fXMax,iDigits);

    int i, iNewCount = 0;
    Real* afNewRoot = WM4_NEW Real[m_iCount+1];

    if ( m_iCount > 0 )
    {
        // find root on [xmin,root[0]]
        if (Bisection(rkPoly,fXMin,m_afRoot[0],iDigits,fRoot))
        {
            afNewRoot[iNewCount++] = fRoot;
        }

        // find root on [root[i],root[i+1]] for 0 <= i <= count-2
        for (i = 0; i <= m_iCount-2; i++)
        {
            if (Bisection(rkPoly,m_afRoot[i],m_afRoot[i+1],iDigits,fRoot))
            {
                afNewRoot[iNewCount++] = fRoot;
            }
        }

        // find root on [root[count-1],xmax]
        if (Bisection(rkPoly,m_afRoot[m_iCount-1],fXMax,iDigits,fRoot))
        {
            afNewRoot[iNewCount++] = fRoot;
        }
    }
    else
    {
        // polynomial is monotone on [xmin,xmax], has at most one root
        if (Bisection(rkPoly,fXMin,fXMax,iDigits,fRoot))
        {
            afNewRoot[iNewCount++] = fRoot;
        }
    }

    // copy to old buffer
    if (iNewCount > 0)
    {
        m_iCount = 1;
        m_afRoot[0] = afNewRoot[0];
        for (i = 1; i < iNewCount; i++)
        {
            Real fRootDiff = afNewRoot[i]-afNewRoot[i-1];
            if (Math<Real>::FAbs(fRootDiff) > m_fEpsilon)
            {
                m_afRoot[m_iCount++] = afNewRoot[i];
            }
        }
    }
    else
    {
        m_iCount = 0;
    }

    WM4_DELETE[] afNewRoot;
    return m_iCount > 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::FindN (const Polynomial1<Real>&, Real, Real, int)
{
    // TO DO:  Implement this.
    assert(false);
    return false;
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::AllRealPartsNegative (
    const Polynomial1<Real>& rkPoly)
{
    // make a copy of coefficients, later calls will change the copy
    int iDegree = rkPoly.GetDegree();
    const Real* afPolyCoeff = (const Real*)rkPoly;
    Real* afCoeff = WM4_NEW Real[iDegree+1];
    size_t uiSize = (iDegree+1)*sizeof(Real);
    System::Memcpy(afCoeff,uiSize,afPolyCoeff,uiSize);

    // make polynomial monic
    if (afCoeff[iDegree] != (Real)1.0)
    {
        Real fInv = ((Real)1.0)/afCoeff[iDegree];
        for (int i = 0; i < iDegree; i++)
        {
            afCoeff[i] *= fInv;
        }
        afCoeff[iDegree] = (Real)1.0;
    }

    return AllRealPartsNegative(iDegree,afCoeff);
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::AllRealPartsPositive (
    const Polynomial1<Real>& rkPoly)
{
    // make a copy of coefficients, later calls will change the copy
    int iDegree = rkPoly.GetDegree();
    const Real* afPolyCoeff = (const Real*)rkPoly;
    Real* afCoeff = WM4_NEW Real[iDegree+1];
    size_t uiSize = (iDegree+1)*sizeof(Real);
    System::Memcpy(afCoeff,uiSize,afPolyCoeff,uiSize);

    // make polynomial monic
    int i;
    if (afCoeff[iDegree] != (Real)1.0)
    {
        Real fInv = ((Real)1.0)/afCoeff[iDegree];
        for (i = 0; i < iDegree; i++)
        {
            afCoeff[i] *= fInv;
        }
        afCoeff[iDegree] = (Real)1.0;
    }

    // reflect z -> -z
    int iSign = -1;
    for (i = iDegree-1; i >= 0; i--, iSign = -iSign)
    {
        afCoeff[i] *= iSign;
    }

    return AllRealPartsNegative(iDegree,afCoeff);
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::AllRealPartsNegative (int iDegree, Real* afCoeff)
{
    // assert:  afCoeff[iDegree] = 1

    if (afCoeff[iDegree-1] <= (Real)0.0)
    {
        return false;
    }
    if (iDegree == 1)
    {
        return true;
    }

    Real* afTmpCoeff = WM4_NEW Real[iDegree];
    afTmpCoeff[0] = ((Real)2.0)*afCoeff[0]*afCoeff[iDegree-1];
    int i;
    for (i = 1; i <= iDegree-2; i++) 
    {
        afTmpCoeff[i] = afCoeff[iDegree-1]*afCoeff[i];
        if (((iDegree-i) % 2) == 0)
        {
            afTmpCoeff[i] -= afCoeff[i-1];
        }
        afTmpCoeff[i] *= (Real)2.0;
    }
    afTmpCoeff[iDegree-1] =
        ((Real)2.0)*afCoeff[iDegree-1]*afCoeff[iDegree-1];

    int iNextDegree;
    for (iNextDegree = iDegree-1; iNextDegree >= 0; iNextDegree--)
    {
        if (afTmpCoeff[iNextDegree] != (Real)0.0)
        {
            break;
        }
    }
    for (i = 0; i <= iNextDegree-1; i++)
    {
        afCoeff[i] = afTmpCoeff[i]/afTmpCoeff[iNextDegree];
    }
    WM4_DELETE[] afTmpCoeff;

    return AllRealPartsNegative(iNextDegree,afCoeff);
}
//----------------------------------------------------------------------------
template <class Real>
int PolynomialRoots<Real>::GetRootCount (const Polynomial1<Real>& rkPoly,
    Real fT0, Real fT1)
{
    int iDegree = rkPoly.GetDegree();
    const Real* afCoeff = (const Real*)rkPoly;

    if (iDegree == 0)
    {
        // polynomial is constant on the interval
        if (afCoeff[0] != (Real)0.0)
        {
            return 0;
        }
        else
        {
            return -1;  // to indicate "infinitely many"
        }
    }

    // generate the Sturm sequence
    std::vector<Polynomial1<Real>*> kSturm;
    Polynomial1<Real>* pkF0 = WM4_NEW Polynomial1<Real>(rkPoly);
    Polynomial1<Real>* pkF1 = WM4_NEW Polynomial1<Real>(
        pkF0->GetDerivative());
    kSturm.push_back(pkF0);
    kSturm.push_back(pkF1);

    while (pkF1->GetDegree() > 0)
    {
        Polynomial1<Real>* pkF2 = WM4_NEW Polynomial1<Real>(-1);
        Polynomial1<Real> kQuot;
        pkF0->Divide(*pkF1,kQuot,*pkF2,Math<Real>::ZERO_TOLERANCE);
        *pkF2 = -(*pkF2);
        kSturm.push_back(pkF2);
        pkF0 = pkF1;
        pkF1 = pkF2;
    }

    int i;
    Real fValue0, fValue1;

    // count the sign changes at t0
    int iSignChanges0 = 0;
    if (fT0 == -Math<Real>::MAX_REAL)
    {
        pkF0 = kSturm[0];
        if (pkF0->GetDegree() & 1)
        {
            fValue0 = -(*pkF0)[pkF0->GetDegree()];
        }
        else
        {
            fValue0 = (*pkF0)[pkF0->GetDegree()];
        }

        if (Math<Real>::FAbs(fValue0) < m_fEpsilon)
        {
            fValue0 = (Real)0.0;
        }

        for (i = 1; i < (int)kSturm.size(); i++)
        {
            pkF1 = kSturm[i];

            if (pkF1->GetDegree() & 1)
            {
                fValue1 = -(*pkF1)[pkF1->GetDegree()];
            }
            else
            {
                fValue1 = (*pkF1)[pkF1->GetDegree()];
            }

            if (Math<Real>::FAbs(fValue1) < m_fEpsilon)
            {
                fValue1 = (Real)0.0;
            }

            if (fValue0*fValue1 < (Real)0.0 || fValue0 == (Real)0.0)
            {
                iSignChanges0++;
            }

            fValue0 = fValue1;
            pkF0 = pkF1;
        }
    }
    else
    {
        pkF0 = kSturm[0];
        fValue0 = (*pkF0)(fT0);
        if (Math<Real>::FAbs(fValue0) < m_fEpsilon)
        {
            fValue0 = (Real)0.0;
        }

        for (i = 1; i < (int)kSturm.size(); i++)
        {
            pkF1 = kSturm[i];
            fValue1 = (*pkF1)(fT0);
            if (Math<Real>::FAbs(fValue1) < m_fEpsilon)
            {
                fValue1 = (Real)0.0;
            }

            if (fValue0*fValue1 < (Real)0.0 || fValue0 == (Real)0.0)
            {
                iSignChanges0++;
            }

            fValue0 = fValue1;
            pkF0 = pkF1;
        }
    }

    // count the sign changes at t1
    int iSignChanges1 = 0;
    if (fT1 == Math<Real>::MAX_REAL)
    {
        pkF0 = kSturm[0];
        fValue0 = (*pkF0)[pkF0->GetDegree()];
        if (Math<Real>::FAbs(fValue0) < m_fEpsilon)
        {
            fValue0 = (Real)0.0;
        }

        for (i = 1; i < (int)kSturm.size(); i++)
        {
            pkF1 = kSturm[i];
            fValue1 = (*pkF1)[pkF1->GetDegree()];
            if (Math<Real>::FAbs(fValue1) < m_fEpsilon)
            {
                fValue1 = (Real)0.0;
            }

            if (fValue0*fValue1 < (Real)0.0 || fValue0 == (Real)0.0)
            {
                iSignChanges1++;
            }

            fValue0 = fValue1;
            pkF0 = pkF1;
        }
    }
    else
    {
        pkF0 = kSturm[0];
        fValue0 = (*pkF0)(fT1);
        if (Math<Real>::FAbs(fValue0) < m_fEpsilon)
        {
            fValue0 = (Real)0.0;
        }

        for (i = 1; i < (int)kSturm.size(); i++)
        {
            pkF1 = kSturm[i];
            fValue1 = (*pkF1)(fT1);
            if (Math<Real>::FAbs(fValue1) < m_fEpsilon)
            {
                fValue1 = (Real)0.0;
            }

            if (fValue0*fValue1 < (Real)0.0 || fValue0 == (Real)0.0 )
            {
                iSignChanges1++;
            }

            fValue0 = fValue1;
            pkF0 = pkF1;
        }
    }

    // clean up
    for (i = 0; i < (int)kSturm.size(); i++)
    {
        WM4_DELETE kSturm[i];
    }

    if (iSignChanges0 >= iSignChanges1)
    {
        return iSignChanges0 - iSignChanges1;
    }

    // theoretically we should not get here
    assert(false);
    return 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool PolynomialRoots<Real>::Bisection (const Polynomial1<Real>& rkPoly,
    Real fXMin, Real fXMax, int iDigits, Real& rfRoot)
{
    Real fP0 = rkPoly(fXMin);
    if (Math<Real>::FAbs(fP0) <= Math<Real>::ZERO_TOLERANCE)
    {
        rfRoot = fXMin;
        return true;
    }
    Real fP1 = rkPoly(fXMax);
    if (Math<Real>::FAbs(fP1) <= Math<Real>::ZERO_TOLERANCE)
    {
        rfRoot = fXMax;
        return true;
    }
    if (fP0*fP1 > (Real)0.0)
        return false;

    // determine number of iterations to get 'digits' accuracy.
    Real fTmp0 = Math<Real>::Log(fXMax-fXMin);
    Real fTmp1 = ((Real)iDigits)*Math<Real>::Log((Real)10.0);
    Real fArg = (fTmp0 + fTmp1)/Math<Real>::Log((Real)2.0);
    int iMaxIter = (int)(fArg + (Real)0.5);

    for (int i = 0; i < iMaxIter; i++)
    {
        rfRoot = ((Real)0.5)*(fXMin + fXMax);
        Real fP = rkPoly(rfRoot);
        Real fProduct = fP*fP0;
        if (fProduct < (Real)0.0)
        {
            fXMax = rfRoot;
            fP1 = fP;
        }
        else if (fProduct > (Real)0.0)
        {
            fXMin = rfRoot;
            fP0 = fP;
        }
        else
        {
            break;
        }
    }

    return true;
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FindE support
//----------------------------------------------------------------------------
template <class Real>
void PolynomialRoots<Real>::GetHouseholderVector (int iSize,
    const Vector3<Real>& rkU, Vector3<Real>& rkV)
{
    // Householder vector V:
    // Given a vector U, compute a vector V such that V[0] = 1 and
    // (I-2*V*V^T/|V|^2)*U is zero in all but the first component.  The
    // matrix P = I-2*V*V^T/|V|^2 is a Householder transformation, a
    // reflection matrix.

    Real fLength = rkU[0]*rkU[0];
    int i;
    for (i = 1; i < iSize; i++)
    {
        fLength += rkU[i]*rkU[i];
    }
    fLength = Math<Real>::Sqrt(fLength);

    if (fLength > m_fEpsilon)
    {
        Real fInv = ((Real)1.0)/(rkU[0]+Math<Real>::Sign(rkU[0])*fLength);
        rkV[0] = (Real)1.0;
        for (i = 1; i < iSize; i++)
        {
            rkV[i] = fInv*rkU[i];
        }
    }
    else
    {
        // U is the zero vector, any vector will do
        rkV[0] = (Real)1.0;
        for (i = 1; i < iSize; i++)
        {
            rkV[i] = (Real)0.0;
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
void PolynomialRoots<Real>::PremultiplyHouseholder (GMatrix<Real>& rkMat,
    GVector<Real>& rkW, int iRMin, int iRMax, int iCMin, int iCMax,
    int iVSize, const Vector3<Real>& rkV)
{
    // Householder premultiplication:
    // Given a matrix A and an m-by-1 vector V with V[0] = 1, let S be the
    // submatrix of A with m rows rmin <= r <= m+rmin-1 and columns
    // cmin <= c <= cmax.  Overwrite S with P*S where P = I-2*V*V^T/|V|^2.

    int iSubRows = iRMax - iRMin + 1, iSubCols = iCMax - iCMin + 1;
    int iRow, iCol;

    Real fSqrLen = rkV[0]*rkV[0];
    for (int i = 1; i < iVSize; i++)
    {
        fSqrLen += rkV[i]*rkV[i];
    }

    Real fBeta = -((Real)2.0)/fSqrLen;
    for (iCol = 0; iCol < iSubCols; iCol++)
    {
        rkW[iCol] = (Real)0.0;
        for (iRow = 0; iRow < iSubRows; iRow++)
        {
            rkW[iCol] += rkV[iRow]*rkMat[iRMin+iRow][iCMin+iCol];
        }
        rkW[iCol] *= fBeta;
    }

    for (iRow = 0; iRow < iSubRows; iRow++)
    {
        for (iCol = 0; iCol < iSubCols; iCol++)
        {
            rkMat[iRMin+iRow][iCMin+iCol] += rkV[iRow]*rkW[iCol];
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
void PolynomialRoots<Real>::PostmultiplyHouseholder (GMatrix<Real>& rkMat,
    GVector<Real>& rkW, int iRMin, int iRMax, int iCMin, int iCMax,
    int iVSize, const Vector3<Real>& rkV)
{
    // Householder postmultiplication:
    // Given a matrix A and an n-by-1 vector V with V[0] = 1, let S be the
    // submatrix of A with n columns cmin <= c <= n+cmin-1 and rows
    // rmin <= r <= rmax.  Overwrite S with S*P where P = I-2*V*V^T/|V|^2.

    int iSubRows = iRMax - iRMin + 1, iSubCols = iCMax - iCMin + 1;
    int iRow, iCol;

    Real fSqrLen = rkV[0]*rkV[0];
    for (int i = 1; i < iVSize; i++)
    {
        fSqrLen += rkV[i]*rkV[i];
    }

    Real fBeta = -((Real)2.0)/fSqrLen;
    for (iRow = 0; iRow < iSubRows; iRow++)
    {
        rkW[iRow] = (Real)0.0;
        for (iCol = 0; iCol < iSubCols; iCol++)
        {
            rkW[iRow] += rkMat[iRMin+iRow][iCMin+iCol]*rkV[iCol];
        }
        rkW[iRow] *= fBeta;
    }

    for (iRow = 0; iRow < iSubRows; iRow++)
    {
        for (iCol = 0; iCol < iSubCols; iCol++)
        {
            rkMat[iRMin+iRow][iCMin+iCol] += rkW[iRow]*rkV[iCol];
        }
    }
}
//----------------------------------------------------------------------------
template <class Real>
void PolynomialRoots<Real>::FrancisQRStep (GMatrix<Real>& rkH,
    GVector<Real>& rkW)
{
    // Given an n-by-n unreduced upper Hessenberg matrix H whose trailing
    // 2-by-2 principal submatrix has eigenvalues a1 and a2, overwrite H
    // with Z^T*H*Z where Z = P(1)*...*P(n-2) is a product of Householder
    // matrices and Z^T*(H-a1*I)*(H-a2*I) is upper triangular.

    // assert:  H is unreduced upper Hessenberg and n >= 3

    // compute first column of (H-a1*I)*(H-a2*I)
    int iN = rkH.GetRows();
    Real fTrace = rkH[iN-2][iN-2] + rkH[iN-1][iN-1];
    Real fDet = rkH[iN-2][iN-2]*rkH[iN-1][iN-1] -
        rkH[iN-2][iN-1]*rkH[iN-1][iN-2];
    Vector3<Real> kU;
    kU[0] = rkH[0][0]*rkH[1][1]+rkH[0][1]*rkH[1][0]-fTrace*rkH[0][0]+fDet;
    kU[1] = rkH[1][0]*(rkH[0][0]+rkH[1][1]-fTrace);
    kU[2] = rkH[1][0]*rkH[2][1];

    // overwrite H with P(0)*H*P(0)^T
    Vector3<Real> kV;
    GetHouseholderVector(3,kU,kV);
    PremultiplyHouseholder(rkH,rkW,0,2,0,iN-1,3,kV);
    PostmultiplyHouseholder(rkH,rkW,0,iN-1,0,2,3,kV);

    for (int i = 1; i <= iN-3; i++)
    {
        // overwrite H with P(i)*H*P(i)^T
        kU[0] = rkH[i  ][i-1];
        kU[1] = rkH[i+1][i-1];
        kU[2] = rkH[i+2][i-1];
        GetHouseholderVector(3,kU,kV);

        // The column range does not need to be 0 to iN-1 because of the
        // pattern of zeros that occur in matrix H.
        PremultiplyHouseholder(rkH,rkW,i,i+2,i-1,iN-1,3,kV);

        // The row range does not need to be 0 to iN-1 because of the pattern
        // of zeros that occur in matrix H.
        int iRMax = i+3;
        if (iRMax >= iN)
        {
            iRMax = iN-1;
        }
        PostmultiplyHouseholder(rkH,rkW,0,iRMax,i,i+2,3,kV);
    }

    // overwrite H with P(n-2)*H*P(n-2)^T
    kU[0] = rkH[iN-2][iN-3];
    kU[1] = rkH[iN-1][iN-3];
    GetHouseholderVector(2,kU,kV);

    // The column range does not need to be 0 to iN-1 because of the pattern
    // of zeros that occur in matrix H.
    PremultiplyHouseholder(rkH,rkW,iN-2,iN-1,iN-3,iN-1,2,kV);

    PostmultiplyHouseholder(rkH,rkW,0,iN-1,iN-2,iN-1,2,kV);
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// explicit instantiation
//----------------------------------------------------------------------------
template WM4_FOUNDATION_ITEM
class PolynomialRoots<float>;

template WM4_FOUNDATION_ITEM
class PolynomialRoots<double>;
//----------------------------------------------------------------------------
}
