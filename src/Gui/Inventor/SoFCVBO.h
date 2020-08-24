/**************************************************************************\
 * Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>
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

#ifndef FC_COIN_VBO_H
#define FC_COIN_VBO_H

#include <unordered_map>
#include <vector>

#include <Inventor/system/gl.h>
#include <Inventor/C/glue/gl.h>

#include "../InventorBase.h"

class SoState;

#ifndef FC_COIN_UNIQUE_ID_DEFINED
#define FC_COIN_UNIQUE_ID_DEFINED
typedef uint64_t SbFCUniqueId;
#endif

class SoFCVBO {
public:
  SoFCVBO(const GLenum target = GL_ARRAY_BUFFER,
          const GLenum usage = GL_STATIC_DRAW);
  virtual ~SoFCVBO();

  static void init(void);

  void setBufferData(const GLvoid * data, intptr_t size, SbFCUniqueId dataid = 0);
  void * allocBufferData(intptr_t size, SbFCUniqueId dataid = 0);
  SbFCUniqueId getBufferDataId(void) const;
  void getBufferData(const GLvoid *& data, intptr_t & size);
  void bindBuffer(SoState *state, uint32_t contextid);

  static void setVertexCountLimits(const int minlimit, const int maxlimit);
  static int getVertexCountMinLimit(void);
  static int getVertexCountMaxLimit(void);

  static SbBool shouldCreateVBO(SoState * state, const uint32_t contextid, const int numdata);
  static SbBool shouldRenderAsVertexArrays(SoState * statea,
                                           const uint32_t contextid,
                                           const int numdata);

protected:
  static void context_destruction_cb(uint32_t context, void * userdata);
  friend struct vbo_schedule;
  static void vbo_delete(void * closure, uint32_t contextid);

  GLenum target;
  GLenum usage;
  const GLvoid * data;
  intptr_t datasize;
  SbFCUniqueId dataid;
  SbBool didalloc;

  std::unordered_map<uint32_t, GLuint> vbohash;

private:
  SoFCVBO(const SoFCVBO & rhs); // N/A
  SoFCVBO & operator = (const SoFCVBO & rhs); // N/A
};

#endif // FC_COIN_VBO_H
// vim: noai:ts=2:sw=2
