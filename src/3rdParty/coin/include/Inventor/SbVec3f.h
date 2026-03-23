#ifndef COIN_SBVEC3F_H
#define COIN_SBVEC3F_H

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
#include <Inventor/SbByteBuffer.h>
#include <Inventor/SbString.h>
#ifndef NDEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // !NDEBUG

class SbPlane;
class SbVec3d;
class SbVec3b;
class SbVec3s;
class SbVec3i32;

class COIN_DLL_API SbVec3f {
public:
  SbVec3f(void) { }
  SbVec3f(const float v[3]) { vec[0] = v[0]; vec[1] = v[1]; vec[2] = v[2]; }
  SbVec3f(float x, float y, float z) { vec[0] = x; vec[1] = y; vec[2] = z; }
  explicit SbVec3f(const SbVec3d & v) { setValue(v); }
  explicit SbVec3f(const SbVec3b & v) { setValue(v); }
  explicit SbVec3f(const SbVec3s & v) { setValue(v); }
  explicit SbVec3f(const SbVec3i32 & v) { setValue(v); }
  SbVec3f(const SbPlane & p0, const SbPlane & p1, const SbPlane & p2);

  SbVec3f & setValue(const float v[3]) { vec[0] = v[0]; vec[1] = v[1]; vec[2] = v[2]; return *this; }
  SbVec3f & setValue(float x, float y, float z) { vec[0] = x; vec[1] = y; vec[2] = z; return *this; }
  SbVec3f & setValue(const SbVec3f & barycentric,
                     const SbVec3f & v0,
                     const SbVec3f & v1,
                     const SbVec3f & v2);
  SbVec3f & setValue(const SbVec3d & v);
  SbVec3f & setValue(const SbVec3b & v);
  SbVec3f & setValue(const SbVec3s & v);
  SbVec3f & setValue(const SbVec3i32 & v);

  const float * getValue(void) const { return vec; }
  void getValue(float & x, float & y, float & z) const { x = vec[0]; y = vec[1]; z = vec[2]; }

  float & operator [] (int i) { return vec[i]; }
  const float & operator [] (int i) const { return vec[i]; }

  SbBool equals(const SbVec3f & v, float tolerance) const;
  SbVec3f cross(const SbVec3f & v) const;
  float dot(const SbVec3f & v) const { return vec[0] * v[0] + vec[1] * v[1] + vec[2] * v[2]; }
  SbVec3f getClosestAxis(void) const;
  float length(void) const;
  float sqrLength(void) const { return vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]; }
  float normalize(void);
  void negate(void) { vec[0] = -vec[0]; vec[1] = -vec[1]; vec[2] = -vec[2]; }

  SbVec3f & operator *= (float d) { vec[0] *= d; vec[1] *= d; vec[2] *= d; return *this; }
  SbVec3f & operator /= (float d) { SbDividerChk("SbVec3f::operator/=(float)", d); return operator *= (1.0f / d); }
  SbVec3f & operator += (const SbVec3f & v) { vec[0] += v[0]; vec[1] += v[1]; vec[2] += v[2]; return *this; }
  SbVec3f & operator -= (const SbVec3f & v) { vec[0] -= v[0]; vec[1] -= v[1]; vec[2] -= v[2]; return *this; }
  SbVec3f operator - (void) const { return SbVec3f(-vec[0], -vec[1], -vec[2]); }

  SbString toString() const;
  SbBool fromString(const SbString & str);

  void print(FILE * fp) const;

protected:
  float vec[3];

}; // SbVec3f

COIN_DLL_API inline SbVec3f operator * (const SbVec3f & v, float d) {
  SbVec3f val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec3f operator * (float d, const SbVec3f & v) {
  SbVec3f val(v); val *= d; return val;
}

COIN_DLL_API inline SbVec3f operator / (const SbVec3f & v, float d) {
  SbDividerChk("operator/(SbVec3f,float)", d);
  SbVec3f val(v); val /= d; return val;
}

COIN_DLL_API inline SbVec3f operator + (const SbVec3f & v1, const SbVec3f & v2) {
  SbVec3f v(v1); v += v2; return v;
}

COIN_DLL_API inline SbVec3f operator - (const SbVec3f & v1, const SbVec3f & v2) {
  SbVec3f v(v1); v -= v2; return v;
}

COIN_DLL_API inline int operator == (const SbVec3f & v1, const SbVec3f & v2) {
  return ((v1[0] == v2[0]) && (v1[1] == v2[1]) && (v1[2] == v2[2]));
}

COIN_DLL_API inline int operator != (const SbVec3f & v1, const SbVec3f & v2) {
  return !(v1 == v2);
}

#endif // !COIN_SBVEC3F_H
