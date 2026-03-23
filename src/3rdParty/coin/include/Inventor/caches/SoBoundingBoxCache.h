#ifndef COIN_SOBOUNDINGBOXCACHE_H
#define COIN_SOBOUNDINGBOXCACHE_H

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
#include <Inventor/SbXfBox3f.h>

class SoBoundingBoxCacheP;

class COIN_DLL_API SoBoundingBoxCache : public SoCache {
  typedef SoCache inherited;
public:
  SoBoundingBoxCache(SoState *state);
  virtual ~SoBoundingBoxCache();

  void set(const SbXfBox3f & boundingbox,
           SbBool centerset,
           const SbVec3f & centerpoint);

  const SbXfBox3f & getBox() const;
  const SbBox3f & getProjectedBox() const;

  SbBool isCenterSet() const;
  const SbVec3f & getCenter() const;

  static void setHasLinesOrPoints(SoState *state);
  SbBool hasLinesOrPoints(void) const;

private:
  SoBoundingBoxCacheP * pimpl;
};

#endif // !COIN_SOBOUNDINGBOXCACHE_H
