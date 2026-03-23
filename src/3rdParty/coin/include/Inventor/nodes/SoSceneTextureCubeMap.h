#ifndef COIN_SOSCENETEXTURECUBEMAP_H
#define COIN_SOSCENETEXTURECUBEMAP_H

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
#include <Inventor/fields/SoSFVec2s.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/fields/SoMFNode.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/elements/SoMultiTextureImageElement.h>

class SoSensor;
class SoSceneTextureCubeMapP;

class COIN_DLL_API SoSceneTextureCubeMap : public SoNode {
  typedef SoNode inherited;

  SO_NODE_HEADER(SoSceneTextureCubeMap);

public:
  static void initClass(void);
  SoSceneTextureCubeMap(void);

  enum Model {
    MODULATE = SoMultiTextureImageElement::MODULATE,
    DECAL = SoMultiTextureImageElement::DECAL,
    BLEND = SoMultiTextureImageElement::BLEND,
    REPLACE = SoMultiTextureImageElement::REPLACE
  };

  enum Wrap {
    REPEAT = SoMultiTextureImageElement::REPEAT,
    CLAMP = SoMultiTextureImageElement::CLAMP
  };

  enum TransparencyFunction {
    NONE,
    ALPHA_BLEND,
    ALPHA_TEST
  };

  SoSFVec2s size;
  SoSFNode scene;

  SoSFEnum wrapS;
  SoSFEnum wrapT;
  SoSFEnum wrapR;
  SoSFEnum model;
  SoSFColor backgroundColor;

  SoSFEnum transparencyFunction;
  SoSFColor blendColor;

  virtual void doAction(SoAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void rayPick(SoRayPickAction * action);

protected:
  virtual ~SoSceneTextureCubeMap();

  virtual void notify(SoNotList * list);

private:
  SoSceneTextureCubeMapP * pimpl;
};

#endif // !COIN_SOSCENETEXTURECUBEMAP_H
