#ifndef COIN_SBTRI3F_H
#define COIN_SBTRI3F_H

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

#include <Inventor/SbVec3f.h>
#include <Inventor/SbBox3f.h>

class SbTri3fP;

class SbTri3f {
public:
  SbTri3f(void);
  SbTri3f(const SbTri3f & t);
  SbTri3f(const SbVec3f & a, const SbVec3f & b, const SbVec3f & c);
  ~SbTri3f(void);

  SbTri3f & setValue(const SbTri3f & t);
  SbTri3f & setValue(const SbVec3f & a, const SbVec3f & b, const SbVec3f & c);
  void getValue(SbTri3f & t) const;
  void getValue(SbVec3f & a, SbVec3f & b, SbVec3f & c) const;
  SbVec3f getNormal() const;
  float getDistance(const SbTri3f & t) const;
  static float getDistance(const SbVec3f & p, 
                           const SbVec3f & p1, const SbVec3f & p2);
  float getDistance(const SbVec3f & p) const;
  float getDistance(const SbVec3f & p1, const SbVec3f & p2) const;
  static float sqrDistance(const SbVec3f & a1, const SbVec3f & a2,
                           const SbVec3f & b1, const SbVec3f & b2,
                           float * linP0, float * linP1);
  float sqrDistance(const SbVec3f & p1, 
                    float * pfSParam = NULL, float * pfTParam = NULL) const;

  SbTri3f & operator = (const SbTri3f & t);

  SbBool intersect(const SbTri3f & triangle) const;
  SbBool intersect(const SbTri3f & triangle, float epsilon) const;

  const SbBox3f getBoundingBox(void) const;

private:
  // FIXME: get rid of this -- a triangle is too simple a class to
  // have a private implementation. 20030328 mortene.
  SbTri3fP * const pimpl;
};

#endif // !COIN_SBTRI3F_H
