#ifndef COIN_SOGLBIGIMAGE_H
#define COIN_SOGLBIGIMAGE_H

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

#include <Inventor/SbBasic.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/misc/SoGLImage.h>

class COIN_DLL_API SoGLBigImage : public SoGLImage {
  typedef SoGLImage inherited;

public:

  SoGLBigImage();
  virtual void unref(SoState * state = NULL);

  static SoType getClassTypeId(void);
  virtual SoType getTypeId(void) const ;

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

  int initSubImages(const SbVec2s & subimagesize) const;
  void handleSubImage(const int idx, SbVec2f & start, SbVec2f & end,
                      SbVec2f & tcmul);
  void applySubImage(SoState * state, const int idx, const float quality,
                     const SbVec2s & projsize);
  SbBool exceededChangeLimit(void);
  static int setChangeLimit(const int limit);

  // will return NULL to avoid that SoGLTextureImageElement will
  // update the texture state.
  virtual SoGLDisplayList * getGLDisplayList(SoState * state);


protected:
  virtual void unrefOldDL(SoState * state, const uint32_t maxage);

public:
  static void initClass(void);

private:
  virtual ~SoGLBigImage();

  class SoGLBigImageP * pimpl;
  friend class SoGLBigImageP;
};

#endif // !COIN_SOGLBIGIMAGE_H
