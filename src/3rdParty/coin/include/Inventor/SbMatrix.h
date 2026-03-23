#ifndef COIN_SBMATRIX_H
#define COIN_SBMATRIX_H

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

class SbLine;
class SbRotation;
class SbVec3f;
class SbVec4f;
class SbDPMatrix;

typedef float SbMat[4][4];

class COIN_DLL_API SbMatrix {
public:
  SbMatrix(void);
  SbMatrix(const float a11, const float a12, const float a13, const float a14,
           const float a21, const float a22, const float a23, const float a24,
           const float a31, const float a32, const float a33, const float a34,
           const float a41, const float a42, const float a43, const float a44);
  SbMatrix(const SbMat & matrix);
  SbMatrix(const SbMat * matrix);
  explicit SbMatrix(const SbDPMatrix & matrix);
  ~SbMatrix(void);

  void setValue(const SbMat & m);
  void setValue(const SbDPMatrix & m);
  void setValue(const float * pMat);
  const SbMat & getValue(void) const;

  void makeIdentity(void);
  void setRotate(const SbRotation & q);
  SbMatrix inverse(void) const;
  float det3(int r1, int r2, int r3,
             int c1, int c2, int c3) const;
  float det3(void) const;
  float det4(void) const;

  SbBool equals(const SbMatrix & m, float tolerance) const;


  void getValue(SbMat & m) const;
  static SbMatrix identity(void);
  void setScale(const float s);
  void setScale(const SbVec3f & s);
  void setTranslate(const SbVec3f & t);
  void setTransform(const SbVec3f & t, const SbRotation & r, const SbVec3f & s);
  void setTransform(const SbVec3f & t, const SbRotation & r, const SbVec3f & s,
                    const SbRotation & so);
  void setTransform(const SbVec3f & translation,
                    const SbRotation & rotation, const SbVec3f & scaleFactor,
                    const SbRotation & scaleOrientation, const SbVec3f & center);
  void getTransform(SbVec3f & t, SbRotation & r,
                    SbVec3f & s, SbRotation & so) const;
  void getTransform(SbVec3f & translation, SbRotation & rotation,
                    SbVec3f & scaleFactor, SbRotation & scaleOrientation,
                    const SbVec3f & center) const;
  SbBool factor(SbMatrix & r, SbVec3f & s, SbMatrix & u, SbVec3f & t,
                SbMatrix & proj) const;
  SbBool LUDecomposition(int index[4], float & d);
  void LUBackSubstitution(int index[4], float b[4]) const;
  SbMatrix transpose(void) const;
  SbMatrix & multRight(const SbMatrix & m);
  SbMatrix & multLeft(const SbMatrix & m);
  void multMatrixVec(const SbVec3f & src, SbVec3f & dst) const;
  void multVecMatrix(const SbVec3f & src, SbVec3f & dst) const;
  void multDirMatrix(const SbVec3f & src, SbVec3f & dst) const;
  void multLineMatrix(const SbLine & src, SbLine & dst) const;
  void multVecMatrix(const SbVec4f & src, SbVec4f & dst) const;

  void print(FILE * fp) const;

  operator float*(void);
  operator SbMat&(void);

  float * operator [](int i);
  const float * operator [](int i) const;

  SbMatrix & operator =(const SbMat & m);
  SbMatrix & operator =(const SbMatrix & m);
  SbMatrix & operator =(const SbRotation & q);
  SbMatrix & operator *=(const SbMatrix & m);

  friend COIN_DLL_API SbMatrix operator *(const SbMatrix & m1, const SbMatrix & m2);
  friend COIN_DLL_API int operator ==(const SbMatrix & m1, const SbMatrix & m2);
  friend COIN_DLL_API int operator !=(const SbMatrix & m1, const SbMatrix & m2);

private:
  float matrix[4][4];

  void operator /=(const float v);
  void operator *=(const float v);
};

COIN_DLL_API SbMatrix operator *(const SbMatrix & m1, const SbMatrix & m2);
COIN_DLL_API int operator ==(const SbMatrix & m1, const SbMatrix & m2);
COIN_DLL_API int operator !=(const SbMatrix & m1, const SbMatrix & m2);

#endif // !COIN_SBMATRIX_H
