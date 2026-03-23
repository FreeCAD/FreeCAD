/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

/*!
  \class SbDPMatrix SbDPMatrix.h Inventor/SbDPMatrix.h
  \brief The SbDPMatrix class is a 4x4 dimensional representation of a double-precision matrix.

  \ingroup coin_base

  This class is like the SbMatrix class, but uses double-precision
  floating-point values for its elements. For more class
  documentation, see SbMatrix.

  \since Coin 2.0.
*/

// FIXME: we should _really_ have double-precision classes compatible
// with those in TGS' API, for several good reasons. So either rename
// this, make a typedef (if that is sufficient), or write a "wrapper
// class" around this with inline functions, using with TGS' name for
// it. 20020225 mortene.

// FIXME:
//
//  * The SbDPMatrix::factor() function has not been implemented yet.
//
//  * The element access methods should be inlined.
//
//  * Optimizations are not done yet, so there's a lot of room for
//    improvements.


#include <Inventor/SbDPMatrix.h>

#include <cassert>
#include <cstring>
#include <cfloat>

#include <Inventor/SbDPRotation.h>
#include <Inventor/SbDPLine.h>
#include <Inventor/SbMatrix.h>

#include "coindefs.h" // COIN_STUB()

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::memmove;
using std::memcmp;
using std::memcpy;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

// FIXME: should merge all the PD code we're using from GGIV into
// SbDPMatrix, SbDPRotation and SbVec3d proper (for two reasons: 1)
// there's a lot of duplicated code here (like for instance the
// matrix->quaternion decomposition, which also exists in
// SbDPRotation::setValue(SbDPMatrix&)), and 2) the remaining code
// snippets look generally useful outside the purpose of breaking down
// a matrix into its transformation components). 20010114 mortene.

/*
 * declarations for polar_decomp algorithm from Graphics Gems IV,
 * by Ken Shoemake <shoemake@graphics.cis.upenn.edu>
 */
enum QuatPart {X, Y, Z, W};
typedef double HMatrix[4][4]; /* Right-handed, for column vectors */
typedef struct {
  SbVec4d t;    /* Translation components */
  SbDPRotation  q;        /* Essential rotation     */
  SbDPRotation  u;        /* Stretch rotation       */
  SbVec4d k;    /* Stretch factors        */
  double f;      /* Sign of determinant    */
} AffineParts;
static double polar_decomp(HMatrix M, HMatrix Q, HMatrix S);
static SbVec4d spect_decomp(HMatrix S, HMatrix U);
static SbDPRotation snuggle(SbDPRotation q, SbVec4d & k);
static void decomp_affine(HMatrix A, AffineParts * parts);

static const SbDPMat IDENTITYMATRIX = {
  { 1.0, 0.0, 0.0, 0.0 },
  { 0.0, 1.0, 0.0, 0.0 },
  { 0.0, 0.0, 1.0, 0.0 },
  { 0.0, 0.0, 0.0, 1.0 }
};

static inline
SbBool SbDPMatrix_isIdentity(const double fm[][4])
{
#if 0 // I would assume that the memcmp() version is faster..? Should run some profile checks.
  return ((fm[0][0] == 1.0) && (fm[0][1] == 0.0) && (fm[0][2] == 0.0) && (fm[0][3] == 0.0) &&
          (fm[1][0] == 0.0) && (fm[1][1] == 1.0) && (fm[1][2] == 0.0) && (fm[1][3] == 0.0) &&
          (fm[2][0] == 0.0) && (fm[2][1] == 0.0) && (fm[2][2] == 1.0) && (fm[2][3] == 0.0) &&
          (fm[3][0] == 0.0) && (fm[3][1] == 0.0) && (fm[3][2] == 0.0) && (fm[3][3] == 1.0));
#else
  // Note: as far as I know, memcmp() only compares bytes until
  // there's a mismatch (and does *not* run over the full array and
  // adds up a total, as it sometimes seems from documentation). So
  // this should be very quick for non-identity matrices.
  //
  // Also, we check the first value on its own, to avoid the function
  // call for the most common case.
  return (fm[0][0]==1.0) && memcmp(&fm[0][1], &IDENTITYMATRIX[0][1], (4 * 3 + 3) * sizeof(double)) == 0;
#endif
}

/*!
  The default constructor does nothing. The matrix will be uninitialized.
 */
SbDPMatrix::SbDPMatrix(void)
{
}


/*!
  Constructs a matrix instance with the given initial elements.
 */
SbDPMatrix::SbDPMatrix(const double a11, const double a12,
                   const double a13, const double a14,
                   const double a21, const double a22,
                   const double a23, const double a24,
                   const double a31, const double a32,
                   const double a33, const double a34,
                   const double a41, const double a42,
                   const double a43, const double a44)
{
  const SbDPMat m = { { a11, a12, a13, a14 },
                    { a21, a22, a23, a24 },
                    { a31, a32, a33, a34 },
                    { a41, a42, a43, a44 } };
  this->setValue(m);
}

/*!
  Constructs a matrix instance with the initial elements from the
  \a matrix argument.
 */
SbDPMatrix::SbDPMatrix(const SbDPMat & matrixref)
{
  this->setValue(matrixref);
}

/*!
  This constructor is courtesy of the Microsoft Visual C++ compiler.
*/
SbDPMatrix::SbDPMatrix(const SbDPMat * matrixptr)
{
  this->setValue(*matrixptr);
}

/*!
  This constructor converts a single-precision matrix to a double-precision matrix.
*/
SbDPMatrix::SbDPMatrix(const SbMatrix & matrixref)
{

  const SbDPMat m = { { matrixref[0][0], matrixref[0][1], matrixref[0][2], matrixref[0][3] },
                      { matrixref[1][0], matrixref[1][1], matrixref[1][2], matrixref[1][3] },
                      { matrixref[2][0], matrixref[2][1], matrixref[2][2], matrixref[2][3] },
                      { matrixref[3][0], matrixref[3][1], matrixref[3][2], matrixref[3][3] } };
  this->setValue(m);
}

/*!
  Default destructor does nothing.
 */
SbDPMatrix::~SbDPMatrix(void)
{
}

/*!
  Returns a pointer to the 2 dimensional double array with the matrix
  elements.

  \sa setValue().
 */
const SbDPMat &
SbDPMatrix::getValue(void) const
{
  return this->matrix;
}

/*!
  Copies the elements from \a m into the matrix.

  \sa getValue().
 */
void
SbDPMatrix::setValue(const SbDPMat & m)
{
  (void)memmove(this->matrix, m, sizeof(double)*4*4);
}

/*!
  Copies the elements from \a m into the matrix.

  \sa getValue().
 */
void
SbDPMatrix::setValue(const double * m)
{
  (void)memmove(this->matrix, m, sizeof(double)*4*4);
}

/*!
  Copies the elements from \a m into the matrix.

  \sa getValue().
*/
void
SbDPMatrix::setValue(const SbMatrix & m)
{
  const SbDPMat dmat = { { m[0][0], m[0][1], m[0][2], m[0][3] },
			 { m[1][0], m[1][1], m[1][2], m[1][3] },
			 { m[2][0], m[2][1], m[2][2], m[2][3] },
			 { m[3][0], m[3][1], m[3][2], m[3][3] } };
  this->setValue(dmat);
}


/*!
  Assignment operator. Copies the elements from \a m to the matrix.
 */
SbDPMatrix &
SbDPMatrix::operator=(const SbDPMat & m)
{
  this->setValue(m);
  return *this;
}

/*!
  Assignment operator. Copies the elements from \a m to the matrix.
 */
SbDPMatrix &
SbDPMatrix::operator=(const SbDPMatrix & m)
{
  this->setValue(m.matrix);
  return *this;
}

/*!
  Set the matrix to be the identity matrix.

  \sa identity().
 */
void
SbDPMatrix::makeIdentity(void)
{
  this->matrix[0][0]=this->matrix[1][1]=
    this->matrix[2][2]=this->matrix[3][3] = 1.0;

  this->matrix[0][1]=this->matrix[0][2]=this->matrix[0][3]=
    this->matrix[1][0]=this->matrix[1][2]=this->matrix[1][3]=
    this->matrix[2][0]=this->matrix[2][1]=this->matrix[2][3]=
    this->matrix[3][0]=this->matrix[3][1]=this->matrix[3][2] = 0.0;
}

/*!
  Set matrix to be a rotation matrix with the given rotation.

  \sa setTranslate(), setScale().
*/
void
SbDPMatrix::setRotate(const SbDPRotation & q)
{
  q.getValue(*this);
}

/*!
  Multiply all element values in the matrix with \a v.
 */
void
SbDPMatrix::operator*=(const double v)
{
  for (int i=0; i < 4; i++) {
    for (int j=0; j < 4; j++) {
      this->matrix[i][j] *= v;
    }
  }
}

/*!
  Divide all element values in the matrix on \a v.
 */
void
SbDPMatrix::operator/=(const double v)
{
#if COIN_DEBUG
  if (v == 0.0)
    SoDebugError::postWarning("SbDPMatrix::operator/=",
                              "Division by zero.");
#endif // COIN_DEBUG

  this->operator*=(1.0/v);
}

/*!
  Returns the determinant of the 3x3 submatrix specified by the row and
  column indices.
 */
double
SbDPMatrix::det3(int r1, int r2, int r3,
               int c1, int c2, int c3) const
{
#if COIN_EXTRA_DEBUG
  // Check indices.
  if (r1<0 || r1>3 || r2<0 || r2>3 || r3<0 || r3>3 ||
      c1<0 || c1>3 || c2<0 || c2>3 || c3<0 || c3>3) {
    SoDebugError::post("SbDPMatrix::det3",
                       "At least one idx out of bounds [0, 3]. ");
  }
  if (r1==r2 || r1==r3 || r2==r3 ||
      c1==c2 || c1==c3 || c2==c3)
    SoDebugError::post("SbDPMatrix::det3", "Indices should be distinct.");
#endif // COIN_EXTRA_DEBUG

  // More or less directly from "Advanced Engineering Mathematics"
  // (E. Kreyszig), 6th edition.

  double a11 = this->matrix[r1][c1];
  double a12 = this->matrix[r1][c2];
  double a13 = this->matrix[r1][c3];
  double a21 = this->matrix[r2][c1];
  double a22 = this->matrix[r2][c2];
  double a23 = this->matrix[r2][c3];
  double a31 = this->matrix[r3][c1];
  double a32 = this->matrix[r3][c2];
  double a33 = this->matrix[r3][c3];

  double M11 = a22 * a33 - a32 * a23;
  double M21 = -(a12 * a33 - a32 * a13);
  double M31 = a12 * a23 - a22 * a13;

  return (a11 * M11 + a21 * M21 + a31 * M31);
}

/*!
  Returns the determinant of the upper left 3x3 submatrix.
 */
double
SbDPMatrix::det3(void) const
{
  return this->det3(0, 1, 2, 0, 1, 2);
}

/*!
  Returns the determinant of the matrix.
 */
double
SbDPMatrix::det4(void) const
{
  double det = 0.0;
  det += this->matrix[0][0] * det3(1, 2, 3, 1, 2, 3);
  det -= this->matrix[1][0] * det3(0, 2, 3, 1, 2, 3);
  det += this->matrix[2][0] * det3(0, 1, 3, 1, 2, 3);
  det -= this->matrix[3][0] * det3(0, 1, 2, 1, 2, 3);
  return det;
}

/*!
  Return a new matrix which is the inverse matrix of this.

  The user is responsible for checking that this is a valid operation
  to execute, by first making sure that the result of SbDPMatrix::det4()
  is not equal to zero.
 */
SbDPMatrix
SbDPMatrix::inverse(void) const
{
  // check for identity matrix
  if (SbDPMatrix_isIdentity(this->matrix)) { return SbMatrix::identity(); }

  SbDPMatrix result;

  // use local pointers for speed
  double (*dst)[4];
  dst = reinterpret_cast<double (*)[4]>(result.matrix[0]);
  const double (*src)[4];
  src = reinterpret_cast<const double (*)[4]>(this->matrix[0]);

  // check for affine matrix (common case)
  if (src[0][3] == 0.0 && src[1][3] == 0.0 &&
      src[2][3] == 0.0 && src[3][3] == 1.0) {

    // More or less directly from:
    // Kevin Wu, "Fast Matrix Inversion",  Graphics Gems II
    double det_1;
    double pos, neg, temp;

#define ACCUMULATE     \
    if (temp >= 0.0)  \
      pos += temp;     \
    else               \
      neg += temp

    /*
     * Calculate the determinant of submatrix A and determine if the
     * the matrix is singular as limited by floating-point data
     * representation.
     */
    pos = neg = 0.0;
    temp =  src[0][0] * src[1][1] * src[2][2];
    ACCUMULATE;
    temp =  src[0][1] * src[1][2] * src[2][0];
    ACCUMULATE;
    temp =  src[0][2] * src[1][0] * src[2][1];
    ACCUMULATE;
    temp = -src[0][2] * src[1][1] * src[2][0];
    ACCUMULATE;
    temp = -src[0][1] * src[1][0] * src[2][2];
    ACCUMULATE;
    temp = -src[0][0] * src[1][2] * src[2][1];
    ACCUMULATE;
    det_1 = pos + neg;

#undef ACCUMULATE

    /* Is the submatrix A singular? */
    if ((det_1 == 0.0) || (SbAbs(det_1 / (pos - neg)) < DBL_EPSILON)) {
      /* Matrix M has no inverse */
#if COIN_DEBUG
      SoDebugError::postWarning("SbMatrix::inverse",
                                "Matrix is singular.");
#endif // COIN_DEBUG
      return *this;
    }
    else {
      /* Calculate inverse(A) = adj(A) / det(A) */
      det_1 = 1.0 / det_1;
      dst[0][0] = (src[1][1] * src[2][2] -
                   src[1][2] * src[2][1]) * det_1;
      dst[1][0] = - (src[1][0] * src[2][2] -
                     src[1][2] * src[2][0]) * det_1;
      dst[2][0] = (src[1][0] * src[2][1] -
                   src[1][1] * src[2][0]) * det_1;
      dst[0][1] = - (src[0][1] * src[2][2] -
                     src[0][2] * src[2][1]) * det_1;
      dst[1][1] = (src[0][0] * src[2][2] -
                   src[0][2] * src[2][0]) * det_1;
      dst[2][1] = - (src[0][0] * src[2][1] -
                     src[0][1] * src[2][0]) * det_1;
      dst[0][2] =  (src[0][1] * src[1][2] -
                    src[0][2] * src[1][1]) * det_1;
      dst[1][2] = - (src[0][0] * src[1][2] -
                     src[0][2] * src[1][0]) * det_1;
      dst[2][2] =  (src[0][0] * src[1][1] -
                    src[0][1] * src[1][0]) * det_1;

      /* Calculate -C * inverse(A) */
      dst[3][0] = - (src[3][0] * dst[0][0] +
                     src[3][1] * dst[1][0] +
                     src[3][2] * dst[2][0]);
      dst[3][1] = - (src[3][0] * dst[0][1] +
                     src[3][1] * dst[1][1] +
                     src[3][2] * dst[2][1]);
      dst[3][2] = - (src[3][0] * dst[0][2] +
                     src[3][1] * dst[1][2] +
                     src[3][2] * dst[2][2]);

      /* Fill in last column */
      dst[0][3] = dst[1][3] = dst[2][3] = 0.0;
      dst[3][3] = 1.0;
    }
  }
  else { // non-affine matrix
    double max, sum, tmp, inv_pivot;
    int p[4];
    int i, j, k;

    // algorithm from: Schwarz, "Numerische Mathematik"
    result = *this;

    for (k = 0; k < 4; k++) {
      max = 0.0;
      p[k] = 0;

      for (i = k; i < 4; i++) {
        sum = 0.0;
        for (j = k; j < 4; j++)
          sum += SbAbs(dst[i][j]);
        if (sum > 0.0) {
          tmp = SbAbs(dst[i][k]) / sum;
          if (tmp > max) {
            max = tmp;
            p[k] = i;
          }
        }
      }

      if (max == 0.0) {
#if COIN_DEBUG
        SoDebugError::postWarning("SbMatrix::inverse",
                                  "Matrix is singular.");
#endif // COIN_DEBUG
        return *this;
      }

      if (p[k] != k) {
        for (j = 0; j < 4; j++) {
          tmp = dst[k][j];
          dst[k][j] = dst[p[k]][j];
          dst[p[k]][j] = tmp;
        }
      }

      inv_pivot = 1.0 / dst[k][k];
      for (j = 0; j < 4; j++) {
        if (j != k) {
          dst[k][j] = - dst[k][j] * inv_pivot;
          for (i = 0; i < 4; i++) {
            if (i != k) dst[i][j] += dst[i][k] * dst[k][j];
          }
        }
      }

      for (i = 0; i < 4; i++) dst[i][k] *= inv_pivot;
      dst[k][k] = inv_pivot;
    }

    for (k = 2; k >= 0; k--) {
      if (p[k] != k) {
        for (i = 0; i < 4; i++) {
          tmp = dst[i][k];
          dst[i][k] = dst[i][p[k]];
          dst[i][p[k]] = tmp;
        }
      }
    }
  }
  return result;
}

/*!
  Check if the \a m matrix is equal to this one, within the given tolerance
  value. The tolerance value is applied in the comparison on a component by
  component basis.
 */
SbBool
SbDPMatrix::equals(const SbDPMatrix & m, double tolerance) const
{
#if COIN_DEBUG
  if (tolerance < 0.0)
    SoDebugError::postWarning("SbDPMatrix::equals",
                              "tolerance should be >=0.0.");
#endif // COIN_DEBUG

  for (int i=0; i < 4; i++) {
    for (int j=0;  j< 4; j++) {
      if (fabs(this->matrix[i][j] - m.matrix[i][j]) > tolerance) return FALSE;
    }
  }

  return TRUE;
}


/*!
  Return pointer to the matrix' 4x4 double array.
 */
SbDPMatrix::operator double*(void)
{
  return &(this->matrix[0][0]);
}


/*!
  Return pointer to the matrix' 4x4 double array.
 */
SbDPMatrix::operator SbDPMat&(void)
{
  return this->matrix;
}

/*!
  Returns pointer to the 4 element array representing a matrix row.
  \a i should be within [0, 3].

  \sa getValue(), setValue().
 */
double *
SbDPMatrix::operator [](int i)
{
#if COIN_EXTRA_DEBUG
  if (i<0 || i>3) {
    SoDebugError::post("SbDPMatrix::operator[]", "Index out of bounds. ");
  }
#endif // COIN_EXTRA_DEBUG

   return this->matrix[i];
}

/*!
  Returns pointer to the 4 element array representing a matrix row.
  \a i should be within [0, 3].

  \sa getValue(), setValue().
 */
const double *
SbDPMatrix::operator [](int i) const
{
#if COIN_EXTRA_DEBUG
  if (i<0 || i>3) {
    SoDebugError::postWarning("SbDPMatrix::operator[]", "Index out of bounds. ");
  }
#endif // COIN_EXTRA_DEBUG

   return this->matrix[i];
}

/*!
  Set matrix to be a rotation matrix with the given rotation.

  \sa setRotate().
*/
SbDPMatrix&
SbDPMatrix::operator =(const SbDPRotation & q)
{
  this->setRotate(q);
  return *this;
}

/*!
  Right-multiply with the \a m matrix.

  \sa multRight().
 */
SbDPMatrix&
SbDPMatrix::operator *=(const SbDPMatrix & m)
{
  return this->multRight(m);
}

/*!
  \relates SbDPMatrix

  Multiplies matrix \a m1 with matrix \a m2 and returns the resultant
  matrix.
*/
SbDPMatrix
operator *(const SbDPMatrix & m1, const SbDPMatrix & m2)
{
  SbDPMatrix result = m1;
  result *= m2;
  return result;
}

/*!
  \relates SbDPMatrix

  Compare matrices to see if they are equal. For two matrices to be equal,
  all their individual elements must be equal.

  \sa equals().
*/
int
operator ==(const SbDPMatrix & m1, const SbDPMatrix & m2)
{
  for (int i=0; i < 4; i++) {
    for (int j=0; j < 4; j++) {
      if (m1.matrix[i][j] != m2.matrix[i][j]) return FALSE;
    }
  }

  return TRUE;
}

/*!
  \relates SbDPMatrix

  Compare matrices to see if they are not equal. For two matrices to not be
  equal, it is enough that at least one of their elements are not equal.

  \sa equals().
*/
int
operator !=(const SbDPMatrix & m1, const SbDPMatrix & m2)
{
  return !(m1 == m2);
}

/*!
  Return matrix components in the SbDPMat structure.

  \sa setValue().
 */
void
SbDPMatrix::getValue(SbDPMat & m) const
{
  (void)memmove(&m[0][0], &(this->matrix[0][0]), sizeof(double)*4*4);
}

/*!
  Return the identity matrix.

  \sa makeIdentity().
 */
SbDPMatrix
SbDPMatrix::identity(void)
{
  return SbDPMatrix(&IDENTITYMATRIX);
}

/*!
  Set matrix to be a pure scaling matrix. Scale factors are specified
  by \a s.

  \sa setRotate(), setTranslate().
 */
void
SbDPMatrix::setScale(const double s)
{
  this->makeIdentity();
  this->matrix[0][0] = s;
  this->matrix[1][1] = s;
  this->matrix[2][2] = s;
}

/*!
  Set matrix to be a pure scaling matrix. Scale factors in x, y and z
  is specified by the \a s vector.

  \sa setRotate(), setTranslate().
 */
void
SbDPMatrix::setScale(const SbVec3d & s)
{
  this->makeIdentity();
  this->matrix[0][0] = s[0];
  this->matrix[1][1] = s[1];
  this->matrix[2][2] = s[2];
}

/*!
  Make this matrix into a pure translation matrix (no scale or rotation
  components) with the given vector \a t as the translation.

  \sa setRotate(), setScale().
 */
void
SbDPMatrix::setTranslate(const SbVec3d & t)
{
  this->makeIdentity();
  this->matrix[3][0] = t[0];
  this->matrix[3][1] = t[1];
  this->matrix[3][2] = t[2];
}

/*!
  Set translation, rotation and scaling all at once. The resulting
  matrix gets calculated like this:

  \code
  M = S * R * T
  \endcode

  where \a S, \a R and \a T is scaling, rotation and translation
  matrices.

  \sa setTranslate(), setRotate(), setScale() and getTransform().
 */
void
SbDPMatrix::setTransform(const SbVec3d & t, const SbDPRotation & r, const SbVec3d & s)
{
  SbDPMatrix tmp;

  this->setScale(s);

  tmp.setRotate(r);
  this->multRight(tmp);

  tmp.setTranslate(t);
  this->multRight(tmp);
}

/*!
  Set translation, rotation and scaling all at once with a specified
  scale orientation. The resulting matrix gets calculated like this:

  \code
  M = Ro^-1 * S * Ro * R * T
  \endcode

  where \a Ro is the scale orientation, and \a S, \a R
  and \a T is scaling, rotation and translation.

  \sa setTranslate(), setRotate(), setScale() and getTransform().
 */
void
SbDPMatrix::setTransform(const SbVec3d & t, const SbDPRotation & r,
                       const SbVec3d & s, const SbDPRotation & so)
{
  SbDPMatrix tmp;

  this->setRotate(so.inverse());

  tmp.setScale(s);
  this->multRight(tmp);

  tmp.setRotate(so);
  this->multRight(tmp);

  tmp.setRotate(r);
  this->multRight(tmp);

  tmp.setTranslate(t);
  this->multRight(tmp);
}

/*!
  Set translation, rotation and scaling all at once with a specified
  scale orientation and center point. The resulting matrix gets
  calculated like this:

  \code
  M = -Tc * Ro^-1 * S * Ro * R * T * Tc
  \endcode

  where \a Tc is the center point, \a Ro the scale orientation, \a S,
  \a R and \a T is scaling, rotation and translation.

  \sa setTranslate(), setRotate(), setScale() and getTransform().
 */
void
SbDPMatrix::setTransform(const SbVec3d & translation,
                       const SbDPRotation & rotation,
                       const SbVec3d & scaleFactor,
                       const SbDPRotation & scaleOrientation,
                       const SbVec3d & center)
{
  SbDPMatrix tmp;

  this->setTranslate(-center);

  tmp.setRotate(scaleOrientation.inverse());
  this->multRight(tmp);

  tmp.setScale(scaleFactor);
  this->multRight(tmp);

  tmp.setRotate(scaleOrientation);
  this->multRight(tmp);

  tmp.setRotate(rotation);
  this->multRight(tmp);

  tmp.setTranslate(translation);
  this->multRight(tmp);

  tmp.setTranslate(center);
  this->multRight(tmp);
}

/*!
  Factor the matrix back into its translation, rotation, scale and
  scale orientation components.

  \sa factor()
 */
void
SbDPMatrix::getTransform(SbVec3d & t, SbDPRotation & r, SbVec3d & s,
                       SbDPRotation & so) const
{
  // FIXME: test if this code works with non-affine matrices.
  // pederb, 2000-01-17

  AffineParts parts;
  HMatrix hmatrix;

  // transpose-copy
  hmatrix[0][0] = this->matrix[0][0];
  hmatrix[0][1] = this->matrix[1][0];
  hmatrix[0][2] = this->matrix[2][0];
  hmatrix[0][3] = this->matrix[3][0];

  hmatrix[1][0] = this->matrix[0][1];
  hmatrix[1][1] = this->matrix[1][1];
  hmatrix[1][2] = this->matrix[2][1];
  hmatrix[1][3] = this->matrix[3][1];

  hmatrix[2][0] = this->matrix[0][2];
  hmatrix[2][1] = this->matrix[1][2];
  hmatrix[2][2] = this->matrix[2][2];
  hmatrix[2][3] = this->matrix[3][2];

  hmatrix[3][0] = this->matrix[0][3];
  hmatrix[3][1] = this->matrix[1][3];
  hmatrix[3][2] = this->matrix[2][3];
  hmatrix[3][3] = this->matrix[3][3];

  decomp_affine(hmatrix, &parts);

  double mul = 1.0;
  if (parts.t[W] != 0.0) mul = 1.0 / parts.t[W];
  t[0] = parts.t[X] * mul;
  t[1] = parts.t[Y] * mul;
  t[2] = parts.t[Z] * mul;

  r = parts.q;
  mul = 1.0;
  if (parts.k[W] != 0.0) mul = 1.0 / parts.k[W];
  // mul be sign of determinant to support negative scales.
  mul *= parts.f;
  s[0] = parts.k[X] * mul;
  s[1] = parts.k[Y] * mul;
  s[2] = parts.k[Z] * mul;

  so = parts.u;
}

/*!
  Factor the matrix back into its \a translation, \a rotation,
  \a scaleFactor and \a scaleorientation components. Will eliminate
  the \a center variable from the matrix.

  \sa factor()
 */
void
SbDPMatrix::getTransform(SbVec3d & translation,
                       SbDPRotation & rotation,
                       SbVec3d & scaleFactor,
                       SbDPRotation & scaleOrientation,
                       const SbVec3d & center) const
{
  SbDPMatrix m2 = *this;
  SbDPMatrix trans;
  trans.setTranslate(center);
  m2.multLeft(trans);
  trans.setTranslate(-center);
  m2.multRight(trans);

  m2.getTransform(translation, rotation, scaleFactor, scaleOrientation);
}

/*!
  This function is not implemented in Coin.

  \sa getTransform()
 */
SbBool
SbDPMatrix::factor(SbDPMatrix & COIN_UNUSED_ARG(r), SbVec3d & COIN_UNUSED_ARG(s), SbDPMatrix & COIN_UNUSED_ARG(u), SbVec3d & COIN_UNUSED_ARG(t),
                 SbDPMatrix & COIN_UNUSED_ARG(proj))
{
  // FIXME: not implemented, not documented. 1998MMDD mortene.
  COIN_STUB();
  return FALSE;
}

/*!
  This function produces a permuted LU decomposition of the matrix.  It
  uses the common single-row-pivoting strategy.

  \a FALSE is returned if the matrix is singular, which it never is, because
  of small adjustment values inserted if a singularity is found (as Open
  Inventor does too).

  The parity argument is always set to 1.0 or -1.0.  Don't really know what
  it's for, so it's not checked for correctness.

  The index[] argument returns the permutation that was done on the matrix
  to LU-decompose it.  index[i] is the row that row i was swapped with at
  step i in the decomposition, so index[] is not the actual permutation of
  the row indexes!

  BUGS:  The function does not produce results that are numerically identical
  with those produced by Open Inventor for the same matrices, because the
  pivoting strategy in OI was never fully understood.

  \sa SbDPMatrix::LUBackSubstitution
*/


SbBool
SbDPMatrix::LUDecomposition(int index[4], double & d)
{
    int i;
    for (i = 0; i < 4; i++) index[i] = i;
    d = 1.0;

    const double MINIMUM_PIVOT = 1e-6f; // Inventor fix...

    for (int row = 1; row < 4; row++) {
        int swap_row = row;
        double max_pivot = 0.0;
        for (int test_row = row; test_row < 4; test_row++) {
            const double test_pivot = SbAbs(matrix[test_row][row]);
            if (test_pivot > max_pivot) {
                swap_row = test_row;
                max_pivot = test_pivot;
            }
        }

        // swap rows
        if (swap_row != row) {
            d = -d;
            index[row] = swap_row;
            for (i = 0; i < 4; i++)
                SbSwap(matrix[row][i], matrix[swap_row][i]);
        }

        double pivot = matrix[row][row];
        if (matrix[row][row] == 0.0) {
//            return FALSE;
            // instead of returning FALSE on singulars...
            matrix[row][row] = pivot = MINIMUM_PIVOT;
        }

        // the factorization
        for (i = row + 1; i < 4; i++) {
            const double factor = (matrix[i][row] /= pivot);
            for (int j = row + 1; j < 4; j++)
                matrix[i][j] -= factor * matrix[row][j];
        }
    }
    return TRUE;
}

/*!
  This function does a solve on the "Ax = b" system, given that the matrix
  is LU-decomposed in advance.  First, a forward substitution is done on the
  lower system (Ly = b), and then a backwards substitution is done on the
  upper triangular system (Ux = y).

  The index[] argument is the one returned from
  SbDPMatrix::LUDecomposition(), so see that function for an explanation.

  The b[] argument must contain the b vector in "Ax = b" when calling the
  function.  After the function has solved the system, the b[] vector contains
  the x vector.

  BUGS:  As is done by Open Inventor, unsolvable x values will not return
  NaN but 0.
*/

void
SbDPMatrix::LUBackSubstitution(int index[4], double b[4]) const
{
    int i;

    // permute b[] the way matrix[][] is permuted
    for (i = 0; i < 4; i++)
        if (i != index[i])
            SbSwap(b[i], b[index[i]]);

    // forward substitution on L (Ly = b)
    double y[4];
    for (i = 0; i < 4; i++) {
        double sum = 0.0;
        for (int j = 0; j < i; j++)
            sum += matrix[i][j] * y[j];
        y[i] = b[i] - sum;
    }

    // backwards substitution on U (Ux = y)
    double x[4];
    for (i = 3; i >= 0; i--) {
        double sum = 0.0;
        for (int j = i + 1; j < 4; j++)
             sum += matrix[i][j] * x[j];
        if (matrix[i][i] != 0.0)
            x[i] = (y[i] - sum) / matrix[i][i];
        else
            x[i] = 0.0;
    }

    // de-permute x[]?  doesn't look like it
//    for (i = 3; i >= 0; i--)
//        if (i != index[i])
//            SbSwap(x[i], x[index[i]]);

    // copy x[] into b[] for "return to sender"
    for (i = 0; i < 4; i++) b[i] = x[i];
}

/*!
  Returns the transpose of this matrix.
*/

SbDPMatrix
SbDPMatrix::transpose(void) const
{
  SbDPMatrix trans = (*this);

  for (int i=0; i < 3; i++) {
    for (int j=i+1; j < 4; j++) {
      SbSwap(trans[i][j], trans[j][i]);
    }
  }

  return trans;
}

/*!
  Let this matrix be right-multiplied by \a m. Returns reference to
  self.

  \sa multLeft()
*/
SbDPMatrix &
SbDPMatrix::multRight(const SbDPMatrix & m)
{
  // Checks if one or the other matrix is equal to the identity matrix
  // before multiplying them. We do this because it's a major
  // optimization if one of them _is_, and the isIdentity() check
  // should be very quick in the common case where a matrix is not the
  // identity matrix.
  const SbDPMat & mfm = m.matrix;
  if (SbDPMatrix_isIdentity(mfm)) { return *this; }
  SbDPMat & tfm = this->matrix;
  if (SbDPMatrix_isIdentity(tfm)) { *this = m; return *this; }

  SbDPMat tmp;
  (void)memcpy(tmp, tfm, 4*4*sizeof(double));

  for (int i=0; i < 4; i++) {
    for (int j=0; j < 4; j++) {
      tfm[i][j] =
        tmp[i][0] * mfm[0][j] +
        tmp[i][1] * mfm[1][j] +
        tmp[i][2] * mfm[2][j] +
        tmp[i][3] * mfm[3][j];
    }
  }

  return *this;
}

/*!
  Let this matrix be left-multiplied by \a m. Returns reference to
  self.

  \sa multRight()
*/
SbDPMatrix&
SbDPMatrix::multLeft(const SbDPMatrix & m)
{
  // Checks if one or the other matrix is equal to the identity
  // matrix.  See also code comments at the start of
  // SbDPMatrix::multRight().
  const SbDPMat & mfm = m.matrix;
  if (SbDPMatrix_isIdentity(mfm)) { return *this; }
  SbDPMat & tfm = this->matrix;
  if (SbDPMatrix_isIdentity(tfm)) { *this = m; return *this; }

  SbDPMat tmp;
  (void)memcpy(tmp, tfm, 4*4*sizeof(double));

  for (int i=0; i < 4; i++) {
    for (int j=0; j < 4; j++) {
      tfm[i][j] =
        tmp[0][j] * mfm[i][0] +
        tmp[1][j] * mfm[i][1] +
        tmp[2][j] * mfm[i][2] +
        tmp[3][j] * mfm[i][3];
    }
  }
  return *this;
}

/*!
  Multiply \a src vector with this matrix and return the result in \a dst.
  Multiplication is done with the vector on the right side of the
  expression, i.e. dst = M * src.

  \sa multVecMatrix(), multDirMatrix() and multLineMatrix().
*/
void
SbDPMatrix::multMatrixVec(const SbVec3d & src, SbVec3d & dst) const
{
  // Checks if the "this" matrix is equal to the identity matrix.  See
  // also code comments at the start of SbDPMatrix::multRight().
  if (SbDPMatrix_isIdentity(this->matrix)) { dst = src; return; }

  const double * t0 = (*this)[0];
  const double * t1 = (*this)[1];
  const double * t2 = (*this)[2];
  const double * t3 = (*this)[3];
  // Copy the src vector, just in case src and dst is the same vector.
  SbVec3d s = src;

  double W = s[0]*t3[0] + s[1]*t3[1] + s[2]*t3[2] + t3[3];

  dst[0] = (s[0]*t0[0] + s[1]*t0[1] + s[2]*t0[2] + t0[3])/W;
  dst[1] = (s[0]*t1[0] + s[1]*t1[1] + s[2]*t1[2] + t1[3])/W;
  dst[2] = (s[0]*t2[0] + s[1]*t2[1] + s[2]*t2[2] + t2[3])/W;
}

/*!
  Multiply \a src vector with this matrix and return the result in \a dst.
  Multiplication is done with the vector on the left side of the
  expression, i.e. dst = src * M.

  It is safe to let \a src and \a dst be the same SbVec3d instance.

  \sa multMatrixVec(), multDirMatrix() and multLineMatrix().
*/
void
SbDPMatrix::multVecMatrix(const SbVec3d & src, SbVec3d & dst) const
{
  // Checks if the "this" matrix is equal to the identity matrix.  See
  // also code comments at the start of SbDPMatrix::multRight().
  if (SbDPMatrix_isIdentity(this->matrix)) { dst = src; return; }

  const double * t0 = this->matrix[0];
  const double * t1 = this->matrix[1];
  const double * t2 = this->matrix[2];
  const double * t3 = this->matrix[3];
  // Copy the src vector, just in case src and dst is the same vector.
  SbVec3d s = src;

  double W = s[0]*t0[3] + s[1]*t1[3] + s[2]*t2[3] + t3[3];

  dst[0] = (s[0]*t0[0] + s[1]*t1[0] + s[2]*t2[0] + t3[0])/W;
  dst[1] = (s[0]*t0[1] + s[1]*t1[1] + s[2]*t2[1] + t3[1])/W;
  dst[2] = (s[0]*t0[2] + s[1]*t1[2] + s[2]*t2[2] + t3[2])/W;
}

/*!
  \overload
*/
void
SbDPMatrix::multVecMatrix(const SbVec4d & src, SbVec4d & dst) const
{
  // Checks if the "this" matrix is equal to the identity matrix.  See
  // also code comments at the start of SbDPMatrix::multRight().
  if (SbDPMatrix_isIdentity(this->matrix)) { dst = src; return; }

  const double * t0 = (*this)[0];
  const double * t1 = (*this)[1];
  const double * t2 = (*this)[2];
  const double * t3 = (*this)[3];

  SbVec4d s = src;

  dst[0] = (s[0]*t0[0] + s[1]*t1[0] + s[2]*t2[0] + s[3]*t3[0]);
  dst[1] = (s[0]*t0[1] + s[1]*t1[1] + s[2]*t2[1] + s[3]*t3[1]);
  dst[2] = (s[0]*t0[2] + s[1]*t1[2] + s[2]*t2[2] + s[3]*t3[2]);
  dst[3] = (s[0]*t0[3] + s[1]*t1[3] + s[2]*t2[3] + s[3]*t3[3]);
}

/*!
  Multiplies \a src by the matrix. \a src is assumed to be a direction
  vector, and the translation components of the matrix are therefore
  ignored.

  Multiplication is done with the vector on the left side of the
  expression, i.e. dst = src * M.

  \sa multVecMatrix(), multMatrixVec() and multLineMatrix().
 */
void
SbDPMatrix::multDirMatrix(const SbVec3d & src, SbVec3d & dst) const
{
  // Checks if the "this" matrix is equal to the identity matrix.  See
  // also code comments at the start of SbDPMatrix::multRight().
  if (SbDPMatrix_isIdentity(this->matrix)) { dst = src; return; }

  const double * t0 = (*this)[0];
  const double * t1 = (*this)[1];
  const double * t2 = (*this)[2];
  // Copy the src vector, just in case src and dst is the same vector.
  SbVec3d s = src;

  dst[0] = s[0]*t0[0] + s[1]*t1[0] + s[2]*t2[0];
  dst[1] = s[0]*t0[1] + s[1]*t1[1] + s[2]*t2[1];
  dst[2] = s[0]*t0[2] + s[1]*t1[2] + s[2]*t2[2];
}

/*!
  Multiplies line point with the full matrix and multiplies the
  line direction with the matrix without the translation components.

  \sa multVecMatrix(), multMatrixVec() and multDirMatrix().
 */
void
SbDPMatrix::multLineMatrix(const SbDPLine & src, SbDPLine & dst) const
{
  SbVec3d newpt, newdir;
  this->multVecMatrix(src.getPosition(), newpt);
  this->multDirMatrix(src.getDirection(), newdir);

  dst.setPosDir(newpt, newdir);
}

/*!
  Write out the matrix contents to the given file.
 */
void
SbDPMatrix::print(FILE * fp) const
{
  for (int i=0; i < 4; i++) {
    fprintf(fp, "%10.5g\t%10.5g\t%10.5g\t%10.5g\n",
            this->matrix[i][0], this->matrix[i][1],
            this->matrix[i][2], this->matrix[i][3]);
  }
}

/***********************************************************************
   below is the polar_decomp implementation by Ken Shoemake
   <shoemake@graphics.cis.upenn.edu>. It was part of the
   Graphics Gems IV archive.
************************************************************************/

// FIXME: should merge all the PD code we're using from GGIV into
// SbDPMatrix, SbDPRotation and SbVec3d proper (for two reasons: 1)
// there's a lot of duplicated code here (like for instance the
// matrix->quaternion decomposition, which also exists in
// SbDPRotation::setValue(SbDPMatrix&)), and 2) the remaining code
// snippets look generally useful outside the purpose of breaking down
// a matrix into its transformation components). 20010114 mortene.


/**** Decompose.c ****/
/* Ken Shoemake, 1993 */

/******* Matrix Preliminaries *******/

/** Fill out 3x3 matrix to 4x4 **/
#define mat_pad(A) (A[W][X]=A[X][W]=A[W][Y]=A[Y][W]=A[W][Z]=A[Z][W]=0, A[W][W]=1)

/** Copy nxn matrix A to C using "gets" for assignment **/
#define mat_copy(C, gets, A, n) {int i, j; for (i=0;i<n;i++) for (j=0;j<n;j++)\
    C[i][j] gets (A[i][j]);}

/** Copy transpose of nxn matrix A to C using "gets" for assignment **/
#define mat_tpose(AT, gets, A, n) {int i, j; for (i=0;i<n;i++) for (j=0;j<n;j++)\
    AT[i][j] gets (A[j][i]);}

/** Assign nxn matrix C the element-wise combination of A and B using "op" **/
#define mat_binop(C, gets, A, op, B, n) {int i, j; for (i=0;i<n;i++) for (j=0;j<n;j++)\
    C[i][j] gets (A[i][j]) op (B[i][j]);}

/** Multiply the upper left 3x3 parts of A and B to get AB **/
static void
mat_mult(HMatrix A, HMatrix B, HMatrix AB)
{
  int i, j;
  for (i=0; i<3; i++) for (j=0; j<3; j++)
    AB[i][j] = A[i][0]*B[0][j] + A[i][1]*B[1][j] + A[i][2]*B[2][j];
}

/** Return dot product of length 3 vectors va and vb **/
static double
vdot(double * va, double * vb)
{
  return (va[0]*vb[0] + va[1]*vb[1] + va[2]*vb[2]);
}

/** Set v to cross product of length 3 vectors va and vb **/
static void
vcross(double * va, double * vb, double * v)
{
  v[0] = va[1]*vb[2] - va[2]*vb[1];
  v[1] = va[2]*vb[0] - va[0]*vb[2];
  v[2] = va[0]*vb[1] - va[1]*vb[0];
}

/** Set MadjT to transpose of inverse of M times determinant of M **/
static void
adjoint_transpose(HMatrix M, HMatrix MadjT)
{
  vcross(M[1], M[2], MadjT[0]);
  vcross(M[2], M[0], MadjT[1]);
  vcross(M[0], M[1], MadjT[2]);
}

/******* Decomp Auxiliaries *******/

static HMatrix mat_id = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};

/** Compute either the 1 or infinity norm of M, depending on tpose **/
static double
mat_norm(HMatrix M, int tpose)
{
  int i;
  double sum, max;
  max = 0.0;
  for (i=0; i<3; i++) {
    if (tpose) sum = static_cast<double>(fabs(M[0][i])+fabs(M[1][i])+fabs(M[2][i]));
    else sum = static_cast<double>(fabs(M[i][0])+fabs(M[i][1])+fabs(M[i][2]));
    if (max<sum) max = sum;
  }
  return max;
}

static double norm_inf(HMatrix M) {return mat_norm(M, 0);}
static double norm_one(HMatrix M) {return mat_norm(M, 1);}

/** Return index of column of M containing maximum abs entry, or -1 if M=0 **/
static int
find_max_col(HMatrix M)
{
  double abs, max;
  int i, j, col;
  max = 0.0; col = -1;
  for (i=0; i<3; i++) for (j=0; j<3; j++) {
    abs = M[i][j]; if (abs<0.0) abs = -abs;
    if (abs>max) {max = abs; col = j;}
  }
    return col;
}

/** Setup u for Household reflection to zero all v components but first **/
static void
make_reflector(double * v, double * u)
{
  double s = static_cast<double>(sqrt(vdot(v, v)));
  u[0] = v[0]; u[1] = v[1];
  u[2] = v[2] + ((v[2]<0.0) ? -s : s);
  s = static_cast<double>(sqrt(2.0/vdot(u, u)));
  u[0] = u[0]*s; u[1] = u[1]*s; u[2] = u[2]*s;
}

/** Apply Householder reflection represented by u to column vectors of M **/
static void
reflect_cols(HMatrix M, double * u)
{
  int i, j;
  for (i=0; i<3; i++) {
    double s = u[0]*M[0][i] + u[1]*M[1][i] + u[2]*M[2][i];
    for (j=0; j<3; j++) M[j][i] -= u[j]*s;
  }
}
/** Apply Householder reflection represented by u to row vectors of M **/
static void
reflect_rows(HMatrix M, double * u)
{
  int i, j;
  for (i=0; i<3; i++) {
    double s = vdot(u, M[i]);
    for (j=0; j<3; j++) M[i][j] -= u[j]*s;
  }
}

/** Find orthogonal factor Q of rank 1 (or less) M **/
static void
do_rank1(HMatrix M, HMatrix Q)
{
  double v1[3], v2[3], s;
  int col;
  mat_copy(Q, =, mat_id, 4);
  /* If rank(M) is 1, we should find a non-zero column in M */
  col = find_max_col(M);
  if (col<0) return; /* Rank is 0 */
  v1[0] = M[0][col]; v1[1] = M[1][col]; v1[2] = M[2][col];
  make_reflector(v1, v1); reflect_cols(M, v1);
  v2[0] = M[2][0]; v2[1] = M[2][1]; v2[2] = M[2][2];
  make_reflector(v2, v2); reflect_rows(M, v2);
  s = M[2][2];
  if (s<0.0) Q[2][2] = -1.0;
  reflect_cols(Q, v1); reflect_rows(Q, v2);
}

/** Find orthogonal factor Q of rank 2 (or less) M using adjoint transpose **/
static void
do_rank2(HMatrix M, HMatrix MadjT, HMatrix Q)
{
  double v1[3], v2[3];
  double w, x, y, z, c, s, d;
  int col;
  /* If rank(M) is 2, we should find a non-zero column in MadjT */
  col = find_max_col(MadjT);
  if (col<0) {do_rank1(M, Q); return;} /* Rank<2 */
  v1[0] = MadjT[0][col]; v1[1] = MadjT[1][col]; v1[2] = MadjT[2][col];
  make_reflector(v1, v1); reflect_cols(M, v1);
  vcross(M[0], M[1], v2);
  make_reflector(v2, v2); reflect_rows(M, v2);
  w = M[0][0]; x = M[0][1]; y = M[1][0]; z = M[1][1];
  if (w*z>x*y) {
    c = z+w; s = y-x; d = static_cast<double>(sqrt(c*c+s*s)); c = c/d; s = s/d;
    Q[0][0] = Q[1][1] = c; Q[0][1] = -(Q[1][0] = s);
  }
  else {
    c = z-w; s = y+x; d = static_cast<double>(sqrt(c*c+s*s)); c = c/d; s = s/d;
    Q[0][0] = -(Q[1][1] = c); Q[0][1] = Q[1][0] = s;
  }
  Q[0][2] = Q[2][0] = Q[1][2] = Q[2][1] = 0.0; Q[2][2] = 1.0;
  reflect_cols(Q, v1); reflect_rows(Q, v2);
}


/******* Polar Decomposition *******/

/* Polar Decomposition of 3x3 matrix in 4x4,
 * M = QS.  See Nicholas Higham and Robert S. Schreiber,
 * Fast Polar Decomposition of An Arbitrary Matrix,
 * Technical Report 88-942, October 1988,
 * Department of Computer Science, Cornell University.
 */
static double
polar_decomp(HMatrix M, HMatrix Q, HMatrix S)
{
#define TOL 1.0e-6
  HMatrix Mk, MadjTk, Ek;
  double det, M_one, M_inf, MadjT_one, MadjT_inf, E_one, gamma, g1, g2;
  int i, j;
  mat_tpose(Mk, =, M, 3);
  M_one = norm_one(Mk);  M_inf = norm_inf(Mk);
  do {
    adjoint_transpose(Mk, MadjTk);
    det = vdot(Mk[0], MadjTk[0]);
    if (det==0.0) {do_rank2(Mk, MadjTk, Mk); break;}
    MadjT_one = norm_one(MadjTk); MadjT_inf = norm_inf(MadjTk);
    gamma = static_cast<double>(sqrt(sqrt((MadjT_one*MadjT_inf)/(M_one*M_inf))/fabs(det)));
    g1 = gamma*0.5;
    g2 = 0.5/(gamma*det);
    mat_copy(Ek, =, Mk, 3);
    mat_binop(Mk, =, g1*Mk, +, g2*MadjTk, 3);
    mat_copy(Ek, -=, Mk, 3);
    E_one = norm_one(Ek);
    M_one = norm_one(Mk);  M_inf = norm_inf(Mk);
  } while (E_one>(M_one*TOL));
  mat_tpose(Q, =, Mk, 3); mat_pad(Q);
  mat_mult(Mk, M, S);    mat_pad(S);
  for (i=0; i<3; i++) for (j=i; j<3; j++)
    S[i][j] = S[j][i] = 0.5*(S[i][j]+S[j][i]);
  return (det);
}

/******* Spectral Decomposition *******/

/* Compute the spectral decomposition of symmetric positive semi-definite S.
 * Returns rotation in U and scale factors in result, so that if K is a diagonal
 * matrix of the scale factors, then S = U K (U transpose). Uses Jacobi method.
 * See Gene H. Golub and Charles F. Van Loan. Matrix Computations. Hopkins 1983.
 */
static SbVec4d
spect_decomp(HMatrix S, HMatrix U)
{
  SbVec4d kv;
  double Diag[3], OffD[3]; /* OffD is off-diag (by omitted index) */
  double g, h, fabsh, fabsOffDi, t, theta, c, s, tau, ta, OffDq, a, b;
  static char nxt[] = {Y, Z, X};
  int sweep, i, j;
  mat_copy(U, =, mat_id, 4);
  Diag[X] = S[X][X]; Diag[Y] = S[Y][Y]; Diag[Z] = S[Z][Z];
  OffD[X] = S[Y][Z]; OffD[Y] = S[Z][X]; OffD[Z] = S[X][Y];
  for (sweep=20; sweep>0; sweep--) {
    double sm = static_cast<double>(fabs(OffD[X])+fabs(OffD[Y])+fabs(OffD[Z]));
    if (sm==0.0) break;
    for (i=Z; i>=X; i--) {
      int p = nxt[i]; int q = nxt[p];
      fabsOffDi = fabs(OffD[i]);
      g = 100.0*fabsOffDi;
      if (fabsOffDi>0.0) {
        h = Diag[q] - Diag[p];
        fabsh = fabs(h);
        if (fabsh+g==fabsh) {
          t = OffD[i]/h;
        }
        else {
          theta = 0.5*h/OffD[i];
          t = 1.0/(fabs(theta)+sqrt(theta*theta+1.0));
          if (theta<0.0) t = -t;
        }
        c = 1.0/sqrt(t*t+1.0); s = t*c;
        tau = s/(c+1.0);
        ta = t*OffD[i]; OffD[i] = 0.0;
        Diag[p] -= ta; Diag[q] += ta;
        OffDq = OffD[q];
        OffD[q] -= s*(OffD[p] + tau*OffD[q]);
        OffD[p] += s*(OffDq   - tau*OffD[p]);
        for (j=Z; j>=X; j--) {
          a = U[j][p]; b = U[j][q];
          U[j][p] -= static_cast<double>(s*(b + tau*a));
          U[j][q] += static_cast<double>(s*(a - tau*b));
        }
      }
    }
  }
  kv[X] = static_cast<double>(Diag[X]);
  kv[Y] = static_cast<double>(Diag[Y]);
  kv[Z] = static_cast<double>(Diag[Z]);
  kv[W] = 1.0;
  return (kv);
}

/* Helper function for the snuggle() function below. */
static inline void
cycle(double * a, SbBool flip)
{
  if (flip) {
    a[3]=a[0]; a[0]=a[1]; a[1]=a[2]; a[2]=a[3];
  }
  else {
    a[3]=a[2]; a[2]=a[1]; a[1]=a[0]; a[0]=a[3];
  }
}

/******* Spectral Axis Adjustment *******/

/* Given a unit quaternion, q, and a scale vector, k, find a unit quaternion, p,
 * which permutes the axes and turns freely in the plane of duplicate scale
 * factors, such that q p has the largest possible w component, i.e. the
 * smallest possible angle. Permutes k's components to go with q p instead of q.
 * See Ken Shoemake and Tom Duff. Matrix Animation and Polar Decomposition.
 * Proceedings of Graphics Interface 1992. Details on p. 262-263.
 */
static SbDPRotation
snuggle(SbDPRotation q, SbVec4d & k)
{
#define SQRTHALF (0.7071067811865475244f)
#define sgn(n, v)    ((n)?-(v):(v))
#define swap(a, i, j) {a[3]=a[i]; a[i]=a[j]; a[j]=a[3];}

  SbDPRotation p;
  double ka[4];
  int i, turn = -1;
  ka[X] = k[X]; ka[Y] = k[Y]; ka[Z] = k[Z];
  if (ka[X]==ka[Y]) {if (ka[X]==ka[Z]) turn = W; else turn = Z;}
  else {if (ka[X]==ka[Z]) turn = Y; else if (ka[Y]==ka[Z]) turn = X;}
  if (turn>=0) {
    SbDPRotation qtoz, qp;
    unsigned neg[3], win;
    double mag[3], t;
    static SbDPRotation qxtoz(0.0, SQRTHALF, 0.0, SQRTHALF);
    static SbDPRotation qytoz(SQRTHALF, 0.0, 0.0, SQRTHALF);
    static SbDPRotation qppmm(0.5, 0.5, -0.5, -0.5);
    static SbDPRotation qpppp(0.5, 0.5, 0.5, 0.5);
    static SbDPRotation qmpmm(-0.5, 0.5, -0.5, -0.5);
    static SbDPRotation qpppm(0.5, 0.5, 0.5, -0.5);
    static SbDPRotation q0001(0.0, 0.0, 0.0, 1.0);
    static SbDPRotation q1000(1.0, 0.0, 0.0, 0.0);
    switch (turn) {
    default: return SbDPRotation(q).invert();
    case X: q = (qtoz = qxtoz) * q; swap(ka, X, Z) break;
    case Y: q = (qtoz = qytoz) * q; swap(ka, Y, Z) break;
    case Z: qtoz = q0001; break;
    }
    q.invert();
    mag[0] = static_cast<double>(q.getValue()[Z]*q.getValue()[Z])+static_cast<double>(q.getValue()[W]*q.getValue()[W]-0.5);
    mag[1] = static_cast<double>(q.getValue()[X]*q.getValue()[Z])-static_cast<double>(q.getValue()[Y]*q.getValue()[W]);
    mag[2] = static_cast<double>(q.getValue()[Y]*q.getValue()[Z]+static_cast<double>(q.getValue()[X]*q.getValue()[W]));
    for (i=0; i<3; i++) if ((neg[i] = (mag[i] < 0.0))) mag[i] = -mag[i];
    if (mag[0]>mag[1]) {if (mag[0]>mag[2]) win = 0; else win = 2;}
    else {if (mag[1]>mag[2]) win = 1; else win = 2;}
    switch (win) {
    case 0: if (neg[0]) p = q1000; else p = q0001; break;
    case 1: if (neg[1]) p = qppmm; else p = qpppp; cycle(ka, FALSE); break;
    case 2: if (neg[2]) p = qmpmm; else p = qpppm; cycle(ka, TRUE); break;
    }
    qp = p * q;
    t = sqrt(mag[win]+0.5);
    p = SbDPRotation(0.0, 0.0, -qp.getValue()[Z]/static_cast<double>(t), qp.getValue()[W]/static_cast<double>(t)) * p;
    p = SbDPRotation(p).invert() * qtoz;
  }
  else {
    double qa[4], pa[4];
    unsigned lo, hi, neg[4], par = 0;
    double all, big, two;
    qa[0] = q.getValue()[X]; qa[1] = q.getValue()[Y]; qa[2] = q.getValue()[Z]; qa[3] = q.getValue()[W];
    for (i=0; i<4; i++) {
      pa[i] = 0.0;
      if ((neg[i] = (qa[i]<0.0))) qa[i] = -qa[i];
      par ^= neg[i];
    }
    /* Find two largest components, indices in hi and lo */
    if (qa[0]>qa[1]) lo = 0; else lo = 1;
    if (qa[2]>qa[3]) hi = 2; else hi = 3;
    if (qa[lo]>qa[hi]) {
      if (qa[lo^1]>qa[hi]) {hi = lo; lo ^= 1;}
      else {hi ^= lo; lo ^= hi; hi ^= lo;}
    } else {if (qa[hi^1]>qa[lo]) lo = hi^1;}
    all = (qa[0]+qa[1]+qa[2]+qa[3])*0.5;
    two = (qa[hi]+qa[lo])*SQRTHALF;
    big = qa[hi];
    if (all>two) {
      if (all>big) {/*all*/
        {int i; for (i=0; i<4; i++) pa[i] = sgn(neg[i], 0.5);}
        cycle(ka, par);
      }
      else {/*big*/ pa[hi] = sgn(neg[hi], 1.0);}
    }
    else {
      if (two>big) {/*two*/
        pa[hi] = sgn(neg[hi], SQRTHALF); pa[lo] = sgn(neg[lo], SQRTHALF);
        if (lo>hi) {hi ^= lo; lo ^= hi; hi ^= lo;}
        if (hi==W) {hi = "\001\002\000"[lo]; lo = 3-hi-lo;}
        swap(ka, hi, lo)
          } else {/*big*/ pa[hi] = sgn(neg[hi], 1.0);}
    }
    // FIXME: p = conjugate(pa)? 20010114 mortene.
    p.setValue(-pa[0], -pa[1], -pa[2], pa[3]);
  }
  k[X] = ka[X]; k[Y] = ka[Y]; k[Z] = ka[Z];
  return (p);
}

/******* Decompose Affine Matrix *******/

/* Decompose 4x4 affine matrix A as TFRUK(U transpose), where t contains the
 * translation components, q contains the rotation R, u contains U, k contains
 * scale factors, and f contains the sign of the determinant.
 * Assumes A transforms column vectors in right-handed coordinates.
 * See Ken Shoemake and Tom Duff. Matrix Animation and Polar Decomposition.
 * Proceedings of Graphics Interface 1992.
 */
static void
decomp_affine(HMatrix A, AffineParts * parts)
{
  HMatrix Q, S, U;
  SbDPRotation p;
  parts->t = SbVec4d(A[X][W], A[Y][W], A[Z][W], 0);
  double det = polar_decomp(A, Q, S);
  if (det<0.0) {
    mat_copy(Q, =, -Q, 3);
    parts->f = -1;
  }
  else parts->f = 1;

  // Transpose for our code (we use OpenGL's convention for numbering
  // rows and columns).

  // reinterpret_cast to humor MSVC6, this should have no effect on
  // other compilers.
  SbDPMatrix TQ(reinterpret_cast<const SbDPMat *>(&Q));
  parts->q = SbDPRotation(TQ.transpose());
  parts->k = spect_decomp(S, U);
  // Transpose for our code (we use OpenGL's convention for numbering
  // rows and columns).

  // reinterpret_cast to humor MSVC6, this should have no effect on
  // other compilers.
  SbDPMatrix TU(reinterpret_cast<const SbDPMat *>(&U));
  parts->u = SbDPRotation(TU.transpose());
  p = snuggle(parts->u, parts->k);
  parts->u = p * parts->u;
}

#undef mat_pad
#undef mat_copy
#undef mat_tpose
#undef mat_binop
#undef TOL
#undef SQRTHALF
#undef sgn
#undef swap

#ifdef COIN_TEST_SUITE
#include <Inventor/SbMatrix.h>

BOOST_AUTO_TEST_CASE(constructFromSbMatrix) {
  SbMatrix a(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
  SbMatrixd b;
  double c[]  = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
  b.setValue(c);
  SbDPMatrix d = SbMatrixd(a);
  BOOST_CHECK_MESSAGE(b == d,
                      "Equality comparison failed!");
}
#endif //COIN_TEST_SUITE
