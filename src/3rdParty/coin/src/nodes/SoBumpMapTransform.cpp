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
  \class SoBumpMapTransform SoBumpMapTransform.h Inventor/nodes/SoBumpMapTransform.h
  \brief The SoBumpMapTransform class is used to define 2D bump map transformations.

  \ingroup coin_nodes

  Bump maps applied to shapes in the scene can be transformed by
  "prefixing" in the state with instances of this node
  type. Translations, rotations and scaling in 2D can all be done.

  The default settings of this node's fields equals a "null
  transform", i.e. no transformation.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    BumpMapTransform {
        translation 0 0
        rotation 0
        scaleFactor 1 1
        center 0 0
    }
  \endcode

  \since Coin 2.2
*/

#include <Inventor/nodes/SoBumpMapTransform.h>
#include "coindefs.h"

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/elements/SoBumpMapMatrixElement.h>
#include <Inventor/actions/SoCallbackAction.h>

#include "nodes/SoSubNodeP.h"

/*!
  \var SoSFVec2f SoBumpMapTransform::translation

  Texture coordinate translation. Default value is [0, 0].
*/
/*!
  \var SoSFFloat SoBumpMapTransform::rotation

  Texture coordinate rotation (around Z-axis, s is X-axis and t is
  Y-axis).  Defaults to an identity rotation (i.e. zero rotation).
*/
/*!
  \var SoSFVec2f SoBumpMapTransform::scaleFactor

  Texture coordinate scale factors. Default value is [1, 1].
*/
/*!
  \var SoSFVec2f SoBumpMapTransform::center

  Center for scale and rotation. Default value is [0, 0].
*/

// *************************************************************************

SO_NODE_SOURCE(SoBumpMapTransform);

/*!
  Constructor.
*/
SoBumpMapTransform::SoBumpMapTransform(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoBumpMapTransform);

  SO_NODE_ADD_FIELD(translation, (0.0f, 0.0f));
  SO_NODE_ADD_FIELD(rotation, (0.0f));
  SO_NODE_ADD_FIELD(scaleFactor, (1.0f, 1.0f));
  SO_NODE_ADD_FIELD(center, (0.0f, 0.0f));
}

/*!
  Destructor.
*/
SoBumpMapTransform::~SoBumpMapTransform()
{
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoBumpMapTransform::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoBumpMapTransform, SO_FROM_COIN_2_2);

  SO_ENABLE(SoGLRenderAction, SoBumpMapMatrixElement);
  SO_ENABLE(SoCallbackAction, SoBumpMapMatrixElement);
  SO_ENABLE(SoPickAction, SoBumpMapMatrixElement);
}


// Documented in superclass.
void
SoBumpMapTransform::GLRender(SoGLRenderAction * action)
{
  SoBumpMapTransform::doAction(action);
}

// Documented in superclass.
void
SoBumpMapTransform::doAction(SoAction *action)
{
  SbMatrix mat;
  this->makeMatrix(mat);
  SoBumpMapMatrixElement::mult(action->getState(), this, mat);
}

// Documented in superclass.
void
SoBumpMapTransform::callback(SoCallbackAction *action)
{
  SoBumpMapTransform::doAction(action);
}

// Documented in superclass.
void
SoBumpMapTransform::getMatrix(SoGetMatrixAction * COIN_UNUSED_ARG(action))
{
  // do nothing
}

// Documented in superclass.
void
SoBumpMapTransform::pick(SoPickAction * action)
{
  SoBumpMapTransform::doAction(action);
}

//
// generate a matrix based on the fields
//
void
SoBumpMapTransform::makeMatrix(SbMatrix & mat)
{
  SbMatrix tmp;
  SbVec2f c = this->center.isIgnored() ?
    SbVec2f(0.0f, 0.0f) :
    center.getValue();

  mat.makeIdentity();
  mat[3][0] = -c[0];
  mat[3][1] = -c[1];

  SbVec2f scale = this->scaleFactor.getValue();
  if (!this->scaleFactor.isIgnored() &&
      scale != SbVec2f(1.0f, 1.0f)) {
    tmp.makeIdentity();
    tmp[0][0] = scale[0];
    tmp[1][1] = scale[1];
    mat.multRight(tmp);
  }
  if (!this->rotation.isIgnored() && (this->rotation.getValue() != 0.0f)) {
    float cosa = (float)cos(this->rotation.getValue());
    float sina = (float)sin(this->rotation.getValue());
    tmp.makeIdentity();
    tmp[0][0] = cosa;
    tmp[1][0] = -sina;
    tmp[0][1] = sina;
    tmp[1][1] = cosa;
    mat.multRight(tmp);
  }
  if (!translation.isIgnored()) c+= this->translation.getValue();
  if (c != SbVec2f(0.0f, 0.0f)) {
    tmp.makeIdentity();
    tmp[3][0] = c[0];
    tmp[3][1] = c[1];
    mat.multRight(tmp);
  }
}
