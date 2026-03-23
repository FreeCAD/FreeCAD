#ifndef COIN_SOMATERIALBINDING_H
#define COIN_SOMATERIALBINDING_H

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

#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/nodes/SoSubNode.h>

class COIN_DLL_API SoMaterialBinding : public SoNode {
  typedef SoNode inherited;

  SO_NODE_HEADER(SoMaterialBinding);

public:
  static void initClass(void);
  SoMaterialBinding(void);

  enum Binding {
    OVERALL = SoMaterialBindingElement::OVERALL,
    PER_PART = SoMaterialBindingElement::PER_PART,
    PER_PART_INDEXED = SoMaterialBindingElement::PER_PART_INDEXED,
    PER_FACE = SoMaterialBindingElement::PER_FACE,
    PER_FACE_INDEXED = SoMaterialBindingElement::PER_FACE_INDEXED,
    PER_VERTEX = SoMaterialBindingElement::PER_VERTEX,
    PER_VERTEX_INDEXED = SoMaterialBindingElement::PER_VERTEX_INDEXED,

    // Note: DEFAULT and NONE bindings are obsolete, but we include them
    // to be compatible with old Open Inventor code.
    DEFAULT = OVERALL,
    NONE = OVERALL
  };

  SoSFEnum value;

  virtual void doAction(SoAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void pick(SoPickAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);

protected:
  virtual ~SoMaterialBinding();
};

#endif // !COIN_SOMATERIALBINDING_H
