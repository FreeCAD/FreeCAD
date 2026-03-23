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
  \class SoPointLight SoPointLight.h Inventor/nodes/SoPointLight.h
  \brief The SoPointLight class is a node type for light sources.

  \ingroup coin_nodes

  Point lights emits light equally in all directions from a specified
  3D location.

  See also documentation of parent class for important information
  regarding light sources in general.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    PointLight {
        on TRUE
        intensity 1
        color 1 1 1
        location 0 0 1
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoPointLight.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/SbColor4f.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoEnvironmentElement.h>
#include <Inventor/elements/SoGLLightIdElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoLightElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/system/gl.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFVec3f SoPointLight::location
  3D position of light source. Default value is <0, 0, 1>.
*/

// *************************************************************************

SO_NODE_SOURCE(SoPointLight);

// *************************************************************************

/*!
  Constructor.
*/
SoPointLight::SoPointLight(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoPointLight);

  SO_NODE_ADD_FIELD(location, (0.0f, 0.0f, 1.0f));
}

/*!
  Destructor.
*/
SoPointLight::~SoPointLight()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoPointLight::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoPointLight, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

// Doc from superclass.
void
SoPointLight::GLRender(SoGLRenderAction * action)
{
  if (!this->on.getValue()) return;

  int idx = SoGLLightIdElement::increment(action->getState());

  if (idx < 0) {
#if COIN_DEBUG
    SoDebugError::post("SoPointLight::GLRender()",
                       "Max # lights exceeded :(\n");
#endif // COIN_DEBUG
    return;
  }

  SoState * state = action->getState();

  SoLightElement::add(state, this, SoModelMatrixElement::get(state) *
                      SoViewingMatrixElement::get(state));
  
  GLenum light = (GLenum) (idx + GL_LIGHT0);

  SbVec3f attenuation = SoEnvironmentElement::getLightAttenuation(state);
  glLightf(light, GL_QUADRATIC_ATTENUATION, attenuation[0]);
  glLightf(light, GL_LINEAR_ATTENUATION, attenuation[1]);
  glLightf(light, GL_CONSTANT_ATTENUATION, attenuation[2]);

  SbColor4f lightcolor(0.0f, 0.0f, 0.0f, 1.0f);
  // disable ambient contribution from this light source
  glLightfv(light, GL_AMBIENT, lightcolor.getValue());

  lightcolor.setRGB(this->color.getValue());
  lightcolor *= this->intensity.getValue();

  glLightfv(light, GL_DIFFUSE, lightcolor.getValue());
  glLightfv(light, GL_SPECULAR, lightcolor.getValue());

  SbVec3f loc = this->location.getValue();

  // point (or spot) light when w = 1.0
  SbVec4f posvec(loc[0], loc[1], loc[2], 1.0f);
  glLightfv(light, GL_POSITION, posvec.getValue());

  // turning off spot light properties for ordinary lights
  glLightf(light, GL_SPOT_EXPONENT, 0.0);
  glLightf(light, GL_SPOT_CUTOFF, 180.0);
}
