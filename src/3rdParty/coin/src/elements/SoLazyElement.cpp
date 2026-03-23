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

/*!
  \class SoLazyElement Inventor/elements/SoLazyElement.h
  \brief The SoLazyElement class is used to handle material and shape properties.

  \ingroup coin_elements

  So[GL]LazyElement is, as the name implies, an element that is lazy
  about sending things to OpenGL. The changes are not sent to OpenGL
  until SoGLLazyElement::send() is called. This means that you can
  change the state of certain attributes several times, but the state
  will only be sent to OpenGL once.

  When creating a new shape node in Coin, it's a common operation to
  modify the OpenGL diffuse color.  There are several ways you can
  send the color to OpenGL. If you're not going to use the color
  outside your node, you can just as well send it using plain
  OpenGL. You can also set the color in the element, and then force a
  send by using SoGLLazyElement::send(state,
  SoLazyElement::DIFFUSE_MASK).

  However, when creating an extension shape node, it's always
  recommended to create an instance of SoMaterialBundle on the
  stack. If this instance is created after you update SoLazyElement
  with a new color, the new color will be sent to OpenGL when you call
  SoMaterialBundle::sendFirst(). This call will also update all other
  lazy OpenGL state, and it is actually required to either use
  SoMaterialBundle::sendFirst() or call SoGLLazyElement::send(state,
  SoLazyElement::ALL_MASK) when creating a shape node.

  If you decide to send the color to OpenGL using glColor*(), you
  should notify SoGLLazyElement about this by calling
  SoGLLazyElement::reset(state, SoLazyElement::DIFFUSE_MASK). This
  will notify SoGLLazyElement that the current OpenGL diffuse color is
  unknown.
*/

#include "coindefs.h"
#include "tidbitsp.h"
#include "SbBasicP.h"

#include <Inventor/elements/SoLazyElement.h>

#include <cassert>
#include <cstring>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/fields/SoMFColor.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoNode.h>

static SbColor * lazy_defaultdiffuse = NULL;
static float * lazy_defaulttransp = NULL;
static int32_t * lazy_defaultindex = NULL;
static uint32_t * lazy_defaultpacked = NULL;
static SbColor * lazy_unpacked = NULL;

extern "C" {

static void
lazyelement_cleanup(void)
{
  delete lazy_defaultdiffuse;
  delete lazy_defaulttransp;
  delete lazy_defaultindex;
  delete lazy_defaultpacked;
  delete lazy_unpacked;
  lazy_defaultdiffuse = NULL; // Only need to NULL this; see initClass().
}

} // extern "C"

// helper functions to handle default diffuse/transp values
static SbUniqueId
get_diffuse_node_id(SoNode * node, const int numdiffuse,
                    const SbColor * color)
{
  if (numdiffuse == 1 && color[0] == SbColor(0.8f, 0.8f, 0.8f)) return 0;
  return node->getNodeId();
}

static SbUniqueId
get_transp_node_id(SoNode * node, const int numtransp,
                   const float * transp)
{
  if (numtransp == 1 && transp[0] == 0.0f) return 0;
  return node->getNodeId();
}


SO_ELEMENT_SOURCE(SoLazyElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoLazyElement::initClass()
{
  SO_ELEMENT_INIT_CLASS(SoLazyElement, inherited);

  if (lazy_defaultdiffuse == NULL) {
    lazy_defaultdiffuse = new SbColor;
    lazy_defaulttransp = new float;
    lazy_defaultindex = new int32_t;
    lazy_defaultpacked = new uint32_t;
    lazy_unpacked = new SbColor;

    *lazy_defaultdiffuse = getDefaultDiffuse();
    *lazy_defaulttransp = getDefaultTransparency();
    *lazy_defaultindex = getDefaultColorIndex();
    *lazy_defaultpacked = getDefaultPacked();

    coin_atexit(lazyelement_cleanup, CC_ATEXIT_NORMAL);
  }
}

/*!
  Destructor.
*/

SoLazyElement::~SoLazyElement()
{
}

// ! FIXME: write doc

void
SoLazyElement::init(SoState * COIN_UNUSED_ARG(state))
{
  this->coinstate.ambient = this->getDefaultAmbient();
  this->coinstate.specular = this->getDefaultSpecular();
  this->coinstate.emissive = this->getDefaultEmissive();
  this->coinstate.shininess = this->getDefaultShininess();
  this->coinstate.blending = FALSE;
  this->coinstate.blend_sfactor = 0;
  this->coinstate.blend_dfactor = 0;
  this->coinstate.alpha_blend_sfactor = 0;
  this->coinstate.alpha_blend_dfactor = 0;
  this->coinstate.lightmodel = PHONG;
  this->coinstate.packeddiffuse = FALSE;
  this->coinstate.numdiffuse = 1;
  this->coinstate.numtransp = 1;
  this->coinstate.diffusearray = lazy_defaultdiffuse;
  this->coinstate.packedarray = lazy_defaultpacked;
  this->coinstate.transparray = lazy_defaulttransp;
  this->coinstate.colorindexarray = lazy_defaultindex;
  this->coinstate.istransparent = FALSE;
  this->coinstate.transptype = static_cast<int32_t>(SoGLRenderAction::BLEND);
  this->coinstate.diffusenodeid = 0;
  this->coinstate.transpnodeid = 0;
  this->coinstate.stipplenum = 0;
  this->coinstate.vertexordering = CCW;
  this->coinstate.twoside = FALSE;
  this->coinstate.culling = FALSE;
  this->coinstate.flatshading = FALSE;
  this->coinstate.alphatestfunc = 0;
  this->coinstate.alphatestvalue = 0.5f;
}

// ! FIXME: write doc

void
SoLazyElement::push(SoState *state)
{
  inherited::push(state);
  const SoLazyElement * prev = coin_assert_cast<const SoLazyElement *>(this->getNextInStack());
  this->coinstate = prev->coinstate;
}


/*!
  Will always return TRUE in Coin.
*/
SbBool
SoLazyElement::matches(const SoElement * COIN_UNUSED_ARG(element)) const
{
  assert(0 && "should never happen");
  return TRUE;
}

/*!
  Just returns NULL in Coin.
*/
SoElement *
SoLazyElement::copyMatchInfo(void) const
{
  assert(0 && "should never happen");
  return NULL;
}

/*!
  Internal function used for resetting the OpenGL state before FBO
  rendering.
*/
void
SoLazyElement::setToDefault(SoState * state)
{
  SoLazyElement * elem = SoLazyElement::getWInstance(state);
  elem->SoLazyElement::init(state);
}

// ! FIXME: write doc

void
SoLazyElement::setDiffuse(SoState * state, SoNode * node, int32_t numcolors,
                          const SbColor * colors, SoColorPacker * packer)
{
  if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
    SoGLVBOElement::setColorVBO(state, NULL);
  }
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (numcolors && (elem->coinstate.diffusenodeid !=
                    get_diffuse_node_id(node, numcolors, colors))) {
    elem = getWInstance(state);
    elem->setDiffuseElt(node, numcolors, colors, packer);
    if (state->isCacheOpen()) elem->lazyDidSet(DIFFUSE_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(DIFFUSE_MASK);
  }
}

// ! FIXME: write doc

void
SoLazyElement::setTransparency(SoState *state, SoNode *node, int32_t numvalues,
                               const float * transparency, SoColorPacker * packer)
{
  if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
    SoGLVBOElement::setColorVBO(state, NULL);
  }
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (numvalues && (elem->coinstate.transpnodeid !=
                    get_transp_node_id(node, numvalues, transparency))) {
    elem = getWInstance(state);
    elem->setTranspElt(node, numvalues, transparency, packer);
    if (state->isCacheOpen()) elem->lazyDidSet(TRANSPARENCY_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(TRANSPARENCY_MASK);
  }
  SoShapeStyleElement::setTransparentMaterial(state, elem->coinstate.istransparent);
}

// ! FIXME: write doc

void
SoLazyElement::setPacked(SoState * state, SoNode * node,
                         int32_t numcolors, const uint32_t * colors,
                         const SbBool packedtransparency)
{
  if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
    SoGLVBOElement::setColorVBO(state, NULL);
  }
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (numcolors && elem->coinstate.diffusenodeid != node->getNodeId()) {
    elem = getWInstance(state);
    elem->setPackedElt(node, numcolors, colors, packedtransparency);
    if (state->isCacheOpen()) elem->lazyDidSet(TRANSPARENCY_MASK|DIFFUSE_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(TRANSPARENCY_MASK|DIFFUSE_MASK);
  }
  SoShapeStyleElement::setTransparentMaterial(state, elem->coinstate.istransparent);
}

// ! FIXME: write doc

void
SoLazyElement::setColorIndices(SoState *state, SoNode *node,
                               int32_t numindices, const int32_t * indices)
{
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (numindices && elem->coinstate.diffusenodeid != node->getNodeId()) {
    elem = getWInstance(state);
    elem->setColorIndexElt(node, numindices, indices);
    if (state->isCacheOpen()) elem->lazyDidSet(DIFFUSE_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(DIFFUSE_MASK);
  }
}

// ! FIXME: write doc

void
SoLazyElement::setAmbient(SoState *state, const SbColor* color)
{
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (elem->coinstate.ambient != *color) {
    elem = getWInstance(state);
    elem->setAmbientElt(color);
    if (state->isCacheOpen()) elem->lazyDidSet(AMBIENT_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(AMBIENT_MASK);
  }
}

// ! FIXME: write doc

void
SoLazyElement::setEmissive(SoState *state, const SbColor* color)
{
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (elem->coinstate.emissive != *color) {
    elem = getWInstance(state);
    elem->setEmissiveElt(color);
    if (state->isCacheOpen()) elem->lazyDidSet(EMISSIVE_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(EMISSIVE_MASK);
  }
}

// ! FIXME: write doc

void
SoLazyElement::setSpecular(SoState *state, const SbColor* color)
{
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (elem->coinstate.specular != *color) {
    elem = getWInstance(state);
    elem->setSpecularElt(color);
    if (state->isCacheOpen()) elem->lazyDidSet(SPECULAR_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(SPECULAR_MASK);
  }
}

// ! FIXME: write doc

void
SoLazyElement::setShininess(SoState *state, float value)
{
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (SbAbs(elem->coinstate.shininess - value) > SO_LAZY_SHINY_THRESHOLD) {
    elem = getWInstance(state);
    elem->setShininessElt(value);
    if (state->isCacheOpen()) elem->lazyDidSet(SHININESS_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(SHININESS_MASK);
  }
}

// ! FIXME: write doc

void
SoLazyElement::setColorMaterial(SoState * COIN_UNUSED_ARG(state), SbBool COIN_UNUSED_ARG(value))
{
}

// ! FIXME: write doc

void
SoLazyElement::enableBlending(SoState * state,  int sfactor, int dfactor)
{
  SoLazyElement::enableSeparateBlending(state, sfactor, dfactor, 0, 0);
}

void
SoLazyElement::enableSeparateBlending(SoState * state,
                                      int sfactor, int dfactor,
                                      int alpha_sfactor, int alpha_dfactor)
{
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (!elem->coinstate.blending ||
      elem->coinstate.blend_sfactor != sfactor ||
      elem->coinstate.blend_dfactor != dfactor ||
      elem->coinstate.alpha_blend_sfactor != alpha_sfactor ||
      elem->coinstate.alpha_blend_dfactor != alpha_dfactor) {
    elem = getWInstance(state);
    elem->enableBlendingElt(sfactor, dfactor, alpha_sfactor, alpha_dfactor);
    if (state->isCacheOpen()) elem->lazyDidSet(BLENDING_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(BLENDING_MASK);
  }
}

// ! FIXME: write doc

void
SoLazyElement::disableBlending(SoState * state)
{
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (elem->coinstate.blending) {
    elem = getWInstance(state);
    elem->disableBlendingElt();
    if (state->isCacheOpen()) elem->lazyDidSet(BLENDING_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(BLENDING_MASK);
  }
}

// ! FIXME: write doc

void
SoLazyElement::setLightModel(SoState * state, const int32_t model)
{
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (elem->coinstate.lightmodel != model) {
    elem = getWInstance(state);
    elem->setLightModelElt(state, model);
    if (state->isCacheOpen()) elem->lazyDidSet(LIGHT_MODEL_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(LIGHT_MODEL_MASK);
  }
}

// ! FIXME: write doc

const SbColor &
SoLazyElement::getDiffuse(SoState * state, int index)
{
  SoLazyElement * elem = getInstance(state);
  if (elem->coinstate.packeddiffuse) {
    float dummy;
    return lazy_unpacked->setPackedValue(elem->coinstate.packedarray[index], dummy);
  }
  return elem->coinstate.diffusearray[index];
}

// ! FIXME: write doc

float
SoLazyElement::getTransparency(SoState *state, int index)
{
  SoLazyElement * elem = getInstance(state);

  if (elem->coinstate.packeddiffuse) {
    float transp;
    SbColor dummy;
    const int numt = elem->coinstate.numdiffuse;
    dummy.setPackedValue(elem->coinstate.packedarray[index < numt ? index : numt-1], transp);
    return transp;
  }
  const int numt = elem->coinstate.numtransp;
  return elem->coinstate.transparray[index < numt ? index : numt-1];
}

// ! FIXME: write doc

const uint32_t *
SoLazyElement::getPackedColors(SoState * state)
{
  SoLazyElement * elem = getInstance(state);
  return elem->coinstate.packedarray;
}

// ! FIXME: write doc

const int32_t *
SoLazyElement::getColorIndices(SoState * state)
{
  SoLazyElement * elem = getInstance(state);
  return elem->coinstate.colorindexarray;
}

// ! FIXME: write doc

int32_t
SoLazyElement::getColorIndex(SoState * state, int num)
{
  SoLazyElement * elem = getInstance(state);
  return elem->coinstate.colorindexarray[num];
}

// ! FIXME: write doc

const SbColor &
SoLazyElement::getAmbient(SoState * state)
{
  SoLazyElement * elem = getInstance(state);
  return elem->coinstate.ambient;
}

// ! FIXME: write doc

const SbColor &
SoLazyElement::getEmissive(SoState * state)
{
  SoLazyElement * elem = getInstance(state);
  return elem->coinstate.emissive;
}

// ! FIXME: write doc

const SbColor &
SoLazyElement::getSpecular(SoState * state)
{
  SoLazyElement * elem = getInstance(state);
  return elem->coinstate.specular;
}

// ! FIXME: write doc

float
SoLazyElement::getShininess(SoState * state)
{
  SoLazyElement * elem = getInstance(state);
  return elem->coinstate.shininess;
}

// ! FIXME: write doc

SbBool
SoLazyElement::getColorMaterial(SoState * COIN_UNUSED_ARG(state))
{
  return TRUE;
}

// ! FIXME: write doc

SbBool
SoLazyElement::getBlending(SoState * state, int & sfactor, int & dfactor)
{
  SoLazyElement * elem = getInstance(state);
  sfactor = elem->coinstate.blend_sfactor;
  dfactor = elem->coinstate.blend_dfactor;
  return elem->coinstate.blending;
}

SbBool
SoLazyElement::getAlphaBlending(SoState * state, int & sfactor, int & dfactor)
{
  SoLazyElement * elem = getInstance(state);
  sfactor = elem->coinstate.alpha_blend_sfactor;
  dfactor = elem->coinstate.alpha_blend_dfactor;

  return elem->coinstate.blending && (sfactor != 0) && (dfactor != 0);
}

// ! FIXME: write doc

int32_t
SoLazyElement::getLightModel(SoState * state)
{
  SoLazyElement * elem = getInstance(state);
  return elem->coinstate.lightmodel;
}

SbBool 
SoLazyElement::getTwoSidedLighting(SoState * state)
{
  SoLazyElement * elem = getInstance(state);
  return elem->coinstate.twoside;
}

// ! FIXME: write doc
int
SoLazyElement::getAlphaTest(SoState * state, float & value)
{
  SoLazyElement * elem = getInstance(state);
  value = elem->coinstate.alphatestvalue;
  return elem->coinstate.alphatestfunc;
}

// ! FIXME: write doc

int32_t
SoLazyElement::getNumDiffuse(void) const
{
  return this->coinstate.numdiffuse;
}

// ! FIXME: write doc

int32_t
SoLazyElement::getNumTransparencies(void) const
{
  if (this->coinstate.packeddiffuse) {
    return this->coinstate.numdiffuse;
  }
  return this->coinstate.numtransp;
}

// ! FIXME: write doc

int32_t
SoLazyElement::getNumColorIndices(void) const
{
  return this->coinstate.numdiffuse;
}

// ! FIXME: write doc

SbBool
SoLazyElement::isPacked(void) const
{
  return this->coinstate.packeddiffuse;
}

// ! FIXME: write doc

SbBool
SoLazyElement::isTransparent(void) const
{
  return this->coinstate.istransparent;
}

// ! FIXME: write doc

SoLazyElement *
SoLazyElement::getInstance(SoState *state)
{
  return
    coin_safe_cast<SoLazyElement *>
    (
     state->getElementNoPush(classStackIndex)
     );
}

// ! FIXME: write doc

float
SoLazyElement::getDefaultAmbientIntensity(void)
{
  return 0.2f;
}

// ! FIXME: write doc

SbColor
SoLazyElement::getDefaultDiffuse(void)
{
  return SbColor(0.8f, 0.8f, 0.8f);
}

// ! FIXME: write doc

SbColor
SoLazyElement::getDefaultAmbient(void)
{
  return SbColor(0.2f, 0.2f, 0.2f);
}

// ! FIXME: write doc

SbColor
SoLazyElement::getDefaultSpecular(void)
{
  return SbColor(0.0f, 0.0f, 0.0f);
}

// ! FIXME: write doc

SbColor
SoLazyElement::getDefaultEmissive(void)
{
  return SbColor(0.0f, 0.0f, 0.0f);
}

// ! FIXME: write doc

float
SoLazyElement::getDefaultShininess(void)
{
  return 0.2f;
}

// ! FIXME: write doc

uint32_t
SoLazyElement::getDefaultPacked(void)
{
  return 0xccccccff;
}

// ! FIXME: write doc

float
SoLazyElement::getDefaultTransparency(void)
{
  return 0.0f;
}

// ! FIXME: write doc

int32_t
SoLazyElement::getDefaultLightModel(void)
{
  return static_cast<int32_t>(SoLazyElement::PHONG);
}

// ! FIXME: write doc

int32_t
SoLazyElement::getDefaultColorIndex(void)
{
  return 0;
}

// ! FIXME: write doc

void
SoLazyElement::setMaterials(SoState * state, SoNode *node, uint32_t bitmask,
                            SoColorPacker * packer,
                            const SbColor * diffuse,
                            const int numdiffuse,
                            const float * transp,
                            const int numtransp,
                            const SbColor & ambient,
                            const SbColor & emissive,
                            const SbColor & specular,
                            const float shininess,
                            const SbBool istransparent)
{
  if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
    SoGLVBOElement::setColorVBO(state, NULL);
  }
  SoLazyElement * elem = SoLazyElement::getInstance(state);

  uint32_t eltbitmask = 0;
  if (bitmask & DIFFUSE_MASK) {
    if (elem->coinstate.diffusenodeid !=
        get_diffuse_node_id(node, numdiffuse, diffuse)) {
      eltbitmask |= DIFFUSE_MASK;
    }
  }
  if (bitmask & TRANSPARENCY_MASK) {
    if (elem->coinstate.transpnodeid != get_transp_node_id(node, numtransp, transp)) {
      eltbitmask |= TRANSPARENCY_MASK;
    }
  }
  if (bitmask & AMBIENT_MASK) {
    if (elem->coinstate.ambient != ambient) {
      eltbitmask |= AMBIENT_MASK;
    }
  }
  if (bitmask & EMISSIVE_MASK) {
    if (elem->coinstate.emissive != emissive) {
      eltbitmask |= EMISSIVE_MASK;
    }
  }
  if (bitmask & SPECULAR_MASK) {
    if (elem->coinstate.specular != specular) {
      eltbitmask |= SPECULAR_MASK;
    }
  }
  if (bitmask & SHININESS_MASK) {
    if (SbAbs(elem->coinstate.shininess-shininess) > SO_LAZY_SHINY_THRESHOLD) {
      eltbitmask |= SHININESS_MASK;
    }
  }

  SoLazyElement * welem = NULL;

  if (eltbitmask) {
    welem = getWInstance(state);
    welem->setMaterialElt(node, eltbitmask, packer, diffuse,
                          numdiffuse, transp, numtransp,
                          ambient, emissive, specular, shininess,
                          istransparent);
    if (state->isCacheOpen()) welem->lazyDidSet(eltbitmask);
  }

  if ((eltbitmask != bitmask) && state->isCacheOpen()) {
    if (welem) elem = welem;
    elem->lazyDidntSet((~eltbitmask) & bitmask);
  }
  if (bitmask & TRANSPARENCY_MASK) {
    SoShapeStyleElement::setTransparentMaterial(state, istransparent);
  }
}

void
SoLazyElement::setVertexOrdering(SoState * state, VertexOrdering ordering)
{
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (elem->coinstate.vertexordering != ordering) {
    elem = getWInstance(state);
    elem->setVertexOrderingElt(ordering);
    if (state->isCacheOpen()) elem->lazyDidSet(VERTEXORDERING_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(VERTEXORDERING_MASK);
  }
}

void
SoLazyElement::setBackfaceCulling(SoState * state, SbBool onoff)
{
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (elem->coinstate.culling != onoff) {
    elem = getWInstance(state);
    elem->setBackfaceCullingElt(onoff);
    if (state->isCacheOpen()) elem->lazyDidSet(CULLING_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(CULLING_MASK);
  }
}

void
SoLazyElement::setTwosideLighting(SoState * state, SbBool onoff)
{
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (elem->coinstate.twoside != onoff) {
    elem = getWInstance(state);
    elem->setTwosideLightingElt(onoff);
    if (state->isCacheOpen()) elem->lazyDidSet(TWOSIDE_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(TWOSIDE_MASK);
  }
}

void
SoLazyElement::setShadeModel(SoState * state, SbBool flatshading)
{
  SoLazyElement * elem = SoLazyElement::getInstance(state);

  if (elem->coinstate.flatshading != flatshading) {
    elem = getWInstance(state);
    elem->setShadeModelElt(flatshading);
    if (state->isCacheOpen()) elem->lazyDidSet(SHADE_MODEL_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(SHADE_MODEL_MASK);
  }
}

void
SoLazyElement::setAlphaTest(SoState * state, int func, float value)
{
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (elem->coinstate.alphatestfunc != func ||
      elem->coinstate.alphatestvalue != value) {
    elem = getWInstance(state);
    elem->setAlphaTestElt(func, value);
    if (state->isCacheOpen()) elem->lazyDidSet(ALPHATEST_MASK);
  }
  else if (state->isCacheOpen()) {
    elem->lazyDidntSet(ALPHATEST_MASK);
  }
}


// ! FIXME: write doc

SoLazyElement *
SoLazyElement::getWInstance(SoState * state)
{
  // don't use SoElement::getConstElement() as this will cause
  // cache dependencies.
  return
    coin_safe_cast<SoLazyElement *>
    (
     state->getElement(classStackIndex)
     );
}

// ! FIXME: write doc

const uint32_t *
SoLazyElement::getPackedPointer(void) const
{
  return this->coinstate.packedarray;
}

// ! FIXME: write doc

const SbColor *
SoLazyElement::getDiffusePointer(void) const
{
  return this->coinstate.diffusearray;
}

// ! FIXME: write doc

const int32_t *
SoLazyElement::getColorIndexPointer(void) const
{
  assert(0 && "color index mode is not supported in Coin");
  return NULL;
}

// ! FIXME: write doc

const float *
SoLazyElement::getTransparencyPointer(void) const
{
  return this->coinstate.transparray;
}

// ! FIXME: write doc

void
SoLazyElement::setTransparencyType(SoState *state, int32_t type)
{
  SoLazyElement * elem = SoLazyElement::getInstance(state);
  if (elem->coinstate.transptype != type) {
    getWInstance(state)->setTranspTypeElt(type);
  }
}


void
SoLazyElement::setDiffuseElt(SoNode * node,  int32_t numcolors,
                             const SbColor * colors, SoColorPacker * COIN_UNUSED_ARG(packer))
{
  this->coinstate.diffusenodeid = get_diffuse_node_id(node, numcolors, colors);
  this->coinstate.diffusearray = colors;
  this->coinstate.numdiffuse = numcolors;
  this->coinstate.packeddiffuse = FALSE;
}

void
SoLazyElement::setPackedElt(SoNode * node, int32_t numcolors,
                            const uint32_t * colors, const SbBool packedtransparency)
{
  this->coinstate.diffusenodeid = node->getNodeId();
  this->coinstate.transpnodeid = node->getNodeId();
  this->coinstate.numdiffuse = numcolors;
  this->coinstate.packedarray = colors;
  this->coinstate.packeddiffuse = TRUE;
  this->coinstate.istransparent = packedtransparency;

  int alpha = colors[0] & 0xff;
  float transp = float(255-alpha)/255.0f;
  this->coinstate.stipplenum = SbClamp(static_cast<int>(transp * 64.0f), 0, 64);
}

void
SoLazyElement::setColorIndexElt(SoNode * COIN_UNUSED_ARG(node), int32_t numindices,
                                const int32_t * indices)
{
  this->coinstate.colorindexarray = indices;
  this->coinstate.numdiffuse = numindices;
  this->coinstate.packeddiffuse = FALSE;
}

void
SoLazyElement::setTranspElt(SoNode * node, int32_t numtransp,
                            const float * transp, SoColorPacker * COIN_UNUSED_ARG(packer))
{
  this->coinstate.transpnodeid = get_transp_node_id(node, numtransp, transp);
  this->coinstate.transparray = transp;
  this->coinstate.numtransp = numtransp;
  this->coinstate.stipplenum = SbClamp(static_cast<int>(transp[0] * 64.0f), 0, 64);

  this->coinstate.istransparent = FALSE;
  for (int i = 0; i < numtransp; i++) {
    if (transp[i] > 0.0f) {
      this->coinstate.istransparent = TRUE;
      break;
    }
  }
}


void
SoLazyElement::setTranspTypeElt(int32_t type)
{
  this->coinstate.transptype = type;
}

void
SoLazyElement::setAmbientElt(const SbColor* color)
{
  this->coinstate.ambient = *color;
}

void
SoLazyElement::setEmissiveElt(const SbColor* color)
{
  this->coinstate.emissive = *color;
}

void
SoLazyElement::setSpecularElt(const SbColor* color)
{
  this->coinstate.specular = *color;
}

void
SoLazyElement::setShininessElt(float value)
{
  this->coinstate.shininess = value;
}

void
SoLazyElement::setColorMaterialElt(SbBool COIN_UNUSED_ARG(value))
{
}

void
SoLazyElement::enableBlendingElt(int sfactor, int dfactor, int alpha_sfactor, int alpha_dfactor)
{
  this->coinstate.blending = TRUE;
  this->coinstate.blend_sfactor = sfactor;
  this->coinstate.blend_dfactor = dfactor;
  this->coinstate.alpha_blend_sfactor = alpha_sfactor;
  this->coinstate.alpha_blend_dfactor = alpha_dfactor;
}

void
SoLazyElement::disableBlendingElt(void)
{
  this->coinstate.blending = FALSE;
}

void
SoLazyElement::setLightModelElt(SoState * state, int32_t model)
{
  SoShapeStyleElement::setLightModel(state, model);
  this->coinstate.lightmodel = model;
}

void
SoLazyElement::setMaterialElt(SoNode * node, uint32_t bitmask,
                              SoColorPacker * COIN_UNUSED_ARG(packer),
                              const SbColor * diffuse, const int numdiffuse,
                              const float * transp, const int numtransp,
                              const SbColor & ambient,
                              const SbColor & emissive,
                              const SbColor & specular,
                              const float shininess,
                              const SbBool istransparent)
{
  if (bitmask & DIFFUSE_MASK) {
    this->coinstate.diffusenodeid = get_diffuse_node_id(node, numdiffuse, diffuse);
    this->coinstate.diffusearray = diffuse;
    this->coinstate.numdiffuse = numdiffuse;
    this->coinstate.packeddiffuse = FALSE;
  }
  if (bitmask & TRANSPARENCY_MASK) {
    this->coinstate.transpnodeid = get_transp_node_id(node, numtransp, transp);
    this->coinstate.transparray = transp;
    this->coinstate.numtransp = numtransp;
    this->coinstate.stipplenum = SbClamp(static_cast<int>(transp[0] * 64.0f), 0, 64);
    // check for common case
    if (numtransp == 1 && transp[0] == 0.0f) {
      this->coinstate.transpnodeid = 0;
      this->coinstate.istransparent = FALSE;
    }
    else {
      this->coinstate.istransparent = istransparent;
    }
  }
  if (bitmask & AMBIENT_MASK) {
    this->coinstate.ambient = ambient;
  }
  if (bitmask & EMISSIVE_MASK) {
    this->coinstate.emissive = emissive;
  }
  if (bitmask & SPECULAR_MASK) {
    this->coinstate.specular = specular;
  }
  if (bitmask & SHININESS_MASK) {
    this->coinstate.shininess = shininess;
  }
}

void
SoLazyElement::setVertexOrderingElt(VertexOrdering ordering)
{
  this->coinstate.vertexordering = ordering;
}

void
SoLazyElement::setBackfaceCullingElt(SbBool onoff)
{
  this->coinstate.culling = onoff;
}

void
SoLazyElement::setTwosideLightingElt(SbBool onoff)
{
  this->coinstate.twoside = onoff;
}

void
SoLazyElement::setShadeModelElt(SbBool flatshading)
{
  this->coinstate.flatshading = flatshading;
}

void
SoLazyElement::setAlphaTestElt(int func, float value)
{
  this->coinstate.alphatestfunc = func;
  this->coinstate.alphatestvalue = value;
}


// SoColorPacker class. FIXME: move to separate file and document, pederb, 2002-09-09

static uint32_t colorpacker_default = 0xccccccff;

SoColorPacker::SoColorPacker(void)
{
  this->array = &colorpacker_default;
  this->arraysize = 0;
  this->diffuseid = 0;
  this->transpid = 0;
}

SoColorPacker::~SoColorPacker()
{
  if (this->array != &colorpacker_default) {
    delete[] this->array;
  }
}

void
SoColorPacker::reallocate(const int32_t size)
{
  assert(size > this->arraysize);
  uint32_t * newarray = new uint32_t[size];
  if (this->array != &colorpacker_default) {
    delete[] this->array;
  }
  this->array = newarray;
  this->arraysize = size;
}

void
SoLazyElement::lazyDidSet(uint32_t COIN_UNUSED_ARG(mask))
{
}

void
SoLazyElement::lazyDidntSet(uint32_t COIN_UNUSED_ARG(mask))
{
}
