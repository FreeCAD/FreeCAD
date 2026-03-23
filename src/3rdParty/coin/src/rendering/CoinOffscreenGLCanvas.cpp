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

#include "CoinOffscreenGLCanvas.h"

#include <climits>

#include <Inventor/C/glue/gl.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoContextHandler.h>
#include <Inventor/elements/SoGLCacheContextElement.h>

#include "tidbitsp.h"

#if defined(HAVE_CONFIG_H)
#include "config.h"
#include "glue/gl_wgl.h"
#endif /* HAVE_CONFIG_H */

// *************************************************************************

unsigned int CoinOffscreenGLCanvas::tilesizeroof = UINT_MAX;

// *************************************************************************

CoinOffscreenGLCanvas::CoinOffscreenGLCanvas(void)
{
  this->size = SbVec2s(0, 0);
  this->context = NULL;
  this->current_hdc = NULL;
}

CoinOffscreenGLCanvas::~CoinOffscreenGLCanvas()
{
  if (this->context) { this->destructContext(); }
}

// *************************************************************************

SbBool
CoinOffscreenGLCanvas::clampSize(SbVec2s & reqsize)
{
  // getMaxTileSize() returns the theoretical maximum gathered from
  // various GL driver information. We're not guaranteed that we'll be
  // able to allocate a buffer of this size -- e.g. due to memory
  // constraints on the gfx card.

  const SbVec2s maxsize = CoinOffscreenGLCanvas::getMaxTileSize();
  if (maxsize == SbVec2s(0, 0)) { return FALSE; }

  reqsize[0] = SbMin(reqsize[0], maxsize[0]);
  reqsize[1] = SbMin(reqsize[1], maxsize[1]);

  // Fit the attempted allocation size to be less than the largest
  // tile size we know have failed allocation. We do this to avoid
  // trying to set the tilesize to dimensions which will very likely
  // fail -- as attempting to find a workable tilesize is an expensive
  // operation when the SoOffscreenRenderer instance already has a GL
  // context set up (destruction and creation of a new one will take
  // time, and it will also kill all GL resources tied to the
  // context).
  while ((((unsigned int)reqsize[0]) * ((unsigned int)reqsize[1])) >
         CoinOffscreenGLCanvas::tilesizeroof) {
    // shrink by halving the largest dimension:
    if (reqsize[0] > reqsize[1]) { reqsize[0] /= 2; }
    else { reqsize[1] /= 2; }
  }

  if ((reqsize[0] == 0) || (reqsize[1] == 0)) { return FALSE; }
  return TRUE;
}

void
CoinOffscreenGLCanvas::setWantedSize(SbVec2s reqsize)
{
  assert((reqsize[0] > 0) && (reqsize[1] > 0) && "invalid dimensions attempted set");

  const SbBool ok = CoinOffscreenGLCanvas::clampSize(reqsize);
  if (!ok) {
    if (this->context) { this->destructContext(); }
    this->size = SbVec2s(0, 0);
    return;
  }

  // We check if the current GL canvas is much larger than what is
  // requested, as to then free up potentially large memory resources,
  // even if we already have a large enough canvas.
  size_t oldres = (size_t)this->size[0] * (size_t)this->size[1];
  size_t newres = (size_t)reqsize[0] * (size_t)reqsize[1];
  const SbBool resourcehog = (oldres > (newres * 16)) && !CoinOffscreenGLCanvas::allowResourcehog();

  // Since the operation of context destruction and reconstruction has
  // the potential to be such a costly operation (because GL caches
  // are smashed, among other things), we try hard to avoid it.
  //
  // So avoid it if not really necessary, by checking that if we
  // already have a working GL context with size equal or larger to
  // the requested, don't destruct (and reconstruct).
  //
  // We can have a different sized internal GL canvas as to what
  // SoOffscreenRenderer wants, because glViewport() is used from
  // SoOffscreenRenderer to render to the correct viewport dimensions.
  if (this->context &&
      (this->size[0] >= reqsize[0]) &&
      (this->size[1] >= reqsize[1]) &&
      !resourcehog) {
    return;
  }

  // Ok, there's no way around it, we need to destruct the GL context:

  if (CoinOffscreenGLCanvas::debug()) {
    SoDebugError::postInfo("CoinOffscreenGLCanvas::setWantedSize",
                           "killing current context, (clamped) reqsize==[%d, %d],"
                           " previous size==[%d, %d], resourcehog==%s",
                           reqsize[0], reqsize[1],
                           this->size[0], this->size[1],
                           resourcehog ? "TRUE" : "FALSE");
  }

  if (resourcehog) {
    // If we were hogging too much memory for the offscreen context,
    // simply go back to the requested size, to free up all that we
    // can.
    this->size = reqsize;
  }
  else {
    // To avoid costly reconstruction on "flutter", by one or two
    // dimensions going a little bit up and down from frame to frame,
    // we try to expand the GL canvas up-front to what perhaps would
    // be sufficient to avoid further GL canvas destruct- /
    // reconstruct-operations.
    this->size[0] = SbMax(reqsize[0], this->size[0]);
    this->size[1] = SbMax(reqsize[1], this->size[1]);
  }

  if (this->context) { this->destructContext(); }
}

const SbVec2s &
CoinOffscreenGLCanvas::getActualSize(void) const
{
  return this->size;
}

// *************************************************************************

uint32_t
CoinOffscreenGLCanvas::tryActivateGLContext(void)
{
  if (this->size == SbVec2s(0, 0)) { return 0; }

  if (this->context == NULL) {
#if defined(HAVE_WGL)
    /* NOTE: This discrepancy between the different glue flavors is due to a
    driver bug that causes the coordinates fed to the gl_FragCoord fragment
    shader input register to be flipped when using render-to-texture capable
    pbuffers. Ref COINSUPPORT-1284. 20101214 tarjei. */
    this->context = wglglue_context_create_offscreen(this->size[0],
                                                     this->size[1],
                                                     FALSE);
#else
    this->context = cc_glglue_context_create_offscreen(this->size[0],
                                                       this->size[1]);
#endif
    if (CoinOffscreenGLCanvas::debug()) {
      SoDebugError::postInfo("CoinOffscreenGLCanvas::tryActivateGLContext",
                             "Tried to create offscreen context of dimensions "
                             "<%d, %d> -- %s",
                             this->size[0], this->size[1],
                             this->context == NULL ? "failed" : "succeeded");
    }

    if (this->context == NULL) { return 0; }

    // Set up mapping from GL context to SoGLRenderAction context id.
    this->renderid = SoGLCacheContextElement::getUniqueCacheContext();

    // need to change this, for the getHDC() function, since a
    // reference to current_hdc is returned (yes, this is dumb, but
    // such is the TGS / Mercury Inventor API)
    this->current_hdc = cc_glglue_win32_HDC(this->context);
  }

  if (cc_glglue_context_make_current(this->context) == FALSE) {
    if (CoinOffscreenGLCanvas::debug()) {
      SoDebugError::post("CoinOffscreenGLCanvas::tryActivateGLContext",
                         "Couldn't make context current.");
    }
    return 0;
  }
  return this->renderid;
}

void
CoinOffscreenGLCanvas::clampToPixelSizeRoof(SbVec2s & s)
{
  unsigned int pixelsize;
  do {
    pixelsize = (unsigned int)s[0] * (unsigned int)s[1];
    if (pixelsize == 0) { return; } // avoid never-ending loop

    if (pixelsize >= CoinOffscreenGLCanvas::tilesizeroof) {
      // halve the largest dimension, and try again:
      if (s[0] > s[1]) { s[0] /= 2; }
      else { s[1] /= 2; }
    }
  } while (pixelsize >= CoinOffscreenGLCanvas::tilesizeroof);
}

// Activates an offscreen GL context, and returns a guaranteed unique
// id to use with SoGLRenderAction::setCacheContext().
//
// If the given context cannot be made current (due to e.g. any error
// condition resulting from the attempt at setting up the offscreen GL
// context), 0 is returned.
uint32_t
CoinOffscreenGLCanvas::activateGLContext(void)
{
  // We try to allocate the wanted size, and then if we fail,
  // successively try with smaller sizes (alternating between halving
  // width and height) until either a workable offscreen buffer was
  // found, or no buffer could be made.
  uint32_t ctx;
  do {
    CoinOffscreenGLCanvas::clampToPixelSizeRoof(this->size);

    ctx = this->tryActivateGLContext();
    if (ctx != 0) { break; }

    // if we've allocated a context, but couldn't make it current
    if (this->context) { this->destructContext(); }

    // we failed with this size, so make sure we only try with smaller
    // tile sizes later
    const unsigned int failedsize =
      (unsigned int)this->size[0] * (unsigned int)this->size[1];
    assert(failedsize <= CoinOffscreenGLCanvas::tilesizeroof);
    CoinOffscreenGLCanvas::tilesizeroof = failedsize;

    // keep trying until 32x32 -- if even those dimensions doesn't
    // work, give up, as too small tiles will cause the processing
    // time to go through the roof due to the huge number of passes:
    if ((this->size[0] <= 32) && (this->size[1] <= 32)) { break; }
  } while (TRUE);

  return ctx;
}

void
CoinOffscreenGLCanvas::deactivateGLContext(void)
{
  assert(this->context);
  cc_glglue_context_reinstate_previous(this->context);
}

// *************************************************************************

void
CoinOffscreenGLCanvas::destructContext(void)
{
  assert(this->context);

  if (cc_glglue_context_make_current(this->context)) {
    SoContextHandler::destructingContext(this->renderid);
    this->deactivateGLContext();
  }
  else {
    if (CoinOffscreenGLCanvas::debug()) {
      SoDebugError::post("CoinOffscreenGLCanvas::destructContext",
                         "Couldn't activate context -- resource cleanup "
                         "not complete.");
    }
  }

  cc_glglue_context_destruct(this->context);

  this->context = NULL;
  this->renderid = 0;
  this->current_hdc = NULL;
}

// *************************************************************************

/* This abomination is needed to support SoOffscreenRenderer::getDC(). */
const void * const &
CoinOffscreenGLCanvas::getHDC(void) const
{
  return this->current_hdc;
}

void CoinOffscreenGLCanvas::updateDCBitmap()
{
cc_glglue_win32_updateHDCBitmap(this->context);
}
// *************************************************************************

// Pushes the rendered pixels into the internal memory array.
void
CoinOffscreenGLCanvas::readPixels(uint8_t * dst,
                                  const SbVec2s & vpdims,
                                  unsigned int dstrowsize,
                                  unsigned int nrcomponents) const
{
  glPushAttrib(GL_ALL_ATTRIB_BITS);

  // First reset all settings that can influence the result of a
  // glReadPixels() call, to make sure we get the actual contents of
  // the buffer, unmodified.
  //
  // The values set up below matches the default settings of an
  // OpenGL driver.

  glPixelStorei(GL_PACK_SWAP_BYTES, 0);
  glPixelStorei(GL_PACK_LSB_FIRST, 0);
  glPixelStorei(GL_PACK_ROW_LENGTH, (GLint)dstrowsize);
  glPixelStorei(GL_PACK_SKIP_ROWS, 0);
  glPixelStorei(GL_PACK_SKIP_PIXELS, 0);

  // FIXME: should use best possible alignment, for speediest
  // operation. 20050617 mortene.
//   glPixelStorei(GL_PACK_ALIGNMENT, 4);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

  glPixelTransferi(GL_MAP_COLOR, 0);
  glPixelTransferi(GL_MAP_STENCIL, 0);
  glPixelTransferi(GL_INDEX_SHIFT, 0);
  glPixelTransferi(GL_INDEX_OFFSET, 0);
  glPixelTransferf(GL_RED_SCALE, 1);
  glPixelTransferf(GL_RED_BIAS, 0);
  glPixelTransferf(GL_GREEN_SCALE, 1);
  glPixelTransferf(GL_GREEN_BIAS, 0);
  glPixelTransferf(GL_BLUE_SCALE, 1);
  glPixelTransferf(GL_BLUE_BIAS, 0);
  glPixelTransferf(GL_ALPHA_SCALE, 1);
  glPixelTransferf(GL_ALPHA_BIAS, 0);
  glPixelTransferf(GL_DEPTH_SCALE, 1);
  glPixelTransferf(GL_DEPTH_BIAS, 0);

  GLuint i = 0;
  GLfloat f = 0.0f;
  glPixelMapfv(GL_PIXEL_MAP_I_TO_I, 1, &f);
  glPixelMapuiv(GL_PIXEL_MAP_S_TO_S, 1, &i);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 1, &f);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 1, &f);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 1, &f);
  glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 1, &f);
  glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 1, &f);
  glPixelMapfv(GL_PIXEL_MAP_G_TO_G, 1, &f);
  glPixelMapfv(GL_PIXEL_MAP_B_TO_B, 1, &f);
  glPixelMapfv(GL_PIXEL_MAP_A_TO_A, 1, &f);

  // The flushing of the OpenGL pipeline before and after the
  // glReadPixels() call is done as a work-around for a reported
  // OpenGL driver bug: on a Win2000 system with ATI Radeon graphics
  // card, the system would hang hard if the flushing was not done.
  //
  // This is obviously an OpenGL driver bug, but the workaround of
  // doing excessive flushing has no real ill effects, so we just do
  // it unconditionally for all drivers. Note that it might not be
  // necessary to flush both before and after glReadPixels() to work
  // around the bug (this was not established with the external
  // reporter), but again it shouldn't matter if we do.
  //
  // For reference, the specific driver which was reported to fail has
  // the following characteristics:
  //
  // GL_VENDOR="ATI Technologies Inc."
  // GL_RENDERER="Radeon 9000 DDR x86/SSE2"
  // GL_VERSION="1.3.3446 Win2000 Release"
  //
  // mortene.

  glFlush(); glFinish();

  assert((nrcomponents >= 1) && (nrcomponents <= 4));

  if (nrcomponents < 3) {
    unsigned char * tmp = new unsigned char[vpdims[0]*vpdims[1]*4];
    glReadPixels(0, 0, vpdims[0], vpdims[1],
                 nrcomponents == 1 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, tmp);

    const unsigned char * src = tmp;
    // manually convert to grayscale
    for (short y = 0; y < vpdims[1]; y++) {
      for (short x = 0; x < vpdims[0]; x++) {
        double v = src[0] * 0.3 + src[1] * 0.59 + src[2] * 0.11;
        *dst++ = (unsigned char) v;
        if (nrcomponents == 2) {
          *dst++ = src[3];
        }
        src += nrcomponents == 1 ? 3 : 4;
      }
    }
    delete[] tmp;
  }
  else {
    glReadPixels(0, 0, vpdims[0], vpdims[1],
                 nrcomponents == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, dst);
  }
  glFlush(); glFinish();

  glPopAttrib();
}

// *************************************************************************

static SbBool tilesize_cached = FALSE;
static unsigned int maxtile[2] = { 0, 0 };

static void tilesize_cleanup(void)
{
  tilesize_cached = FALSE;
  maxtile[0] = maxtile[1] = 0;
}

// Return largest size of offscreen canvas system can handle. Will
// cache result, so only the first look-up is expensive.
SbVec2s
CoinOffscreenGLCanvas::getMaxTileSize(void)
{
  // cache the values in static variables so that a new context is not
  // created every time render() is called in SoOffscreenRenderer
  if (tilesize_cached) return SbVec2s((short)maxtile[0], (short)maxtile[1]);

  tilesize_cached = TRUE; // Flip on first run.

  coin_atexit((coin_atexit_f*) tilesize_cleanup, CC_ATEXIT_NORMAL);

  unsigned int width, height;
  cc_glglue_context_max_dimensions(&width, &height);

  if (CoinOffscreenGLCanvas::debug()) {
    SoDebugError::postInfo("CoinOffscreenGLCanvas::getMaxTileSize",
                           "cc_glglue_context_max_dimensions()==[%u, %u]",
                           width, height);
  }

  // Makes it possible to override the default tilesizes. Should prove
  // useful for debugging problems on remote sites.
  const char * env = coin_getenv("COIN_OFFSCREENRENDERER_TILEWIDTH");
  const unsigned int forcedtilewidth = env ? atoi(env) : 0;
  env = coin_getenv("COIN_OFFSCREENRENDERER_TILEHEIGHT");
  const unsigned int forcedtileheight = env ? atoi(env) : 0;

  if (forcedtilewidth != 0) { width = forcedtilewidth; }
  if (forcedtileheight != 0) { height = forcedtileheight; }

  // Also make it possible to force a maximum tilesize.
  env = coin_getenv("COIN_OFFSCREENRENDERER_MAX_TILESIZE");
  const unsigned int maxtilesize = env ? atoi(env) : 0;
  if (maxtilesize != 0) {
    width = SbMin(width, maxtilesize);
    height = SbMin(height, maxtilesize);
  }

  // cache result for later calls, and clamp to fit within a short
  // integer type
  maxtile[0] = SbMin(width, (unsigned int)SHRT_MAX);
  maxtile[1] = SbMin(height, (unsigned int)SHRT_MAX);

  return SbVec2s((short)maxtile[0], (short)maxtile[1]);
}

// *************************************************************************

SbBool
CoinOffscreenGLCanvas::debug(void)
{
  static int flag = -1; // -1 means "not initialized" in this context
  if (flag == -1) {
    const char * env = coin_getenv("COIN_DEBUG_SOOFFSCREENRENDERER");
    flag = env && (atoi(env) > 0);
  }
  return flag;
}

SbBool
CoinOffscreenGLCanvas::allowResourcehog(void)
{
  static int resourcehog_flag = -1; // -1 means "not initialized" in this context
  if (resourcehog_flag == -1) {
    const char * env = coin_getenv("COIN_SOOFFSCREENRENDERER_ALLOW_RESOURCEHOG");
    resourcehog_flag = env && (atoi(env) > 0);
    SoDebugError::postInfo("CoinOffscreenGLCanvas",
                           "Ignoring resource hogging due to set COIN_SOOFFSCREENRENDERER_ALLOW_RESOURCEHOG environment variable.");
  }
  return resourcehog_flag;
}

// *************************************************************************
