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
  \class SoTexture3Transform SoTexture3Transform.h Inventor/nodes/SoTexture3Transform.h
  \brief The SoTexture3Transform class is used to define 3D texture transformations.

  \ingroup coin_nodes

  Textures applied to shapes in the scene can be transformed by
  "prefixing" in the state with instances of this node
  type. Translations, rotations and scaling in 3D can all be done.

  The default settings of this node's fields equals a "null
  transform", i.e. no transformation.

  \COIN_CLASS_EXTENSION

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Texture3Transform {
        translation 0 0 0
        rotation 0 0 1  0
        scaleFactor 1 1 1
        scaleOrientation 0 0 1  0
        center 0 0 0
    }
  \endcode

  \sa SoTexture2Transform
  \since Coin 2.0
  \since TGS Inventor 2.6
*/

// *************************************************************************

#include <Inventor/nodes/SoTexture3Transform.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/elements/SoGLMultiTextureMatrixElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/C/glue/gl.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFVec3f SoTexture3Transform::translation

  Texture coordinate translation. Default value is [0, 0, 0].
*/
/*!
  \var SoSFRotation SoTexture3Transform::rotation

  Texture coordinate rotation (s is X-axis, t is Y-axis and r is
  Z-axis).  Defaults to an identity rotation (i.e. zero rotation).
*/
/*!
  \var SoSFVec3f SoTexture3Transform::scaleFactor

  Texture coordinate scale factors. Default value is [1, 1, 1].
*/
/*!
  \var SoSFRotation SoTexture3Transform::scaleOrientation

  The orientation the texture is set to before scaling.  Defaults to
  an identity rotation (i.e. zero rotation).
*/
/*!
  \var SoSFVec3f SoTexture3Transform::center

  Center for scale and rotation. Default value is [0, 0, 0].
*/

// *************************************************************************

SO_NODE_SOURCE(SoTexture3Transform);

/*!
  Constructor.
*/
SoTexture3Transform::SoTexture3Transform(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTexture3Transform);

  SO_NODE_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(rotation, (SbRotation::identity()));
  SO_NODE_ADD_FIELD(scaleFactor, (1.0f, 1.0f, 1.0f));
  SO_NODE_ADD_FIELD(scaleOrientation, (SbRotation::identity()));
  SO_NODE_ADD_FIELD(center, (0.0f, 0.0f, 0.0f));
}

/*!
  Destructor.
*/
SoTexture3Transform::~SoTexture3Transform()
{
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTexture3Transform::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTexture3Transform, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureMatrixElement);
  SO_ENABLE(SoCallbackAction, SoMultiTextureMatrixElement);
  SO_ENABLE(SoPickAction, SoMultiTextureMatrixElement);
}


// Documented in superclass.
void
SoTexture3Transform::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state); 
  const cc_glglue * glue = 
    cc_glglue_instance(SoGLCacheContextElement::get(state));
  int maxunits = cc_glglue_max_texture_units(glue);
  
  if (unit < maxunits) {
    SbMatrix mat;
    mat.setTransform(this->translation.getValue(),
                     this->rotation.getValue(),
                     this->scaleFactor.getValue(),
                     this->scaleOrientation.getValue(),
                     this->center.getValue());
    SoMultiTextureMatrixElement::mult(state, this, unit, mat);
  }
  else {
    // we already warned in SoTextureUnit. I think it's best to just
    // ignore the texture here so that all textures for non-supported
    // units will be ignored. pederb, 2003-11-11
  }
}

// Documented in superclass.
void
SoTexture3Transform::doAction(SoAction *action)
{
  SoState * state = action->getState();
  int unit = SoTextureUnitElement::get(state); 
  SbMatrix mat;
  mat.setTransform(this->translation.getValue(),
                   this->rotation.getValue(),
                   this->scaleFactor.getValue(),
                   this->scaleOrientation.getValue(),
                   this->center.getValue());
  SoMultiTextureMatrixElement::mult(action->getState(), this, unit, mat);
}

// Documented in superclass.
void
SoTexture3Transform::callback(SoCallbackAction *action)
{
  SoTexture3Transform::doAction(action);
}

// Documented in superclass.
void
SoTexture3Transform::getMatrix(SoGetMatrixAction * action)
{
  int unit = SoTextureUnitElement::get(action->getState()); 
  if (unit == 0) {
    SbMatrix mat;
    mat.setTransform(this->translation.getValue(),
                     this->rotation.getValue(),
                     this->scaleFactor.getValue(),
                     this->scaleOrientation.getValue(),
                     this->center.getValue());
    action->getTextureMatrix().multLeft(mat);
    action->getTextureInverse().multRight(mat.inverse());
  }
}

// Documented in superclass.
void
SoTexture3Transform::pick(SoPickAction * action)
{
  SoTexture3Transform::doAction(action);
}
