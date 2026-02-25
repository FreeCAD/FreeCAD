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
#include "Wm4Matrix2.h"
#include "Wm4Matrix3.h"
#include "Wm4Matrix4.h"
#include "Wm4GMatrix.h"

namespace Wm4
{

template <class Real>
class WM4_FOUNDATION_ITEM Eigen
{
public:
    Eigen (int iSize);
    Eigen (const Matrix2<Real>& rkM);
    Eigen (const Matrix3<Real>& rkM);
    Eigen (const GMatrix<Real>& rkM);
    ~Eigen ();

    // set the matrix for eigensolving
    Real& operator() (int iRow, int iCol);
    Eigen& operator= (const Matrix2<Real>& rkM);
    Eigen& operator= (const Matrix3<Real>& rkM);
    Eigen& operator= (const GMatrix<Real>& rkM);

    // Get the eigenresults (eigenvectors are columns of eigenmatrix).  The
    // GetEigenvector calls involving Vector2 and Vector3 should only be
    // called if you know that the eigenmatrix is of the appropriate size.
    Real GetEigenvalue (int i) const;
    const Real* GetEigenvalues () const;
    void GetEigenvector (int i, Vector2<Real>& rkV) const;
    void GetEigenvector (int i, Vector3<Real>& rkV) const;
    GVector<Real> GetEigenvector (int i) const;
    const GMatrix<Real>& GetEigenvectors () const;

    // solve eigensystem
    void EigenStuff2 ();
    void EigenStuff3 ();
    void EigenStuffN ();
    void EigenStuff  ();

    // solve eigensystem, use decreasing sort on eigenvalues
    void DecrSortEigenStuff2 ();
    void DecrSortEigenStuff3 ();
    void DecrSortEigenStuffN ();
    void DecrSortEigenStuff  ();

    // solve eigensystem, use increasing sort on eigenvalues
    void IncrSortEigenStuff2 ();
    void IncrSortEigenStuff3 ();
    void IncrSortEigenStuffN ();
    void IncrSortEigenStuff  ();

private:
    int m_iSize;
    GMatrix<Real> m_kMat;
    Real* m_afDiag;
    Real* m_afSubd;

    // For odd size matrices, the Householder reduction involves an odd
    // number of reflections.  The product of these is a reflection.  The
    // QL algorithm uses rotations for further reductions.  The final
    // orthogonal matrix whose columns are the eigenvectors is a reflection,
    // so its determinant is -1.  For even size matrices, the Householder
    // reduction involves an even number of reflections whose product is a
    // rotation.  The final orthogonal matrix has determinant +1.  Many
    // algorithms that need an eigendecomposition want a rotation matrix.
    // We want to guarantee this is the case, so m_bRotation keeps track of
    // this.  The DecrSort and IncrSort further complicate the issue since
    // they swap columns of the orthogonal matrix, causing the matrix to
    // toggle between rotation and reflection.  The value m_bRotation must
    // be toggled accordingly.
    bool m_bIsRotation;
    void GuaranteeRotation ();

    // Householder reduction to tridiagonal form
    void Tridiagonal2 ();
    void Tridiagonal3 ();
    void TridiagonalN ();

    // QL algorithm with implicit shifting, applies to tridiagonal matrices
    bool QLAlgorithm ();

    // sort eigenvalues from largest to smallest
    void DecreasingSort ();

    // sort eigenvalues from smallest to largest
    void IncreasingSort ();
};

typedef Eigen<float> Eigenf;
typedef Eigen<double> Eigend;

}