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
  \class SoListenerPositionElement Inventor/elements/SoListenerPositionElement.h
  \brief The SoListenerPositionElement holds the position of the current listener.

  \ingroup coin_elements

  This position is set by SoListener nodes and SoCamera Nodes during audio
  rendering. When a SoListener is visited by the SoAudioRenderAction,
  it will add a new SoListenerPositionElement to the state, holding its
  position and with the setbylistener flag set. When a SoCamera is visited
  by SoAudioRenderAction, it will add a new SoListenerPositionElement only
  if there are no previous elements with the setbylistener flag set.

  The SoListenerPositionElement is used when the SoVRMLSound nodes render
  themselves.

  \COIN_CLASS_EXTENSION

  \since Coin 2.0
*/

#include <Inventor/elements/SoListenerPositionElement.h>

#include "coindefs.h"
#include "SbBasicP.h"

#include <Inventor/nodes/SoNode.h>

/*!
  \fn SoListenerPositionElement::position

  The position of the listener in world space. Can be set by the
  SoListener class or the SoCamera class.
*/

SO_ELEMENT_SOURCE(SoListenerPositionElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoListenerPositionElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoListenerPositionElement, inherited);
}

/*!
  Destructor.
*/

SoListenerPositionElement::~SoListenerPositionElement(void)
{
}

/*!
  Initializes the element to its default value. The default
  value for the position is (0.0, 0.0, 0.0) and the
  default value for the setByListener flag is FALSE.
*/

void
SoListenerPositionElement::init(SoState * state)
{
  inherited::init(state);
  this->position = SbVec3f(0.0f, 0.0f, 0.0f);
  this->setbylistener = FALSE;
}

/*!
  Sets the current listener position, and indicates if it was set
  by a SoListener node or a SoCamera node.
*/

void
SoListenerPositionElement::set(SoState * const state,
                               SoNode * const COIN_UNUSED_ARG(node),
                               const SbVec3f & position,
                               SbBool setbylistener)
{
  SoListenerPositionElement * elem =
    coin_safe_cast<SoListenerPositionElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->position = position;
    elem->setbylistener = setbylistener;
  }
}

//! Returns the current listener position

const SbVec3f &
SoListenerPositionElement::get(SoState * const state)
{
  const SoListenerPositionElement * elem =
    coin_assert_cast<const SoListenerPositionElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return elem->position;
}

/*!
  Returns TRUE if the position was set by a SoListener node,
  and FALSE if it was set by a SoCamera node
*/

SbBool
SoListenerPositionElement::isSetByListener(SoState * const state)
{
  const SoListenerPositionElement * elem =
    coin_assert_cast<const SoListenerPositionElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return elem->setbylistener;
}

//! Prints contents of element (not implemented)

void
SoListenerPositionElement::print(FILE * /* file */) const
{
}
