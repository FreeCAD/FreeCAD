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
  \class SoShapeHintsV10 SoShapeHintsV10.h
  \brief The SoShapeHintsV10 class is a node is for Inventor V1.0 support only.

  \ingroup coin_nodes

  \sa SoShapeHints
*/

#include "upgraders/SoShapeHintsV10.h"

#include <Inventor/nodes/SoShapeHints.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

SO_NODE_SOURCE(SoShapeHintsV10);

/*!
  Constructor.
*/
SoShapeHintsV10::SoShapeHintsV10()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoShapeHintsV10);

  SO_NODE_ADD_FIELD(hints, (SURFACE | UNORDERED | CONCAVE));
  SO_NODE_ADD_FIELD(creaseAngle, (0.0));

  SO_NODE_DEFINE_ENUM_VALUE(Hint, SURFACE);
  SO_NODE_DEFINE_ENUM_VALUE(Hint, UNORDERED);
  SO_NODE_DEFINE_ENUM_VALUE(Hint, CONCAVE);
  SO_NODE_DEFINE_ENUM_VALUE(Hint, SOLID);
  SO_NODE_DEFINE_ENUM_VALUE(Hint, ORDERED);
  SO_NODE_DEFINE_ENUM_VALUE(Hint, CONVEX);

  SO_NODE_SET_SF_ENUM_TYPE(hints, Hint);
}

/*!
  Destructor.
*/
SoShapeHintsV10::~SoShapeHintsV10()
{
}

// Doc from superclass.
void
SoShapeHintsV10::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoShapeHintsV10, SoNode::INVENTOR_1);
}

SoShapeHints *
SoShapeHintsV10::createUpgrade(void) const
{
  SoShapeHints * pp = new SoShapeHints;
  pp->ref();

  Hint hint = (Hint) this->hints.getValue();

  pp->shapeType = (hint & SOLID) ?
    SoShapeHintsElement::SOLID :
    SoShapeHintsElement::UNKNOWN_SHAPE_TYPE;
  pp->vertexOrdering = (hint & ORDERED) ?
    SoShapeHintsElement::COUNTERCLOCKWISE :
    SoShapeHintsElement::UNKNOWN_ORDERING;
  pp->faceType = (hint & CONVEX) ?
    SoShapeHintsElement::CONVEX :
    SoShapeHintsElement::UNKNOWN_FACE_TYPE;
  
  if (this->hints.isIgnored()) {
    pp->shapeType.setIgnored(TRUE);
    pp->vertexOrdering.setIgnored(TRUE);
    pp->faceType.setIgnored(TRUE);
  }

  pp->creaseAngle = this->creaseAngle.getValue();
  if (this->creaseAngle.isIgnored()) {
    pp->creaseAngle.setIgnored(TRUE);
  }

  pp->unrefNoDelete();
  return pp;
}
