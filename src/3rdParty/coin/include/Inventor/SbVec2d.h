#ifndef COIN_SBVEC2D_H
#define COIN_SBVEC2D_H

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
#ifndef NDEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // !NDEBUG

class SbVec2f;
class SbVec2b;
class SbVec2s;
class SbVec2i32;

class COIN_DLL_API SbVec2d {
public:
  SbVec2d(void) { }
  SbVec2d(const double v[2]) { vec[0] = v[0]; vec[1] = v[1]; }
  SbVec2d(double x, double y) { vec[0] = x; vec[1] = y; }
  explicit SbVec2d(const SbVec2f & v) { setValue(v); }
  explicit SbVec2d(const SbVec2b & v) { setValue(v); }
  explicit SbVec2d(const SbVec2s & v) { setValue(v); }
  explicit SbVec2d(const SbVec2i32 & v) { setValue(v); }

  SbVec2d & setValue(const double v[2]) { vec[0] = v[0]; vec[1] = v[1]; return *this; }
  SbVec2d & setValue(double x, double y) { vec[0] = x; vec[1] = y; return *this; }
  SbVec2d & setValue(const SbVec2f & v);
  SbVec2d & setValue(const SbVec2b & v);
  SbVec2d & setValue(const SbVec2s & v);
  SbVec2d & setValue(const SbVec2i32 & v);

  const double * getValue(void) const { return vec; }
  void getValue(double & x, double & y) const { x = vec[0]; y = vec[1]; }

  double & operator [] (int i) { return vec[i]; } 
  const double & operator [] (int i) const { return vec[i]; }

  SbBool equals(const SbVec2d & v, double tolerance) const;
  double dot(const SbVec2d & v) const { return vec[0] * v[0] + vec[1] * v[1]; }
  double length(void) const;
  double sqrLength(void) const { return vec[0]*vec[0] + vec[1]*vec[1]; }
  double normalize(void);
  void negate(void) { vec[0] = -vec[0]; vec[1] = -vec[1]; }

  SbVec2d & operator *= (double d) { vec[0] *= d; vec[1] *= d; return *this; }
  SbVec2d & operator /= (double d) { SbDividerChk("SbVec2d::operator/=(double)", d); return operator *= (1.0 / d); }
  SbVec2d & operator += (const SbVec2d & v) { vec[0] += v[0]; vec[1] += v[1]; return *this; }
  SbVec2d & operator -= (const SbVec2d & v) { vec[0] -= v[0]; vec[1] -= v[1]; return *this; }
  SbVec2d operator - (void) const { return SbVec2d(-vec[0], -vec[1]); }

  void print(FILE * fp) const;

protected:
  double vec[2];

}; // SbVec2d

COIN_DLL_API inline SbVec2d operator * (const SbVec2d & v, const double d) {
  SbVec2d val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec2d operator * (const double d, const SbVec2d & v) {
  SbVec2d val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec2d operator / (const SbVec2d & v, const double d) {
  SbDividerChk("operator/(SbVec2d,double)", d);
  SbVec2d val(v); val /= d; return val;
}

COIN_DLL_API inline SbVec2d operator + (const SbVec2d & v1, const SbVec2d & v2) {
  SbVec2d v(v1); v += v2; return v;
}

COIN_DLL_API inline SbVec2d operator - (const SbVec2d & v1, const SbVec2d & v2) {
  SbVec2d v(v1); v -= v2; return v;
}

COIN_DLL_API inline int operator == (const SbVec2d & v1, const SbVec2d & v2) {
  return ((v1[0] == v2[0]) && (v1[1] == v2[1]));
}

COIN_DLL_API inline int operator != (const SbVec2d & v1, const SbVec2d & v2) {
  return !(v1 == v2);
}

// *************************************************************************

#endif // !COIN_SBVEC2D_H
