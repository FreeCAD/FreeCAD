#ifndef COIN_SOENVIRONMENTELEMENT_H
#define COIN_SOENVIRONMENTELEMENT_H

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

#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/SbColor.h>

class COIN_DLL_API SoEnvironmentElement : public SoReplacedElement {
  typedef SoReplacedElement inherited;

  SO_ELEMENT_HEADER(SoEnvironmentElement);
public:
  static void initClass(void);
protected:
  virtual ~SoEnvironmentElement();

public:

  enum FogType {
    NONE,
    HAZE,
    FOG,
    SMOKE
  };

  virtual void init(SoState * state);

  static  void set(SoState * const state, SoNode * const node,
                   const float ambientIntensity, const SbColor & ambientColor,
                   const SbVec3f & attenuation, const int32_t fogType,
                   const SbColor & fogColor, const float fogVisibility,
                   const float fogStart = 0.0f);
  static  void get(SoState * const state,
                   float & ambientIntensity, SbColor & ambientColor,
                   SbVec3f & attenuation, int32_t & fogType,
                   SbColor & fogColor, float & fogVisibility,
                   float & fogStart);

  static float getAmbientIntensity(SoState * const state);
  static float getFogVisibility(SoState * const state);
  static const SbVec3f & getLightAttenuation(SoState * const state);
  static const SbColor & getAmbientColor(SoState * const state);
  static const SbColor & getFogColor(SoState * const state);
  static int32_t getFogType(SoState * const state);

  static void getDefault(float & ambientIntensity,
                         SbColor & ambientColor, SbVec3f & attenuation,
                         int32_t & fogType, SbColor & fogColor,
                         float & fogVisibility, float & fogNear);

  virtual void print(FILE * file) const;

protected:

  virtual void setElt(SoState * const state,
                      const float ambientIntensity,
                      const SbColor & ambientColor,
                      const SbVec3f & attenuation,
                      const int32_t fogType,
                      const SbColor & fogColor,
                      const float fogVisibility,
                      const float fogNear);

  float ambientIntensity;
  SbColor ambientColor;
  SbVec3f attenuation;
  int32_t fogType;
  SbColor fogColor;
  float fogVisibility;
  float fogStart;
};

#endif // !COIN_SOENVIRONMENTELEMENT_H
