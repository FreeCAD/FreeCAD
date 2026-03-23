#ifndef COIN_SBGLUTESSELLATOR_H
#define COIN_SBGLUTESSELLATOR_H

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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* ! COIN_INTERNAL */

// *************************************************************************

#include <Inventor/SbVec3f.h>
#include <Inventor/lists/SbList.h>

#include "glue/GLUWrapper.h"

// *************************************************************************

class SbGLUTessellator {
public:
  static SbBool available(void);

  SbGLUTessellator(void (*callback)(void * v0, void * v1, void * v2,
                                    void * data) = NULL, void * userdata = NULL);
  ~SbGLUTessellator(void);

  void beginPolygon(const SbVec3f & normal = SbVec3f(0.0f, 0.0f, 0.0f));
  void addVertex(const SbVec3f & v, void * data);
  void endPolygon(void);

  static SbBool preferred(void);

private:
  static void APIENTRY cb_begin(GLenum primitivetype, void * x);
  static void APIENTRY cb_vertex(void * vertex_data, void * x);
  static void APIENTRY cb_error(GLenum err, void * x);

  void (* callback)(void *, void *, void *, void *);
  void * cbdata;
  coin_GLUtessellator * tessobj;

  struct v { GLdouble c[3]; };
  SbList<struct v> coords;

  GLenum triangletessmode;
  unsigned int vertexidx;
  void * vertexdata[2];
  SbBool stripflipflop;
};

#endif // !COIN_SBGLUTESSELLATOR_H
