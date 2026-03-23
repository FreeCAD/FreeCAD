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
  \class SoTextOutlineEnabledElement Inventor/elements/SoTextOutlineEnabledElement.h
  \brief The SoTextOutlineEnabledElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.

  \since Coin 1.0
  \since TGS Inventor 2.4
*/


#include <Inventor/elements/SoTextOutlineEnabledElement.h>
#include <cassert>

SO_ELEMENT_SOURCE(SoTextOutlineEnabledElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoTextOutlineEnabledElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoTextOutlineEnabledElement, inherited);
}

/*!
  Destructor.
*/

SoTextOutlineEnabledElement::~SoTextOutlineEnabledElement(void)
{
}

//! FIXME: write doc.

void
SoTextOutlineEnabledElement::set(SoState * const state,
                                 SoNode * const node,
                                 const SbBool enabled)
{
  SoInt32Element::set(classStackIndex, state, node, enabled);
}

/*!
  FIXME: write doc.
*/

void
SoTextOutlineEnabledElement::set(SoState * const state, const SbBool enabled)
{
  set(state, NULL, enabled);
}

/*!
  FIXME: write doc.
*/

SbBool
SoTextOutlineEnabledElement::get(SoState * const state)
{
  return static_cast<SbBool>(SoInt32Element::get(classStackIndex, state));
}

/*!
  FIXME: write doc.
*/

SbBool
SoTextOutlineEnabledElement::getDefault(void)
{
  return FALSE;
}

/*!
  Initializes the element to its default value. The default
  value is FALSE.
*/

void
SoTextOutlineEnabledElement::init(SoState * state)
{
  inherited::init(state);
  this->data = getDefault();
}

//! FIXME: write doc.

void
SoTextOutlineEnabledElement::push(SoState * state)
{
  inherited::push(state);
}

//! FIXME: write doc.

void
SoTextOutlineEnabledElement::pop(SoState * state,
                                 const SoElement * prevTopElement)
{
  inherited::pop(state, prevTopElement);
}
