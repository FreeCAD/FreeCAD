#ifndef COIN_SOMULTITEXTUREENABLEDELEMENT_H
#define COIN_SOMULTITEXTUREENABLEDELEMENT_H

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

#include <Inventor/elements/SoElement.h>
#include <Inventor/elements/SoSubElement.h>

class SoMultiTextureEnabledElementP;

class COIN_DLL_API SoMultiTextureEnabledElement : public SoElement {
  typedef SoElement inherited;

  SO_ELEMENT_HEADER(SoMultiTextureEnabledElement);
public:
  static void initClass(void);
protected:
  virtual ~SoMultiTextureEnabledElement();

public:
  enum Mode {
    DISABLED = 0,
    TEXTURE2D,
    RECTANGLE,
    CUBEMAP,
    TEXTURE3D
  };

  virtual void init(SoState * state);
  static void set(SoState * state, 
                  SoNode * node,
                  const int unit,
                  const SbBool enabled);

  static SbBool get(SoState * state, const int unit = 0);
  virtual void setElt(const int unit, const int mode);

  virtual void push(SoState * state);
  virtual SbBool matches(const SoElement * elem) const;
  SoElement * copyMatchInfo(void) const;

  static const SbBool * getEnabledUnits(SoState * state,
                                        int & lastenabled);
  static const Mode * getActiveUnits(SoState * state,
                                     int & lastenabled);

  static void enableRectangle(SoState * state, SoNode * node, const int unit = 0);
  static void enableCubeMap(SoState * state, SoNode * node, const int unit = 0);
  static void enableTexture3(SoState * state, SoNode * node, const int unit = 0);
  static Mode getMode(SoState * state, const int unit = 0);
  static void disableAll(SoState * state);

  // Coin-3 support
  static void set(SoState * state, 
                  const SbBool enabled) {
    set(state, NULL, 0, enabled);
  }

  static void set(SoState * state, SoNode * node,
                  const SbBool enabled) {
    set(state, node, 0, enabled);
  }
  
protected:
  int getMaxUnits() const;
  SbBool isEnabled(const int unit) const;
  Mode getMode(const int unit) const;

private:
  SoMultiTextureEnabledElementP * pimpl;
};

#endif // !COIN_SOMULTITEXTUREENABLEDELEMENT_H
