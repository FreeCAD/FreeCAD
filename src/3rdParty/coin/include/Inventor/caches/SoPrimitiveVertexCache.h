#ifndef COIN_SOPRIMITIVEVERTEXCACHE_H
#define COIN_SOPRIMITIVEVERTEXCACHE_H

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

#include <Inventor/caches/SoCache.h>
#include <Inventor/system/gl.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/tools/SbPimplPtr.h>

class SoPrimitiveVertexCacheP;
class SoPrimitiveVertex;
class SoPointDetail;
class SoState;

class COIN_DLL_API SoPrimitiveVertexCache : public SoCache {
  typedef SoCache inherited;
public:
  SoPrimitiveVertexCache(SoState * state);
  virtual ~SoPrimitiveVertexCache();

  enum Arrays {
    NORMAL = 0x01,
    TEXCOORD = 0x02,
    COLOR = 0x04,
    ALL = (NORMAL|TEXCOORD|COLOR)
  };

  virtual SbBool isValid(const SoState * state) const;
  void close(SoState * state);

  void renderTriangles(SoState * state, const int arrays = ALL) const;
  void renderLines(SoState * state, const int arrays = ALL) const;
  void renderPoints(SoState * state, const int array = ALL) const;

  void addTriangle(const SoPrimitiveVertex * v0,
                   const SoPrimitiveVertex * v1,
                   const SoPrimitiveVertex * v2,
                   const int * pointdetailidx = NULL);
  void addLine(const SoPrimitiveVertex * v0,
               const SoPrimitiveVertex * v1);
  void addPoint(const SoPrimitiveVertex * v);

  int getNumVertices(void) const;
  const SbVec3f * getVertexArray(void) const;
  const SbVec3f * getNormalArray(void) const;
  const SbVec4f * getTexCoordArray(void) const;
  const SbVec2f * getBumpCoordArray(void) const;
  const uint8_t * getColorArray(void) const;

  int getNumTriangleIndices(void) const;
  const GLint * getTriangleIndices(void) const;
  int32_t getTriangleIndex(const int idx) const;

  SbBool colorPerVertex(void) const;
  const SbVec4f * getMultiTextureCoordinateArray(const int unit) const;

  int getNumLineIndices(void) const;
  const GLint * getLineIndices(void) const;

  int getNumPointIndices(void) const;
  const GLint * getPointIndices(void) const;

  void fit(void);
  void depthSortTriangles(SoState * state);

private:
  SbPimplPtr<SoPrimitiveVertexCacheP> pimpl;

  SoPrimitiveVertexCache(const SoPrimitiveVertexCache & rhs); // N/A
  SoPrimitiveVertexCache & operator = (const SoPrimitiveVertexCache & rhs); // N/A

};

#endif // COIN_SOPRIMITIVEVERTEXCACHE_H
