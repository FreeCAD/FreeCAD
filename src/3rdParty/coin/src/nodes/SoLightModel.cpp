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
  \class SoLightModel SoLightModel.h Inventor/nodes/SoLightModel.h
  \brief The SoLightModel class is a node for specifying the model for geometry lighting.

  \ingroup coin_nodes

  Use nodes of this type to set up how lighting should affect
  subsequent geometry in the scene.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    LightModel {
        model PHONG
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/actions/SoCallbackAction.h>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/elements/SoLightModelElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \enum SoLightModel::Model
  Enumerates available light models.
*/
/*!
  \var SoLightModel::Model SoLightModel::BASE_COLOR

  Use the current diffuse material color for subsequent geometry, and
  do not let any light sources influence the appearance of the
  rendering primitives.
*/
/*!
  \var SoLightModel::Model SoLightModel::PHONG

  Use Phong style shading for the geometry.
*/


/*!
  \var SoSFEnum SoLightModel::model

  Light model to use. Defaults to SoLightModel::PHONG.
*/

// *************************************************************************

SO_NODE_SOURCE(SoLightModel);

/*!
  Constructor.
*/
SoLightModel::SoLightModel(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoLightModel);

  SO_NODE_ADD_FIELD(model, (SoLightModel::PHONG));
  SO_NODE_DEFINE_ENUM_VALUE(Model, BASE_COLOR);
  SO_NODE_DEFINE_ENUM_VALUE(Model, PHONG);
  SO_NODE_SET_SF_ENUM_TYPE(model, Model);
}

/*!
  Destructor.
*/
SoLightModel::~SoLightModel()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoLightModel::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoLightModel, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoCallbackAction, SoLazyElement);
  SO_ENABLE(SoGLRenderAction, SoGLLazyElement);

  SO_ENABLE(SoCallbackAction, SoLightModelElement);
  SO_ENABLE(SoGLRenderAction, SoLightModelElement);
}

// Doc from superclass.
void
SoLightModel::GLRender(SoGLRenderAction * action)
{
  SoLightModel::doAction(action);
}

// Doc from superclass.
void
SoLightModel::doAction(SoAction * action)
{
  if (!model.isIgnored()
      && !SoOverrideElement::getLightModelOverride(action->getState())) {
    SoLazyElement::setLightModel(action->getState(),
                                 (int32_t) this->model.getValue());
    if (this->isOverride()) {
      SoOverrideElement::setLightModelOverride(action->getState(), this, TRUE);
    }
  }
}

// Doc from superclass.
void
SoLightModel::callback(SoCallbackAction * action)
{
  SoLightModel::doAction(action);
}
