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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97

/*!
  \class SoVRMLVertexShape SoVRMLVertexShape.h Inventor/VRMLnodes/SoVRMLVertexShape.h
  \brief The SoVRMLVertexShape class is a superclass for vertex based shapes.
*/

/*!
  \var SoSFNode SoVRMLVertexShape::coord
  Should contain an SoVRMLCoordinate node.
*/

/*!
  \var SoSFNode SoVRMLVertexShape::texCoord
  Can contain an SoVRMLTextureCoordinate node.
*/

/*!
  \var SoSFNode SoVRMLVertexShape::normal
  Can contain an SoVRMLNormal node.
*/

/*!
  \var SoSFNode SoVRMLVertexShape::color
  Can contain an SoVRMLColor node.
*/

/*!
  \var SoSFBool SoVRMLVertexShape::colorPerVertex
  When TRUE, colors are applied per vertex. Default value is TRUE.
*/

/*!
  \var SoSFBool SoVRMLVertexShape::normalPerVertex
  When TRUE, normals are applied per vertex. Default value is TRUE.
*/

#include <Inventor/VRMLnodes/SoVRMLVertexShape.h>

#include <cstddef>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/VRMLnodes/SoVRMLNormal.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/errors/SoDebugError.h>
#ifdef HAVE_THREADS
#include <Inventor/threads/SbRWMutex.h>
#endif // HAVE_THREADS

#include "nodes/SoSubNodeP.h"

// *************************************************************************

class SoVRMLVertexShapeP {
public:
  SoVRMLVertexShapeP(void) 
#ifdef COIN_THREADSAFE
    : normalcachemutex(SbRWMutex::READ_PRECEDENCE)
#endif // COIN_THREADSAFE
  { }

  SoNormalCache * normalcache;
#ifdef COIN_THREADSAFE
  SbRWMutex normalcachemutex;
#endif // COIN_THREADSAFE
};

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

SO_NODE_ABSTRACT_SOURCE(SoVRMLVertexShape);

// *************************************************************************

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLVertexShape::initClass(void)
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoVRMLVertexShape, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLVertexShape::SoVRMLVertexShape(void)
{
  PRIVATE(this) = new SoVRMLVertexShapeP;
  PRIVATE(this)->normalcache = NULL;

  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLVertexShape);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(coord, (NULL));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(texCoord, (NULL));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(normal, (NULL));
  SO_VRMLNODE_ADD_FIELD(color, (NULL));
  SO_VRMLNODE_ADD_FIELD(colorPerVertex, (TRUE));
  SO_VRMLNODE_ADD_FIELD(normalPerVertex, (TRUE));
}

/*!
  Destructor.
*/
SoVRMLVertexShape::~SoVRMLVertexShape()
{
  if (PRIVATE(this)->normalcache) PRIVATE(this)->normalcache->unref();
  delete PRIVATE(this);
}

// This documentation block has a copy in shapenodes/VertexShape.cpp.
/*!
  \COININTERNAL

  Subclasses should override this method to generate default normals
  using the SoNormalBundle class. \c TRUE should be returned if
  normals were generated, \c FALSE otherwise.

  Default method returns \c FALSE.

  \COIN_FUNCTION_EXTENSION
*/
SbBool
SoVRMLVertexShape::generateDefaultNormals(SoState * ,
                                          SoNormalBundle *)
{
  return FALSE;
}


// This documentation block has a copy in shapenodes/VertexShape.cpp.
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
SoVRMLVertexShape::generateDefaultNormals(SoState * /* state */,
                                          SoNormalCache * /* nc */)
{
  return FALSE;
}

void
SoVRMLVertexShape::doAction(SoAction * action)
{
  SoNode * node;

  node = this->coord.getValue();
  if (node) node->doAction(action);

  node = this->texCoord.getValue();
  if (node) node->doAction(action);

  node = this->normal.getValue();
  if (node) node->doAction(action);

  node = this->color.getValue();
  if (node) node->doAction(action);
}

void
SoVRMLVertexShape::GLRender(SoGLRenderAction * action)
{
  SoNode * node;

  node = this->coord.getValue();
  if (node) node->GLRender(action);

  node = this->texCoord.getValue();
  if (node) node->GLRender(action);

  node = this->normal.getValue();
  if (node) node->GLRender(action);

  node = this->color.getValue();
  if (node) node->GLRender(action);
}

void
SoVRMLVertexShape::getBoundingBox(SoGetBoundingBoxAction * action)
{
  inherited::getBoundingBox(action);
}

void
SoVRMLVertexShape::callback(SoCallbackAction * action)
{
  inherited::callback(action);
}

void
SoVRMLVertexShape::pick(SoPickAction * action)
{
  inherited::pick(action);
}

void
SoVRMLVertexShape::notify(SoNotList * list)
{
  SoField * f = list->getLastField();
  
  if (f == &this->coord) {
    this->readLockNormalCache();
    if (PRIVATE(this)->normalcache) {
      PRIVATE(this)->normalcache->invalidate();
    }
    this->readUnlockNormalCache();
  }
  inherited::notify(list);
}

SbBool
SoVRMLVertexShape::shouldGLRender(SoGLRenderAction * action)
{
  return SoShape::shouldGLRender(action);
}

void
SoVRMLVertexShape::setNormalCache(SoState * state,
                                  int num,
                                  SbVec3f * normals)
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
  state->pop();
  this->writeUnlockNormalCache();
}

/*!  

  Convenience method that can be used by subclasses to return or
  create a normal cache. If the current cache is not valid, it takes
  care of unrefing the old cache and pushing and popping the state to
  create element dependencies when creating the new cache.

  When returning from this method, the normal cache will be
  read locked, and the caller should call readUnlockNormalCache()
  when the normals in the cache is no longer needed.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
SoNormalCache *
SoVRMLVertexShape::generateAndReadLockNormalCache(SoState * const state)
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

SoNormalCache *
SoVRMLVertexShape::getNormalCache(void) const
{
  return PRIVATE(this)->normalcache;
}

/*!
  Convenience method that returns the current coordinate and normal
  element. This method is not part of the OIV API.
*/
void
SoVRMLVertexShape::getVertexData(SoState * state,
                                 const SoCoordinateElement *& coords,
                                 const SbVec3f *& normals,
                                 const SbBool neednormals)
{
  coords = SoCoordinateElement::getInstance(state);
  assert(coords);

  normals = NULL;
  if (neednormals) {
    SoVRMLNormal * node = (SoVRMLNormal*) this->normal.getValue();
    normals = (node && node->vector.getNum()) ? node->vector.getValues(0) : NULL;
  }
}

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
SoVRMLVertexShape::readLockNormalCache(void)
{
#ifdef COIN_THREADSAFE
  PRIVATE(this)->normalcachemutex.readLock();
#endif // COIN_THREADSAFE
}

/*!
  Read unlock the normal cache. Should be called when the read-locked
  cached normals are no longer needed.

  \COIN_FUNCTION_EXTENSION

  \sa readLockNormalCache()
  \since Coin 2.0
*/
void 
SoVRMLVertexShape::readUnlockNormalCache(void)
{
#ifdef COIN_THREADSAFE
  PRIVATE(this)->normalcachemutex.readUnlock();
#endif // COIN_THREADSAFE
}

// write lock normal cache
void 
SoVRMLVertexShape::writeLockNormalCache(void)
{
#ifdef COIN_THREADSAFE
  PRIVATE(this)->normalcachemutex.writeLock();
#endif // COIN_THREADSAFE
}

// write unlock normal cache
void 
SoVRMLVertexShape::writeUnlockNormalCache(void)
{
#ifdef COIN_THREADSAFE
  PRIVATE(this)->normalcachemutex.writeUnlock();
#endif // COIN_THREADSAFE
}

#undef PRIVATE

#endif // HAVE_VRML97
