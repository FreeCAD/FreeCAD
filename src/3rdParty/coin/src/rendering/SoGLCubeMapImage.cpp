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
  \class SoGLCubeMapImage include/Inventor/misc/SoGLCubeMapImage.h
  \brief The SoGLCubeMapImage class is used to handle cube map textures.

  \COIN_CLASS_EXTENSION

  \since Coin 3.0
*/

// *************************************************************************

#include <Inventor/misc/SoGLCubeMapImage.h>

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
#include <Inventor/lists/SbList.h>
#include <Inventor/C/glue/gl.h>
#include <Inventor/misc/SoContextHandler.h>

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbMutex.h>
#endif // COIN_THREADSAFE

#include "tidbitsp.h"
#include "glue/glp.h"
#include "rendering/SoGL.h"

// *************************************************************************

static GLenum get_gltarget(SoGLCubeMapImage::Target target)
{
  GLenum ret;
  switch (target) {
  default:
  case SoGLCubeMapImage::POSITIVE_X:
    ret = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    break;
  case SoGLCubeMapImage::NEGATIVE_X:
    ret = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
    break;
  case SoGLCubeMapImage::POSITIVE_Y:
    ret = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
    break;
  case SoGLCubeMapImage::NEGATIVE_Y:
    ret = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
    break;
  case SoGLCubeMapImage::POSITIVE_Z:
    ret = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
    break;
  case SoGLCubeMapImage::NEGATIVE_Z:
    ret = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
    break;
  }
  return ret;
}

class SoGLCubeMapImageP {
public:
  SoGLCubeMapImageP(void) {
  }
  ~SoGLCubeMapImageP() { }

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
    SoGLDisplayList * dlist;
    uint32_t age;
  };

  SoGLDisplayList * findDL(SoState *state) {
    int currcontext = SoGLCacheContextElement::get(state);
    int i, n = this->dlists.getLength();
    SoGLDisplayList *dl;
    for (i = 0; i < n; i++) {
      dl = this->dlists[i].dlist;
      if (dl->getContext() == currcontext) return dl;
    }
    return NULL;
  }

  SbList <dldata> dlists;
  SbImage fakeimage;

  static SoType classTypeId;
  SbImage image[6];

#ifdef COIN_THREADSAFE
  static SbMutex * mutex;
#endif // !COIN_THREADSAFE

  inline void lock(void) {
#ifdef COIN_THREADSAFE
    SoGLCubeMapImageP::mutex->lock();
#endif // COIN_THREADSAFE
  }
  inline void unlock(void) {
#ifdef COIN_THREADSAFE
    SoGLCubeMapImageP::mutex->unlock();
#endif // COIN_THREADSAFE
  }

  static void contextCleanup(uint32_t context, void * closure)
  {
    SoGLCubeMapImageP * thisp = (SoGLCubeMapImageP *) closure;
    thisp->lock();
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
    thisp->unlock();
  }
};

SoType SoGLCubeMapImageP::classTypeId STATIC_SOTYPE_INIT;
#ifdef COIN_THREADSAFE
SbMutex * SoGLCubeMapImageP::mutex = NULL;
#endif // !COIN_THREADSAFE

#define PRIVATE(obj) (obj->pimpl)

// *************************************************************************

/*!
  Constructor.
*/
SoGLCubeMapImage::SoGLCubeMapImage(void)
{
  PRIVATE(this) = new SoGLCubeMapImageP;
  SoContextHandler::addContextDestructionCallback(SoGLCubeMapImageP::contextCleanup, PRIVATE(this));
}

/*!
  Destructor.
*/
SoGLCubeMapImage::~SoGLCubeMapImage()
{
  SoContextHandler::removeContextDestructionCallback(SoGLCubeMapImageP::contextCleanup, PRIVATE(this));
  delete PRIVATE(this);
}

// Doc in superclass.
void
SoGLCubeMapImage::unref(SoState * state)
{
  int n = PRIVATE(this)->dlists.getLength();
  for (int i = 0; i < n; i++) {
    PRIVATE(this)->dlists[i].dlist->unref(state);
  }
  PRIVATE(this)->dlists.truncate(0);
  inherited::unref(state);
}

/*!
  This static method initializes static data for the SoGLCubeMapImage class.
*/
void
SoGLCubeMapImage::initClass(void)
{
  assert(SoGLCubeMapImageP::classTypeId.isBad());
  SoGLCubeMapImageP::classTypeId =
    SoType::createType(SoGLImage::getClassTypeId(), SbName("GLCubeMapImage"));
#ifdef COIN_THREADSAFE
  SoGLCubeMapImageP::mutex = new SbMutex;
#endif // COIN_THREADSAFE
  coin_atexit((coin_atexit_f*)SoGLCubeMapImage::cleanupClass, CC_ATEXIT_NORMAL);
}

/*!
  This static method cleans up static data for the SoGLCubeMapImage class.
*/
void
SoGLCubeMapImage::cleanupClass(void)
{
#ifdef COIN_THREADSAFE
  delete SoGLCubeMapImageP::mutex;
  SoGLCubeMapImageP::mutex = NULL;
#endif // COIN_THREADSAFE
  SoGLCubeMapImageP::classTypeId STATIC_SOTYPE_INIT;
}

/*!
  This static method returns the SoType object associated with
  objects of this class.
*/
SoType
SoGLCubeMapImage::getClassTypeId(void)
{
  assert(!SoGLCubeMapImageP::classTypeId.isBad());
  return SoGLCubeMapImageP::classTypeId;
}

// Doc in superclass.
SoType
SoGLCubeMapImage::getTypeId(void) const
{
  return SoGLCubeMapImage::getClassTypeId();
}


void
SoGLCubeMapImage::setCubeMapImage(const Target target,
                                  const unsigned char * bytes,
                                  const SbVec2s & size,
                                  const int numcomponents)
{
  int idx = (int) target;
  PRIVATE(this)->image[idx].setValuePtr(size, numcomponents, bytes);

  PRIVATE(this)->lock();
  for (int i = 0; i < PRIVATE(this)->dlists.getLength(); i++) {
    PRIVATE(this)->dlists[i].dlist->unref(NULL);
  }
  PRIVATE(this)->dlists.truncate(0);
  PRIVATE(this)->unlock();

  // FIXME: this is a hack. Just set one of the images in
  // SoGLImage. Needed for rendering to work correctly.
  if (bytes) {
    this->SoGLImage::setData(bytes, size, numcomponents,
                             CLAMP_TO_EDGE, CLAMP_TO_EDGE,
                             0.9f, 0, NULL);
  }
}


void
SoGLCubeMapImage::setData(const SbImage * image,
                          const Wrap wraps,
                          const Wrap wrapt,
                          const float quality,
                          const int border,
                          SoState * createinstate)
{
  inherited::setData(image, wraps, wrapt, quality, border, createinstate);
  //  assert(0 && "Use setCubeMapImage to set data in SoGLCubeMapImage");
}

void
SoGLCubeMapImage::setData(const SbImage * image,
                          const Wrap wraps,
                          const Wrap wrapt,
                          const Wrap wrapr,
                          const float quality,
                          const int border,
                          SoState * createinstate)
{
  inherited::setData(image, wraps, wrapt, wrapr, quality, border, createinstate);
}

SoGLDisplayList *
SoGLCubeMapImage::getGLDisplayList(SoState * state)
{
  PRIVATE(this)->lock();
  SoGLDisplayList * dl = PRIVATE(this)->findDL(state);
  if (!dl) {
    dl = new SoGLDisplayList(state,
                             SoGLDisplayList::TEXTURE_OBJECT);
    if (dl) {
      dl->ref();
      dl->setTextureTarget((int) GL_TEXTURE_CUBE_MAP);

      dl->open(state);

      for (int i = 0; i < 6; i++) {
        const SbImage * img = &PRIVATE(this)->image[i];
        if (img->hasData()) {
          SbVec2s size;
          int numcomponents;
          unsigned char * bytes = img->getValue(size, numcomponents);
          GLenum format;
          switch (numcomponents) {
          default: // avoid compiler warnings
          case 1: format = GL_LUMINANCE; break;
          case 2: format = GL_LUMINANCE_ALPHA; break;
          case 3: format = GL_RGB; break;
          case 4: format = GL_RGBA; break;
          }

          // FIXME: resize image if not power of two
          glTexImage2D(get_gltarget((Target) i),
                       0, numcomponents, size[0], size[1], 0,
                       format, GL_UNSIGNED_BYTE, bytes);

        }
      }

      // FIXME: make it possible to configure filter and mipmap on/off
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      // FIXME: make it possible to configure wrap modes
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
      dl->close(state);
      PRIVATE(this)->dlists.append(SoGLCubeMapImageP::dldata(dl));
    }
  }
  PRIVATE(this)->unlock();
  return dl;
}

#undef PRIVATE
