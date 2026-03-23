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
  \class SoMatrixTransform SoMatrixTransform.h Inventor/nodes/SoMatrixTransform.h
  \brief The SoMatrixTransform class is a transformation node.

  \ingroup coin_nodes

  This class is the most flexible transformation node, as you can use
  it to accumulate any kind of transformation matrix on top of the
  current model transformation matrix.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    MatrixTransform {
        matrix 1 0 0 0
  0 1 0 0
  0 0 1 0
  0 0 0 1
    }
  \endcode

  \sa SoTransform
*/

// *************************************************************************

#include <Inventor/nodes/SoMatrixTransform.h>

#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFMatrix SoMatrixTransform::matrix
  The transformation matrix. Defaults to the identity matrix.
*/

// *************************************************************************

SO_NODE_SOURCE(SoMatrixTransform);

/*!
  Constructor.
*/
SoMatrixTransform::SoMatrixTransform(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoMatrixTransform);

  SO_NODE_ADD_FIELD(matrix, (SbMatrix::identity()));
}

/*!
  Destructor.
*/
SoMatrixTransform::~SoMatrixTransform()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoMatrixTransform::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoMatrixTransform, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

// Doc from superclass.
void
SoMatrixTransform::doAction(SoAction * action)
{
  if (this->matrix.isIgnored()) { return; }

  const SbMatrix & mat = this->matrix.getValue();
  if (mat != SbMatrix::identity()) {
    SoModelMatrixElement::mult(action->getState(), this,
                               mat);
  }
}

// Doc from superclass.
void
SoMatrixTransform::GLRender(SoGLRenderAction * action)
{
  SoMatrixTransform::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoMatrixTransform::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoMatrixTransform::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoMatrixTransform::callback(SoCallbackAction * action)
{
  SoMatrixTransform::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoMatrixTransform::getMatrix(SoGetMatrixAction * action)
{
  if (this->matrix.isIgnored()) { return; }

  SbMatrix m = this->matrix.getValue();
  action->getMatrix().multLeft(m);

  SbMatrix mi = m.inverse();
  action->getInverse().multRight(mi);
}

// Doc from superclass.
void
SoMatrixTransform::pick(SoPickAction * action)
{
  SoMatrixTransform::doAction((SoAction *)action);
}

// Doc from superclass. Overrides the traversal method in this class for
// the SoGetPrimitiveCountAction because the number of primitives can
// be different depending on scene location (and thereby distance to
// camera) if there are e.g. SoLOD nodes in the scene.
void
SoMatrixTransform::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoMatrixTransform::doAction((SoAction *)action);
}
