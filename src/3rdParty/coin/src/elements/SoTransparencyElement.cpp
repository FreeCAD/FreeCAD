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
  \class SoTransparencyElement Inventor/elements/SoTransparencyElement.h
  \brief The SoTransparencyElement is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoTransparencyElement.h>

#include "SbBasicP.h"

#include <Inventor/elements/SoLazyElement.h>
#include <cassert>


SO_ELEMENT_SOURCE(SoTransparencyElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoTransparencyElement::initClass()
{
  SO_ELEMENT_INIT_CLASS(SoTransparencyElement, inherited);
}

/*!
  Destructor.
*/

SoTransparencyElement::~SoTransparencyElement()
{
}

// Doc from superclass

void
SoTransparencyElement::init(SoState * stateptr)
{
  inherited::init(state);
  this->state = stateptr;
}

//! FIXME: write doc.

void
SoTransparencyElement::set(SoState * const state, SoNode * const node,
                           const int32_t numvalues,
                           const float * const values)
{
  SoTransparencyElement * elem = const_cast<SoTransparencyElement *>
    (
     SoTransparencyElement::getInstance(state)
     );

  SoLazyElement::setTransparency(state, node, numvalues, values, &elem->colorpacker);
}

//! FIXME: write doc.

//$ EXPORT INLINE
int32_t
SoTransparencyElement::getNum(void) const
{
  SoLazyElement * lazy = SoLazyElement::getInstance(this->state);
  return lazy->getNumTransparencies();
}

//! FIXME: write doc.

float
SoTransparencyElement::get(const int index) const
{
  return SoLazyElement::getTransparency(this->state, index);
}

/*!
  Returns a pointer to the transparency values. This method is not part of the OIV API.
*/
const float *
SoTransparencyElement::getArrayPtr(void) const
{
  SoLazyElement * lazy = SoLazyElement::getInstance(this->state);
  return lazy->getTransparencyPointer();
}

//! FIXME: write doc.

const SoTransparencyElement *
SoTransparencyElement::getInstance(SoState *state)
{
  return coin_assert_cast<const SoTransparencyElement *>
    (
     state->getElementNoPush(classStackIndex)
     );
}
