#ifndef COIN_SOGLIMAGE_H
#define COIN_SOGLIMAGE_H

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

// WARNING: This is work in progress. Do not expect this class to have
// a stable interface over a long period of time. It is installed
// because we need it in an internal project.

// FIXME: make sure we do a design review for this class for Coin v3.0
// pederb, 2001-11-28

#include <Inventor/SbBasic.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec3s.h>
#include <Inventor/SoType.h>
#include <cstddef>

class SoGLDisplayList;
class SoState;
class SbImage;

class COIN_DLL_API SoGLImage {
public:

  SoGLImage();
  virtual void unref(SoState * state = NULL);

  static SoType getClassTypeId(void);
  virtual SoType getTypeId(void) const ;
  virtual SbBool isOfType(SoType type) const;

  enum Wrap {
    REPEAT = 0,
    CLAMP,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER
  };

  enum ResizeReason {
    IMAGE = 0,
    SUBIMAGE,
    MIPMAP
  };

  typedef SbBool SoGLImageResizeCB(SoState * state,
                                   const SbVec3s &newsize,
                                   unsigned char * destbuffer,
                                   ResizeReason reason,
                                   void * closure,
                                   class SoGLImage * image);

  void setGLDisplayList(SoGLDisplayList * dl,
                        SoState * state,
                        const Wrap wraps = REPEAT,
                        const Wrap wrapt = REPEAT,
                        const float quality = 0.5f);

  void setPBuffer(SoState * state,
                  void * context,
                  const Wrap wraps = REPEAT,
                  const Wrap wrapt = REPEAT,
                  const float quality = 0.5f);

  void setData(const unsigned char * bytes,
               const SbVec2s & size,
               const int numcomponents,
               const Wrap wraps = REPEAT,
               const Wrap wrapt = REPEAT,
               const float quality = 0.5f,
               const int border = 0,
               SoState * createinstate = NULL);

  void setData(const unsigned char * bytes,
               const SbVec3s & size,
               const int numcomponents,
               const Wrap wraps = REPEAT,
               const Wrap wrapt = REPEAT,
               const Wrap wrapr = REPEAT,
               const float quality = 0.5f,
               const int border = 0,
               SoState * createinstate = NULL);

  virtual void setData(const SbImage * image,
                       const Wrap wraps = REPEAT,
                       const Wrap wrapt = REPEAT,
                       const float quality = 0.5f,
                       const int border = 0,
                       SoState * createinstate = NULL);

  virtual void setData(const SbImage * image,
                       const Wrap wraps,
                       const Wrap wrapt,
                       const Wrap wrapr,
                       const float quality = 0.5f,
                       const int border = 0,
                       SoState * createinstate = NULL);

  // these flags can be used to set texture properties.
  enum Flags {
    // mipmap, scaling and filtering settings
    SCALE_DOWN =                0x0001,
    NO_MIPMAP =                 0x0002,
    LINEAR_MAG_FILTER =         0x0004,
    LINEAR_MIN_FILTER =         0x0008,
    LINEAR_MIPMAP_FILTER =      0x0010,

    // use if you know your image properties.
    FORCE_TRANSPARENCY_TRUE   = 0x0020,
    FORCE_TRANSPARENCY_FALSE  = 0x0040,
    FORCE_ALPHA_TEST_TRUE     = 0x0080,
    FORCE_ALPHA_TEST_FALSE    = 0x0100,

    INVINCIBLE                = 0x0200, // never die of old age

    // use GL_NV_texture_rectangle or GL_EXT_texture_rectangle
    RECTANGLE                 = 0x0400,

    // Compress texture if available from OpenGL
    COMPRESSED                = 0x0800,

    // use quality value to decide mipmap, filtering and scaling. This
    // is the default.
    USE_QUALITY_VALUE         = 0X8000
  };

  void setFlags(const uint32_t flags);
  uint32_t getFlags(void) const;

  const SbImage * getImage(void) const;

  virtual SoGLDisplayList * getGLDisplayList(SoState * state);
  SbBool hasTransparency(void) const;
  SbBool useAlphaTest(void) const;
  Wrap getWrapS(void) const;
  Wrap getWrapT(void) const;
  Wrap getWrapR(void) const;

  float getQuality(void) const;
  uint32_t getGLImageId(void) const;

protected:

  void incAge(void) const;
  void resetAge(void) const;
  virtual void unrefOldDL(SoState * state, const uint32_t maxage);
  virtual ~SoGLImage();

private:

  class SoGLImageP * pimpl;
  friend class SoGLImageP;
  static void cleanupClass(void);

public:
  // internal methods for texture resource management
  static void beginFrame(SoState * state);
  static void tagImage(SoState * state, SoGLImage * image);
  static void endFrame(SoState * state);
  static void setDisplayListMaxAge(const uint32_t maxage);
  static void freeAllImages(SoState * state = NULL);

  void setEndFrameCallback(void (*cb)(void *), void * closure);
  int getNumFramesSinceUsed(void) const;

public:
  static void initClass(void);
  static void setResizeCallback(SoGLImageResizeCB * f, void * closure);

private:
  static void registerImage(SoGLImage * image);
  static void unregisterImage(SoGLImage * image);
};

#endif // !COIN_SOGLIMAGE_H
