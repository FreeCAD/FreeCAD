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
  \class SoEnvironment SoEnvironment.h Inventor/nodes/SoEnvironment.h
  \brief The SoEnvironment class is a node for specifying global rendering parameters.

  \ingroup coin_nodes

  This node type provides the application programmer with the ability
  to set global parameters influencing lighting and fog.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Environment {
        ambientIntensity 0.2
        ambientColor 1 1 1
        attenuation 0 0 1
        fogType NONE
        fogColor 1 1 1
        fogVisibility 0
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoEnvironment.h>

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLEnvironmentElement.h>
#include <Inventor/elements/SoLightAttenuationElement.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \enum SoEnvironment::FogType
  Enumeration of available types of fog.
*/
/*!
  \var SoEnvironment::FogType SoEnvironment::NONE

  No fog. Visibility will be equal for all objects, independent of
  distance to camera.
*/
/*!
  \var SoEnvironment::FogType SoEnvironment::HAZE

  Fog where visibility will decrease linearly with distance to camera.
*/
/*!
  \var SoEnvironment::FogType SoEnvironment::FOG

  Fog where visibility will decrease exponentially with distance to
  camera.
*/
/*!
  \var SoEnvironment::FogType SoEnvironment::SMOKE

  Fog where visibility will decrease exponentially with the square of
  the distance to camera (simulating really thick fog).
*/


/*!
  \var SoSFFloat SoEnvironment::ambientIntensity

  A global ambient value for the light intensity for the complete
  scene. This will provide some light even when there are no light
  sources defined for the scene.

  Valid values is from 0.0 (no ambient light) to 1.0 (full ambient
  light intensity). Default value is 0.2.
*/
/*!
  \var SoSFColor SoEnvironment::ambientColor

  The color of the global ambient light. Defaults to full intensity
  white, i.e. <1.0, 1.0, 1.0>.

  Each component of this RGB vector is multiplied with the scalar
  value of SoEnvironment::ambientIntensity.
*/
/*!
  \var SoSFVec3f SoEnvironment::attenuation

  Squared, linear, and constant light attenuation coefficients (in
  that order). Please note that this is the reverse order of how
  attenuation is usually specified.
*/

/*!
  \var SoSFEnum SoEnvironment::fogType

  The fog model. See SoEnvironment::FoType.
*/
/*!
  \var SoSFColor SoEnvironment::fogColor

  Color of fog. Defaults to full intensity white.
*/
/*!
  \var SoSFFloat SoEnvironment::fogVisibility

  The "cut-off" distance from the camera where objects will be totally
  obscured by fog. If set to 0.0, the far plane distance will be used
  instead.

  Default value is 0.0.
*/

// *************************************************************************

SO_NODE_SOURCE(SoEnvironment);

/*!
  Constructor.
*/
SoEnvironment::SoEnvironment()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoEnvironment);

  SO_NODE_ADD_FIELD(ambientIntensity, (0.2f));
  SO_NODE_ADD_FIELD(ambientColor, (1.0f, 1.0f, 1.0f));
  SO_NODE_ADD_FIELD(attenuation, (0.0f, 0.0f, 1.0f));
  SO_NODE_ADD_FIELD(fogType, (SoEnvironment::NONE));
  SO_NODE_ADD_FIELD(fogColor, (1.0f, 1.0f, 1.0f));
  SO_NODE_ADD_FIELD(fogVisibility, (0.0f));

  SO_NODE_DEFINE_ENUM_VALUE(FogType, NONE);
  SO_NODE_DEFINE_ENUM_VALUE(FogType, HAZE);
  SO_NODE_DEFINE_ENUM_VALUE(FogType, FOG);
  SO_NODE_DEFINE_ENUM_VALUE(FogType, SMOKE);
  SO_NODE_SET_SF_ENUM_TYPE(fogType, FogType);
}

/*!
  Destructor.
*/
SoEnvironment::~SoEnvironment()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoEnvironment::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoEnvironment, SO_FROM_INVENTOR_2_0);

  SO_ENABLE(SoGLRenderAction, SoGLEnvironmentElement);
  SO_ENABLE(SoCallbackAction, SoEnvironmentElement);
  SO_ENABLE(SoGLRenderAction, SoLightAttenuationElement);
  SO_ENABLE(SoCallbackAction, SoLightAttenuationElement);
}

// Doc from superclass.
void
SoEnvironment::GLRender(SoGLRenderAction * action)
{
  SoLightAttenuationElement::set(action->getState(), this,
                                 this->attenuation.getValue());
  SoEnvironmentElement::set(action->getState(),
                            this,
                            this->ambientIntensity.getValue(),
                            this->ambientColor.getValue(),
                            this->attenuation.getValue(),
                            (int32_t)fogType.getValue(),
                            this->fogColor.getValue(),
                            this->fogVisibility.getValue());
}

// Doc from superclass.
void
SoEnvironment::callback(SoCallbackAction *action)
{
  SoLightAttenuationElement::set(action->getState(), this,
                                 this->attenuation.getValue());
  SoEnvironmentElement::set(action->getState(),
                            this,
                            this->ambientIntensity.getValue(),
                            this->ambientColor.getValue(),
                            this->attenuation.getValue(),
                            (int32_t)fogType.getValue(),
                            this->fogColor.getValue(),
                            this->fogVisibility.getValue());
}
