#ifndef COIN_VBO_H
#define COIN_VBO_H

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
#endif /* !COIN_INTERNAL */

#include <Inventor/system/gl.h>
#include <Inventor/C/glue/gl.h>

#include "misc/SbHash.h"

class SoState;

class SoVBO {
 public:
  SoVBO(const GLenum target = GL_ARRAY_BUFFER,
        const GLenum usage = GL_STATIC_DRAW);
  ~SoVBO();

  static void init(void);

  void setBufferData(const GLvoid * data, intptr_t size, SbUniqueId dataid = 0);
  void * allocBufferData(intptr_t size, SbUniqueId dataid = 0);
  SbUniqueId getBufferDataId(void) const;
  void getBufferData(const GLvoid *& data, intptr_t & size);
  void bindBuffer(uint32_t contextid);

  static void setVertexCountLimits(const int minlimit, const int maxlimit);
  static int getVertexCountMinLimit(void);
  static int getVertexCountMaxLimit(void);

  static void testGLPerformance(const uint32_t contextid);
  static SbBool shouldCreateVBO(SoState * state, const uint32_t contextid, const int numdata);
  static SbBool shouldRenderAsVertexArrays(SoState * statea,
                                           const uint32_t contextid,
                                           const int numdata);

 private:
  static void context_created(const uint32_t contextid, void * closure);
  static SbBool isVBOFast(const uint32_t contextid);
  static void context_destruction_cb(uint32_t context, void * userdata);
  friend struct vbo_schedule;
  static void vbo_delete(void * closure, uint32_t contextid);

  GLenum target;
  GLenum usage;
  const GLvoid * data;
  intptr_t datasize;
  SbUniqueId dataid;
  SbBool didalloc;

  SbHash<uint32_t, GLuint> vbohash;
};

#endif // COIN_VERTEXARRAYINDEXER_H
