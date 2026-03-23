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
  \class SoGLRenderCache SoGLRenderCache.h Inventor/caches/SoGLRenderCache.h
  \brief The SoGLRenderCache class is used to cache OpenGL calls.

  \ingroup coin_caches
*/

// *************************************************************************

#include <cassert>

#include <Inventor/caches/SoGLRenderCache.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLDisplayList.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/C/tidbits.h> // coin_getenv()

// *************************************************************************

class SoGLRenderCacheP {
public:
  SoGLDisplayList * displaylist;
  SoState * openstate;
  SbList <SoGLDisplayList*> nestedcachelist;
  SoGLLazyElement::GLState prestate;
  SoGLLazyElement::GLState poststate;
};

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

/*!
  Constructor with \a state being the current state.
*/
SoGLRenderCache::SoGLRenderCache(SoState * state)
  : SoCache(state)
{
  PRIVATE(this) = new SoGLRenderCacheP;
  PRIVATE(this)->displaylist = NULL;
  PRIVATE(this)->openstate = NULL;
}

/*!
  Destructor.
*/
SoGLRenderCache::~SoGLRenderCache()
{
  // stuff should have been deleted in destroy()
  assert(PRIVATE(this)->displaylist == NULL);
  assert(PRIVATE(this)->nestedcachelist.getLength() == 0);
  
  delete PRIVATE(this);
}

/*!
  Opens the cache. All GL calls will be cached until close() is called.

  \sa close()
  \sa call()
*/
void
SoGLRenderCache::open(SoState * state)
{
  assert(PRIVATE(this)->displaylist == NULL);
  assert(PRIVATE(this)->openstate == NULL); // cache should not be open
  PRIVATE(this)->openstate = state;
  PRIVATE(this)->displaylist =
    new SoGLDisplayList(state, SoGLDisplayList::DISPLAY_LIST);
  PRIVATE(this)->displaylist->ref();
  PRIVATE(this)->displaylist->open(state);
}

/*!
  Closes the cache. Should be called after all GL calls.

  \sa open()
*/
void
SoGLRenderCache::close(void)
{
  assert(PRIVATE(this)->openstate != NULL);
  assert(PRIVATE(this)->displaylist != NULL);
  PRIVATE(this)->displaylist->close(PRIVATE(this)->openstate);
  PRIVATE(this)->openstate = NULL;
}

/*!
  Executes the cached display list.

  \sa open()
*/
void
SoGLRenderCache::call(SoState * state)
{
  assert(PRIVATE(this)->displaylist != NULL);

  static int COIN_NESTED_CACHING = -1;
  if (COIN_NESTED_CACHING < 0) {
    const char * env = coin_getenv("COIN_NESTED_CACHING");
    if (env) COIN_NESTED_CACHING = atoi(env);
    else COIN_NESTED_CACHING = 0;
  }
  
  if (COIN_NESTED_CACHING) {
    if (state->isCacheOpen()) {
      SoCacheElement::addCacheDependency(state, this);  
      
      PRIVATE(this)->displaylist->call(state);
      SoGLLazyElement::mergeCacheInfo(state, 
                                      &PRIVATE(this)->prestate,
                                      &PRIVATE(this)->poststate);
      
      SoGLRenderCache* parentCache = static_cast<SoGLRenderCache *>(
        SoCacheElement::getCurrentCache(state)
       );
      parentCache->addNestedCache(PRIVATE(this)->displaylist);
    }
    else {
      PRIVATE(this)->displaylist->call(state);
    }
  }
  else { // no nested caching
    SoCacheElement::invalidate(state); // destroy any parent caches
    PRIVATE(this)->displaylist->call(state);
  }
}

/*!
  Returns the cache context of this cache. This is used to quickly
  determine if cache can be used for a state.

  This method is an extension versus the Open Inventor API.
*/
int
SoGLRenderCache::getCacheContext(void) const
{
  if (PRIVATE(this)->displaylist) return PRIVATE(this)->displaylist->getContext();
  return -1;
}

// Documented in superclass. Overridden to test and update lazy GL
// elements.
SbBool
SoGLRenderCache::isValid(const SoState * state) const
{
  // pre and post cache state is handled in SoGLCacheList
  return inherited::isValid(state);
}

/*!
  Adds a display list which is called from this cache. This is to
  make sure the child display list isn't destroyed too early.
*/
void
SoGLRenderCache::addNestedCache(SoGLDisplayList * child)
{
  child->ref();
  PRIVATE(this)->nestedcachelist.append(child);
}

// Documented in superclass. Overridden to unref display lists.
void
SoGLRenderCache::destroy(SoState * state)
{
  int n = PRIVATE(this)->nestedcachelist.getLength();
  for (int i = 0; i < n; i++) {
    PRIVATE(this)->nestedcachelist[i]->unref(state);
  }
  PRIVATE(this)->nestedcachelist.truncate(0);
  if (PRIVATE(this)->displaylist) {
    PRIVATE(this)->displaylist->unref(state);
    PRIVATE(this)->displaylist = NULL;
  }
}

SoGLLazyElement::GLState * 
SoGLRenderCache::getPreLazyState(void)
{
  return &PRIVATE(this)->prestate;
}

SoGLLazyElement::GLState * 
SoGLRenderCache::getPostLazyState(void)
{
  return &PRIVATE(this)->poststate;
}



#undef PRIVATE
