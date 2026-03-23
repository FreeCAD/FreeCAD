#ifndef COIN_SOGLLAZYELEMENT_H
#define COIN_SOGLLAZYELEMENT_H

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

#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/C/glue/gl.h>

class SoGLRenderCache;
class SoGLLazyElementP;
class SoVBO;

class COIN_DLL_API SoGLLazyElement : public SoLazyElement {
  typedef SoLazyElement inherited;

  SO_ELEMENT_HEADER(SoGLLazyElement);

public:
  static void initClass();
protected:
  ~SoGLLazyElement();
public:

  virtual void init(SoState *state);
  virtual void push(SoState *state);
  virtual void pop(SoState *state, const SoElement * prevtopelement);

  static void sendAllMaterial(SoState * state);
  static void sendNoMaterial(SoState * state);
  static void sendOnlyDiffuseColor(SoState * state);
  static void sendLightModel(SoState * state, const int32_t model);
  static void sendPackedDiffuse(SoState * state, const uint32_t diffuse);
  static void sendFlatshading(SoState * state, const SbBool onoff);
  static void sendVertexOrdering(SoState * state, const VertexOrdering ordering);
  static void sendTwosideLighting(SoState * state, const SbBool onoff);
  static void sendBackfaceCulling(SoState * state, const SbBool onoff);

  void sendDiffuseByIndex(const int index) const;
  static SbBool isColorIndex(SoState *state);
  static SoGLLazyElement * getInstance(const SoState *state);
  void send(const SoState *state, uint32_t mask) const;

  void sendVPPacked(SoState* state, const unsigned char* pcolor);

  void reset(SoState* state, uint32_t bitmask) const;

  struct COIN_DLL_API GLState {
    uint32_t cachebitmask;
    uint32_t diffuse;
    SbColor ambient;
    SbColor emissive;
    SbColor specular;
    float shininess;
    int32_t lightmodel;
    int32_t blending;
    int32_t blend_sfactor;
    int32_t blend_dfactor;
    int32_t alpha_blend_sfactor;
    int32_t alpha_blend_dfactor;
    int32_t stipplenum;
    int32_t vertexordering;
    int32_t culling;
    int32_t twoside;
    int32_t flatshading;
    int32_t alphatestfunc;
    float alphatestvalue;
    SbUniqueId diffusenodeid;
    SbUniqueId transpnodeid;
    uint32_t reserved[4];
  };

  virtual void setDiffuseElt(SoNode*,  int32_t numcolors,
                             const SbColor * colors, SoColorPacker * packer);
  virtual void setPackedElt(SoNode * node, int32_t numcolors,
                            const uint32_t * colors, const SbBool packedtransparency);
  virtual void setColorIndexElt(SoNode * node, int32_t numindices,
                                const int32_t * indices);
  virtual void setTranspElt(SoNode * node, int32_t numtransp,
                            const float * transp, SoColorPacker * packer);

  virtual void setTranspTypeElt(int32_t type);
  virtual void setAmbientElt(const SbColor* color);
  virtual void setEmissiveElt(const SbColor* color);
  virtual void setSpecularElt(const SbColor* color);
  virtual void setShininessElt(float value);
  virtual void setColorMaterialElt(SbBool value);
  virtual void enableBlendingElt(int sfactor, int dfactor, int alpha_sfactor, int alpha_dfactor);
  virtual void disableBlendingElt(void);
  virtual void setLightModelElt(SoState *state, int32_t model);
  virtual void setMaterialElt(SoNode * node, uint32_t bitmask,
                              SoColorPacker * packer,
                              const SbColor * diffuse, const int numdiffuse,
                              const float * transp, const int numtransp,
                              const SbColor & ambient,
                              const SbColor & emissive,
                              const SbColor & specular,
                              const float shininess,
                              const SbBool istransparent);
  virtual void setVertexOrderingElt(VertexOrdering ordering);
  virtual void setBackfaceCullingElt(SbBool onoff);
  virtual void setTwosideLightingElt(SbBool onoff);
  virtual void setShadeModelElt(SbBool flatshading);
  virtual void setAlphaTestElt(int func, float value);

  static void beginCaching(SoState * state,
                           SoGLLazyElement::GLState * prestate,
                           SoGLLazyElement::GLState * poststate);
  static void endCaching(SoState * state);

  static SbBool preCacheCall(const SoState * state, const SoGLLazyElement::GLState * prestate);
  static void postCacheCall(const SoState * state, const SoGLLazyElement::GLState * poststate);

  static void mergeCacheInfo(SoState * state,
                             SoGLLazyElement::GLState * childprestate,
                             SoGLLazyElement::GLState * childpoststate);

  void updateColorVBO(SoVBO * vbo);

protected:
  virtual void lazyDidSet(uint32_t mask);
  virtual void lazyDidntSet(uint32_t mask);

private:
  void sendPackedDiffuse(const uint32_t diffuse) const;
  void sendAmbient(const SbColor & color) const;
  void sendEmissive(const SbColor & color) const;
  void sendSpecular(const SbColor & specular) const;
  void sendShininess(const float shininess) const;
  void sendTransparency(const int stipplenum) const;
  void enableBlending(const int sfactor, const int dfactor) const;
  void enableSeparateBlending(const cc_glglue * glue,
                              const int sfactor, const int dfactor,
                              const int alpha_sfactor, const int alpha_dfactor) const;
  void disableBlending(void) const;

  void sendLightModel(const int32_t model) const;
  void sendFlatshading(const SbBool onoff) const;
  void sendVertexOrdering(const VertexOrdering ordering) const;
  void sendTwosideLighting(const SbBool onoff) const;
  void sendBackfaceCulling(const SbBool onoff) const;
  void sendAlphaTest(int func, float value) const;
  void initGL(void);
  void packColors(SoColorPacker * packer) const;

  mutable uint32_t didsetbitmask;
  mutable uint32_t didntsetbitmask;
  mutable uint32_t cachebitmask;
  mutable uint32_t opencacheflags;

  mutable GLState glstate;
  GLState * postcachestate;
  GLState * precachestate;
  SbBool colorindex;
  mutable SoColorPacker * colorpacker;
  mutable const uint32_t * packedpointer;
  uint32_t transpmask;
  SoState * state;
  SoGLLazyElementP * pimpl; // for future use
};

#endif // !COIN_SOGLLAZYELEMENT_H
