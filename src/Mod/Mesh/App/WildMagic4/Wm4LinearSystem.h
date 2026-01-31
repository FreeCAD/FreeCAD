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

#include "Wm4FoundationLIB.h"
#include "Wm4BandedMatrix.h"
#include "Wm4GMatrix.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM LinearSystem
{
public:
    // construction
    LinearSystem ();

    // 2x2 and 3x3 systems (avoids overhead of Gaussian elimination)
    bool Solve2 (const Real aafA[2][2], const Real afB[2], Real afX[2]);
    bool Solve3 (const Real aafA[3][3], const Real afB[3], Real afX[3]);

    // Input:
    //     A[iSize][iSize], entries are A[row][col]
    // Output:
    //     return value is true if successful, false if pivoting failed
    //     InvA[iSize][iSize], inverse matrix
    bool Inverse (const GMatrix<Real>& rkA, GMatrix<Real>& rkInvA);

    // Input:
    //     A[iSize][iSize] coefficient matrix, entries are A[row][col]
    //     B[iSize] vector, entries are B[row]
    // Output:
    //     return value is true if successful, false if pivoting failed
    //     X[iSize] is solution X to AX = B
    bool Solve (const GMatrix<Real>& rkA, const Real* afB, Real* afX);

    // Input:
    //     Matrix is tridiagonal.
    //     Lower diagonal A[iSize-1]
    //     Main  diagonal B[iSize]
    //     Upper diagonal C[iSize-1]
    //     Right-hand side R[iSize]
    // Output:
    //     return value is true if successful, false if pivoting failed
    //     U[iSize] is solution
    bool SolveTri (int iSize, Real* afA, Real* afB, Real* afC, Real* afR,
        Real* afU);

    // Input:
    //     Matrix is tridiagonal.
    //     Lower diagonal is constant, A
    //     Main  diagonal is constant, B
    //     Upper diagonal is constant, C
    //     Right-hand side Rr[iSize]
    // Output:
    //     return value is true if successful, false if pivoting failed
    //     U[iSize] is solution
    bool SolveConstTri (int iSize, Real fA, Real fB, Real fC, Real* afR,
        Real* afU);

    // Solution using the conjugate gradient method.
    // Input:
    //    A[iSize][iSize] symmetrix matrix, entries are A[row][col]
    //    B[iSize] vector, entries are B[row]
    // Output:
    //    X[iSize] is the solution x to Ax = B
    bool SolveSymmetricCG (const GMatrix<Real>& rkA, const Real* afB,
        Real* afX);

    // Conjugate gradient method for sparse, symmetric matrices.
    // Input:
    //    The nonzero entries of the symmetrix matrix A are stored in a map
    //    whose keys are pairs (i,j) and whose values are real numbers.  The
    //    pair (i,j) is the location of the value in the array.  Only one of
    //    (i,j) and (j,i) should be stored since A is symmetric.  The code
    //    assumes this is how you set up A.  The column vector B is stored as
    //    an array of contiguous values.
    // Output:
    //    X[iSize] is the solution x to Ax = B
    typedef std::map<std::pair<int,int>,Real> SparseMatrix;
    bool SolveSymmetricCG (int iSize, const SparseMatrix& rkA,
        const Real* afB, Real* afX);

    // solve banded matrix systems
    // Input:
    //     A, a banded matrix
    //     B[iSize] vector, entries are B[row]
    // Output:
    //     return value is true if successful, false if pivoting failed
    //     X[iSize] is solution X to AX = B
    bool SolveBanded (const BandedMatrix<Real>& rkA, const Real* afB,
        Real* afX);

    // invert a banded matrix
    // Input:
    //     A, a banded matrix
    // Output:
    //     return value is true if the inverse exists, false otherwise
    //     InvA, the inverse of A
    bool Invert (const BandedMatrix<Real>& rkA, GMatrix<Real>& rkInvA);

    // tolerance for linear system solving
    Real ZeroTolerance;  // default = Math<Real>::ZERO_TOLERANCE

private:
    // support for the conjugate gradient method for standard arrays
    Real Dot (int iSize, const Real* afU, const Real* afV);
    void Multiply (const GMatrix<Real>& rkA, const Real* afX, Real* afProd);
    void UpdateX (int iSize, Real* afX, Real fAlpha, const Real* afP);
    void UpdateR (int iSize, Real* afR, Real fAlpha, const Real* afW);
    void UpdateP (int iSize, Real* afP, Real fBeta, const Real* afR);

    // support for the conjugate gradient method for sparse arrays
    void Multiply (int iSize, const SparseMatrix& rkA, const Real* afX,
        Real* afProd);

    // support for banded matrices
    bool ForwardEliminate (int iReduceRow, BandedMatrix<Real>& rkA,
        Real* afB);
    bool ForwardEliminate (int iReduceRow, BandedMatrix<Real>& rkA,
        GMatrix<Real>& rkB);
    void BackwardEliminate (int iReduceRow, BandedMatrix<Real>& rkA,
        GMatrix<Real>& rkB);
};

typedef LinearSystem<float> LinearSystemf;
typedef LinearSystem<double> LinearSystemd;

}