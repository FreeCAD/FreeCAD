#ifndef COIN_SOSCXMLFLIGHTCONTROLTARGET_H
#define COIN_SOSCXMLFLIGHTCONTROLTARGET_H

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


#define COIN_NAVIGATION_FLIGHT_CONTROL_TARGET_EVENT_PREFIX SOSCXML_NAVIGATION_TARGETTYPE ".FlightControl"

class COIN_DLL_API SoScXMLFlightControlTarget : public SoScXMLNavigationTarget {
  typedef SoScXMLNavigationTarget inherited;
  SCXML_OBJECT_HEADER(SoScXMLFlightControlTarget)

public:
  static void initClass(void);
  static void cleanClass(void);

  static SoScXMLFlightControlTarget * constructSingleton(void);
  static void destructSingleton(void);
  static SoScXMLFlightControlTarget * singleton(void);

  static const SbName & PITCH(void);
  static const SbName & YAW(void);
  static const SbName & ROLL(void);
  static const SbName & RESET_ROLL(void);

  static void pitch(SoCamera * camera, float radians);
  static void yaw(SoCamera * camera, float radians);
  static void roll(SoCamera * camera, float radians);

  static void resetRoll(SoCamera * camera, const SbVec3f & upvector);

protected:
  SoScXMLFlightControlTarget(void);
  virtual ~SoScXMLFlightControlTarget(void);

  virtual SbBool processOneEvent(const ScXMLEvent * event);

private:
  class PImpl;
  static SoScXMLFlightControlTarget * theSingleton;

}; // SoScXMLFlightControlTarget

#endif // !COIN_SOSCXMLFLIGHTCONTROLTARGET_H

