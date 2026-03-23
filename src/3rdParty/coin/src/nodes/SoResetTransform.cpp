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
  \class SoResetTransform SoResetTransform.h Inventor/nodes/SoResetTransform.h
  \brief The SoResetTransform class is a node type which makes it possible to "nullify" state during traversal.

  \ingroup coin_nodes

  SoResetTransform is useful for setting up geometry in the scene
  graph which will not be influenced by the transformation nodes
  before it during traversal.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    ResetTransform {
        whatToReset TRANSFORM
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoResetTransform.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/elements/SoGLModelMatrixElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoCacheElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \enum SoResetTransform::ResetType

  The different options for what to reset.
*/
/*!
  \var SoResetTransform::ResetType SoResetTransform::TRANSFORM

  Reset current model transformation.
*/
/*!
  \var SoResetTransform::ResetType SoResetTransform::BBOX

  Reset current bounding box settings.
*/


/*!
  \var SoSFBitMask SoResetTransform::whatToReset

  What this node instance should reset in the state when met during
  traversal. Default value is SoResetTransform::TRANSFORM.
*/


// *************************************************************************

SO_NODE_SOURCE(SoResetTransform);

/*!
  Constructor.
*/
SoResetTransform::SoResetTransform(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoResetTransform);

  SO_NODE_ADD_FIELD(whatToReset, (SoResetTransform::TRANSFORM));

  SO_NODE_DEFINE_ENUM_VALUE(ResetType, TRANSFORM);
  SO_NODE_DEFINE_ENUM_VALUE(ResetType, BBOX);
  SO_NODE_SET_SF_ENUM_TYPE(whatToReset, ResetType);
}

/*!
  Destructor.
*/
SoResetTransform::~SoResetTransform()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoResetTransform::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoResetTransform, SO_FROM_INVENTOR_1);
}

// Doc from superclass.
void
SoResetTransform::GLRender(SoGLRenderAction * action)
{
  if (!this->whatToReset.isIgnored() &&
      (this->whatToReset.getValue() & SoResetTransform::TRANSFORM)) {
    SoState * state = action->getState();
    SoGLModelMatrixElement::makeIdentity(state, this);
  }
}

// Doc from superclass.
void
SoResetTransform::getBoundingBox(SoGetBoundingBoxAction * action)
{
  if (!this->whatToReset.isIgnored() &&
      (this->whatToReset.getValue() & SoResetTransform::TRANSFORM)) {
    SoState * state = action->getState();
    // do this instead of calling SoResetTransform::doAction(action), so
    // that SoLocalBBoxMatrixElement and SoBBoxModelMatrixElement can do
    // the right thing.
    SoModelMatrixElement::mult(state, 
                               this, 
                               SoModelMatrixElement::get(state).inverse());
  }
  if (!this->whatToReset.isIgnored() &&
      (this->whatToReset.getValue() & SoResetTransform::BBOX)) {
    action->getXfBoundingBox().makeEmpty();
    action->resetCenter();
  }
}

// Doc from superclass.
void
SoResetTransform::doAction(SoAction *action)
{
  if (!this->whatToReset.isIgnored() &&
      (this->whatToReset.getValue() & SoResetTransform::TRANSFORM)) {
    SoModelMatrixElement::makeIdentity(action->getState(), this);
  }
}

// Doc from superclass.
void
SoResetTransform::callback(SoCallbackAction *action)
{
  SoResetTransform::doAction((SoAction*)action);
}

// Doc from superclass.
void
SoResetTransform::getMatrix(SoGetMatrixAction *action)
{
  if (!this->whatToReset.isIgnored() &&
      (this->whatToReset.getValue() & SoResetTransform::TRANSFORM)) {
    action->getMatrix().makeIdentity();
    action->getInverse().makeIdentity();
  }
}

// Doc from superclass.
void
SoResetTransform::pick(SoPickAction *action)
{
  SoResetTransform::doAction((SoAction*)action);
}

// Doc from superclass. Overrides the traversal method in this class for
// the SoGetPrimitiveCountAction because the number of primitives can
// be different depending on scene location (and thereby distance to
// camera) if there are e.g. SoLOD nodes in the scene.
void
SoResetTransform::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  SoResetTransform::doAction((SoAction*)action);
}
