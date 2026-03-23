#ifndef COIN_SBXFBOX3D_H
#define COIN_SBXFBOX3D_H

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

#include <Inventor/SbBox3d.h>
#include <Inventor/SbDPMatrix.h>

class COIN_DLL_API SbXfBox3d : public SbBox3d {
  typedef SbBox3d inherited;

public:
  SbXfBox3d(void);
  SbXfBox3d(const SbVec3d & boxmin, const SbVec3d & boxmax);
  SbXfBox3d(const SbBox3d & box);

  void setTransform(const SbDPMatrix & matrix);

  const SbDPMatrix & getTransform(void) const { return matrix; }
  const SbDPMatrix & getInverse(void) const;
  SbVec3d getCenter(void) const;

  void extendBy(const SbVec3d & pt);
  void extendBy(const SbBox3d & bb);
  void extendBy(const SbXfBox3d & bb);
  SbBool intersect(const SbVec3d & pt) const;
  SbBool intersect(const SbBox3d & bb) const;
  SbBool intersect(const SbXfBox3d & bb) const;
  SbBox3d project(void) const;
  void getSpan(const SbVec3d & direction, double & dMin, double & dMax) const;

  // Must override the transform() method from SbBox3f, as the box and
  // the transform matrix are supposed to be kept separate in
  // SbXfBox3f. --mortene
  void transform(const SbDPMatrix & matrix);

  // Overridden from SbBox3d
  double getVolume(void) const;

private:
  // These are incorrect for SbXfBox3d. Privatize/hide them.
  using SbBox3d::getMin;
  using SbBox3d::getMax;

  void calcInverse(void) const;
  void makeInvInvalid(void);

  SbDPMatrix matrix;
  mutable SbDPMatrix invertedmatrix; // lazy cache

}; // SbXfBox3d

COIN_DLL_API int operator == (const SbXfBox3d & b1, const SbXfBox3d & b2);
COIN_DLL_API int operator != (const SbXfBox3d & b1, const SbXfBox3d & b2);

#endif // !COIN_SBXFBOX3D_H
