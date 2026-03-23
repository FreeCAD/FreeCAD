#ifndef COIN_SBXFBOX3F_H
#define COIN_SBXFBOX3F_H

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
#include <Inventor/SbBox3f.h>
#include <Inventor/SbMatrix.h>

class COIN_DLL_API SbXfBox3f : public SbBox3f {
  typedef SbBox3f inherited;

public:
  SbXfBox3f(void);
  SbXfBox3f(const SbVec3f & boxmin, const SbVec3f & boxmax);
  SbXfBox3f(const SbBox3f & box);
  ~SbXfBox3f();

  void setTransform(const SbMatrix & m);
  const SbMatrix & getTransform(void) const;
  const SbMatrix & getInverse(void) const;
  SbVec3f getCenter(void) const;
  void extendBy(const SbVec3f & pt);
  void extendBy(const SbBox3f & bb);
  void extendBy(const SbXfBox3f & bb);
  SbBool intersect(const SbVec3f & pt) const;
  SbBool intersect(const SbBox3f & bb) const;
  SbBool intersect(const SbXfBox3f & bb) const;
  void getSpan(const SbVec3f & direction, float & dMin, float & dMax) const;
  SbBox3f project(void) const;
  friend COIN_DLL_API int operator ==(const SbXfBox3f & b1, const SbXfBox3f & b2);
  friend COIN_DLL_API int operator !=(const SbXfBox3f & b1, const SbXfBox3f & b2);
  // Must override the transform() method from SbBox3f, as the box and
  // the transform matrix are supposed to be kept separate in
  // SbXfBox3f. --mortene
  void transform(const SbMatrix & m);
  // Overridden from SbBox3f
  float getVolume(void) const;

  void print(FILE * file) const;

private:
  // incorrect for SbXfBox3f. Hide them
  const SbVec3f & getMin(void) const {return SbBox3f::getMin(); }
  const SbVec3f & getMax(void) const { return SbBox3f::getMax(); }

  void calcInverse(void) const;
  void makeInvInvalid(void);

  SbMatrix matrix, invertedmatrix;
};

COIN_DLL_API int operator ==(const SbXfBox3f & b1, const SbXfBox3f & b2);
COIN_DLL_API int operator !=(const SbXfBox3f & b1, const SbXfBox3f & b2);

#endif // !COIN_SBXFBOX3F_H
