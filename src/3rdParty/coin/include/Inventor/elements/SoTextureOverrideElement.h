#ifndef COIN_SOTEXTUREOVERRIDEELEMENT_H
#define COIN_SOTEXTUREOVERRIDEELEMENT_H

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

class COIN_DLL_API SoTextureOverrideElement : public SoElement {
  typedef SoElement inherited;

  SO_ELEMENT_HEADER(SoTextureOverrideElement);

public:
  static void initClass(void);
protected:
  virtual ~SoTextureOverrideElement();

  enum {
    TEXTURE_QUALITY = 0x1,
    TEXTURE_IMAGE   = 0x2,
    BUMP_MAP        = 0x4
  };
public:
  virtual SbBool matches(const SoElement *element) const;
  virtual SoElement *copyMatchInfo() const;

  virtual void init(SoState *state);
  virtual void push(SoState *state);
  static SbBool getQualityOverride(SoState *state);
  static SbBool getImageOverride(SoState *state);
  static SbBool getBumpMapOverride(SoState * state);

  static void setQualityOverride(SoState *state, const SbBool value);
  static void setImageOverride(SoState *state, const SbBool value);
  static void setBumpMapOverride(SoState *state, const SbBool value);

  virtual void print(FILE *fp) const;

private:
  uint32_t flags;
};

#endif // !COIN_SOTEXTUREOVERRIDEELEMENT_H
