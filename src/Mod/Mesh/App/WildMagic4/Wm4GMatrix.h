// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

// Matrix operations are applied on the left.  For example, given a matrix M
// and a vector V, matrix-times-vector is M*V.  That is, V is treated as a
// column vector.  Some graphics APIs use V*M where V is treated as a row
// vector.  In this context the "M" matrix is really a transpose of the M as
// represented in Wild Magic.  Similarly, to apply two matrix operations M0
// and M1, in that order, you compute M1*M0 so that the transform of a vector
// is (M1*M0)*V = M1*(M0*V).  Some graphics APIs use M0*M1, but again these
// matrices are the transpose of those as represented in Wild Magic.  You
// must therefore be careful about how you interface the transformation code
// with graphics APIS.
//
// Matrices are stored in row-major order, matrix[row][col].

#include "Wm4FoundationLIB.h"
#include "Wm4GVector.h"

namespace Wm4
{

template <class Real>
class GMatrix
{
public:
    // construction and destruction
    GMatrix (int iRows = 0, int iCols = 0);
    GMatrix (int iRows, int iCols, const Real* afData);
    GMatrix (int iRows, int iCols, const Real** aafEntry);
    GMatrix (const GMatrix& rkM);
    ~GMatrix ();

    // member access
    void SetSize (int iRows, int iCols);
    void GetSize (int& riRows, int& riCols) const;
    int GetRows () const;
    int GetColumns () const;
    int GetQuantity () const;
    operator const Real* () const;
    operator Real* ();
    const Real* operator[] (int iRow) const;
    Real* operator[] (int iRow);
    void SwapRows (int iRow0, int iRow1);
    Real operator() (int iRow, int iCol) const;
    Real& operator() (int iRow, int iCol);
    void SetRow (int iRow, const GVector<Real>& rkV);
    GVector<Real> GetRow (int iRow) const;
    void SetColumn (int iCol, const GVector<Real>& rkV);
    GVector<Real> GetColumn (int iCol) const;
    void SetMatrix (int iRows, int iCols, const Real* afEntry);
    void SetMatrix (int iRows, int iCols, const Real** aafMatrix);
    void GetColumnMajor (Real* afCMajor) const;

    // assignment
    GMatrix& operator= (const GMatrix& rkM);

    // comparison
    bool operator== (const GMatrix& rkM) const;
    bool operator!= (const GMatrix& rkM) const;
    bool operator<  (const GMatrix& rkM) const;
    bool operator<= (const GMatrix& rkM) const;
    bool operator>  (const GMatrix& rkM) const;
    bool operator>= (const GMatrix& rkM) const;

    // arithmetic operations
    GMatrix operator+ (const GMatrix& rkM) const;
    GMatrix operator- (const GMatrix& rkM) const;
    GMatrix operator* (const GMatrix& rkM) const;
    GMatrix operator* (Real fScalar) const;
    GMatrix operator/ (Real fScalar) const;
    GMatrix operator- () const;

    // arithmetic updates
    GMatrix& operator+= (const GMatrix& rkM);
    GMatrix& operator-= (const GMatrix& rkM);
    GMatrix& operator*= (Real fScalar);
    GMatrix& operator/= (Real fScalar);

    // matrix products
    GMatrix Transpose () const;  // M^T
    GMatrix TransposeTimes (const GMatrix& rkM) const;  // this^T * M
    GMatrix TimesTranspose (const GMatrix& rkM) const;  // this * M^T

    // matrix-vector operations
    GVector<Real> operator* (const GVector<Real>& rkV) const;  // M * v
    Real QForm (const GVector<Real>& rkU, const GVector<Real>& rkV)
        const;  // u^T*M*v

    // Inversion.  The matrix must be square.  The function returns true
    // whenever the matrix is square and invertible.
    bool GetInverse (GMatrix<Real>& rkInverse) const;

protected:
    // Support for allocation and deallocation.  The allocation call requires
    // m_iRows, m_iCols, and m_iQuantity to have already been correctly
    // initialized.
    void Allocate (bool bSetToZero);
    void Deallocate ();

    // support for comparisons
    int CompareArrays (const GMatrix& rkM) const;

    int m_iRows, m_iCols, m_iQuantity;

    // the matrix is stored in row-major form as a 1-dimensional array
    Real* m_afData;

    // An array of pointers to the rows of the matrix.  The separation of
    // row pointers and actual data supports swapping of rows in linear
    // algebraic algorithms such as solving linear systems of equations.
    Real** m_aafEntry;
};

// c * M
template <class Real>
GMatrix<Real> operator* (Real fScalar, const GMatrix<Real>& rkM);

// v^T * M
template <class Real>
GVector<Real> operator* (const GVector<Real>& rkV, const GMatrix<Real>& rkM);

} //namespace Wm4

#include "Wm4GMatrix.inl"

namespace Wm4
{
typedef GMatrix<float> GMatrixf;
typedef GMatrix<double> GMatrixd;
}