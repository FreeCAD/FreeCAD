#ifndef COIN_SOGLDISPLAYLIST_H
#define COIN_SOGLDISPLAYLIST_H

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

class SoState;
class SoGLDisplayListP;

// *************************************************************************

class COIN_DLL_API SoGLDisplayList {
public:
  enum Type {
    DISPLAY_LIST,
    TEXTURE_OBJECT
  };
  SoGLDisplayList(SoState * state, Type type, int allocnum = 1,
                  SbBool mipmaptexobj = FALSE);
  void ref(void);
  void unref(SoState * state = NULL);

  void open(SoState *state, int index = 0);
  void close(SoState *state);

  void call(SoState * state, int index = 0);
  void addDependency(SoState * state);

  SbBool isMipMapTextureObject(void) const;
  Type getType(void) const;
  int getNumAllocated(void) const;
  // this returns GLuint in Inventor, but we try to avoid including
  // gl.h in the header files so we just return unsigned int.
  unsigned int getFirstIndex(void) const;
  int getContext(void) const;

  void setTextureTarget(int target);
  int getTextureTarget(void) const;

private:
  ~SoGLDisplayList();
  SoGLDisplayListP * pimpl;
  void bindTexture(SoState *state);

  friend class SoGLCacheContextElement;
};

#endif // !COIN_SOGLDISPLAYLIST_H
