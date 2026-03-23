#ifndef COIN_SBVEC3UB_H
#define COIN_SBVEC3UB_H

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

class SbVec3b;
class SbVec3us;
class SbVec3ui32;

class COIN_DLL_API SbVec3ub {
public:
  SbVec3ub(void) { }
  SbVec3ub(const uint8_t v[3]) { vec[0] = v[0]; vec[1] = v[1]; vec[2] = v[2]; }
  SbVec3ub(uint8_t x, uint8_t y, uint8_t z) { vec[0] = x; vec[1] = y; vec[2] = z; }
  explicit SbVec3ub(const SbVec3b & v) { setValue(v); }
  explicit SbVec3ub(const SbVec3us & v) { setValue(v); }
  explicit SbVec3ub(const SbVec3ui32 & v) { setValue(v); }

  SbVec3ub & setValue(const uint8_t v[3]) { vec[0] = v[0]; vec[1] = v[1]; vec[2] = v[2]; return *this; }
  SbVec3ub & setValue(uint8_t x, uint8_t y, uint8_t z) { vec[0] = x; vec[1] = y; vec[2] = z; return *this; }
  SbVec3ub & setValue(const SbVec3b & v);
  SbVec3ub & setValue(const SbVec3us & v);
  SbVec3ub & setValue(const SbVec3ui32 & v);

  const uint8_t * getValue(void) const { return vec; }
  void getValue(uint8_t & x, uint8_t & y, uint8_t & z) const { x = vec[0]; y = vec[1]; z = vec[2]; }

  uint8_t & operator [] (int i) { return vec[i]; }
  const uint8_t & operator [] (int i) const { return vec[i]; }

  int32_t dot(SbVec3ub v) const { return vec[0] * v[0] + vec[1] * v[1] + vec[2] * v[2]; }
  void negate(void);

  SbVec3ub & operator *= (int d) { vec[0] = uint8_t(vec[0] * d); vec[1] = uint8_t(vec[1] * d); vec[2] = uint8_t(vec[2] * d); return *this; }
  SbVec3ub & operator *= (double d);
  SbVec3ub & operator /= (int d) { SbDividerChk("SbVec3ub::operator/=(int)", d); vec[0] = uint8_t(vec[0] / d); vec[1] = uint8_t(vec[1] / d); vec[2] = uint8_t(vec[2] / d); return *this; }
  SbVec3ub & operator /= (double d) { SbDividerChk("SbVec3ub::operator/=(double)", d); return operator *= (1.0 / d); }
  SbVec3ub & operator += (SbVec3ub v) { vec[0] += v[0]; vec[1] += v[1]; vec[2] += v[2]; return *this; }
  SbVec3ub & operator -= (SbVec3ub v) { vec[0] -= v[0]; vec[1] -= v[1]; vec[2] -= v[2]; return *this; }
  SbVec3ub operator - (void) const { SbVec3ub v(*this); v.negate(); return v; }

protected:
  uint8_t vec[3];

}; // SbVec3ub

COIN_DLL_API inline SbVec3ub operator * (SbVec3ub v, int d) {
  SbVec3ub val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec3ub operator * (SbVec3ub v, double d) {
  SbVec3ub val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec3ub operator * (int d, SbVec3ub v) {
  SbVec3ub val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec3ub operator * (double d, SbVec3ub v) {
  SbVec3ub val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec3ub operator / (SbVec3ub v, int d) {
  SbDividerChk("operator/(SbVec3ub,int)", d);
  SbVec3ub val(v); val /= d; return val;
}

COIN_DLL_API inline SbVec3ub operator / (SbVec3ub v, double d) {
  SbDividerChk("operator/(SbVec3ub,double)", d);
  SbVec3ub val(v); val /= d; return val;
}

COIN_DLL_API inline SbVec3ub operator + (SbVec3ub v1, SbVec3ub v2) {
  SbVec3ub v(v1); v += v2; return v;
}

COIN_DLL_API inline SbVec3ub operator - (SbVec3ub v1, SbVec3ub v2) {
  SbVec3ub v(v1); v -= v2; return v;
}

COIN_DLL_API inline int operator == (SbVec3ub v1, SbVec3ub v2) {
  return ((v1[0] == v2[0]) && (v1[1] == v2[1]) && (v1[2] == v2[2]));
}

COIN_DLL_API inline int operator != (SbVec3ub v1, SbVec3ub v2) {
  return !(v1 == v2);
}

#endif // !COIN_SBVEC3UB_H
