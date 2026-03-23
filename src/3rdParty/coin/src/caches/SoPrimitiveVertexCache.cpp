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
  \class SoPrimitiveVertexCache SoPrimitiveVertexCache.h Inventor/caches/SoPrimitiveVertexCache.h
  The SoPrimitiveVertexClass is used to cache generated triangles.
*/

/*!
  \class SoPrimitiveVertexCache SoPrimitiveVertexCache.h Inventor/caches/SoPrimitiveVertexCache.h
  \brief This cache contains an organized version of the geometry in vertex array form.

  \ingroup coin_caches

  \since Coin 3.0
*/

// *************************************************************************

#include <Inventor/caches/SoPrimitiveVertexCache.h>

#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <Inventor/C/glue/gl.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/elements/SoBumpMapCoordinateElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoCacheHintElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/system/gl.h>
#include <Inventor/SbPlane.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoGLDriverDatabase.h>

#include "tidbitsp.h"
#include "misc/SbHash.h"
#include "rendering/SoGL.h"
#include "rendering/SoVBO.h"
#include "rendering/SoVertexArrayIndexer.h"
#include "SbBasicP.h"

// *************************************************************************

class SoPrimitiveVertexCacheP {
public:
  SoPrimitiveVertexCacheP(void)
    : vertexlist(256),
      normallist(256),
      texcoordlist(256),
      bumpcoordlist(256),
      rgbalist(256),
      tangentlist(256),
      vhash(1024),
      deptharray(NULL),
      triangleindexer(NULL),
      lineindexer(NULL),
      pointindexer(NULL),
      vertexvbo(NULL),
      normalvbo(NULL),
      texcoord0vbo(NULL),
      rgbavbo(NULL),
      tangentvbo(NULL)
  { }
  ~SoPrimitiveVertexCacheP()
  {
    delete triangleindexer;
    delete lineindexer;
    delete pointindexer;
    delete vertexvbo;
    delete normalvbo;
    delete texcoord0vbo;
    delete rgbavbo;
    delete tangentvbo;

    for (int i = 0; i < multitexvbo.getLength(); i++) {
      delete multitexvbo[i];
    }
    if (lastenabled >= 1) {
      delete[] multitexcoords;
    }
    delete[] deptharray;
  }

  class Vertex {
  public:
    SbVec3f vertex;
    SbVec3f normal;
    SbVec4f texcoord0;
    SbVec2f bumpcoord;
    uint8_t rgba[4];
    int texcoordidx;

    // needed for SbHash
    operator unsigned long(void) const;

    // needed, since if we don't add this the unsigned long operator
    // will be used when comparing two vertices.
    int operator==(const Vertex & v);
  };

  SbList <Vertex> vertices;

  SbList <SbVec3f> vertexlist;
  SbList <SbVec3f> normallist;
  SbList <SbVec4f> texcoordlist;
  SbList <SbVec2f> bumpcoordlist;
  SbList <uint8_t> rgbalist;
  SbList <SbVec3f> tangentlist;
  SbHash<Vertex, int32_t> vhash;

  const SbVec2f * bumpcoords;
  int numbumpcoords;

  const uint32_t * packedptr;
  const SbColor * diffuseptr;
  const float * transpptr;

  const SoLazyElement * lazyelement;
  int numdiffuse;
  int numtransp;
  int prevfaceidx;
  SbBool colorpervertex;
  uint32_t firstcolor;

  const SbBool * enabledunits;
  int lastenabled;
  const SoMultiTextureCoordinateElement * multielem;
  SbList <SbVec4f> * multitexcoords;
  SoState * state;
  SbPlane prevsortplane;
  float * deptharray;

  SoVertexArrayIndexer * triangleindexer;
  SoVertexArrayIndexer * lineindexer;
  SoVertexArrayIndexer * pointindexer;
  SoVBO * vertexvbo;
  SoVBO * normalvbo;
  SoVBO * texcoord0vbo;
  SoVBO * rgbavbo;
  SoVBO * tangentvbo;
  SbList <SoVBO*> multitexvbo;

  SoGLLazyElement::GLState prestate;
  SoGLLazyElement::GLState poststate;

  void addVertex(const Vertex & v);

  void renderImmediate(const cc_glglue * glue,
                       const GLint * indices,
                       const int numindices,
                       const SbBool color, const SbBool normal,
                       const SbBool texture, const SbBool * enabled,
                       const int lastenabled);

  void enableArrays(const cc_glglue * glue,
                    const SbBool color, const SbBool normal,
                    const SbBool texture, const SbBool * enabled,
                    const int lastenabled);

  void disableArrays(const cc_glglue * glue,
                     const SbBool color, const SbBool normal,
                     const SbBool texture, const SbBool * enabled,
                     const int lastenabled);

  void enableVBOs(const cc_glglue * glue,
                  const uint32_t contextid,
                  const SbBool color, const SbBool normal,
                  const SbBool texture, const SbBool * enabled,
                  const int lastenabled);

  void disableVBOs(const cc_glglue * glue,
                   const SbBool color, const SbBool normal,
                   const SbBool texture, const SbBool * enabled,
                   const int lastenabled);

  unsigned long countVBOSize(const cc_glglue * glue,
                             const uint32_t contextid,
                             const SbBool color, const SbBool normal,
                             const SbBool texture, const SbBool * enabled,
                             const int lastenabled);

};

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

namespace {
  // SoGLLazyElement shares the same class stack index as
  // SoLazyElement. Do a type check to test if SoGLLazyElement is
  // enabled but not SoLazyElement
  SbBool SoGLLazyElement_enabled(const SoState * state) {
    if (state->isElementEnabled(SoLazyElement::getClassStackIndex())) {
      return SoLazyElement::getInstance(const_cast<SoState*>(state))->getTypeId().isDerivedFrom(SoGLLazyElement::getClassTypeId());
    }
    return FALSE;
  }
};

// *************************************************************************

/*!
  Constructor.
*/
SoPrimitiveVertexCache::SoPrimitiveVertexCache(SoState * state)
  : SoCache(state)
{
  PRIVATE(this)->state = state;
  const SoBumpMapCoordinateElement * belem =
    SoBumpMapCoordinateElement::getInstance(state);

  PRIVATE(this)->numbumpcoords = belem->getNum();
  PRIVATE(this)->bumpcoords = belem->getArrayPtr();

  SoLazyElement * lelem = SoLazyElement::getInstance(state);

  PRIVATE(this)->numdiffuse = lelem->getNumDiffuse();
  PRIVATE(this)->numtransp = lelem->getNumTransparencies();
  if (lelem->isPacked()) {
    PRIVATE(this)->packedptr = lelem->getPackedPointer();
    PRIVATE(this)->diffuseptr = NULL;
    PRIVATE(this)->transpptr = NULL;
  }
  else {
    PRIVATE(this)->packedptr = NULL;
    PRIVATE(this)->diffuseptr = lelem->getDiffusePointer();
    PRIVATE(this)->transpptr = lelem->getTransparencyPointer();
  }

  // set up variables to test if we need to supply color per vertex
  PRIVATE(this)->colorpervertex = FALSE;

  // just store diffuse color with index 0
  uint32_t col;
  if (PRIVATE(this)->packedptr) {
    col = PRIVATE(this)->packedptr[0];
  }
  else {
    SbColor tmpc = PRIVATE(this)->diffuseptr[0];
    float tmpt = PRIVATE(this)->transpptr[0];
    col = tmpc.getPackedValue(tmpt);
  }
  PRIVATE(this)->firstcolor = col;

  // set up for multi texturing
  PRIVATE(this)->lastenabled = -1;
  PRIVATE(this)->enabledunits =
    SoMultiTextureEnabledElement::getEnabledUnits(state, PRIVATE(this)->lastenabled);
  PRIVATE(this)->multielem = NULL;
  PRIVATE(this)->multitexcoords = NULL;
  if (PRIVATE(this)->lastenabled >= 1) {
    PRIVATE(this)->multitexcoords = new SbList<SbVec4f>[PRIVATE(this)->lastenabled+1];
    // delay fetching SoMultiTextureCoordinateElement until the first
    // triangle callback. SoTextureCoordinateBundle might push a new
    // element.
  }
#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoPrimitiveVertexCache::SoPrimitiveVertexCache",
                           "Cache constructed: %p", this);

  }
#endif // debug

  if (SoGLLazyElement_enabled(state)) {
    SoGLLazyElement::beginCaching(state, &PRIVATE(this)->prestate, &PRIVATE(this)->poststate);
  }
}

/*!
  Destructor.
*/
SoPrimitiveVertexCache::~SoPrimitiveVertexCache()
{
#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoPrimitiveVertexCache::~SoPrimitiveVertexCache",
                           "Cache destructed: %p", this);

  }
#endif // debug
}

SbBool 
SoPrimitiveVertexCache::isValid(const SoState * state) const
{
  if (SoGLLazyElement_enabled(state)) {
    if (!SoGLLazyElement::preCacheCall(state , &PRIVATE(this)->prestate)) return FALSE;
  }
  return inherited::isValid(state);
}

/*!
  Closes the cache after it is created. Takes care of SoGLLazyElement synchronization.
*/
void 
SoPrimitiveVertexCache::close(SoState * state)
{
  if (SoGLLazyElement_enabled(state)) {
    SoGLLazyElement::endCaching(state);
  }
  this->fit();
}

void
SoPrimitiveVertexCache::renderTriangles(SoState * state, const int arrays) const
{
  int lastenabled = -1;
  const int n = this->getNumTriangleIndices();
  if (n == 0) return;

  const SbBool * enabled = NULL;
  const SbBool normal = (arrays & NORMAL) != 0;
  const SbBool texture = (arrays & TEXCOORD) != 0;
  const SbBool color = this->colorPerVertex() && ((arrays & COLOR) != 0);
  if (texture) {
    enabled = SoMultiTextureEnabledElement::getEnabledUnits(state, lastenabled);
  }

  const uint32_t contextid = SoGLCacheContextElement::get(state);
  const cc_glglue * glue = cc_glglue_instance(static_cast<int>(contextid));

  SbBool renderasvbo =
    PRIVATE(this)->vertexvbo ||
    SoGLVBOElement::shouldCreateVBO(state, PRIVATE(this)->vertexlist.getLength());

  if (renderasvbo) {
    if (!SoGLDriverDatabase::isSupported(glue, SO_GL_VBO_IN_DISPLAYLIST)) {
      SoCacheElement::invalidate(state);
      SoGLCacheContextElement::shouldAutoCache(state,
                                               SoGLCacheContextElement::DONT_AUTO_CACHE);
    }

    SoPrimitiveVertexCacheP * thisp = const_cast<SoPrimitiveVertexCacheP *>(&PRIVATE(this).get());

    thisp->enableVBOs(glue, contextid, color, normal, texture, enabled, lastenabled);
    PRIVATE(this)->triangleindexer->render(state, TRUE, contextid);
    thisp->disableVBOs(glue, color, normal, texture, enabled, lastenabled);
  }
  else if (SoGLDriverDatabase::isSupported(glue, SO_GL_VERTEX_ARRAY)) {
    SoPrimitiveVertexCacheP * thisp = const_cast<SoPrimitiveVertexCacheP *>(&PRIVATE(this).get());
    thisp->enableArrays(glue, color, normal, texture, enabled, lastenabled);
    PRIVATE(this)->triangleindexer->render(state, FALSE, contextid);
    thisp->disableArrays(glue, color, normal, texture, enabled, lastenabled);
  }
  else {
    // fall back to immediate mode rendering
    SoPrimitiveVertexCacheP * thisp = const_cast<SoPrimitiveVertexCacheP *>(&PRIVATE(this).get());
    glBegin(GL_TRIANGLES);
    thisp->renderImmediate(glue,
                           this->getTriangleIndices(),
                           this->getNumTriangleIndices(),
                           color, normal, texture, enabled, lastenabled);
    glEnd();
  }

  // inform SoGLLazyElement that we might have changed the current color
  if (color) {
    SoGLLazyElement::getInstance(state)->reset(state,
                                               SoLazyElement::DIFFUSE_MASK);
  }
}

void
SoPrimitiveVertexCache::renderLines(SoState * state, const int arrays) const
{
  // FIXME: VBO support for lines, pederb 2004-02-24
  int lastenabled = -1;
  const int n = this->getNumLineIndices();
  if (n == 0) return;
  const SbBool * enabled = NULL;
  const SbBool normal = (arrays & NORMAL) != 0;
  const SbBool texture = (arrays & TEXCOORD) != 0;
  SbBool color = this->colorPerVertex() && ((arrays & COLOR) != 0);
  if (texture) {
    enabled = SoMultiTextureEnabledElement::getEnabledUnits(state, lastenabled);
  }
  const cc_glglue * glue = sogl_glue_instance(state);
  const uint32_t contextid = SoGLCacheContextElement::get(state);

  if (SoGLDriverDatabase::isSupported(glue, SO_GL_VERTEX_ARRAY)) {
    SoPrimitiveVertexCacheP * thisp = const_cast<SoPrimitiveVertexCacheP *>(&PRIVATE(this).get());
    thisp->enableArrays(glue, color, normal, texture, enabled, lastenabled);
    PRIVATE(this)->lineindexer->render(state, FALSE, contextid);
    thisp->disableArrays(glue, color, normal, texture, enabled, lastenabled);
  }
  else {
    // fall back to immediate mode rendering
    SoPrimitiveVertexCacheP * thisp = const_cast<SoPrimitiveVertexCacheP *>(&PRIVATE(this).get());
    glBegin(GL_LINES);
    thisp->renderImmediate(glue,
                           this->getLineIndices(),
                           this->getNumLineIndices(),
                           color, normal, texture, enabled, lastenabled);
    glEnd();
  }
  // inform SoGLLazyElement that we might have changed the current color
  if (color) {
    SoGLLazyElement::getInstance(state)->reset(state,
                                               SoLazyElement::DIFFUSE_MASK);
  }
}

void
SoPrimitiveVertexCache::renderPoints(SoState * state, const int arrays) const
{
  // FIXME: VBO support for points, pederb 2004-02-24
  int lastenabled = -1;
  const int n = this->getNumPointIndices();
  if (n == 0) return;
  const SbBool * enabled = NULL;
  const SbBool normal = (arrays & NORMAL) != 0;
  const SbBool texture = (arrays & TEXCOORD) != 0;
  SbBool color = this->colorPerVertex() && ((arrays & COLOR) != 0);
  if (texture) {
    enabled = SoMultiTextureEnabledElement::getEnabledUnits(state, lastenabled);
  }
  const cc_glglue * glue = sogl_glue_instance(state);
  const uint32_t contextid = SoGLCacheContextElement::get(state);

  if (SoGLDriverDatabase::isSupported(glue, SO_GL_VERTEX_ARRAY)) {
    SoPrimitiveVertexCacheP * thisp = const_cast<SoPrimitiveVertexCacheP *>(&PRIVATE(this).get());
    thisp->enableArrays(glue, color, normal, texture, enabled, lastenabled);
    PRIVATE(this)->pointindexer->render(state, FALSE, contextid);
    thisp->disableArrays(glue, color, normal, texture, enabled, lastenabled);
  }
  else {
    // fall back to immediate mode rendering
    SoPrimitiveVertexCacheP * thisp = const_cast<SoPrimitiveVertexCacheP *>(&PRIVATE(this).get());
    glBegin(GL_POINTS);
    thisp->renderImmediate(glue,
                           this->getPointIndices(),
                           this->getNumPointIndices(),
                           color, normal, texture, enabled, lastenabled);
    glEnd();
  }
  // inform SoGLLazyElement that we might have changed the current color
  if (color) {
    SoGLLazyElement::getInstance(state)->reset(state,
                                               SoLazyElement::DIFFUSE_MASK);
  }
}


void
SoPrimitiveVertexCache::addTriangle(const SoPrimitiveVertex * v0,
                                    const SoPrimitiveVertex * v1,
                                    const SoPrimitiveVertex * v2,
                                    const int * pointdetailidx)
{
  if (PRIVATE(this)->lastenabled >= 1 && PRIVATE(this)->multielem == NULL) {
    // fetch SoMultiTextureCoordinateElement the first time we get here
    PRIVATE(this)->multielem = SoMultiTextureCoordinateElement::getInstance(PRIVATE(this)->state);
  }
  const SoPrimitiveVertex *vp[3] = { v0, v1, v2 };

  int32_t triangleindices[3];

  for (int i = 0; i < 3; i++) {
    SoPrimitiveVertexCacheP::Vertex v;
    v.vertex = vp[i]->getPoint();
    v.normal = vp[i]->getNormal();
    const SbVec4f & tmp = vp[i]->getTextureCoords();
    v.bumpcoord = SbVec2f(tmp[0], tmp[1]);
    v.texcoord0 = tmp;
    v.texcoordidx = -1;

    int midx = vp[i]->getMaterialIndex();
    uint32_t col;
    if (PRIVATE(this)->packedptr) {
      col = PRIVATE(this)->packedptr[SbClamp(midx, 0, PRIVATE(this)->numdiffuse-1)];
    }
    else {
      SbColor tmpc = PRIVATE(this)->diffuseptr[SbClamp(midx,0,PRIVATE(this)->numdiffuse-1)];
      float tmpt = PRIVATE(this)->transpptr[SbClamp(midx,0,PRIVATE(this)->numtransp-1)];
      col = tmpc.getPackedValue(tmpt);
    }
    if (col != PRIVATE(this)->firstcolor) PRIVATE(this)->colorpervertex = TRUE;

    v.rgba[0] = col>>24;
    v.rgba[1] = (col>>16)&0xff;
    v.rgba[2] = (col>>8)&0xff;
    v.rgba[3] = col&0xff;

    const SoDetail * d = coin_safe_cast<const SoDetail *>(vp[i]->getDetail());

    if (d && d->isOfType(SoFaceDetail::getClassTypeId()) && pointdetailidx) {
      const SoFaceDetail * fd = coin_assert_cast<const SoFaceDetail *>(d);
      assert(pointdetailidx[i] < fd->getNumPoints());

      const SoPointDetail * pd = coin_assert_cast<const SoPointDetail *>(
        fd->getPoint(pointdetailidx[i])
       );

      int tidx  = v.texcoordidx = pd->getTextureCoordIndex();
      if (PRIVATE(this)->numbumpcoords) {
        v.bumpcoord = PRIVATE(this)->bumpcoords[SbClamp(tidx, 0, PRIVATE(this)->numbumpcoords-1)];
      }
    }
    int32_t idx;
    if (!PRIVATE(this)->vhash.get(v, idx)) {
      idx = PRIVATE(this)->vertexlist.getLength();
      PRIVATE(this)->vhash.put(v, idx);
      PRIVATE(this)->addVertex(v);
      triangleindices[i] = idx;

      // update texture coordinates for unit 1-n
      for (int j = 1; j <= PRIVATE(this)->lastenabled; j++) {
        if (v.texcoordidx >= 0 &&
            (PRIVATE(this)->multielem->getType(j) == SoMultiTextureCoordinateElement::EXPLICIT)) {
          PRIVATE(this)->multitexcoords[j].append(PRIVATE(this)->multielem->get4(j, v.texcoordidx));
        }
        else if (PRIVATE(this)->multielem->getType(j) == SoMultiTextureCoordinateElement::FUNCTION) {
          PRIVATE(this)->multitexcoords[j].append(PRIVATE(this)->multielem->get(j, v.vertex, v.normal));
        }
        else {
          PRIVATE(this)->multitexcoords[j].append(v.texcoord0);
        }
      }
    }
    else {
      triangleindices[i] = idx;
    }
  }
  if (PRIVATE(this)->triangleindexer == NULL) {
    PRIVATE(this)->triangleindexer = new SoVertexArrayIndexer;
  }
  PRIVATE(this)->triangleindexer->addTriangle(triangleindices[0],
                                              triangleindices[1],
                                              triangleindices[2]);
}

void
SoPrimitiveVertexCache::addLine(const SoPrimitiveVertex * v0,
                                const SoPrimitiveVertex * v1)
{
  if (PRIVATE(this)->lastenabled >= 1 && PRIVATE(this)->multielem == NULL) {
    // fetch SoMultiTextureCoordinateElement the first time we get here
    PRIVATE(this)->multielem = SoMultiTextureCoordinateElement::getInstance(PRIVATE(this)->state);
  }
  const SoPrimitiveVertex *vp[2] = { v0,v1 };

  int32_t lineindices[2];

  for (int i = 0; i < 2; i++) {
    SoPrimitiveVertexCacheP::Vertex v;
    v.vertex = vp[i]->getPoint();
    v.normal = vp[i]->getNormal();
    const SbVec4f & tmp = vp[i]->getTextureCoords();
    v.bumpcoord = SbVec2f(tmp[0], tmp[1]);
    v.texcoord0 = tmp;
    v.texcoordidx = -1;

    int midx = vp[i]->getMaterialIndex();
    uint32_t col;
    if (PRIVATE(this)->packedptr) {
      col = PRIVATE(this)->packedptr[SbClamp(midx, 0, PRIVATE(this)->numdiffuse-1)];
    }
    else {
      SbColor tmpc = PRIVATE(this)->diffuseptr[SbClamp(midx,0,PRIVATE(this)->numdiffuse-1)];
      float tmpt = PRIVATE(this)->transpptr[SbClamp(midx,0,PRIVATE(this)->numtransp-1)];
      col = tmpc.getPackedValue(tmpt);
    }
    if (col != PRIVATE(this)->firstcolor) PRIVATE(this)->colorpervertex = TRUE;

    v.rgba[0] = col>>24;
    v.rgba[1] = (col>>16)&0xff;
    v.rgba[2] = (col>>8)&0xff;
    v.rgba[3] = col&0xff;

    const SoDetail * d = coin_assert_cast<const SoDetail *>(vp[i]->getDetail());

    if (d && d->isOfType(SoLineDetail::getClassTypeId())) {
      const SoLineDetail * ld = coin_assert_cast<const SoLineDetail *>(d);
      const SoPointDetail * pd;
      if (i == 0) pd = ld->getPoint0();
      else pd = ld->getPoint1();

      int tidx  = v.texcoordidx = coin_assert_cast<const SoPointDetail *>(pd)->getTextureCoordIndex();
      if (PRIVATE(this)->numbumpcoords) {
        v.bumpcoord = PRIVATE(this)->bumpcoords[SbClamp(tidx, 0, PRIVATE(this)->numbumpcoords-1)];
      }
    }
    int32_t idx;
    if (!PRIVATE(this)->vhash.get(v, idx)) {
      idx = PRIVATE(this)->vertexlist.getLength();
      PRIVATE(this)->vhash.put(v, idx);
      PRIVATE(this)->addVertex(v);
      lineindices[i] = idx;

      // update texture coordinates for unit 1-n
      for (int j = 1; j <= PRIVATE(this)->lastenabled; j++) {
        if (v.texcoordidx >= 0 &&
            (PRIVATE(this)->multielem->getType(j) == SoMultiTextureCoordinateElement::EXPLICIT)) {
          PRIVATE(this)->multitexcoords[j].append(PRIVATE(this)->multielem->get4(j, v.texcoordidx));
        }
        else if (PRIVATE(this)->multielem->getType(j) == SoMultiTextureCoordinateElement::FUNCTION) {
          PRIVATE(this)->multitexcoords[j].append(PRIVATE(this)->multielem->get(j, v.vertex, v.normal));
        }
        else {
          PRIVATE(this)->multitexcoords[j].append(v.texcoord0);
        }
      }
    }
    else {
      lineindices[i] = idx;
    }
  }
  if (PRIVATE(this)->lineindexer == NULL) {
    PRIVATE(this)->lineindexer = new SoVertexArrayIndexer;
  }
  PRIVATE(this)->lineindexer->addLine(lineindices[0], lineindices[1]);
}

void
SoPrimitiveVertexCache::addPoint(const SoPrimitiveVertex * v0)
{
  if (PRIVATE(this)->lastenabled >= 1 && PRIVATE(this)->multielem == NULL) {
    // fetch SoMultiTextureCoordinateElement the first time we get here
    PRIVATE(this)->multielem = SoMultiTextureCoordinateElement::getInstance(PRIVATE(this)->state);
  }
  SoPrimitiveVertexCacheP::Vertex v;
  v.vertex = v0->getPoint();
  v.normal = v0->getNormal();
  const SbVec4f & tmp = v0->getTextureCoords();
  v.bumpcoord = SbVec2f(tmp[0], tmp[1]);
  v.texcoord0 = tmp;
  v.texcoordidx = -1;

  int midx = v0->getMaterialIndex();
  uint32_t col;
  if (PRIVATE(this)->packedptr) {
    col = PRIVATE(this)->packedptr[SbClamp(midx, 0, PRIVATE(this)->numdiffuse-1)];
  }
  else {
    SbColor tmpc = PRIVATE(this)->diffuseptr[SbClamp(midx,0,PRIVATE(this)->numdiffuse-1)];
    float tmpt = PRIVATE(this)->transpptr[SbClamp(midx,0,PRIVATE(this)->numtransp-1)];
    col = tmpc.getPackedValue(tmpt);
  }
  if (col != PRIVATE(this)->firstcolor) PRIVATE(this)->colorpervertex = TRUE;

  v.rgba[0] = col>>24;
  v.rgba[1] = (col>>16)&0xff;
  v.rgba[2] = (col>>8)&0xff;
  v.rgba[3] = col&0xff;

  const SoDetail * d = coin_assert_cast<const SoDetail *>(v0->getDetail());

  if (d && d->isOfType(SoPointDetail::getClassTypeId())) {
    const SoPointDetail * pd = coin_assert_cast<const SoPointDetail *>(d);
    int tidx  = v.texcoordidx = pd->getTextureCoordIndex();
    if (PRIVATE(this)->numbumpcoords) {
      v.bumpcoord = PRIVATE(this)->bumpcoords[SbClamp(tidx, 0, PRIVATE(this)->numbumpcoords-1)];
    }
  }

  if (PRIVATE(this)->pointindexer == NULL) {
    PRIVATE(this)->pointindexer = new SoVertexArrayIndexer;
  }

  int32_t idx;
  if (!PRIVATE(this)->vhash.get(v, idx)) {
    idx = PRIVATE(this)->vertexlist.getLength();
    PRIVATE(this)->vhash.put(v, idx);
    PRIVATE(this)->addVertex(v);
    PRIVATE(this)->pointindexer->addPoint(idx);

    // update texture coordinates for unit 1-n
    for (int j = 1; j <= PRIVATE(this)->lastenabled; j++) {
      if (v.texcoordidx >= 0 &&
          (PRIVATE(this)->multielem->getType(j) == SoMultiTextureCoordinateElement::EXPLICIT)) {
        PRIVATE(this)->multitexcoords[j].append(PRIVATE(this)->multielem->get4(j, v.texcoordidx));
      }
      else if (PRIVATE(this)->multielem->getType(j) == SoMultiTextureCoordinateElement::FUNCTION) {
        PRIVATE(this)->multitexcoords[j].append(PRIVATE(this)->multielem->get(j, v.vertex, v.normal));
      }
      else {
        PRIVATE(this)->multitexcoords[j].append(v.texcoord0);
      }
    }
  }
  else {
    PRIVATE(this)->pointindexer->addPoint(idx);
  }
}

int
SoPrimitiveVertexCache::getNumVertices(void) const
{
  return PRIVATE(this)->vertexlist.getLength();
}

const SbVec3f *
SoPrimitiveVertexCache::getVertexArray(void) const
{
  return PRIVATE(this)->vertexlist.getArrayPtr();
}

const SbVec3f *
SoPrimitiveVertexCache::getNormalArray(void) const
{
  return PRIVATE(this)->normallist.getArrayPtr();
}

const SbVec4f *
SoPrimitiveVertexCache::getTexCoordArray(void) const
{
  return PRIVATE(this)->texcoordlist.getArrayPtr();
}

const SbVec2f *
SoPrimitiveVertexCache::getBumpCoordArray(void) const
{
  return PRIVATE(this)->bumpcoordlist.getArrayPtr();
}

const uint8_t *
SoPrimitiveVertexCache::getColorArray(void) const
{
  return PRIVATE(this)->rgbalist.getArrayPtr();
}

int
SoPrimitiveVertexCache::getNumTriangleIndices(void) const
{
  return PRIVATE(this)->triangleindexer ? PRIVATE(this)->triangleindexer->getNumIndices() : 0;
}

const GLint *
SoPrimitiveVertexCache::getTriangleIndices(void) const
{
  assert(PRIVATE(this)->triangleindexer);
  return PRIVATE(this)->triangleindexer->getIndices();
}

int32_t
SoPrimitiveVertexCache::getTriangleIndex(const int idx) const
{
  assert(PRIVATE(this)->triangleindexer);
  return PRIVATE(this)->triangleindexer->getIndices()[idx];
}

SbBool
SoPrimitiveVertexCache::colorPerVertex(void) const
{
  return PRIVATE(this)->colorpervertex;
}

const SbVec4f *
SoPrimitiveVertexCache::getMultiTextureCoordinateArray(const int unit) const
{
  assert(unit <= PRIVATE(this)->lastenabled);
  return PRIVATE(this)->multitexcoords[unit].getArrayPtr();
}

int
SoPrimitiveVertexCache::getNumLineIndices(void) const
{
  return PRIVATE(this)->lineindexer ? PRIVATE(this)->lineindexer->getNumIndices() : 0;
}

int
SoPrimitiveVertexCache::getNumPointIndices(void) const
{
  return PRIVATE(this)->pointindexer ? PRIVATE(this)->pointindexer->getNumIndices() : 0;
}


const GLint *
SoPrimitiveVertexCache::getLineIndices(void) const
{
  assert(PRIVATE(this)->lineindexer);
  return PRIVATE(this)->lineindexer->getIndices();
}

const GLint *
SoPrimitiveVertexCache::getPointIndices(void) const
{
  assert(PRIVATE(this)->pointindexer);
  return PRIVATE(this)->pointindexer->getIndices();
}

void
SoPrimitiveVertexCache::fit(void)
{
  PRIVATE(this)->vertexlist.fit();
  PRIVATE(this)->normallist.fit();
  PRIVATE(this)->texcoordlist.fit();
  PRIVATE(this)->bumpcoordlist.fit();
  PRIVATE(this)->rgbalist.fit();
  PRIVATE(this)->vhash.clear();

  if (PRIVATE(this)->triangleindexer) PRIVATE(this)->triangleindexer->close();
  if (PRIVATE(this)->lineindexer) PRIVATE(this)->lineindexer->close();
  if (PRIVATE(this)->pointindexer) PRIVATE(this)->pointindexer->close();
}

void
SoPrimitiveVertexCache::depthSortTriangles(SoState * state)
{
  int numv = PRIVATE(this)->vertexlist.getLength();
  int numtri = this->getNumTriangleIndices() / 3;
  if (numv == 0 || numtri == 0) return;

  SbPlane sortplane = SoViewVolumeElement::get(state).getPlane(0.0);
  // move plane into object space
  sortplane.transform(SoModelMatrixElement::get(state).inverse());

  if (PRIVATE(this)->deptharray == NULL ||
      (sortplane != PRIVATE(this)->prevsortplane)) {
    if (!PRIVATE(this)->deptharray) {
      PRIVATE(this)->deptharray = new float[numtri];
    }
    PRIVATE(this)->prevsortplane = sortplane;
    float * darray = PRIVATE(this)->deptharray;
    const SbVec3f * vptr = PRIVATE(this)->vertexlist.getArrayPtr();
    GLint * iptr = PRIVATE(this)->triangleindexer->getWriteableIndices();
    int i,j;
    for (i = 0; i < numtri; i++) {
      float acc = 0.0;
      for (j = 0; j < 3; j++) {
        acc += sortplane.getDistance(vptr[iptr[i*3+j]]);
      }
      darray[i] = acc / 3.0f;
    }
    int distance;
    float dtmp;
    int itmp[3];

    // shell sort algorithm (O(nlog(n))
    for (distance = 1; distance <= numtri/9; distance = 3*distance + 1) ;
    for (; distance > 0; distance /= 3) {
      for (i = distance; i < numtri; i++) {
        dtmp = darray[i];
        itmp[0] = iptr[i*3];
        itmp[1] = iptr[i*3+1];
        itmp[2] = iptr[i*3+2];
        j = i;
        while (j >= distance && darray[j-distance] > dtmp) {
          darray[j] = darray[j-distance];
          iptr[j*3] = iptr[(j-distance)*3];
          iptr[j*3+1] = iptr[(j-distance)*3+1];
          iptr[j*3+2] = iptr[(j-distance)*3+2];
          j -= distance;
        }
        darray[j] = dtmp;
        iptr[j*3] = itmp[0];
        iptr[j*3+1] = itmp[1];
        iptr[j*3+2] = itmp[2];
      }
    }
  }
}

SoPrimitiveVertexCacheP::Vertex::operator unsigned long(void) const
{
  unsigned long key = 0;
  // create an xor key based on coordinates, normal and texcoords
  const unsigned char * ptr = reinterpret_cast<const unsigned char *>(this);

  // a bit hackish. Stop xor'ing at bumpcoord
  const unsigned char * stop = reinterpret_cast<const unsigned char *>(&this->bumpcoord);
  const ptrdiff_t size = stop-ptr;

  for (int i = 0; i < size; i++) {
    int shift = (i%4) * 8;
    key ^= (ptr[i]<<shift);
  }
  return key;
}

int
SoPrimitiveVertexCacheP::Vertex::operator==(const Vertex & v)
{
  return
    (this->vertex == v.vertex) &&
    (this->normal == v.normal) &&
    (this->texcoord0 == v.texcoord0) &&
    (this->bumpcoord == v.bumpcoord) &&
    (this->texcoordidx == v.texcoordidx) &&
    (this->rgba[0] == v.rgba[0]) &&
    (this->rgba[1] == v.rgba[1]) &&
    (this->rgba[2] == v.rgba[2]) &&
    (this->rgba[3] == v.rgba[3]);
}

void
SoPrimitiveVertexCacheP::addVertex(const Vertex & v)
{
  this->vertexlist.append(v.vertex);
  this->normallist.append(v.normal);
  this->texcoordlist.append(v.texcoord0);
  this->bumpcoordlist.append(v.bumpcoord);
  for (int c = 0; c < 4; c++) {
    this->rgbalist.append(v.rgba[c]);
  }
}

void
SoPrimitiveVertexCacheP::enableArrays(const cc_glglue * glue,
                                      const SbBool color, const SbBool normal,
                                      const SbBool texture, const SbBool * enabled,
                                      const int lastenabled)
{
  int i;
  if (color) {
    cc_glglue_glColorPointer(glue, 4, GL_UNSIGNED_BYTE, 0,
                             reinterpret_cast<const GLvoid *>(this->rgbalist.getArrayPtr()));
    cc_glglue_glEnableClientState(glue, GL_COLOR_ARRAY);
  }

  if (texture) {
    cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0,
                                reinterpret_cast<const GLvoid *>(this->texcoordlist.getArrayPtr()));
    cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);

    for (i = 1; i <= lastenabled; i++) {
      if (enabled[i]) {
        cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0 + i);
        cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0,
                                    reinterpret_cast<const GLvoid *>(this->multitexcoords[i].getArrayPtr()));
        cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);
      }
    }
  }
  if (normal) {
    cc_glglue_glNormalPointer(glue, GL_FLOAT, 0,
                              reinterpret_cast<const GLvoid *>(this->normallist.getArrayPtr()));
    cc_glglue_glEnableClientState(glue, GL_NORMAL_ARRAY);
  }

  cc_glglue_glVertexPointer(glue, 3, GL_FLOAT, 0,
                            reinterpret_cast<const GLvoid *>(this->vertexlist.getArrayPtr()));
  cc_glglue_glEnableClientState(glue, GL_VERTEX_ARRAY);
}


void
SoPrimitiveVertexCacheP::disableArrays(const cc_glglue * glue,
                                       const SbBool color, const SbBool normal,
                                       const SbBool texture, const SbBool * enabled,
                                       const int lastenabled)
{
  int i;
  if (normal) {
    cc_glglue_glDisableClientState(glue, GL_NORMAL_ARRAY);
  }
  if (texture) {
    for (i = 1; i <= lastenabled; i++) {
      if (enabled[i]) {
        cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0 + i);
        cc_glglue_glDisableClientState(glue, GL_TEXTURE_COORD_ARRAY);
      }
    }
    if (lastenabled >= 1) {
      // reset to default
      cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0);
    }
    cc_glglue_glDisableClientState(glue, GL_TEXTURE_COORD_ARRAY);
  }
  if (color) {
    cc_glglue_glDisableClientState(glue, GL_COLOR_ARRAY);
  }
  cc_glglue_glDisableClientState(glue, GL_VERTEX_ARRAY);
}

void
SoPrimitiveVertexCacheP::enableVBOs(const cc_glglue * glue,
                                    uint32_t contextid,
                                    const SbBool color, const SbBool normal,
                                    const SbBool texture, const SbBool * enabled,
                                    const int lastenabled)
{
  int i;
  if (color) {
    if (this->rgbavbo == NULL) {
      this->rgbavbo = new SoVBO;
      this->rgbavbo->setBufferData(this->rgbalist.getArrayPtr(),
                                   this->rgbalist.getLength() * sizeof(uint8_t));
    }
    this->rgbavbo->bindBuffer(contextid);
    cc_glglue_glColorPointer(glue, 4, GL_UNSIGNED_BYTE, 0, NULL);
    cc_glglue_glEnableClientState(glue, GL_COLOR_ARRAY);
  }
  if (texture) {
    if (this->texcoord0vbo == NULL) {
      this->texcoord0vbo = new SoVBO;
      this->texcoord0vbo->setBufferData(this->texcoordlist.getArrayPtr(),
                                        this->texcoordlist.getLength()*4*sizeof(float));
    }
    this->texcoord0vbo->bindBuffer(contextid);
    cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0, NULL);
    cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);

    for (i = 1; i <= lastenabled; i++) {
      while (this->multitexvbo.getLength() <= i) {
        this->multitexvbo.append(NULL);
      }
      if (enabled[i]) {
        if (this->multitexvbo[i] == NULL) {
          SoVBO * vbo = new SoVBO;
          vbo->setBufferData(this->multitexcoords[i].getArrayPtr(),
                             this->multitexcoords[i].getLength()*4*sizeof(float));
          this->multitexvbo[i] = vbo;
        }
        this->multitexvbo[i]->bindBuffer(contextid);
        cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0 + i);
        cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0, NULL);
        cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);
      }
    }
  }
  if (normal) {
    if (this->normalvbo == NULL) {
      this->normalvbo = new SoVBO;
      this->normalvbo->setBufferData(this->normallist.getArrayPtr(),
                                     this->normallist.getLength()*3*sizeof(float));
    }
    this->normalvbo->bindBuffer(contextid);
    cc_glglue_glNormalPointer(glue, GL_FLOAT, 0, NULL);
    cc_glglue_glEnableClientState(glue, GL_NORMAL_ARRAY);
  }

  if (this->vertexvbo == NULL) {
    this->vertexvbo = new SoVBO;
    this->vertexvbo->setBufferData(this->vertexlist.getArrayPtr(),
                                   this->vertexlist.getLength()*3*sizeof(float));
  }
  this->vertexvbo->bindBuffer(contextid);
  cc_glglue_glVertexPointer(glue, 3, GL_FLOAT, 0, NULL);
  cc_glglue_glEnableClientState(glue, GL_VERTEX_ARRAY);
}

void
SoPrimitiveVertexCacheP::disableVBOs(const cc_glglue * glue,
                                     const SbBool color, const SbBool normal,
                                     const SbBool texture, const SbBool * enabled,
                                     const int lastenabled)
{
  this->disableArrays(glue, color, normal, texture, enabled, lastenabled);
  cc_glglue_glBindBuffer(glue, GL_ARRAY_BUFFER, 0); // Reset VBO binding
}

void
SoPrimitiveVertexCacheP::renderImmediate(const cc_glglue * glue,
                                         const GLint * indices,
                                         const int numindices,
                                         const SbBool color, const SbBool normal,
                                         const SbBool texture, const SbBool * enabled,
                                         const int lastenabled)
{
  const unsigned char * colorptr = NULL;
  const SbVec3f * normalptr = NULL;
  const SbVec3f * vertexptr = NULL;
  const SbVec4f * texcoordptr = NULL;

  if (color) {
    colorptr = this->rgbalist.getArrayPtr();
  }
  if (normal) {
    normalptr = this->normallist.getArrayPtr();
  }
  if (texture) {
    texcoordptr = this->texcoordlist.getArrayPtr();
  }
  vertexptr = this->vertexlist.getArrayPtr();

  for (int i = 0; i < numindices; i++) {
    const int idx = indices[i];
    if (normal) {
      glNormal3fv(reinterpret_cast<const GLfloat *>(&normalptr[idx]));
    }
    if (color) {
      glColor3ubv(reinterpret_cast<const GLubyte *>(&colorptr[idx*4]));
    }
    if (texture) {
      glTexCoord4fv(reinterpret_cast<const GLfloat *>(&texcoordptr[idx]));

      for (int j = 1; j <= lastenabled; j++) {
        if (enabled[j]) {
          const SbVec4f * mt = this->multitexcoords[j].getArrayPtr();
          cc_glglue_glMultiTexCoord4fv(glue,
                                       GL_TEXTURE0 + j,
                                       reinterpret_cast<const GLfloat *>(&mt[idx]));
        }
      }
      }
    glVertex3fv(reinterpret_cast<const GLfloat *>(&vertexptr[idx]));
  }
}

#undef PRIVATE
