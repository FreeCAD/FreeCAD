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
// Version: 4.0.2 (2006/09/05)

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
// For memory organization it might seem natural to choose Real[N][N] for the
// matrix storage, but this can be a problem on a platform/console that
// chooses to store the data in column-major rather than row-major format.
// To avoid potential portability problems, the matrix is stored as Real[N*N]
// and organized in row-major order.  That is, the entry of the matrix in row
// r (0 <= r < N) and column c (0 <= c < N) is stored at index i = c+N*r
// (0 <= i < N*N).

// The (x,y,z) coordinate system is assumed to be right-handed.  Coordinate
// axis rotation matrices are of the form
//   RX =    1       0       0
//           0     cos(t) -sin(t)
//           0     sin(t)  cos(t)
// where t > 0 indicates a counterclockwise rotation in the yz-plane
//   RY =  cos(t)    0     sin(t)
//           0       1       0
//        -sin(t)    0     cos(t)
// where t > 0 indicates a counterclockwise rotation in the zx-plane
//   RZ =  cos(t) -sin(t)    0
//         sin(t)  cos(t)    0
//           0       0       1
// where t > 0 indicates a counterclockwise rotation in the xy-plane.

#include "Wm4FoundationLIB.h"
#include "Wm4Vector3.h"

namespace Wm4
{

template <class Real>
class Matrix3
{
public:
    // If bZero is true, create the zero matrix.  Otherwise, create the
    // identity matrix.
    Matrix3 (bool bZero = true);

    // copy constructor
    Matrix3 (const Matrix3& rkM);

    // input Mrc is in row r, column c.
    Matrix3 (Real fM00, Real fM01, Real fM02,
             Real fM10, Real fM11, Real fM12,
             Real fM20, Real fM21, Real fM22);

    // Create a matrix from an array of numbers.  The input array is
    // interpreted based on the Boolean input as
    //   true:  entry[0..8]={m00,m01,m02,m10,m11,m12,m20,m21,m22} [row major]
    //   false: entry[0..8]={m00,m10,m20,m01,m11,m21,m02,m12,m22} [col major]
    Matrix3 (const Real afEntry[9], bool bRowMajor);

    // Create matrices based on vector input.  The Boolean is interpreted as
    //   true: vectors are columns of the matrix
    //   false: vectors are rows of the matrix
    Matrix3 (const Vector3<Real>& rkU, const Vector3<Real>& rkV,
        const Vector3<Real>& rkW, bool bColumns);
    Matrix3 (const Vector3<Real>* akV, bool bColumns);

    // create a diagonal matrix
    Matrix3 (Real fM00, Real fM11, Real fM22);

    // Create rotation matrices (positive angle - counterclockwise).  The
    // angle must be in radians, not degrees.
    Matrix3 (const Vector3<Real>& rkAxis, Real fAngle);

    // create a tensor product U*V^T
    Matrix3 (const Vector3<Real>& rkU, const Vector3<Real>& rkV);

    // create various matrices
    Matrix3& MakeZero ();
    Matrix3& MakeIdentity ();
    Matrix3& MakeDiagonal (Real fM00, Real fM11, Real fM22);
    Matrix3& FromAxisAngle (const Vector3<Real>& rkAxis, Real fAngle);
    Matrix3& MakeTensorProduct (const Vector3<Real>& rkU,
        const Vector3<Real>& rkV);

    // member access
    operator const Real* () const;
    operator Real* ();
    const Real* operator[] (int iRow) const;
    Real* operator[] (int iRow);
    Real operator() (int iRow, int iCol) const;
    Real& operator() (int iRow, int iCol);
    void SetRow (int iRow, const Vector3<Real>& rkV);
    Vector3<Real> GetRow (int iRow) const;
    void SetColumn (int iCol, const Vector3<Real>& rkV);
    Vector3<Real> GetColumn (int iCol) const;
    void GetColumnMajor (Real* afCMajor) const;

    // assignment
    Matrix3& operator= (const Matrix3& rkM);

    // comparison
    bool operator== (const Matrix3& rkM) const;
    bool operator!= (const Matrix3& rkM) const;
    bool operator<  (const Matrix3& rkM) const;
    bool operator<= (const Matrix3& rkM) const;
    bool operator>  (const Matrix3& rkM) const;
    bool operator>= (const Matrix3& rkM) const;

    // arithmetic operations
    Matrix3 operator+ (const Matrix3& rkM) const;
    Matrix3 operator- (const Matrix3& rkM) const;
    Matrix3 operator* (const Matrix3& rkM) const;
    Matrix3 operator* (Real fScalar) const;
    Matrix3 operator/ (Real fScalar) const;
    Matrix3 operator- () const;

    // arithmetic updates
    Matrix3& operator+= (const Matrix3& rkM);
    Matrix3& operator-= (const Matrix3& rkM);
    Matrix3& operator*= (Real fScalar);
    Matrix3& operator/= (Real fScalar);

    // matrix times vector
    Vector3<Real> operator* (const Vector3<Real>& rkV) const;  // M * v

    // other operations
    Matrix3 Transpose () const;  // M^T
    Matrix3 TransposeTimes (const Matrix3& rkM) const;  // this^T * M
    Matrix3 TimesTranspose (const Matrix3& rkM) const;  // this * M^T
    Matrix3 Inverse () const;
    Matrix3 Adjoint () const;
    Real Determinant () const;
    Real QForm (const Vector3<Real>& rkU,
        const Vector3<Real>& rkV) const;  // u^T*M*v
    Matrix3 TimesDiagonal (const Vector3<Real>& rkDiag) const;  // M*D
    Matrix3 DiagonalTimes (const Vector3<Real>& rkDiag) const;  // D*M

    // The matrix must be a rotation for these functions to be valid.  The
    // last function uses Gram-Schmidt orthonormalization applied to the
    // columns of the rotation matrix.  The angle must be in radians, not
    // degrees.
    void ToAxisAngle (Vector3<Real>& rkAxis, Real& rfAngle) const;
    void Orthonormalize ();

    // The matrix must be symmetric.  Factor M = R * D * R^T where
    // R = [u0|u1|u2] is a rotation matrix with columns u0, u1, and u2 and
    // D = diag(d0,d1,d2) is a diagonal matrix whose diagonal entries are d0,
    // d1, and d2.  The eigenvector u[i] corresponds to eigenvector d[i].
    // The eigenvalues are ordered as d0 <= d1 <= d2.
    void EigenDecomposition (Matrix3& rkRot, Matrix3& rkDiag) const;

    // Create rotation matrices from Euler angles.
    Matrix3& FromEulerAnglesXYZ (Real fXAngle, Real fYAngle, Real fZAngle);
    Matrix3& FromEulerAnglesXZY (Real fXAngle, Real fZAngle, Real fYAngle);
    Matrix3& FromEulerAnglesYXZ (Real fYAngle, Real fXAngle, Real fZAngle);
    Matrix3& FromEulerAnglesYZX (Real fYAngle, Real fZAngle, Real fXAngle);
    Matrix3& FromEulerAnglesZXY (Real fZAngle, Real fXAngle, Real fYAngle);
    Matrix3& FromEulerAnglesZYX (Real fZAngle, Real fYAngle, Real fXAngle);

    // Extract Euler angles from rotation matrices.  The return value is
    // 'true' iff the factorization is unique relative to certain angle
    // ranges.  That is, if (U,V,W) is some permutation of (X,Y,Z), the angle
    // ranges for the outputs from ToEulerAnglesUVW(uAngle,vAngle,wAngle) are
    // uAngle in [-pi,pi], vAngle in [-pi/2,pi/2], and wAngle in [-pi,pi].  If
    // the function returns 'false', wAngle is 0 and vAngle is either pi/2 or
    // -pi/2.
    bool ToEulerAnglesXYZ (Real& rfXAngle, Real& rfYAngle, Real& rfZAngle)
        const;
    bool ToEulerAnglesXZY (Real& rfXAngle, Real& rfZAngle, Real& rfYAngle)
        const;
    bool ToEulerAnglesYXZ (Real& rfYAngle, Real& rfXAngle, Real& rfZAngle)
        const;
    bool ToEulerAnglesYZX (Real& rfYAngle, Real& rfZAngle, Real& rfXAngle)
        const;
    bool ToEulerAnglesZXY (Real& rfZAngle, Real& rfXAngle, Real& rfYAngle)
        const;
    bool ToEulerAnglesZYX (Real& rfZAngle, Real& rfYAngle, Real& rfXAngle)
        const;

    // SLERP (spherical linear interpolation) without quaternions.  Computes
    // R(t) = R0*(Transpose(R0)*R1)^t.  If Q is a rotation matrix with
    // unit-length axis U and angle A, then Q^t is a rotation matrix with
    // unit-length axis U and rotation angle t*A.
    Matrix3& Slerp (Real fT, const Matrix3& rkR0, const Matrix3& rkR1);

    // Singular value decomposition, M = L*S*R, where L and R are orthogonal
    // and S is a diagonal matrix whose diagonal entries are nonnegative.
    void SingularValueDecomposition (Matrix3& rkL, Matrix3& rkS,
        Matrix3& rkR) const;
    void SingularValueComposition (const Matrix3& rkL, const Matrix3& rkS,
        const Matrix3& rkR);

    // factor M = Q*D*U with orthogonal Q, diagonal D, upper triangular U
    void QDUDecomposition (Matrix3& rkQ, Matrix3& rkD, Matrix3& rkU) const;

    // special matrices
    WM4_FOUNDATION_ITEM static const Matrix3 ZERO;
    WM4_FOUNDATION_ITEM static const Matrix3 IDENTITY;

private:
    // Support for eigendecomposition.  The Tridiagonalize function applies
    // a Householder transformation to the matrix.  If that transformation
    // is the identity (the matrix is already tridiagonal), then the return
    // value is 'false'.  Otherwise, the transformation is a reflection and
    // the return value is 'true'.  The QLAlgorithm returns 'true' iff the
    // QL iteration scheme converged.
    bool Tridiagonalize (Real afDiag[3], Real afSubd[2]);
    bool QLAlgorithm (Real afDiag[3], Real afSubd[2]);

    // support for singular value decomposition
    static void Bidiagonalize (Matrix3& rkA, Matrix3& rkL, Matrix3& rkR);
    static void GolubKahanStep (Matrix3& rkA, Matrix3& rkL, Matrix3& rkR);

    // support for comparisons
    int CompareArrays (const Matrix3& rkM) const;

    Real m_afEntry[9];
};

// c * M
template <class Real>
Matrix3<Real> operator* (Real fScalar, const Matrix3<Real>& rkM);

// v^T * M
template <class Real>
Vector3<Real> operator* (const Vector3<Real>& rkV, const Matrix3<Real>& rkM);

} //namespace Wm4

#include "Wm4Matrix3.inl"

namespace Wm4
{
typedef Matrix3<float> Matrix3f;
typedef Matrix3<double> Matrix3d;
}