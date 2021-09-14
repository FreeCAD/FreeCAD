/***************************************************************************
 *   Copyright (c) 2005 Imetric 3D GmbH                                    *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef BASE_MATRIX_H
#define BASE_MATRIX_H

#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <string>

#include "Vector3D.h"
#ifndef FC_GLOBAL_H
#include <FCGlobal.h>
#endif

namespace Base {

/**
 * The Matrix4D class.
 */
class BaseExport Matrix4D
{
  typedef float_traits<double> traits_type;

public:
  /// Default constructor
  /*!
   * Initialises to an identity matrix
   */
  Matrix4D(void);

  /// Construction
  Matrix4D (float a11, float a12, float a13, float a14,
            float a21, float a22, float a23, float a24,
            float a31, float a32, float a33, float a34,
            float a41, float a42, float a43, float a44 );
  /// Construction
  Matrix4D (double a11, double a12, double a13, double a14,
            double a21, double a22, double a23, double a24,
            double a31, double a32, double a33, double a34,
            double a41, double a42, double a43, double a44 );
  /// Construction
  Matrix4D (const Matrix4D& rclMtrx);
  /// Construction with an Axis
  Matrix4D (const Vector3f& rclBase, const Vector3f& rclDir, float fAngle);
  Matrix4D (const Vector3d& rclBase, const Vector3d& rclDir, double fAngle);
  /// Destruction
  ~Matrix4D () {}

  /** @name Operators */
  //@{
  /// Matrix addition
  inline Matrix4D  operator +  (const Matrix4D& rclMtrx) const;
  inline Matrix4D& operator += (const Matrix4D& rclMtrx);
  /// Matrix subtraction
  inline Matrix4D  operator -  (const Matrix4D& rclMtrx) const;
  inline Matrix4D& operator -= (const Matrix4D& rclMtrx);
  /// Matrix multiplication
  inline Matrix4D& operator *= (const Matrix4D& rclMtrx);
  /// Assignment
  inline Matrix4D& operator =  (const Matrix4D& rclMtrx);
  /// Matrix multiplication
  inline Matrix4D  operator *  (const Matrix4D& rclMtrx) const;
  /// Multiplication matrix with vector
  inline Vector3f  operator *  (const Vector3f& rclVct) const;
  inline Vector3d  operator *  (const Vector3d& rclVct) const;
  inline void multVec(const Vector3d & src, Vector3d & dst) const;
  inline void multVec(const Vector3f & src, Vector3f & dst) const;
  /// Comparison
  inline bool      operator != (const Matrix4D& rclMtrx) const;
  /// Comparison
  inline bool      operator == (const Matrix4D& rclMtrx) const;
  /// Index operator
  inline double*   operator [] (unsigned short usNdx);
  /// Index operator
  inline const double*    operator[] (unsigned short usNdx) const;
  /// Compute the determinant of the matrix
  double determinant() const;
  /// Analyse the transformation
  std::string analyse(void) const;
  /// Outer product (Dyadic product)
  Matrix4D& Outer(const Vector3f& rV1, const Vector3f& rV2);
  Matrix4D& Outer(const Vector3d& rV1, const Vector3d& rV2);
  /// Hat operator (skew symmetric)
  Matrix4D& Hat(const Vector3f& rV);
  Matrix4D& Hat(const Vector3d& rV);
  //@}

  void getMatrix  (double dMtrx[16]) const;
  void setMatrix  (const double dMtrx[16]);
  /// get the matrix in OpenGL style
  void getGLMatrix  (double dMtrx[16]) const;
  /// set the matrix in OpenGL style
  void setGLMatrix  (const double dMtrx[16]);

  unsigned long getMemSpace (void);

  /** @name Manipulation */
  //@{
  /// Makes unity matrix
  void setToUnity(void);
  /// Makes a null matrix
  void nullify(void);
  /// moves the coordinatesystem for the x,y,z value
  void move         (float x, float y, float z)
  { move(Vector3f(x,y,z)); }
  void move         (double x, double y, double z)
  { move(Vector3d(x,y,z)); }
  /// moves the coordinatesystem for the vector
  void move         (const Vector3f& rclVct);
  void move         (const Vector3d& rclVct);
  /// scale for the vector
  void scale        (float x, float y, float z)
  { scale(Vector3f(x,y,z)); }
  void scale        (double x, double y, double z)
  { scale(Vector3d(x,y,z)); }
  /// scale for the x,y,z value
  void scale        (const Vector3f& rclVct);
  void scale        (const Vector3d& rclVct);
  /// Check for scaling factor, 0: not scale, 1: uniform scale, or else -1
  int hasScale(double tol=0.0) const;
  /// Rotate around the X axis (in transformed space) for the given value in radians
  void rotX         (double fAngle);
  /// Rotate around the Y axis (in transformed space) for the given value in radians
  void rotY         (double fAngle);
  /// Rotate around the Z axis (in transformed space) for the given value in radians
  void rotZ         (double fAngle);
  /// Rotate around an arbitrary axis passing the origin in radians
  void rotLine      (const Vector3f& rclVct, float fAngle);
  /// Rotate around an arbitrary axis passing the origin in radians
  void rotLine      (const Vector3d& rclVct, double fAngle);
  /// Rotate around an arbitrary axis that needn't necessarily pass the origin in radians
  void rotLine      (const Vector3f& rclBase, const Vector3f& rclDir, float fAngle);
  /// Rotate around an arbitrary axis that needn't necessarily pass the origin in radians
  void rotLine      (const Vector3d& rclBase, const Vector3d& rclDir, double fAngle);
  /// Extract the rotation axis and angle. Therefore the 3x3 submatrix must be orthogonal.
  bool toAxisAngle (Vector3f& rclBase, Vector3f& rclDir, float& fAngle, float& fTranslation) const;
  bool toAxisAngle (Vector3d& rclBase, Vector3d& rclDir, double& fAngle, double& fTranslation) const;
  /// transform (move,scale,rotate) around a point
  void transform    (const Vector3f& rclVct, const Matrix4D& rclMtrx);
  void transform    (const Vector3d& rclVct, const Matrix4D& rclMtrx);
  /// Matrix is expected to have a 3x3 rotation submatrix.
  void inverse      (void);
  /// Matrix is expected to have a 3x3 rotation submatrix.
  void inverseOrthogonal(void);
  /// Arbitrary, non-singular matrix
  void inverseGauss (void);
  void transpose    (void);
  //@}

  void Print        (void) const;
  /// write the 16 double of the matrix into a string
  std::string toString(void) const;
  /// read the 16 double of the matrix from a string
  void fromString (const std::string &str);

private:
  double  dMtrx4D[4][4];
};

inline Matrix4D Matrix4D::operator  +  (const Matrix4D& rclMtrx) const
{
  Matrix4D  clMat;
  unsigned short iz, is;

  for (iz = 0; iz < 4; iz++) {
    for (is = 0; is < 4; is++) {
      clMat.dMtrx4D[iz][is] = dMtrx4D[iz][is] + rclMtrx[iz][is];
    }
  }

  return clMat;
}

inline Matrix4D& Matrix4D::operator += (const Matrix4D& rclMtrx)
{
  unsigned short iz, is;

  for (iz = 0; iz < 4; iz++) {
    for (is = 0; is < 4; is++) {
      dMtrx4D[iz][is] += rclMtrx[iz][is];
    }
  }

  return *this;
}

inline Matrix4D Matrix4D::operator  -  (const Matrix4D& rclMtrx) const
{
  Matrix4D  clMat;
  unsigned short  iz, is;

  for (iz = 0; iz < 4; iz++) {
    for (is = 0; is < 4; is++) {
      clMat.dMtrx4D[iz][is] = dMtrx4D[iz][is] - rclMtrx[iz][is];
    }
  }

  return clMat;
}

inline Matrix4D& Matrix4D::operator -= (const Matrix4D& rclMtrx)
{
  unsigned short iz, is;

  for (iz = 0; iz < 4; iz++) {
    for (is = 0; is < 4; is++) {
      dMtrx4D[iz][is] -= rclMtrx[iz][is];
    }
  }

  return *this;
}

inline Matrix4D& Matrix4D::operator *= (const Matrix4D& rclMtrx)
{
  Matrix4D  clMat;
  unsigned short ie, iz, is;

  for (iz = 0; iz < 4; iz++)
    for (is = 0; is < 4; is++) {
      clMat.dMtrx4D[iz][is] = 0;
      for (ie = 0; ie < 4; ie++)
        clMat.dMtrx4D[iz][is] += dMtrx4D[iz][ie] *
                          rclMtrx.dMtrx4D[ie][is];
    }

  (*this) = clMat;

  return *this;
}

inline Matrix4D Matrix4D::operator * (const Matrix4D& rclMtrx) const
{
  Matrix4D  clMat;
  unsigned short ie, iz, is;

  for (iz = 0; iz < 4; iz++)
    for (is = 0; is < 4; is++) {
      clMat.dMtrx4D[iz][is] = 0;
      for (ie = 0; ie < 4; ie++)
       	clMat.dMtrx4D[iz][is] += dMtrx4D[iz][ie] *
                          rclMtrx.dMtrx4D[ie][is];
    }

  return clMat;
}

inline Matrix4D& Matrix4D::operator= (const Matrix4D& rclMtrx)
{
  unsigned short iz, is;

  for (iz = 0; iz < 4; iz++) {
    for (is = 0; is < 4; is++) {
      dMtrx4D[iz][is] = rclMtrx.dMtrx4D[iz][is];
    }
  }

  return *this;
}

inline Vector3f Matrix4D::operator* (const Vector3f& rclVct) const
{
  double x = static_cast<double>(rclVct.x);
  double y = static_cast<double>(rclVct.y);
  double z = static_cast<double>(rclVct.z);
  return Vector3f(
    static_cast<float>(dMtrx4D[0][0]*x + dMtrx4D[0][1]*y +
                       dMtrx4D[0][2]*z + dMtrx4D[0][3]),
    static_cast<float>(dMtrx4D[1][0]*x + dMtrx4D[1][1]*y +
                       dMtrx4D[1][2]*z + dMtrx4D[1][3]),
    static_cast<float>(dMtrx4D[2][0]*x + dMtrx4D[2][1]*y +
                       dMtrx4D[2][2]*z + dMtrx4D[2][3])
  );
}

inline Vector3d Matrix4D::operator* (const Vector3d& rclVct) const
{
  return Vector3d((dMtrx4D[0][0]*rclVct.x + dMtrx4D[0][1]*rclVct.y +
                   dMtrx4D[0][2]*rclVct.z + dMtrx4D[0][3]),
                  (dMtrx4D[1][0]*rclVct.x + dMtrx4D[1][1]*rclVct.y +
                   dMtrx4D[1][2]*rclVct.z + dMtrx4D[1][3]),
                  (dMtrx4D[2][0]*rclVct.x + dMtrx4D[2][1]*rclVct.y +
                   dMtrx4D[2][2]*rclVct.z + dMtrx4D[2][3]));
}

inline void Matrix4D::multVec(const Vector3d & src, Vector3d & dst) const
{
  double x = (dMtrx4D[0][0]*src.x + dMtrx4D[0][1]*src.y +
              dMtrx4D[0][2]*src.z + dMtrx4D[0][3]);
  double y = (dMtrx4D[1][0]*src.x + dMtrx4D[1][1]*src.y +
              dMtrx4D[1][2]*src.z + dMtrx4D[1][3]);
  double z = (dMtrx4D[2][0]*src.x + dMtrx4D[2][1]*src.y +
              dMtrx4D[2][2]*src.z + dMtrx4D[2][3]);
  dst.Set(x,y,z);
}

inline void Matrix4D::multVec(const Vector3f & src, Vector3f & dst) const
{
  double sx = static_cast<double>(src.x);
  double sy = static_cast<double>(src.y);
  double sz = static_cast<double>(src.z);

  double x = (dMtrx4D[0][0]*sx + dMtrx4D[0][1]*sy +
              dMtrx4D[0][2]*sz + dMtrx4D[0][3]);
  double y = (dMtrx4D[1][0]*sx + dMtrx4D[1][1]*sy +
              dMtrx4D[1][2]*sz + dMtrx4D[1][3]);
  double z = (dMtrx4D[2][0]*sx + dMtrx4D[2][1]*sy +
              dMtrx4D[2][2]*sz + dMtrx4D[2][3]);
  dst.Set(static_cast<float>(x),
          static_cast<float>(y),
          static_cast<float>(z));
}

inline bool Matrix4D::operator== (const Matrix4D& rclMtrx) const
{
  unsigned short iz, is;

  for (iz = 0; iz < 4; iz++) {
    for (is = 0; is < 4; is++) {
      if (fabs(dMtrx4D[iz][is] - rclMtrx.dMtrx4D[iz][is]) > traits_type::epsilon())
        return false;
    }
  }

  return true;
}

inline bool Matrix4D::operator!= (const Matrix4D& rclMtrx) const
{
  return !( (*this) == rclMtrx );
}

inline Vector3f& operator*= (Vector3f& rclVect,
                              const Matrix4D& rclMtrx)
{
  rclVect = rclMtrx * rclVect;
  return rclVect;
}

inline double* Matrix4D::operator[] (unsigned short usNdx)
{
  return dMtrx4D[usNdx];
}

inline const double* Matrix4D::operator[] (unsigned short usNdx) const
{
  return dMtrx4D[usNdx];
}

} // namespace Base


#endif // BASE_MATRIX_H


