#ifndef COIN_SOSCXMLROTATETARGET_H
#define COIN_SOSCXMLROTATETARGET_H

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

#include <Inventor/navigation/SoScXMLNavigationTarget.h>

class SbVec3f;
class SbRotation;
class SoCamera;

#define COIN_NAVIGATION_ROTATE_TARGET_EVENT_PREFIX COIN_NAVIGATION_EVENT_PREFIX ".Rotate"

class COIN_DLL_API SoScXMLRotateTarget : public SoScXMLNavigationTarget {
  typedef SoScXMLNavigationTarget inherited;
  SCXML_OBJECT_HEADER(SoScXMLRotateTarget)

public:
  static void initClass(void);
  static void cleanClass(void);

  static SoScXMLRotateTarget * constructSingleton(void);
  static void destructSingleton(void);
  static SoScXMLRotateTarget * singleton(void);

  static const SbName & BEGIN(void);
  static const SbName & UPDATE(void);
  static const SbName & END(void);
  static const SbName & SET_FOCAL_POINT(void);

  //void rotateCamera();
  static void setFocalPoint(SoCamera * camera, const SbVec3f & worldspace);
  static void setFocalPoint(SoCamera * camera, const SbVec3f & worldspace, const SbVec3f & upvector);
  static void reorientCamera(SoCamera * camera, const SbRotation & rot);

protected:
  SoScXMLRotateTarget(void);
  virtual ~SoScXMLRotateTarget(void);

  // sent events
  static const SbName & TRIGGER_SPIN(void);

  virtual SbBool processOneEvent(const ScXMLEvent * event);

private:
  class PImpl;
  static SoScXMLRotateTarget * theSingleton;

}; // SoScXMLRotateTarget

#endif // !COIN_SOSCXMLROTATETARGET_H
