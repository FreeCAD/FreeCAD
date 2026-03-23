#ifndef COIN_SBBOX2I32_H
#define COIN_SBBOX2I32_H

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

#include <Inventor/SbVec2i32.h>
#include <Inventor/SbVec2f.h>

class SbBox2s;
class SbBox2f;
class SbBox2d;

class COIN_DLL_API SbBox2i32 {
public:
  SbBox2i32(void) { makeEmpty(); }
  SbBox2i32(int32_t xmin, int32_t ymin, int32_t xmax, int32_t ymax)
    : minpt(xmin, ymin), maxpt(xmax, ymax) { }
  SbBox2i32(const SbVec2i32 & minpoint, const SbVec2i32 & maxpoint)
    : minpt(minpoint), maxpt(maxpoint) { }
  explicit SbBox2i32(const SbBox2s & box) { setBounds(box); }
  explicit SbBox2i32(const SbBox2f & box) { setBounds(box); }
  explicit SbBox2i32(const SbBox2d & box) { setBounds(box); }

  SbBox2i32 & setBounds(int32_t xmin, int32_t ymin, int32_t xmax, int32_t ymax)
    { minpt.setValue(xmin, ymin); maxpt.setValue(xmax, ymax); return *this; }
  SbBox2i32 & setBounds(const SbVec2i32 & minpoint, const SbVec2i32 & maxpoint)
    { minpt = minpoint; maxpt = maxpoint; return *this; }
  SbBox2i32 & setBounds(const SbBox2s & box);
  SbBox2i32 & setBounds(const SbBox2f & box);
  SbBox2i32 & setBounds(const SbBox2d & box);

  void getBounds(int32_t & xmin, int32_t & ymin, int32_t & xmax, int32_t & ymax) const
    { minpt.getValue(xmin, ymin); maxpt.getValue(xmax, ymax); }
  void getBounds(SbVec2i32 & minpoint, SbVec2i32 & maxpoint) const
    { minpoint = minpt; maxpoint = maxpt; }

  const SbVec2i32 & getMin(void) const { return minpt; }
  SbVec2i32 & getMin(void) { return minpt; }
  const SbVec2i32 & getMax(void) const { return maxpt; }
  SbVec2i32 & getMax(void) { return maxpt; }

  void extendBy(const SbVec2i32 & point);
  void extendBy(const SbBox2i32 & box);
  void makeEmpty(void);
  SbBool isEmpty(void) const { return (maxpt[0] < minpt[0]); }
  SbBool hasArea(void) const { return ((maxpt[0] > minpt[0]) && (maxpt[1] > minpt[1])); }

  SbBool intersect(const SbVec2i32 & point) const;
  SbBool intersect(const SbBox2i32 & box) const;

  SbVec2f getCenter(void) const { return SbVec2f((minpt[0] + maxpt[0]) * 0.5f, (minpt[0] + maxpt[0]) * 0.5f); }
  void getOrigin(int32_t & originX, int32_t & originY) const
    { minpt.getValue(originX, originY); }
  void getSize(int32_t & sizeX, int32_t & sizeY) const
    { if (isEmpty()) { sizeX = sizeY = 0; }
      else { sizeX = maxpt[0] - minpt[0]; sizeY = maxpt[1] - minpt[1]; } }
  SbVec2i32 getSize(void) const {
    SbVec2i32 v;
    this->getSize(v[0], v[1]);
    return v;
  }
  float getAspectRatio(void) const
    { SbDividerChk("SbBox2i32::getAspectRatio()", maxpt[1] - minpt[1]);
      return float(maxpt[0] - minpt[0]) / float(maxpt[1] - minpt[1]); }

private:
  SbVec2i32 minpt, maxpt;

}; // SbBox2i32

COIN_DLL_API inline int operator == (const SbBox2i32 & b1, const SbBox2i32 & b2) {
  return ((b1.getMin() == b2.getMin()) && (b1.getMax() == b2.getMax()));
}

COIN_DLL_API inline int operator != (const SbBox2i32 & b1, const SbBox2i32 & b2) {
  return !(b1 == b2);
}

#endif // !COIN_SBBOX2I32_H
