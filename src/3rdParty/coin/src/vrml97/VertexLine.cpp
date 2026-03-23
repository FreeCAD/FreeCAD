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
  \class SoVRMLVertexLine SoVRMLVertexLine.h Inventor/VRMLnodes/SovRMLVertexLine.h
  \brief The SoVRMLVertexLine class is a superclass for line based VRML geometry.
*/

/*!
  \var SoSFNode SoVRMLVertexLine::coord
  Should contain an SoVRMLCoordinate node.
*/

/*!
  \var SoSFNode SoVRMLVertexLine::color
  Can contain an SoVRMLColor node if multiple colors are wanted.
*/

/*!
  \var SoSFBool SoVRMLVertexLine::colorPerVertex

  When TRUE, colors will be bound per vertex, otherwise per
  line. Default value is TRUE.
  
*/

#include <Inventor/VRMLnodes/SoVRMLVertexLine.h>

#include <cstddef>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/actions/SoGLRenderAction.h>

#include "nodes/SoSubNodeP.h"

SO_NODE_ABSTRACT_SOURCE(SoVRMLVertexLine);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLVertexLine::initClass(void)
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoVRMLVertexLine, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLVertexLine::SoVRMLVertexLine(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLVertexLine);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(coord, (NULL));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(color, (NULL));
  SO_VRMLNODE_ADD_FIELD(colorPerVertex, (TRUE));
}

/*!
  Destructor.
*/
SoVRMLVertexLine::~SoVRMLVertexLine()
{
}

// Doc in parent
void
SoVRMLVertexLine::doAction(SoAction * action)
{
  SoNode * node;

  node = this->coord.getValue();
  if (node) node->doAction(action);

  node = this->color.getValue();
  if (node) node->doAction(action);
}

// Doc in parent
void
SoVRMLVertexLine::GLRender(SoGLRenderAction * action)
{
  SoNode * node;
  
  node = this->coord.getValue();
  if (node) node->GLRender(action);
  
  node = this->color.getValue();
  if (node) node->GLRender(action);
}

// Doc in parent
void
SoVRMLVertexLine::getBoundingBox(SoGetBoundingBoxAction * action)
{
  inherited::getBoundingBox(action);
}

// Doc in parent
void
SoVRMLVertexLine::callback(SoCallbackAction * action)
{
  inherited::callback(action);
}

// Doc in parent
void
SoVRMLVertexLine::pick(SoPickAction * action)
{
  inherited::pick(action);
}

// Doc in parent
void
SoVRMLVertexLine::notify(SoNotList * list)
{
  inherited::notify(list);
}

// Doc in parent
SbBool
SoVRMLVertexLine::shouldGLRender(SoGLRenderAction * action)
{
  if (this->coord.getValue() == NULL) return FALSE;
  return inherited::shouldGLRender(action);
}

#endif // HAVE_VRML97
