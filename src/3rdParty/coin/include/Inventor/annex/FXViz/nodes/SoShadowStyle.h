#ifndef COIN_SOSHADOWSTYLE_H
#define COIN_SOSHADOWSTYLE_H

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

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/annex/FXViz/elements/SoShadowStyleElement.h>

class COIN_DLL_API SoShadowStyle : public SoNode {
  typedef SoNode inherited;

  SO_NODE_HEADER(SoShadowStyle);

public:
  static void initClass(void);
  SoShadowStyle(void);

  enum Style {
    NO_SHADOWING = SoShadowStyleElement::NO_SHADOWING, 
    CASTS_SHADOW = SoShadowStyleElement::CASTS_SHADOW, 
    SHADOWED = SoShadowStyleElement::SHADOWED, 
    CASTS_SHADOW_AND_SHADOWED = SoShadowStyleElement::CASTS_SHADOW_AND_SHADOWED 
  };
  
  SoSFEnum style;

  virtual void GLRender(SoGLRenderAction * action);

protected:
  virtual ~SoShadowStyle();
};

#endif // !COIN_SOSHADOWSTYLE_H
