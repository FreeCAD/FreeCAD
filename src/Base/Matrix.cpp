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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <cstring>
# include <sstream>
#endif

#include "Matrix.h"
#include "Converter.h"


using namespace Base;

Matrix4D::Matrix4D ()
{
    setToUnity();
}

Matrix4D::Matrix4D (float a11, float a12, float a13, float a14,
                    float a21, float a22, float a23, float a24,
                    float a31, float a32, float a33, float a34,
                    float a41, float a42, float a43, float a44 )
{
    dMtrx4D[0][0] = static_cast<double>(a11);
    dMtrx4D[0][1] = static_cast<double>(a12);
    dMtrx4D[0][2] = static_cast<double>(a13);
    dMtrx4D[0][3] = static_cast<double>(a14);
    dMtrx4D[1][0] = static_cast<double>(a21);
    dMtrx4D[1][1] = static_cast<double>(a22);
    dMtrx4D[1][2] = static_cast<double>(a23);
    dMtrx4D[1][3] = static_cast<double>(a24);
    dMtrx4D[2][0] = static_cast<double>(a31);
    dMtrx4D[2][1] = static_cast<double>(a32);
    dMtrx4D[2][2] = static_cast<double>(a33);
    dMtrx4D[2][3] = static_cast<double>(a34);
    dMtrx4D[3][0] = static_cast<double>(a41);
    dMtrx4D[3][1] = static_cast<double>(a42);
    dMtrx4D[3][2] = static_cast<double>(a43);
    dMtrx4D[3][3] = static_cast<double>(a44);
}

Matrix4D::Matrix4D (double a11, double a12, double a13, double a14,
                    double a21, double a22, double a23, double a24,
                    double a31, double a32, double a33, double a34,
                    double a41, double a42, double a43, double a44 )
{
    dMtrx4D[0][0] = a11; dMtrx4D[0][1] = a12; dMtrx4D[0][2] = a13; dMtrx4D[0][3] = a14;
    dMtrx4D[1][0] = a21; dMtrx4D[1][1] = a22; dMtrx4D[1][2] = a23; dMtrx4D[1][3] = a24;
    dMtrx4D[2][0] = a31; dMtrx4D[2][1] = a32; dMtrx4D[2][2] = a33; dMtrx4D[2][3] = a34;
    dMtrx4D[3][0] = a41; dMtrx4D[3][1] = a42; dMtrx4D[3][2] = a43; dMtrx4D[3][3] = a44;
}


Matrix4D::Matrix4D (const Matrix4D& rclMtrx)
{
    (*this) = rclMtrx;
}

Matrix4D::Matrix4D (const Vector3f& rclBase, const Vector3f& rclDir, float fAngle)
{
    setToUnity();
    this->rotLine(rclBase,rclDir,fAngle);
}

Matrix4D::Matrix4D (const Vector3d& rclBase, const Vector3d& rclDir, double fAngle)
{
    setToUnity();
    this->rotLine(rclBase,rclDir,fAngle);
}

void Matrix4D::setToUnity ()
{
    dMtrx4D[0][0] = 1.0; dMtrx4D[0][1] = 0.0; dMtrx4D[0][2] = 0.0; dMtrx4D[0][3] = 0.0;
    dMtrx4D[1][0] = 0.0; dMtrx4D[1][1] = 1.0; dMtrx4D[1][2] = 0.0; dMtrx4D[1][3] = 0.0;
    dMtrx4D[2][0] = 0.0; dMtrx4D[2][1] = 0.0; dMtrx4D[2][2] = 1.0; dMtrx4D[2][3] = 0.0;
    dMtrx4D[3][0] = 0.0; dMtrx4D[3][1] = 0.0; dMtrx4D[3][2] = 0.0; dMtrx4D[3][3] = 1.0;
}

bool Matrix4D::isUnity() const
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (i == j) {
                if (dMtrx4D[i][j] != 1.0)
                    return false;
            }
            else {
                if (dMtrx4D[i][j] != 0.0)
                    return false;
            }
        }
    }

    return true;
}

void Matrix4D::nullify()
{
    dMtrx4D[0][0] = 0.0; dMtrx4D[0][1] = 0.0; dMtrx4D[0][2] = 0.0; dMtrx4D[0][3] = 0.0;
    dMtrx4D[1][0] = 0.0; dMtrx4D[1][1] = 0.0; dMtrx4D[1][2] = 0.0; dMtrx4D[1][3] = 0.0;
    dMtrx4D[2][0] = 0.0; dMtrx4D[2][1] = 0.0; dMtrx4D[2][2] = 0.0; dMtrx4D[2][3] = 0.0;
    dMtrx4D[3][0] = 0.0; dMtrx4D[3][1] = 0.0; dMtrx4D[3][2] = 0.0; dMtrx4D[3][3] = 0.0;
}

bool Matrix4D::isNull() const
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (dMtrx4D[i][j] != 0.0)
                return false;
        }
    }

    return true;
}

double Matrix4D::determinant() const
{
    double fA0 = dMtrx4D[0][0]*dMtrx4D[1][1] - dMtrx4D[0][1]*dMtrx4D[1][0];
    double fA1 = dMtrx4D[0][0]*dMtrx4D[1][2] - dMtrx4D[0][2]*dMtrx4D[1][0];
    double fA2 = dMtrx4D[0][0]*dMtrx4D[1][3] - dMtrx4D[0][3]*dMtrx4D[1][0];
    double fA3 = dMtrx4D[0][1]*dMtrx4D[1][2] - dMtrx4D[0][2]*dMtrx4D[1][1];
    double fA4 = dMtrx4D[0][1]*dMtrx4D[1][3] - dMtrx4D[0][3]*dMtrx4D[1][1];
    double fA5 = dMtrx4D[0][2]*dMtrx4D[1][3] - dMtrx4D[0][3]*dMtrx4D[1][2];
    double fB0 = dMtrx4D[2][0]*dMtrx4D[3][1] - dMtrx4D[2][1]*dMtrx4D[3][0];
    double fB1 = dMtrx4D[2][0]*dMtrx4D[3][2] - dMtrx4D[2][2]*dMtrx4D[3][0];
    double fB2 = dMtrx4D[2][0]*dMtrx4D[3][3] - dMtrx4D[2][3]*dMtrx4D[3][0];
    double fB3 = dMtrx4D[2][1]*dMtrx4D[3][2] - dMtrx4D[2][2]*dMtrx4D[3][1];
    double fB4 = dMtrx4D[2][1]*dMtrx4D[3][3] - dMtrx4D[2][3]*dMtrx4D[3][1];
    double fB5 = dMtrx4D[2][2]*dMtrx4D[3][3] - dMtrx4D[2][3]*dMtrx4D[3][2];
    double fDet = fA0*fB5-fA1*fB4+fA2*fB3+fA3*fB2-fA4*fB1+fA5*fB0;
    return fDet;
}

double Matrix4D::determinant3() const
{
    double a = dMtrx4D[0][0] * dMtrx4D[1][1] * dMtrx4D[2][2];
    double b = dMtrx4D[0][1] * dMtrx4D[1][2] * dMtrx4D[2][0];
    double c = dMtrx4D[1][0] * dMtrx4D[2][1] * dMtrx4D[0][2];
    double d = dMtrx4D[0][2] * dMtrx4D[1][1] * dMtrx4D[2][0];
    double e = dMtrx4D[1][0] * dMtrx4D[0][1] * dMtrx4D[2][2];
    double f = dMtrx4D[0][0] * dMtrx4D[2][1] * dMtrx4D[1][2];
    double det = (a + b + c) - (d + e + f);
    return det;
}

void Matrix4D::move (const Vector3f& rclVct)
{
    move(convertTo<Vector3d>(rclVct));
}

void Matrix4D::move (const Vector3d& rclVct)
{
    dMtrx4D[0][3] += rclVct.x;
    dMtrx4D[1][3] += rclVct.y;
    dMtrx4D[2][3] += rclVct.z;
}

void Matrix4D::scale (const Vector3f& rclVct)
{
    scale(convertTo<Vector3d>(rclVct));
}

void Matrix4D::scale (const Vector3d& rclVct)
{
    Matrix4D clMat;

    clMat.dMtrx4D[0][0] = rclVct.x;
    clMat.dMtrx4D[1][1] = rclVct.y;
    clMat.dMtrx4D[2][2] = rclVct.z;
    (*this) = clMat * (*this);
}

void Matrix4D::rotX (double fAngle)
{
    Matrix4D clMat;
    double fsin, fcos;

    fsin = sin (fAngle);
    fcos = cos (fAngle);
    clMat.dMtrx4D[1][1] =  fcos;  clMat.dMtrx4D[2][2] = fcos;
    clMat.dMtrx4D[1][2] = -fsin;  clMat.dMtrx4D[2][1] = fsin;

    (*this) = clMat * (*this);
}

void Matrix4D::rotY (double fAngle)
{
    Matrix4D clMat;
    double fsin, fcos;

    fsin = sin (fAngle);
    fcos = cos (fAngle);
    clMat.dMtrx4D[0][0] =  fcos;  clMat.dMtrx4D[2][2] = fcos;
    clMat.dMtrx4D[2][0] = -fsin;  clMat.dMtrx4D[0][2] = fsin;

    (*this) = clMat * (*this);
}

void Matrix4D::rotZ (double fAngle)
{
    Matrix4D clMat;
    double fsin, fcos;

    fsin = sin (fAngle);
    fcos = cos (fAngle);
    clMat.dMtrx4D[0][0] =  fcos;  clMat.dMtrx4D[1][1] = fcos;
    clMat.dMtrx4D[0][1] = -fsin;  clMat.dMtrx4D[1][0] = fsin;

    (*this) = clMat * (*this);
}

void Matrix4D::rotLine(const Vector3d& rclVct, double fAngle)
{
    // **** algorithm was taken from a math book
    Matrix4D  clMA, clMB, clMC, clMRot;
    Vector3d  clRotAxis(rclVct);
    short iz, is;
    double fcos, fsin;

    // set all entries to "0"
    for (iz = 0; iz < 4; iz++) {
        for (is = 0; is < 4; is++)  {
            clMA.dMtrx4D[iz][is] = 0;
            clMB.dMtrx4D[iz][is] = 0;
            clMC.dMtrx4D[iz][is] = 0;
        }
    }

    // ** normalize the rotation axis
    clRotAxis.Normalize();

    // ** set the rotation matrix (formula taken from a math book) */
    fcos = cos(fAngle);
    fsin = sin(fAngle);

    clMA.dMtrx4D[0][0] = (1-fcos) * clRotAxis.x * clRotAxis.x;
    clMA.dMtrx4D[0][1] = (1-fcos) * clRotAxis.x * clRotAxis.y;
    clMA.dMtrx4D[0][2] = (1-fcos) * clRotAxis.x * clRotAxis.z;
    clMA.dMtrx4D[1][0] = (1-fcos) * clRotAxis.x * clRotAxis.y;
    clMA.dMtrx4D[1][1] = (1-fcos) * clRotAxis.y * clRotAxis.y;
    clMA.dMtrx4D[1][2] = (1-fcos) * clRotAxis.y * clRotAxis.z;
    clMA.dMtrx4D[2][0] = (1-fcos) * clRotAxis.x * clRotAxis.z;
    clMA.dMtrx4D[2][1] = (1-fcos) * clRotAxis.y * clRotAxis.z;
    clMA.dMtrx4D[2][2] = (1-fcos) * clRotAxis.z * clRotAxis.z;

    clMB.dMtrx4D[0][0] = fcos;
    clMB.dMtrx4D[1][1] = fcos;
    clMB.dMtrx4D[2][2] = fcos;

    clMC.dMtrx4D[0][1] = -fsin * clRotAxis.z;
    clMC.dMtrx4D[0][2] =  fsin * clRotAxis.y;
    clMC.dMtrx4D[1][0] =  fsin * clRotAxis.z;
    clMC.dMtrx4D[1][2] = -fsin * clRotAxis.x;
    clMC.dMtrx4D[2][0] = -fsin * clRotAxis.y;
    clMC.dMtrx4D[2][1] =  fsin * clRotAxis.x;

    for (iz = 0; iz < 3; iz++) {
        for (is = 0; is < 3; is++)
            clMRot.dMtrx4D[iz][is] = clMA.dMtrx4D[iz][is] +
                                     clMB.dMtrx4D[iz][is] +
                                     clMC.dMtrx4D[iz][is];
    }

    (*this) = clMRot * (*this);
}

void Matrix4D::rotLine(const Vector3f& rclVct, float fAngle)
{
    Vector3d tmp = convertTo<Vector3d>(rclVct);
    rotLine(tmp, static_cast<double>(fAngle));
}

void Matrix4D::rotLine(const Vector3d& rclBase, const Vector3d& rclDir, double fAngle)
{
    Matrix4D clMRot;
    clMRot.rotLine(rclDir, fAngle);
    transform(rclBase, clMRot);
}

void Matrix4D::rotLine(const Vector3f& rclBase, const Vector3f& rclDir, float fAngle)
{
    Vector3d pnt = convertTo<Vector3d>(rclBase);
    Vector3d dir = convertTo<Vector3d>(rclDir);
    rotLine(pnt,dir,static_cast<double>(fAngle));
}

/**
 * If this matrix describes a rotation around an arbitrary axis with a translation (in axis direction)
 * then the base point of the axis, its direction, the rotation angle and the translation part get calculated.
 * In this case the return value is set to true, if this matrix doesn't describe a rotation false is returned.
 *
 * The translation vector can be calculated with \a fTranslation * \a rclDir, whereas the length of \a rclDir
 * is normalized to 1.
 *
 * Note: In case the \a fTranslation part is zero then passing \a rclBase, \a rclDir and \a rfAngle to a new
 * matrix object creates an identical matrix.
 */
bool Matrix4D::toAxisAngle (Vector3f& rclBase, Vector3f& rclDir, float& rfAngle, float& fTranslation) const
{
    Vector3d pnt = convertTo<Vector3d>(rclBase);
    Vector3d dir = convertTo<Vector3d>(rclDir);
    double dAngle = static_cast<double>(rfAngle);
    double dTranslation = static_cast<double>(fTranslation);

    bool ok = toAxisAngle(pnt, dir, dAngle, dTranslation);
    if (ok) {
        rclBase = convertTo<Vector3f>(pnt);
        rclDir = convertTo<Vector3f>(dir);
        rfAngle = static_cast<float>(dAngle);
        fTranslation = static_cast<float>(dTranslation);
    }

    return ok;
}

bool Matrix4D::toAxisAngle (Vector3d& rclBase, Vector3d& rclDir, double& rfAngle, double& fTranslation) const
{
  // First check if the 3x3 submatrix is orthogonal
  for ( int i=0; i<3; i++ ) {
    // length must be one
    if ( fabs(dMtrx4D[0][i]*dMtrx4D[0][i]+dMtrx4D[1][i]*dMtrx4D[1][i]+dMtrx4D[2][i]*dMtrx4D[2][i]-1.0) > 0.01 )
      return false;
    // scalar product with other rows must be zero
    if ( fabs(dMtrx4D[0][i]*dMtrx4D[0][(i+1)%3]+dMtrx4D[1][i]*dMtrx4D[1][(i+1)%3]+dMtrx4D[2][i]*dMtrx4D[2][(i+1)%3]) > 0.01 )
      return false;
  }

  // Okay, the 3x3 matrix is orthogonal.
  // Note: The section to get the rotation axis and angle was taken from WildMagic Library.
  //
  // Let (x,y,z) be the unit-length axis and let A be an angle of rotation.
  // The rotation matrix is R = I + sin(A)*P + (1-cos(A))*P^2 where
  // I is the identity and
  //
  //       +-        -+
  //   P = |  0 -z +y |
  //       | +z  0 -x |
  //       | -y +x  0 |
  //       +-        -+
  //
  // If A > 0, R represents a counterclockwise rotation about the axis in
  // the sense of looking from the tip of the axis vector towards the
  // origin.  Some algebra will show that
  //
  //   cos(A) = (trace(R)-1)/2  and  R - R^t = 2*sin(A)*P
  //
  // In the event that A = pi, R-R^t = 0 which prevents us from extracting
  // the axis through P.  Instead note that R = I+2*P^2 when A = pi, so
  // P^2 = (R-I)/2.  The diagonal entries of P^2 are x^2-1, y^2-1, and
  // z^2-1.  We can solve these for axis (x,y,z).  Because the angle is pi,
  // it does not matter which sign you choose on the square roots.
  //
  // For more details see also http://www.math.niu.edu/~rusin/known-math/97/rotations

  double fTrace = dMtrx4D[0][0] + dMtrx4D[1][1] + dMtrx4D[2][2];
  double fCos = 0.5*(fTrace-1.0);
  rfAngle = acos(fCos);  // in [0,PI]

  if ( rfAngle > 0.0 )
  {
    if ( rfAngle < D_PI )
    {
      rclDir.x = (dMtrx4D[2][1]-dMtrx4D[1][2]);
      rclDir.y = (dMtrx4D[0][2]-dMtrx4D[2][0]);
      rclDir.z = (dMtrx4D[1][0]-dMtrx4D[0][1]);
      rclDir.Normalize();
    }
    else
    {
      // angle is PI
      double fHalfInverse;
      if ( dMtrx4D[0][0] >= dMtrx4D[1][1] )
      {
        // r00 >= r11
        if ( dMtrx4D[0][0] >= dMtrx4D[2][2] )
        {
          // r00 is maximum diagonal term
          rclDir.x = (0.5*sqrt(dMtrx4D[0][0] - dMtrx4D[1][1] - dMtrx4D[2][2] + 1.0));
          fHalfInverse = 0.5/rclDir.x;
          rclDir.y = (fHalfInverse*dMtrx4D[0][1]);
          rclDir.z = (fHalfInverse*dMtrx4D[0][2]);
        }
        else
        {
          // r22 is maximum diagonal term
          rclDir.z = (0.5*sqrt(dMtrx4D[2][2] - dMtrx4D[0][0] - dMtrx4D[1][1] + 1.0));
          fHalfInverse = 0.5/rclDir.z;
          rclDir.x = (fHalfInverse*dMtrx4D[0][2]);
          rclDir.y = (fHalfInverse*dMtrx4D[1][2]);
        }
      }
      else
      {
        // r11 > r00
        if ( dMtrx4D[1][1] >= dMtrx4D[2][2] )
        {
          // r11 is maximum diagonal term
          rclDir.y = (0.5*sqrt(dMtrx4D[1][1] - dMtrx4D[0][0] - dMtrx4D[2][2] + 1.0));
          fHalfInverse  = 0.5/rclDir.y;
          rclDir.x = (fHalfInverse*dMtrx4D[0][1]);
          rclDir.z = (fHalfInverse*dMtrx4D[1][2]);
        }
        else
        {
          // r22 is maximum diagonal term
          rclDir.z = (0.5*sqrt(dMtrx4D[2][2] - dMtrx4D[0][0] - dMtrx4D[1][1] + 1.0));
          fHalfInverse = 0.5/rclDir.z;
          rclDir.x = (fHalfInverse*dMtrx4D[0][2]);
          rclDir.y = (fHalfInverse*dMtrx4D[1][2]);
        }
      }
    }
  }
  else
  {
    // The angle is 0 and the matrix is the identity.  Any axis will
    // work, so just use the x-axis.
    rclDir.x = 1.0;
    rclDir.y = 0.0;
    rclDir.z = 0.0;
    rclBase.x = 0.0;
    rclBase.y = 0.0;
    rclBase.z = 0.0;
  }

  // This is the translation part in axis direction
  fTranslation = (dMtrx4D[0][3]*rclDir.x+dMtrx4D[1][3]*rclDir.y+dMtrx4D[2][3]*rclDir.z);
  Vector3d cPnt(dMtrx4D[0][3],dMtrx4D[1][3],dMtrx4D[2][3]);
  cPnt = cPnt - fTranslation * rclDir;

  // This is the base point of the rotation axis
  if ( rfAngle > 0.0 )
  {
    double factor = 0.5*(1.0+fTrace)/sin(rfAngle);
    rclBase.x = (0.5*(cPnt.x+factor*(rclDir.y*cPnt.z-rclDir.z*cPnt.y)));
    rclBase.y = (0.5*(cPnt.y+factor*(rclDir.z*cPnt.x-rclDir.x*cPnt.z)));
    rclBase.z = (0.5*(cPnt.z+factor*(rclDir.x*cPnt.y-rclDir.y*cPnt.x)));
  }

  return true;
}

void Matrix4D::transform (const Vector3f& rclVct, const Matrix4D& rclMtrx)
{
    move(-rclVct);
    (*this) = rclMtrx * (*this);
    move(rclVct);
}

void Matrix4D::transform (const Vector3d& rclVct, const Matrix4D& rclMtrx)
{
    move(-rclVct);
    (*this) = rclMtrx * (*this);
    move(rclVct);
}

void Matrix4D::inverse ()
{
  Matrix4D clInvTrlMat, clInvRotMat;
  short  iz, is;

  /**** Herausnehmen und Inversion der TranslationsMatrix
  aus der TransformationMatrix                      ****/
  for (iz = 0 ;iz < 3; iz++)
    clInvTrlMat.dMtrx4D[iz][3] = -dMtrx4D[iz][3];

  /**** Herausnehmen und Inversion der RotationsMatrix
  aus der TransformationMatrix                      ****/
  for (iz = 0 ;iz < 3; iz++)
    for (is = 0 ;is < 3; is++)
      clInvRotMat.dMtrx4D[iz][is] = dMtrx4D[is][iz];

  /**** inv(M) = inv(Mtrl * Mrot) = inv(Mrot) * inv(Mtrl) ****/
  (*this) = clInvRotMat * clInvTrlMat;
}

typedef  double * Matrix;

void Matrix_gauss(Matrix a, Matrix b)
{
  int ipiv[4], indxr[4], indxc[4];
  int i,j,k,l,ll;
  int irow=0, icol=0;
  double big, pivinv;
  double dum;
  for (j = 0; j < 4; j++)
    ipiv[j] = 0;
  for (i = 0; i < 4; i++) {
    big = 0;
    for (j = 0; j < 4; j++) {
      if (ipiv[j] != 1) {
  for (k = 0; k < 4; k++) {
    if (ipiv[k] == 0) {
      if (fabs(a[4*j+k]) >= big) {
        big = fabs(a[4*j+k]);
        irow = j;
        icol = k;
      }
    } else if (ipiv[k] > 1)
      return;  /* Singular matrix */
  }
      }
    }
    ipiv[icol] = ipiv[icol]+1;
    if (irow != icol) {
      for (l = 0; l < 4; l++) {
  dum = a[4*irow+l];
  a[4*irow+l] = a[4*icol+l];
  a[4*icol+l] = dum;
      }
      for (l = 0; l < 4; l++) {
  dum = b[4*irow+l];
  b[4*irow+l] = b[4*icol+l];
  b[4*icol+l] = dum;
      }
    }
    indxr[i] = irow;
    indxc[i] = icol;
    if (a[4*icol+icol] == 0.0)
        return;
    pivinv = 1.0/a[4*icol+icol];
    a[4*icol+icol] = 1.0;
    for (l = 0; l < 4; l++)
      a[4*icol+l] = a[4*icol+l]*pivinv;
    for (l = 0; l < 4; l++)
      b[4*icol+l] = b[4*icol+l]*pivinv;
    for (ll = 0; ll < 4; ll++) {
      if (ll != icol) {
  dum = a[4*ll+icol];
  a[4*ll+icol] = 0;
  for (l = 0; l < 4; l++)
    a[4*ll+l] = a[4*ll+l] - a[4*icol+l]*dum;
  for (l = 0; l < 4; l++)
    b[4*ll+l] = b[4*ll+l] - b[4*icol+l]*dum;
      }
    }
  }
  for (l = 3; l >= 0; l--) {
    if (indxr[l] != indxc[l]) {
      for (k = 0; k < 4; k++) {
  dum = a[4*k+indxr[l]];
  a[4*k+indxr[l]] = a[4*k+indxc[l]];
  a[4*k+indxc[l]] = dum;
      }
    }
  }
}

void  Matrix4D::inverseOrthogonal()
{
    Base::Vector3d c(dMtrx4D[0][3],dMtrx4D[1][3],dMtrx4D[2][3]);
    transpose();
    c = this->operator * (c);
    dMtrx4D[0][3] = -c.x; dMtrx4D[3][0] = 0;
    dMtrx4D[1][3] = -c.y; dMtrx4D[3][1] = 0;
    dMtrx4D[2][3] = -c.z; dMtrx4D[3][2] = 0;
}

void Matrix4D::inverseGauss ()
{
  double matrix        [16];
  double inversematrix [16] = { 1 ,0 ,0 ,0 ,
                                0 ,1 ,0 ,0 ,
                                0 ,0 ,1 ,0 ,
                                0 ,0 ,0 ,1 };
  getGLMatrix(matrix);

  Matrix_gauss(matrix,inversematrix);

  setGLMatrix(inversematrix);
}

void Matrix4D::getMatrix  (double dMtrx[16]) const
{
  short iz, is;

  for (iz = 0; iz < 4; iz++)
    for (is = 0; is < 4; is++)
      dMtrx[ 4*iz + is ] = dMtrx4D[iz][is];
}

void Matrix4D::setMatrix  (const double dMtrx[16])
{
  short iz, is;

  for (iz = 0; iz < 4; iz++)
    for (is = 0; is < 4; is++)
      dMtrx4D[iz][is] = dMtrx[ 4*iz + is ];
}

void Matrix4D::getGLMatrix (double dMtrx[16]) const
{
  short iz, is;

  for (iz = 0; iz < 4; iz++)
    for (is = 0; is < 4; is++)
      dMtrx[ iz + 4*is ] = dMtrx4D[iz][is];
}

void Matrix4D::setGLMatrix (const double dMtrx[16])
{
  short iz, is;

  for (iz = 0; iz < 4; iz++)
    for (is = 0; is < 4; is++)
      dMtrx4D[iz][is] = dMtrx[ iz + 4*is ];
}

unsigned long Matrix4D::getMemSpace ()
{
    return sizeof(Matrix4D);
}

void Matrix4D::Print () const
{
    short i;
    for (i = 0; i < 4; i++)
        printf("%9.3f %9.3f %9.3f %9.3f\n", dMtrx4D[i][0], dMtrx4D[i][1], dMtrx4D[i][2], dMtrx4D[i][3]);
}

void Matrix4D::transpose ()
{
  double  dNew[4][4];

  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
      dNew[j][i] = dMtrx4D[i][j];
  }

  memcpy(dMtrx4D, dNew, sizeof(dMtrx4D));
}



// write the 12 double of the matrix in a stream
std::string Matrix4D::toString() const
{
  std::stringstream str;
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
      str << dMtrx4D[i][j] << " ";
  }

  return str.str();
}

// read the 12 double of the matrix from a stream
void Matrix4D::fromString(const std::string &str)
{
  std::stringstream input;
  input.str(str);

  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
      input >> dMtrx4D[i][j];
  }
}

// Analyse the a transformation Matrix and describe the transformation
std::string Matrix4D::analyse() const
{
    const double eps=1.0e-06;
    bool hastranslation = (dMtrx4D[0][3] != 0.0 ||
            dMtrx4D[1][3] != 0.0 || dMtrx4D[2][3] != 0.0);
    const Base::Matrix4D unityMatrix = Base::Matrix4D();
    std::string text;
    if (*this == unityMatrix)
    {
        text = "Unity Matrix";
    }
    else
    {
        if (dMtrx4D[3][0] != 0.0 || dMtrx4D[3][1] != 0.0 ||
            dMtrx4D[3][2] != 0.0 || dMtrx4D[3][3] != 1.0)
        {
            text = "Projection";
        }
        else //translation and affine
        {
            if (dMtrx4D[0][1] == 0.0 &&  dMtrx4D[0][2] == 0.0 &&
                dMtrx4D[1][0] == 0.0 &&  dMtrx4D[1][2] == 0.0 &&
                dMtrx4D[2][0] == 0.0 &&  dMtrx4D[2][1] == 0.0) //scaling
            {
                std::ostringstream stringStream;
                stringStream << "Scale [" << dMtrx4D[0][0] << ", "  <<
                    dMtrx4D[1][1] << ", " << dMtrx4D[2][2] << "]";
                text = stringStream.str();
            }
            else
            {
                Base::Matrix4D sub;
                sub[0][0] = dMtrx4D[0][0]; sub[0][1] = dMtrx4D[0][1];
                sub[0][2] = dMtrx4D[0][2]; sub[1][0] = dMtrx4D[1][0];
                sub[1][1] = dMtrx4D[1][1]; sub[1][2] = dMtrx4D[1][2];
                sub[2][0] = dMtrx4D[2][0]; sub[2][1] = dMtrx4D[2][1];
                sub[2][2] = dMtrx4D[2][2];

                Base::Matrix4D trp = sub;
                trp.transpose();
                trp = trp * sub;
                bool ortho = true;
                for (unsigned short i=0; i<4 && ortho; i++) {
                    for (unsigned short j=0; j<4 && ortho; j++) {
                        if (i != j) {
                            if (fabs(trp[i][j]) > eps) {
                                ortho = false;
                                break;
                            }
                        }
                    }
                }

                double determinant = sub.determinant();
                if (ortho)
                {
                    if (fabs(determinant-1.0)<eps ) //rotation
                    {
                        text = "Rotation Matrix";
                    }
                    else
                    {
                        if (fabs(determinant+1.0)<eps ) //rotation
                        {
                            text = "Rotinversion Matrix";
                        }
                        else //scaling with rotation
                        {
                            std::ostringstream stringStream;
                            stringStream << "Scale and Rotate ";
                            if (determinant<0.0 )
                                stringStream << "and Invert ";
                            stringStream << "[ " <<
            sqrt(trp[0][0]) << ", "  << sqrt(trp[1][1]) << ", " <<
            sqrt(trp[2][2]) << "]";
                                text = stringStream.str();
                        }
                    }
                }
                else
                {
                    std::ostringstream stringStream;
                    stringStream << "Affine with det= " <<
                        determinant;
                    text = stringStream.str();
                }
            }
        }
        if (hastranslation)
            text += " with Translation";
    }
    return text;
}

Matrix4D& Matrix4D::Outer(const Vector3f& rV1, const Vector3f& rV2)
{
    setToUnity();

    Outer(convertTo<Vector3d>(rV1), convertTo<Vector3d>(rV2));

    return *this;
}

Matrix4D& Matrix4D::Outer(const Vector3d& rV1, const Vector3d& rV2)
{
    setToUnity();

    dMtrx4D[0][0] = rV1.x * rV2.x;
    dMtrx4D[0][1] = rV1.x * rV2.y;
    dMtrx4D[0][2] = rV1.x * rV2.z;

    dMtrx4D[1][0] = rV1.y * rV2.x;
    dMtrx4D[1][1] = rV1.y * rV2.y;
    dMtrx4D[1][2] = rV1.y * rV2.z;

    dMtrx4D[2][0] = rV1.z * rV2.x;
    dMtrx4D[2][1] = rV1.z * rV2.y;
    dMtrx4D[2][2] = rV1.z * rV2.z;

    return *this;
}

Matrix4D& Matrix4D::Hat(const Vector3f& rV)
{
    setToUnity();

    Hat(convertTo<Vector3d>(rV));

    return *this;
}

Matrix4D& Matrix4D::Hat(const Vector3d& rV)
{
    setToUnity();

    dMtrx4D[0][0] = 0.0;
    dMtrx4D[0][1] = -rV.z;
    dMtrx4D[0][2] = rV.y;

    dMtrx4D[1][0] = rV.z;
    dMtrx4D[1][1] = 0.0;
    dMtrx4D[1][2] = -rV.x;

    dMtrx4D[2][0] = -rV.y;
    dMtrx4D[2][1] = rV.x;
    dMtrx4D[2][2] = 0.0;

    return *this;
}

ScaleType Matrix4D::hasScale(double tol) const
{
    // check for uniform scaling
    //
    // For a scaled rotation matrix it matters whether
    // the scaling was applied from the left or right side.
    // Only in case of uniform scaling it doesn't make a difference.
    if (tol == 0.0)
        tol = 1e-9;

    // get column vectors
    double dx = Vector3d(dMtrx4D[0][0],dMtrx4D[1][0],dMtrx4D[2][0]).Sqr();
    double dy = Vector3d(dMtrx4D[0][1],dMtrx4D[1][1],dMtrx4D[2][1]).Sqr();
    double dz = Vector3d(dMtrx4D[0][2],dMtrx4D[1][2],dMtrx4D[2][2]).Sqr();
    double dxyz = sqrt(dx * dy * dz);

    // get row vectors
    double du = Vector3d(dMtrx4D[0][0],dMtrx4D[0][1],dMtrx4D[0][2]).Sqr();
    double dv = Vector3d(dMtrx4D[1][0],dMtrx4D[1][1],dMtrx4D[1][2]).Sqr();
    double dw = Vector3d(dMtrx4D[2][0],dMtrx4D[2][1],dMtrx4D[2][2]).Sqr();
    double duvw = sqrt(du * dv * dw);

    double d3 = determinant3();

    // This could be e.g. a projection, a shearing,... matrix
    if (fabs(dxyz - d3) > tol && fabs(duvw - d3) > tol) {
        return ScaleType::Other;
    }

    if (fabs(duvw - d3) <= tol && (fabs(du - dv) > tol || fabs(dv - dw) > tol)) {
        return ScaleType::NonUniformLeft;
    }

    if (fabs(dxyz - d3) <= tol && (fabs(dx - dy) > tol || fabs(dy - dz) > tol)) {
        return ScaleType::NonUniformRight;
    }

    if (fabs(dx - 1.0) > tol) {
        return ScaleType::Uniform;
    }

    return ScaleType::NoScaling;
}
