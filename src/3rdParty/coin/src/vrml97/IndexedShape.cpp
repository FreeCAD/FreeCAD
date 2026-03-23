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
  \class SoVRMLIndexedShape SoVRMLIndexedShape.h Inventor/VRMLnodes/SoVRMLIndexedShape.h
  \brief The SoVRMLIndexedShape class is a superclass for geometry that use indexes.
*/

/*!
  \var SoMFInt32 SoVRMLIndexedShape::coordIndex
  The coordinate index array.
*/

/*!
  \var SoMFInt32 SoVRMLIndexedShape::colorIndex
  The color index array.
*/

/*!
  \var SoMFInt32 SoVRMLIndexedShape::normalIndex
  The normal index array.
*/

/*!
  \var SoMFInt32 SoVRMLIndexedShape::texCoordIndex
  The texture coordinate index array.
*/

#include <Inventor/VRMLnodes/SoVRMLIndexedShape.h>
#include "coindefs.h"

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/VRMLnodes/SoVRMLCoordinate.h>
#include <Inventor/caches/SoNormalCache.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/errors/SoDebugError.h>

#include "nodes/SoSubNodeP.h"

SO_NODE_ABSTRACT_SOURCE(SoVRMLIndexedShape);

/*!
  \copydetails SoNode::initClass(void)
*/
void 
SoVRMLIndexedShape::initClass(void)
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoVRMLIndexedShape, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLIndexedShape::SoVRMLIndexedShape(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLIndexedShape);

  SO_VRMLNODE_ADD_EMPTY_MFIELD(coordIndex);
  SO_VRMLNODE_ADD_EMPTY_MFIELD(colorIndex);
  SO_VRMLNODE_ADD_EMPTY_MFIELD(normalIndex);
  SO_VRMLNODE_ADD_EMPTY_MFIELD(texCoordIndex);
}

/*!
  Destructor.
*/
SoVRMLIndexedShape::~SoVRMLIndexedShape()
{
}

/*!
  Convenience method that will fetch data needed for rendering or
  generating primitives. Takes care of normal cache.
*/
SbBool
SoVRMLIndexedShape::getVertexData(SoState * state,
                                  const SoCoordinateElement *& coords,
                                  const SbVec3f *& normals,
                                  const int32_t *& cindices,
                                  const int32_t *& nindices,
                                  const int32_t *& tindices,
                                  const int32_t *& mindices,
                                  int & numcindices,
                                  const SbBool neednormals,
                                  SbBool & normalcacheused)
{
  SoVRMLVertexShape::getVertexData(state, coords, normals, neednormals);

  cindices = this->coordIndex.getValues(0);
  numcindices = this->coordIndex.getNum();

  mindices = this->colorIndex.getValues(0);
  if (this->colorIndex.getNum() <= 0 || mindices[0] < 0) mindices = NULL;

  tindices = this->texCoordIndex.getValues(0);
  if (this->texCoordIndex.getNum() <= 0 || tindices[0] < 0) tindices = NULL;

  normalcacheused = FALSE;
  nindices = NULL;

  if (neednormals) {
    nindices = this->normalIndex.getValues(0);
    if (this->normalIndex.getNum() <= 0 || nindices[0] < 0) nindices = NULL;
    
    if (normals == NULL) {
      SoNormalCache * nc = this->generateAndReadLockNormalCache(state);
      normals = nc->getNormals();
      nindices = nc->getIndices();
      normalcacheused = TRUE;
      // if no normals were generated, unlock normal cache before
      // returning
      if (normals == NULL) {
        this->readUnlockNormalCache();
        normalcacheused = FALSE;
      }
    }
  }
  return TRUE;
}

// Doc in parent
void
SoVRMLIndexedShape::notify(SoNotList * list)
{
  SoField * f = list->getLastField();
  if (f == &this->coordIndex) {
    SoNormalCache * nc = this->getNormalCache();
    if (nc) nc->invalidate();
  }
  inherited::notify(list);
}

// Doc in parent
void
SoVRMLIndexedShape::computeBBox(SoAction * COIN_UNUSED_ARG(action), SbBox3f & box,
                                SbVec3f & center)
{
  SoVRMLCoordinate * node = (SoVRMLCoordinate*) this->coord.getValue();
  if (node == NULL) return;

  int numCoords = node->point.getNum();
  const SbVec3f * coords = node->point.getValues(0);

  box.makeEmpty();
  const int32_t * ptr = coordIndex.getValues(0);
  const int32_t * endptr = ptr + coordIndex.getNum();
  while (ptr < endptr) {
    int idx = *ptr++;

    if (idx >= numCoords) {
      SoDebugError::post("SoVRMLIndexedShape::computeBBox",
                         "index @ %d: %d is out of bounds [%d, %d]",
                         (ptr - coordIndex.getValues(0)) / sizeof(*ptr),
                         idx, numCoords ? 0 : -1, numCoords - 1);
      continue;
    }

    if (idx >= 0) box.extendBy(coords[idx]);
  }
  if (!box.isEmpty()) center = box.getCenter();
}

#endif // HAVE_VRML97
