#ifndef COIN_SBVEC4B_H
#define COIN_SBVEC4B_H

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

#include <Inventor/SbBasic.h>
#include <Inventor/system/inttypes.h>
#ifndef NDEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // !NDEBUG

class SbVec4ub;
class SbVec4s;
class SbVec4i32;
class SbVec4f;
class SbVec4d;

class COIN_DLL_API SbVec4b {
public:
  SbVec4b(void) { }
  SbVec4b(const int8_t v[4]) { vec[0] = v[0]; vec[1] = v[1]; vec[2] = v[2]; vec[3] = v[3]; }
  SbVec4b(int8_t x, int8_t y, int8_t z, int8_t w) { vec[0] = x; vec[1] = y; vec[2] = z; vec[3] = w; }
  explicit SbVec4b(const SbVec4ub & v) { setValue(v); }
  explicit SbVec4b(const SbVec4s & v) { setValue(v); }
  explicit SbVec4b(const SbVec4i32 & v) { setValue(v); }
  explicit SbVec4b(const SbVec4f & v) { setValue(v); }
  explicit SbVec4b(const SbVec4d & v) { setValue(v); }

  SbVec4b & setValue(const int8_t v[4]) { vec[0] = v[0]; vec[1] = v[1]; vec[2] = v[2]; vec[3] = v[3]; return *this; }
  SbVec4b & setValue(int8_t x, int8_t y, int8_t z, int8_t w) { vec[0] = x; vec[1] = y; vec[2] = z; vec[3] = w; return *this; }
  SbVec4b & setValue(const SbVec4ub & v);
  SbVec4b & setValue(const SbVec4s & v);
  SbVec4b & setValue(const SbVec4i32 & v);
  SbVec4b & setValue(const SbVec4f & v);
  SbVec4b & setValue(const SbVec4d & v);

  const int8_t * getValue(void) const { return vec; }
  void getValue(int8_t & x, int8_t & y, int8_t & z, int8_t & w) const { x = vec[0]; y = vec[1]; z = vec[2]; w = vec[3]; }

  int8_t & operator [] (int i) { return vec[i]; }
  const int8_t & operator [] (int i) const { return vec[i]; }

  int32_t dot(SbVec4b v) const { return vec[0] * v[0] + vec[1] * v[1] + vec[2] * v[2] + vec[3] * v[3]; }
  void negate(void) { vec[0] = -vec[0]; vec[1] = -vec[1]; vec[2] = -vec[2]; vec[3] = -vec[3]; }

  SbVec4b & operator *= (int d) { vec[0] = int8_t(vec[0] * d); vec[1] = int8_t(vec[1] * d); vec[2] = int8_t(vec[2] * d); vec[3] = int8_t(vec[3] * d); return *this; }
  SbVec4b & operator *= (double d);
  SbVec4b & operator /= (int d) { SbDividerChk("SbVec4b::operator/=(int)", d); vec[0] = int8_t(vec[0] / d); vec[1] = int8_t(vec[1] / d); vec[2] = int8_t(vec[2] / d); vec[3] = int8_t(vec[3] / d); return *this; }
  SbVec4b & operator /= (double d) { SbDividerChk("SbVec4b::operator/=(double)", d); return operator *= (1.0 / d); }
  SbVec4b & operator += (SbVec4b v) { vec[0] += v[0]; vec[1] += v[1]; vec[2] += v[2]; vec[3] += v[3]; return *this; }
  SbVec4b & operator -= (SbVec4b v) { vec[0] -= v[0]; vec[1] -= v[1]; vec[2] -= v[2]; vec[3] -= v[3]; return *this; }
  SbVec4b operator - (void) const { return SbVec4b(-vec[0], -vec[1], -vec[2], -vec[3]); }

protected:
  int8_t vec[4];

}; // SbVec4b

COIN_DLL_API inline SbVec4b operator * (SbVec4b v, int d) {
  SbVec4b val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec4b operator * (SbVec4b v, double d) {
  SbVec4b val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec4b operator * (int d, SbVec4b v) {
  SbVec4b val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec4b operator * (double d, SbVec4b v) {
  SbVec4b val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec4b operator / (SbVec4b v, int d) {
  SbDividerChk("operator/(SbVec4b,int)", d);
  SbVec4b val(v); val /= d; return val;
}

COIN_DLL_API inline SbVec4b operator / (SbVec4b v, double d) {
  SbDividerChk("operator/(SbVec4b,double)", d);
  SbVec4b val(v); val /= d; return val;
}

COIN_DLL_API inline SbVec4b operator + (SbVec4b v1, SbVec4b v2) {
  SbVec4b v(v1); v += v2; return v;
}

COIN_DLL_API inline SbVec4b operator - (SbVec4b v1, SbVec4b v2) {
  SbVec4b v(v1); v -= v2; return v;
}

COIN_DLL_API inline int operator == (SbVec4b v1, SbVec4b v2) {
  return ((v1[0] == v2[0]) && (v1[1] == v2[1]) && (v1[2] == v2[2]) && (v1[3] == v2[3]));
}

COIN_DLL_API inline int operator != (SbVec4b v1, SbVec4b v2) {
  return !(v1 == v2);
}

#endif // !COIN_SBVEC4B_H
