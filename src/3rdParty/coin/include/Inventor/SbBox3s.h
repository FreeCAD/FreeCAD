#ifndef COIN_SBBOX3S_H
#define COIN_SBBOX3S_H

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

#include <Inventor/SbVec3s.h>
#include <Inventor/SbVec3f.h>

class SbBox3i32;
class SbBox3f;
class SbBox3d;

class COIN_DLL_API SbBox3s {
public:
  SbBox3s(void) { makeEmpty(); }
  SbBox3s(short xmin, short ymin, short zmin, short xmax, short ymax, short zmax)
    : minpt(xmin, ymin, zmin), maxpt(xmax, ymax, zmax) { }
  SbBox3s(const SbVec3s & minpoint, const SbVec3s & maxpoint)
    : minpt(minpoint), maxpt(maxpoint) { }
  explicit SbBox3s(const SbBox3i32 & box) { setBounds(box); }
  explicit SbBox3s(const SbBox3f & box) { setBounds(box); }
  explicit SbBox3s(const SbBox3d & box) { setBounds(box); }

  SbBox3s & setBounds(short xmin, short ymin, short zmin, short xmax, short ymax, short zmax)
    { minpt.setValue(xmin, ymin, zmin); maxpt.setValue(xmax, ymax, zmax); return *this; }
  SbBox3s & setBounds(const SbVec3s & minpoint, const SbVec3s & maxpoint)
    { minpt = minpoint; maxpt = maxpoint; return *this; }
  SbBox3s & setBounds(const SbBox3i32 & box);
  SbBox3s & setBounds(const SbBox3f & box);
  SbBox3s & setBounds(const SbBox3d & box);

  void getBounds(short & xmin, short & ymin, short & zmin,
                 short & xmax, short & ymax, short & zmax) const
    { minpt.getValue(xmin, ymin, zmin); maxpt.getValue(xmax, ymax, zmax); }
  void getBounds(SbVec3s & minpoint, SbVec3s & maxpoint) const
    { minpoint = minpt; maxpoint = maxpt; }

  const SbVec3s & getMin(void) const { return minpt; }
  SbVec3s & getMin(void) { return minpt; }
  const SbVec3s & getMax(void) const { return maxpt; }
  SbVec3s & getMax(void) { return maxpt; }

  void extendBy(const SbVec3s & pt);
  void extendBy(const SbBox3s & box);
  void makeEmpty(void);
  SbBool isEmpty(void) const { return (maxpt[0] < minpt[0]); }
  SbBool hasVolume(void) const
    { return ((maxpt[0] > minpt[0]) && (maxpt[1] > minpt[1]) && (maxpt[2] > minpt[2])); }
  int getVolume(void) const
    { short dx = 0, dy = 0, dz = 0; getSize(dx, dy, dz); return (dx * dy * dz); }

  SbBool intersect(const SbVec3s & pt) const;
  SbBool intersect(const SbBox3s & box) const;
  SbVec3f getClosestPoint(const SbVec3f & pt) const;

  SbVec3f getCenter(void) const
    { return SbVec3f((minpt[0]+maxpt[0])*0.5f, (minpt[1]+maxpt[1])*0.5f, (minpt[2]+maxpt[2])*0.5f); }
  void getOrigin(short & originX, short & originY, short & originZ) const
    { minpt.getValue(originX, originY, originZ); }
  void getSize(short & sizeX, short & sizeY, short & sizeZ) const
    { if (isEmpty()) { sizeX = sizeY = sizeZ = 0; }
      else { sizeX = maxpt[0] - minpt[0]; sizeY = maxpt[1] - minpt[1]; sizeZ = maxpt[2] - minpt[2]; } }
  SbVec3s getSize(void) const {
    SbVec3s v;
    this->getSize(v[0], v[1],v[2]);
    return v;
  }
  
protected:
  SbVec3s minpt, maxpt;

}; // SbBox3s

COIN_DLL_API inline int operator == (const SbBox3s & b1, const SbBox3s & b2) {
  return ((b1.getMin() == b2.getMin()) && (b1.getMax() == b2.getMax()));
}

COIN_DLL_API inline int operator != (const SbBox3s & b1, const SbBox3s & b2) {
  return !(b1 == b2);
}

#endif // !COIN_SBBOX3S_H
