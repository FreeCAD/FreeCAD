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
  \class SoShaderProgramCache SoShaderProgramCache.h Inventor/caches/SoShaderProgramCache.h
  \brief The SoShaderProgramCache class is used to cache shader programs.

  \ingroup coin_caches
*/

// *************************************************************************

#include "caches/SoShaderProgramCache.h"
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SbString.h>

#include "tidbitsp.h"

// *************************************************************************

class SoShaderProgramCacheP {
public:
  SbString program;
};

#define PRIVATE(p) ((p)->pimpl)

// *************************************************************************

/*!
  Constructor with \a state being the current state.
*/
SoShaderProgramCache::SoShaderProgramCache(SoState *state)
  : SoCache(state)
{
  PRIVATE(this) = new SoShaderProgramCacheP;

#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoShaderProgramCache::SoShaderProgramCache",
                           "Cache created: %p", this);

  }
#endif // debug
}

/*!
  Destructor.
*/
SoShaderProgramCache::~SoShaderProgramCache()
{
#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoShaderProgramCache::~SoShaderProgramCache",
                           "Cache destructed: %p", this);
  }
#endif // debug

  delete PRIVATE(this);
}

/*!
  Sets the shader program for this cache.
*/
void
SoShaderProgramCache::set(const SbString & program)
{
  PRIVATE(this)->program = program;
}

/*!
  Returns the shader program for this cache.
*/
const SbString &
SoShaderProgramCache::get(void) const
{
  return PRIVATE(this)->program;
}

#undef PRIVATE
