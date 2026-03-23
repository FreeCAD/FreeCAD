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
  \class SoTransparencyType SoTransparencyType.h Inventor/nodes/SoTransparencyType.h
  \brief The SoTransparencyType class is a node for setting the transparency type for shapes.

  \ingroup coin_nodes

  In earlier versions of Coin/Open Inventor it was only possible to
  set the transparency mode globally for an entire scene graph, which
  could be inconvenient if different transparency types were wanted for
  different shapes.

  Here is a screenshot of the different transparency modes used in a
  single scene.
 
  <center>
  \image html transparencytype.png "Rendering of Different Transparency Modes"
  </center>

  \COIN_CLASS_EXTENSION

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    TransparencyType {
        value SCREEN_DOOR
    }
  \endcode

  \sa SoGLRenderAction::TransparencyType
  \since Coin 2.0
*/

// *************************************************************************

#include <Inventor/nodes/SoTransparencyType.h>

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoLazyElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \enum SoTransparencyType::Type
  Enumeration of available transparency types. See documentation in
  SoGLRenderAction for a description of the different types.
*/

/*!
  \var SoSFEnum SoTransparencyType::value

  The transparency type to use for subsequent shape nodes in the scene
  graph.
*/


// *************************************************************************

SO_NODE_SOURCE(SoTransparencyType);

/*!
  Constructor.
*/
SoTransparencyType::SoTransparencyType(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTransparencyType);

  SO_NODE_ADD_FIELD(value, (SCREEN_DOOR));

  SO_NODE_DEFINE_ENUM_VALUE(Type, SCREEN_DOOR);
  SO_NODE_DEFINE_ENUM_VALUE(Type, ADD);
  SO_NODE_DEFINE_ENUM_VALUE(Type, DELAYED_ADD);
  SO_NODE_DEFINE_ENUM_VALUE(Type, BLEND);
  SO_NODE_DEFINE_ENUM_VALUE(Type, DELAYED_BLEND);
  SO_NODE_DEFINE_ENUM_VALUE(Type, SORTED_OBJECT_ADD);
  SO_NODE_DEFINE_ENUM_VALUE(Type, SORTED_OBJECT_BLEND);
  SO_NODE_DEFINE_ENUM_VALUE(Type, SORTED_OBJECT_SORTED_TRIANGLE_ADD);
  SO_NODE_DEFINE_ENUM_VALUE(Type, SORTED_OBJECT_SORTED_TRIANGLE_BLEND);
  SO_NODE_DEFINE_ENUM_VALUE(Type, NONE);

  SO_NODE_SET_SF_ENUM_TYPE(value, Type);
}


/*!
  Destructor.
*/
SoTransparencyType::~SoTransparencyType()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoTransparencyType::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTransparencyType, SO_FROM_INVENTOR_1);
}

// Doc from superclass.
void
SoTransparencyType::GLRender(SoGLRenderAction * action)
{
  SoTransparencyType::doAction(action);
}

// Doc from superclass.
void
SoTransparencyType::doAction(SoAction * action)
{
  if (!this->value.isIgnored()
      && !SoOverrideElement::getTransparencyTypeOverride(action->getState())) {
    SoShapeStyleElement::setTransparencyType(action->getState(),
                                             this->value.getValue());
    SoLazyElement::setTransparencyType(action->getState(),
                                       this->value.getValue());
    if (this->isOverride()) {
      SoOverrideElement::setTransparencyTypeOverride(action->getState(), this, TRUE);
    }
  }
}

// Doc from superclass.
void
SoTransparencyType::callback(SoCallbackAction * action)
{
  SoTransparencyType::doAction((SoAction *)action);
}
