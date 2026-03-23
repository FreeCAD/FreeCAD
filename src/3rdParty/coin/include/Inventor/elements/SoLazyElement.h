#ifndef COIN_SOLAZYELEMENT_H
#define COIN_SOLAZYELEMENT_H

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
#include <Inventor/SbColor.h>

class SoMFFloat;
class SoMFColor;
class SoColorPacker;
class SoLazyElementP;

#define SO_LAZY_SHINY_THRESHOLD 0.005f


class COIN_DLL_API SoLazyElement : public SoElement {
  typedef SoElement inherited;

  SO_ELEMENT_HEADER(SoLazyElement);

public:
  static void initClass();
protected:
  ~SoLazyElement();
public:
  enum cases {
    LIGHT_MODEL_CASE = 0,
    COLOR_MATERIAL_CASE,
    DIFFUSE_CASE,
    AMBIENT_CASE,
    EMISSIVE_CASE,
    SPECULAR_CASE,
    SHININESS_CASE,
    BLENDING_CASE,
    TRANSPARENCY_CASE,
    VERTEXORDERING_CASE,
    TWOSIDE_CASE,
    CULLING_CASE,
    SHADE_MODEL_CASE,
    ALPHATEST_CASE,
    GLIMAGE_CASE, // OBSOLETED
    LAZYCASES_LAST // must be last
  };
  enum masks{
    LIGHT_MODEL_MASK = 1 << LIGHT_MODEL_CASE,           // 0x0001
    COLOR_MATERIAL_MASK = 1 << COLOR_MATERIAL_CASE,     // 0x0002
    DIFFUSE_MASK = 1 << DIFFUSE_CASE,                   // 0x0004
    AMBIENT_MASK = 1 << AMBIENT_CASE,                   // 0x0008
    EMISSIVE_MASK = 1<<EMISSIVE_CASE,                   // 0x0010
    SPECULAR_MASK = 1 << SPECULAR_CASE,                 // 0x0020
    SHININESS_MASK = 1 << SHININESS_CASE,               // 0x0040
    TRANSPARENCY_MASK = 1 << TRANSPARENCY_CASE,         // 0x0080
    BLENDING_MASK = 1 << BLENDING_CASE,                 // 0x0100
    VERTEXORDERING_MASK = 1 << VERTEXORDERING_CASE,     // 0x0200
    TWOSIDE_MASK = 1 << TWOSIDE_CASE,                   // 0x0400
    CULLING_MASK = 1 << CULLING_CASE,                   // 0x0800
    SHADE_MODEL_MASK = 1 << SHADE_MODEL_CASE,           // 0x1000
    ALPHATEST_MASK = 1 << ALPHATEST_CASE,               // 0x2000
    GLIMAGE_MASK = 1 << GLIMAGE_CASE,                   // obsoleted
    ALL_MASK = (1 << LAZYCASES_LAST)-1
  };

  enum internalMasks{
    OTHER_COLOR_MASK = AMBIENT_MASK|EMISSIVE_MASK|SPECULAR_MASK|SHININESS_MASK,
    ALL_COLOR_MASK = OTHER_COLOR_MASK|DIFFUSE_MASK,
    NO_COLOR_MASK = ALL_MASK & (~ALL_COLOR_MASK),
    ALL_BUT_DIFFUSE_MASK = ALL_MASK &(~ DIFFUSE_MASK),
    DIFFUSE_ONLY_MASK = ALL_MASK &(~ OTHER_COLOR_MASK)
  };

  enum LightModel {
    BASE_COLOR,
    PHONG
  };

  enum VertexOrdering {
    CW,
    CCW
  };

  virtual void init(SoState *state);
  virtual void push(SoState *state);
  virtual SbBool matches(const SoElement *) const;
  virtual SoElement *copyMatchInfo(void) const;

  static void setToDefault(SoState * state);
  static void setDiffuse(SoState * state, SoNode * node, int32_t numcolors,
                         const SbColor * colors, SoColorPacker * packer);
  static void setTransparency(SoState *state, SoNode *node, int32_t numvalues,
                              const float * transparency, SoColorPacker * packer);
  static void setPacked(SoState * state, SoNode * node,
                        int32_t numcolors, const uint32_t * colors,
                        const SbBool packedtransparency = FALSE);
  static void setColorIndices(SoState *state, SoNode *node,
                              int32_t numindices, const int32_t *indices);
  static void setAmbient(SoState *state, const SbColor * color);
  static void setEmissive(SoState *state, const SbColor * color);
  static void setSpecular(SoState *state, const SbColor * color);
  static void setShininess(SoState *state, float value);
  static void setColorMaterial(SoState *state, SbBool value);
  static void enableBlending(SoState *state,  
                             int sfactor, 
                             int dfactor);
  static void enableSeparateBlending(SoState *state,  
                                     int sfactor, 
                                     int dfactor,
                                     int alpha_sfactor,
                                     int alpha_dfactor);

  static void disableBlending(SoState * state);
  static void setLightModel(SoState *state, const int32_t model);
  static void setVertexOrdering(SoState * state, VertexOrdering ordering);
  static void setBackfaceCulling(SoState * state, SbBool onoff);
  static void setTwosideLighting(SoState * state, SbBool onoff);
  static void setShadeModel(SoState * state, SbBool flatshading);
  static void setAlphaTest(SoState * state, int func, float value);

  static const SbColor & getDiffuse(SoState* state, int index);
  static float getTransparency(SoState*, int index);
  static const uint32_t * getPackedColors(SoState*);
  static const int32_t  * getColorIndices(SoState*);
  static int32_t getColorIndex(SoState*, int num);
  static const SbColor & getAmbient(SoState *);
  static const SbColor & getEmissive(SoState *);
  static const SbColor & getSpecular(SoState *);
  static float getShininess(SoState*);
  static SbBool getColorMaterial(SoState*);
  static SbBool getBlending(SoState *, 
                            int & sfactor, int & dfactor);
  static SbBool getAlphaBlending(SoState *, 
                                 int & sfactor, int & dfactor);
  
  static int32_t getLightModel(SoState*);
  static int getAlphaTest(SoState * state, float & value);
  static SbBool getTwoSidedLighting(SoState * state);

  int32_t getNumDiffuse(void) const;
  int32_t getNumTransparencies(void) const;
  int32_t getNumColorIndices(void) const;
  SbBool isPacked(void) const;
  SbBool isTransparent(void) const;
  static SoLazyElement * getInstance(SoState *state);
  static float getDefaultAmbientIntensity(void);

  static SbColor getDefaultDiffuse(void);
  static SbColor getDefaultAmbient(void);
  static SbColor getDefaultSpecular(void);
  static SbColor getDefaultEmissive(void);
  static float getDefaultShininess(void);
  static uint32_t getDefaultPacked(void);
  static float getDefaultTransparency(void);
  static int32_t getDefaultLightModel(void);
  static int32_t getDefaultColorIndex(void);

  static void setMaterials(SoState * state, SoNode *node, uint32_t bitmask,
                           SoColorPacker * cPacker,
                           const SbColor * diffuse,
                           const int numdiffuse,
                           const float * transp,
                           const int numtransp,
                           const SbColor & ambient,
                           const SbColor & emissive,
                           const SbColor & specular,
                           const float shininess,
                           const SbBool istransparent);

  static SoLazyElement * getWInstance(SoState *state);

  const uint32_t * getPackedPointer(void) const;
  const SbColor * getDiffusePointer(void) const;
  const int32_t * getColorIndexPointer(void) const;

  const float * getTransparencyPointer(void) const;
  static void setTransparencyType(SoState * state, int32_t type);

protected:

  struct COIN_DLL_API CoinState {
    SbColor ambient;
    SbColor specular;
    SbColor emissive;
    float shininess;
    SbBool blending;
    int blend_sfactor;
    int blend_dfactor;
    int alpha_blend_sfactor;
    int alpha_blend_dfactor;
    int32_t lightmodel;
    SbBool packeddiffuse;
    int32_t numdiffuse;
    int32_t numtransp;
    const SbColor * diffusearray;
    const uint32_t * packedarray;
    const float * transparray;
    const int32_t * colorindexarray;
    int32_t transptype;
    SbBool istransparent;
    SbUniqueId diffusenodeid;
    SbUniqueId transpnodeid;
    int32_t stipplenum;
    VertexOrdering vertexordering;
    SbBool twoside;
    SbBool culling;
    SbBool flatshading;
    int alphatestfunc;
    float alphatestvalue;
  } coinstate;

protected:
  virtual void lazyDidSet(uint32_t mask);
  virtual void lazyDidntSet(uint32_t mask);

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

private:
  SoLazyElementP * pimpl; // for future use

};

class COIN_DLL_API SoColorPacker {
public:
  SoColorPacker(void);
  ~SoColorPacker();

  uint32_t * getPackedColors(void) const {
    return this->array;
  }
  SbBool diffuseMatch(const SbUniqueId nodeid) const {
    return nodeid == this->diffuseid;
  }
  SbBool transpMatch(const SbUniqueId nodeid) const {
    return nodeid == this->transpid;
  }
  void setNodeIds(const SbUniqueId diffuse, const SbUniqueId transp) {
    this->diffuseid = diffuse;
    this->transpid = transp;
  }
  int32_t getSize(void) const {
    return this->arraysize;
  }
  void reallocate(const int32_t size);

  SbUniqueId getDiffuseId(void) const {
    return this->diffuseid;
  }
  SbUniqueId getTranspId(void) const {
    return this->transpid;
  }
private:
  SbUniqueId transpid;
  SbUniqueId diffuseid;
  uint32_t * array;
  int32_t arraysize;
};

#endif // !COIN_SOLAZYELEMENT_H
