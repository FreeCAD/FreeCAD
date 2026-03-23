#ifndef COIN_SOSCXMLZOOMTARGET_H
#define COIN_SOSCXMLZOOMTARGET_H

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

class SoCamera;
class SoPerspectiveCamera;
class SoOrthographicCamera;
class SoFrustumCamera;

#define COIN_NAVIGATION_ZOOM_TARGET_EVENT_PREFIX SOSCXML_NAVIGATION_TARGETTYPE ".Zoom"

class COIN_DLL_API SoScXMLZoomTarget : public SoScXMLNavigationTarget {
  typedef SoScXMLNavigationTarget inherited;
  SCXML_OBJECT_HEADER(SoScXMLZoomTarget)

public:
  static void initClass(void);
  static void cleanClass(void);

  static SoScXMLZoomTarget * constructSingleton(void);
  static void destructSingleton(void);
  static SoScXMLZoomTarget * singleton(void);

  static const SbName & BEGIN(void);
  static const SbName & UPDATE(void);
  static const SbName & END(void);
  static const SbName & ZOOM(void);
  static const SbName & ZOOM_IN(void);
  static const SbName & ZOOM_OUT(void);
  static const SbName & RESET(void);

  static void zoom(SoCamera * camera, float factor);
  static void reset(SoCamera * camera);

protected:
  SoScXMLZoomTarget(void);
  virtual ~SoScXMLZoomTarget(void);

  virtual SbBool processOneEvent(const ScXMLEvent * event);

private:
  class PImpl;
  static SoScXMLZoomTarget * theSingleton;

}; // SoScXMLZoomTarget

#endif // !COIN_SOSCXMLZOOMTARGET_H
