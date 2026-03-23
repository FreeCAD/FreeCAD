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
  \class SoTexture2Transform SoTexture2Transform.h Inventor/nodes/SoTexture2Transform.h
  \brief The SoTexture2Transform class is used to define 2D texture transformations.

  \ingroup coin_nodes

  Textures applied to shapes in the scene can be transformed by
  "prefixing" in the state with instances of this node
  type. Translations, rotations and scaling in 2D can all be done.

  The default settings of this node's fields equals a "null
  transform", i.e. no transformation.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Texture2Transform {
        translation 0 0
        rotation 0
        scaleFactor 1 1
        center 0 0
    }
  \endcode

  \sa SoTexture3Transform
*/

// *************************************************************************

#include <Inventor/nodes/SoTexture2Transform.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/elements/SoGLMultiTextureMatrixElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/C/glue/gl.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFVec2f SoTexture2Transform::translation

  Texture coordinate translation. Default value is [0, 0].
*/
/*!
  \var SoSFFloat SoTexture2Transform::rotation

  Texture coordinate rotation in radians (around Z-axis, s is X-axis and t is
  Y-axis).  Defaults to an identity rotation (i.e. zero rotation).
*/
/*!
  \var SoSFVec2f SoTexture2Transform::scaleFactor

  Texture coordinate scale factors. Default value is [1, 1].
*/
/*!
  \var SoSFVec2f SoTexture2Transform::center

  Center for scale and rotation. Default value is [0, 0].
*/

// *************************************************************************

SO_NODE_SOURCE(SoTexture2Transform);

/*!
  Constructor.
*/
SoTexture2Transform::SoTexture2Transform(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTexture2Transform);

  SO_NODE_ADD_FIELD(translation, (0.0f, 0.0f));
  SO_NODE_ADD_FIELD(rotation, (0.0f));
  SO_NODE_ADD_FIELD(scaleFactor, (1.0f, 1.0f));
  SO_NODE_ADD_FIELD(center, (0.0f, 0.0f));
}

/*!
  Destructor.
*/
SoTexture2Transform::~SoTexture2Transform()
{
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTexture2Transform::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTexture2Transform, SO_FROM_INVENTOR_1|SoNode::VRML1);

  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureMatrixElement);
  SO_ENABLE(SoCallbackAction, SoMultiTextureMatrixElement);
  SO_ENABLE(SoPickAction, SoMultiTextureMatrixElement);
}


// Documented in superclass.
void
SoTexture2Transform::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();

  // don't modify the texture matrix while rendering the shadow map
  if (SoShapeStyleElement::get(state)->getFlags() & SoShapeStyleElement::SHADOWMAP) return;

  int unit = SoTextureUnitElement::get(state);

  const cc_glglue * glue =
    cc_glglue_instance(SoGLCacheContextElement::get(state));
  int maxunits = cc_glglue_max_texture_units(glue);

  if (unit < maxunits) {
    SbMatrix mat;
    this->makeMatrix(mat);
    SoMultiTextureMatrixElement::mult(state, this, unit, mat);
  }
  else {
    // we already warned in SoTextureUnit. I think it's best to just
    // ignore the texture here so that all textures for non-supported
    // units will be ignored. pederb, 2003-11-10
  }
}

// Documented in superclass.
void
SoTexture2Transform::doAction(SoAction *action)
{
  SbMatrix mat;
  this->makeMatrix(mat);
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state);
  SoMultiTextureMatrixElement::mult(state, this, unit, mat);
}

// Documented in superclass.
void
SoTexture2Transform::callback(SoCallbackAction *action)
{
  SoTexture2Transform::doAction(action);
}

// Documented in superclass.
void
SoTexture2Transform::getMatrix(SoGetMatrixAction * action)
{
  int unit = SoTextureUnitElement::get(action->getState()); 
  if (unit == 0) {
    SbMatrix mat;
    this->makeMatrix(mat);
    action->getTextureMatrix().multLeft(mat);
    action->getTextureInverse().multRight(mat.inverse());
  }
}

// Documented in superclass.
void
SoTexture2Transform::pick(SoPickAction * action)
{
  SoTexture2Transform::doAction(action);
}

//
// generate a matrix based on the fields
//
void
SoTexture2Transform::makeMatrix(SbMatrix & mat)
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
