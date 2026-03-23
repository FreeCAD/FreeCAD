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

// FIXME: in my not so humble opinion, this class is an ugly mess --
// or at least its interface.
//
// Some examples: there are *4* functions named "setData()" -- which
// should throw up a huge red warning sign alone. Methods named
// "setData()" and "setFlags()" (the latter which we also have in
// SoGLImage) should make API designers cringe.  There's a whole bunch
// of *public* methods marked as being *private* and *internal* --
// another warning sign. The setData() method that creates a 2D
// texture object simply calls into the method that makes a 3D texture
// object, while the obvious right thing to do would be to refactor
// common code to private methods.
//
// Since this was made part of the public API of Coin 2, I guess we'll
// have to support it until the end of time, or at least a couple of
// major versions down the road. What we should do with it is to mark
// it as obsolete, then refactor the functionality out of it,
// preferably into a nice, *clean* little C API to complement the
// stuff contained in cc_glglue_*(), then convert all internal code to
// use that instead.
//
// 20030312 mortene.

// *************************************************************************

// FIXME: Add TEX3 environment variables (or general TEX variables?)
// (kintel 20011112)

// *************************************************************************

/*!
  \class SoGLImage include/Inventor/misc/SoGLImage.h
  \brief The SoGLImage class is used to handle OpenGL 2D/3D textures.

  A number of environment variables can be set to control how textures
  are created. This is useful to tune Coin to fit your system. E.g. if you
  are running on a laptop, it might be a good idea to disable linear
  filtering and mipmaps.

  \li COIN_TEX2_LINEAR_LIMIT: Linear filtering is enabled if
  Complexity::textureQuality is greater or equal to this
  value. Default value is 0.2.

  \li COIN_TEX2_MIPMAP_LIMIT: Mipmaps are created if textureQuality is
  greater or equal to this value. Default value is 0.5.

  \li COIN_TEX2_LINEAR_MIPMAP_LIMIT: Linear filtering between mipmap
  levels is enabled if textureQuality is greater or equal to this
  value. Default value is 0.8.

  \li COIN_TEX2_SCALEUP_LIMIT: Textures with width or height not equal
  to a power of two will always be scaled up if textureQuality is
  greater or equal to this value.  Default value is 0.7. If
  textureQuality is lower than this value, and the width or height is
  larger than 256 pixels, the texture is only scaled up if it is
  relatively close to the next power of two size. This could save a
  lot of texture memory.

  \li COIN_TEX2_USE_GLTEXSUBIMAGE: When set, and when the new texture
  data have the same attributes as the old data, glTexSubImage() will
  be used to copy new data into the texture instead of recreating the
  texture.  This is not enabled by default, since it seems to trigger
  a bug in the Linux nVidia drivers. It just happens in some
  unreproduceable cases.  It could be a bug in our glTexSubImage()
  code, of course. :)

  \li COIN_TEX2_USE_SGIS_GENERATE_MIPMAP: When set, use the
  GL_SGIS_generate_mipmap extension (if available) to generate mipmaps,
  otherwise use a fast internal routine to generate them. Use of
  GL_SGIS_generate_mipmap is not enabled by default since we suspect some
  ATi drivers have problems with this extension.

  \li COIN_ENABLE_CONFORMANT_GL_CLAMP: When set, GL_CLAMP will be used
  when SoGLImage::CLAMP is specified as the texture wrap mode. By
  default GL_CLAMP_TO_EDGE is used, since this is usually what people
  want.  See
  http://www.opengl.org/discussion_boards/ubb/Forum3/HTML/007306.html
  for a discussion regarding GL_CLAMP and GL_CLAMP_TO_EDGE.

  \li COIN_TEX2_ANISOTROPIC_LIMIT: Anisotropic filtering is enabled
  for textures when the texture quality is higher than this value.
  Default value is 0.85

  \COIN_CLASS_EXTENSION

  \since Coin 2.0
*/

// *************************************************************************

/*!
  \enum SoGLImage::Wrap

  Used to specify how texture coordinates < 0.0 and > 1.0 should be handled.
  It can either be repeated (REPEAT), clamped (CLAMP) or clamped to edge
  (CLAMP_TO_EDGE), which is useful when tiling textures. Since 2002-11-18,
  CLAMP will be treated as CLAMP_TO_EDGE. The environment variable
  COIN_ENABLE_CONFORMANT_GL_CLAMP can be used to override this behaviour.
*/

/*!
  \enum SoGLImage::ResizeReason

  Sent as a parameter to SoGLImageResizeCB as a hint to why an image
  is being resized. IMAGE means that a whole image is being initially
  resized (e.g. a texture image). SUBIMAGE and MIPMAP are not in use and
  reserved for future use.
*/

/*!
  \enum SoGLImage::Flags

  Can be used to tune/optimize the GL texture handling. Normally the
  texture quality will be used to decide scaling and filtering, and
  the image data will be scanned to decide if the image is (partially)
  transparent, and if the texture can be rendered using the cheaper
  alpha test instead of blending if it does contain transparency. If
  you know the contents of your texture image, or if you have special
  requirements on how the texture should be rendered, you can set the
  flags using the SoGLImage::setFlags() method.

*/

// FIXME: Support other reason values than IMAGE (kintel 20050531)
/*!
  \typedef bool SoGLImage::SoGLImageResizeCB(SoState * state,
                                             const SbVec3s &newsize,
                                             unsigned char * destbuffer,
                                             SoGLImage::ResizeReason reason,
                                             void * closure,
                                             class SoGLImage * image)

  Image resize callback type.
  If registered using setResizeCallback(), this function will be called
  whenever Coin needs to resize an image. The function will be called
  both for 2D and 3D images.

  \e state is the current state at the time of resizing.
  \e newsize is the requested new image size. Note that the z size of a
  2D image is 0.
  \e destbuffer is a preallocated buffer big enough to hold the pixels
  for the resized image. The # of bytes per pixel is the same as for the
  original image.
  \e reason is a hint about why the image is resized. At the moment,
  only IMAGE is supported.
  \e image is the original image.

  Return value: TRUE if the resize has been resized, FALSE if not.
  If FALSE is returned, Coin will resize the image instead.
*/

// *************************************************************************

#include <Inventor/misc/SoGLImage.h>

#include <cassert>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/C/glue/gl.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/SbImage.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLDisplayList.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/system/gl.h>
#include <Inventor/threads/SbStorage.h>
#include <Inventor/misc/SoContextHandler.h>
#include <Inventor/misc/SoGLCubeMapImage.h>
#include <Inventor/misc/SoGLDriverDatabase.h>

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbMutex.h>
#endif // COIN_THREADSAFE

#include "tidbitsp.h"
#include "rendering/SoGL.h"
#include "elements/SoTextureScaleQualityElement.h"
#include "glue/GLUWrapper.h"
#include "glue/glp.h"
#include "glue/simage_wrapper.h"
#include "threads/threadsutilp.h"
#include "coindefs.h"

#if COIN_WORKAROUND(COIN_MSVC, <= COIN_MSVC_6_0_VERSION)
// symbol length truncation
#pragma warning(disable:4786)
#endif // VC6.0

// *************************************************************************

static float DEFAULT_LINEAR_LIMIT = 0.2f;
static float DEFAULT_MIPMAP_LIMIT = 0.5f;
static float DEFAULT_LINEAR_MIPMAP_LIMIT = 0.8f;
static float DEFAULT_SCALEUP_LIMIT = 0.7f;
static float DEFAULT_ANISOTROPIC_LIMIT = 0.85f;

static float COIN_TEX2_LINEAR_LIMIT = -1.0f;
static float COIN_TEX2_MIPMAP_LIMIT = -1.0f;
static float COIN_TEX2_LINEAR_MIPMAP_LIMIT = -1.0f;
static float COIN_TEX2_SCALEUP_LIMIT = -1.0f;
static float COIN_TEX2_ANISOTROPIC_LIMIT = -1.0f;
static int COIN_TEX2_USE_GLTEXSUBIMAGE = -1;
static int COIN_TEX2_USE_SGIS_GENERATE_MIPMAP = -1;
static int COIN_ENABLE_CONFORMANT_GL_CLAMP = -1;

// *************************************************************************

// buffer used for creating mipmap images
static SbStorage * glimage_bufferstorage = NULL;

typedef struct {
  unsigned char * buffer;
  unsigned char * mipmapbuffer;
  int buffersize;
  int mipmapbuffersize;
} soglimage_buffer;

static void
glimage_buffer_construct(void * buffer)
{
  soglimage_buffer * buf = (soglimage_buffer*) buffer;
  buf->buffer = NULL;
  buf->buffersize = 0;
  buf->mipmapbuffer = NULL;
  buf->mipmapbuffersize = 0;
}

static void
glimage_buffer_destruct(void * buffer)
{
  soglimage_buffer * buf = (soglimage_buffer*) buffer;
  delete[] buf->buffer;
  delete[] buf->mipmapbuffer;
}


static unsigned char *
glimage_get_buffer(const int buffersize, const SbBool mipmap)
{
  soglimage_buffer * buf = NULL;
  assert(glimage_bufferstorage != NULL);

  buf = (soglimage_buffer*)
    glimage_bufferstorage->get();
  if (mipmap) {
    if (buf->mipmapbuffersize < buffersize) {
      delete[] buf->mipmapbuffer;
      buf->mipmapbuffer = new unsigned char[buffersize];
      buf->mipmapbuffersize = buffersize;
    }
    return buf->mipmapbuffer;
  }
  else {
    if (buf->buffersize < buffersize) {
      delete[] buf->buffer;
      buf->buffer = new unsigned char[buffersize];
      buf->buffersize = buffersize;
      // FIXME: this is an extremely lame workaround for a Purify UMR
      // reported by Tore Kristiansen of HitecO.
      //
      // An UMR is reported from buf->buffer (when disabling the
      // memset() workaround below) from somewhere within the
      // fast_mipmap() method. Purify doesn't go far enough down the
      // call-stack (probably because fast_mipmap() is a local static
      // method?)  for us to easily see where the exact error happens,
      // though.
      //
      // My guess is that the mipmap-buffer isn't completely "filled
      // out", and so the glTex[Sub]Image2D() call asks for more
      // pixels than was generated.
      //
      // Should try to get this problem reproduced locally before
      // attempting to fix it.
      //
      // 20030514 mortene.
      (void)memset(buf->buffer, 0x55, buf->buffersize);
    }
    return buf->buffer;
  }
}

// *************************************************************************

static int
compute_log(int value)
{
  int i = 0;
  while (value > 1) { value>>=1; i++; }
  return i;
}

//FIXME: Use as a special case of 3D image to reduce codelines ? (kintel 20011115)
static void
halve_image(const int width, const int height, const int nc,
            const unsigned char *datain, unsigned char *dataout)
{
  assert(width > 1 || height > 1);

  int nextrow = width *nc;
  int newwidth = width >> 1;
  int newheight = height >> 1;
  unsigned char *dst = dataout;
  const unsigned char *src = datain;

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

static void
halve_image(const int width, const int height, const int depth, const int nc,
            const unsigned char *datain, unsigned char *dataout)
{
  assert(width > 1 || height > 1 || depth > 1);

  int rowsize = width * nc;
  int imagesize = width * height * nc;
  int newwidth = width >> 1;
  int newheight = height >> 1;
  int newdepth = depth >> 1;
  unsigned char *dst = dataout;
  const unsigned char *src = datain;

  int numdims = (width>=1?1:0)+(height>=1?1:0)+(depth>=1?1:0);
  // check for 1D images.
  if (numdims == 1) {
    int n = SbMax(SbMax(newwidth, newheight), newdepth);
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < nc; j++) {
        *dst = (src[0] + src[nc]) >> 1;
        dst++; src++;
      }
      src += nc; // skip to next pixel/row/image
    }
  }
  // check for 2D images
  else if (numdims == 2) {
    int s1,s2,blocksize;
    if (width==1) {
      s1 = newheight;
      blocksize = height * nc;
    }
    else {
      s1 = newwidth;
      blocksize = width * nc;
    }
    s2 = depth==1?newheight:newdepth;
    for (int j = 0; j < s2; j++) {
      for (int i = 0; i < s1; i++) {
        for (int k = 0; k < nc; k++) {
          *dst = (src[0] + src[nc] + src[blocksize] + src[blocksize+nc] + 2) >> 2;
          dst++; src++;
        }
        src += nc; // skip to next pixel (x or y direction)
      }
      src += blocksize; // Skip to next row/image
    }
  }
  else { // 3D image
    for (int k = 0; k < newdepth; k++) {
      for (int j = 0; j < newheight; j++) {
        for (int i = 0; i < newwidth; i++) {
          for (int c = 0; c < nc; c++) {
            *dst = (src[0] + src[nc] +
                    src[rowsize] + src[rowsize+nc] +
                    src[imagesize] + src[imagesize+nc] +
                    src[imagesize+rowsize] + src[imagesize+rowsize+nc] +
                    4) >> 3;
            dst++; src++;
          }
          src += nc; // skip one pixel
        }
        src += rowsize; // skip one row
      }
      src += imagesize; // skip one image
    }
  }
}


// fast mipmap creation. no repeated memory allocations.
static void
fast_mipmap(SoState * state, int width, int height, int nc,
            const unsigned char *data, const SbBool useglsubimage,
            SbBool compress)
{
  const cc_glglue * glw = sogl_glue_instance(state);
  GLint internalFormat = coin_glglue_get_internal_texture_format(glw, nc, compress);
  GLenum format = coin_glglue_get_texture_format(glw, nc);
  int levels = compute_log(width);
  int level = compute_log(height);
  if (level > levels) levels = level;

  int memreq = (SbMax(width>>1,1))*(SbMax(height>>1,1))*nc;
  unsigned char * mipmap_buffer = glimage_get_buffer(memreq, TRUE);

  if (useglsubimage) {
    if (SoGLDriverDatabase::isSupported(glw, SO_GL_TEXSUBIMAGE)) {
      cc_glglue_glTexSubImage2D(glw, GL_TEXTURE_2D, 0, 0, 0,
                                width, height, format,
                                GL_UNSIGNED_BYTE, data);
    }
  }
  else {
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
  }
  unsigned char *src = (unsigned char *) data;
  for (level = 1; level <= levels; level++) {
    halve_image(width, height, nc, src, mipmap_buffer);
    if (width > 1) width >>= 1;
    if (height > 1) height >>= 1;
    src = mipmap_buffer;
    if (useglsubimage) {
      if (SoGLDriverDatabase::isSupported(glw, SO_GL_TEXSUBIMAGE)) {
        cc_glglue_glTexSubImage2D(glw, GL_TEXTURE_2D, level, 0, 0,
                                  width, height, format,
                                  GL_UNSIGNED_BYTE, src);
      }
    }
    else {
      glTexImage2D(GL_TEXTURE_2D, level, internalFormat, width,
                   height, 0, format, GL_UNSIGNED_BYTE, src);
    }
  }
}

// fast mipmap creation. no repeated memory allocations. 3D version.
static void
fast_mipmap(SoState * state, int width, int height, int depth,
            int nc, const unsigned char *data, const SbBool useglsubimage,
            SbBool compress)
{
  const cc_glglue * glw = sogl_glue_instance(state);
  GLint internalFormat = coin_glglue_get_internal_texture_format(glw, nc, compress);
  GLenum format = coin_glglue_get_texture_format(glw, nc);
  int levels = compute_log(SbMax(SbMax(width, height), depth));

  int memreq = (SbMax(width>>1,1))*(SbMax(height>>1,1))*(SbMax(depth>>1,1))*nc;
  unsigned char * mipmap_buffer = glimage_get_buffer(memreq, TRUE);

  // Send level 0 (original image) to OpenGL
  if (useglsubimage) {
    if (SoGLDriverDatabase::isSupported(glw, SO_GL_3D_TEXTURES)) {
      cc_glglue_glTexSubImage3D(glw, GL_TEXTURE_3D, 0, 0, 0, 0,
                                width, height, depth, format,
                                GL_UNSIGNED_BYTE, data);
    }
  }
  else {
    if (SoGLDriverDatabase::isSupported(glw, SO_GL_3D_TEXTURES)) {
      cc_glglue_glTexImage3D(glw, GL_TEXTURE_3D, 0, internalFormat,
                             width, height, depth, 0, format,
                             GL_UNSIGNED_BYTE, data);
    }
  }
  unsigned char *src = (unsigned char *) data;
  for (int level = 1; level <= levels; level++) {
    halve_image(width, height, depth, nc, src, mipmap_buffer);
    if (width > 1) width >>= 1;
    if (height > 1) height >>= 1;
    if (depth > 1) depth >>= 1;
    src = mipmap_buffer;
    if (useglsubimage) {
      if (SoGLDriverDatabase::isSupported(glw, SO_GL_3D_TEXTURES)) {
        cc_glglue_glTexSubImage3D(glw, GL_TEXTURE_3D, level, 0, 0, 0,
                                  width, height, depth, format,
                                  GL_UNSIGNED_BYTE, src);
      }
    }
    else {
      if (SoGLDriverDatabase::isSupported(glw, SO_GL_3D_TEXTURES)) {
        cc_glglue_glTexImage3D(glw, GL_TEXTURE_3D, level, internalFormat,
                               width, height, depth, 0, format,
                               GL_UNSIGNED_BYTE, src);
      }
    }
  }
}

// A low quality resize function. It is only used when neither simage
// nor GLU is available.
static void
fast_image_resize(const unsigned char * src,
                  unsigned char * dest,
                  int width,
                  int height, int num_comp,
                  int newwidth, int newheight)
{
  float sx, sy, dx, dy;
  int src_bpr, dest_bpr, xstop, ystop, x, y, offset, i;

  dx = ((float)width)/((float)newwidth);
  dy = ((float)height)/((float)newheight);
  src_bpr = width * num_comp;
  dest_bpr = newwidth * num_comp;

  sy = 0.0f;
  ystop = newheight * dest_bpr;
  xstop = newwidth * num_comp;
  for (y = 0; y < ystop; y += dest_bpr) {
    sx = 0.0f;
    for (x = 0; x < xstop; x += num_comp) {
      offset = ((int)sy)*src_bpr + ((int)sx)*num_comp;
      for (i = 0; i < num_comp; i++) dest[x+y+i] = src[offset+i];
      sx += dx;
    }
    sy += dy;
  }
}

// A low quality resize function for 3D texture image buffers. It is
// only used when neither simage nor GLU is available.
static void
fast_image_resize3d(const unsigned char * src,
                    unsigned char * dest,
                    int width, int height,
                    int nc, int layers,
                    int newwidth, int newheight,
                    int newlayers)
{
  float sx, sy, sz, dx, dy, dz;
  int src_bpr, dest_bpr, src_bpl, dest_bpl, xstop, ystop, zstop;
  int x, y, z, offset, i;

  dx = ((float)width)/((float)newwidth);
  dy = ((float)height)/((float)newheight);
  dz = ((float)layers)/((float)newlayers);
  src_bpr = width * nc;
  dest_bpr = newwidth * nc;
  src_bpl = src_bpr * height;
  dest_bpl = dest_bpr * newheight;

  zstop = newlayers * dest_bpl;
  ystop = dest_bpl;
  xstop = dest_bpr;
  sz = 0.0f;
  for (z = 0; z < zstop; z += dest_bpl) {
    sy = 0.0f;
    for (y = 0; y < ystop; y += dest_bpr) {
      sx = 0.0f;
      for (x = 0; x < xstop; x += nc) {
        offset = ((int)sz)*src_bpl + ((int)sy)*src_bpr + ((int)sx)*nc;
        for (i = 0; i < nc; i++) dest[x+y+z+i] = src[offset+i];
        sx += dx;
      }
      sy += dy;
    }
    sz += dz;
  }
}

// *************************************************************************

class SoGLImageP {
public:
#ifdef COIN_THREADSAFE
  static SbMutex * mutex;
#endif // COIN_THREADSAFE

  static SoType classTypeId;
  static uint32_t current_glimageid;
  static uint32_t getNextGLImageId(void);

  SoGLDisplayList *createGLDisplayList(SoState *state);
  void checkTransparency(void);
  void unrefDLists(SoState *state);
  void reallyCreateTexture(SoState *state,
                           const unsigned char *const texture,
                           const int numComponents,
                           const int w, const int h, const int d,
                           const SbBool dlist,
                           const SbBool mipmap,
                           const int border);
  void reallyBindPBuffer(SoState *state);
  void resizeImage(SoState * state, unsigned char *&imageptr,
                   uint32_t &xsize, uint32_t &ysize, uint32_t &zsize);
  SbBool shouldCreateMipmap(void);
  void applyFilter(const SbBool ismipmap);

  void * pbuffer;
  const SbImage *image;
  SbImage dummyimage;
  SbVec3s glsize;
  int glcomp;

  SbBool needtransparencytest;
  SbBool hastransparency;
  SbBool usealphatest;
  uint32_t flags;
  float quality;

  SoGLImage::Wrap wraps;
  SoGLImage::Wrap wrapt;
  SoGLImage::Wrap wrapr;
  int border;
  SbBool isregistered;
  uint32_t imageage;
  void (*endframecb)(void*);
  void *endframeclosure;

  class dldata {
  public:
    dldata(void)
      : dlist(NULL), age(0) { }
    dldata(SoGLDisplayList *dl)
      : dlist(dl),
        age(0) { }
    dldata(const dldata & org)
      : dlist(org.dlist),
        age(org.age) { }
    SoGLDisplayList *dlist;
    uint32_t age;
  };

  SbList <dldata> dlists;
  SoGLDisplayList *findDL(SoState *state);
  void tagDL(SoState *state);
  void unrefOldDL(SoState *state, const uint32_t maxage);
  SoGLImage *owner;
  uint32_t glimageid;
  void init(void);
  static void contextCleanup(uint32_t context, void * closure);

  static SoGLImage::SoGLImageResizeCB * resizecb;
  static void * resizeclosure;
};

SoType SoGLImageP::classTypeId STATIC_SOTYPE_INIT;
uint32_t SoGLImageP::current_glimageid = 1;
SoGLImage::SoGLImageResizeCB * SoGLImageP::resizecb = NULL;
void * SoGLImageP::resizeclosure = NULL;
#ifdef COIN_THREADSAFE
SbMutex * SoGLImageP::mutex;
#endif // COIN_THREADSAFE

#undef PRIVATE
#define PRIVATE(p) ((p)->pimpl)

// *************************************************************************


// This class is not 100% threadsafe. It is threadsafe for rendering
// only. It is assumed that setData() is called by only one thread at
// a time. The reason for this is that all threads should use the same
// data, and it would be meaningless if two threads set different data
// for an SoGLImage. The nodes using SoGLImage should use a mutex to
// ensure that only one thread calls setData(), the other threads
// should wait for that thread to finish. This is done in Coin now.
//
// SoGLImage::getGLDisplayList() use a mutex so that several
// threads can call this method safely.


// we now share one mutex among all glimages to avoid allocating too
// many mutexes.
#ifdef COIN_THREADSAFE
#define LOCK_GLIMAGE SoGLImageP::mutex->lock()
#define UNLOCK_GLIMAGE SoGLImageP::mutex->unlock()
#else // COIN_THREADSAFE
#define LOCK_GLIMAGE
#define UNLOCK_GLIMAGE
#endif // !COIN_THREADSAFE

// *************************************************************************

/*!
  Constructor.
*/
SoGLImage::SoGLImage(void)
{
  PRIVATE(this) = new SoGLImageP;
  SoContextHandler::addContextDestructionCallback(SoGLImageP::contextCleanup, PRIVATE(this));
  PRIVATE(this)->isregistered = FALSE;
  PRIVATE(this)->init(); // init members to default values
  PRIVATE(this)->owner = this;

  // check environment variables
  if (COIN_TEX2_LINEAR_LIMIT < 0.0f) {
    const char *env = coin_getenv("COIN_TEX2_LINEAR_LIMIT");
    if (env) COIN_TEX2_LINEAR_LIMIT = (float) atof(env);
    if (COIN_TEX2_LINEAR_LIMIT < 0.0f || COIN_TEX2_LINEAR_LIMIT > 1.0f) {
      COIN_TEX2_LINEAR_LIMIT = DEFAULT_LINEAR_LIMIT;
    }
  }
  if (COIN_TEX2_MIPMAP_LIMIT < 0.0f) {
    const char *env = coin_getenv("COIN_TEX2_MIPMAP_LIMIT");
    if (env) COIN_TEX2_MIPMAP_LIMIT = (float) atof(env);
    if (COIN_TEX2_MIPMAP_LIMIT < 0.0f || COIN_TEX2_MIPMAP_LIMIT > 1.0f) {
      COIN_TEX2_MIPMAP_LIMIT = DEFAULT_MIPMAP_LIMIT;
    }
  }
  if (COIN_TEX2_LINEAR_MIPMAP_LIMIT < 0.0f) {
    const char *env = coin_getenv("COIN_TEX2_LINEAR_MIPMAP_LIMIT");
    if (env) COIN_TEX2_LINEAR_MIPMAP_LIMIT = (float) atof(env);
    if (COIN_TEX2_LINEAR_MIPMAP_LIMIT < 0.0f || COIN_TEX2_LINEAR_MIPMAP_LIMIT > 1.0f) {
      COIN_TEX2_LINEAR_MIPMAP_LIMIT = DEFAULT_LINEAR_MIPMAP_LIMIT;
    }
  }

  if (COIN_TEX2_SCALEUP_LIMIT < 0.0f) {
    const char *env = coin_getenv("COIN_TEX2_SCALEUP_LIMIT");
    if (env) COIN_TEX2_SCALEUP_LIMIT = (float) atof(env);
    if (COIN_TEX2_SCALEUP_LIMIT < 0.0f || COIN_TEX2_SCALEUP_LIMIT > 1.0f) {
      COIN_TEX2_SCALEUP_LIMIT = DEFAULT_SCALEUP_LIMIT;
    }
  }

  if (COIN_TEX2_USE_GLTEXSUBIMAGE < 0) {
    const char *env = coin_getenv("COIN_TEX2_USE_GLTEXSUBIMAGE");
    if (env && atoi(env) == 1) {
      COIN_TEX2_USE_GLTEXSUBIMAGE = 1;
    }
    else COIN_TEX2_USE_GLTEXSUBIMAGE = 0;
  }
  if (COIN_TEX2_USE_SGIS_GENERATE_MIPMAP < 0) {
    const char *env = coin_getenv("COIN_TEX2_USE_SGIS_GENERATE_MIPMAP");
    if (env && atoi(env) == 1) {
      COIN_TEX2_USE_SGIS_GENERATE_MIPMAP = 1;
    }
    else COIN_TEX2_USE_SGIS_GENERATE_MIPMAP = 0;
  }

  if (COIN_ENABLE_CONFORMANT_GL_CLAMP < 0) {
    const char * env = coin_getenv("COIN_ENABLE_CONFORMANT_GL_CLAMP");
    if (env && atoi(env) == 1) {
      COIN_ENABLE_CONFORMANT_GL_CLAMP = 1;
    }
    else COIN_ENABLE_CONFORMANT_GL_CLAMP = 0;
  }
  if (COIN_TEX2_ANISOTROPIC_LIMIT < 0.0f) {
    const char *env = coin_getenv("COIN_TEX2_ANISOTROPIC_LIMIT");
    if (env) COIN_TEX2_ANISOTROPIC_LIMIT = (float) atof(env);
    else COIN_TEX2_ANISOTROPIC_LIMIT = DEFAULT_ANISOTROPIC_LIMIT;
  }
}


/*!
  This static method initializes static data for the SoGLImage class.
*/
void
SoGLImage::initClass(void)
{
  assert(SoGLImageP::classTypeId.isBad());
  SoGLImageP::classTypeId = SoType::createType(SoType::badType(),
                                               SbName("GLImage"));
#ifdef COIN_THREADSAFE
  SoGLImageP::mutex = new SbMutex;
#endif // COIN_THREADSAFE
  glimage_bufferstorage = new SbStorage(sizeof(soglimage_buffer),
                                        glimage_buffer_construct, glimage_buffer_destruct);

  coin_atexit((coin_atexit_f*)SoGLImage::cleanupClass, CC_ATEXIT_NORMAL);

  SoGLCubeMapImage::initClass();
}

/*!
  This static method cleans up static data for the SoGLImage class.
*/
void
SoGLImage::cleanupClass(void)
{
  delete glimage_bufferstorage;
  glimage_bufferstorage = NULL;
#ifdef COIN_THREADSAFE
  delete SoGLImageP::mutex;
  SoGLImageP::mutex = NULL;
#endif // COIN_THREADSAFE
  SoGLImageP::classTypeId STATIC_SOTYPE_INIT;

  SoGLImageP::resizecb = NULL;
  SoGLImageP::resizeclosure = NULL;
  SoGLImageP::current_glimageid = 1;
}

/*!
  This static method returns the SoType object associated with
  objects of this class.
*/
SoType
SoGLImage::getClassTypeId(void)
{
  assert(!SoGLImageP::classTypeId.isBad());
  return SoGLImageP::classTypeId;
}

/*!
  Returns the type identification of an object derived from a
  class inheriting SoGLImage. This is used for runtime type checking and
  "downward" casting.
*/
SoType
SoGLImage::getTypeId(void) const
{
  return SoGLImage::getClassTypeId();
}

/*!
  Returns \c TRUE if the type of this object is either of the
  same type or inherited from \a type. This is used for runtime type
  checking and "downward" casting.
*/
SbBool
SoGLImage::isOfType(SoType type) const
{
  return this->getTypeId().isDerivedFrom(type);
}

/*!
  Can be used for creating a custom OpenGL texture inside an SoGLImage instance.

  Example use (creates a depth texture):

  SoGLDisplayList * depthmap = new SoGLDisplayList(state, SoGLDisplayList::TEXTURE_OBJECT);
  depthmap->ref();
  depthmap->open(state);

  glTexImage2D(GL_TEXTURE_2D, 0,
               GL_DEPTH_COMPONENT, // GL_DEPTH_COMPONENT24
               size[0], size[1],
               0,
               GL_DEPTH_COMPONENT,
               GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  depthmap->close(state);

  SoGLImage * image = new SoGLImage;
  image->setGLDisplayList(depthmap, state);

  \since Coin 2.5
*/

void
SoGLImage::setGLDisplayList(SoGLDisplayList * dl,
                            SoState * state,
                            const Wrap wraps,
                            const Wrap wrapt,
                            const float quality)
{
  if (PRIVATE(this)->isregistered) SoGLImage::unregisterImage(this);
  PRIVATE(this)->unrefDLists(state);
  dl->ref();
  PRIVATE(this)->dlists.append(SoGLImageP::dldata(dl));
  PRIVATE(this)->image = NULL; // we have no data. Texture is organized outside this image
  PRIVATE(this)->wraps = wraps;
  PRIVATE(this)->wrapt = wrapt;
  PRIVATE(this)->glimageid = SoGLImageP::getNextGLImageId(); // assign an unique id to this image
  PRIVATE(this)->needtransparencytest = FALSE;
  PRIVATE(this)->hastransparency = FALSE;
  PRIVATE(this)->usealphatest = FALSE;
  PRIVATE(this)->quality = quality;

  // don't register this image. There's no way we can reload it if we
  // delete it because of old age.
}

/*!
  Sets the pbuffer for this texture. Experimental code, use with care.
*/
void
SoGLImage::setPBuffer(SoState * state,
                      void * pbuffer,
                      const Wrap wraps,
                      const Wrap wrapt,
                      const float quality)

{
  if (PRIVATE(this)->pbuffer && state) {
    // bind texture before releasing pbuffer
    this->getGLDisplayList(state)->call(state);
    cc_glglue_context_release_pbuffer(PRIVATE(this)->pbuffer);
  }

  if (PRIVATE(this)->isregistered) SoGLImage::unregisterImage(this);
  PRIVATE(this)->unrefDLists(state);
  PRIVATE(this)->init(); // init to default values

  if (pbuffer) {
    PRIVATE(this)->pbuffer = pbuffer;
    PRIVATE(this)->wraps = wraps;
    PRIVATE(this)->wrapt = wrapt;

    PRIVATE(this)->glimageid = SoGLImageP::getNextGLImageId(); // assign an unique id to this image
    PRIVATE(this)->needtransparencytest = TRUE;
    PRIVATE(this)->hastransparency = FALSE;
    PRIVATE(this)->usealphatest = FALSE;
    PRIVATE(this)->quality = quality;


    if (PRIVATE(this)->pbuffer && !PRIVATE(this)->isregistered &&
        !(this->getFlags() & INVINCIBLE)) {
      SoGLImage::registerImage(this);
    }
  }
}


/*!
  Convenience 2D wrapper function around the 3D setData().
*/
void
SoGLImage::setData(const SbImage * image,
                   const Wrap wraps,
                   const Wrap wrapt,
                   const float quality,
                   const int border,
                   SoState * createinstate)

{
  this->setData(image, wraps, wrapt, (Wrap)PRIVATE(this)->wrapr,
                quality, border, createinstate);
}

/*!
  Sets the data for this GL image. Should only be called when one
  of the parameters have changed, since this will cause the GL texture
  object to be recreated.  Caller is responsible for sending legal
  wrap values.  CLAMP_TO_EDGE is only supported on OpenGL v1.2
  implementations, and as an extension on some earlier SGI
  implementations (GL_SGIS_texture_edge_clamp).

  For now, if quality > 0.5 when created, we create mipmaps, otherwise
  a regular texture is created.  Be aware, if you for instance create
  a texture with texture quality 0.4, and then later try to apply the
  texture with a texture quality greater than 0.5, the texture object
  will be recreated as a mipmap texture object. This will happen only
  once though, of course.

  If \a border != 0, the OpenGL texture will be created with this
  border size. Be aware that this might be extremely slow on most PC
  hardware.

  Normally, the OpenGL texture object isn't created until the first
  time it is needed, but if \a createinstate is != NULL, the texture
  object is created immediately. This is useful if you use a temporary
  buffer to hold the texture data. Be careful when using this feature,
  since the texture data might be needed at a later stage (for
  instance to create a texture object for another context).  It will
  not be possible to create texture objects for other cache contexts
  when \a createinstate is != NULL.

  Also if \a createinstate is supplied, and all the attributes are the
  same as the current data in the image, glTexSubImage() will be used
  to insert the image data instead of creating a new texture object.
  This is much faster on most OpenGL drivers, and is very useful, for
  instance when doing animated textures.

  If you supply NULL for \a image, the instance will be reset, causing
  all display lists and memory to be freed.
*/
void
SoGLImage::setData(const SbImage *image,
                   const Wrap wraps,
                   const Wrap wrapt,
                   const Wrap wrapr,
                   const float quality,
                   const int border,
                   SoState *createinstate)

{
  PRIVATE(this)->imageage = 0;

  if (image == NULL) {
    PRIVATE(this)->unrefDLists(createinstate);
    if (PRIVATE(this)->isregistered) SoGLImage::unregisterImage(this);
    PRIVATE(this)->init(); // init to default values
    return;
  }

  PRIVATE(this)->glimageid = SoGLImageP::getNextGLImageId(); // assign an unique id to this image
  PRIVATE(this)->needtransparencytest = TRUE;
  PRIVATE(this)->hastransparency = FALSE;
  PRIVATE(this)->usealphatest = FALSE;
  PRIVATE(this)->quality = quality;

  // check for special case where glTexSubImage can be used.
  // faster for most drivers.
  if (createinstate) { // We need the state for cc_glglue
    const cc_glglue * glw = sogl_glue_instance(createinstate);
    SoGLDisplayList *dl = NULL;

    SbBool copyok =
      wraps == PRIVATE(this)->wraps &&
      wrapt == PRIVATE(this)->wrapt &&
      wrapr == PRIVATE(this)->wrapr &&
      border == PRIVATE(this)->border &&
      border == 0 && // haven't tested with borders yet. Play it safe.
      (dl = PRIVATE(this)->findDL(createinstate)) != NULL;

    SbVec3s size;
    int nc;
    const unsigned char * bytes = image->getValue(size, nc);
    copyok = copyok && bytes && (size == PRIVATE(this)->glsize) && (nc == PRIVATE(this)->glcomp);

    SbBool is3D = (size[2]==0)?FALSE:TRUE;
    SbBool usesubimage = COIN_TEX2_USE_GLTEXSUBIMAGE &&
      ((is3D && SoGLDriverDatabase::isSupported(glw, SO_GL_3D_TEXTURES)) ||
       (!is3D && SoGLDriverDatabase::isSupported(glw, SO_GL_TEXSUBIMAGE)));

    if (!usesubimage) copyok=FALSE;
    if (PRIVATE(this)->flags & RECTANGLE) copyok = FALSE;

    if (copyok) {
      dl->ref();
      PRIVATE(this)->unrefDLists(createinstate);
      PRIVATE(this)->dlists.append(SoGLImageP::dldata(dl));
      PRIVATE(this)->image = NULL; // data is temporary, and only for current context
      dl->call(createinstate);

      SbBool compress =
        (PRIVATE(this)->flags & COMPRESSED) &&
        SoGLDriverDatabase::isSupported(glw, SO_GL_TEXTURE_COMPRESSION);

      if (dl->isMipMapTextureObject()) {
        if (is3D)
          fast_mipmap(createinstate, size[0], size[1], size[2], nc, bytes,
                      TRUE, compress);
        else
          fast_mipmap(createinstate, size[0], size[1], nc, bytes,
                      TRUE, compress);
      }
      else {
        GLenum format = coin_glglue_get_texture_format(glw, nc);
        if (is3D) {
          cc_glglue_glTexSubImage3D(glw, GL_TEXTURE_3D, 0, 0, 0, 0,
                                    size[0], size[1], size[2],
                                    format, GL_UNSIGNED_BYTE,
                                    (void*) bytes);
        }
        else {
          cc_glglue_glTexSubImage2D(glw, GL_TEXTURE_2D, 0, 0, 0,
                                    size[0], size[1],
                                    format, GL_UNSIGNED_BYTE,
                                    (void*) bytes);
        }
      }
    }
    else {
      PRIVATE(this)->image = image;
      PRIVATE(this)->wraps = wraps;
      PRIVATE(this)->wrapt = wrapt;
      PRIVATE(this)->wrapr = wrapr;
      PRIVATE(this)->border = border;
      PRIVATE(this)->unrefDLists(createinstate);
      if (createinstate) {
        PRIVATE(this)->dlists.append(SoGLImageP::dldata(PRIVATE(this)->createGLDisplayList(createinstate)));
        PRIVATE(this)->image = NULL; // data is assumed to be temporary
      }
    }
  }
  else {
    PRIVATE(this)->image = image;
    PRIVATE(this)->wraps = wraps;
    PRIVATE(this)->wrapt = wrapt;
    PRIVATE(this)->wrapr = wrapr;
    PRIVATE(this)->border = border;
    PRIVATE(this)->unrefDLists(createinstate);
  }

  if (PRIVATE(this)->image && !PRIVATE(this)->isregistered && !(this->getFlags() & INVINCIBLE)) {
    SoGLImage::registerImage(this);
  }
}

/*!
  2D setData() wrapper. Supplies raw data, size and numcomponents instead of
  an SbImage. Creates a temporary image, then calls the read setData().
  \overload
*/
void
SoGLImage::setData(const unsigned char *bytes,
                   const SbVec2s & size,
                   const int numcomponents,
                   const Wrap wraps,
                   const Wrap wrapt,
                   const float quality,
                   const int border,
                   SoState *createinstate)
{
  PRIVATE(this)->dummyimage.setValuePtr(size, numcomponents, bytes);
  this->setData(&PRIVATE(this)->dummyimage,
                wraps, wrapt, quality,
                border, createinstate);
}

/*!
  3D setData() wrapper. Supplies raw data, size and numcomponents instead of
  an SbImage. Creates a temporary image, then calls the read setData().
  \overload
*/
void
SoGLImage::setData(const unsigned char *bytes,
                   const SbVec3s & size,
                   const int numcomponents,
                   const Wrap wraps,
                   const Wrap wrapt,
                   const Wrap wrapr,
                   const float quality,
                   const int border,
                   SoState *createinstate)
{
  PRIVATE(this)->dummyimage.setValuePtr(size, numcomponents, bytes);
  this->setData(&PRIVATE(this)->dummyimage,
                wraps, wrapt, wrapr, quality,
                border, createinstate);
}


/*!
  Destructor.
*/
SoGLImage::~SoGLImage()
{
  SoContextHandler::removeContextDestructionCallback(SoGLImageP::contextCleanup, PRIVATE(this));
  if (PRIVATE(this)->isregistered) SoGLImage::unregisterImage(this);
  PRIVATE(this)->unrefDLists(NULL);
  delete PRIVATE(this);
}

/*!
  This class has a private destructor since we want users to supply
  the current GL state when deleting the image. This is to make sure
  GL texture objects are freed as soon as possible. If you supply
  NULL to this method, the GL texture objects won't be deleted
  until the next time an GLRenderAction is applied in the image's
  cache context(s).
*/
void
SoGLImage::unref(SoState *state)
{
  if (PRIVATE(this)->pbuffer) this->setPBuffer(state, NULL);
  PRIVATE(this)->unrefDLists(state);
  delete this;
}

/*!
  Sets flags to control how the texture is handled/initialized.
*/
void
SoGLImage::setFlags(const uint32_t flags)
{
  PRIVATE(this)->flags = flags;
}

/*!
  Returns the flags.

  \sa setFlags()
*/
uint32_t
SoGLImage::getFlags(void) const
{
  return PRIVATE(this)->flags;
}

/*!
  Returns a pointer to the image data.
*/
const SbImage *
SoGLImage::getImage(void) const
{
  return PRIVATE(this)->image;
}

/*!
  Returns or creates a SoGLDisplayList to be used for rendering.
  Returns NULL if no SoGLDisplayList could be created.
*/
SoGLDisplayList *
SoGLImage::getGLDisplayList(SoState *state)
{
  LOCK_GLIMAGE;
  SoGLDisplayList *dl = PRIVATE(this)->findDL(state);
  UNLOCK_GLIMAGE;

  if (dl == NULL) {
    dl = PRIVATE(this)->createGLDisplayList(state);
    if (dl) {
      LOCK_GLIMAGE;
      PRIVATE(this)->dlists.append(SoGLImageP::dldata(dl));
      UNLOCK_GLIMAGE;
    }
  }
  if (dl && !dl->isMipMapTextureObject() && PRIVATE(this)->image) {
    float quality = SoTextureQualityElement::get(state);
    float oldquality = PRIVATE(this)->quality;
    PRIVATE(this)->quality = quality;
    if (PRIVATE(this)->shouldCreateMipmap()) {
      LOCK_GLIMAGE;
      // recreate DL to get a mipmapped image
      int n = PRIVATE(this)->dlists.getLength();
      for (int i = 0; i < n; i++) {
        if (PRIVATE(this)->dlists[i].dlist == dl) {
          dl->unref(state); // unref old DL
          dl = PRIVATE(this)->createGLDisplayList(state);
          PRIVATE(this)->dlists[i].dlist = dl;
          break;
        }
      }
      UNLOCK_GLIMAGE;
    }
    else PRIVATE(this)->quality = oldquality;
  }
  return dl;
}


/*!
  Returns \e TRUE if this texture has some pixels with alpha value != 255
*/
SbBool
SoGLImage::hasTransparency(void) const
{
  if (PRIVATE(this)->flags & FORCE_TRANSPARENCY_TRUE) return TRUE;
  if (PRIVATE(this)->flags & FORCE_TRANSPARENCY_FALSE) return FALSE;

  if (PRIVATE(this)->needtransparencytest) {
    ((SoGLImage*)this)->pimpl->checkTransparency();
  }
  return PRIVATE(this)->hastransparency;
}

/*!
  Returns TRUE if this image has some alpha value != 255, and all
  these values are 0. If this is the case, alpha test can be used
  to render this texture instead of for instance blending, which
  is usually slower and might yield z-buffer artifacts.
*/
SbBool
SoGLImage::useAlphaTest(void) const
{
  if (PRIVATE(this)->flags & FORCE_ALPHA_TEST_TRUE) return TRUE;
  if (PRIVATE(this)->flags & FORCE_ALPHA_TEST_FALSE) return FALSE;

  if (PRIVATE(this)->needtransparencytest) {
    ((SoGLImage*)this)->pimpl->checkTransparency();
  }
  return PRIVATE(this)->usealphatest;
}

/*!
  Returns the wrap strategy for the S (horizontal) direction.
*/
SoGLImage::Wrap
SoGLImage::getWrapS(void) const
{
  return PRIVATE(this)->wraps;
}

/*!
  Returns the wrap strategy for the T (vertical) direction.
*/
SoGLImage::Wrap
SoGLImage::getWrapT(void) const
{
  return PRIVATE(this)->wrapt;
}

/*!
  Returns the wrap strategy for the R (depth) direction.
*/
SoGLImage::Wrap
SoGLImage::getWrapR(void) const
{
  return PRIVATE(this)->wrapr;
}

/*!
  Returns the texture quality for this texture image.

  \since Coin 2.5
*/
float
SoGLImage::getQuality(void) const
{
  return PRIVATE(this)->quality;
}

/*!
  Returns an unique id for this GL image. This id can be used to
  test for changes in an SoGLImage's internal data.
*/
uint32_t
SoGLImage::getGLImageId(void) const
{
  return PRIVATE(this)->glimageid;
}

/*!
  Virtual method that will be called once each frame.  The method
  should unref display lists that have an age bigger or equal to \a
  maxage, and increment the age for other display lists.
*/
void
SoGLImage::unrefOldDL(SoState *state, const uint32_t maxage)
{
  PRIVATE(this)->unrefOldDL(state, maxage);
  this->incAge();
}

// *************************************************************************

void
SoGLImageP::init(void)
{
  assert(this->isregistered == FALSE);
  this->image = NULL;
  this->pbuffer = NULL;
  this->glsize.setValue(0,0,0);
  this->glcomp = 0;
  this->wraps = SoGLImage::CLAMP;
  this->wrapt = SoGLImage::CLAMP;
  this->wrapr = SoGLImage::CLAMP;
  this->border = 0;
  this->flags = SoGLImage::USE_QUALITY_VALUE;
  this->needtransparencytest = TRUE;
  this->hastransparency = FALSE;
  this->usealphatest = FALSE;
  this->quality = 0.4f;
  this->imageage = 0;
  this->endframecb = NULL;
  this->glimageid = 0; // glimageid 0 is an empty image
}

//
// resize image if necessary. Returns pointer to temporary
// buffer if that happens, and the new size in xsize, ysize.
//
void
SoGLImageP::resizeImage(SoState * state, unsigned char *& imageptr,
                        uint32_t & xsize, uint32_t & ysize, uint32_t & zsize)
{
  SbVec3s size;
  int numcomponents;
  unsigned char *bytes = this->image->getValue(size, numcomponents);

  uint32_t newx = xsize;
  uint32_t newy = ysize;
  uint32_t newz = zsize;

  uint32_t maxrectsize = 0;

  if (!(this->flags & SoGLImage::RECTANGLE)) {
    newx = coin_geq_power_of_two(xsize - 2*this->border);
    newy = coin_geq_power_of_two(ysize - 2*this->border);
    newz = zsize ? coin_geq_power_of_two(zsize - 2*this->border) : 0;

    // if >= 256 and low quality, don't scale up unless size is
    // close to an above power of two. This saves a lot of texture memory

    if (this->flags & SoGLImage::SCALE_DOWN) {
      // no use scaling down for very small images
      if (newx > xsize && newx > 16) newx >>= 1;
      if (newy > ysize && newy > 16) newy >>= 1;
      if (newz > zsize && newz > 16) newz >>= 1;
    }
    else if (this->flags & SoGLImage::USE_QUALITY_VALUE) {
      if (this->quality < COIN_TEX2_SCALEUP_LIMIT) {
        if ((newx >= 256) && ((newx - (xsize-2*this->border)) > (newx>>3)))
          newx >>= 1;
        if ((newy >= 256) && ((newy - (ysize-2*this->border)) > (newy>>3)))
          newy >>= 1;
        if ((newz >= 256) && ((newz - (zsize-2*this->border)) > (newz>>3)))
          newz >>= 1;
      }
    }
  }
  else {
    GLint maxr;
    glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT, &maxr);
    maxrectsize = (uint32_t) maxr;
  }


  // downscale to legal GL size (implementation dependent)
  const cc_glglue * glw = sogl_glue_instance(state);
  SbBool sizeok = FALSE;
#if COIN_DEBUG
  uint32_t orgsize[3] = { newx, newy, newz };
#endif // COIN_DEBUG
  while (!sizeok) {
    SbBool compressed =
      (this->flags & SoGLImage::COMPRESSED) ? TRUE : FALSE &&
      SoGLDriverDatabase::isSupported(glw, SO_GL_TEXTURE_COMPRESSION);

    if (this->flags & SoGLImage::RECTANGLE) {
      // FIXME: add support for rectangular textures in glglue proxy test
      sizeok = (newx <= maxrectsize) && (newy <= maxrectsize);
    }
    else {
      GLenum internalformat = coin_glglue_get_internal_texture_format(glw, numcomponents, compressed);
      GLenum format = coin_glglue_get_texture_format(glw, numcomponents);

      sizeok = coin_glglue_is_texture_size_legal(glw, newx, newy, newz,
                                                 internalformat,
                                                 format,
                                                 GL_UNSIGNED_BYTE,
                                                 this->shouldCreateMipmap());
    }
    if (!sizeok) {
      unsigned int max = SbMax(newx, SbMax(newy, newz));
      if (max==newz) newz >>= 1;
      else if (max==newy) newy >>= 1;
      else newx >>= 1;
    }

    if (newy == 0) { // Avoid endless loop in a buggy driver environment.
      SoDebugError::post("SoGLImageP::resizeImage",
                         "There is something seriously wrong with OpenGL on "
                         "this system -- can't find *any* valid texture "
                         "size! Expect further problems.");
      break;
    }
  }

#if COIN_DEBUG
  if (orgsize[0] != newx || orgsize[1] != newy || orgsize[2] != newz) {
    if (orgsize[2] != 0) {
      SoDebugError::postWarning("SoGLImageP::resizeImage",
                                "Original 3D texture too large for "
                                "your graphics hardware and / or OpenGL "
                                "driver. Rescaled from (%d x %d x %d) "
                                "to (%d x %d x %d).",
                                orgsize[0], orgsize[1], orgsize[2],
                                newx, newy, newz);
    }
    else {
      SoDebugError::postWarning("SoGLImageP::resizeImage",
                                "Original 2D texture too large for "
                                "your graphics hardware and / or OpenGL "
                                "driver. Rescaled from (%d x %d) "
                                "to (%d x %d).",
                                orgsize[0], orgsize[1], newx, newy);
    }
  }
#endif // COIN_DEBUG

  newx += 2 * this->border;
  newy += 2 * this->border;
  newz = (zsize==0)?0:newz + (2 * this->border);

  if ((newx != xsize) || (newy != ysize) || (newz != zsize)) {
    // We need to resize.

    int numbytes = newx * newy * ((newz==0)?1:newz) * numcomponents;
    unsigned char * glimage_tmpimagebuffer = glimage_get_buffer(numbytes, FALSE);

    // First check if there is a custom resize function registered
    SbBool customresizedone = FALSE;
    if (SoGLImageP::resizecb) {
      customresizedone = SoGLImageP::resizecb(state,
                                              SbVec3s(newx, newy, newz),
                                              glimage_tmpimagebuffer,
                                              SoGLImage::IMAGE,
                                              SoGLImageP::resizeclosure,
                                              this->owner);
    }

    if (!customresizedone) {
      // simage version 1.1.1 has a pretty high quality resize
      // function. We prefer to use that to avoid using GLU, since
      // there are lots of buggy GLU libraries out there.
      if (zsize == 0) { // 2D image
        // simage_resize and gluScaleImage can be pretty slow. Use
        // fast_image_resize() if high quality isn't needed
        if (SoTextureScaleQualityElement::get(state) < 0.5f) {
          fast_image_resize(bytes, glimage_tmpimagebuffer,
                            xsize, ysize, numcomponents,
                            newx, newy);
        }
        else if (simage_wrapper()->available &&
                 simage_wrapper()->versionMatchesAtLeast(1,1,1) &&
                 simage_wrapper()->simage_resize) {

          unsigned char *result =
            simage_wrapper()->simage_resize((unsigned char*) bytes,
                                            xsize, ysize, numcomponents,
                                            newx, newy);
          (void)memcpy(glimage_tmpimagebuffer, result, numbytes);
          simage_wrapper()->simage_free_image(result);
        }
        else if (GLUWrapper()->available) {
          glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
          glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
          glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
          glPixelStorei(GL_PACK_ROW_LENGTH, 0);
          glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
          glPixelStorei(GL_PACK_SKIP_ROWS, 0);
          glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
          glPixelStorei(GL_PACK_ALIGNMENT, 1);

          // FIXME: ignoring the error code. Silly. 20000929 mortene.
          (void)GLUWrapper()->gluScaleImage(coin_glglue_get_texture_format(glw, numcomponents),
                                            xsize, ysize,
                                            GL_UNSIGNED_BYTE, bytes,
                                            newx, newy, GL_UNSIGNED_BYTE,
                                            glimage_tmpimagebuffer);
          glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
          glPixelStorei(GL_PACK_ALIGNMENT, 4);
        }
        else { // fall back to the internal low-quality resize function
          fast_image_resize(bytes, glimage_tmpimagebuffer,
                            xsize, ysize, numcomponents,
                            newx, newy);
        }
      }
      else { // (zsize > 0) => 3D image
        if (simage_wrapper()->available &&
            simage_wrapper()->versionMatchesAtLeast(1,3,0) &&
            simage_wrapper()->simage_resize3d) {
          unsigned char *result =
            simage_wrapper()->simage_resize3d((unsigned char*) bytes,
                                              xsize, ysize, numcomponents, zsize,
                                              newx, newy, newz);
          (void)memcpy(glimage_tmpimagebuffer, result, numbytes);
          simage_wrapper()->simage_free_image(result);
        }
        else {
          // fall back to the internal low-quality resize function
          fast_image_resize3d(bytes, glimage_tmpimagebuffer,
                              xsize, ysize, numcomponents, zsize,
                              newx, newy, newz);
        }
      }
    }
    imageptr = glimage_tmpimagebuffer;
  }
  xsize = newx;
  ysize = newy;
  zsize = newz;
}

//
// private method that in addition to creating the display list,
// tests the size of the image and performs a resize if the size is not
// a power of two.
// reallyCreateTexture is called (only) from here.
//
SoGLDisplayList *
SoGLImageP::createGLDisplayList(SoState *state)
{
  SbVec3s size;
  int numcomponents;
  unsigned char *bytes =
    this->image ? this->image->getValue(size, numcomponents) : NULL;

  if (!this->pbuffer && !bytes) return NULL;

  uint32_t xsize = size[0];
  uint32_t ysize = size[1];
  uint32_t zsize = size[2];
  SbBool is3D = (size[2]==0)?FALSE:TRUE;

  // these might change if image is resized
  unsigned char *imageptr = (unsigned char *) bytes;

  const cc_glglue * glw = sogl_glue_instance(state);
  SbBool mipmap = this->shouldCreateMipmap();

  if (imageptr) {
    if (is3D ||
        (!SoGLDriverDatabase::isSupported(glw, SO_GL_NON_POWER_OF_TWO_TEXTURES) ||
         (mipmap && (!SoGLDriverDatabase::isSupported(glw, SO_GL_GENERATE_MIPMAP) &&
                     !SoGLDriverDatabase::isSupported(glw, "GL_SGIS_generate_mipmap"))))) {
      this->resizeImage(state, imageptr, xsize, ysize, zsize);
    }
  }
  SoCacheElement::setInvalid(TRUE);
  if (state->isCacheOpen()) {
    SoCacheElement::invalidate(state);
  }
  SoGLDisplayList *dl = new SoGLDisplayList(state,
                                            SoGLDisplayList::TEXTURE_OBJECT,
                                            1, mipmap);
  dl->ref();

  if (bytes) {
    if (is3D) {
      dl->setTextureTarget((int) GL_TEXTURE_3D);
    }
    else {
      dl->setTextureTarget((int) ((this->flags & SoGLImage::RECTANGLE) ?
                                  GL_TEXTURE_RECTANGLE_EXT : GL_TEXTURE_2D));
    }
  }

  dl->open(state);

  if (this->pbuffer) {
    this->reallyBindPBuffer(state);
  }
  else {
    this->reallyCreateTexture(state, imageptr, numcomponents,
                              xsize, ysize, zsize,
                              dl->getType() == SoGLDisplayList::DISPLAY_LIST,
                              mipmap,
                              this->border);
  }
  dl->close(state);
  return dl;
}

//
// Test image data for transparency by checking each texel.
//
void
SoGLImageP::checkTransparency(void)
{
  this->needtransparencytest = FALSE;
  this->usealphatest = FALSE;
  this->hastransparency = FALSE;

  SbVec3s size;
  int numcomponents;
  unsigned char *bytes = this->image ?
    this->image->getValue(size, numcomponents) : NULL;

  if (bytes == NULL) {
    if (this->glcomp == 2 || this->glcomp == 4) {
      // we must assume it has transparency, and that we
      // can't use alpha testing
      this->hastransparency = TRUE;
    }
  }
  else {
    if (numcomponents == 2 || numcomponents == 4) {
      int n = size[0] * size[1] * (size[2] ? size[2] : 1);
      int nc = numcomponents;
      unsigned char *ptr = (unsigned char *) bytes + nc - 1;

      while (n) {
        if (*ptr != 255 && *ptr != 0) break;
        if (*ptr == 0) this->usealphatest = TRUE;
        ptr += nc;
        n--;
      }
      if (n > 0) {
        this->hastransparency = TRUE;
        this->usealphatest = FALSE;
      }
      else {
        this->hastransparency = this->usealphatest;
      }
    }
  }
}

static GLenum
translate_wrap(SoState *state, const SoGLImage::Wrap wrap)
{
  if (wrap == SoGLImage::REPEAT) return (GLenum) GL_REPEAT;
  if (wrap == SoGLImage::CLAMP_TO_BORDER) return (GLenum) GL_CLAMP_TO_BORDER;
  if (COIN_ENABLE_CONFORMANT_GL_CLAMP) {
    if (wrap == SoGLImage::CLAMP_TO_EDGE) {
      const cc_glglue * glw = sogl_glue_instance(state);
      if (SoGLDriverDatabase::isSupported(glw, SO_GL_TEXTURE_EDGE_CLAMP)) return (GLenum) GL_CLAMP_TO_EDGE;
    }
    return (GLenum) GL_CLAMP;
  }
  const cc_glglue * glw = sogl_glue_instance(state);
  if (SoGLDriverDatabase::isSupported(glw, SO_GL_TEXTURE_EDGE_CLAMP)) return (GLenum) GL_CLAMP_TO_EDGE;
  return (GLenum) GL_CLAMP;
}

void
SoGLImageP::reallyBindPBuffer(SoState * state)
{
  GLenum target = this->flags & SoGLImage::RECTANGLE ?
    GL_TEXTURE_RECTANGLE_EXT : GL_TEXTURE_2D;

  glTexParameteri(target, GL_TEXTURE_WRAP_S,
                  translate_wrap(state, this->wraps));
  glTexParameteri(target, GL_TEXTURE_WRAP_T,
                  translate_wrap(state, this->wrapt));

  SbBool mipmap = FALSE;

#if 0
  // disabled, we probably need to allocate space for the mipmaps in
  // the pbuffer pederb, 2003-11-27
  if (this->shouldCreateMipmap() && SoGLDriverDatabase::isSupported(glue, "GL_SGIS_generate_mipmap")) {
    glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
    // glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
    mipmap = TRUE;
  }
#endif // disabled

  this->applyFilter(mipmap);
  cc_glglue_context_bind_pbuffer(this->pbuffer);
}

void
SoGLImageP::reallyCreateTexture(SoState *state,
                                const unsigned char *const texture,
                                const int numComponents,
                                const int w, const int h, const int d,
                                const SbBool COIN_UNUSED_ARG(dlist), //FIXME: Not in use (kintel 20011129)
                                const SbBool mipmap,
                                const int border)
{
  const cc_glglue * glw = sogl_glue_instance(state);
  this->glsize = SbVec3s((short) w, (short) h, (short) d);
  this->glcomp = numComponents;

  SbBool compress =
    (this->flags & SoGLImage::COMPRESSED) &&
    SoGLDriverDatabase::isSupported(glw, SO_GL_TEXTURE_COMPRESSION);
  GLint internalFormat =
    coin_glglue_get_internal_texture_format(glw, numComponents, compress);
  GLenum dataFormat = coin_glglue_get_texture_format(glw, numComponents);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  //FIXME: Check cc_glglue capability as well? (kintel 20011129)
  if (SoMultiTextureEnabledElement::getMode(state) == 
      SoMultiTextureEnabledElement::TEXTURE3D) { // 3D textures
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S,
                    translate_wrap(state, this->wraps));
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T,
                    translate_wrap(state, this->wrapt));
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R,
                    translate_wrap(state, this->wrapr));


    this->applyFilter(mipmap);

    if (!mipmap) {
      if (SoGLDriverDatabase::isSupported(glw, SO_GL_3D_TEXTURES)) {
        cc_glglue_glTexImage3D(glw, GL_TEXTURE_3D, 0, internalFormat, w, h, d,
                               border, dataFormat, GL_UNSIGNED_BYTE, texture);
      }
    }
    else { // mipmaps
      // We used to default to calling GLU's gluBuild3DMipmaps() here,
      // but that was axed, because the gluBuild[2|3]DMipmaps()
      // functions implicitly uses glGenTextures() and other OpenGL
      // 1.1+ functions -- which again can cause trouble when doing
      // remote rendering. (At least we've had lots of problems with
      // NVidia's GLX implementation for non-1.0 OpenGL stuff.)
      //
      //   (void)GLUWrapper()->gluBuild3DMipmaps(GL_TEXTURE_3D, internalFormat,
      //                                         w, h, d, dataFormat,
      //                                         GL_UNSIGNED_BYTE, texture);

      fast_mipmap(state, w, h, d, numComponents, texture, FALSE, compress);
    }
  }
  else { // 2D textures
    SbBool mipmapimage = mipmap;
    SbBool mipmapfilter = mipmap;
    SbBool generatemipmap = FALSE;

    GLenum target = this->flags & SoGLImage::RECTANGLE ?
      GL_TEXTURE_RECTANGLE_EXT : GL_TEXTURE_2D;

    glTexParameteri(target, GL_TEXTURE_WRAP_S,
                    translate_wrap(state, this->wraps));
    glTexParameteri(target, GL_TEXTURE_WRAP_T,
                    translate_wrap(state, this->wrapt));

    if (mipmap && (this->flags & SoGLImage::RECTANGLE)) {
      mipmapimage = FALSE;
      if (SoGLDriverDatabase::isSupported(glw, "GL_SGIS_generate_mipmap")) {
        glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
      }
      else mipmapfilter = FALSE;
    }
    // prefer GL_SGIS_generate_mipmap to glGenerateMipmap. It seems to
    // be better supported in drivers.
    else if (mipmap && SoGLDriverDatabase::isSupported(glw, "GL_SGIS_generate_mipmap")) {
      glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
      mipmapimage = FALSE;
    }
    // using glGenerateMipmap() while creating a display list is not
    // supported (even if the display list is never used). This is
    // probably because the OpenGL driver creates each mipmap level by
    // rendering it using normal OpenGL calls.
    else if (mipmap && SoGLDriverDatabase::isSupported(glw, SO_GL_GENERATE_MIPMAP) && !state->isCacheOpen()) {
      mipmapimage = FALSE;
      generatemipmap = TRUE; // delay until after the texture image is set up
    }
    if ((this->quality > COIN_TEX2_ANISOTROPIC_LIMIT) &&
        SoGLDriverDatabase::isSupported(glw, SO_GL_ANISOTROPIC_FILTERING)) {
      glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                      cc_glglue_get_max_anisotropy(glw));
    }
    if (!mipmapimage) {
      // Create only level 0 texture. Mimpamps might be created by glGenerateMipmap
      glTexImage2D(target, 0, internalFormat, w, h,
                   border, dataFormat, GL_UNSIGNED_BYTE, texture);

      if (generatemipmap) {
        SbBool wasenabled = TRUE;
        // Workaround for ATi driver bug. GL_TEXTURE_2D needs to be
        // enabled when using glGenerateMipmap(), according to
        // discussions on the opengl.org forums.
        if (glw->vendor_is_ati) {
          if (!glIsEnabled(GL_TEXTURE_2D)) {
            wasenabled = FALSE;
            glEnable(GL_TEXTURE_2D);
          }
        }
        cc_glglue_glGenerateMipmap(glw, target);
        if (!wasenabled) glDisable(GL_TEXTURE_2D);
      }
    }
    else { // mipmaps
      // The GLU function invocation has been disabled, for the
      // reasons stated in the code comments ~20 lines above on the
      // construction of 3D mipmap textures.
      //
      //   (void)GLUWrapper()->gluBuild2DMipmaps(GL_TEXTURE_2D, internalFormat,
      //                                         w, h, dataFormat,
      //                                         GL_UNSIGNED_BYTE, texture);
      fast_mipmap(state, w, h, numComponents, texture, FALSE, compress);
    }
    // apply the texture filters
    this->applyFilter(mipmapfilter);
  }
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

//
// unref all dlists stored in image
//
void
SoGLImageP::unrefDLists(SoState *state)
{
  int n = this->dlists.getLength();
  for (int i = 0; i < n; i++) {
    this->dlists[i].dlist->unref(state);
  }
  this->dlists.truncate(0);
}

// find dl for a context, NULL if not found
SoGLDisplayList *
SoGLImageP::findDL(SoState *state)
{
  int currcontext = SoGLCacheContextElement::get(state);
  int i, n = this->dlists.getLength();
  SoGLDisplayList *dl;
  for (i = 0; i < n; i++) {
    dl = this->dlists[i].dlist;
    if (dl->getContext() == currcontext) return dl;
  }
  return NULL;
}

void
SoGLImageP::tagDL(SoState *state)
{
  int currcontext = SoGLCacheContextElement::get(state);
  int i, n = this->dlists.getLength();
  SoGLDisplayList *dl;
  for (i = 0; i < n; i++) {
    dl = this->dlists[i].dlist;
    if (dl->getContext() == currcontext) {
      this->dlists[i].age = 0;
      break;
    }
  }
}

void
SoGLImage::incAge(void) const
{
  PRIVATE(this)->imageage++;
}

void
SoGLImage::resetAge(void) const
{
  PRIVATE(this)->imageage = 0;
}

void
SoGLImageP::unrefOldDL(SoState *state, const uint32_t maxage)
{
  int n = this->dlists.getLength();
  int i = 0;

  while (i < n) {
    dldata & data = this->dlists[i];
    if (data.age >= maxage) {
#if COIN_DEBUG && 0 // debug
      SoDebugError::postInfo("SoGLImageP::unrefOldDL",
                             "DL killed because of old age: %p",
                             this->owner);
#endif // debug
      data.dlist->unref(state);
      this->dlists.removeFast(i);
      n--; // one less in list now
    }
    else {
      // increment age
      data.age++;
      i++;
    }
  }
}

SbBool
SoGLImageP::shouldCreateMipmap(void)
{
  if (this->flags & SoGLImage::USE_QUALITY_VALUE) {
    return this->quality >= COIN_TEX2_MIPMAP_LIMIT;
  }
  else {
    return (this->flags & SoGLImage::NO_MIPMAP) == 0;
  }
}

//
// Actually apply the texture filters using OpenGL calls.
//
void
SoGLImageP::applyFilter(const SbBool ismipmap)
{
  GLenum target;

  // Casting away const
  const SbVec3s size = this->image ? this->image->getSize() : this->glsize;

  if (size[2] >= 1) target = GL_TEXTURE_3D;
  else {
    target = this->flags & SoGLImage::RECTANGLE ?
      GL_TEXTURE_RECTANGLE_EXT : GL_TEXTURE_2D;
  }
  if (this->flags & SoGLImage::USE_QUALITY_VALUE) {
    if (this->quality < COIN_TEX2_LINEAR_LIMIT) {
      glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    else if ((this->quality < COIN_TEX2_MIPMAP_LIMIT) || !ismipmap) {
      glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else if (this->quality < COIN_TEX2_LINEAR_MIPMAP_LIMIT) {
      glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    }
    else { // max quality
      glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
  }
  else {
    if ((this->flags & SoGLImage::NO_MIPMAP) || !ismipmap) {
      glTexParameteri(target, GL_TEXTURE_MAG_FILTER,
                      (this->flags & SoGLImage::LINEAR_MAG_FILTER) ?
                      GL_LINEAR : GL_NEAREST);
      glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
                      (this->flags & SoGLImage::LINEAR_MIN_FILTER) ?
                      GL_LINEAR : GL_NEAREST);
    }
    else {
      glTexParameteri(target, GL_TEXTURE_MAG_FILTER,
                      (this->flags & SoGLImage::LINEAR_MAG_FILTER) ?
                      GL_LINEAR : GL_NEAREST);
      GLenum minfilter = GL_NEAREST_MIPMAP_NEAREST;
      if (this->flags & SoGLImage::LINEAR_MIPMAP_FILTER) {
        if (this->flags & SoGLImage::LINEAR_MIN_FILTER)
          minfilter = GL_LINEAR_MIPMAP_LINEAR;
        else
          minfilter = GL_LINEAR_MIPMAP_NEAREST;
      }
      else if (this->flags & SoGLImage::LINEAR_MIN_FILTER)
        minfilter = GL_NEAREST_MIPMAP_LINEAR;

      glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
                      minfilter);
    }
  }
}

// returns an unique uint32_t id for gl images
uint32_t
SoGLImageP::getNextGLImageId(void)
{
  return current_glimageid++;
}

// *************************************************************************

//
// Texture resource management.
//
// FIXME: consider sorting images on an LRU strategy to
// speed up the process of searching for GL-images to free.
//

static SbList <SoGLImage*> * glimage_reglist;
static uint32_t glimage_maxage = 60;

static void
regimage_cleanup(void)
{
  delete glimage_reglist;
  glimage_reglist = NULL;
  glimage_maxage = 60;
}

/*!
  When doing texture resource control, call this method before
  rendering the scene, typically in the viewer's actualRedraw().
  \a state should be your SoGLRenderAction state.

  \sa endFrame(), tagImage(), setDisplayListMaxAge()
*/
void
SoGLImage::beginFrame(SoState * /* state */)
{
  // nothing is done for now
}

/*!
  Should be called when a texture image is used. In Coin this is
  handled by SoGLTextureImageElement, but if you use an SoGLImage on
  your own, you should call this method to avoid that the display list
  is deleted too soon. \a state should be your SoGLRenderAction state,
  \a image the image you are about to use/have used.
*/
void
SoGLImage::tagImage(SoState *state, SoGLImage *image)
{
  assert(image);
  if (image) {
    LOCK_GLIMAGE;
    image->resetAge();
    image->pimpl->tagDL(state);
    UNLOCK_GLIMAGE;
  }
}

/*!
  Should be called after your scene is rendered. Old display
  lists will be deleted when you call this method. \a state
  should be your SoGLRenderAction state.

  \sa beginFrame(), tagImage(), setDisplayListMaxAge()
*/
void
SoGLImage::endFrame(SoState *state)
{
  if (glimage_reglist) {
    std::vector<std::pair<void (*)(void *), void *> > cb_list;
    LOCK_GLIMAGE;
    int n = glimage_reglist->getLength();
    cb_list.reserve(n);
    for (int i = 0; i < n; i++) {
      SoGLImage *img = (*glimage_reglist)[i];
      img->unrefOldDL(state, glimage_maxage);
      if (img->pimpl->endframecb)
        cb_list.push_back(std::make_pair(img->pimpl->endframecb,
                                         img->pimpl->endframeclosure));
    }
    UNLOCK_GLIMAGE;

    // the actual invocation of the callbacks should be performed outside
    // the locked region to avoid deadlocks
    for (std::vector<std::pair<void (*)(void *), void *> >::iterator it = cb_list.begin(),
           end = cb_list.end(); it != end; ++it)
      it->first(it->second);
  }
}

void
SoGLImage::setEndFrameCallback(void (*cb)(void *), void *closure)
{
  PRIVATE(this)->endframecb = cb;
  PRIVATE(this)->endframeclosure = closure;
}

int
SoGLImage::getNumFramesSinceUsed(void) const
{
  return PRIVATE(this)->imageage;
}

/*!
  Free all GL images currently used. This can be used to help the
  operating system and/or OpenGL driver's resource handling.  If you
  know you're not going to render for a while, maybe you're switching
  to a different application or something, calling this method could
  be a good idea since it will release all the texture memory used by
  your application.
*/
void
SoGLImage::freeAllImages(SoState *state)
{
  int oldmax = glimage_maxage;
  glimage_maxage = 0;
  // call begin/end with maxage 0 to free all images
  SoGLImage::beginFrame(state);
  SoGLImage::endFrame(state);
  glimage_maxage = oldmax;
}

/*!
  Set the maximum age for a texture object/display list.  The age
  of an image is the number of frames since it has been used.
  Default maximum age is 60.
*/
void
SoGLImage::setDisplayListMaxAge(const uint32_t maxage)
{
  glimage_maxage = maxage;
}

// used internally to keep track of the SoGLImages
void
SoGLImage::registerImage(SoGLImage *image)
{
  LOCK_GLIMAGE;
  if (glimage_reglist == NULL) {
    coin_atexit((coin_atexit_f *)regimage_cleanup, CC_ATEXIT_NORMAL);
    glimage_reglist = new SbList<SoGLImage*>;
  }
  assert(glimage_reglist->find(image) < 0);
  glimage_reglist->append(image);
  PRIVATE(image)->isregistered = TRUE;
  UNLOCK_GLIMAGE;
}

// used internally to keep track of the SoGLImages
void
SoGLImage::unregisterImage(SoGLImage *image)
{
  assert(glimage_reglist);

  LOCK_GLIMAGE;
  int idx = glimage_reglist->find(image);
  assert(idx >= 0);
  if (idx >= 0) {
    glimage_reglist->removeFast(idx);
  }
  PRIVATE(image)->isregistered = FALSE;
  UNLOCK_GLIMAGE;
}

/*!
  Sets a custom image resize function.

  \since Coin 2.5
*/
void
SoGLImage::setResizeCallback(SoGLImageResizeCB * f, void * closure)
{
  SoGLImageP::resizecb = f;
  SoGLImageP::resizeclosure = closure;
}

// *************************************************************************

//
// Callback from SoContextHandler
//
void
SoGLImageP::contextCleanup(uint32_t context, void * closure)
{
  SoGLImageP * thisp = (SoGLImageP *) closure;
#ifdef COIN_THREADSAFE
  SoGLImageP::mutex->lock();
#endif // COIN_THREADSAFE

  int n = thisp->dlists.getLength();
  int i = 0;

  while (i < n) {
    if (thisp->dlists[i].dlist->getContext() == (int) context) {
      thisp->dlists[i].dlist->unref(NULL);
      thisp->dlists.remove(i);
      n--;
    }
    else i++;
  }
#ifdef COIN_THREADSAFE
  SoGLImageP::mutex->unlock();
#endif // COIN_THREADSAFE
}

// *************************************************************************

#undef PRIVATE
#undef LOCK_GLIMAGE
#undef UNLOCK_GLIMAGE
