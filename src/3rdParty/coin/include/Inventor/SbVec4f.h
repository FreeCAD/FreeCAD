#ifndef COIN_SBVEC4F_H
#define COIN_SBVEC4F_H

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

class SbVec4d;
class SbVec4b;
class SbVec4s;
class SbVec4i32;
class SbVec3f;

class COIN_DLL_API SbVec4f {
public:
  SbVec4f(void) { }
  SbVec4f(const float v[4]) { vec[0] = v[0]; vec[1] = v[1]; vec[2] = v[2]; vec[3] = v[3]; }
  SbVec4f(float x, float y, float z, float w) { vec[0] = x; vec[1] = y; vec[2] = z; vec[3] = w; }
  explicit SbVec4f(const SbVec4d & v) { setValue(v); }
  explicit SbVec4f(const SbVec4b & v) { setValue(v); }
  explicit SbVec4f(const SbVec4s & v) { setValue(v); }
  explicit SbVec4f(const SbVec4i32 & v) { setValue(v); }

  SbVec4f & setValue(const float v[4]) { vec[0] = v[0]; vec[1] = v[1]; vec[2] = v[2]; vec[3] = v[3]; return *this; }
  SbVec4f & setValue(float x, float y, float z, float w) { vec[0] = x; vec[1] = y; vec[2] = z; vec[3] = w; return *this; }
  SbVec4f & setValue(const SbVec4d & v);
  SbVec4f & setValue(const SbVec4b & v);
  SbVec4f & setValue(const SbVec4s & v);
  SbVec4f & setValue(const SbVec4i32 & v);

  const float * getValue(void) const { return vec; }
  void getValue(float & x, float & y, float & z, float & w) const { x = vec[0]; y = vec[1]; z = vec[2]; w = vec[3]; }

  float & operator [] (int i) { return vec[i]; }
  const float & operator [] (int i) const { return vec[i]; }

  SbBool equals(const SbVec4f & v, float tolerance) const;
  float dot(const SbVec4f & v) const { return vec[0] * v[0] + vec[1] * v[1] + vec[2] * v[2] + vec[3] * v[3]; }
  void getReal(SbVec3f & v) const;
  float length(void) const;
  float sqrLength(void) const { return vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2] + vec[3] * vec[3]; }
  void negate(void) { vec[0] = -vec[0]; vec[1] = -vec[1]; vec[2] = -vec[2]; vec[3] = -vec[3]; }
  float normalize(void);

  SbVec4f & operator *= (float d) { vec[0] *= d; vec[1] *= d; vec[2] *= d; vec[3] *= d; return *this; }
  SbVec4f & operator /= (float d) { SbDividerChk("SbVec4f::operator/=(float)", d); return operator *= (1.0f / d); }
  SbVec4f & operator += (const SbVec4f & v) { vec[0] += v[0]; vec[1] += v[1]; vec[2] += v[2]; vec[3] += v[3]; return *this; }
  SbVec4f & operator -= (const SbVec4f & v) { vec[0] -= v[0]; vec[1] -= v[1]; vec[2] -= v[2]; vec[3] -= v[3]; return *this; }
  SbVec4f operator - (void) const { return SbVec4f(-vec[0], -vec[1], -vec[2], -vec[3]); }

  void print(FILE * fp) const;

protected:
  float vec[4];

}; // SbVec4f

COIN_DLL_API inline SbVec4f operator * (const SbVec4f & v, float d) {
  SbVec4f val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec4f operator * (float d, const SbVec4f & v) {
  SbVec4f val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec4f operator / (const SbVec4f & v, float d) {
  SbDividerChk("operator/(SbVec4f,float)", d);
  SbVec4f val(v); val /= d; return val;
}

COIN_DLL_API inline SbVec4f operator + (const SbVec4f & v1, const SbVec4f & v2) {
  SbVec4f v(v1); v += v2; return v;
}

COIN_DLL_API inline SbVec4f operator - (const SbVec4f & v1, const SbVec4f & v2) {
  SbVec4f v(v1); v -= v2; return v;
}

COIN_DLL_API inline int operator == (const SbVec4f & v1, const SbVec4f & v2) {
  return ((v1[0] == v2[0]) && (v1[1] == v2[1]) && (v1[2] == v2[2]) && (v1[3] == v2[3]));
}

COIN_DLL_API inline int operator != (const SbVec4f & v1, const SbVec4f & v2) {
  return !(v1 == v2);
}

#endif // !COIN_SBVEC4F_H
