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
  \class SoGlyphCache SoGlyphCache.h Inventor/caches/SoGlyphCache.h
  The SoGlyphClass is used to cache glyphs.

  \internal
*/

#include "caches/SoGlyphCache.h"

#include <cassert>

#include <Inventor/lists/SbList.h>
#include <Inventor/elements/SoFontNameElement.h>
#include <Inventor/elements/SoFontSizeElement.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/errors/SoDebugError.h>

#include "tidbitsp.h"

class SoGlyphCacheP {
public:
  SbList <cc_glyph2d*> glyphlist2d;
  SbList <cc_glyph3d*> glyphlist3d;
  cc_font_specification * fontspec;
};

#define PRIVATE(obj) ((obj)->pimpl)

SoGlyphCache::SoGlyphCache(SoState * state)
  : SoCache(state)
{
  PRIVATE(this) = new SoGlyphCacheP;
  PRIVATE(this)->fontspec = NULL;

#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoGlyphCache::SoGlyphCache",
                           "Cache constructed: %p", this);

  }
#endif // debug
}

SoGlyphCache::~SoGlyphCache()
{
#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoGlyphCache::~SoGlyphCache",
                           "Cache destructed: %p", this);

  }
#endif // debug

  int i;
  this->readFontspec(NULL);
  for (i = 0; i < PRIVATE(this)->glyphlist2d.getLength(); i++) {
    cc_glyph2d_unref(PRIVATE(this)->glyphlist2d[i]);
  }
  for (i = 0; i < PRIVATE(this)->glyphlist3d.getLength(); i++) {
    cc_glyph3d_unref(PRIVATE(this)->glyphlist3d[i]);
  }
  delete PRIVATE(this);
}

/*
  Add a glyph that is created using cc_glyph2d_ref(). The cache will
  call cc_glyph2d_unref() when destructed.
*/
void
SoGlyphCache::addGlyph(cc_glyph2d * glyph)
{
  PRIVATE(this)->glyphlist2d.append(glyph);
}

/*
  Add a glyph that is created using cc_glyph2d_ref(). The cache will
  call cc_glyph2d_unref() when destructed.
*/

void
SoGlyphCache::addGlyph(cc_glyph3d * glyph)
{
  PRIVATE(this)->glyphlist3d.append(glyph);
}

/*!
  Read and store current font specification. Will create cache dependencies
  since some elements are read. We can't read the font specification in the
  constructor since we need to update SoCacheElement before reading
  the elements.
*/
void
SoGlyphCache::readFontspec(SoState * state)
{
  if (PRIVATE(this)->fontspec) {
    cc_fontspec_clean(PRIVATE(this)->fontspec);
    delete PRIVATE(this)->fontspec;
    PRIVATE(this)->fontspec = NULL;
  }
  if (state) {
    PRIVATE(this)->fontspec = new cc_font_specification;
    cc_fontspec_construct(PRIVATE(this)->fontspec,
                          SoFontNameElement::get(state).getString(),
                          SoFontSizeElement::get(state),
                          SoComplexityElement::get(state));
  }
}

/*!
  Returns the cached font specification.
*/
const cc_font_specification *
SoGlyphCache::getCachedFontspec(void) const
{
  assert(PRIVATE(this)->fontspec);
  return PRIVATE(this)->fontspec;
}


#undef PRIVATE
