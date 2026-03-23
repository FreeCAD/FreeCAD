#ifndef COIN_SOSHAPE_BUMPRENDER_H
#define COIN_SOSHAPE_BUMPRENDER_H

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

#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/C/glue/gl.h>

#include "misc/SbHash.h"

// *************************************************************************

class SoState;
class SoLight;
class SoGLImage;
class SbMatrix;
class SoPrimitiveVertexCache;

// *************************************************************************

// FIXME: inherit from SoCache to avoid regenerating everything every frame

class soshape_bumprender {
public:
  soshape_bumprender(void);
  ~soshape_bumprender();

  void calcTangentSpace(const SoPrimitiveVertexCache * cache);
  void renderBump(SoState * state,
                  const SoPrimitiveVertexCache * cache,
                  SoLight * light, const SbMatrix & toobjectspace);
  void renderBumpSpecular(SoState * state,
                          const SoPrimitiveVertexCache * cache,
                          SoLight * light, const SbMatrix & toobjectspace);
  void renderNormal(SoState * state, const SoPrimitiveVertexCache * cache);

private:

  void initLight(SoLight * light, const SbMatrix & m);
  void calcTSBCoords(const SoPrimitiveVertexCache * cache, SoLight * light);
  SbVec3f getLightVec(const SbVec3f & v) const;
  void initPrograms(const cc_glglue * glue, SoState * state);
  void initDiffusePrograms(const cc_glglue * glue, SoState * state);

  void soshape_diffuseprogramdeletion(unsigned long key, void * value);
  void soshape_specularprogramdeletion(unsigned long key, void * value);

  struct spec_programidx {
    const cc_glglue * glue;
    GLuint dirlight;
    GLuint pointlight;
    GLuint fragment;
  };

  struct diffuse_programidx {
    const cc_glglue * glue;
    GLuint pointlight; // Pointlight diffuse rendering not implemented as a program yet.
    GLuint dirlight;
    GLuint normalrendering;
  };

  SbList <SbVec3f> cubemaplist;
  SbList <SbVec3f> tangentlist;

  SbVec3f lightvec;
  SbBool ispointlight;

  typedef SbHash<int, struct diffuse_programidx *> ContextId2DiffuseStruct;
  ContextId2DiffuseStruct diffuseprogramdict;

  typedef SbHash<int, struct spec_programidx *> ContextId2SpecStruct;
  ContextId2SpecStruct specularprogramdict;

  GLuint fragmentprogramid;
  GLuint dirlightvertexprogramid;
  GLuint pointlightvertexprogramid;
  SbBool programsinitialized;

  GLuint normalrenderingvertexprogramid;
  GLuint diffusebumpdirlightvertexprogramid;
  SbBool diffuseprogramsinitialized;
};

#endif // COIN_SOSHAPE_BUMPRENDER
