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
  \class SoVRMLIndexedLine SoVRMLIndexedLine.h Inventor/VRMLnodes/SoVRMLIndexedLine.h
  \brief The SoVRMLIndexedLine class is an abstract superclass for lines specified with indices.

  \ingroup coin_VRMLnodes

  \COININTERNAL
*/

#include <Inventor/VRMLnodes/SoVRMLIndexedLine.h>
#include "coindefs.h"

#include <cstddef>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/VRMLnodes/SoVRMLCoordinate.h>

#include "nodes/SoSubNodeP.h"

SO_NODE_ABSTRACT_SOURCE(SoVRMLIndexedLine);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLIndexedLine::initClass(void) // static
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoVRMLIndexedLine, SO_VRML97_NODE_TYPE);
}

SoVRMLIndexedLine::SoVRMLIndexedLine(void) // protected
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLIndexedLine);

  SO_VRMLNODE_ADD_EMPTY_MFIELD(coordIndex);
  SO_VRMLNODE_ADD_EMPTY_MFIELD(colorIndex);
}

SoVRMLIndexedLine::~SoVRMLIndexedLine() // virtual, protected
{
}

// Doc in superclass.
void
SoVRMLIndexedLine::computeBBox(SoAction * COIN_UNUSED_ARG(action),
                               SbBox3f & box,
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
    assert(idx < numCoords);
    if (idx >= 0) box.extendBy(coords[idx]);
  }
  if (!box.isEmpty()) center = box.getCenter();
}

int
SoVRMLIndexedLine::getNumVerts(int COIN_UNUSED_ARG(startcoord))
{
  // FIXME: why is there just a dummy implementation of this method?)
  // Please document special cases like this.  20030603 mortene.
  return 0;
}

void
SoVRMLIndexedLine::setupIndices(int COIN_UNUSED_ARG(numFaces))
{
  // FIXME: why is there just a dummy implementation of this method?
  // Please document special cases like this.  20030603 mortene.
}

const int32_t *
SoVRMLIndexedLine::getColorIndices(void) // protected
{
  // FIXME: why is there just a dummy implementation of this method?
  // Please document special cases like this.  20030603 mortene.
  return NULL;
}

void
SoVRMLIndexedLine::notify(SoNotList * list)
{
  // FIXME: why is there just a dummy implementation of this method?
  // Please document special cases like this.  20030603 mortene.
  inherited::notify(list);
}

#endif // HAVE_VRML97
