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
  \class SoGLLazyElement Inventor/elements/SoGLLazyElement.h
  \brief The SoGLLazyElement class is meant to optimize GL rendering.

  \ingroup coin_elements

  This is just a wrap-around implementation for compatibility. It should
  (hopefully) work in the same way as the Inventor class though.
*/

// FIXME: is the above class doc comment still correct? Or do we have
// an implementation of this class in the same manner as the other
// Inventors now? 20040702 mortene.

// *************************************************************************

#include <Inventor/elements/SoGLLazyElement.h>

#include <cassert>

#include <Inventor/C/glue/gl.h>
#include <Inventor/SbImage.h>
#include <Inventor/SoFullPath.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLDisplayList.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoTextureCombineElement.h>
#include <Inventor/elements/SoGLShaderProgramElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/C/tidbits.h>
#include "rendering/SoVBO.h"
#include <coindefs.h> // COIN_OBSOLETED

#include "shaders/SoGLShaderProgram.h"

// *************************************************************************

#define FLAG_FORCE_DIFFUSE      0x0001
#define FLAG_DIFFUSE_DEPENDENCY 0x0002

#if COIN_DEBUG
// #define GLLAZY_DEBUG(_x_) (SoDebugError::postInfo(COIN_STUB_FUNC, _x_))
#define GLLAZY_DEBUG(x)
#else
#define GLLAZY_DEBUG(x)
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

// Some data and functions to create Bayer dither matrices (used for
// screen door transparency)
static unsigned char stipple_patterns[64 + 1][32 * 4];
static uint32_t two_by_two[] = {0, 2, 3, 1};

//
// Used to generate a matrix twice the size of the input
//
static void
generate_next_matrix(uint32_t * old, int oldsize,
                     uint32_t * matrix)
{
  int i,j;
  int newsize = oldsize << 1;
  for (i = 0; i <  newsize; i++) {
    for (j = 0; j < newsize; j++) {
      matrix[i*newsize+j] = 4 * old[(i%oldsize)*oldsize + (j%oldsize)];
      matrix[i*newsize+j] += two_by_two[(i/oldsize)*2 + (j/oldsize)];
    }
  }
}

//
// Creates a matrix by starting with a 2x2 and doubling until size
//
static void
make_dither_matrix(uint32_t * ptr, int size)
{
  int currsize = 2;

  uint32_t * currmatrix = two_by_two;
  uint32_t * nextmatrix = NULL;
  int nextsize;

  while (currsize < size) {
    nextsize = currsize << 1;
    nextmatrix = new uint32_t[nextsize*nextsize];
    generate_next_matrix(currmatrix, currsize, nextmatrix);
    if (currmatrix != two_by_two) delete[] currmatrix;
    currmatrix = nextmatrix;
    currsize = nextsize;
  }
  // copy matrix
  int i;
  for (i = 0; i < size*size; i++)
    ptr[i] = currmatrix[i];

  if (currmatrix != two_by_two) delete[] currmatrix;
}

//
// Sets a bit bitnr bits from ptr
//
static void
set_bit(int bitnr, unsigned char * ptr)
{
  int byte = bitnr / 8;
  int bit = bitnr % 8;

  unsigned char mask = (unsigned char) (0x80 >> bit);

  ptr[byte] |= mask;
}

//
// Create a bitmap from a 32x32 matrix
//
static void
create_matrix_bitmap(int intensity, unsigned char * bitmap,
                     uint32_t * matrix, int size)
{
  int cnt = 0;
  int i,j;
  for (i = 0; i < 32*4; i++) bitmap[i] = 0;
  for (i = 0; i < size; i++) {
    for (j = 0; j < size; j++) {
      if (matrix[i*size+j] > (uint32_t) intensity) {
        set_bit(i*32+j, bitmap);
        cnt++;
      }
    }
  }
}


SO_ELEMENT_SOURCE(SoGLLazyElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLLazyElement::initClass()
{
  SO_ELEMENT_INIT_CLASS(SoGLLazyElement, inherited);

  // create stipple patterns
  int i;
  uint32_t matrix[32*32];
  make_dither_matrix((uint32_t*)matrix, 32);
  for (i = 0; i <= 64; i++) {
    int intensity = (32 * 32 * i) / 64 - 1;
    create_matrix_bitmap((intensity >= 0) ? intensity : 0,
                         stipple_patterns[i], (uint32_t*) matrix, 32);
  }
}

/*!
  Destructor.
*/

SoGLLazyElement::~SoGLLazyElement()
{
}

//! FIXME: write doc
SoGLLazyElement *
SoGLLazyElement::getInstance(const SoState *state)
{
  return (SoGLLazyElement*)
    state->getConstElement(classStackIndex);
}

inline void
SoGLLazyElement::sendPackedDiffuse(const uint32_t col) const
{
  glColor4ub((unsigned char)((col>>24)&0xff),
             (unsigned char)((col>>16)&0xff),
             (unsigned char)((col>>8)&0xff),
             (unsigned char)(col&0xff));
  this->glstate.diffuse = col;
  this->cachebitmask |= DIFFUSE_MASK;
}

inline void
SoGLLazyElement::sendLightModel(const int32_t model) const
{
  if (model == PHONG) glEnable(GL_LIGHTING);
  else glDisable(GL_LIGHTING);
  this->glstate.lightmodel = model;
  this->cachebitmask |= LIGHT_MODEL_MASK;
}

inline void
SoGLLazyElement::sendFlatshading(const SbBool onoff) const
{
  if (onoff) glShadeModel(GL_FLAT);
  else glShadeModel(GL_SMOOTH);
  this->glstate.flatshading = (int32_t) onoff;
  this->cachebitmask |= SHADE_MODEL_MASK;
}

inline void
SoGLLazyElement::sendAlphaTest(int func, float value) const
{
  if (func) {
    glAlphaFunc((GLenum) func, value);
    glEnable(GL_ALPHA_TEST);
  }
  else {
    glDisable(GL_ALPHA_TEST);
  }
  this->cachebitmask |= ALPHATEST_MASK;
  this->glstate.alphatestfunc = func;
  this->glstate.alphatestvalue = value;
}


inline void
SoGLLazyElement::sendVertexOrdering(const VertexOrdering ordering) const
{
  glFrontFace(ordering == CW ? GL_CW : GL_CCW);
  this->glstate.vertexordering = (int32_t) ordering;
  this->cachebitmask |= VERTEXORDERING_MASK;
}

inline void
SoGLLazyElement::sendTwosideLighting(const SbBool onoff) const
{
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, onoff ? GL_TRUE : GL_FALSE);
  this->glstate.twoside = (int32_t) onoff;
  this->cachebitmask |= TWOSIDE_MASK;
}

inline void
SoGLLazyElement::sendBackfaceCulling(const SbBool onoff) const
{
  if (onoff) glEnable(GL_CULL_FACE);
  else glDisable(GL_CULL_FACE);
  this->glstate.culling = onoff;
  this->cachebitmask |= CULLING_MASK;
}

static inline void
send_gl_material(GLenum pname, const SbColor & color)
{
  GLfloat col[4];
  color.getValue(col[0], col[1], col[2]);
  col[3] = 1.0f;
  glMaterialfv(GL_FRONT_AND_BACK, pname, col);
}


inline void
SoGLLazyElement::sendAmbient(const SbColor & color) const
{
  send_gl_material(GL_AMBIENT, color);
  this->glstate.ambient = color;
  this->cachebitmask |= AMBIENT_MASK;
}

inline void
SoGLLazyElement::sendEmissive(const SbColor & color) const
{
  send_gl_material(GL_EMISSION, color);
  this->glstate.emissive = color;
  this->cachebitmask |= EMISSIVE_MASK;
}

inline void
SoGLLazyElement::sendSpecular(const SbColor & color) const
{
  send_gl_material(GL_SPECULAR, color);
  this->glstate.specular = color;
  this->cachebitmask |= SPECULAR_MASK;
}

inline void
SoGLLazyElement::sendShininess(const float shine) const
{
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shine*128.0f);
  this->glstate.shininess = shine;
  this->cachebitmask |= SHININESS_MASK;
}

inline void
SoGLLazyElement::sendTransparency(const int stipplenum) const
{
  if (stipplenum == 0) {
    glDisable(GL_POLYGON_STIPPLE);
  }
  else {
    if (this->glstate.stipplenum <= 0) glEnable(GL_POLYGON_STIPPLE);
    glPolygonStipple(stipple_patterns[stipplenum]);
  }
  this->glstate.stipplenum = stipplenum;
  this->cachebitmask |= TRANSPARENCY_MASK;
}

inline void
SoGLLazyElement::enableBlending(const int sfactor, const int dfactor) const
{
  glEnable(GL_BLEND);
  glBlendFunc((GLenum) sfactor, (GLenum) dfactor);
  this->glstate.blending = TRUE;
  this->glstate.blend_sfactor = sfactor;
  this->glstate.blend_dfactor = dfactor;
  this->glstate.alpha_blend_sfactor = 0;
  this->glstate.alpha_blend_dfactor = 0;
  this->cachebitmask |= BLENDING_MASK;
}

inline void
SoGLLazyElement::enableSeparateBlending(const cc_glglue * glue,
                                        const int sfactor,
                                        const int dfactor,
                                        const int alpha_sfactor,
                                        const int alpha_dfactor) const
{
  glEnable(GL_BLEND);

  if (cc_glglue_has_blendfuncseparate(glue)) {
    cc_glglue_glBlendFuncSeparate(glue, sfactor, dfactor, alpha_sfactor, alpha_dfactor);
  }
  else {
      // fall back to normal blending
    glBlendFunc((GLenum) sfactor, (GLenum) dfactor);
  }
  this->glstate.blending = TRUE;
  this->glstate.blend_sfactor = sfactor;
  this->glstate.blend_dfactor = dfactor;
  this->glstate.alpha_blend_sfactor = alpha_sfactor;
  this->glstate.alpha_blend_dfactor = alpha_dfactor;
  this->cachebitmask |= BLENDING_MASK;
}

inline void
SoGLLazyElement::disableBlending(void) const
{
  glDisable(GL_BLEND);
  this->glstate.blending = FALSE;
  this->cachebitmask |= BLENDING_MASK;
}

void
SoGLLazyElement::init(SoState * stateptr)
{
  inherited::init(stateptr);
  this->state = stateptr; // needed to send GL texture
  this->colorindex = FALSE;
  this->glstate.ambient.setValue(-1.0f, -1.0f, -1.0f);
  this->glstate.emissive.setValue(-1.0f, -1.0f, -1.0f);
  this->glstate.specular.setValue(-1.0f, -1.0f, -1.0f);
  this->glstate.shininess = -1.0f;
  this->glstate.lightmodel = -1;
  this->glstate.blending = -1;
  this->glstate.blend_sfactor = -1;
  this->glstate.blend_dfactor = -1;
  this->glstate.alpha_blend_sfactor = -1;
  this->glstate.alpha_blend_dfactor = -1;
  this->glstate.stipplenum = -1;
  this->glstate.vertexordering = -1;
  this->glstate.twoside = -1;
  this->glstate.culling = -1;
  this->glstate.flatshading = -1;
  this->glstate.alphatestfunc = -1;
  this->glstate.alphatestvalue = -1.0f;
  this->glstate.diffuse = 0xccccccff;
  this->glstate.diffusenodeid = 0;
  this->glstate.transpnodeid = 0;
  this->packedpointer = NULL;
  // when doing screen door rendering, we need to always supply 0xff as alpha.
  this->transpmask = (this->coinstate.transptype == SoGLRenderAction::SCREEN_DOOR) ? 0xff : 0x00;
  this->colorpacker = NULL;
  this->precachestate = NULL;
  this->postcachestate = NULL;
  this->opencacheflags = 0;

  // initialize this here to avoid UMR reports from
  // Purify. cachebitmask is updated even when there are no open
  // caches. It is only used (and properly initialized) when recording
  // a cache though.
  this->cachebitmask = 0;

  glDisable(GL_POLYGON_STIPPLE);

  GLboolean rgba;
  glGetBooleanv(GL_RGBA_MODE, &rgba);
  if (!rgba) this->colorindex = TRUE;
  else {
    this->sendPackedDiffuse(0xccccccff);
  }
}

void
SoGLLazyElement::push(SoState * stateptr)
{
  inherited::push(stateptr);
  SoGLLazyElement * prev = (SoGLLazyElement*) this->getNextInStack();
  this->state = stateptr; // needed to send GL texture
  this->glstate = prev->glstate;
  this->colorindex = prev->colorindex;
  this->transpmask = prev->transpmask;
  this->colorpacker = prev->colorpacker;
  this->precachestate = prev->precachestate;
  this->postcachestate = prev->postcachestate;
  this->didsetbitmask = prev->didsetbitmask;
  this->didntsetbitmask = prev->didntsetbitmask;
  this->cachebitmask = prev->cachebitmask;
  this->opencacheflags = prev->opencacheflags;
}

void
SoGLLazyElement::pop(SoState *stateptr, const SoElement * prevtopelement)
{
  inherited::pop(stateptr, prevtopelement);
  SoGLLazyElement * prev = (SoGLLazyElement*) prevtopelement;
  this->glstate = prev->glstate;
  this->colorindex = prev->colorindex;
  this->didsetbitmask = prev->didsetbitmask;
  this->didntsetbitmask = prev->didntsetbitmask;
  this->cachebitmask = prev->cachebitmask;
  this->opencacheflags = prev->opencacheflags;
}

//! FIXME: write doc

void
SoGLLazyElement::sendAllMaterial(SoState * state)
{
  SoGLLazyElement * elem = getInstance(state);
  elem->send(state, ALL_MASK);
}

//! FIXME: write doc

void
SoGLLazyElement::sendNoMaterial(SoState * state)
{
  SoGLLazyElement * elem = getInstance(state);
  elem->send(state, NO_COLOR_MASK);
}

//! FIXME: write doc

void
SoGLLazyElement::sendOnlyDiffuseColor(SoState * state)
{
  SoGLLazyElement * elem = getInstance(state);
  elem->send(state, DIFFUSE_ONLY_MASK);
}

//! FIXME: write doc

void
SoGLLazyElement::sendDiffuseByIndex(const int index) const
{
  int safeindex = index;
#if COIN_DEBUG
  if (index < 0 || index >= this->coinstate.numdiffuse) {
    static int first = 1;
    if (first) {
      SoFullPath * path = (SoFullPath*) this->state->getAction()->getCurPath();
      SoNode * tail = path->getTail();
      SbName name = tail->getName();
      SoDebugError::postWarning("SoGLLazyElement::sendDiffuseByIndex",
                                "index %d out of bounds [0, %d] in node %p: %s "
                                "(this warning will only be printed once, but there "
                                "might be more errors)",
                                index,
                                this->coinstate.numdiffuse-1,
                                tail, name != SbName::empty() ? name.getString() : "<noname>");
      first = 0;
    }

    safeindex = SbClamp((long) index, (long) 0, (long) (this->coinstate.numdiffuse-1));
  }
#endif // COIN_DEBUG

  if (this->colorindex) {
    glIndexi((GLint)this->coinstate.colorindexarray[safeindex]);
  }
  else {
    uint32_t col = this->packedpointer[safeindex] | this->transpmask;
    // this test is really not necessary. SoMaterialBundle does the
    // same test.  We also need to send the color here to work around
    // an nVIDIA bug
    // if (col != this->glstate.diffuse)
    this->sendPackedDiffuse(col);
  }
}

//! FIXME: write doc

SbBool
SoGLLazyElement::isColorIndex(SoState * state)
{
  SoGLLazyElement * elem = getInstance(state);
  return elem->colorindex;
}

//! FIXME: write doc

void
SoGLLazyElement::send(const SoState * stateptr, uint32_t mask) const
{
  if (this->colorpacker) {
    if (!this->colorpacker->diffuseMatch(this->coinstate.diffusenodeid) ||
        !this->colorpacker->transpMatch(this->coinstate.transpnodeid)) {
      this->packColors(this->colorpacker);
    }
    this->packedpointer = this->colorpacker->getPackedColors();
  }
  else this->packedpointer = this->coinstate.packedarray;

  assert(this->packedpointer);

  int stipplenum;

  for (int i = 0; (i < LAZYCASES_LAST)&&mask; i++, mask>>=1) {
    if (mask&1) {
      switch (i) {
      case LIGHT_MODEL_CASE:
        if (this->coinstate.lightmodel != this->glstate.lightmodel) {
          SoGLShaderProgram * prog = SoGLShaderProgramElement::get((SoState*) stateptr);
          if (prog) prog->updateCoinParameter((SoState*)stateptr, SbName("coin_light_model"), this->coinstate.lightmodel);
          this->sendLightModel(this->coinstate.lightmodel);
        }
        break;
      case DIFFUSE_CASE:
        if (this->precachestate) {
          // we are currently building a cache. Check if we're using
          // colors from a material node outside the cache.
          if ((this->precachestate->diffusenodeid == this->coinstate.diffusenodeid) ||
              (this->precachestate->transpnodeid == this->coinstate.transpnodeid)) {
            this->opencacheflags |= FLAG_DIFFUSE_DEPENDENCY;
          }
        }
        if (this->opencacheflags & FLAG_FORCE_DIFFUSE) {
          // we always send the first diffuse color for the first
          // material in an open cache
          if (this->colorindex) {
            glIndexi((GLint)this->coinstate.colorindexarray[0]);
          }
          else {
            this->sendPackedDiffuse(this->packedpointer[0]|this->transpmask);
          }
          this->opencacheflags &= ~FLAG_FORCE_DIFFUSE;
        }
        else {
          this->sendDiffuseByIndex(0);
        }
        break;
      case AMBIENT_CASE:
        if (this->coinstate.ambient != this->glstate.ambient) {
          this->sendAmbient(this->coinstate.ambient);
        }
        break;
      case SPECULAR_CASE:
        if (this->coinstate.specular != this->glstate.specular) {
          this->sendSpecular(this->coinstate.specular);
        }
        break;
      case EMISSIVE_CASE:
        if (this->coinstate.emissive != this->glstate.emissive) {
          this->sendEmissive(this->coinstate.emissive);
        }
        break;
      case SHININESS_CASE:
        if (this->coinstate.shininess != this->glstate.shininess) {
          this->sendShininess(this->coinstate.shininess);
        }
        break;
      case BLENDING_CASE:
        if (this->coinstate.blending) {
          if (this->glstate.blending != this->coinstate.blending ||
              this->coinstate.blend_sfactor != this->glstate.blend_sfactor ||
              this->coinstate.blend_dfactor != this->glstate.blend_dfactor ||
              this->coinstate.alpha_blend_sfactor != this->glstate.alpha_blend_sfactor ||
              this->coinstate.alpha_blend_dfactor != this->glstate.alpha_blend_dfactor) {
            if ((this->coinstate.alpha_blend_sfactor != 0) &&
                (this->coinstate.alpha_blend_dfactor != 0)) {
              this->enableSeparateBlending(cc_glglue_instance(SoGLCacheContextElement::get((SoState*)stateptr)),
                                           this->coinstate.blend_sfactor,
                                           this->coinstate.blend_dfactor,
                                           this->coinstate.alpha_blend_sfactor,
                                           this->coinstate.alpha_blend_dfactor);
            }
            else {
              this->enableBlending(this->coinstate.blend_sfactor, this->coinstate.blend_dfactor);
            }
          }
        }
        else {
          if (this->coinstate.blending != this->glstate.blending) {
            this->disableBlending();
          }
        }
        break;
      case TRANSPARENCY_CASE:
        stipplenum =
          this->coinstate.transptype == SoGLRenderAction::SCREEN_DOOR ?
          this->coinstate.stipplenum : 0;

        if (stipplenum != this->glstate.stipplenum) {
          this->sendTransparency(stipplenum);
        }
        break;
      case VERTEXORDERING_CASE:
        if (this->glstate.vertexordering != this->coinstate.vertexordering) {
          this->sendVertexOrdering(this->coinstate.vertexordering);
        }
        break;
      case CULLING_CASE:
        if (this->glstate.culling != this->coinstate.culling) {
          this->sendBackfaceCulling(this->coinstate.culling);
        }
        break;
      case TWOSIDE_CASE:
        if (this->glstate.twoside != this->coinstate.twoside) {
          SoGLShaderProgram * prog = SoGLShaderProgramElement::get((SoState*) stateptr);
          if (prog) prog->updateCoinParameter((SoState*)stateptr, SbName("coin_two_sided_lighting"), this->coinstate.twoside);
          this->sendTwosideLighting(this->coinstate.twoside);
        }
        break;
      case SHADE_MODEL_CASE:
        if (this->glstate.flatshading != this->coinstate.flatshading) {
          this->sendFlatshading(this->coinstate.flatshading);
        }
        break;
      case ALPHATEST_CASE:
        if (this->glstate.alphatestfunc != (int32_t) this->coinstate.alphatestfunc ||
            this->glstate.alphatestvalue != this->coinstate.alphatestvalue) {
            this->sendAlphaTest(this->coinstate.alphatestfunc, this->coinstate.alphatestvalue);
        }
        break;
      }

    }
  }
}

//! FIXME: write doc

void
SoGLLazyElement::sendVPPacked(SoState* COIN_UNUSED_ARG(stateptr), const unsigned char* COIN_UNUSED_ARG(pcolor))
{
  assert(0 && "Not implemented yet. Provided for API compatibility.");
}

/*!
  Reset element GL state (set state to invalid). Use this method to
  notify this element when you use your own GL code that changes the
  OpenGL state.
*/
void
SoGLLazyElement::reset(SoState * stateptr,  uint32_t mask) const
{
  SoGLLazyElement * elem = getInstance(stateptr);

  if (stateptr->isCacheOpen()) {
    elem->cachebitmask |= mask;
  }

  for (int i = 0; (i < LAZYCASES_LAST)&&mask; i++, mask>>=1) {
    if (mask&1) {
      switch (i) {
      case LIGHT_MODEL_CASE:
        elem->glstate.lightmodel = -1;
        break;
      case DIFFUSE_CASE:
        elem->sendPackedDiffuse(0xccccccff);
        break;
      case AMBIENT_CASE:
        elem->glstate.ambient = SbColor(-1.f, -1.0f, -1.0f);
        break;
      case SPECULAR_CASE:
        elem->glstate.specular = SbColor(-1.0f, -1.0f, -1.0f);
        break;
      case EMISSIVE_CASE:
        elem->glstate.emissive = SbColor(-1.0f, -1.0f, -1.0f);
        break;
      case SHININESS_CASE:
        elem->glstate.shininess = -1.0f;
        break;
      case BLENDING_CASE:
        elem->glstate.blending = -1;
        elem->glstate.blend_sfactor = -1;
        elem->glstate.blend_dfactor = -1;
        elem->glstate.alpha_blend_sfactor = -1;
        elem->glstate.alpha_blend_dfactor = -1;
        break;
      case TRANSPARENCY_CASE:
        elem->glstate.stipplenum = -1;
        break;
      case VERTEXORDERING_CASE:
        elem->glstate.vertexordering = -1;
        break;
      case CULLING_CASE:
        elem->glstate.culling = -1;
        break;
      case TWOSIDE_CASE:
        elem->glstate.twoside = -1;
        break;
      case SHADE_MODEL_CASE:
        elem->glstate.flatshading = -1;
        break;
      case ALPHATEST_CASE:
        elem->glstate.alphatestfunc = -1;
        elem->glstate.alphatestvalue = -1.0f;
        break;
      }
    }
  }
}

void
SoGLLazyElement::sendPackedDiffuse(SoState * state, const uint32_t diffuse)
{
  SbBool cacheopen = state->isCacheOpen();
  SoGLLazyElement * elem = getInstance(state);
  if (elem->glstate.diffuse != diffuse) {
    elem->sendPackedDiffuse(diffuse);
    if (cacheopen) elem->lazyDidSet(DIFFUSE_MASK|TRANSPARENCY_MASK);
  }
  else if (cacheopen) {
    elem->lazyDidntSet(DIFFUSE_MASK|TRANSPARENCY_MASK);
  }
}

void
SoGLLazyElement::sendLightModel(SoState * state, const int32_t model)
{
  SbBool cacheopen = state->isCacheOpen();
  SoGLLazyElement * elem = getInstance(state);
  if (elem->glstate.lightmodel != model) {
    elem->sendLightModel(model);
    if (cacheopen) elem->lazyDidSet(LIGHT_MODEL_MASK);
  }
  else if (cacheopen) {
    elem->lazyDidntSet(LIGHT_MODEL_MASK);
  }
}

void
SoGLLazyElement::sendFlatshading(SoState * state, const SbBool onoff)
{
  SbBool cacheopen = state->isCacheOpen();
  SoGLLazyElement * elem = getInstance(state);
  if (elem->glstate.flatshading != onoff) {
    elem->sendFlatshading(onoff);
    if (cacheopen) elem->lazyDidSet(SHADE_MODEL_MASK);
  }
  else if (cacheopen) {
    elem->lazyDidntSet(SHADE_MODEL_MASK);
  }
}

void
SoGLLazyElement::sendVertexOrdering(SoState * state, const VertexOrdering ordering)
{
  SbBool cacheopen = state->isCacheOpen();
  SoGLLazyElement * elem = getInstance(state);
  if (elem->glstate.vertexordering != (int32_t) ordering) {
    elem->sendVertexOrdering(ordering);
    if (cacheopen) elem->lazyDidSet(VERTEXORDERING_MASK);
  }
  else if (cacheopen) {
    elem->lazyDidntSet(VERTEXORDERING_MASK);
  }
}

void
SoGLLazyElement::sendTwosideLighting(SoState * state, const SbBool onoff)
{
  SbBool cacheopen = state->isCacheOpen();
  SoGLLazyElement * elem = getInstance(state);
  if (elem->glstate.twoside != (int32_t) onoff) {
    elem->sendTwosideLighting(onoff);
    if (cacheopen) elem->lazyDidSet(TWOSIDE_MASK);
  }
  else if (cacheopen) {
    elem->lazyDidntSet(TWOSIDE_MASK);
  }
}

void
SoGLLazyElement::sendBackfaceCulling(SoState * state, const SbBool onoff)
{
  SbBool cacheopen = state->isCacheOpen();
  SoGLLazyElement * elem = getInstance(state);
  if (elem->glstate.culling != (int32_t) onoff) {
    elem->sendBackfaceCulling(onoff);
    if (cacheopen) elem->lazyDidSet(CULLING_MASK);
  }
  else if (cacheopen) {
    elem->lazyDidntSet(CULLING_MASK);
  }
}

void
SoGLLazyElement::setDiffuseElt(SoNode * node,  int32_t numcolors,
                               const SbColor * colors, SoColorPacker * packer)
{
  inherited::setDiffuseElt(node, numcolors, colors, packer);
  this->colorpacker = packer;
}

void
SoGLLazyElement::setPackedElt(SoNode * node, int32_t numcolors,
                              const uint32_t * colors, const SbBool packedtransparency)
{
  inherited::setPackedElt(node, numcolors, colors, packedtransparency);
  this->colorpacker = NULL;
  this->packedpointer = colors;
}

void
SoGLLazyElement::setColorIndexElt(SoNode * node, int32_t numindices,
                                  const int32_t * indices)
{
  inherited::setColorIndexElt(node, numindices, indices);
}

void
SoGLLazyElement::setTranspElt(SoNode * node, int32_t numtransp,
                              const float * transp, SoColorPacker * packer)
{
  inherited::setTranspElt(node, numtransp, transp, packer);
  this->colorpacker = packer;
}


void
SoGLLazyElement::setTranspTypeElt(int32_t type)
{
  inherited::setTranspTypeElt(type);
  this->transpmask = type == SoGLRenderAction::SCREEN_DOOR ? 0xff : 0x00;
}

void
SoGLLazyElement::setAmbientElt(const SbColor* color)
{
  inherited::setAmbientElt(color);
}

void
SoGLLazyElement::setEmissiveElt(const SbColor* color)
{
  inherited::setEmissiveElt(color);
}

void
SoGLLazyElement::setSpecularElt(const SbColor* color)
{
  inherited::setSpecularElt(color);
}

void
SoGLLazyElement::setShininessElt(float value)
{
  inherited::setShininessElt(value);
}

void
SoGLLazyElement::setColorMaterialElt(SbBool value)
{
  inherited::setColorMaterialElt(value);
}

void
SoGLLazyElement::enableBlendingElt(int sfactor, int dfactor, int alpha_sfactor, int alpha_dfactor)
{
  inherited::enableBlendingElt(sfactor, dfactor, alpha_sfactor, alpha_dfactor);
}

void
SoGLLazyElement::disableBlendingElt(void)
{
  inherited::disableBlendingElt();
}

void
SoGLLazyElement::setLightModelElt(SoState * stateptr, int32_t model)
{
  inherited::setLightModelElt(stateptr, model);
}

void
SoGLLazyElement::setMaterialElt(SoNode * node, uint32_t bitmask,
                                SoColorPacker * packer,
                                const SbColor * diffuse, const int numdiffuse,
                                const float * transp, const int numtransp,
                                const SbColor & ambient,
                                const SbColor & emissive,
                                const SbColor & specular,
                                const float shininess,
                                const SbBool istransparent)
{
  inherited::setMaterialElt(node, bitmask,
                            packer, diffuse, numdiffuse,
                            transp, numtransp, ambient,
                            emissive, specular, shininess, istransparent);
  this->colorpacker = packer;
}

void
SoGLLazyElement::setVertexOrderingElt(VertexOrdering ordering)
{
  inherited::setVertexOrderingElt(ordering);
}

void
SoGLLazyElement::setBackfaceCullingElt(SbBool onoff)
{
  inherited::setBackfaceCullingElt(onoff);
}

void
SoGLLazyElement::setTwosideLightingElt(SbBool onoff)
{
  inherited::setTwosideLightingElt(onoff);
}

void
SoGLLazyElement::setShadeModelElt(SbBool flatshading)
{
  inherited::setShadeModelElt(flatshading);
}

void
SoGLLazyElement::setAlphaTestElt(int func, float value)
{
  inherited::setAlphaTestElt(func, value);
}

void
SoGLLazyElement::packColors(SoColorPacker * packer) const
{
  const int n = this->coinstate.numdiffuse;
  const SbColor * diffuse = this->coinstate.diffusearray;
  const int numtransp = this->coinstate.numtransp;
  const float * transp = this->coinstate.transparray;

  if (packer->getSize() < n) packer->reallocate(n);
  uint32_t * ptr = packer->getPackedColors();

  int ti = 0;

  for (int i = 0; i < n; i++) {
    ptr[i] = diffuse[i].getPackedValue(transp[ti]);
    if (ti < numtransp-1) ti++;
  }

  packer->setNodeIds(this->coinstate.diffusenodeid,
                     this->coinstate.transpnodeid);
}

void
SoGLLazyElement::beginCaching(SoState * state, GLState * prestate,
                              GLState * poststate)
{
  SoGLLazyElement * elem = getInstance(state);
  elem->send(state, ALL_MASK); // send lazy state before starting to build cache
  *prestate = elem->glstate; // copy current GL state
  prestate->diffusenodeid = elem->coinstate.diffusenodeid;
  prestate->transpnodeid = elem->coinstate.transpnodeid;
  elem->precachestate = prestate;
  elem->postcachestate = poststate;
  elem->precachestate->cachebitmask = 0;
  elem->postcachestate->cachebitmask = 0;
  elem->didsetbitmask = 0;
  elem->didntsetbitmask = 0;
  elem->cachebitmask = 0;
  elem->opencacheflags = 0;
}

void
SoGLLazyElement::endCaching(SoState * state)
{
  SoGLLazyElement * elem = getInstance(state);

  *elem->postcachestate = elem->glstate;
  elem->postcachestate->cachebitmask = elem->cachebitmask;
  elem->precachestate->cachebitmask = elem->didntsetbitmask;

  // unset diffuse mask since it's used by the dependency test
  elem->precachestate->cachebitmask &= ~DIFFUSE_MASK;

  // set diffuse mask if this cache depends on a material outside the
  // cache.
  if (elem->opencacheflags & FLAG_DIFFUSE_DEPENDENCY) {
    elem->precachestate->cachebitmask |= DIFFUSE_MASK;
  }

  elem->precachestate = NULL;
  elem->postcachestate = NULL;
  elem->opencacheflags = 0;
}

void
SoGLLazyElement::postCacheCall(const SoState * state, const GLState * poststate)
{
  SoGLLazyElement * elem = getInstance(state);
  uint32_t mask = poststate->cachebitmask;

  for (int i = 0; (i < LAZYCASES_LAST)&&mask; i++, mask>>=1) {
    if (mask&1) {
      switch (i) {
      case LIGHT_MODEL_CASE:
        elem->glstate.lightmodel = poststate->lightmodel;
        break;
      case DIFFUSE_CASE:
        elem->glstate.diffuse = poststate->diffuse;
        break;
      case AMBIENT_CASE:
        elem->glstate.ambient = poststate->ambient;
        break;
      case SPECULAR_CASE:
        elem->glstate.specular = poststate->specular;
        break;
      case EMISSIVE_CASE:
        elem->glstate.emissive = poststate->emissive;
        break;
      case SHININESS_CASE:
        elem->glstate.shininess = poststate->shininess;
        break;
      case BLENDING_CASE:
        elem->glstate.blending = poststate->blending;
        elem->glstate.blend_sfactor = poststate->blend_sfactor;
        elem->glstate.blend_dfactor = poststate->blend_dfactor;
        elem->glstate.alpha_blend_sfactor = poststate->alpha_blend_sfactor;
        elem->glstate.alpha_blend_dfactor = poststate->alpha_blend_dfactor;
        break;
      case TRANSPARENCY_CASE:
        elem->glstate.stipplenum = poststate->stipplenum;
        break;
      case VERTEXORDERING_CASE:
        elem->glstate.vertexordering = poststate->vertexordering;
        break;
      case CULLING_CASE:
        elem->glstate.culling = poststate->culling;
        break;
      case TWOSIDE_CASE:
        elem->glstate.twoside = poststate->twoside;
        break;
      case SHADE_MODEL_CASE:
        elem->glstate.flatshading = poststate->flatshading;
        break;
      case ALPHATEST_CASE:
        elem->glstate.alphatestfunc = poststate->alphatestfunc;
        elem->glstate.alphatestvalue = poststate->alphatestvalue;
        break;
      }
    }
  }
}

SbBool
SoGLLazyElement::preCacheCall(const SoState * state, const GLState * prestate)
{
  SoGLLazyElement * elem = getInstance(state);

  struct CoinState & curr = elem->coinstate;
  uint32_t mask = prestate->cachebitmask;

  for (int i = 0; (i < LAZYCASES_LAST)&&mask; i++, mask>>=1) {
    if (mask&1) {
      switch (i) {
      case LIGHT_MODEL_CASE:
        if (curr.lightmodel != prestate->lightmodel) {
          GLLAZY_DEBUG("light model failed");
          return FALSE;
        }
        break;
      case DIFFUSE_CASE:
        // this is a special case, since we can have multiple diffuse
        // and transparency values. Check the node ids.
        if ((prestate->diffusenodeid != curr.diffusenodeid) ||
            (prestate->transpnodeid != curr.transpnodeid)) {
          GLLAZY_DEBUG("material dependency failed");
          return FALSE;
        }
        break;
      case AMBIENT_CASE:
        if (curr.ambient != prestate->ambient) {
          GLLAZY_DEBUG("ambient failed");
          return FALSE;
        }
        break;
      case SPECULAR_CASE:
        if (curr.specular != prestate->specular) {
          GLLAZY_DEBUG("specular failed");
          return FALSE;
        }
        break;
      case EMISSIVE_CASE:
        if (curr.emissive != prestate->emissive) {
          GLLAZY_DEBUG("emissive failed");
          return FALSE;
        }
        break;
      case SHININESS_CASE:
        if (curr.shininess != prestate->shininess) {
          GLLAZY_DEBUG("shininess failed");
          return FALSE;
        }
        break;
      case BLENDING_CASE:
        if (curr.blending != prestate->blending) {
          GLLAZY_DEBUG("blending failed");
          return FALSE;
        }
        if (prestate->blending) {
          if (curr.blend_sfactor != prestate->blend_sfactor ||
              curr.blend_dfactor != prestate->blend_dfactor ||
              curr.alpha_blend_sfactor != prestate->alpha_blend_sfactor ||
              curr.alpha_blend_dfactor != prestate->alpha_blend_dfactor) {
            GLLAZY_DEBUG("blending failed");
            return FALSE;
          }
        }
        break;
      case TRANSPARENCY_CASE:
        if (curr.stipplenum != prestate->stipplenum) {
          GLLAZY_DEBUG("transparency failed");
          return FALSE;
        }
        break;
      case VERTEXORDERING_CASE:
        if (curr.vertexordering != prestate->vertexordering) {
          GLLAZY_DEBUG("vertexordering failed");
          return FALSE;
        }
        break;
      case CULLING_CASE:
        if (curr.culling != prestate->culling) {
          GLLAZY_DEBUG("culling failed");
          return FALSE;
        }
        break;
      case TWOSIDE_CASE:
        if (curr.twoside != prestate->twoside) {
          GLLAZY_DEBUG("twoside failed");
          return FALSE;
        }
        break;
      case SHADE_MODEL_CASE:
        if (curr.flatshading != prestate->flatshading) {
          GLLAZY_DEBUG("shade model failed");
          return FALSE;
        }
        break;
      case ALPHATEST_CASE:
        if (curr.alphatestfunc != prestate->alphatestfunc ||
            curr.alphatestvalue != prestate->alphatestvalue) {
          GLLAZY_DEBUG("alphatest failed");
          return FALSE;
        }
        break;
      }
    }
  }
  return TRUE;
}


void
SoGLLazyElement::lazyDidSet(uint32_t mask)
{
  if (mask & DIFFUSE_MASK) {
    if (!(this->didsetbitmask & DIFFUSE_MASK)) {
      // to be safe, always send first diffuse when a cache is open
      this->opencacheflags |= FLAG_FORCE_DIFFUSE;
    }
  }
  this->didsetbitmask |= mask;
}

void
SoGLLazyElement::lazyDidntSet(uint32_t mask)
{
  if (mask & DIFFUSE_MASK) {
    if (!(this->didsetbitmask & DIFFUSE_MASK)) {
      // to be safe, always send first diffuse when a cache is open
      this->didsetbitmask |= DIFFUSE_MASK;
      this->opencacheflags = FLAG_FORCE_DIFFUSE;
    }
  }
  this->didntsetbitmask |= mask&(~this->didsetbitmask);
}

void
SoGLLazyElement::updateColorVBO(SoVBO * vbo)
{
  if (this->colorpacker) {
    SbUniqueId maxid = this->colorpacker->getDiffuseId();
    SbUniqueId tid = this->colorpacker->getTranspId();
    if (tid > maxid) {
      maxid = tid;
    }
    SbUniqueId vboid = vbo->getBufferDataId();
    if (vboid != maxid) {
      const int n = this->coinstate.numdiffuse;
      // need to update the VBO
      const uint32_t * src = this->colorpacker->getPackedColors();
      if (coin_host_get_endianness() == COIN_HOST_IS_BIGENDIAN) {
        vbo->setBufferData(src, n * sizeof(uint32_t),
                           maxid);
      }
      else {
        uint32_t * dst = (uint32_t*)
          vbo->allocBufferData(n * sizeof(uint32_t),
                               maxid);
        for (int i = 0; i < n; i++) {
          uint32_t tmp = src[i];
          dst[i] =
            (tmp << 24) |
            ((tmp & 0xff00) << 8) |
            ((tmp & 0xff0000) >> 8) |
            (tmp >> 24);
        }
      }
    }
  }
}

/*!
  Merge cache info from a child cache (when doing nested caching)
  into the current cache.
*/
void
SoGLLazyElement::mergeCacheInfo(SoState * state,
                                SoGLLazyElement::GLState * childprestate,
                                SoGLLazyElement::GLState * childpoststate)
{
  SoGLLazyElement * elt = SoGLLazyElement::getInstance(state);

  // just add pre-dependencies from child cache
  elt->lazyDidntSet(childprestate->cachebitmask);

  // update current element's didsetbitmask
  elt->lazyDidSet(childpoststate->cachebitmask);

  // also update cachebitmask so that the current cache knows about the changes
  // done by the child cache
  elt->cachebitmask |= childpoststate->cachebitmask;
}

#undef FLAG_FORCE_DIFFUSE
#undef FLAG_DIFFUSE_DEPENDENCY
#undef GLLAZY_DEBUG
