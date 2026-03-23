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
  \class SoTextureUnitElement Inventor/elements/SoTextureUnitElement.h
  \brief The SoTextureUnitElement class is yet to be documented.

  This element is used for keeping control of multitexturing usage.

  FIXME: write doc.

  \ingroup coin_elements
  \COIN_CLASS_EXTENSION
  \since Coin 2.2
*/

#include <Inventor/elements/SoTextureUnitElement.h>
#include <cassert>

SO_ELEMENT_SOURCE(SoTextureUnitElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoTextureUnitElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoTextureUnitElement, inherited);
}

/*!
  Destructor.
*/

SoTextureUnitElement::~SoTextureUnitElement(void)
{
}

//! FIXME: write doc.

void
SoTextureUnitElement::set(SoState * const state,
                          SoNode * const node,
                          const int unit)
{
  SoInt32Element::set(classStackIndex, state, node, unit);
}

/*!
  Initializes the element to its default value. The default
  value is 0.
*/

void
SoTextureUnitElement::init(SoState * state)
{
  inherited::init(state);
  this->data = 0;
}

//! FIXME: write doc.

int
SoTextureUnitElement::get(SoState * state)
{
  return static_cast<int>(SoInt32Element::get(classStackIndex, state));
}
