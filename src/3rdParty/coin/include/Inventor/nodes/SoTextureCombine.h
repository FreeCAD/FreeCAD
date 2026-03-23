#ifndef COIN_SOTEXTURECOMBINE_H
#define COIN_SOTEXTURECOMBINE_H

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
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoMFEnum.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFVec4f.h>
#include <Inventor/elements/SoTextureCombineElement.h>

class COIN_DLL_API SoTextureCombine : public SoNode {
  typedef SoNode inherited;

  SO_NODE_HEADER(SoTextureCombine);

public:
  static void initClass(void);
  SoTextureCombine(void);

  enum Source {
    PRIMARY_COLOR = SoTextureCombineElement::PRIMARY_COLOR,
    TEXTURE = SoTextureCombineElement::TEXTURE,
    CONSTANT = SoTextureCombineElement::CONSTANT,
    PREVIOUS = SoTextureCombineElement::PREVIOUS
  };
  enum Operand {
    SRC_COLOR = SoTextureCombineElement::SRC_COLOR,
    ONE_MINUS_SRC_COLOR = SoTextureCombineElement::ONE_MINUS_SRC_COLOR,
    SRC_ALPHA = SoTextureCombineElement::SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA = SoTextureCombineElement::ONE_MINUS_SRC_ALPHA
  };
  enum Operation {
    REPLACE = SoTextureCombineElement::REPLACE,
    MODULATE = SoTextureCombineElement::MODULATE,
    ADD = SoTextureCombineElement::ADD,
    ADD_SIGNED = SoTextureCombineElement::ADD_SIGNED,
    SUBTRACT = SoTextureCombineElement::SUBTRACT,
    INTERPOLATE = SoTextureCombineElement::INTERPOLATE,
    DOT3_RGB = SoTextureCombineElement::DOT3_RGB,
    DOT3_RGBA = SoTextureCombineElement::DOT3_RGBA
  };
  
  SoMFEnum rgbSource;
  SoMFEnum alphaSource;

  SoMFEnum rgbOperand;
  SoMFEnum alphaOperand;

  SoSFEnum rgbOperation;
  SoSFEnum alphaOperation;
  
  SoSFFloat rgbScale;
  SoSFFloat alphaScale;

  SoSFVec4f constantColor;

  virtual void doAction(SoAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void pick(SoPickAction * action);

protected:
  virtual ~SoTextureCombine();
};

#endif // !COIN_SOTEXTURECOMBINE_H
