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
  \page caches The Cache Classes

  The cache classes are mostly internal to Coin.

  \ingroup coin_caches
*/

/*!
  \class SoCache SoCache.h Inventor/caches/SoCache.h
  \brief The SoCache class is the superclass for all internal cache classes.

  \ingroup coin_caches

  It organizes reference counting to make it possible to share cache
  instances. It also organizes a list of elements that will affect the
  cache. If any of the elements have changed since the cache was
  created, the cache is invalid.

  The cache element test algorithm in Coin works like this:

  Every element that is read before it is written when a cache is
  created is stored in the SoCache's element list. This is done to
  detect when something outside the cache changes.

  Example: you have a SoCoordinate3 node outside an SoSeparator, but
  an SoIndexedFaceSet inside the SoSeparator. If the SoSeparator
  creates a cache, SoIndexedFaceSet will read SoCoordinateElement, and
  since SoCoordinateElement hasn't been set after the cache was
  opened, the cache stores that element in the cache's list of element
  dependencies.

  At the next frame, the SoSeparator will test if the cache is valid,
  and will then test all dependency elements. If one of the elements
  doesn't match, the cache is not valid and can't be used.

  That's the basics. There are some steps you have to do when creating
  a cache to make the cache dependencies work. Basically you have to
  do it like this:
  
  \code
  SbBool storedinvalid = SoCacheElement::setInvalid(FALSE);
  state->push();
  SoMyCache * cache = new SoMyCache(state);
  cache->ref();
  SoCacheElement::set(state, cache);
  buildMyCache();
  state->pop();
  SoCacheElement::setInvalid(storedinvalid);
  \endcode
  
  First you reset and store the old value of the cache
  invalid-flag. Then you push the state so that the cache can detect
  when something outside the cache is changed (and to be able to
  change the cache element).  Next, you create the cache - don't
  forget to ref it. Finally, set the current cache in the cache
  element and build the cache. After building the cache, you pop the
  state and restore the invalid-cache flag.
  
  When building the cache, all elements that are read will be copied
  into the cache (using SoElement::copyMatchInfo()), and these
  copied elements are used to test the validity of the cache
  (in SoCache::isValid()).
  
  You don't have to manually add element dependencies. They will
  automatically be picked up when creating the cache. This is
  handled in SoElement::getConstElement().
  
  If you want the cache to be invalidated when some field inside your
  node is changed, it's common to overload the notify()-method, and
  call SoCache::invalidate() whenever the notify()-method for your
  node is called. See for instance SoShape::notify().

  Also, don't delete the cache in your notify() method. Wait until the
  next time the cache is needed before unref-ing the old cache.
*/


// FIXME: this really needs a usage example, preferably with source
// code for when using an extension cache. 20040722 mortene.

// *************************************************************************

#include <Inventor/caches/SoCache.h>

#include <cstring>
#include <cassert>

#include <Inventor/SbName.h>
#include <Inventor/elements/SoElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/C/tidbits.h>

#include "tidbitsp.h"
#include "coindefs.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::memset;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

// *************************************************************************

class SoCacheP {
public:
  SbList <SoElement *> elements;
  unsigned char * elementflags;
  int refcount;
  SbBool invalidated;
  int statedepth;
};

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

/*!
  Constructor with \a state being the current state.
*/
SoCache::SoCache(SoState * const state)
{
  PRIVATE(this) = new SoCacheP;
  PRIVATE(this)->elementflags = NULL;
  PRIVATE(this)->refcount = 0;
  PRIVATE(this)->invalidated = FALSE;
  PRIVATE(this)->statedepth = state ? state->getDepth() : 0;

  int numidx = SoElement::getNumStackIndices();
  int numbytes = (numidx >> 3) + 1;
  // one bit per element is used to quickly determine whether an
  // element of a given type already has been added.
  PRIVATE(this)->elementflags = new unsigned char[numbytes];
  memset(PRIVATE(this)->elementflags, 0, numbytes);
}

/*!
  Destructor
*/
SoCache::~SoCache()
{
  delete [] PRIVATE(this)->elementflags;

  int n = PRIVATE(this)->elements.getLength();
  for (int i = 0; i < n; i++) {
    delete PRIVATE(this)->elements[i];
  }
  delete PRIVATE(this);
}

// *************************************************************************

/*!
  Increases the reference count by one.
*/
void
SoCache::ref(void)
{
  PRIVATE(this)->refcount++;
}

/*!
  Decreases the reference count by one. When the reference count reaches
  zero, the cache is deleted. The SoCache::destroy() method is called
  before the destructor is called.
*/
void
SoCache::unref(SoState *state)
{
  assert(PRIVATE(this)->refcount > 0);
  if (--PRIVATE(this)->refcount == 0) {
    this->destroy(state);
    delete this;
  }
}

// *************************************************************************

/*!
  Adds \a elem to the list of elements this cache depends on.
*/
void
SoCache::addElement(const SoElement * const elem)
{
  if (elem->getDepth() < PRIVATE(this)->statedepth) {
    int idx = elem->getStackIndex();
    int flag = 0x1 << (idx & 0x7);
    idx >>= 3; // get byte number
    if (!(PRIVATE(this)->elementflags[idx] & flag)) {
#if COIN_DEBUG // debug
      if (coin_debug_caching_level() > 1) {
        SoDebugError::postInfo("SoCache::addElement",
                               "cache: %p, elem: %s", this,
                               elem->getTypeId().getName().getString());
      }
#endif // debug
      SoElement * copy = elem->copyMatchInfo();
      if (copy) PRIVATE(this)->elements.append(copy);
      PRIVATE(this)->elementflags[idx] |= flag;
    }
  }
}

/*!
  Adds dependencies from \a cache to this cache.
*/
void
SoCache::addCacheDependency(const SoState * state, SoCache * cache)
{
  if (cache == this) return;

  // local variables for speed
  int n = cache->pimpl->elements.getLength();
  const SoElement * const * ptr = cache->pimpl->elements.getArrayPtr();
  for (int i = 0; i < n; i++) {
    // use elements in state to get correct element depth
    this->addElement(state->getConstElement(ptr[i]->getStackIndex()));
  }
}

/*!
  Return \e TRUE if this cache is valid, \e FALSE otherwise.
*/
SbBool
SoCache::isValid(const SoState * state) const
{
  if (PRIVATE(this)->invalidated) return FALSE;
  return this->getInvalidElement(state) == NULL;
}

/*!
  Returns the element that caused the invalidation. Returns \e NULL
  if the cache is valid, or if the cache was not invalidated
  because of an element.
*/
const SoElement *
SoCache::getInvalidElement(const SoState * const state) const
{
  if (PRIVATE(this)->invalidated) return NULL;

  // use local variables for speed
  int n = PRIVATE(this)->elements.getLength();
  const SoElement * const * ptr = PRIVATE(this)->elements.getArrayPtr();
  const SoElement * elem;
  for (int i = 0; i < n; i++) {
    elem = ptr[i];
    if (!elem->matches(state->getConstElement(elem->getStackIndex()))) {
#if COIN_DEBUG
      if (coin_debug_caching_level() > 0) {
        SoDebugError::postInfo("SoCache::getInvalidElement",
                               "cache: %p, invalid element: %s", this,
                               elem->getTypeId().getName().getString());
      }
#endif // debug
      return elem;
    }
  }
  return NULL;
}

/*!
  Forces a cache to be invalid.
*/
void
SoCache::invalidate(void)
{
  PRIVATE(this)->invalidated = TRUE;
}

/*!
  Can be overridden by subclasses to clean up before they are
  deleted. Default method does nothing.
*/
void
SoCache::destroy(SoState *)
{
}

#undef PRIVATE

// *************************************************************************
