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
  \class SoTextureUnit SoTextureUnit.h Inventor/nodes/SoTextureUnit.h
  \brief The SoTextureUnit class is a node for setting the active texture unit.

  \ingroup coin_nodes

  When an SoTextureUnit node is inserted into the scene graph, all
  subsequent texture nodes (SoTexture2, SoTextureCoordinate2,
  SoTextureCoordinate3, SoTexture2Transform, SoTexture3Transform,
  SoTextureCoordinateEnvironment, SoTextureCoordinatePlane and SoComplexity)
  will affect the texture unit set in the unit field.

  See the SoGuiExample module for an usage example for this node.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TextureUnit {
        unit 0
        mappingMethod IMAGE_MAPPING
    }
  \endcode

  \since Coin 2.2
*/

#include <Inventor/nodes/SoTextureUnit.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoGLMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoGLMultiTextureMatrixElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/C/glue/gl.h>
#include <Inventor/errors/SoDebugError.h>

#include "nodes/SoSubNodeP.h"

/*!
  \var SoSFInt32 SoTextureUnit::unit

  The texture unit which will be used for texture nodes following this
  node in the traversal. Default value of the field is 0.
*/

/*!
  \var SoSFEnum SoTextureUnit::mappingMethod

  The mapping method for this unit. Default is IMAGE_MAPPING.

  This field is not currently supported in Coin. It is included to
  support TGS' API. We might support the field in the future.
*/


/*!
  \var SoTextureUnit::MappingMethod SoTextureUnit::IMAGE_MAPPING

  Normal image mapping is used.
*/

/*!
  \var SoTextureUnit::MappingMethod SoTextureUnit::BUMP_MAPPING

  Bump mapping is used.
*/

// *************************************************************************

SO_NODE_SOURCE(SoTextureUnit);

/*!
  Constructor.
*/
SoTextureUnit::SoTextureUnit(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTextureUnit);

  SO_NODE_ADD_FIELD(unit, (0));
  SO_NODE_ADD_FIELD(mappingMethod, (IMAGE_MAPPING));

  SO_NODE_DEFINE_ENUM_VALUE(MappingMethod, IMAGE_MAPPING);
  SO_NODE_DEFINE_ENUM_VALUE(MappingMethod, BUMP_MAPPING);

  SO_NODE_SET_SF_ENUM_TYPE(mappingMethod, MappingMethod);
}

/*!
  Destructor.
*/
SoTextureUnit::~SoTextureUnit()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTextureUnit::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTextureUnit, SO_FROM_COIN_2_2);

  SO_ENABLE(SoGLRenderAction, SoTextureUnitElement);
  SO_ENABLE(SoCallbackAction, SoTextureUnitElement);
  SO_ENABLE(SoPickAction, SoTextureUnitElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoTextureUnitElement);
  SO_ENABLE(SoGetMatrixAction, SoTextureUnitElement);
  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureCoordinateElement);
  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureImageElement);
  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureEnabledElement);
  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureMatrixElement);
}

// Doc from superclass.
void
SoTextureUnit::GLRender(SoGLRenderAction * action)
{
  SoTextureUnit::doAction((SoAction*)action);

  SoState * state = action->getState();
  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));
  int maxunits = cc_glglue_max_texture_units(glue);

  if (this->unit.getValue() >= maxunits) {
    static SbBool first = TRUE;
    if (first) {
      SoDebugError::postWarning("SoTextureUnit::GLRender",
                                "Texture unit %d (counting from 0) requested. "
                                "Your system only supports %d texture unit%s. "
                                "(This warning message only shown once, but "
                                "there could be more cases of this in the "
                                "scene graph.)",
                                this->unit.getValue(), maxunits,
                                maxunits == 1 ? "" : "s");
      first = FALSE;
    }
  }
}

// Doc from superclass.
void
SoTextureUnit::doAction(SoAction * action)
{
  SoTextureUnitElement::set(action->getState(), this,
                            this->unit.getValue());
}

// Doc from superclass.
void
SoTextureUnit::callback(SoCallbackAction * action)
{
  SoTextureUnit::doAction(action);
}

// Doc from superclass.
void
SoTextureUnit::pick(SoPickAction * action)
{
  SoTextureUnit::doAction(action);
}

// Doc from superclass.
void
SoTextureUnit::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoTextureUnit::doAction(action);
}

void
SoTextureUnit::getMatrix(SoGetMatrixAction * action)
{
  SoTextureUnit::doAction(action);
}

/*!

  Returns the maximum number of texture units for the current GL
  context.  Do not call this method if you don't have a current active
  GL context. You should also know that your OpenGL driver supports
  multi-texturing.

  This function is provided only to be compatible with TGS Inventor.
  It's better to use cc_glglue_max_texture_units() if you're using
  Coin (declared in Inventor/C/glue/gl.h).
*/
uint32_t
SoTextureUnit::getMaxTextureUnit(void)
{
  GLint tmp;
  glGetIntegerv(GL_MAX_TEXTURE_UNITS, &tmp);

  return (uint32_t) tmp;
}
