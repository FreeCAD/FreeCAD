#ifndef COIN_SBBOX3I32_H
#define COIN_SBBOX3I32_H

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

#include <Inventor/SbVec3i32.h>
#include <Inventor/SbVec3f.h>

class SbBox3s;
class SbBox3f;
class SbBox3d;
class SbMatrix;

class COIN_DLL_API SbBox3i32 {
public:
  SbBox3i32(void) { makeEmpty(); }
  SbBox3i32(int32_t xmin, int32_t ymin, int32_t zmin, int32_t xmax, int32_t ymax, int32_t zmax)
    : minpt(xmin, ymin, zmin), maxpt(xmax, ymax, zmax) { }
  SbBox3i32(const SbVec3i32 & minpoint, const SbVec3i32 & maxpoint)
    : minpt(minpoint), maxpt(maxpoint) { }
  explicit SbBox3i32(const SbBox3s & box) { setBounds(box); }
  explicit SbBox3i32(const SbBox3f & box) { setBounds(box); }
  explicit SbBox3i32(const SbBox3d & box) { setBounds(box); }

  SbBox3i32 & setBounds(int32_t xmin, int32_t ymin, int32_t zmin, int32_t xmax, int32_t ymax, int32_t zmax)
    { minpt.setValue(xmin, ymin, zmin); maxpt.setValue(xmax, ymax, zmax); return *this; }
  SbBox3i32 & setBounds(const SbVec3i32 & minpoint, const SbVec3i32 & maxpoint)
    { minpt = minpoint; maxpt = maxpoint; return *this; }
  SbBox3i32 & setBounds(const SbBox3s & box);
  SbBox3i32 & setBounds(const SbBox3f & box);
  SbBox3i32 & setBounds(const SbBox3d & box);

  void getBounds(int32_t & xmin, int32_t & ymin, int32_t & zmin, int32_t & xmax, int32_t & ymax, int32_t & zmax) const
    { minpt.getValue(xmin, ymin, zmin); maxpt.getValue(xmax, ymax, zmax); }
  void getBounds(SbVec3i32 & minpoint, SbVec3i32 & maxpoint) const
    { minpoint = minpt; maxpoint = maxpt; }

  const SbVec3i32 & getMin(void) const { return minpt; }
  SbVec3i32 & getMin(void) { return minpt; }
  const SbVec3i32 & getMax(void) const { return maxpt; }
  SbVec3i32 & getMax(void) { return maxpt; }

  void extendBy(const SbVec3i32 & point);
  void extendBy(const SbBox3i32 & box);
  void extendBy(const SbVec3f & point);
  void transform(const SbMatrix & m);
  void makeEmpty(void);
  SbBool isEmpty(void) const { return (maxpt[0] < minpt[0]); }
  SbBool hasVolume(void) const
    { return ((maxpt[0] > minpt[0]) && (maxpt[1] > minpt[1]) && (maxpt[2] > minpt[2])); }
  float getVolume(void) const
    { int32_t dx = 0, dy = 0, dz = 0; getSize(dx, dy, dz); return (float(dx) * float(dy) * float(dz)); }

  SbBool intersect(const SbVec3i32 & point) const;
  SbBool intersect(const SbBox3i32 & box) const;
  SbBool intersect(const SbVec3f & point) const;

  SbBool outside(const SbMatrix & mvp, int & cullBits) const;
  SbVec3f getClosestPoint(const SbVec3f & point) const;

  SbVec3f getCenter(void) const { return SbVec3f(minpt + maxpt) * 0.5f; }
  void getOrigin(int32_t & originX, int32_t & originY, int32_t & originZ) const
    { minpt.getValue(originX, originY, originZ); }
  void getSize(int32_t & sizeX, int32_t & sizeY, int32_t & sizeZ) const
    { if (isEmpty()) { sizeX = sizeY = sizeZ = 0; }
      else { sizeX = maxpt[0] - minpt[0]; sizeY = maxpt[1] - minpt[1]; sizeZ = maxpt[2] - minpt[2]; } }

  SbVec3i32 getSize(void) const {
    SbVec3i32 v;
    this->getSize(v[0], v[1], v[2]);
    return v;
  }

  void getSpan(const SbVec3f & direction, float & dmin, float & dmax) const;

protected:
  SbVec3i32 minpt, maxpt;

}; // SbBox3i32

COIN_DLL_API inline int operator == (const SbBox3i32 & b1, const SbBox3i32 & b2)
{
  return ((b1.getMin() == b2.getMin()) && (b1.getMax() == b2.getMax()));
}

COIN_DLL_API inline int operator != (const SbBox3i32 & b1, const SbBox3i32 & b2)
{
  return !(b1 == b2);
}

#endif // !COIN_SBBOX3I32_H
