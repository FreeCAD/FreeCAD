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
  \class SoVertexShape SoVertexShape.h Inventor/nodes/SoVertexShape.h
  \brief The SoVertexShape class is the superclass for all vertex based shapes.

  \ingroup coin_nodes

  Basically, every polygon, line, or point based shape will inherit
  this class.  It contains methods for organizing the normal cache,
  and also holds the SoVertexShape::vertexProperty field which can be
  used to set vertex data inside the node.
*/

// *************************************************************************

#include <Inventor/nodes/SoVertexShape.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoCreaseAngleElement.h>
#include <Inventor/elements/SoGLShapeHintsElement.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/threads/SbRWMutex.h>

#include "nodes/SoSubNodeP.h"
#include "tidbitsp.h"

// *************************************************************************

/*!
  \var SoSFNode SoVertexShape::vertexProperty

  If you set the vertexProperty field, it should be with an
  SoVertexProperty node. Otherwise it will simply be
  ignored. Node types inheriting SoVertexShape will then get their
  coordinate data from the vertexProperty node instead of from the
  global traversal state.

  The vertexProperty field of SoVertexShape-derived nodes breaks
  somewhat with the basic design of Open Inventor, as its contents are
  not passed to the global state. This is done to provide a simple
  path to highly optimized rendering of vertex based shapes.

  \sa SoVertexProperty

  \since Coin 1.0
  \since SGI Inventor 2.1
*/

// *************************************************************************

class SoVertexShapeP {
public:
  SoNormalCache * normalcache;

  // we can use a per-instance mutex here instead of this class-wide
  // one, but we go for the class-wide one since at least Microsoft Windows
  // might have a rather strict limit on the total amount of mutex
  // resources a process / user can hold at any one time.
  //
  // i haven't looked too hard at the affected code regions in the
  // sub-classes, however. it might be that a class-wide lock can
  // cause significantly less efficient execution in a multi-threaded
  // environment. if so, we will have to come up with something better
  // than just a class-wide lock (a mutex pool or something, i
  // suppose).
  //
  // -mortene.
  static SbRWMutex * normalcachemutex;

  static void cleanup(void);
};

// called by atexit
void
SoVertexShapeP::cleanup(void)
{
  delete SoVertexShapeP::normalcachemutex;
  SoVertexShapeP::normalcachemutex = NULL;
}

SbRWMutex * SoVertexShapeP::normalcachemutex = NULL;

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

SO_NODE_ABSTRACT_SOURCE(SoVertexShape);

// *************************************************************************


/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVertexShape::initClass(void)
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoVertexShape, SO_FROM_INVENTOR_1);

#ifdef COIN_THREADSAFE
  SoVertexShapeP::normalcachemutex = new SbRWMutex(SbRWMutex::READ_PRECEDENCE);
#endif // COIN_THREADSAFE

  coin_atexit((coin_atexit_f *)SoVertexShapeP::cleanup, CC_ATEXIT_NORMAL);
}

/*!
  Constructor.
*/
SoVertexShape::SoVertexShape(void)
{
  PRIVATE(this) = new SoVertexShapeP;
  PRIVATE(this)->normalcache = NULL;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoVertexShape);

  SO_NODE_ADD_FIELD(vertexProperty, (NULL));
}

/*!
  Destructor.
*/
SoVertexShape::~SoVertexShape()
{
  if (PRIVATE(this)->normalcache) PRIVATE(this)->normalcache->unref();
  delete PRIVATE(this);
}

// *************************************************************************

// Documented in superclass.
void
SoVertexShape::notify(SoNotList * nl)
{
  this->readLockNormalCache();
  if (PRIVATE(this)->normalcache) PRIVATE(this)->normalcache->invalidate();
  this->readUnlockNormalCache();
  inherited::notify(nl);
}

// This documentation block has a copy in vrml97/VertexShape.cpp.
/*!
  \COININTERNAL

  Subclasses should override this method to generate default normals
  using the SoNormalBundle class. \c TRUE should be returned if
  normals were generated, \c FALSE otherwise.

  Default method returns \c FALSE.

  \COIN_FUNCTION_EXTENSION
*/
SbBool
SoVertexShape::generateDefaultNormals(SoState *, SoNormalBundle *)
{
  return FALSE;
}

// This documentation block has a copy in vrml97/VertexShape.cpp.
/*!
  \COININTERNAL

  Subclasses should override this method to generate default normals
  using the SoNormalCache class. This is more effective than using
  SoNormalGenerator. Return \c TRUE if normals were generated, \c
  FALSE otherwise.

  Default method just returns \c FALSE.

  \COIN_FUNCTION_EXTENSION
*/
SbBool
SoVertexShape::generateDefaultNormals(SoState * /* state */,
                                      SoNormalCache * /* nc */)
{
  return FALSE;
}

// doc from superclass
SbBool
SoVertexShape::shouldGLRender(SoGLRenderAction * action)
{
  return SoShape::shouldGLRender(action);
}

/*!
  Sets normal cache to contain the normals specified by \a normals and \a num,
  and forces cache dependencies on coordinates, shape hints and crease angle.
*/
void
SoVertexShape::setNormalCache(SoState * const state,
                              const int num,
                              const SbVec3f * normals)
{
  this->writeLockNormalCache();
  if (PRIVATE(this)->normalcache) PRIVATE(this)->normalcache->unref();
  // create new normal cache with no dependencies
  state->push();
  PRIVATE(this)->normalcache = new SoNormalCache(state);
  PRIVATE(this)->normalcache->ref();
  PRIVATE(this)->normalcache->set(num, normals);
  // force element dependencies
  (void) SoCoordinateElement::getInstance(state);
  (void) SoShapeHintsElement::getVertexOrdering(state);
  (void) SoCreaseAngleElement::get(state);
  state->pop();
  this->writeUnlockNormalCache();
}

/*!
  Returns the current normal cache, or NULL if there is none.
*/
SoNormalCache *
SoVertexShape::getNormalCache(void) const
{
  return PRIVATE(this)->normalcache;
}

/*!  

  Convenience method that can be used by subclasses to return or
  create a normal cache. If the current cache is not valid, it takes
  care of unrefing the old cache and pushing and popping the state to
  create element dependencies when creating the new cache.

  When returning from this method, the normal cache will be
  read locked, and the caller should call readUnlockNormalCache()
  when the normals in the cache are no longer needed.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
SoNormalCache *
SoVertexShape::generateAndReadLockNormalCache(SoState * const state)
{
  this->readLockNormalCache();
  if (PRIVATE(this)->normalcache && PRIVATE(this)->normalcache->isValid(state)) {
    return PRIVATE(this)->normalcache;
  }
  this->readUnlockNormalCache();
  this->writeLockNormalCache();
  
  SbBool storeinvalid = SoCacheElement::setInvalid(FALSE);
  
  if (PRIVATE(this)->normalcache) PRIVATE(this)->normalcache->unref();
  state->push(); // need to push for cache dependencies
  PRIVATE(this)->normalcache = new SoNormalCache(state);
  PRIVATE(this)->normalcache->ref();
  SoCacheElement::set(state, PRIVATE(this)->normalcache);
  //
  // See if the node supports the Coin-way of generating normals
  //
  if (!generateDefaultNormals(state, PRIVATE(this)->normalcache)) {
    // FIXME: implement SoNormalBundle
    if (generateDefaultNormals(state, (SoNormalBundle *)NULL)) {
      // FIXME: set generator in normal cache
    }
  }
  state->pop(); // don't forget this pop
  
  SoCacheElement::setInvalid(storeinvalid);
  this->writeUnlockNormalCache();
  this->readLockNormalCache();
  return PRIVATE(this)->normalcache;
}

/*!
  Convenience method that returns the current coordinate and normal
  element. This method is not part of the Open Inventor API.
*/
void
SoVertexShape::getVertexData(SoState * state,
                             const SoCoordinateElement *& coords,
                             const SbVec3f *& normals,
                             const SbBool neednormals)
{
  coords = SoCoordinateElement::getInstance(state);
  assert(coords);

  normals = NULL;
  if (neednormals) {
    normals = SoNormalElement::getInstance(state)->getArrayPtr();
  }
}

// doc from superclass
void
SoVertexShape::write(SoWriteAction * action)
{
  inherited::write(action);
}

// *************************************************************************

/*!
  Read lock the normal cache. This method should be called before
  fetching the normal cache (using getNormalCache()). When the cached
  normals are no longer needed, readUnlockNormalCache() must be called.
  
  It is also possible to use generateAndReadLockNormalCache().

  \COIN_FUNCTION_EXTENSION

  \sa readUnlockNormalCache()
  \since Coin 2.0
*/
void 
SoVertexShape::readLockNormalCache(void)
{
  if (SoVertexShapeP::normalcachemutex) {
    SoVertexShapeP::normalcachemutex->readLock();
  }
}

/*!
  Read unlock the normal cache. Should be called when the read-locked
  cached normals are no longer needed.

  \sa readLockNormalCache()
  \since Coin 2.0
*/
void 
SoVertexShape::readUnlockNormalCache(void)
{
  if (SoVertexShapeP::normalcachemutex) {
    SoVertexShapeP::normalcachemutex->readUnlock();
  }
}

void 
SoVertexShape::writeLockNormalCache(void)
{
  if (SoVertexShapeP::normalcachemutex) {
    SoVertexShapeP::normalcachemutex->writeLock();
  }
}

void 
SoVertexShape::writeUnlockNormalCache(void)
{
  if (SoVertexShapeP::normalcachemutex) {
    SoVertexShapeP::normalcachemutex->writeUnlock();
  }
}

// *************************************************************************

#undef PRIVATE
