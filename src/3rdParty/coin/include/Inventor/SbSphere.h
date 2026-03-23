#ifndef COIN_SBSPHERE_H
#define COIN_SBSPHERE_H

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
#include <Inventor/SbVec3f.h>

class SbLine;
class SbBox3f;

class COIN_DLL_API SbSphere {
public:
  SbSphere(void);
  SbSphere(const SbVec3f & center, const float radius);
  void setValue(const SbVec3f & center, const float radius);
  void setCenter(const SbVec3f & center);
  void setRadius(const float radius);
  const SbVec3f &  getCenter(void) const;
  float getRadius(void) const;
  void circumscribe(const SbBox3f & box);
  SbBool intersect(const SbLine & l, SbVec3f & intersection) const;
  SbBool intersect(const SbLine & l, SbVec3f & enter, SbVec3f & exit) const;
  SbBool pointInside(const SbVec3f & p) const;

  void print(FILE * file) const;

private:
  SbVec3f center;
  float radius;
};

#endif // !COIN_SBSPHERE_H
