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
  \class SoListenerDopplerElement Inventor/elements/SoListenerDopplerElement.h
  \brief The SoListenerDopplerElement holds the Doppler velocity and factor of the current listener.

  \ingroup coin_elements

  The dopplerVelocity and dopplerFactor is set by SoListener nodes
  during audio rendering. The SoListenerDopplerElement is used when
  the SoVRMLSound nodes render themselves.

  \COIN_CLASS_EXTENSION

  \since Coin 2.0
*/

#include <Inventor/elements/SoListenerDopplerElement.h>

#include "coindefs.h"
#include "SbBasicP.h"

#include <Inventor/nodes/SoNode.h>

/*!
  \fn SoListenerDopplerElement::dopplerVelocity

  The Doppler velocity of the listener.
  It is the application programmer's responsibility to
  set this value. Coin does not update this value
  automatically based on changes in a listener's position.
*/

/*!
  \fn SoListenerDopplerElement::dopplerFactor

  The amount of Doppler effect applied to the listener. The normal
  range would be [0.0f to 1.0f], where 0.0f is default and disable all
  Doppler effects.
*/

SO_ELEMENT_SOURCE(SoListenerDopplerElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoListenerDopplerElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoListenerDopplerElement, inherited);
}

/*!
  Destructor.
*/

SoListenerDopplerElement::~SoListenerDopplerElement(void)
{
}

/*!
  Initializes the element to its default value. The default value
  for the velocity is (0.0, 0.0, 0.0), in other words, the listener
  is not moving. The default value for the dopplerFactor is 0.0, in
  other words, Doppler effect is disabled.
*/

void
SoListenerDopplerElement::init(SoState * state)
{
  inherited::init(state);
  this->dopplerVelocity = SbVec3f(0.0f, 0.0f, 0.0f);
  this->dopplerFactor = 0.0f;
}

/*!
  Sets the current listener's Doppler velocity.
*/

void
SoListenerDopplerElement::setDopplerVelocity(SoState * const state,
                                             SoNode * const COIN_UNUSED_ARG(node),
                                             const SbVec3f & velocity)
{
  SoListenerDopplerElement * elem =
    coin_safe_cast<SoListenerDopplerElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );

  if (elem) {
    elem->dopplerVelocity = velocity;
  }
}

/*!
  Sets the current listener's Doppler factor.
*/

void
SoListenerDopplerElement::setDopplerFactor(SoState * const state,
                                             SoNode * const COIN_UNUSED_ARG(node),
                                             float factor)
{
  SoListenerDopplerElement * elem =
    coin_safe_cast<SoListenerDopplerElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->dopplerFactor = factor;
  }
}

//! Returns the current listener's Doppler velocity

const SbVec3f &
SoListenerDopplerElement::getDopplerVelocity(SoState * const state)
{
  const SoListenerDopplerElement * elem =
    coin_assert_cast<const SoListenerDopplerElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return elem->dopplerVelocity;
}

//! Returns the current listener's Doppler factor

float
SoListenerDopplerElement::getDopplerFactor(SoState * const state)
{
  const SoListenerDopplerElement * elem =
    coin_assert_cast<const SoListenerDopplerElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return elem->dopplerFactor;
}

//! Prints the contents of the element (unimplemented)

void
SoListenerDopplerElement::print(FILE * /* file */) const
{
}
