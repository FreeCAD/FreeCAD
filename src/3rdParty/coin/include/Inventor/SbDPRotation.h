#ifndef COIN_SBDPROTATION_H
#define COIN_SBDPROTATION_H

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
#include <Inventor/SbVec4d.h>

class SbDPMatrix;
class SbVec3d;

class COIN_DLL_API SbDPRotation {
public:
  SbDPRotation(void);
  SbDPRotation(const SbVec3d & axis, const double radians);
  SbDPRotation(const double q[4]);
  SbDPRotation(const double q0, const double q1, const double q2, const double q3);
  SbDPRotation(const SbDPMatrix & m);
  SbDPRotation(const SbVec3d & rotateFrom, const SbVec3d & rotateTo);
  const double * getValue(void) const;
  void getValue(double & q0, double & q1, double & q2, double & q3) const;
  SbDPRotation & setValue(const double q0, const double q1,
                        const double q2, const double q3);
  void getValue(SbVec3d & axis, double & radians) const;
  void getValue(SbDPMatrix & matrix) const;
  SbDPRotation & invert(void);
  SbDPRotation inverse(void) const;
  SbDPRotation & setValue(const double q[4]);
  SbDPRotation & setValue(const SbDPMatrix & m);
  SbDPRotation & setValue(const SbVec3d & axis, const double radians);
  SbDPRotation & setValue(const SbVec3d & rotateFrom, const SbVec3d & rotateTo);
  SbBool equals(const SbDPRotation & r, double tolerance) const;
  void multVec(const SbVec3d & src, SbVec3d & dst) const;

  void scaleAngle(const double scaleFactor);
  static SbDPRotation slerp(const SbDPRotation & rot0, const SbDPRotation & rot1,
                          double t);
  static SbDPRotation identity(void);

  void print(FILE * fp) const;

  SbDPRotation & operator*=(const SbDPRotation & q);
  SbDPRotation & operator*=(const double s);
  friend COIN_DLL_API int operator==(const SbDPRotation & q1, const SbDPRotation & q2);
  friend COIN_DLL_API int operator!=(const SbDPRotation & q1, const SbDPRotation & q2);
  friend COIN_DLL_API SbDPRotation operator *(const SbDPRotation & q1, const SbDPRotation & q2);
private:
  SbVec4d quat;
};

typedef SbDPRotation SbRotationd;


COIN_DLL_API int operator ==(const SbDPRotation & q1, const SbDPRotation & q2);
COIN_DLL_API int operator !=(const SbDPRotation & q1, const SbDPRotation & q2);
COIN_DLL_API SbDPRotation operator *(const SbDPRotation & q1, const SbDPRotation & q2);

#endif // !COIN_SBDPROTATION_H
