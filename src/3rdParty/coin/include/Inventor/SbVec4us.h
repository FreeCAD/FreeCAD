#ifndef COIN_SBVEC4US_H
#define COIN_SBVEC4US_H

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

class SbVec4s;
class SbVec4ub;
class SbVec4ui32;

class COIN_DLL_API SbVec4us {
public:
  SbVec4us(void) { }
  SbVec4us(const unsigned short v[4]) { vec[0] = v[0]; vec[1] = v[1]; vec[2] = v[2]; vec[3] = v[3]; }
  SbVec4us(unsigned short x, unsigned short y, unsigned short z, unsigned short w) { vec[0] = x; vec[1] = y; vec[2] = z; vec[3] = w; }
  explicit SbVec4us(const SbVec4s & v) { setValue(v); }
  explicit SbVec4us(const SbVec4ub & v) { setValue(v); }
  explicit SbVec4us(const SbVec4ui32 & v) { setValue(v); }

  SbVec4us & setValue(const unsigned short v[4]) { vec[0] = v[0]; vec[1] = v[1]; vec[2] = v[2]; vec[3] = v[3]; return *this; }
  SbVec4us & setValue(unsigned short x, unsigned short y, unsigned short z, unsigned short w) { vec[0] = x; vec[1] = y; vec[2] = z; vec[3] = w; return *this; }
  SbVec4us & setValue(const SbVec4s & v);
  SbVec4us & setValue(const SbVec4ub & v);
  SbVec4us & setValue(const SbVec4ui32 & v);

  const unsigned short * getValue(void) const { return vec; }
  void getValue(unsigned short & x, unsigned short & y, unsigned short & z, unsigned short & w) const { x = vec[0]; y = vec[1]; z = vec[2]; w = vec[3]; }

  unsigned short & operator [] (int i) { return vec[i]; }
  const unsigned short & operator [] (int i) const { return vec[i]; }

  int32_t dot(const SbVec4us & v) const { return vec[0] * v[0] + vec[1] * v[1] + vec[2] * v[2] + vec[3] * v[3]; }
  void negate(void);

  SbVec4us & operator *= (int d) { vec[0] = (unsigned short)(vec[0] * d); vec[1] = (unsigned short)(vec[1] * d); vec[2] = (unsigned short)(vec[2] * d); vec[3] = (unsigned short)(vec[3] * d); return *this; }
  SbVec4us & operator *= (double d);
  SbVec4us & operator /= (int d) { SbDividerChk("SbVec4us::operator/=(int)", d); vec[0] = (unsigned short)(vec[0] / d); vec[1] = (unsigned short)(vec[1] / d); vec[2] = (unsigned short)(vec[2] / d); vec[3] = (unsigned short)(vec[3] / d); return *this; }
  SbVec4us & operator /= (double d) { SbDividerChk("SbVec4us::operator/=(double)", d); return operator *= (1.0 / d); }
  SbVec4us & operator += (const SbVec4us & v) { vec[0] += v[0]; vec[1] += v[1]; vec[2] += v[2]; vec[3] += v[3]; return *this; }
  SbVec4us & operator -= (const SbVec4us & v) { vec[0] -= v[0]; vec[1] -= v[1]; vec[2] -= v[2]; vec[3] -= v[3]; return *this; }
  SbVec4us operator - (void) const { SbVec4us v(*this); v.negate(); return v; }

protected:
  unsigned short vec[4];

}; // SbVec4us

COIN_DLL_API inline SbVec4us operator * (const SbVec4us & v, int d) {
  SbVec4us val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec4us operator * (const SbVec4us & v, double d) {
  SbVec4us val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec4us operator * (int d, const SbVec4us & v) {
  SbVec4us val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec4us operator * (double d, const SbVec4us & v) {
  SbVec4us val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec4us operator / (const SbVec4us & v, int d) {
  SbDividerChk("operator/(SbVec4us,int)", d);
  SbVec4us val(v); val /= d; return val;
}

COIN_DLL_API inline SbVec4us operator / (const SbVec4us & v, double d) {
  SbDividerChk("operator/(SbVec4us,double)", d);
  SbVec4us val(v); val /= d; return val;
}

COIN_DLL_API inline SbVec4us operator + (const SbVec4us & v1, const SbVec4us & v2) {
  SbVec4us v(v1); v += v2; return v;
}

COIN_DLL_API inline SbVec4us operator - (const SbVec4us & v1, const SbVec4us & v2) {
  SbVec4us v(v1); v -= v2; return v;
}

COIN_DLL_API inline int operator == (const SbVec4us & v1, const SbVec4us & v2) {
  return ((v1[0] == v2[0]) && (v1[1] == v2[1]) && (v1[2] == v2[2]) && (v1[3] == v2[3]));
}

COIN_DLL_API inline int operator != (const SbVec4us & v1, const SbVec4us & v2) {
  return !(v1 == v2);
}

#endif // !COIN_SBVEC4US_H
