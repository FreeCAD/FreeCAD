#ifndef COIN_SOCONVEXDATACACHE_H
#define COIN_SOCONVEXDATACACHE_H

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
#include <Inventor/system/inttypes.h>

class SbVec3f;
class SoCoordinateElement;
class SbMatrix;
class SoConvexDataCacheP;

class COIN_DLL_API SoConvexDataCache : public SoCache {
  typedef SoCache inherited;
public:
  SoConvexDataCache(SoState * const state);
  virtual ~SoConvexDataCache();

  enum Binding {
    // do not change these values. We rely on them matching
    // values in SoIndededFaceSet.h and SoGL.cpp...
    NONE = 0,
    PER_FACE,
    PER_FACE_INDEXED,
    PER_VERTEX,
    PER_VERTEX_INDEXED
  };

  void generate(const SoCoordinateElement * const coords,
                const SbMatrix & matrix,
                const int32_t *coordindices,
                const int numcoordindices,
                const int32_t *matindices, const int32_t *normindices,
                const int32_t *texindices,
                const Binding matbinding, const Binding normbinding,
                const Binding texbinding);

  const int32_t *getCoordIndices(void) const;
  int getNumCoordIndices(void) const;
  const int32_t *getMaterialIndices(void) const;
  int getNumMaterialIndices(void) const;
  const int32_t *getNormalIndices(void) const;
  int getNumNormalIndices(void) const;
  const int32_t *getTexIndices(void) const;
  int getNumTexIndices(void) const;

private:
  SoConvexDataCacheP * pimpl;
};

#endif // !COIN_SOCONVEXDATACACHE_H
