#ifndef COIN_SOGLMULTITEXTUREIMAGEELEMENT_H
#define COIN_SOGLMULTITEXTUREIMAGEELEMENT_H

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

#include <Inventor/elements/SoMultiTextureImageElement.h>
#include <cstdlib>

class SoGLImage;
class SoGLDisplayList;
class SoGLMultiTextureImageElementP;

class COIN_DLL_API SoGLMultiTextureImageElement : public SoMultiTextureImageElement {
  typedef SoMultiTextureImageElement inherited;

  SO_ELEMENT_HEADER(SoGLMultiTextureImageElement);
public:
  static void initClass(void);
protected:
  virtual ~SoGLMultiTextureImageElement();

public:
  virtual void init(SoState * state);
  virtual void push(SoState * state);
  virtual void pop(SoState * state,
                   const SoElement * prevTopElement);

  static void set(SoState * const state, SoNode * const node,
                  const int unit,
                  SoGLImage * image, const Model model,
                  const SbColor & blendColor);

  static void restore(SoState * state, const int unit);

  static SoGLImage * get(SoState * state,
                         const int unit,
                         Model & model,
                         SbColor & blendcolor);

  class GLUnitData {
  public:
  GLUnitData() : glimage(NULL) {}
  GLUnitData(const GLUnitData & org) : glimage(org.glimage) {}
    SoGLImage * glimage;
  };
  
  static SbBool hasTransparency(SoState * state);
  
 protected:
  virtual SbBool hasTransparency(const int unit = 0) const;
  
private:
  void updateGL(const int unit);
  SoGLMultiTextureImageElementP * pimpl;

 public: // Coin-3 support
  
  static void set(SoState * const state, SoNode * const node,
                  SoGLImage * image, const Model model,
                  const SbColor & blendColor) {
    set(state, node, 0, image, model, blendColor);
  }

  static SoGLImage * get(SoState * state, Model & model,
                         SbColor & blendcolor) {
    return get(state, 0, model, blendcolor);
  }
  static int32_t getMaxGLTextureSize(void);
};

#endif // !COIN_SOGLMULTITEXTUREIMAGEELEMENT_H
