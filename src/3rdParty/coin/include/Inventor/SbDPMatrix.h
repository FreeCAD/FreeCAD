#ifndef COIN_SBDPMATRIX_H
#define COIN_SBDPMATRIX_H

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

#include <cstdio>
#include <Inventor/SbBasic.h>

class SbDPLine;
class SbDPRotation;
class SbVec3d;
class SbVec4d;
class SbMatrix;

typedef double SbDPMat[4][4];

class COIN_DLL_API SbDPMatrix {
public:
  SbDPMatrix(void);
  SbDPMatrix(const double a11, const double a12, const double a13, const double a14,
             const double a21, const double a22, const double a23, const double a24,
             const double a31, const double a32, const double a33, const double a34,
             const double a41, const double a42, const double a43, const double a44);
  SbDPMatrix(const SbDPMat & matrix);
  SbDPMatrix(const SbDPMat * matrix);
  SbDPMatrix(const SbMatrix & matrix);
  ~SbDPMatrix(void);

  void setValue(const SbDPMat & m);
  void setValue(const SbMatrix & m);
  void setValue(const double * pMat);
  const SbDPMat & getValue(void) const;

  void makeIdentity(void);
  void setRotate(const SbDPRotation & q);
  SbDPMatrix inverse(void) const;
  double det3(int r1, int r2, int r3,
             int c1, int c2, int c3) const;
  double det3(void) const;
  double det4(void) const;

  SbBool equals(const SbDPMatrix & m, double tolerance) const;


  void getValue(SbDPMat & m) const;
  static SbDPMatrix identity(void);
  void setScale(const double s);
  void setScale(const SbVec3d & s);
  void setTranslate(const SbVec3d & t);
  void setTransform(const SbVec3d & t, const SbDPRotation & r, const SbVec3d & s);
  void setTransform(const SbVec3d & t, const SbDPRotation & r, const SbVec3d & s,
                    const SbDPRotation & so);
  void setTransform(const SbVec3d & translation,
                    const SbDPRotation & rotation, const SbVec3d & scaleFactor,
                    const SbDPRotation & scaleOrientation, const SbVec3d & center);
  void getTransform(SbVec3d & t, SbDPRotation & r,
                    SbVec3d & s, SbDPRotation & so) const;
  void getTransform(SbVec3d & translation, SbDPRotation & rotation,
                    SbVec3d & scaleFactor, SbDPRotation & scaleOrientation,
                    const SbVec3d & center) const;
  SbBool factor(SbDPMatrix & r, SbVec3d & s, SbDPMatrix & u, SbVec3d & t,
                SbDPMatrix & proj);
  SbBool LUDecomposition(int index[4], double & d);
  void LUBackSubstitution(int index[4], double b[4]) const;
  SbDPMatrix transpose(void) const;
  SbDPMatrix & multRight(const SbDPMatrix & m);
  SbDPMatrix & multLeft(const SbDPMatrix & m);
  void multMatrixVec(const SbVec3d & src, SbVec3d & dst) const;
  void multVecMatrix(const SbVec3d & src, SbVec3d & dst) const;
  void multDirMatrix(const SbVec3d & src, SbVec3d & dst) const;
  void multLineMatrix(const SbDPLine & src, SbDPLine & dst) const;
  void multVecMatrix(const SbVec4d & src, SbVec4d & dst) const;

  void print(FILE * fp) const;

  operator double*(void);
  operator SbDPMat&(void);
  double * operator [](int i);
  const double * operator [](int i) const;

  SbDPMatrix & operator =(const SbDPMat & m);
  SbDPMatrix & operator =(const SbDPMatrix & m);
  SbDPMatrix & operator =(const SbDPRotation & q);
  SbDPMatrix & operator *=(const SbDPMatrix & m);

  friend COIN_DLL_API SbDPMatrix operator *(const SbDPMatrix & m1, const SbDPMatrix & m2);
  friend COIN_DLL_API int operator ==(const SbDPMatrix & m1, const SbDPMatrix & m2);
  friend COIN_DLL_API int operator !=(const SbDPMatrix & m1, const SbDPMatrix & m2);
private:
  double matrix[4][4];

  void operator /=(const double v);
  void operator *=(const double v);
};

typedef SbDPMatrix SbMatrixd;

COIN_DLL_API SbDPMatrix operator *(const SbDPMatrix & m1, const SbDPMatrix & m2);
COIN_DLL_API int operator ==(const SbDPMatrix & m1, const SbDPMatrix & m2);
COIN_DLL_API int operator !=(const SbDPMatrix & m1, const SbDPMatrix & m2);

#endif // !COIN_SBDPMATRIX_H
