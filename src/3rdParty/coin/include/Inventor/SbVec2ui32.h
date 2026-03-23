#ifndef COIN_SBVEC2UI32_H
#define COIN_SBVEC2UI32_H

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

class SbVec2i32;
class SbVec2ub;
class SbVec2us;

class COIN_DLL_API SbVec2ui32 {
public:
  SbVec2ui32(void) { }
  SbVec2ui32(const uint32_t v[2]) { vec[0] = v[0]; vec[1] = v[1]; }
  SbVec2ui32(uint32_t x, uint32_t y) { vec[0] = x; vec[1] = y; }
  explicit SbVec2ui32(const SbVec2i32 & v) { setValue(v); }
  explicit SbVec2ui32(const SbVec2ub & v) { setValue(v); }
  explicit SbVec2ui32(const SbVec2us & v) { setValue(v); }

  SbVec2ui32 & setValue(const uint32_t v[2]) { vec[0] = v[0]; vec[1] = v[1]; return *this; }
  SbVec2ui32 & setValue(uint32_t x, uint32_t y) { vec[0] = x; vec[1] = y; return *this; }
  SbVec2ui32 & setValue(const SbVec2i32 & v);
  SbVec2ui32 & setValue(const SbVec2ub & v);
  SbVec2ui32 & setValue(const SbVec2us & v);

  const uint32_t * getValue(void) const { return vec; }
  void getValue(uint32_t & x, uint32_t & y) const { x = vec[0]; y = vec[1]; }

  uint32_t & operator [] (int i) { return vec[i]; }
  const uint32_t & operator [] (int i) const { return vec[i]; }

  int32_t dot(const SbVec2ui32 & v) const { return vec[0] * v[0] + vec[1] * v[1]; }
  void negate(void);

  SbVec2ui32 & operator *= (int d) { vec[0] *= d; vec[1] *= d; return *this; }
  SbVec2ui32 & operator *= (double d);
  SbVec2ui32 & operator /= (int d) { SbDividerChk("SbVec2ui32::operator/=(int)", d); vec[0] /= d; vec[1] /= d; return *this; }
  SbVec2ui32 & operator /= (double d) { SbDividerChk("SbVec2ui32::operator/=(double)", d); return operator *= (1.0 / d); }
  SbVec2ui32 & operator += (const SbVec2ui32 & v) { vec[0] += v[0]; vec[1] += v[1]; return *this; }
  SbVec2ui32 & operator -= (const SbVec2ui32 & v) { vec[0] -= v[0]; vec[1] -= v[1]; return *this; }
  SbVec2ui32 operator - (void) const { SbVec2ui32 v(*this); v.negate(); return v; }

protected:
  uint32_t vec[2];

}; // SbVec2ui32

COIN_DLL_API inline SbVec2ui32 operator * (const SbVec2ui32 & v, int d) {
  SbVec2ui32 val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec2ui32 operator * (const SbVec2ui32 & v, double d) {
  SbVec2ui32 val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec2ui32 operator * (int d, const SbVec2ui32 & v) {
  SbVec2ui32 val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec2ui32 operator * (double d, const SbVec2ui32 & v) {
  SbVec2ui32 val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec2ui32 operator / (const SbVec2ui32 & v, int d) {
  SbDividerChk("operator/=(SbVec2ui32,int)", d);
  SbVec2ui32 val(v); val /= d; return val;
}

COIN_DLL_API inline SbVec2ui32 operator / (const SbVec2ui32 & v, double d) {
  SbDividerChk("operator/=(SbVec2ui32,double)", d);
  SbVec2ui32 val(v); val /= d; return val;
}

COIN_DLL_API inline SbVec2ui32 operator + (const SbVec2ui32 & v1, const SbVec2ui32 & v2) {
  SbVec2ui32 v(v1); v += v2; return v;
}

COIN_DLL_API inline SbVec2ui32 operator - (const SbVec2ui32 & v1, const SbVec2ui32 & v2) {
  SbVec2ui32 v(v1); v -= v2; return v;
}

COIN_DLL_API inline int operator == (const SbVec2ui32 & v1, const SbVec2ui32 & v2) {
  return ((v1[0] == v2[0]) && (v1[1] == v2[1]));
}

COIN_DLL_API inline int operator != (const SbVec2ui32 & v1, const SbVec2ui32 & v2) {
  return !(v1 == v2);
}

#endif // !COIN_SBVEC2UI32_H
