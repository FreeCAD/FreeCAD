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
  \class SoShininessElement Inventor/elements/SoShininessElement.h
  \brief The SoShininessElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.

  This element is part of the SoLazyElement in some other OI implementations.
*/

#include <Inventor/elements/SoShininessElement.h>

#include "coindefs.h"
#include "SbBasicP.h"

#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/errors/SoDebugError.h>

#include <cassert>

SO_ELEMENT_SOURCE(SoShininessElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoShininessElement::initClass()
{
  SO_ELEMENT_INIT_CLASS(SoShininessElement, inherited);
}

/*!
  Destructor.
*/

SoShininessElement::~SoShininessElement()
{
}

// Doc from superclass

void
SoShininessElement::init(SoState * stateptr)
{
  inherited::init(state);
  this->state = stateptr;
}

//! FIXME: write doc.

void
SoShininessElement::set(SoState * const state, SoNode * const COIN_UNUSED_ARG(node),
                           const int32_t numvalues,
                           const float * const values)
{
  SoLazyElement::setShininess(state, values[0]);
#if COIN_DEBUG
  if (numvalues > 1) {
    SoDebugError::postWarning("SoShininessElement::set",
                              "Multiple shininess values not supported. "
                              "All values except the first will be ignored.");
  }
#endif // COIN_DEBIG

}

//! FIXME: write doc.
int32_t
SoShininessElement::getNum(void) const
{
  return 1;
}

//! FIXME: write doc.

float
SoShininessElement::get(const int index) const
{
  assert(index == 0);
  return SoLazyElement::getShininess(this->state);
}

/*!
  Returns a pointer to the shininess values. This method is not part of the OIV API.
*/
const float *
SoShininessElement::getArrayPtr(void) const
{
  const_cast<SoShininessElement *>(this)->dummyvalue =
    SoLazyElement::getShininess(this->state);

  return &this->dummyvalue;
}

//! FIXME: write doc.

const SoShininessElement *
SoShininessElement::getInstance(SoState *state)
{
  return coin_assert_cast<const SoShininessElement *>
    (
     state->getElementNoPush(classStackIndex)
     );
}
