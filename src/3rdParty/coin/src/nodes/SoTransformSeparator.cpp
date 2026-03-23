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
  \class SoTransformSeparator SoTransformSeparator.h Inventor/nodes/SoTransformSeparator.h
  \brief The SoTransformSeparator class is a group node preserving the current transformations.

  \ingroup coin_nodes

  This node works like the SoSeparator group node, except that it only
  stores and restores the current model matrix transformation. Other
  appearance settings, like materials, textures, cameras, lights, etc,
  will affect the remaining parts of the scene graph after traversal,
  just like as for the SoGroup node.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TransformSeparator {
    }
  \endcode

  \sa SoSeparator, SoGroup
*/

// *************************************************************************

#include <Inventor/nodes/SoTransformSeparator.h>

#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/elements/SoBBoxModelMatrixElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/misc/SoChildList.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

SO_NODE_SOURCE(SoTransformSeparator);

// *************************************************************************

/*!
  Default constructor.
*/
SoTransformSeparator::SoTransformSeparator(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTransformSeparator);
}

/*!
  Constructor.

  The argument should be the approximate number of children which is
  expected to be inserted below this node. The number need not be
  exact, as it is only used as a hint for better memory resource
  allocation.
*/
SoTransformSeparator::SoTransformSeparator(int nChildren)
  : inherited(nChildren)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTransformSeparator);
}

/*!
  Destructor.
*/
SoTransformSeparator::~SoTransformSeparator()
{
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTransformSeparator::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTransformSeparator, SO_FROM_INVENTOR_1);
}

// Documented in superclass.
void
SoTransformSeparator::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SbMatrix matrix, localMatrix;
  SoBBoxModelMatrixElement::pushMatrix(action->getState(),
                                       matrix,
                                       localMatrix);
  inherited::getBoundingBox(action);
  SoBBoxModelMatrixElement::popMatrix(action->getState(),
                                      matrix,
                                      localMatrix);
}

// Documented in superclass.
void
SoTransformSeparator::doAction(SoAction *action)
{
  SbMatrix matrix = SoModelMatrixElement::pushMatrix(action->getState());
  inherited::doAction(action);
  SoModelMatrixElement::popMatrix(action->getState(), matrix);
}

// Documented in superclass.
void
SoTransformSeparator::callback(SoCallbackAction * action)
{
  SbMatrix matrix = SoModelMatrixElement::pushMatrix(action->getState());
  inherited::callback(action);
  SoModelMatrixElement::popMatrix(action->getState(), matrix);
}

// Documented in superclass.
void
SoTransformSeparator::GLRender(SoGLRenderAction * action)
{
  SbMatrix matrix = SoModelMatrixElement::pushMatrix(action->getState());
  inherited::GLRender(action);
  SoModelMatrixElement::popMatrix(action->getState(), matrix);
  SoGLCacheContextElement::shouldAutoCache(action->getState(),
                                           SoGLCacheContextElement::DONT_AUTO_CACHE);
}

// Documented in superclass.
void
SoTransformSeparator::pick(SoPickAction * action)
{
  SbMatrix matrix = SoModelMatrixElement::pushMatrix(action->getState());
  inherited::pick(action);
  SoModelMatrixElement::popMatrix(action->getState(), matrix);
}

// Documented in superclass.
void
SoTransformSeparator::getMatrix(SoGetMatrixAction * action)
{
  // Will only need to traverse if IN_PATH. Other path codes will have
  // no effect on the result.
  int numindices;
  const int * indices;
  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
    this->children->traverseInPath(action, numindices, indices);
  }
}

// Documented in superclass.
void
SoTransformSeparator::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  // FIXME: this looks mysterious -- doesn't we implicitly assume
  // SoGroup::getPrimitiveCount() here? If so, that shouldn't be
  // necessary. Ask pederb. 20020107 mortene.
  SoTransformSeparator::doAction((SoAction *)action);
}

// Documented in superclass.
void
SoTransformSeparator::audioRender(SoAudioRenderAction * action)
{
  SbMatrix matrix = SoModelMatrixElement::pushMatrix(action->getState());
  inherited::audioRender(action);
  SoModelMatrixElement::popMatrix(action->getState(), matrix);
}
