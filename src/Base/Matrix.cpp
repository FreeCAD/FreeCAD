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
# include <memory>
# include <cstring>
# include <sstream>
#endif


#include "Matrix.h"

using namespace Base;

Matrix4D::Matrix4D (void)
{
  setToUnity();
}

Matrix4D::Matrix4D (float a11, float a12, float a13, float a14, 
                    float a21, float a22, float a23, float a24,
                    float a31, float a32, float a33, float a34,
                    float a41, float a42, float a43, float a44 )
{
  dMtrx4D[0][0] = a11; dMtrx4D[0][1] = a12; dMtrx4D[0][2] = a13; dMtrx4D[0][3] = a14;
  dMtrx4D[1][0] = a21; dMtrx4D[1][1] = a22; dMtrx4D[1][2] = a23; dMtrx4D[1][3] = a24;
  dMtrx4D[2][0] = a31; dMtrx4D[2][1] = a32; dMtrx4D[2][2] = a33; dMtrx4D[2][3] = a34;
  dMtrx4D[3][0] = a41; dMtrx4D[3][1] = a42; dMtrx4D[3][2] = a43; dMtrx4D[3][3] = a44;
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

void Matrix4D::setToUnity (void)
{
  dMtrx4D[0][0] = 1.0; dMtrx4D[0][1] = 0.0; dMtrx4D[0][2] = 0.0; dMtrx4D[0][3] = 0.0;
  dMtrx4D[1][0] = 0.0; dMtrx4D[1][1] = 1.0; dMtrx4D[1][2] = 0.0; dMtrx4D[1][3] = 0.0;
  dMtrx4D[2][0] = 0.0; dMtrx4D[2][1] = 0.0; dMtrx4D[2][2] = 1.0; dMtrx4D[2][3] = 0.0;
  dMtrx4D[3][0] = 0.0; dMtrx4D[3][1] = 0.0; dMtrx4D[3][2] = 0.0; dMtrx4D[3][3] = 1.0;
}

void Matrix4D::nullify(void)
{
  dMtrx4D[0][0] = 0.0; dMtrx4D[0][1] = 0.0; dMtrx4D[0][2] = 0.0; dMtrx4D[0][3] = 0.0;
  dMtrx4D[1][0] = 0.0; dMtrx4D[1][1] = 0.0; dMtrx4D[1][2] = 0.0; dMtrx4D[1][3] = 0.0;
  dMtrx4D[2][0] = 0.0; dMtrx4D[2][1] = 0.0; dMtrx4D[2][2] = 0.0; dMtrx4D[2][3] = 0.0;
  dMtrx4D[3][0] = 0.0; dMtrx4D[3][1] = 0.0; dMtrx4D[3][2] = 0.0; dMtrx4D[3][3] = 0.0;
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

/*
void Matrix4D::setMoveX (float fMove)
{
  Matrix4D clMat;

  clMat.dMtrx4D[0][3] = fMove;
  (*this) *= clMat;
}

void Matrix4D::setMoveY (float fMove)
{
  Matrix4D clMat;

  clMat.dMtrx4D[1][3] = fMove;
  (*this) *= clMat;
}

void Matrix4D::setMoveZ (float fMove)
{
  Matrix4D clMat;

  clMat.dMtrx4D[2][3] = fMove;
  (*this) *= clMat;
}
*/
void Matrix4D::move (const Vector3f& rclVct)
{
  Matrix4D clMat;

  clMat.dMtrx4D[0][3] = rclVct.x;
  clMat.dMtrx4D[1][3] = rclVct.y;
  clMat.dMtrx4D[2][3] = rclVct.z;
  (*this) *= clMat;
}
void Matrix4D::move (const Vector3d& rclVct)
{
  Matrix4D clMat;

  clMat.dMtrx4D[0][3] = rclVct.x;
  clMat.dMtrx4D[1][3] = rclVct.y;
  clMat.dMtrx4D[2][3] = rclVct.z;
  (*this) *= clMat;
}
/*
void Matrix4D::setScaleX (float fScale)
{
  Matrix4D clMat;

  clMat.dMtrx4D[0][0] = fScale;
  
  (*this) *= clMat;
}

void Matrix4D::setScaleY (float fScale)
{
  Matrix4D clMat;

  clMat.dMtrx4D[1][1] = fScale;
  (*this) *= clMat;
}

void Matrix4D::setScaleZ (float fScale)
{
  Matrix4D clMat;

  clMat.dMtrx4D[2][2] = fScale;
  (*this) *= clMat;
}
*/

void Matrix4D::scale (const Vector3f& rclVct)
{
  Matrix4D clMat;

  clMat.dMtrx4D[0][0] = rclVct.x;
  clMat.dMtrx4D[1][1] = rclVct.y;
  clMat.dMtrx4D[2][2] = rclVct.z;
  (*this) *= clMat;
}
void Matrix4D::scale (const Vector3d& rclVct)
{
  Matrix4D clMat;

  clMat.dMtrx4D[0][0] = rclVct.x;
  clMat.dMtrx4D[1][1] = rclVct.y;
  clMat.dMtrx4D[2][2] = rclVct.z;
  (*this) *= clMat;
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

void Matrix4D::rotLine (const Vector3d& rclVct, double fAngle)
{
  // **** algorithm was taken from a math book
  Matrix4D  clMA, clMB, clMC, clMRot;
  Vector3d  clRotAxis(rclVct);
  short iz, is;
  double fcos, fsin;

  // set all entries to "0"
  for (iz = 0; iz < 4; iz++)
    for (is = 0; is < 4; is++)  {
      clMA.dMtrx4D[iz][is] = 0;
      clMB.dMtrx4D[iz][is] = 0;
      clMC.dMtrx4D[iz][is] = 0;
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

  for (iz = 0; iz < 3; iz++)
    for (is = 0; is < 3; is++)
      clMRot.dMtrx4D[iz][is] = clMA.dMtrx4D[iz][is]  + clMB.dMtrx4D[iz][is] +
                               clMC.dMtrx4D[iz][is];
  (*this) *= clMRot;
}

void Matrix4D::rotLine (const Vector3f& rclVct, float fAngle)
{
    Vector3d tmp(rclVct.x,rclVct.y,rclVct.z);
    rotLine(tmp,fAngle);
}

void Matrix4D::rotLine(const Vector3d& rclBase, const Vector3d& rclDir, double fAngle)
{
  Matrix4D  clMT, clMRot, clMInvT, clM;
  Vector3d clBase(rclBase);
  
  clMT.move(clBase);            // Translation
  clMInvT.move(clBase *= (-1.0f));  // inverse Translation
  clMRot.rotLine(rclDir, fAngle);

  clM = clMRot * clMInvT;
  clM = clMT * clM; 
  (*this) *= clM;  
}

void Matrix4D::rotLine   (const Vector3f& rclBase, const Vector3f& rclDir, float fAngle)
{
  Matrix4D  clMT, clMRot, clMInvT, clM;
  Vector3f clBase(rclBase);
  
  clMT.move(clBase);            // Translation
  clMInvT.move(clBase *= (-1.0f));  // inverse Translation
  clMRot.rotLine(rclDir, fAngle);

  clM = clMRot * clMInvT;
  clM = clMT * clM; 
  (*this) *= clM;  
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
  rfAngle = (float)acos(fCos);  // in [0,PI]

  if ( rfAngle > 0.0f )
  {
    if ( rfAngle < F_PI )
    {
      rclDir.x = (float)(dMtrx4D[2][1]-dMtrx4D[1][2]);
      rclDir.y = (float)(dMtrx4D[0][2]-dMtrx4D[2][0]);
      rclDir.z = (float)(dMtrx4D[1][0]-dMtrx4D[0][1]);
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
          rclDir.x = (float)(0.5*sqrt(dMtrx4D[0][0] - dMtrx4D[1][1] - dMtrx4D[2][2] + 1.0));
          fHalfInverse = 0.5/rclDir.x;
          rclDir.y = (float)(fHalfInverse*dMtrx4D[0][1]);
          rclDir.z = (float)(fHalfInverse*dMtrx4D[0][2]);
        }
        else
        {
          // r22 is maximum diagonal term
          rclDir.z = (float)(0.5*sqrt(dMtrx4D[2][2] - dMtrx4D[0][0] - dMtrx4D[1][1] + 1.0));
          fHalfInverse = 0.5/rclDir.z;
          rclDir.x = (float)(fHalfInverse*dMtrx4D[0][2]);
          rclDir.y = (float)(fHalfInverse*dMtrx4D[1][2]);
        }
      }
      else
      {
        // r11 > r00
        if ( dMtrx4D[1][1] >= dMtrx4D[2][2] )
        {
          // r11 is maximum diagonal term
          rclDir.y = (float)(0.5*sqrt(dMtrx4D[1][1] - dMtrx4D[0][0] - dMtrx4D[2][2] + 1.0));
          fHalfInverse  = 0.5/rclDir.y;
          rclDir.x = (float)(fHalfInverse*dMtrx4D[0][1]);
          rclDir.z = (float)(fHalfInverse*dMtrx4D[1][2]);
        }
        else
        {
          // r22 is maximum diagonal term
          rclDir.z = (float)(0.5*sqrt(dMtrx4D[2][2] - dMtrx4D[0][0] - dMtrx4D[1][1] + 1.0));
          fHalfInverse = 0.5/rclDir.z;
          rclDir.x = (float)(fHalfInverse*dMtrx4D[0][2]);
          rclDir.y = (float)(fHalfInverse*dMtrx4D[1][2]);
        }
      }
    }
  }
  else
  {
    // The angle is 0 and the matrix is the identity.  Any axis will
    // work, so just use the x-axis.
    rclDir.x = 1.0f;
    rclDir.y = 0.0f;
    rclDir.z = 0.0f;
    rclBase.x = 0.0f;
    rclBase.y = 0.0f;
    rclBase.z = 0.0f;
  }

  // This is the translation part in axis direction
  fTranslation = (float)(dMtrx4D[0][3]*rclDir.x+dMtrx4D[1][3]*rclDir.y+dMtrx4D[2][3]*rclDir.z);
  Vector3f cPnt((float)dMtrx4D[0][3],(float)dMtrx4D[1][3],(float)dMtrx4D[2][3]); 
  cPnt = cPnt - fTranslation * rclDir;

  // This is the base point of the rotation axis
  if ( rfAngle > 0.0f )
  {
    double factor = 0.5*(1.0+fTrace)/sin(rfAngle);
    rclBase.x = (float)(0.5*(cPnt.x+factor*(rclDir.y*cPnt.z-rclDir.z*cPnt.y)));
    rclBase.y = (float)(0.5*(cPnt.y+factor*(rclDir.z*cPnt.x-rclDir.x*cPnt.z)));
    rclBase.z = (float)(0.5*(cPnt.z+factor*(rclDir.x*cPnt.y-rclDir.y*cPnt.x)));
  }

  return true;
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

  if ( rfAngle > 0.0f )
  {
    if ( rfAngle < F_PI )
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
  if ( rfAngle > 0.0f )
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
  (*this) *= rclMtrx;
  move(rclVct);
}

void Matrix4D::transform (const Vector3d& rclVct, const Matrix4D& rclMtrx)
{
  move(-rclVct);
  (*this) *= rclMtrx;
  move(rclVct);
}

void Matrix4D::inverse (void)
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
    if (a[4*icol+icol] == 0) return;
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
/* ------------------------------------------------------------------------
   Matrix_identity(Matrix a)

   Puts an identity matrix in matrix a
   ------------------------------------------------------------------------ */

void Matrix_identity (Matrix a)
{
  int i;
  for (i = 0; i < 16; i++) a[i] = 0;
  a[0] = 1;
  a[5] = 1;
  a[10] = 1;
  a[15] = 1;
}

/* ------------------------------------------------------------------------
   Matrix_invert(Matrix a, Matrix inva)

   Inverts Matrix a and places the result in inva.
   Relies on the Gaussian Elimination code above. (See Numerical recipes).
   ------------------------------------------------------------------------ */
void Matrix_invert (Matrix a, Matrix inva)
{

  double  temp[16];
  int     i;

  for (i = 0; i < 16; i++)
    temp[i] = a[i];
  Matrix_identity(inva);
  Matrix_gauss(temp,inva);
}

void  Matrix4D::inverseOrthogonal(void)
{
    Base::Vector3d c(dMtrx4D[0][3],dMtrx4D[1][3],dMtrx4D[2][3]);
    transpose();
    c = this->operator * (c);
    dMtrx4D[0][3] = -c.x; dMtrx4D[3][0] = 0;
    dMtrx4D[1][3] = -c.y; dMtrx4D[3][1] = 0;
    dMtrx4D[2][3] = -c.z; dMtrx4D[3][2] = 0;
}

void Matrix4D::inverseGauss (void)
{
  double matrix        [16]; 
  double inversematrix [16] = { 1 ,0 ,0 ,0 ,
                                0 ,1 ,0 ,0 ,
                                0 ,0 ,1 ,0 ,
                                0 ,0 ,0 ,1 }; 
  getGLMatrix(matrix);

//  Matrix_invert(matrix, inversematrix);
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

unsigned long Matrix4D::getMemSpace (void)
{
  return sizeof(Matrix4D);
}

void Matrix4D::Print (void) const
{
  short i;

  for (i = 0; i < 4; i++)
    printf("%9.3f %9.3f %9.3f %9.3f\n", dMtrx4D[i][0], dMtrx4D[i][1], dMtrx4D[i][2], dMtrx4D[i][3]);
}

void Matrix4D::transpose (void)
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
std::string Matrix4D::toString(void) const
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
std::string Matrix4D::analyse(void) const
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
                for (int i=0; i<4 && ortho; i++) {
                    for (int j=0; j<4 && ortho; j++) {
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
