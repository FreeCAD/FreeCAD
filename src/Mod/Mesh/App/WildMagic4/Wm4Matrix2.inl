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
// Version: 4.0.1 (2006/08/19)

namespace Wm4
{
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2 (bool bZero)
{
    if (bZero)
    {
        MakeZero();
    }
    else
    {
        MakeIdentity();
    }
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2 (const Matrix2& rkM)
{
    m_afEntry[0] = rkM.m_afEntry[0];
    m_afEntry[1] = rkM.m_afEntry[1];
    m_afEntry[2] = rkM.m_afEntry[2];
    m_afEntry[3] = rkM.m_afEntry[3];
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2 (Real fM00, Real fM01, Real fM10, Real fM11)
{
    m_afEntry[0] = fM00;
    m_afEntry[1] = fM01;
    m_afEntry[2] = fM10;
    m_afEntry[3] = fM11;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2 (const Real afEntry[4], bool bRowMajor)
{
    if (bRowMajor)
    {
        m_afEntry[0] = afEntry[0];
        m_afEntry[1] = afEntry[1];
        m_afEntry[2] = afEntry[2];
        m_afEntry[3] = afEntry[3];
    }
    else
    {
        m_afEntry[0] = afEntry[0];
        m_afEntry[1] = afEntry[2];
        m_afEntry[2] = afEntry[1];
        m_afEntry[3] = afEntry[3];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2 (const Vector2<Real>& rkU, const Vector2<Real>& rkV,
    bool bColumns)
{
    if (bColumns)
    {
        m_afEntry[0] = rkU[0];
        m_afEntry[1] = rkV[0];
        m_afEntry[2] = rkU[1];
        m_afEntry[3] = rkV[1];
    }
    else
    {
        m_afEntry[0] = rkU[0];
        m_afEntry[1] = rkU[1];
        m_afEntry[2] = rkV[0];
        m_afEntry[3] = rkV[1];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2 (const Vector2<Real>* akV, bool bColumns)
{
    if (bColumns)
    {
        m_afEntry[0] = akV[0][0];
        m_afEntry[1] = akV[1][0];
        m_afEntry[2] = akV[0][1];
        m_afEntry[3] = akV[1][1];
    }
    else
    {
        m_afEntry[0] = akV[0][0];
        m_afEntry[1] = akV[0][1];
        m_afEntry[2] = akV[1][0];
        m_afEntry[3] = akV[1][1];
    }
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2 (Real fM00, Real fM11)
{
    MakeDiagonal(fM00,fM11);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2 (Real fAngle)
{
    FromAngle(fAngle);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::Matrix2 (const Vector2<Real>& rkU, const Vector2<Real>& rkV)
{
    MakeTensorProduct(rkU,rkV);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::operator const Real* () const
{
    return m_afEntry;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>::operator Real* ()
{
    return m_afEntry;
}
//----------------------------------------------------------------------------
template <class Real>
const Real* Matrix2<Real>::operator[] (int iRow) const
{
    return &m_afEntry[2*iRow];
}
//----------------------------------------------------------------------------
template <class Real>
Real* Matrix2<Real>::operator[] (int iRow)
{
    return &m_afEntry[2*iRow];
}
//----------------------------------------------------------------------------
template <class Real>
Real Matrix2<Real>::operator() (int iRow, int iCol) const
{
    return m_afEntry[iCol + 2*iRow];
}
//----------------------------------------------------------------------------
template <class Real>
Real& Matrix2<Real>::operator() (int iRow, int iCol)
{
    return m_afEntry[iCol + 2*iRow];
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::MakeZero ()
{
    m_afEntry[0] = (Real)0.0;
    m_afEntry[1] = (Real)0.0;
    m_afEntry[2] = (Real)0.0;
    m_afEntry[3] = (Real)0.0;
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::MakeIdentity ()
{
    m_afEntry[0] = (Real)1.0;
    m_afEntry[1] = (Real)0.0;
    m_afEntry[2] = (Real)0.0;
    m_afEntry[3] = (Real)1.0;
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::MakeDiagonal (Real fM00, Real fM11)
{
    m_afEntry[0] = fM00;
    m_afEntry[1] = (Real)0.0;
    m_afEntry[2] = (Real)0.0;
    m_afEntry[3] = fM11;
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::FromAngle (Real fAngle)
{
    m_afEntry[0] = Math<Real>::Cos(fAngle);
    m_afEntry[2] = Math<Real>::Sin(fAngle);
    m_afEntry[1] = -m_afEntry[2];
    m_afEntry[3] =  m_afEntry[0];
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::MakeTensorProduct (const Vector2<Real>& rkU,
    const Vector2<Real>& rkV)
{
    m_afEntry[0] = rkU[0]*rkV[0];
    m_afEntry[1] = rkU[0]*rkV[1];
    m_afEntry[2] = rkU[1]*rkV[0];
    m_afEntry[3] = rkU[1]*rkV[1];
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::SetRow (int iRow, const Vector2<Real>& rkV)
{
    int i0 = 2*iRow ,i1 = i0+1;
    m_afEntry[i0] = rkV[0];
    m_afEntry[i1] = rkV[1];
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Matrix2<Real>::GetRow (int iRow) const
{
    int i0 = 2*iRow ,i1 = i0+1;
    return Vector2<Real>(m_afEntry[i0],m_afEntry[i1]);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::SetColumn (int iCol, const Vector2<Real>& rkV)
{
    m_afEntry[iCol] = rkV[0];
    m_afEntry[iCol+2] = rkV[1];
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Matrix2<Real>::GetColumn (int iCol) const
{
    return Vector2<Real>(m_afEntry[iCol],m_afEntry[iCol+2]);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::GetColumnMajor (Real* afCMajor) const
{
    afCMajor[0] = m_afEntry[0];
    afCMajor[1] = m_afEntry[2];
    afCMajor[2] = m_afEntry[1];
    afCMajor[3] = m_afEntry[3];
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>& Matrix2<Real>::operator= (const Matrix2& rkM)
{
    m_afEntry[0] = rkM.m_afEntry[0];
    m_afEntry[1] = rkM.m_afEntry[1];
    m_afEntry[2] = rkM.m_afEntry[2];
    m_afEntry[3] = rkM.m_afEntry[3];
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
int Matrix2<Real>::CompareArrays (const Matrix2& rkM) const
{
    return memcmp(m_afEntry,rkM.m_afEntry,4*sizeof(Real));
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix2<Real>::operator== (const Matrix2& rkM) const
{
    return CompareArrays(rkM) == 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix2<Real>::operator!= (const Matrix2& rkM) const
{
    return CompareArrays(rkM) != 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix2<Real>::operator<  (const Matrix2& rkM) const
{
    return CompareArrays(rkM) < 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix2<Real>::operator<= (const Matrix2& rkM) const
{
    return CompareArrays(rkM) <= 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix2<Real>::operator>  (const Matrix2& rkM) const
{
    return CompareArrays(rkM) > 0;
}
//----------------------------------------------------------------------------
template <class Real>
bool Matrix2<Real>::operator>= (const Matrix2& rkM) const
{
    return CompareArrays(rkM) >= 0;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> Matrix2<Real>::operator+ (const Matrix2& rkM) const
{
    return Matrix2<Real>(
        m_afEntry[0] + rkM.m_afEntry[0],
        m_afEntry[1] + rkM.m_afEntry[1],
        m_afEntry[2] + rkM.m_afEntry[2],
        m_afEntry[3] + rkM.m_afEntry[3]);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> Matrix2<Real>::operator- (const Matrix2& rkM) const
{
    return Matrix2<Real>(
        m_afEntry[0] - rkM.m_afEntry[0],
        m_afEntry[1] - rkM.m_afEntry[1],
        m_afEntry[2] - rkM.m_afEntry[2],
        m_afEntry[3] - rkM.m_afEntry[3]);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> Matrix2<Real>::operator* (const Matrix2& rkM) const
{
    return Matrix2<Real>(
        m_afEntry[0]*rkM.m_afEntry[0] + m_afEntry[1]*rkM.m_afEntry[2],
        m_afEntry[0]*rkM.m_afEntry[1] + m_afEntry[1]*rkM.m_afEntry[3],
        m_afEntry[2]*rkM.m_afEntry[0] + m_afEntry[3]*rkM.m_afEntry[2],
        m_afEntry[2]*rkM.m_afEntry[1] + m_afEntry[3]*rkM.m_afEntry[3]);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> Matrix2<Real>::operator* (Real fScalar) const
{
    return Matrix2<Real>(
        fScalar*m_afEntry[0],
        fScalar*m_afEntry[1],
        fScalar*m_afEntry[2],
        fScalar*m_afEntry[3]);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> Matrix2<Real>::operator/ (Real fScalar) const
{
    if (fScalar != (Real)0.0)
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        return Matrix2<Real>(
            fInvScalar*m_afEntry[0],
            fInvScalar*m_afEntry[1],
            fInvScalar*m_afEntry[2],
            fInvScalar*m_afEntry[3]);
    }

    return Matrix2<Real>(
        Math<Real>::MAX_REAL,
        Math<Real>::MAX_REAL,
        Math<Real>::MAX_REAL,
        Math<Real>::MAX_REAL);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> Matrix2<Real>::operator- () const
{
    return Matrix2<Real>(
        -m_afEntry[0],
        -m_afEntry[1],
        -m_afEntry[2],
        -m_afEntry[3]);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>& Matrix2<Real>::operator+= (const Matrix2& rkM)
{
    m_afEntry[0] += rkM.m_afEntry[0];
    m_afEntry[1] += rkM.m_afEntry[1];
    m_afEntry[2] += rkM.m_afEntry[2];
    m_afEntry[3] += rkM.m_afEntry[3];
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>& Matrix2<Real>::operator-= (const Matrix2& rkM)
{
    m_afEntry[0] -= rkM.m_afEntry[0];
    m_afEntry[1] -= rkM.m_afEntry[1];
    m_afEntry[2] -= rkM.m_afEntry[2];
    m_afEntry[3] -= rkM.m_afEntry[3];
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>& Matrix2<Real>::operator*= (Real fScalar)
{
    m_afEntry[0] *= fScalar;
    m_afEntry[1] *= fScalar;
    m_afEntry[2] *= fScalar;
    m_afEntry[3] *= fScalar;
    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real>& Matrix2<Real>::operator/= (Real fScalar)
{
    if (fScalar != (Real)0.0)
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        m_afEntry[0] *= fInvScalar;
        m_afEntry[1] *= fInvScalar;
        m_afEntry[2] *= fInvScalar;
        m_afEntry[3] *= fInvScalar;
    }
    else
    {
        m_afEntry[0] = Math<Real>::MAX_REAL;
        m_afEntry[1] = Math<Real>::MAX_REAL;
        m_afEntry[2] = Math<Real>::MAX_REAL;
        m_afEntry[3] = Math<Real>::MAX_REAL;
    }

    return *this;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> Matrix2<Real>::operator* (const Vector2<Real>& rkV) const
{
    return Vector2<Real>(
        m_afEntry[0]*rkV[0] + m_afEntry[1]*rkV[1],
        m_afEntry[2]*rkV[0] + m_afEntry[3]*rkV[1]);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> Matrix2<Real>::Transpose () const
{
    return Matrix2<Real>(
        m_afEntry[0],
        m_afEntry[2],
        m_afEntry[1],
        m_afEntry[3]);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> Matrix2<Real>::TransposeTimes (const Matrix2& rkM) const
{
    // P = A^T*B
    return Matrix2<Real>(
        m_afEntry[0]*rkM.m_afEntry[0] + m_afEntry[2]*rkM.m_afEntry[2],
        m_afEntry[0]*rkM.m_afEntry[1] + m_afEntry[2]*rkM.m_afEntry[3],
        m_afEntry[1]*rkM.m_afEntry[0] + m_afEntry[3]*rkM.m_afEntry[2],
        m_afEntry[1]*rkM.m_afEntry[1] + m_afEntry[3]*rkM.m_afEntry[3]);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> Matrix2<Real>::TimesTranspose (const Matrix2& rkM) const
{
    // P = A*B^T
    return Matrix2<Real>(
        m_afEntry[0]*rkM.m_afEntry[0] + m_afEntry[1]*rkM.m_afEntry[1],
        m_afEntry[0]*rkM.m_afEntry[2] + m_afEntry[1]*rkM.m_afEntry[3],
        m_afEntry[2]*rkM.m_afEntry[0] + m_afEntry[3]*rkM.m_afEntry[1],
        m_afEntry[2]*rkM.m_afEntry[2] + m_afEntry[3]*rkM.m_afEntry[3]);
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> Matrix2<Real>::Inverse () const
{
    Matrix2<Real> kInverse;

    Real fDet = m_afEntry[0]*m_afEntry[3] - m_afEntry[1]*m_afEntry[2];
    if (Math<Real>::FAbs(fDet) > Math<Real>::ZERO_TOLERANCE)
    {
        Real fInvDet = ((Real)1.0)/fDet;
        kInverse.m_afEntry[0] =  m_afEntry[3]*fInvDet;
        kInverse.m_afEntry[1] = -m_afEntry[1]*fInvDet;
        kInverse.m_afEntry[2] = -m_afEntry[2]*fInvDet;
        kInverse.m_afEntry[3] =  m_afEntry[0]*fInvDet;
    }
    else
    {
        kInverse.m_afEntry[0] = (Real)0.0;
        kInverse.m_afEntry[1] = (Real)0.0;
        kInverse.m_afEntry[2] = (Real)0.0;
        kInverse.m_afEntry[3] = (Real)0.0;
    }

    return kInverse;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> Matrix2<Real>::Adjoint () const
{
    return Matrix2<Real>(
        m_afEntry[3],
        -m_afEntry[1],
        -m_afEntry[2],
        m_afEntry[0]);
}
//----------------------------------------------------------------------------
template <class Real>
Real Matrix2<Real>::Determinant () const
{
    return m_afEntry[0]*m_afEntry[3] - m_afEntry[1]*m_afEntry[2];
}
//----------------------------------------------------------------------------
template <class Real>
Real Matrix2<Real>::QForm (const Vector2<Real>& rkU,
    const Vector2<Real>& rkV) const
{
    return rkU.Dot((*this)*rkV);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::ToAngle (Real& rfAngle) const
{
    // assert:  matrix is a rotation
    rfAngle = Math<Real>::ATan2(m_afEntry[2],m_afEntry[0]);
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::Orthonormalize ()
{
    // Algorithm uses Gram-Schmidt orthogonalization.  If 'this' matrix is
    // M = [m0|m1], then orthonormal output matrix is Q = [q0|q1],
    //
    //   q0 = m0/|m0|
    //   q1 = (m1-(q0*m1)q0)/|m1-(q0*m1)q0|
    //
    // where |V| indicates length of vector V and A*B indicates dot
    // product of vectors A and B.

    // compute q0
    Real fInvLength = Math<Real>::InvSqrt(m_afEntry[0]*m_afEntry[0] +
        m_afEntry[2]*m_afEntry[2]);

    m_afEntry[0] *= fInvLength;
    m_afEntry[2] *= fInvLength;

    // compute q1
    Real fDot0 = m_afEntry[0]*m_afEntry[1] + m_afEntry[2]*m_afEntry[3];
    m_afEntry[1] -= fDot0*m_afEntry[0];
    m_afEntry[3] -= fDot0*m_afEntry[2];

    fInvLength = Math<Real>::InvSqrt(m_afEntry[1]*m_afEntry[1] +
        m_afEntry[3]*m_afEntry[3]);

    m_afEntry[1] *= fInvLength;
    m_afEntry[3] *= fInvLength;
}
//----------------------------------------------------------------------------
template <class Real>
void Matrix2<Real>::EigenDecomposition (Matrix2& rkRot, Matrix2& rkDiag) const
{
    Real fTrace = m_afEntry[0] + m_afEntry[3];
    Real fDiff = m_afEntry[0] - m_afEntry[3];
    Real fDiscr = Math<Real>::Sqrt(fDiff*fDiff +
        ((Real)4.0)*m_afEntry[1]*m_afEntry[1]);
    Real fEVal0 = ((Real)0.5)*(fTrace-fDiscr);
    Real fEVal1 = ((Real)0.5)*(fTrace+fDiscr);
    rkDiag.MakeDiagonal(fEVal0,fEVal1);

    Real fCos, fSin;
    if (fDiff >= (Real)0.0)
    {
        fCos = m_afEntry[1];
        fSin = fEVal0 - m_afEntry[0];
    }
    else
    {
        fCos = fEVal0 - m_afEntry[3];
        fSin = m_afEntry[1];
    }
    Real fTmp = Math<Real>::InvSqrt(fCos*fCos + fSin*fSin);
    fCos *= fTmp;
    fSin *= fTmp;

    rkRot.m_afEntry[0] = fCos;
    rkRot.m_afEntry[1] = -fSin;
    rkRot.m_afEntry[2] = fSin;
    rkRot.m_afEntry[3] = fCos;
}
//----------------------------------------------------------------------------
template <class Real>
Matrix2<Real> operator* (Real fScalar, const Matrix2<Real>& rkM)
{
    return rkM*fScalar;
}
//----------------------------------------------------------------------------
template <class Real>
Vector2<Real> operator* (const Vector2<Real>& rkV, const Matrix2<Real>& rkM)
{
    return Vector2<Real>(
        rkV[0]*rkM[0][0] + rkV[1]*rkM[1][0],
        rkV[0]*rkM[0][1] + rkV[1]*rkM[1][1]);
}
//----------------------------------------------------------------------------
} //namespace Wm4

