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
  \class SoListenerGainElement Inventor/elements/SoListenerGainElement.h
  \brief The SoListenerGainElement class stores the SoListener gain during a scene graph traversal.

  \ingroup coin_elements

  This gain is set by SoListener nodes during audio rendering.
  The SoListenerGainElement is used when the SoVRMLSound nodes render
  themselves.

  \COIN_CLASS_EXTENSION

  \since Coin 2.0
*/

#include <Inventor/elements/SoListenerGainElement.h>


#include <cassert>

SO_ELEMENT_SOURCE(SoListenerGainElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoListenerGainElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoListenerGainElement, inherited);
}

/*!
  Destructor.
*/
SoListenerGainElement::~SoListenerGainElement(void)
{
}

/*!
  Initializes the element to its default value. The default
  value is 1.0.
*/
void
SoListenerGainElement::init(SoState * state)
{
  inherited::init(state);

  this->data = 1.0f;
}

/*!
  Sets the value of this element.
*/
void
SoListenerGainElement::set(SoState * const state, SoNode * const node,
                          const float gain)
{
  SoFloatElement::set(classStackIndex, state, node, gain);
}


/*!
  \overload
*/
void
SoListenerGainElement::set(SoState * const state, const float gain)
{
  SoListenerGainElement::set(state, NULL, gain);
}

/*!
  Returns the element value.
*/
float
SoListenerGainElement::get(SoState * const state)
{
  float val = SoFloatElement::get(classStackIndex, state);
  return val;
}
