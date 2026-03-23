#ifndef COIN_SOSHAPESTYLEELEMENT_H
#define COIN_SOSHAPESTYLEELEMENT_H

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

#include <Inventor/elements/SoSubElement.h>

class COIN_DLL_API SoShapeStyleElement : public SoElement {
  typedef SoElement inherited;

  SO_ELEMENT_HEADER(SoShapeStyleElement);
public:
  static void initClass(void);
protected:
  virtual ~SoShapeStyleElement();

public:

  // flags used for optimized testing of features
  enum Flags {
    LIGHTING                = 0x00000100,
    TEXENABLED              = 0x00000200,
    TEXFUNC                 = 0x00000400,
    BBOXCMPLX               = 0x00000800,
    INVISIBLE               = 0x00001000,
    ABORTCB                 = 0x00002000,
    OVERRIDE                = 0x00004000,
    TEX3ENABLED             = 0x00008000,
    BIGIMAGE                = 0x00010000,
    BUMPMAP                 = 0x00020000,
    VERTEXARRAY             = 0x00040000,
    TRANSP_TEXTURE          = 0x00080000,
    TRANSP_MATERIAL         = 0x00100000,
    TRANSP_SORTED_TRIANGLES = 0x00200000,
    SHADOWMAP               = 0x00400000,
    SHADOWS                 = 0x00800000
  };
  
  virtual void init(SoState * state);

  virtual void push(SoState * state);
  virtual void pop(SoState * state, const SoElement * prevTopElement);

  virtual SbBool matches(const SoElement * element) const;
  virtual SoElement * copyMatchInfo(void) const;

  static const SoShapeStyleElement * get(SoState * const state);

  unsigned int getFlags(void) const;
  SbBool mightNotRender(void) const;
  SbBool needNormals(void) const;
  SbBool needTexCoords(void) const;
  int getRenderCaseMask(void) const;

  static void setDrawStyle(SoState * const state, const int32_t value);
  static void setComplexityType(SoState * const state,
                                const int32_t value);
  static void setTransparencyType(SoState * const state,
                                  const int32_t value);
  static void setTextureEnabled(SoState * const state, const SbBool value);
  static void setTexture3Enabled(SoState * const state, const SbBool value);
  static void setTextureFunction(SoState * const state,
                                 const SbBool value);
  static void setLightModel(SoState * const state, const int32_t value);
  static void setOverrides(SoState * const state, const SbBool value);

  static SbBool isScreenDoor(SoState * const state);
  static int getTransparencyType(SoState * const state);
  SbBool isTextureFunction(void) const;

  static void setBumpmapEnabled(SoState * state, const SbBool value);
  static void setBigImageEnabled(SoState * state, const SbBool value);
  static void setVertexArrayRendering(SoState * state, const SbBool value);

  static void setTransparentMaterial(SoState * state, const SbBool value);
  static void setTransparentTexture(SoState * state, const SbBool value);

  static void setShadowMapRendering(SoState * state, const SbBool value);
  static void setShadowsRendering(SoState * state, const SbBool value);

private:

  static SoShapeStyleElement * getElement(SoState * const state);
  static const SoShapeStyleElement * getConstElement(SoState * const state);

  unsigned int flags;
};

#endif // !COIN_SOSHAPESTYLEELEMENT_H
