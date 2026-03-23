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
  \class SoVertexArrayIndexer
  \brief The SoVertexArrayIndexer class is used to simplify index handling for vertex array rendering.

  FIXME: more doc. when/if this class is made public, pederb 20050111
*/

#include "rendering/SoVertexArrayIndexer.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/misc/SoGLDriverDatabase.h>

#include "tidbitsp.h"
#include "rendering/SoVBO.h"
#include "rendering/SoGL.h"
#include "coindefs.h"

#if COIN_WORKAROUND(COIN_MSVC, <= COIN_MSVC_6_0_VERSION)
// symbol length truncation
#pragma warning(disable:4786)
#endif // VC6.0

/*!
  Constructor
*/
SoVertexArrayIndexer::SoVertexArrayIndexer(void)
  : target(0),
    next(NULL),
    vbo(NULL),
    use_shorts(TRUE)
{
}

/*!
  Destructor
*/
SoVertexArrayIndexer::~SoVertexArrayIndexer()
{
  delete this->vbo;
  delete this->next;
}

inline void 
SoVertexArrayIndexer::addIndex(int32_t i) 
{
  if (i >= 65536) this->use_shorts = FALSE;
  this->indexarray.append(static_cast<GLint> (i));
}

/*!
  Adds a line to be indexed.
*/
void
SoVertexArrayIndexer::addLine(const int32_t v0,
                              const int32_t v1)
{
  if (this->target == 0) this->target = GL_LINES;
  if (this->target == GL_LINES) {
    this->addIndex(v0);
    this->addIndex(v1);
  }
  else {
    this->getNext()->addLine(v0, v1);
  }
}

/*!
  Adds a point to be indexed.
*/
void
SoVertexArrayIndexer::addPoint(const int32_t v0)
{
  if (this->target == 0) this->target = GL_POINTS;
  if (this->target == GL_POINTS) {
    this->addIndex(v0);
  }
  else {
    this->getNext()->addPoint(v0);
  }
}

/*!
  Adds a triangle to be indexed.
*/
void
SoVertexArrayIndexer::addTriangle(const int32_t v0,
                                  const int32_t v1,
                                  const int32_t v2)
{
  if (this->target == 0) this->target = GL_TRIANGLES;
  if (this->target == GL_TRIANGLES) {
    this->addIndex(v0);
    this->addIndex(v1);
    this->addIndex(v2);
  }
  else {
    this->getNext()->addTriangle(v0,v1,v2);
  }
}

/*!
  Adds a quad to be indexed.
*/
void
SoVertexArrayIndexer::addQuad(const int32_t v0,
                              const int32_t v1,
                              const int32_t v2,
                              const int32_t v3)
{
  if (this->target == 0) this->target = GL_QUADS;
  if (this->target == GL_QUADS) {
    this->addIndex(v0);
    this->addIndex(v1);
    this->addIndex(v2);
    this->addIndex(v3);
  }
  else {
    this->getNext()->addQuad(v0,v1,v2,v3);
  }
}

/*!
  Sets up indexer for new indices of type \a targetin. Use
  targetVertex() to add indices, and finish the target by using
  endTarget().

  \sa targetVertex()
  \sa endTarget()
*/
void
SoVertexArrayIndexer::beginTarget(GLenum targetin)
{
  if (this->target == 0) this->target = targetin;
  if (this->target == targetin) {
    this->targetcounter = 0;
  }
  else {
    this->getNext()->beginTarget(targetin);
  }
}

/*!
  Adds an index to the indexer.

  \sa beginTarget()
  \sa endTarget()
*/
void
SoVertexArrayIndexer::targetVertex(GLenum targetin, const int32_t v)
{
  assert(this->target != 0);
  if (this->target == targetin) {
    this->targetcounter++;
    this->addIndex(v);
  }
  else {
    this->getNext()->targetVertex(targetin, v);
  }
}

/*!
  Ends the current target.

  \sa beginTarget()
  \sa targetVertex()
*/
void
SoVertexArrayIndexer::endTarget(GLenum targetin)
{
  assert(this->target != 0);
  if (this->target == targetin) {
    this->countarray.append((GLsizei) this->targetcounter);
  }
  else {
    this->getNext()->endTarget(targetin);
  }
}

/*!
  Closes the indexer. This will reallocate the growable arrays to use as little
  memory as possible. The indexer will also sort triangles and lines to
  optimize rendering.
*/
void
SoVertexArrayIndexer::close(void)
{
  this->indexarray.fit();
  this->countarray.fit();
  this->ciarray.truncate(0);

  if (this->target != GL_TRIANGLES && this->target != GL_QUADS && this->target != GL_LINES && this->target != GL_POINTS) {
    const GLint * ptr = this->indexarray.getArrayPtr();
    for (int i = 0; i < this->countarray.getLength(); i++) {
      this->ciarray.append(ptr);
      ptr += (int) this->countarray[i];
    }
  }
  if (this->target == GL_TRIANGLES) {
    this->sort_triangles();
  }
  else if (this->target == GL_LINES) {
    this->sort_lines();
  }
  // FIXME: sort lines and points
  if (this->next) this->next->close();
}

/*!
  Render all added targets/indices.
*/
void
SoVertexArrayIndexer::render(SoState * state, const SbBool renderasvbo, const uint32_t contextid)
{
  const cc_glglue * glue = sogl_glue_instance(state);

  switch (this->target) {
  case GL_TRIANGLES:
  case GL_QUADS:
  case GL_LINES:
  case GL_POINTS:
    // common case
    if (renderasvbo) {
      if (this->vbo == NULL) {
        this->vbo = new SoVBO(GL_ELEMENT_ARRAY_BUFFER);
        if (this->use_shorts) {
          GLushort * dst = reinterpret_cast<GLushort*> 
            (this->vbo->allocBufferData(this->indexarray.getLength()*sizeof(GLushort)));
          const int32_t * src = this->indexarray.getArrayPtr();
          for (int i = 0; i < this->indexarray.getLength(); i++) {
            dst[i] = static_cast<GLushort> (src[i]);
          }
        }
        else {
          this->vbo->setBufferData(this->indexarray.getArrayPtr(),
                                   this->indexarray.getLength()*sizeof(int32_t));
        }
      }
      this->vbo->bindBuffer(contextid);
      cc_glglue_glDrawElements(glue,
                               this->target,
                               this->indexarray.getLength(),
                               this->use_shorts ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, NULL);
      cc_glglue_glBindBuffer(glue, GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    else {
      const GLint * idxptr = this->indexarray.getArrayPtr();
      cc_glglue_glDrawElements(glue,
                               this->target,
                               this->indexarray.getLength(),
                               GL_UNSIGNED_INT,
                               idxptr);
    }
    break;
  default:
    if (SoGLDriverDatabase::isSupported(glue, SO_GL_MULTIDRAW_ELEMENTS)) {
      cc_glglue_glMultiDrawElements(glue,
                                    this->target,
                                    (GLsizei*) this->countarray.getArrayPtr(),
                                    GL_UNSIGNED_INT,
                                    (const GLvoid**) this->ciarray.getArrayPtr(),
                                    this->countarray.getLength());
    }
    else {
      for (int i = 0; i < this->countarray.getLength(); i++) {
        const GLsizei * ptr = this->ciarray[i];
        GLsizei cnt = this->countarray[i];
        cc_glglue_glDrawElements(glue,
                                 this->target,
                                 cnt,
                                 GL_UNSIGNED_INT,
                                 (const GLvoid*) ptr);
      }
    }
    break;
  }

  if (this->next) this->next->render(state, renderasvbo, contextid);
}

/*!
  Returns the total number of vertex indices added to the indexer.
*/
int
SoVertexArrayIndexer::getNumVertices(void)
{
  int count = this->indexarray.getLength();
  if (this->next) count += this->next->getNumVertices();
  return count;
}

//
//  Returns the next indexer. If more than one target type is added to
//  an indexer, the indexer will automatically create a new indexer to
//  store the new target type.
//
SoVertexArrayIndexer *
SoVertexArrayIndexer::getNext(void)
{
  if (this->next == NULL) {
    this->next = new SoVertexArrayIndexer;
  }
  return this->next;
}

// sort an array of three integers
static void sort3(int32_t * arr)
{
  // simple bubble-sort
  int32_t tmp;
  if (arr[1] < arr[0]) {
    tmp = arr[0];
    arr[0] = arr[1];
    arr[1] = tmp;
  }
  if (arr[2] < arr[1]) {
    tmp = arr[1];
    arr[1] = arr[2];
    arr[2] = tmp;
  }
  if (arr[1] < arr[0]) {
    tmp = arr[0];
    arr[0] = arr[1];
    arr[1] = tmp;
  }
}

// sort an array of two integers
static void sort2(int32_t * arr)
{
  int32_t tmp;
  if (arr[1] < arr[0]) {
    tmp = arr[0];
    arr[0] = arr[1];
    arr[1] = tmp;
  }
}

// qsort callback used for sorting triangles based on vertex indices
extern "C" {
static int
compare_triangle(const void * v0, const void * v1)
{
  int i;
  int32_t * t0 = (int32_t*) v0;
  int32_t * t1 = (int32_t*) v1;

  int32_t ti0[3];
  int32_t ti1[3];
  for (i = 0; i < 3; i++) {
    ti0[i] = t0[i];
    ti1[i] = t1[i];
  }
  sort3(ti0);
  sort3(ti1);

  for (i = 0; i < 3; i++) {
    int32_t diff = ti0[i] - ti1[i];
    if (diff != 0) return diff;
  }
  return 0;
}
}

// qsort callback used for sorting lines based on vertex indices
extern "C" {
static int
compare_line(const void * v0, const void * v1)
{
  int i;
  int32_t * t0 = (int32_t*) v0;
  int32_t * t1 = (int32_t*) v1;

  int32_t ti0[2];
  int32_t ti1[2];
  for (i = 0; i < 2; i++) {
    ti0[i] = t0[i];
    ti1[i] = t1[i];
  }
  sort2(ti0);
  sort2(ti1);

  for (i = 0; i < 2; i++) {
    int32_t diff = ti0[i] - ti1[i];
    if (diff != 0) return diff;
  }
  return 0;
}
}

//
// sort triangles to optimize rendering
//
void
SoVertexArrayIndexer::sort_triangles(void)
{
  // sort triangles based on vertex indices to get more hits in the
  // GPU vertex cache. Not the optimal solution, but should work
  // pretty well. Example: bunny.iv (~70000 triangles) went from 238
  // fps with no sorting to 380 fps with sorting.
  if (this->indexarray.getLength()) {
    qsort((void*) this->indexarray.getArrayPtr(),
          this->indexarray.getLength() / 3,
          sizeof(int32_t) * 3,
          compare_triangle);
  }
}

//
// sort lines to optimize rendering
//
void
SoVertexArrayIndexer::sort_lines(void)
{
  // sort lines based on vertex indices to get more hits in the
  // GPU vertex cache.
  if (this->indexarray.getLength()) {
    qsort((void*) this->indexarray.getArrayPtr(),
          this->indexarray.getLength() / 2,
          sizeof(int32_t) * 2,
          compare_line);
  }

}

/*!
  Returns the number of indices in the indexer.
*/
int
SoVertexArrayIndexer::getNumIndices(void) const
{
  return this->indexarray.getLength();

}

/*!
  Returns a pointer to the index array.
*/
const GLint *
SoVertexArrayIndexer::getIndices(void) const
{
  return this->indexarray.getArrayPtr();
}

/*!
  Returns a pointer to the index array. It's allowed to reorganize
  these indices to change the rendering order. Calling this function
  will invalidate any VBO caches used by the indexer.
*/
GLint *
SoVertexArrayIndexer::getWriteableIndices(void)
{
  delete this->vbo;
  this->vbo = NULL;
  return (GLint*) this->indexarray.getArrayPtr();
}
