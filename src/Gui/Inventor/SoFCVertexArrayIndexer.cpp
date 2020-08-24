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

#include "PreCompiled.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#include <Inventor/misc/SoGLDriverDatabase.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/SbName.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbMatrix.h>

#include "SoFCVBO.h"
#include "SoFCVertexArrayIndexer.h"

SoFCVertexArrayIndexer::SoFCVertexArrayIndexer(SbFCUniqueId _dataid,
                                               IndexArray *prev)
  : dataid(_dataid)
  , target(0)
  , lastlineindex(0)
  , use_shorts(TRUE)
{
  if (prev && prev->isAttached())
    this->previndexarray = prev;
}

SoFCVertexArrayIndexer::SoFCVertexArrayIndexer(const SoFCVertexArrayIndexer & other,
                                               const std::set<int> & partindices,
                                               int maxindex)
  : dataid(other.dataid)
  , target(other.target)
  , use_shorts(other.use_shorts)
{
  assert(other.indexarray && other.indexarray->isAttached());
  this->indexarray = other.indexarray;
  this->partarray = other.partarray;
  this->partialindices.reserve(partindices.size());
  if (this->partarray.size())
    maxindex = static_cast<int>(this->partarray.size());
  for (int i : partindices) {
    if (i >=0 && i < maxindex)
      this->partialindices.push_back(i);
  }
}

SoFCVertexArrayIndexer::~SoFCVertexArrayIndexer()
{
}

const int *
SoFCVertexArrayIndexer::getPartOffsets(void) const
{
  return &this->partarray[0];
}

int
SoFCVertexArrayIndexer::getNumParts(void) const
{
  return static_cast<int>(this->partarray.size());
}

inline void 
SoFCVertexArrayIndexer::addIndex(int32_t i) 
{
  if (i >= 65536) this->use_shorts = FALSE;
  if (!this->indexarray) {
    this->indexarray =
      new IndexArray(dataid + 0x9a8d5763, this->previndexarray, GL_ELEMENT_ARRAY_BUFFER);
    this->previndexarray.reset();
  }
  this->indexarray->append(static_cast<GLint> (i));
}

void
SoFCVertexArrayIndexer::addLine(const int32_t v0, const int32_t v1, int lineindex)
{
  if (this->target == 0) this->target = GL_LINES;
  if (this->target == GL_LINES) {
    for (;lineindex > this->lastlineindex; ++this->lastlineindex)
      this->partarray.push_back(this->indexarray->getLength());
    this->addIndex(v0);
    this->addIndex(v1);
  }
}

void
SoFCVertexArrayIndexer::addPoint(const int32_t v0)
{
  if (this->target == 0) this->target = GL_POINTS;
  if (this->target == GL_POINTS) {
    this->addIndex(v0);
  }
}

void
SoFCVertexArrayIndexer::addTriangle(const int32_t v0,
                                    const int32_t v1,
                                    const int32_t v2)
{
  if (this->target == 0) this->target = GL_TRIANGLES;
  if (this->target == GL_TRIANGLES) {
    this->addIndex(v0);
    this->addIndex(v1);
    this->addIndex(v2);
  }
}

void
SoFCVertexArrayIndexer::initClass()
{
}

void
SoFCVertexArrayIndexer::cleanup()
{
}

void
SoFCVertexArrayIndexer::close(const int *parts, int count)
{
  if (!this->indexarray) return;

  if (count) {
    int step;
    switch(this->target) {
    case GL_TRIANGLES:
      step = 3;
      break;
    case GL_LINES:
      step = 2;
      break;
    default:
      step = 1;
    }
    this->partarray.reserve(count);
    int partindex = 0;
    for (int i=0; i<count; ++i) {
      partindex += parts[i]*step;
      if (partindex > this->indexarray->getLength())
        break;
      this->partarray.push_back(partindex);
    }
  }
  else if (this->target == GL_LINES
          && this->partarray.size()
          && this->partarray.back() < this->indexarray->getLength())
  {
    this->partarray.push_back(this->indexarray->getLength());
  }

  if (this->target == GL_TRIANGLES)
    this->sort_triangles();
  else if (this->target == GL_LINES)
    this->sort_lines();
}

void
SoFCVertexArrayIndexer::close(std::vector<int> && parts)
{
  if (!this->indexarray) return;
  this->partarray = std::move(parts);
}

void
SoFCVertexArrayIndexer::render(SoState * state,
                               const cc_glglue * glue,
                               const SbBool renderasvbo,
                               const uint32_t contextid,
                               const intptr_t * offsets,
                               const int32_t * counts,
                               int32_t drawcount)
{
  if (!this->indexarray) return;
  if (renderasvbo) {
    if (!this->indexarray->isAttached()) {
      if (this->use_shorts)
        this->indexarray = this->indexarray->attachAndConvert<uint16_t>();
      else
        this->indexarray = this->indexarray->attach();
    }
    this->indexarray->bindBuffer(state, contextid);
  }

  if (!drawcount && !this->partialindices.empty()) {
    if (this->partialoffsets.empty()) {
      this->partialoffsets.reserve(this->partialindices.size());
      this->partialcounts.reserve(this->partialindices.size());
      int typesize = this->use_shorts ? 2 : 4;
      for (int i : this->partialindices) {
        int prev = i ? this->partarray[i-1] : 0;
        this->partialoffsets.push_back(prev * typesize);
        this->partialcounts.push_back(this->partarray[i] - prev);
      }
    }
    drawcount = static_cast<int>(this->partialindices.size());
    offsets = &this->partialoffsets[0];
    counts = &this->partialcounts[0];
  }

  if (!drawcount) {
    if (renderasvbo)
      cc_glglue_glDrawElements(glue,
                              this->target,
                              this->indexarray->getLength(),
                              this->use_shorts ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                              NULL);
    else {
      const GLint * idxptr = this->indexarray->getArrayPtr();
      cc_glglue_glDrawElements(glue,
                               this->target,
                               this->indexarray->getLength(),
                               GL_UNSIGNED_INT,
                               idxptr);
    }
  }
  else if (cc_glglue_has_multidraw_vertex_arrays(glue)) {
    cc_glglue_glMultiDrawElements(glue,
                                  this->target,
                                  (GLsizei*) counts,
                                  renderasvbo && this->use_shorts ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                  (const GLvoid **) offsets,
                                  drawcount);
  }
  else {
    for (int i=0; i<drawcount; ++i)
      cc_glglue_glDrawElements(glue,
                               this->target,
                               counts[i],
                               renderasvbo && this->use_shorts ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                               (const GLvoid *) offsets[i]);
  }

  if (renderasvbo)
    cc_glglue_glBindBuffer(glue, GL_ELEMENT_ARRAY_BUFFER, 0);
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
SoFCVertexArrayIndexer::sort_triangles(void)
{
  if (!this->indexarray) return;
  // sort triangles based on vertex indices to get more hits in the
  // GPU vertex cache. Not the optimal solution, but should work
  // pretty well. Example: bunny.iv (~70000 triangles) went from 238
  // fps with no sorting to 380 fps with sorting.
  if (this->partarray.size() <= 1) {
    if (this->indexarray->getLength() > 10 * 3)
      qsort((void*) this->indexarray->getArrayPtr(),
            this->indexarray->getLength() / 3,
            sizeof(int32_t) * 3,
            compare_triangle);
  }
  else {
    GLint *indices = (GLint*) this->indexarray->getArrayPtr();
    int prev = 0;
    for (GLint idx : this->partarray) {
      if (idx - prev > 10*3)
        qsort(indices + prev,
              (idx - prev) / 3,
              sizeof(int32_t) * 3,
              compare_triangle);
      prev = idx;
    }
  }
}

//
// sort lines to optimize rendering
//
void
SoFCVertexArrayIndexer::sort_lines(void)
{
  if (!this->indexarray->getLength()) return;
  // sort lines based on vertex indices to get more hits in the
  // GPU vertex cache.
  if (this->partarray.size() <= 1) {
    if (this->indexarray->getLength() > 10 * 2)
      qsort((void*) this->indexarray->getArrayPtr(),
            this->indexarray->getLength() / 2,
            sizeof(int32_t) * 2,
            compare_line);
  }
  else {
    GLint *indices = (GLint*) this->indexarray->getArrayPtr();
    int prev = 0;
    for (GLint idx : this->partarray) {
      if (idx - prev > 10*2)
        qsort(indices + prev,
              (idx - prev) / 2,
              sizeof(int32_t) * 2,
              compare_line);
      prev = idx;
    }
  }
}

/*!
  Returns the number of indices in the indexer.
*/
int
SoFCVertexArrayIndexer::getNumIndices(void) const
{
  return this->indexarray ? this->indexarray->getLength() : 0;

}

const GLint *
SoFCVertexArrayIndexer::getIndices(void) const
{
  return this->indexarray ? this->indexarray->getArrayPtr() : NULL;
}

/*!
  Returns a pointer to the index array. It's allowed to reorganize
  these indices to change the rendering order. Calling this function
  will invalidate any VBO caches used by the indexer.
*/
GLint *
SoFCVertexArrayIndexer::getWriteableIndices(void)
{
  if (!this->indexarray) return NULL;
  if (this->indexarray->isAttached()) {
    this->indexarray = new IndexArray(
        dataid, this->indexarray, GL_ELEMENT_ARRAY_BUFFER, GL_STREAM_DRAW);
  }
  return this->indexarray->getWritableArrayPtr();
}

void
SoFCVertexArrayIndexer::getBoundingBox(const SbMatrix * matrix,
                                       SbBox3f &bbox,
                                       const SbVec3f * vertices)
{
  if (!this->indexarray)
    return;

  const GLint * indices = this->indexarray->getArrayPtr();
  if (this->partialindices.empty()) {
    if (matrix) {
      for (int i=0, n=this->indexarray->getLength(); i<n; ++i) {
        SbVec3f v;
        matrix->multVecMatrix(vertices[indices[i]], v);
        bbox.extendBy(v);
      }
    }
    else {
      for (int i=0, n=this->indexarray->getLength(); i<n; ++i)
        bbox.extendBy(vertices[indices[i]]);
    }
    return;
  }

  if (matrix) {
    for (int i : this->partialindices) {
      int j = i ? this->partarray[i-1] : 0;
      for (int end = this->partarray[i]; j < end; ++j) {
        SbVec3f v;
        matrix->multVecMatrix(vertices[indices[j]], v);
        bbox.extendBy(v);
      }
    }
  }
  else {
    for (int i : this->partialindices) {
      int j = i ? this->partarray[i-1] : 0;
      for (int end = this->partarray[i]; j < end; ++j)
        bbox.extendBy(vertices[indices[j]]);
    }
  }
}

// vim: noai:ts=2:sw=2
