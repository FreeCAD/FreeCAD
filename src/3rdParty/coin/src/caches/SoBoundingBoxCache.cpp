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
  \class SoBoundingBoxCache SoBoundingBoxCache.h Inventor/caches/SoBoundingBoxCache.h
  \brief The SoBoundingBoxCache class is used to cache bounding boxes.

  \ingroup coin_caches
*/

// *************************************************************************

#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/errors/SoDebugError.h>

#include "tidbitsp.h"

// *************************************************************************

class SoBoundingBoxCacheP {
public:
  SbXfBox3f bbox;
  SbBox3f localbbox;
  SbVec3f centerpoint;
  unsigned int centerset : 1;
  unsigned int linesorpoints : 1;
};

#define PRIVATE(p) ((p)->pimpl)

// *************************************************************************

/*!
  Constructor with \a state being the current state.
*/
SoBoundingBoxCache::SoBoundingBoxCache(SoState *state)
  : SoCache(state)
{
  PRIVATE(this) = new SoBoundingBoxCacheP;
  PRIVATE(this)->centerset = 0;
  PRIVATE(this)->linesorpoints = 0;

#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoBoundingBoxCache::SoBoundingBoxCache",
                           "Cache created: %p", this);
    
  }
#endif // debug
}

/*!
  Destructor.
*/
SoBoundingBoxCache::~SoBoundingBoxCache()
{
#if COIN_DEBUG
  if (coin_debug_caching_level() > 0) {
    SoDebugError::postInfo("SoBoundingBoxCache::~SoBoundingBoxCache",
                           "Cache destructed: %p", this);
  }
#endif // debug

  delete PRIVATE(this);
}

/*!
  Sets the data for this cache. \a boundingBox is the node's bounding
  box, \a centerSet and \a centerPoints specifies the center of the
  geometry inside \a boundingBox.
*/
void
SoBoundingBoxCache::set(const SbXfBox3f & boundingbox,
                        SbBool centerset,
                        const SbVec3f & centerpoint)
{
  PRIVATE(this)->bbox = boundingbox;
  PRIVATE(this)->localbbox = boundingbox.project();
  PRIVATE(this)->centerset = centerset ? 1 : 0;
  if (centerset) { PRIVATE(this)->centerpoint = centerpoint; }
}

/*!
  Returns the bounding box for this cache.
*/
const SbXfBox3f &
SoBoundingBoxCache::getBox(void) const
{
  return PRIVATE(this)->bbox;
}

/*!
  Returns the projected bounding box for this cache.
*/
const SbBox3f &
SoBoundingBoxCache::getProjectedBox(void) const
{
  return PRIVATE(this)->localbbox;
}

/*!
  Returns whether the center of the bounding box was set in the
  SoBoundingBoxCache::set() method.

  \sa SoBoundingBoxCache::getCenter()
*/
SbBool
SoBoundingBoxCache::isCenterSet(void) const
{
  return PRIVATE(this)->centerset == 1;
}

/*!
  Returns the center of the bounding box. Should only be used if
  SoBoundingBoxCache::isCenterSet() returns \c TRUE.
*/
const SbVec3f &
SoBoundingBoxCache::getCenter(void) const
{
  return PRIVATE(this)->centerpoint;
}

/*!
  Sets the flag returned from SoBoundingBoxCache::hasLinesOrPoints()
  to \c TRUE for all open bounding box caches.

  The reason bounding box caches keep a lines-or-points flag is to
  make it known to client code if the shape(s) they contain have any
  of these primitives -- or are rendered with these primitives. The
  reason this is important to know for the client code is because it
  might need to add an "epsilon" slack value to the calculated
  bounding box to account for smoothing / anti-aliasing effects in the
  renderer, so line and point graphics are not accidentally clipped by
  near and far clipping planes, for instance.

  This method is a static method on the class. It will upon invocation
  scan through the state stack and set the flag for all open
  SoBoundingBoxCache elements. It has been made to work like this so
  it can easily be invoked on all current bounding box cache instances
  from the SoShape-type nodes using lines and / or point primitives.

  \sa hasLinesOrPoints()
*/
void
SoBoundingBoxCache::setHasLinesOrPoints(SoState * state)
{
  SoCacheElement * elem = static_cast<SoCacheElement *>(
    state->getElementNoPush(SoCacheElement::getClassStackIndex())
    );

  while (elem) {
    SoBoundingBoxCache * cache = static_cast<SoBoundingBoxCache *>(elem->getCache());
    if (cache) { PRIVATE(cache)->linesorpoints = TRUE; }
    elem = elem->getNextCacheElement();
  }
}

/*!
  Return \c TRUE if the hasLinesOrPoints flag has been set.

  \sa setHasLinesOrPoints()
*/
SbBool
SoBoundingBoxCache::hasLinesOrPoints(void) const
{
  return PRIVATE(this)->linesorpoints == 1;
}

#undef PRIVATE
