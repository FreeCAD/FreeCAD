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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97

/*!
  \class SoVRMLSpotLight SoVRMLSpotLight.h Inventor/VRMLnodes/SoVRMLSpotLight.h
  \brief The SoVRMLSpotLight class defines a spot light source.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  SpotLight {
    exposedField SFFloat ambientIntensity  0         # [0,1]
    exposedField SFVec3f attenuation       1 0 0     # [0,inf)
    exposedField SFFloat beamWidth         1.570796  # (0,pi/2]
    exposedField SFColor color             1 1 1     # [0,1]
    exposedField SFFloat cutOffAngle       0.785398  # (0,pi/2]
    exposedField SFVec3f direction         0 0 -1    # (-inf, inf)
    exposedField SFFloat intensity         1         # [0,1]
    exposedField SFVec3f location          0 0 0     # (-inf, inf)
    exposedField SFBool  on                TRUE
    exposedField SFFloat radius            100       # [0, inf)
  }
  \endverbatim

  The SpotLight node defines a light source that emits light from a specific
  point along a specific direction vector and constrained within a solid angle.
  Spotlights may illuminate geometry nodes that respond to light sources and
  intersect the solid angle defined by the SpotLight. Spotlight nodes are
  specified in the local coordinate system and are affected by ancestors'
  transformations.

  A detailed description of ambientIntensity, color, intensity, and
  VRML's lighting equations is provided in 4.6.6, Light sources
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.6>).
  More information on lighting concepts can be found in 4.14, Lighting
  model
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.14>),
  including a detailed description of the VRML lighting equations.

  The \e location field specifies a translation offset of the centre
  point of the light source from the light's local coordinate system
  origin.  This point is the apex of the solid angle which bounds
  light emission from the given light source.

  The \e direction field
  specifies the direction vector of the light's central axis defined
  in the local coordinate system.

  The \e on field specifies whether the
  light source emits light. If on is TRUE, the light source is
  emitting light and may illuminate geometry in the scene. If on is
  FALSE, the light source does not emit light and does not illuminate
  any geometry.

  The \e radius field specifies the radial extent of the
  solid angle and the maximum distance from location that may be
  illuminated by the light source. The light source does not emit
  light outside this radius.  The radius shall be greater than or
  equal to zero.

  Both \e radius and \e location are affected by ancestors'
  transformations (scales affect radius and transformations affect
  location).

  The \e cutOffAngle field specifies the outer bound of the
  solid angle.  The light source does not emit light outside of this
  solid angle.

  The \e beamWidth field specifies an inner solid angle in
  which the light source emits light at uniform full intensity. The
  light source's emission intensity drops off from the inner solid
  angle (beamWidth) to the outer solid angle (cutOffAngle) as
  described in the following equations:

  \verbatim
  angle = the angle between the Spotlight's direction vector
          and the vector from the Spotlight location to the point
          to be illuminated

  if (angle >= cutOffAngle):
    multiplier = 0
  else if (angle <= beamWidth):
    multiplier = 1
  else:
    multiplier = (angle - cutOffAngle) / (beamWidth - cutOffAngle)

  intensity(angle) = SpotLight.intensity × multiplier
  \endverbatim

  If the beamWidth
  is greater than the cutOffAngle, beamWidth is defined to be equal to
  the cutOffAngle and the light source emits full intensity within the
  entire solid angle defined by cutOffAngle.  Both beamWidth and
  cutOffAngle shall be greater than 0.0 and less than or equal to
  pi/2.

  Figure 6.16 depicts the beamWidth, cutOffAngle, direction, location,
  and radius fields of the SpotLight node.

  <center>
  <img src="http://www.web3d.org/documents/specifications/14772/V2.0/Images/spotlight.gif">
  Figure 6.16 -- SpotLight node
  </center>

  SpotLight illumination falls off with distance as specified by three
  attenuation coefficients. The attenuation factor is

  \verbatim
  1/max(attenuation[0] + attenuation[1]×r + attenuation[2]×r^2 , 1),
  \endverbatim

  where r is the distance from the light to the surface being
  illuminated. The default is no attenuation. An attenuation value of
  (0, 0, 0) is identical to (1, 0, 0). Attenuation values shall be
  greater than or equal to zero. A detailed description of VRML's
  lighting equations is contained in 4.14, Lighting model
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.14>).

*/

/*!
  \var SoSFVec3f SoVRMLSpotLight::location
  The light position. Default value is (0, 0, 0).
*/

/*!
  \var SoSFVec3f SoVRMLSpotLight::direction
  The light direction. Default value is (0, 0, 1).
*/

/*!
  \var SoSFFloat SoVRMLSpotLight::beamWidth
  The spot beam width. Default value is PI/2.
*/

/*!
  \var SoSFFloat SoVRMLSpotLight::cutOffAngle
  The spot light cut off angle. Default value is PI/4.
*/

/*!
  \var SoSFFloat SoVRMLSpotLight::radius
  The light radius. Light is not emitted past it. Default value is 100.
*/

/*!
  \var SoSFVec3f SoVRMLSpotLight::attenuation
  The attenuiation vector. Default value is (1, 0, 0).
*/

#include <Inventor/VRMLnodes/SoVRMLSpotLight.h>

#include <cmath>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/SbColor4f.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoEnvironmentElement.h>
#include <Inventor/elements/SoGLLightIdElement.h>
#include <Inventor/system/gl.h>
#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include "nodes/SoSubNodeP.h"

SO_NODE_SOURCE(SoVRMLSpotLight);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLSpotLight::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLSpotLight, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLSpotLight::SoVRMLSpotLight(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLSpotLight);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(location, (0.0f, 0.0f, 0.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(direction,(0.0f, 0.0f, -1.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(beamWidth, (float(M_PI)/2.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(cutOffAngle, (float(M_PI)/4.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(radius, (100.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(attenuation, (1.0f, 0.0f, 0.0f));
}

/*!
  Destructor.
*/
SoVRMLSpotLight::~SoVRMLSpotLight()
{
}

// Doc in parent
void
SoVRMLSpotLight::GLRender(SoGLRenderAction * action)
{
  if (!this->on.getValue()) return;

  SoState * state = action->getState();
  int idx = SoGLLightIdElement::increment(state);

  if (idx < 0) {
#if COIN_DEBUG
    SoDebugError::post("SoSpotLight::GLRender()",
                       "Max # lights exceeded :(\n");
#endif // COIN_DEBUG
    return;
  }

  GLenum light = (GLenum) (idx + GL_LIGHT0);

  SbVec3f att = this->attenuation.getValue();

  glLightf(light, GL_CONSTANT_ATTENUATION, att[0]);
  glLightf(light, GL_LINEAR_ATTENUATION, att[1]);
  glLightf(light, GL_QUADRATIC_ATTENUATION, att[2]);

  SbColor4f lightcolor(0.0f, 0.0f, 0.0f, 1.0f);
  lightcolor.setRGB(this->color.getValue());
  lightcolor *= this->ambientIntensity.getValue();
  glLightfv(light, GL_AMBIENT, lightcolor.getValue());

  lightcolor.setRGB(this->color.getValue());
  lightcolor *= this->intensity.getValue();

  glLightfv(light, GL_DIFFUSE, lightcolor.getValue());
  glLightfv(light, GL_SPECULAR, lightcolor.getValue());

  SbVec3f loc = this->location.getValue();

  // point (or spot) light when w = 1.0
  SbVec4f posvec(loc[0], loc[1], loc[2], 1.0f);
  glLightfv(light, GL_POSITION, posvec.getValue());
  glLightfv(light, GL_SPOT_DIRECTION, this->direction.getValue().getValue());

  float cutoff = SbClamp(this->cutOffAngle.getValue(), 0.0f, float(M_PI)*0.5f) * 180.0f / float(M_PI);
  glLightf(light, GL_SPOT_EXPONENT, 0.0f);
  glLightf(light, GL_SPOT_CUTOFF, cutoff);

  // FIXME: consider radius and beamWidth
}

#endif // HAVE_VRML97
