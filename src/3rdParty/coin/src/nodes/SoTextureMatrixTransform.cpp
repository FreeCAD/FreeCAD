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
  \class SoTextureMatrixTransform SoTextureMatrixTransform.h Inventor/nodes/SoTextureMatrixTransform.h
  \brief The SoTextureMatrixTransform class is used to define a texture matrix transformation.

  \ingroup coin_nodes

  Textures applied to shapes in the scene can be transformed by
  "prefixing" in the state with instances of this node type.

  \COIN_CLASS_EXTENSION

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TextureMatrixTransform {
        matrix
          1 0 0 0
          0 1 0 0
          0 0 1 0
          0 0 0 1
    }
  \endcode

  \sa SoTexture3Transform
  \since Coin 2.5
*/

// *************************************************************************

#include <Inventor/nodes/SoTextureMatrixTransform.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/elements/SoGLMultiTextureMatrixElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/C/glue/gl.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFVec3f SoTextureMatrixTransform::matrix

  Texture coordinate matrix. Default is the identity matrix.
*/

// *************************************************************************

SO_NODE_SOURCE(SoTextureMatrixTransform);

/*!
  Constructor.
*/
SoTextureMatrixTransform::SoTextureMatrixTransform(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTextureMatrixTransform);

  SO_NODE_ADD_FIELD(matrix, (SbMatrix::identity()));
}

/*!
  Destructor.
*/
SoTextureMatrixTransform::~SoTextureMatrixTransform()
{
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTextureMatrixTransform::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTextureMatrixTransform, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureMatrixElement);
  SO_ENABLE(SoCallbackAction, SoMultiTextureMatrixElement);
  SO_ENABLE(SoPickAction, SoMultiTextureMatrixElement);
}


// Documented in superclass.
void
SoTextureMatrixTransform::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();

  // don't modify the texture matrix while rendering the shadow map
  if (SoShapeStyleElement::get(state)->getFlags() & SoShapeStyleElement::SHADOWMAP) return;

  int unit = SoTextureUnitElement::get(state);
  const cc_glglue * glue =
    cc_glglue_instance(SoGLCacheContextElement::get(state));
  int maxunits = cc_glglue_max_texture_units(glue);
  
  if (unit < maxunits) {
    SbMatrix mat = this->matrix.getValue();
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
SoTextureMatrixTransform::doAction(SoAction *action)
{
  SbMatrix mat = this->matrix.getValue();
  int unit = SoTextureUnitElement::get(action->getState());
  SoMultiTextureMatrixElement::mult(action->getState(), this, unit, mat);
}

// Documented in superclass.
void
SoTextureMatrixTransform::callback(SoCallbackAction *action)
{
  SoTextureMatrixTransform::doAction(action);
}

// Documented in superclass.
void
SoTextureMatrixTransform::getMatrix(SoGetMatrixAction * action)
{
  int unit = SoTextureUnitElement::get(action->getState()); 
  if (unit == 0) {
    SbMatrix mat = this->matrix.getValue();
    action->getTextureMatrix().multLeft(mat);
    action->getTextureInverse().multRight(mat.inverse());
  }
}

// Documented in superclass.
void
SoTextureMatrixTransform::pick(SoPickAction * action)
{
  SoTextureMatrixTransform::doAction(action);
}
