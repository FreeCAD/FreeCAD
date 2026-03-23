#ifndef COIN_SOCAMERAUTILS_H
#define COIN_SOCAMERAUTILS_H

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

#include <Inventor/SbBasic.h>

class SoCamera;
class SbVec3f;

class SoCameraManager {
public:
  static SoCameraManager * createFor(SoCamera * camera);

  virtual ~SoCameraManager(void);

  virtual void setZoomLimits(float minzoomvalue = 0.0f, float maxzoomvalue = 0.0f);
  virtual SbBool getZoomLimits(float & minzoomvalue, float & maxzoomvalue) const;

  virtual void setZoomValue(float zoomvalue, SbBool limit = FALSE) = 0;
  virtual void adjustZoomValue(float diffvalue, SbBool limit = TRUE) = 0; // diffvalue=0.0 is identity
  virtual void adjustZoom(float factor, SbBool limit = TRUE) = 0; // factor=1.0 is identity

  virtual float getZoomValue(void) const;

  virtual void setZoomByDollyLimits(float mindollydistance = 0.0f, float maxdollydistance = 0.0f, float unitydistance = 0.0f);
  virtual SbBool getZoomByDollyLimits(float & mindollydistance, float & maxdollydistance, float & unitydistance) const;

  virtual void setZoomValueByDolly(float zoomvalue, SbBool limit = FALSE) = 0;
  virtual void adjustZoomByDollyDistance(float distance, SbBool limit = TRUE) = 0; // distance=0.0 is identity
  virtual void adjustZoomByDolly(float factor, SbBool limit = TRUE) = 0; // factor=1.0 is identity

  virtual float getZoomByDollyValue(void) const;

  SoCamera * getCamera(void) const { return this->camera; }
  SbVec3f getFocalPoint(void) const;

  virtual void copyLimits(const SoCameraManager * other);

protected:
  SoCameraManager(SoCamera * camera);

  SbBool havezoomlimits;
  float minzoom, maxzoom;
  SbBool havezoombydollylimits;
  float mindollydistance, maxdollydistance, unitydistance;

private:
  SoCamera * camera;

}; // SoCameraManager

#endif // !COIN_SOCAMERAUTILS_H
