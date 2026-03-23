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
  \class SoGLBigImage include/Inventor/misc/SoGLBigImage.h
  \brief The SoGLBigImage class is used to handle 2D OpenGL textures of any size.

  This class is internal. To enable/disable big-image texture handling
  you should use the SoTextureScalePolicy node.

  The technique used is the following: split the texture into x*y
  equal size blocks. All these subtextures are of size 2^n, and are
  typically quite small (256x256 or smaller).  Each triangle is
  clipped, based on the texture coordinates, into several smaller
  triangles. The triangles will then be guaranteed to use only one
  subtexture. Then the triangles are projected onto the screen, and
  the maximum projected size for each subtexture is
  calculated. Subtextures outside the viewport will be culled. Each
  subtexture is then sampled down to a 2^n value close to the
  projected size, and a GL texture is created with this size. This GL
  texture is used when rendering triangles that are clipped into that
  subtexture.

  Mipmapping is disabled for SoGLBigImage. Aliasing problems shouldn't
  occur because the projected size of the texture is calculated on the
  fly.  When mipmapping is enabled, the amount of texture memory used
  is doubled, and creating the texture object is much slower, so we
  avoid this for SoGLBigImage.

  \COIN_CLASS_EXTENSION

  \since Coin 2.0
*/

// *************************************************************************

#include <Inventor/misc/SoGLBigImage.h>
#include "coindefs.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/C/threads/storage.h>
#include <Inventor/SbImage.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLDisplayList.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/system/gl.h>

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbMutex.h>
#endif // COIN_THREADSAFE

#include "tidbitsp.h"
#include "rendering/SoGL.h"

// *************************************************************************

// the number of subtextures that can be changed (resized) each frame.
// By keeping this number small, we avoid slow updates when zooming in
// on an image, as only few textures are changed each frame.
static int CHANGELIMIT = 4;

// the texturequality limit when linear filtering will be used
#define LINEAR_LIMIT 0.1f

typedef struct {
  SbVec2s imagesize;
  SbVec2s glimagesize;
  SbVec2s remain;
  SbVec2f tcmul;
  SbVec2s dim;
  SbVec2s currentdim;

  unsigned char * tmpbuf;
  int tmpbufsize;
  SoGLImage ** glimagearray;
  SbImage ** imagearray;
  int * glimagediv;
  uint32_t * glimageage;
  int changecnt;
  unsigned int * averagebuf;
} SoGLBigImageTls;

class SoGLBigImageP {
public:
  SoGLBigImageP(void);
  ~SoGLBigImageP();

  static SoType classTypeId;

  cc_storage * storage;
#ifdef COIN_THREADSAFE
  SbMutex mutex;
#endif // !COIN_THREADSAFE
  unsigned char ** cache;
  SbVec2s * cachesize;
  int numcachelevels;

  // inline for speed
  inline SoGLBigImageTls * getTls(void) {
    return (SoGLBigImageTls*) cc_storage_get(this->storage);
  }

  inline void lock(void) {
#ifdef COIN_THREADSAFE
    this->mutex.lock();
#endif // COIN_THREADSAFE
  }
  inline void unlock(void) {
#ifdef COIN_THREADSAFE
    this->mutex.unlock();
#endif // COIN_THREADSAFE
  }

  void copySubImage(SoGLBigImageTls * tls,
                    const int idx,
                    const unsigned char * src,
                    const SbVec2s & fullsize,
                    const int nc,
                    unsigned char * dst,
                    const int div,
                    const int level);
  void copyResizeSubImage(SoGLBigImageTls * tls,
                          const int idx,
                          const unsigned char * src,
                          const SbVec2s & fullsize,
                          const int nc,
                          unsigned char * dst,
                          const SbVec2s & targetsize);
  void resetAllTls(SoState * state);
  void resetCache(void);
  static void reset(SoGLBigImageTls * tls, SoState * state = NULL);
  static void unrefOldDL(SoGLBigImageTls * tls, SoState * state, const uint32_t maxage);
  void createCache(const unsigned char * bytes, const SbVec2s & size, const int nc);
};

SoType SoGLBigImageP::classTypeId STATIC_SOTYPE_INIT;

static void soglbigimagep_cleanup(void)
{
  SoGLBigImageP::classTypeId STATIC_SOTYPE_INIT;
  CHANGELIMIT = 4;
}

static void
soglbigimagetls_construct(void * closure)
{
  SoGLBigImageTls * storage = (SoGLBigImageTls*) closure;
  storage->imagesize.setValue(0, 0);
  storage->remain.setValue(0, 0);
  storage->dim.setValue(0, 0);
  storage->currentdim.setValue(0, 0);
  storage->tmpbuf = NULL;
  storage->tmpbufsize = 0;
  storage->glimagearray = NULL;
  storage->imagearray = NULL;
  storage->glimagediv = NULL;
  storage->glimageage = NULL;
  storage->averagebuf = NULL;
}

static void
soglbigimagetls_destruct(void * closure)
{
  SoGLBigImageTls * tls = (SoGLBigImageTls*) closure;
  SoGLBigImageP::reset(tls, NULL);

  // these are not destructed in reset()
  delete[] tls->tmpbuf;
  delete[] tls->averagebuf;
}

#define PRIVATE(obj) (obj->pimpl)

// *************************************************************************

/*!
  Constructor.
*/
SoGLBigImage::SoGLBigImage(void)
{
  PRIVATE(this) = new SoGLBigImageP;
}

/*!
  Destructor.
*/
SoGLBigImage::~SoGLBigImage()
{
  PRIVATE(this)->resetCache();
  delete PRIVATE(this);
}

// Doc in superclass.
void
SoGLBigImage::unref(SoState * state)
{
  PRIVATE(this)->resetAllTls(state);
  inherited::unref(state);
}

/*!
  This static method initializes static data for the SoGLBigImage class.
*/
void
SoGLBigImage::initClass(void)
{
  assert(SoGLBigImageP::classTypeId.isBad());
  SoGLBigImageP::classTypeId =
    SoType::createType(SoGLImage::getClassTypeId(), SbName("GLBigImage"));
  coin_atexit((coin_atexit_f*) soglbigimagep_cleanup, CC_ATEXIT_NORMAL);
}

/*!
  This static method returns the SoType object associated with
  objects of this class.
*/
SoType
SoGLBigImage::getClassTypeId(void)
{
  assert(!SoGLBigImageP::classTypeId.isBad());
  return SoGLBigImageP::classTypeId;
}

// Doc in superclass.
SoType
SoGLBigImage::getTypeId(void) const
{
  return SoGLBigImage::getClassTypeId();
}

void
SoGLBigImage::setData(const SbImage * image,
                      const Wrap wraps,
                      const Wrap wrapt,
                      const float quality,
                      const int border,
                      SoState * createinstate)
{
  if (createinstate) {
    SoDebugError::postWarning("SoGLBigImage::setData",
                              "createinstate must be NULL for SoGLBigImage");
  }
  delete PRIVATE(this);
  PRIVATE(this) = new SoGLBigImageP;
  inherited::setData(image, wraps, wrapt, quality, border, NULL);
}

void
SoGLBigImage::setData(const SbImage * image,
                      const Wrap wraps,
                      const Wrap wrapt,
                      const Wrap wrapr,
                      const float quality,
                      const int border,
                      SoState * createinstate)
{
  if (createinstate) {
    SoDebugError::postWarning("SoGLBigImage::setData",
                              "createinstate must be NULL for SoGLBigImage");
  }
  delete PRIVATE(this);
  PRIVATE(this) = new SoGLBigImageP;
  inherited::setData(image, wraps, wrapt, wrapr, quality, border, NULL);
}


SoGLDisplayList *
SoGLBigImage::getGLDisplayList(SoState * COIN_UNUSED_ARG(state))
{
  return NULL;
}

int
SoGLBigImage::initSubImages(const SbVec2s & subimagesize) const
{
  SoGLBigImageTls * tls = PRIVATE(this)->getTls();

  tls->changecnt = 0;
  if (subimagesize == tls->imagesize &&
      tls->dim[0] > 0) return tls->dim[0] * tls->dim[1];

  tls->imagesize = subimagesize;
  tls->glimagesize[0] = coin_geq_power_of_two(tls->imagesize[0]);
  tls->glimagesize[1] = coin_geq_power_of_two(tls->imagesize[1]);

  // FIXME: hardcoding for maximum 265x256 tiles is a bad strategy, as
  // it will often give bad performance vs larger tile sizes.
  //
  // pederb has the following input on this issue:
  // ------------8<--------- [snip] --------------------------8<-----
  // That part of SoGLBigImage should be recoded. We should use a quad
  // tree instead, so that the number of subtextures depends on the
  // needed resolution. Right now the number of subtextures is static
  // for a texture. This can lead to slow rendering when the entire
  // texture is viewed from a long distance.
  //
  // We should also precalculate the triangle clipping done on this
  // quadtree.  This would lead to much faster rendering on models
  // with many triangles (the rendering is very slow per triangle
  // now).
  //
  // As a temporary workaround it might be possible to calculate a new
  // subtexture size, based on the size of the original image. We
  // could base this on maximum number of subtextures to create or
  // something.
  // ------------8<--------- [snip] --------------------------8<-----
  //
  // Note also that there's hardcoding for 256x256 in
  // src/shapenodes/soshape_bigtexture.cpp's beginShape().
  //
  // 20050701 mortene.

  if (tls->glimagesize[0] > tls->imagesize[0] && tls->glimagesize[0] >= 256) {
    int diff = tls->imagesize[0] - (tls->glimagesize[0]>>1);
    float ratio = float(diff) / float(tls->glimagesize[0]>>1);
    if (ratio < 0.3) tls->glimagesize[0] >>= 1;
  }

  if (tls->glimagesize[1] > tls->imagesize[1] && tls->glimagesize[1] >= 256) {
    int diff = tls->imagesize[1] - (tls->glimagesize[1]>>1);
    float ratio = float(diff) / float(tls->glimagesize[1]>>1);
    if (ratio < 0.3) tls->glimagesize[1] >>= 1;
  }

  SbVec2s size(0,0);
  int nc = 0;

  if (this->getImage() != NULL)
    (void)(this->getImage()->getValue(size, nc));

  tls->dim[0] = size[0] / subimagesize[0];
  tls->dim[1] = size[1] / subimagesize[1];

  tls->remain[0] = size[0] % subimagesize[0];
  if (tls->remain[0]) tls->dim[0] += 1;
  tls->remain[1] = size[1] % subimagesize[1];
  if (tls->remain[1]) tls->dim[1] += 1;

  tls->tcmul[0] = float(tls->dim[0] * subimagesize[0]) / float(size[0]);
  tls->tcmul[1] = float(tls->dim[1] * subimagesize[1]) / float(size[1]);
  return tls->dim[0] * tls->dim[1];
}

void
SoGLBigImage::handleSubImage(const int idx,
                             SbVec2f & start,
                             SbVec2f & end,
                             SbVec2f & tcmul)
{
  SoGLBigImageTls * tls = PRIVATE(this)->getTls();

  SbVec2s pos(idx % tls->dim[0], idx / tls->dim[0]);
  start[0] = float(pos[0]) / float(tls->dim[0]);
  start[1] = float(pos[1]) / float(tls->dim[1]);
  end[0] = float(pos[0]+1) / float(tls->dim[0]);
  end[1] = float(pos[1]+1) / float(tls->dim[1]);

  start[0] *= tls->tcmul[0];
  start[1] *= tls->tcmul[1];
  end[0] *= tls->tcmul[0];
  end[1] *= tls->tcmul[1];
  tcmul = tls->tcmul;
}

void
SoGLBigImage::applySubImage(SoState * state, const int idx,
                            const float quality,
                            const SbVec2s & projsize)
{
  SbVec2s size;
  int numcomponents;
  unsigned char * bytes = this->getImage() ?
    this->getImage()->getValue(size, numcomponents) : NULL;

  SoGLBigImageTls * tls = PRIVATE(this)->getTls();

  if (tls->currentdim != tls->dim) {
    SoGLBigImageP::reset(tls, state);
    tls->currentdim = tls->dim;
    const int numimages = tls->dim[0] * tls->dim[1];

    tls->glimagediv = new int[numimages];
    tls->glimagearray = new SoGLImage*[numimages];
    tls->imagearray = new SbImage*[numimages];
    tls->glimageage = new uint32_t[numimages];
    for (int i = 0; i < numimages; i++) {
      tls->glimagearray[i] = NULL;
      tls->imagearray[i] = NULL;
      tls->glimagediv[i] = 1;
      tls->glimageage[i] = 0;
    }

    int numbytes = tls->imagesize[0] * tls->imagesize[1] * numcomponents;
    tls->averagebuf =
      new unsigned int[numbytes ? numbytes : 1];

    // lock before testing/creating cache to avoid race conditions
    PRIVATE(this)->lock();
    if (PRIVATE(this)->cache == NULL) {
      PRIVATE(this)->createCache(bytes, size, numcomponents);
    }
    PRIVATE(this)->unlock();
  }

  int level = 0;
  int div = 2;
  while ((tls->imagesize[0]/div > projsize[0]) &&
         (tls->imagesize[1]/div > projsize[1])) {
    div <<= 1;
    level++;
  }
  div >>= 1;

  if (tls->glimagearray[idx] == NULL ||
      (tls->glimagediv[idx] != div && tls->changecnt < CHANGELIMIT)) {

    if (tls->glimagearray[idx] == NULL) {
      tls->glimagearray[idx] = new SoGLImage();
      if (tls->imagearray[idx] == NULL) {
        tls->imagearray[idx] = new SbImage;
      }
    }
    else {
      tls->changecnt++;
    }
    tls->glimagediv[idx] = div;

    uint32_t flags = this->getFlags();
    flags |= NO_MIPMAP|INVINCIBLE;
    
    if (flags & USE_QUALITY_VALUE) {
      flags &= ~USE_QUALITY_VALUE;
      if (quality >= LINEAR_LIMIT) {
        flags |= LINEAR_MIN_FILTER|LINEAR_MAG_FILTER;
      }
    }
    tls->glimagearray[idx]->setFlags(flags);

    SbVec2s actualsize(tls->glimagesize[0]/div,
                       tls->glimagesize[1]/div);
    if (bytes) {
      int numbytes = actualsize[0]*actualsize[1]*numcomponents;
      if (numbytes > tls->tmpbufsize) {
        delete[] tls->tmpbuf;
        tls->tmpbuf = new unsigned char[numbytes];
        tls->tmpbufsize = numbytes;
      }

      if (tls->glimagesize == tls->imagesize) {
        PRIVATE(this)->copySubImage(tls,
                           idx,
                           bytes,
                           size,
                           numcomponents,
                           tls->tmpbuf, div, level);
      }
      else {
        PRIVATE(this)->copyResizeSubImage(tls,
                                 idx,
                                 bytes,
                                 size,
                                 numcomponents,
                                 tls->tmpbuf,
                                 actualsize);
      }
      tls->imagearray[idx]->setValue(actualsize, numcomponents, tls->tmpbuf);
    }
    else tls->imagearray[idx]->setValuePtr(SbVec2s(0,0), 0, NULL);
    
    // do not create-in-state, since the same thread might be used to
    // render into more than one context
    tls->glimagearray[idx]->setData(tls->imagearray[idx],
                                    SoGLImage::CLAMP_TO_EDGE,
                                    SoGLImage::CLAMP_TO_EDGE,
                                    quality,
                                    0, NULL);
  }

  SoGLDisplayList * dl = tls->glimagearray[idx]->getGLDisplayList(state);
  assert(dl);
  tls->glimageage[idx] = 0;
  SoGLImage::tagImage(state, tls->glimagearray[idx]);
  this->resetAge();
  dl->call(state);
}

/*!
  To avoid doing too much work in one frame, there is a limit on the
  number of subtextures that can be changed each frame. If this limit
  is exceeded, this function will return TRUE, otherwise FALSE.

  \sa setChangeLimit()
*/
SbBool
SoGLBigImage::exceededChangeLimit(void)
{
  return PRIVATE(this)->getTls()->changecnt >= CHANGELIMIT;
}

/*!
  Sets the change limit. Returns the old limit.
  
  \sa exceededChangeLimit()
  \since Coin 2.3
*/
int
SoGLBigImage::setChangeLimit(const int limit)
{
  int old = CHANGELIMIT;
  CHANGELIMIT = limit;
  return old;
}

// needed for cc_storage_apply_to_all() callback
typedef struct {
  uint32_t maxage;
  SoState * state;
} soglbigimage_unrefolddl_data;

// cc_storage_apply_to_all() callback
static void
soglbigimage_unrefolddl_cb(void * tls, void * closure)
{
  soglbigimage_unrefolddl_data * data =
    (soglbigimage_unrefolddl_data *) closure;

  SoGLBigImageP::unrefOldDL((SoGLBigImageTls*)tls, data->state, data->maxage);
}

// Documented in superclass. Overridden to handle age on subimages.
void
SoGLBigImage::unrefOldDL(SoState * state, const uint32_t maxage)
{
  soglbigimage_unrefolddl_data data;
  data.maxage = maxage;
  data.state = state;
  cc_storage_apply_to_all(PRIVATE(this)->storage, soglbigimage_unrefolddl_cb, &data);

  this->incAge();
}

#undef PRIVATE

#ifndef DOXYGEN_SKIP_THIS

SoGLBigImageP::SoGLBigImageP(void) :
  cache(NULL),
  cachesize(NULL),
  numcachelevels(0)
{
  this->storage = cc_storage_construct_etc(sizeof(SoGLBigImageTls),
                                           soglbigimagetls_construct,
                                           soglbigimagetls_destruct);
}

SoGLBigImageP::~SoGLBigImageP()
{
  this->resetCache();
  cc_storage_destruct(this->storage);
}

//  The method copySubImage() handles the downsampling. It averages
//  the full-resolution pixels to create the low resolution image.
void
SoGLBigImageP::copySubImage(SoGLBigImageTls * tls,
                            const int idx,
                            const unsigned char * src,
                            const SbVec2s & fsize,
                            const int nc,
                            unsigned char * dst,
                            const int div,
                            const int level)
{
  if ((div == 1) || (this->cache && level < this->numcachelevels && this->cache[level])) {
    SbVec2s pos(idx % tls->dim[0], idx / tls->dim[0]);

    // FIXME: investigate if it is possible to set the pixel transfer
    // mode so that we don't have to copy the data into a temporary
    // image. This is probably fast enough though.  pederb?.

    int origin[2];
    int fullsize[2];
    int w, h;
    const unsigned char * datasrc;

    if (div == 1) { // use original image
      origin[0] = pos[0] * tls->imagesize[0];
      origin[1] = pos[1] * tls->imagesize[1];

      fullsize[0] = fsize[0];
      fullsize[1] = fsize[1];
      w = tls->imagesize[0];
      h = tls->imagesize[1];
      datasrc = src;
    }
    else { // use cache image
      origin[0] = pos[0] * (tls->imagesize[0] >> level);
      origin[1] = pos[1] * (tls->imagesize[1] >> level);
      fullsize[0] = this->cachesize[level][0];
      fullsize[1] = this->cachesize[level][1];
      w = tls->imagesize[0] >> level;
      h = tls->imagesize[1] >> level;
      datasrc = this->cache[level];
    }

    assert(fullsize[0] > 0 && fullsize[1] > 0);

    // check for fast loop (common case)
    if ((origin[0] + w) < fullsize[0] && (origin[1] + h) < fullsize[1]) {
      for (int y = 0; y < h; y++) {
        int tmpyadd = fullsize[0] * (origin[1]+y);
        for (int x = 0; x < w; x++) {
          const unsigned char * srcptr =
            datasrc + nc * (tmpyadd + origin[0]+x);
          for (int c = 0; c < nc; c++) {
            *dst++ = srcptr[c];
          }
        }
      }
    }
    else { // slower loop (x and y values are clamped)
      for (int y = 0; y < h; y++) {
        int tmpyadd = fullsize[0] * SbClamp(origin[1]+y, 0, fullsize[1]-1);
        for (int x = 0; x < w; x++) {
          const unsigned char * srcptr =
            datasrc + nc * (tmpyadd + SbClamp(origin[0]+x, 0, fullsize[0]-1));
          for (int c = 0; c < nc; c++) {
            *dst++ = srcptr[c];
          }
        }
      }
    }
  }
  else {
    SbVec2s pos(idx % tls->dim[0], idx / tls->dim[0]);

    int origin[2];
    origin[0] = pos[0] * tls->imagesize[0];
    origin[1] = pos[1] * tls->imagesize[1];

    int fullsize[2];
    fullsize[0] = fsize[0];
    fullsize[1] = fsize[1];

    int w = tls->imagesize[0];
    int h = tls->imagesize[1];

    unsigned int mask = (unsigned int) div-1;

    if ((origin[0] + w) > fullsize[0]) {
      w = fullsize[0] - origin[0];
      if (w & mask) {
        w = w - (w & mask);
      }
    }
    if ((origin[1] + h) > fullsize[1]) {
      h = fullsize[1] - origin[1];
      if (h & mask) {
        h = h - (h & mask);
      }
    }

    memset(tls->averagebuf, 0, size_t(w)* size_t(h)* size_t(nc)*sizeof(int) / size_t(div));
    unsigned int * aptr = tls->averagebuf;
    int y;
    for (y = 0; y < h; y++) {
      unsigned int * tmpaptr = aptr;
      const unsigned char * srcptr =
        src + (fullsize[0] * (origin[1]+y) + origin[0]) * nc;
      for (int x = 0; x < w; x++) {
        for (int c = 0; c < nc; c++) {
          aptr[c] += srcptr[c];
        }
        srcptr += nc;
        if (!((x+1) & mask)) aptr += nc;
      }
      if ((y+1) & mask) aptr = tmpaptr;
    }

    aptr = tls->averagebuf;
    int mydiv = div * div;

    int lineadd = tls->imagesize[0] - w;

    lineadd /= div;
    w /= div;
    h /= div;

    for (y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        for (int c = 0; c < nc; c++) {
          dst[c] = (unsigned char) (aptr[c] / mydiv);
        }
        dst += nc;
        aptr += nc;
      }
      dst += lineadd*nc;
    }
  }
}

void
SoGLBigImageP::copyResizeSubImage(SoGLBigImageTls * tls,
                                  const int idx,
                                  const unsigned char * src,
                                  const SbVec2s & fullsize,
                                  const int nc,
                                  unsigned char * dst,
                                  const SbVec2s & targetsize)
{
  SbVec2s pos(idx % tls->dim[0], idx / tls->dim[0]);

  SbVec2s origin;
  origin[0] = pos[0] * tls->imagesize[0];
  origin[1] = pos[1] * tls->imagesize[1];

  int incy = ((tls->imagesize[1]<<8) / targetsize[1]);
  int incx = ((tls->imagesize[0]<<8) / targetsize[0]);

  const int w = targetsize[0];
  const int h = targetsize[1];

  int addy = 0;

  for (int y = 0; y < h; y++) {
    int addx = 0;
    int tmpaddy = ((addy>>8)+origin[1])*fullsize[0]*nc;
    for (int x  = 0; x < w; x++) {
      const unsigned char * ptr = src + tmpaddy + ((addx>>8)+origin[0]) * nc;
      for (int c = 0; c < nc; c++) {
        *dst++ = *ptr++;
      }
      addx += incx;
    }
    addy += incy;
  }
}

#if 0 // FIXME: Not in use
// create a lower resolution image by averaging all pixels in a block
// (from the full resolution image) into a new pixel. This is pretty
// slow, but yields a higher quality result compared to when each
// level is calculated based on the previous level.
static  unsigned char *
image_downsample(const unsigned char * bytes, const SbVec2s fullsize,
                 const int nc, const SbVec2s subsize, const int div)
{
  unsigned char * dst = new unsigned char[subsize[0]*subsize[1]*nc];
  unsigned char * dstptr = dst;

  int starty = 0;
  int stopy = div;
  for (int y = 0; y < subsize[1]; y++) {
    assert(starty < fullsize[1]);

    int startx = 0;
    int stopx = div;

    for (int x = 0; x < subsize[0]; x++) {
      assert(startx < fullsize[0]);

      int avg[4] = {0};
      int numavg = 0;

      for (int y2 = starty; y2 < stopy; y2++) {
        for (int x2 = startx; x2 < stopx; x2++) {
          const unsigned char * src = bytes + (fullsize[0]*y2 + x2) * nc;
          for (int c = 0; c < nc; c++) {
            avg[c] += src[c];
          }
          numavg++;
        }
      }
      assert(numavg > 0);
      for (int c = 0; c < nc; c++) {
        *dstptr++ = avg[c] / numavg;
      }
      startx += div;
      if (startx >= fullsize[0]) startx = fullsize[0] - 1;
      stopx += div;
      if (stopx > fullsize[0]) stopx = fullsize[0];
    }
    starty += div;
    if (starty >= fullsize[1]) starty = fullsize[1]-1;
    stopy += div;
    if (stopy > fullsize[1]) stopy = fullsize[1];
  }

  return dst;
}
#endif

// create a lower resolution image by averaging four and four pixels
// into a new pixel. This is the same technique as the one usually
// used when creating OpenGL mipmaps. Each level is calculated based
// on the previous level, not on the full-resolution image.
static void
image_downsample_fast(const int width, const int height, const int nc,
                      const unsigned char * datain, unsigned char * dataout)
{
  assert(width > 1 || height > 1);

  int nextrow = width * nc;
  if (width & 1) nextrow += nc; // in case original image has odd size
  int newwidth = width >> 1;
  int newheight = height >> 1;
  unsigned char * dst = dataout;
  const unsigned char * src = datain;

  // check for 1D images
  if (width == 1 || height == 1) {
    int n = SbMax(newwidth, newheight);
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < nc; j++) {
        *dst = (src[0] + src[nc]) >> 1;
        dst++; src++;
      }
      src += nc; // skip to next pixel
    }
  }
  else {
    for (int i = 0; i < newheight; i++) {
      for (int j = 0; j < newwidth; j++) {
        for (int c = 0; c < nc; c++) {
          *dst = (src[0] + src[nc] + src[nextrow] + src[nextrow+nc] + 2) >> 2;
          dst++; src++;
        }
        src += nc; // skip to next pixel
      }
      src += nextrow;
    }
  }
}

void
SoGLBigImageP::createCache(const unsigned char * bytes, const SbVec2s& size, const int nc)
{
  int levels = 0;

  while (((size[0]>>levels) != 0) || ((size[1]>>levels) != 0)) {
    levels++;
  }
  if (levels == 0) return;
  this->numcachelevels = levels;

  this->cache = new unsigned char*[levels];
  this->cachesize = new SbVec2s[levels];
  // temporarily set first cache to simplify code below
  this->cache[0] = (unsigned char*) bytes;
  this->cachesize[0] = size;

  for (int l = 1; l < levels; l++) {
#if 0 // high-quality downsample is too slow, currently disabled
    int sx = size[0] >> l;
    if (sx == 0) sx = 1;
    int sy = size[1] >> l;
    if (sy == 0) sy = 1;

    this->cachesize[l] = SbVec2s((short)sx, (short)sy);
    this->cache[l] = image_downsample(bytes, size, nc, this->cachesize[l], 1<<l);
#else // end of high quality downsample
    short w = size[0]>>l;
    short h = size[1]>>l;
    if (w == 0) w = 1;
    if (h == 0) h = 1;
    this->cachesize[l] = SbVec2s(w, h);
    this->cache[l] = new unsigned char[w*h*nc];
    image_downsample_fast(this->cachesize[l-1][0], this->cachesize[l-1][1], nc,
                          this->cache[l-1], this->cache[l]);
#endif // end of low quality downsample
  }
  this->cache[0] = NULL;
  this->cachesize[0] = SbVec2s(0, 0);
}

void
SoGLBigImageP::resetCache(void)
{
  for (int i = 0; i < this->numcachelevels; i++) {
    delete[] this->cache[i];
  }
  delete[] this->cache;
  delete[] this->cachesize;
  this->cache = NULL;
  this->cachesize = NULL;
  this->numcachelevels = 0;
}

void
SoGLBigImageP::reset(SoGLBigImageTls * tls, SoState * state)
{
  const int n = tls->currentdim[0] * tls->currentdim[1];
  for (int i = 0; i < n; i++) {
    if (tls->glimagearray[i]) {
      tls->glimagearray[i]->unref(state);
      tls->glimagearray[i] = NULL;
    }
    if (tls->imagearray[i]) {
      delete tls->imagearray[i];
      tls->imagearray[i] = NULL;
    }
  }
  delete[] tls->glimagearray;
  delete[] tls->imagearray;
  delete[] tls->glimageage;
  delete[] tls->glimagediv;
  delete[] tls->averagebuf;
  tls->glimagearray = NULL;
  tls->imagearray = NULL;
  tls->glimageage = NULL;
  tls->glimagediv = NULL;
  tls->averagebuf = NULL;
  tls->currentdim.setValue(0,0);
}

void
SoGLBigImageP::unrefOldDL(SoGLBigImageTls * tls, SoState * state, const uint32_t maxage)
{
  const int numimages = tls->currentdim[0] * tls->currentdim[1];
  for (int i = 0; i < numimages; i++) {
    if (tls->glimagearray[i]) {
      if (tls->glimageage[i] >= maxage) {
#if COIN_DEBUG && 0 // debug
        SoDebugError::postInfo("SoGLBigImageP::unrefOldDL",
                               "Killed image because of old age.");
#endif // debug
        tls->glimagearray[i]->unref(state);
        tls->glimagearray[i] = NULL;
      }
      else tls->glimageage[i] += 1;
    }
  }
}

// cc_storage_apply_to_all callback used by resetAllTls()
static void
soglbigimage_resetall_cb(void * tls, void * closure)
{
  // simply call SoGLBigImageP::reset()
  SoGLBigImageP::reset((SoGLBigImageTls*) tls, (SoState*) closure);
}

void
SoGLBigImageP::resetAllTls(SoState * state)
{
  cc_storage_apply_to_all(this->storage, soglbigimage_resetall_cb, state);
}

#endif // DOXYGEN_SKIP_THIS

#undef LINEAR_LIMIT
