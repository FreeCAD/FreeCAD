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

#ifndef WM4MATRIX2_H
#define WM4MATRIX2_H

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

// Rotation matrices are of the form
//   R = cos(t) -sin(t)
//       sin(t)  cos(t)
// where t > 0 indicates a counterclockwise rotation in the xy-plane.

#include "Wm4FoundationLIB.h"
#include "Wm4Vector2.h"

namespace Wm4
{

template <class Real>
class Matrix2
{
public:
    // If bZero is true, create the zero matrix.  Otherwise, create the
    // identity matrix.
    Matrix2 (bool bZero = true);

    // copy constructor
    Matrix2 (const Matrix2& rkM);

    // input Mrc is in row r, column c.
    Matrix2 (Real fM00, Real fM01, Real fM10, Real fM11);

    // Create a matrix from an array of numbers.  The input array is
    // interpreted based on the Boolean input as
    //   true:  entry[0..3] = {m00,m01,m10,m11}  [row major]
    //   false: entry[0..3] = {m00,m10,m01,m11}  [column major]
    Matrix2 (const Real afEntry[4], bool bRowMajor);

    // Create matrices based on vector input.  The Boolean is interpreted as
    //   true: vectors are columns of the matrix
    //   false: vectors are rows of the matrix
    Matrix2 (const Vector2<Real>& rkU, const Vector2<Real>& rkV,
        bool bColumns);
    Matrix2 (const Vector2<Real>* akV, bool bColumns);

    // create a diagonal matrix
    Matrix2 (Real fM00, Real fM11);

    // create a rotation matrix (positive angle - counterclockwise)
    Matrix2 (Real fAngle);

    // create a tensor product U*V^T
    Matrix2 (const Vector2<Real>& rkU, const Vector2<Real>& rkV);

    // create various matrices
    void MakeZero ();
    void MakeIdentity ();
    void MakeDiagonal (Real fM00, Real fM11);
    void FromAngle (Real fAngle);
    void MakeTensorProduct (const Vector2<Real>& rkU,
        const Vector2<Real>& rkV);

    // member access
    operator const Real* () const;
    operator Real* ();
    const Real* operator[] (int iRow) const;
    Real* operator[] (int iRow);
    Real operator() (int iRow, int iCol) const;
    Real& operator() (int iRow, int iCol);
    void SetRow (int iRow, const Vector2<Real>& rkV);
    Vector2<Real> GetRow (int iRow) const;
    void SetColumn (int iCol, const Vector2<Real>& rkV);
    Vector2<Real> GetColumn (int iCol) const;
    void GetColumnMajor (Real* afCMajor) const;

    // assignment
    Matrix2& operator= (const Matrix2& rkM);

    // comparison
    bool operator== (const Matrix2& rkM) const;
    bool operator!= (const Matrix2& rkM) const;
    bool operator<  (const Matrix2& rkM) const;
    bool operator<= (const Matrix2& rkM) const;
    bool operator>  (const Matrix2& rkM) const;
    bool operator>= (const Matrix2& rkM) const;

    // arithmetic operations
    Matrix2 operator+ (const Matrix2& rkM) const;
    Matrix2 operator- (const Matrix2& rkM) const;
    Matrix2 operator* (const Matrix2& rkM) const;
    Matrix2 operator* (Real fScalar) const;
    Matrix2 operator/ (Real fScalar) const;
    Matrix2 operator- () const;

    // arithmetic updates
    Matrix2& operator+= (const Matrix2& rkM);
    Matrix2& operator-= (const Matrix2& rkM);
    Matrix2& operator*= (Real fScalar);
    Matrix2& operator/= (Real fScalar);

    // matrix times vector
    Vector2<Real> operator* (const Vector2<Real>& rkV) const;  // M * v

    // other operations
    Matrix2 Transpose () const;  // M^T
    Matrix2 TransposeTimes (const Matrix2& rkM) const;  // this^T * M
    Matrix2 TimesTranspose (const Matrix2& rkM) const;  // this * M^T
    Matrix2 Inverse () const;
    Matrix2 Adjoint () const;
    Real Determinant () const;
    Real QForm (const Vector2<Real>& rkU,
        const Vector2<Real>& rkV) const;  // u^T*M*v

    // The matrix must be a rotation for these functions to be valid.  The
    // last function uses Gram-Schmidt orthonormalization applied to the
    // columns of the rotation matrix.  The angle must be in radians, not
    // degrees.
    void ToAngle (Real& rfAngle) const;
    void Orthonormalize ();

    // The matrix must be symmetric.  Factor M = R * D * R^T where
    // R = [u0|u1] is a rotation matrix with columns u0 and u1 and
    // D = diag(d0,d1) is a diagonal matrix whose diagonal entries are d0 and
    // d1.  The eigenvector u[i] corresponds to eigenvector d[i].  The
    // eigenvalues are ordered as d0 <= d1.
    void EigenDecomposition (Matrix2& rkRot, Matrix2& rkDiag) const;

    WM4_FOUNDATION_ITEM static const Matrix2 ZERO;
    WM4_FOUNDATION_ITEM static const Matrix2 IDENTITY;

private:
    // support for comparisons
    int CompareArrays (const Matrix2& rkM) const;

    // matrix stored in row-major order
    Real m_afEntry[4];
};

// c * M
template <class Real>
Matrix2<Real> operator* (Real fScalar, const Matrix2<Real>& rkM);

// v^T * M
template <class Real>
Vector2<Real> operator* (const Vector2<Real>& rkV, const Matrix2<Real>& rkM);

} //namespace Wm4

#include "Wm4Matrix2.inl"

namespace Wm4
{
typedef Matrix2<float> Matrix2f;
typedef Matrix2<double> Matrix2d;
}

#endif
