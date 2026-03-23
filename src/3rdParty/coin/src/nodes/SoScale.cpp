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
  \class SoScale SoScale.h Inventor/nodes/SoScale.h
  \brief The SoScale class is a node type for scaling scene graph geometry.

  \ingroup coin_nodes

  Use nodes of this type to apply scaling operations during scene graph
  traversals for e.g. rendering. Scale values are specified in a
  triple-value vector, with one scale factor for each of the 3
  principal axes.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Scale {
        scaleFactor 1 1 1
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoScale.h>

#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFVec3f SoScale::scaleFactor

  Specifies scaling along the 3 axes.

  To get a uniform scale applied to the affected shapes, set the
  scaleFactor field to a vector with the same value for all
  components.

  A common error when doing non-uniform scaling in a single direction
  is to set the value for the other two components of the scaleFactor
  vector to 0. This is obviously wrong, they should be set to 1 to \e
  not scale the shape(s) in the other two directions.

  Be careful with setting scaleFactor component values to 0 or to
  negative values.  Most shapes should handle those cases somehow, but
  the results are undefined unless otherwise specified.

  The default value of this vector field is [1.0, 1.0, 1.0].
*/

// *************************************************************************

SO_NODE_SOURCE(SoScale);

// *************************************************************************

/*!
  Constructor.
*/
SoScale::SoScale(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoScale);

  SO_NODE_ADD_FIELD(scaleFactor, (1.0f, 1.0f, 1.0f));
}

/*!
  Destructor.
*/
SoScale::~SoScale()
{
}

// Doc in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoScale::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoScale, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

// Doc in superclass.
void
SoScale::doAction(SoAction * action)
{
  SoModelMatrixElement::scaleBy(action->getState(), this,
                                this->scaleFactor.getValue());
}

// Doc in superclass.
void
SoScale::GLRender(SoGLRenderAction * action)
{
  SoScale::doAction((SoAction *)action);
}

// Doc in superclass.
void
SoScale::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoScale::doAction((SoAction *)action);
}

// Doc in superclass.
void
SoScale::callback(SoCallbackAction *action)
{
  SoScale::doAction((SoAction*)action);
}

// Doc in superclass.
void
SoScale::getMatrix(SoGetMatrixAction * action)
{
  SbVec3f scalevec = this->scaleFactor.getValue();
  SbMatrix m;

  m.setScale(scalevec);
  action->getMatrix().multLeft(m);

  m.setScale(SbVec3f(1.0f / scalevec[0], 1.0f / scalevec[1], 1.0f / scalevec[2]));
  action->getInverse().multRight(m);
}

// Doc in superclass.
void
SoScale::pick(SoPickAction *action)
{
  SoScale::doAction((SoAction*)action);
}

// Doc in superclass. Overrides the traversal method in this class for
// the SoGetPrimitiveCountAction because the number of primitives can
// be different depending on scene location (and thereby distance to
// camera) if there are e.g. SoLOD nodes in the scene.
void
SoScale::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  SoScale::doAction((SoAction*)action);
}
