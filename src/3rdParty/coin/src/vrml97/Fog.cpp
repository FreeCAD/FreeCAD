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
  \class SoVRMLFog SoVRMLFog.h Inventor/VRMLnodes/SoVRMLFog.h
  \brief The SoVRMLFog class is used to specify a global scene fog.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  Fog {
    exposedField SFColor  color            1 1 1      # [0,1]
    exposedField SFString fogType          "LINEAR"
    exposedField SFFloat  visibilityRange  0          # [0,inf)
    eventIn      SFBool   set_bind
    eventOut     SFBool   isBound
  }
  \endverbatim

  The Fog node provides a way to simulate atmospheric effects by
  blending objects with the colour specified by the color field based
  on the distances of the various objects from the viewer. The
  distances are calculated in the coordinate space of the Fog
  node. The visibilityRange specifies the distance in metres (in the
  local coordinate system) at which objects are totally obscured by
  the fog. Objects located outside the visibilityRange from the viewer
  are drawn with a constant colour of color. Objects very close to the
  viewer are blended very little with the fog color.  

  A visibilityRange of 0.0 disables the Fog node. The visibilityRange
  is affected by the scaling transformations of the Fog node's
  parents; translations and rotations have no affect on
  visibilityRange. Values of the visibilityRange field shall be in the
  range [0, ).  

  Since Fog nodes are bindable children nodes (see 4.6.10, Bindable
  children nodes
  http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.10),
  a Fog node stack exists, in which the top- most Fog node on the
  stack is currently active. To push a Fog node onto the top of the
  stack, a TRUE value is sent to the set_bind eventIn. Once active,
  the Fog node is bound to the browser view. A FALSE value sent to
  set_bind, pops the Fog node from the stack and unbinds it from the
  browser viewer.  More details on the Fog node stack can be found in
  4.6.10, Bindable children nodes
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.10>).

  The fogType field controls how much of the fog colour is blended
  with the object as a function of distance. If fogType is "LINEAR",
  the amount of blending is a linear function of the distance,
  resulting in a depth cueing effect. If fogType is "EXPONENTIAL," an
  exponential increase in blending is used, resulting in a more
  natural fog appearance.  The effect of fog on lighting calculations
  is described in 4.14, Lighting model
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.6.14>).
  

*/

/*!
  \var SoSFString SoVRMLFog::fogType
  Type of fog. Can be either "LINEAR" or "EXPONENTIAL". Default value is "LINEAR".
*/

/*!
  \var SoSFFloat SoVRMLFog::visibilityRange
  Range of visibility. The distance when only the fog color can be seen. Default value is (0.0).
*/

/*!
  \var SoSFColor SoVRMLFog::color
  The fog color. Default value is (1, 1, 1).
*/

/*!
  \var SoSFBool SoVRMLFog::set_bind
  An eventIn that is used to bind this node (make the node active).
*/


/*!
  \var SoSFBool SoVRMLFog::isBound
  An eventOut that is sent when the node is bound/unbound.
*/

#include <Inventor/VRMLnodes/SoVRMLFog.h>

#include <cstring>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/elements/SoEnvironmentElement.h>
#include <Inventor/elements/SoLightAttenuationElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/actions/SoGLRenderAction.h>

#include "nodes/SoSubNodeP.h"

#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->master)

SO_NODE_SOURCE(SoVRMLFog);

static void fog_fieldsensorCB(void * data, SoSensor * sensor);
static void fog_bindingchangeCB(void * data, SoSensor * sensor);

class SoVRMLFogP {

public:
  SoVRMLFogP(SoVRMLFog * masterptr) {
    this->master = masterptr;
  };

  SoVRMLFog * master;

  SoFieldSensor * setbindsensor;
  SoFieldSensor * isboundsensor;

  SoFieldSensor * fogtypesensor;
  SoFieldSensor * visibilitysensor;
  SoFieldSensor * colorsensor;

  SbColor fogColor;
  float visibilityRange;
  int fogType;

};


/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLFog::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLFog, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLFog::SoVRMLFog(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLFog);

  SO_VRMLNODE_ADD_EXPOSED_FIELD(fogType, ("LINEAR"));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(visibilityRange, (0.0f));
  SO_VRMLNODE_ADD_EXPOSED_FIELD(color, (1.0f, 1.0f, 1.0f));
  
  SO_VRMLNODE_ADD_EVENT_IN(set_bind);
  SO_VRMLNODE_ADD_EVENT_OUT(isBound);

  PRIVATE(this) = new SoVRMLFogP(this);

  PRIVATE(this)->fogType = SoEnvironmentElement::HAZE; // 'HAZE' == Linear fog
  PRIVATE(this)->fogColor = SbVec3f(1,1,1);
  PRIVATE(this)->visibilityRange = 0;

  // Binding sensors 
  PRIVATE(this)->setbindsensor = new SoFieldSensor(fog_bindingchangeCB, PRIVATE(this));
  PRIVATE(this)->isboundsensor = new SoFieldSensor(fog_bindingchangeCB, PRIVATE(this));
  PRIVATE(this)->setbindsensor->attach(&this->set_bind);
  PRIVATE(this)->isboundsensor->attach(&this->isBound);
  PRIVATE(this)->setbindsensor->setPriority(0);
  PRIVATE(this)->isboundsensor->setPriority(0);

  // Field sensor
  PRIVATE(this)->fogtypesensor = new SoFieldSensor(fog_fieldsensorCB, PRIVATE(this));
  PRIVATE(this)->fogtypesensor->attach(&this->fogType);
  PRIVATE(this)->fogtypesensor->setPriority(0);

  PRIVATE(this)->visibilitysensor = new SoFieldSensor(fog_fieldsensorCB, PRIVATE(this));
  PRIVATE(this)->visibilitysensor->attach(&this->visibilityRange);
  PRIVATE(this)->visibilitysensor->setPriority(0);

  PRIVATE(this)->colorsensor = new SoFieldSensor(fog_fieldsensorCB, PRIVATE(this));
  PRIVATE(this)->colorsensor->attach(&this->color);
  PRIVATE(this)->colorsensor->setPriority(0);

}

/*!
  Destructor.
*/
SoVRMLFog::~SoVRMLFog()
{
  delete PRIVATE(this)->setbindsensor;
  delete PRIVATE(this)->isboundsensor;
  delete PRIVATE(this)->fogtypesensor;
  delete PRIVATE(this)->visibilitysensor;
  delete PRIVATE(this)->colorsensor;
  delete PRIVATE(this);
}

// Doc in parent
void
SoVRMLFog::GLRender(SoGLRenderAction * action)
{
 
  SoState * state = action->getState();
  SbColor ambColor = SoEnvironmentElement::getAmbientColor(state);
  float ambIntensity = SoEnvironmentElement::getAmbientIntensity(state);
  SbVec3f attenuation = SoLightAttenuationElement::get(state);
 
  SoLightAttenuationElement::set(action->getState(), this,
                                 attenuation);

  SoEnvironmentElement::set(action->getState(),
                            this,
                            ambIntensity,
                            ambColor,
                            attenuation,
                            PRIVATE(this)->fogType,
                            PRIVATE(this)->fogColor,
                            PRIVATE(this)->visibilityRange);

}

void
fog_fieldsensorCB(void * data, SoSensor * sensor)
{
  SoVRMLFogP * pimpl = (SoVRMLFogP *) data;
 
  if (sensor == pimpl->colorsensor) {    
    pimpl->fogColor = pimpl->master->color.getValue();
  }
  else if (sensor == pimpl->fogtypesensor) {    
    if (!strcmp(pimpl->master->fogType.getValue().getString(), "LINEAR")) {
      pimpl->fogType = SoEnvironmentElement::HAZE;
    } 
    else if (!strcmp(pimpl->master->fogType.getValue().getString(), "EXPONENTIAL")) {
      pimpl->fogType = SoEnvironmentElement::SMOKE;
    }    
  }
  else if (sensor == pimpl->visibilitysensor) {
    pimpl->visibilityRange = pimpl->master->visibilityRange.getValue();    
  }

}


void
fog_bindingchangeCB(void * data, SoSensor * sensor)
{
  SoVRMLFogP * pimpl = (SoVRMLFogP *) data;

  // FIXME: Support for 'set_bind' and 'isBound' must be implemented.
  // But first, a Coin viewer must support this kind of special node
  // treatment (this applies to 'Background', 'NavigationInfo' and
  // 'Viewport' nodes as well) (20030811 handegar)

  if (sensor == pimpl->setbindsensor) {
    SoDebugError::postWarning("fog_bindingchangeCB", "'set_bind' event not implemented yet");
  }
  else if (sensor == pimpl->isboundsensor) {
    SoDebugError::postWarning("fog_bindingchangeCB", "'isBound' event not implemented yet");
  }

}

#undef PRIVATE
#undef PUBLIC

#endif // HAVE_VRML97
