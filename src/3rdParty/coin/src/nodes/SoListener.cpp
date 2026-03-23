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
  \class SoListener SoListener.h Inventor/nodes/SoListener.h
  \brief The SoListener class defines listener attributes used when rendering sound.

  \ingroup coin_nodes
  \ingroup coin_sound

  When rendering geometry, one needs to have a camera defining certain
  attributes related to viewing. The SoListener plays a similar
  role when it comes to rendering audio.

  If no SoListener has been encountered when a SoVRMLSound node
  renders itself, it will use the position and the orientation of the
  current camera. In this case, a gain of 1, a dopplerVelocity of 0
  and a dopplerFactor of 0 will be assumed.

  Coin does not currently support Doppler effects, so the
  dopplerVelocity and dopplerFactor fields are currently ignored.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Listener {
        position 0 0 0
        orientation 0 0 1  0
        dopplerVelocity 0 0 0
        dopplerFactor 0
        gain 1
    }
  \endcode

  \sa SoVRMLSound
*/

#include <Inventor/nodes/SoListener.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoAudioDevice.h>
#include <Inventor/elements/SoListenerPositionElement.h>
#include <Inventor/elements/SoListenerOrientationElement.h>
#include <Inventor/elements/SoListenerDopplerElement.h>
#include <Inventor/elements/SoListenerGainElement.h>

#include "nodes/SoSubNodeP.h"

/*!
  \var SoSFVec3f SoListener::position

  Listener position. Defaults to (0.0f, 0.0f, 0.0f).

*/

/*!
  \var SoSFVec3f SoListener::orientation

  Listener orientation specified as a rotation value from the default
  orientation where the listener is looking pointing along the
  negative Z-axis, with "up" along the positive Y-axis. Defaults to
  SbRotation(SbVec3f(0.0f, 0.0f, 1.0f), 0.0f).

*/

/*!
  \var SoSFVec3f SoListener::gain

  The gain is a scalar amplitude multiplier that attenuates all sounds
  in the scene. The legal range is [0.0f, any), however a gain value >
  1.0f might be clamped to 1.0f by the audio device. Defaults to 1.0f,
  meaning that the sound is unattenuated. A gain value of 0.5f would
  be equivalent to a 6dB attenuation. If gain is set to 0.0f, no sound
  can be heard.

*/

/*!
  \var SoSFVec3f SoListener::dopplerVelocity

  The Doppler velocity of the sound. It is the application
  programmer's responsibility to set this value. Coin does not update
  this value automatically based on changes in a sound's
  position. The default value is (0.0f, 0.0f, 0.0f).

  Coin does not yet support Doppler effects.  
*/

/*!
  \var SoSFFloat SoListener::dopplerFactor

  The amount of Doppler effect applied to the sound. The legal range
  is [0.0f, any>, where 0.0f is default and disable all Doppler
  effects, 1.0f would be a typical value for this field if Doppler
  effects are required.

  Coin does not yet support Doppler effects.  
*/


SO_NODE_SOURCE(SoListener);

/*!
  \copybrief SoBase::initClass(void)
*/
void SoListener::initClass()
{
  SO_NODE_INTERNAL_INIT_CLASS(SoListener, SO_FROM_COIN_2_0);

  SO_ENABLE(SoAudioRenderAction, SoListenerPositionElement);
  SO_ENABLE(SoAudioRenderAction, SoListenerOrientationElement);
  SO_ENABLE(SoAudioRenderAction, SoListenerDopplerElement);
  SO_ENABLE(SoAudioRenderAction, SoListenerGainElement);
}

/*!
  Constructor.
*/
SoListener::SoListener()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoListener);
  SO_NODE_ADD_FIELD(position, (0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(orientation, (SbRotation::identity()));
  SO_NODE_ADD_FIELD(dopplerVelocity, (0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(dopplerFactor, (0.0f));
  SO_NODE_ADD_FIELD(gain, (1.0f));
}

/*!
  Destructor.
*/
SoListener::~SoListener()
{
}

// Doc in superclass
void
SoListener::audioRender(SoAudioRenderAction *action)
{
  SoState * state = action->getState();

  if (! this->position.isIgnored()) {
    SbVec3f pos, worldpos;
    pos = this->position.getValue();
    SoModelMatrixElement::get(state).multVecMatrix(pos, worldpos); 
    SoListenerPositionElement::set(state, this, worldpos, TRUE);
#if COIN_DEBUG && 0
    float x, y, z;
    worldpos.getValue(x, y, z);
    SoDebugError::postInfo("SoListener::audioRender", "listenerpos "
            "(%0.2f, %0.2f, %0.2f)", x, y, z);
#endif // debug
  }
  if (! this->orientation.isIgnored()) {
    SbVec3f t;
    SbRotation r;
    SbVec3f s;
    SbRotation so;
    SoModelMatrixElement::get(state).getTransform(t, r, s, so);
    r *= this->orientation.getValue();
    SoListenerOrientationElement::set(state, this, r, TRUE);
  }
  if (! this->dopplerVelocity.isIgnored())
    SoListenerDopplerElement::setDopplerVelocity(state, this, 
                                   this->dopplerVelocity.getValue());
  if (! this->dopplerFactor.isIgnored())
    SoListenerDopplerElement::setDopplerFactor(state, this, 
                                  this->dopplerFactor.getValue());
  if (! this->gain.isIgnored())
    SoListenerGainElement::set(state, this, this->gain.getValue());
}
