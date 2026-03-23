#ifndef COIN_SBBOX2F_H
#define COIN_SBBOX2F_H

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

#include <Inventor/SbVec2f.h>

class SbBox2d;
class SbBox2s;
class SbBox2i32;

class COIN_DLL_API SbBox2f {
public:
  SbBox2f(void) { makeEmpty(); }
  SbBox2f(float xmin, float ymin, float xmax, float ymax)
    : minpt(xmin, ymin), maxpt(xmax, ymax) { }
  SbBox2f(const SbVec2f & minpoint, const SbVec2f & maxpoint)
    : minpt(minpoint), maxpt(maxpoint) { }
  explicit SbBox2f(const SbBox2d & box) { setBounds(box); }
  explicit SbBox2f(const SbBox2s & box) { setBounds(box); }
  explicit SbBox2f(const SbBox2i32 & box) { setBounds(box); }

  SbBox2f & setBounds(float xmin, float ymin, float xmax, float ymax)
    { minpt.setValue(xmin, ymin); maxpt.setValue(xmax, ymax); return *this; }
  SbBox2f & setBounds(const SbVec2f & minpoint, const SbVec2f & maxpoint)
    { minpt = minpoint; maxpt = maxpoint; return *this; }
  SbBox2f & setBounds(const SbBox2d & box);
  SbBox2f & setBounds(const SbBox2s & box);
  SbBox2f & setBounds(const SbBox2i32 & box);

  void getBounds(float & xmin, float & ymin, float & xmax, float & ymax) const
    { minpt.getValue(xmin, ymin); maxpt.getValue(xmax, ymax); }
  void getBounds(SbVec2f & minpoint, SbVec2f & maxpoint) const
    { minpoint = minpt; maxpoint = maxpt; }

  const SbVec2f & getMin(void) const { return minpt; }
  SbVec2f & getMin(void) { return minpt; }

  const SbVec2f & getMax(void) const { return maxpt; }
  SbVec2f & getMax(void) { return maxpt; }

  void extendBy(const SbVec2f & point);
  void extendBy(const SbBox2f & box);
  void makeEmpty(void);
  SbBool isEmpty(void) const { return (maxpt[0] < minpt[0]); }
  SbBool hasArea(void) const { return ((maxpt[0] > minpt[0]) && (maxpt[1] > minpt[1])); }

  SbBool intersect(const SbVec2f & point) const;
  SbBool intersect(const SbBox2f & box) const;
  SbVec2f getClosestPoint(const SbVec2f & point) const;

  SbVec2f getCenter(void) const { return (minpt + maxpt) * 0.5f; }
  void getOrigin(float & originX, float & originY) const
    { minpt.getValue(originX, originY); }
  void getSize(float & sizeX, float & sizeY) const
    { if (isEmpty()) { sizeX = sizeY = 0.0f; }
      else { sizeX = maxpt[0] - minpt[0]; sizeY = maxpt[1] - minpt[1]; } }
  SbVec2f getSize(void) const {
    SbVec2f v;
    this->getSize(v[0], v[1]);
    return v;
  }

  float getAspectRatio(void) const
    { SbDividerChk("SbBox2f::getAspectRatio()", maxpt[1] - minpt[1]);
      return (maxpt[0] - minpt[0]) / (maxpt[1] - minpt[1]); }

protected:
  SbVec2f minpt, maxpt;

}; // SbBox2f

COIN_DLL_API inline int operator == (const SbBox2f & b1, const SbBox2f & b2) {
  return ((b1.getMin() == b2.getMin()) && (b1.getMax() == b2.getMax()));
}

COIN_DLL_API inline int operator != (const SbBox2f & b1, const SbBox2f & b2) {
  return !(b1 == b2);
}

#endif // !COIN_SBBOX2F_H
