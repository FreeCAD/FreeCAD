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
  \class SoVRMLVertexPoint SoVRMLVertexPoint.h Inventor/VRMLnodes/SoVRMLVertexPoint.h
  \brief The SoVRMLVertexPoint class is a superclass for point based VRML shapes.
*/

/*!
  \var SoSFNode SoVRMLVertexPoint::coord
  Should contain an SoVRMLCoordinate node.
*/

/*!
  \var SoSFNode SoVRMLVertexPoint::color
  Can contain an SoVRMLColor node when color per point is needed.
*/

#include <Inventor/VRMLnodes/SoVRMLVertexPoint.h>
#include "coindefs.h"

#include <cstddef>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/VRMLnodes/SoVRMLCoordinate.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>

#include "nodes/SoSubNodeP.h"

SO_NODE_ABSTRACT_SOURCE(SoVRMLVertexPoint);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLVertexPoint::initClass(void)
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoVRMLVertexPoint, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLVertexPoint::SoVRMLVertexPoint(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLVertexPoint);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(coord, (NULL));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(color, (NULL));
}

/*!
  Destructor.
*/
SoVRMLVertexPoint::~SoVRMLVertexPoint()
{
}

// Doc in parent
void
SoVRMLVertexPoint::doAction(SoAction * action)
{
  SoNode * node;

  node = this->coord.getValue();
  if (node) node->doAction(action);

  node = this->color.getValue();
  if (node) node->doAction(action);
}

// Doc in parent
void
SoVRMLVertexPoint::GLRender(SoGLRenderAction * action)
{
  SoNode * node;

  node = this->coord.getValue();
  if (node) node->GLRender(action);

  node = this->color.getValue();
  if (node) node->GLRender(action);
}

// Doc in parent
void
SoVRMLVertexPoint::getBoundingBox(SoGetBoundingBoxAction * action)
{
  inherited::getBoundingBox(action);
}

// Doc in parent
void
SoVRMLVertexPoint::callback(SoCallbackAction * action)
{
  inherited::callback(action);
}

// Doc in parent
void
SoVRMLVertexPoint::pick(SoPickAction * action)
{
  inherited::pick(action);
}

// Doc in parent
void
SoVRMLVertexPoint::notify(SoNotList * list)
{
  inherited::notify(list);
}

// Doc in parent
void
SoVRMLVertexPoint::computeBBox(SoAction * COIN_UNUSED_ARG(action), SbBox3f & box,
                               SbVec3f & center)
{
  SoVRMLCoordinate * node = (SoVRMLCoordinate*) this->coord.getValue();
  if (node == NULL) return;

  int num = node->point.getNum();
  const SbVec3f * coords = node->point.getValues(0);

  box.makeEmpty();
  while (num--) {
    box.extendBy(*coords++);
  }
  if (!box.isEmpty()) center = box.getCenter();
}

// Doc in parent
void
SoVRMLVertexPoint::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  SoVRMLCoordinate * c = (SoVRMLCoordinate*) this->coord.getValue();
  if (c) {
    action->addNumPoints(c->point.getNum());
  }
}

// Doc in parent
SbBool
SoVRMLVertexPoint::shouldGLRender(SoGLRenderAction * action)
{
  if (this->coord.getValue() == NULL) return FALSE;
  return inherited::shouldGLRender(action);
}

#endif // HAVE_VRML97
