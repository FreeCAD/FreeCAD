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
  \class SoMultipleCopy SoMultipleCopy.h Inventor/nodes/SoMultipleCopy.h
  \brief The SoMultipleCopy class redraws its children multiple times at different transformations.

  \ingroup coin_nodes

  The SoMultipleCopy group node duplicates its child nodes /
  subgraphs without using additional memory resources.

  It can do general transformations (translations, rotation and
  scaling) for its children. Apart from transformations, the
  appearance of its children will be identical.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    MultipleCopy {
        matrix 1 0 0 0
        0 1 0 0
        0 0 1 0
        0 0 0 1
    }
  \endcode

  \sa SoArray
*/

// *************************************************************************

#include <Inventor/nodes/SoMultipleCopy.h>

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/elements/SoBBoxModelMatrixElement.h>
#include <Inventor/elements/SoSwitchElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoSwitch.h> // SO_SWITCH_ALL

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoMFMatrix SoMultipleCopy::matrix

  A set of geometry transformation matrices.

  The number of duplicated redraws of the child geometry will be the
  same as the number of matrices specified in this field. I.e., each
  duplication will be transformed according to a transformation
  matrix.

  The default value of the field is to contain just a single identity
  matrix.
*/

// *************************************************************************

SO_NODE_SOURCE(SoMultipleCopy);

/*!
  Constructor.
*/
SoMultipleCopy::SoMultipleCopy(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoMultipleCopy);

  SO_NODE_ADD_FIELD(matrix, (SbMatrix::identity()));
}

/*!
  Destructor.
*/
SoMultipleCopy::~SoMultipleCopy()
{
}

// Doc in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoMultipleCopy::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoMultipleCopy, SO_FROM_INVENTOR_1);
}

// Doc in superclass.
void
SoMultipleCopy::getBoundingBox(SoGetBoundingBoxAction * action)
{
  // get reference to the box
  SbXfBox3f & box = action->getXfBoundingBox();

  // store current bbox
  SbXfBox3f incomingbox = box;

  // accumulation variables
  SbVec3f acccenter(0.0f, 0.0f, 0.0f);
  int numCenters = 0;
  
  // make current box empty to calculate bbox of this separator
  box.makeEmpty();
  box.setTransform(SbMatrix::identity());

  // traverse all children, calculate the local bbox
  inherited::getBoundingBox(action);
  SbXfBox3f lbbox = box;

  // Empty the box again
  box.makeEmpty();
  box.setTransform(SbMatrix::identity());

  for (int i=0; i < matrix.getNum(); i++) {
    // Apply transformation to the local bbox
    SbXfBox3f tbbox = lbbox;
    tbbox.transform(matrix[i]);

    // Accumulate center
    acccenter += tbbox.getCenter();
    numCenters++;

    // expand box by transformed bbox
    if (!tbbox.isEmpty()) box.extendBy(tbbox);
  }

  // Extend the bbox by the incoming bbox
  if (!incomingbox.isEmpty()) box.extendBy(incomingbox);

  if (numCenters != 0) {
    action->resetCenter();
    action->setCenter(acccenter / float(numCenters), TRUE);
  }
}

// Doc in superclass.
void
SoMultipleCopy::GLRender(SoGLRenderAction * action)
{
  SoMultipleCopy::doAction((SoAction*)action);
}

// Doc in superclass
void
SoMultipleCopy::audioRender(SoAudioRenderAction * action)
{
  SoMultipleCopy::doAction((SoAction*)action);
}

// Doc in superclass.
SbBool
SoMultipleCopy::affectsState(void) const
{
  // state is pushed/popped for each traversal
  return FALSE;
}

// Doc in superclass.
void
SoMultipleCopy::doAction(SoAction *action)
{
  for (int i=0; i < matrix.getNum(); i++) {
    action->getState()->push();
    SoSwitchElement::set(action->getState(), i);
    SoModelMatrixElement::mult(action->getState(), this, matrix[i]);
    inherited::doAction(action);
    action->getState()->pop();
  }
}

// Doc in superclass.
void
SoMultipleCopy::callback(SoCallbackAction *action)
{
  SoMultipleCopy::doAction((SoAction*)action);
}

// Doc in superclass.
void
SoMultipleCopy::pick(SoPickAction *action)
{
  // We came across what we think is a bug in TGS/SGI OIV when
  // implementing picking for this node and testing against the
  // original Inventor library. The SoPickedPoint class can return the
  // object space point, normal and texture coordinates. TGS/SGI OIV
  // do not consider the translation inside this node before returning
  // the object space data from SoPickedPoint, since the path in
  // SoPickedPoint does not say anything about on which copy the pick
  // occurred.
  //
  // We solved this simply by extending SoPickedPoint for storing both
  // world space and object space data.

  SoMultipleCopy::doAction((SoAction*)action);
}

// Doc in superclass.
void
SoMultipleCopy::handleEvent(SoHandleEventAction *action)
{
  inherited::handleEvent(action);
}

// Doc in superclass.
void
SoMultipleCopy::getMatrix(SoGetMatrixAction *action)
{
  // path does not specify which copy to traverse
  inherited::getMatrix(action);
}

// Doc in superclass.
void
SoMultipleCopy::search(SoSearchAction *action)
{
  SoState * state = action->getState();
  state->push();
  // set Switch element so that subgraphs depending on this element
  // will traverse all children (it's set during normal traversal in
  // doAction()).
  SoSwitchElement::set(action->getState(), SO_SWITCH_ALL);
  // just use SoGroup::search() to traverse all children.
  inherited::search(action);
  state->pop();  
}

// Doc in superclass.
void
SoMultipleCopy::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  SoMultipleCopy::doAction((SoAction*)action);
}
