#ifndef COIN_SBDPPLANE_H
#define COIN_SBDPPLANE_H

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

#include <Inventor/SbVec3d.h>

class SbDPLine;
class SbDPMatrix;

class COIN_DLL_API SbDPPlane {
public:
  SbDPPlane(void);
  SbDPPlane(const SbVec3d & normal, const double D);
  SbDPPlane(const SbVec3d & p0, const SbVec3d & p1, const SbVec3d & p2);
  SbDPPlane(const SbVec3d & normal, const SbVec3d & point);

  void offset(const double d);
  SbBool intersect(const SbDPLine & l, SbVec3d & intersection) const;
  void transform(const SbDPMatrix & matrix);
  SbBool isInHalfSpace(const SbVec3d & point) const;
  double getDistance(const SbVec3d & point) const;
  const SbVec3d & getNormal(void) const;
  double getDistanceFromOrigin(void) const;
  friend COIN_DLL_API int operator ==(const SbDPPlane & p1, const SbDPPlane & p2);
  friend COIN_DLL_API int operator !=(const SbDPPlane & p1, const SbDPPlane & p2);

  void print(FILE * file) const;

  SbBool intersect(const SbDPPlane & pl, SbDPLine & line) const;

private:
  SbVec3d normal;
  double distance;
};

COIN_DLL_API int operator ==(const SbDPPlane & p1, const SbDPPlane & p2);
COIN_DLL_API int operator !=(const SbDPPlane & p1, const SbDPPlane & p2);

#endif // !COIN_SBPLANE_H
