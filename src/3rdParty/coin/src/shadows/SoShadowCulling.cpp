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
  \class SoShadowCulling SoShadowCulling.h Inventor/annex/FXViz/nodes/SoShadowCulling.h
  \brief The SoShadowCulling class is a node for setting the shadow style on nodes.

  \ingroup coin_nodes

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    ShadowCulling {
        mode AS_IS_CULLING
    }
  \endcode

  \since Coin 2.5
*/

// *************************************************************************

#include <Inventor/annex/FXViz/nodes/SoShadowCulling.h>

#include <cstdio>

#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoGLShaderProgramElement.h>
#include <Inventor/actions/SoGLRenderAction.h>

#include "nodes/SoSubNodeP.h"
#include "shaders/SoGLShaderProgram.h"

// *************************************************************************

/*!
  \enum SoShadowCulling::Mode

  Enumerates the available culling modes.
*/

/*!
  \var SoSFEnum SoShadowCulling::mode
  
  Sets the culling mode. Default is AS_IS_CULLING.
*/

/*!
  \var SoShadowCulling::Mode SoShadowCulling::AS_IS_CULLING
  
  Use the culling specified in the scene graph.
*/

/*!
  \var SoShadowCulling::Style SoShadowCulling::NO_CULLING

  Render both back facing and front facing triangles into the shadow map.
*/


// *************************************************************************


SO_NODE_SOURCE(SoShadowCulling);

/*!
  Constructor.
*/
SoShadowCulling::SoShadowCulling(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoShadowCulling);

  SO_NODE_ADD_FIELD(mode, (AS_IS_CULLING));

  SO_NODE_DEFINE_ENUM_VALUE(Mode, AS_IS_CULLING);
  SO_NODE_DEFINE_ENUM_VALUE(Mode, NO_CULLING);
  SO_NODE_SET_SF_ENUM_TYPE(mode, Mode);
}

/*!
  Destructor.
*/
SoShadowCulling::~SoShadowCulling()
{
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoShadowCulling::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoShadowCulling, SO_FROM_COIN_2_5);
  SO_ENABLE(SoGLRenderAction, SoGLShadowCullingElement);
}

// Doc from superclass.
void
SoShadowCulling::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();

  if (SoShapeStyleElement::get(state)->getFlags() & SoShapeStyleElement::SHADOWMAP) {
    int32_t mode = this->mode.getValue();
    SoGLShadowCullingElement::set(state, this, mode);
    
    if (mode == NO_CULLING) {
      SoShapeHintsElement::set(state, NULL, 
                               SoShapeHintsElement::UNKNOWN_ORDERING,
                               SoShapeHintsElement::UNKNOWN_SHAPE_TYPE,
                               SoShapeHintsElement::UNKNOWN_FACE_TYPE);
      SoOverrideElement::setShapeHintsOverride(state, NULL, TRUE);
    }
    else {
      // FIXME: need to restore the previous ShapeHints settings in some way,
      // or require that this node is used only inside a separator
      SoOverrideElement::setShapeHintsOverride(state, NULL, FALSE);
    }
  }
}


#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoShadowCulling * node = new SoShadowCulling;
  assert(node);
  node->ref();
  BOOST_CHECK_MESSAGE(node->getTypeId() != SoType::badType(),
                      "missing class initialization");
  node->unref();
}

#endif // COIN_TEST_SUITE
