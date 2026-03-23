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
  \class SoShadowStyle SoShadowStyle.h Inventor/annex/FXViz/nodes/SoShadowStyle.h
  \brief The SoShadowStyle class is a node for setting the shadow style on nodes.

  \ingroup coin_nodes

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    ShadowStyle {
        style CASTS_SHADOW_AND_SHADOWED
    }
  \endcode

  \since Coin 2.5
*/

// *************************************************************************

#include <Inventor/annex/FXViz/nodes/SoShadowStyle.h>

#include <cstdio>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoGLShaderProgramElement.h>

#include "nodes/SoSubNodeP.h"
#include "shaders/SoGLShaderProgram.h"

// *************************************************************************

/*!
  \enum SoShadowStyle::Style

  Enumerates the available style settings.
*/

/*!
  \var SoSFEnum SoShadowStyle::style

  Set the current shadow style. Default value is CASTS_SHADOW_AND_SHADOWED.
*/

/*!
  \var SoShadowStyle::Style SoShadowStyle::NO_SHADOWING
  Neither casts or receives shadows.
*/

/*!
  \var SoShadowStyle::Style SoShadowStyle::CASTS_SHADOW
  Casts shadow, but will no receive any shadows.
*/

/*!
  \var SoShadowStyle::Style SoShadowStyle::SHADOWED
  Receives shadows, but will not cast any shadow.
*/

/*!
  \var SoShadowStyle::Style SoShadowStyle::CASTS_SHADOW_AND_SHADOWED
  Will cast and receive shadows.
*/

// *************************************************************************


SO_NODE_SOURCE(SoShadowStyle);

/*!
  Constructor.
*/
SoShadowStyle::SoShadowStyle(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoShadowStyle);

  SO_NODE_ADD_FIELD(style, (SoShadowStyle::CASTS_SHADOW_AND_SHADOWED));

  SO_NODE_DEFINE_ENUM_VALUE(Style, NO_SHADOWING);
  SO_NODE_DEFINE_ENUM_VALUE(Style, CASTS_SHADOW);
  SO_NODE_DEFINE_ENUM_VALUE(Style, SHADOWED);
  SO_NODE_DEFINE_ENUM_VALUE(Style, CASTS_SHADOW_AND_SHADOWED);
  SO_NODE_SET_SF_ENUM_TYPE(style, Style);
}

/*!
  Destructor.
*/
SoShadowStyle::~SoShadowStyle()
{
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoShadowStyle::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoShadowStyle, SO_FROM_COIN_2_5);
  SO_ENABLE(SoGLRenderAction, SoShadowStyleElement);
}

// Doc from superclass.
void
SoShadowStyle::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();

  SoShadowStyleElement::set(state,
                            this,
                            (int) this->style.getValue());

  if (SoShapeStyleElement::get(state)->getFlags() & SoShapeStyleElement::SHADOWS) {

    if (this->style.getValue() & SHADOWED) {
      SoGLShaderProgramElement::enable(state, TRUE);
    }
    else {
      SoGLShaderProgramElement::enable(state, FALSE);
    }
  }
}



#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  SoShadowStyle * node = new SoShadowStyle;
  assert(node);
  node->ref();
  BOOST_CHECK_MESSAGE(node->getTypeId() != SoType::badType(),
                      "missing class initialization");
  node->unref();
}

#endif // COIN_TEST_SUITE
