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
  \class SoTextureScalePolicyElement elements/SoTextureScalePolicyElement.h
  \brief The SoTextureScalePolicyElement class is yet to be documented.

  \ingroup coin_elements

  This is currently an internal Coin element. The header file is not
  installed, and the API for this element might change without notice.
*/

#include "elements/SoTextureScalePolicyElement.h"

#include <cassert>

/*!
  \fn SoTextureScalePolicyElement::Policy

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoTextureScalePolicyElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoTextureScalePolicyElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoTextureScalePolicyElement, inherited);
}

/*!
  Destructor.
*/

SoTextureScalePolicyElement::~SoTextureScalePolicyElement(void)
{
}

//! FIXME: write doc.

void
SoTextureScalePolicyElement::set(SoState * const state,
                                 SoNode * const node,
                                 const Policy policy)
{
  inherited::set(classStackIndex, state, node, static_cast<int>(policy));
}

/*!
  Initializes the element to its default value. The default
  value is SoTextureScalePolicyElement::USE_TEXTURE_QUALITY.
*/

void
SoTextureScalePolicyElement::init(SoState * state)
{
  inherited::init(state);
  this->data = getDefault();
}

//! FIXME: write doc.
void
SoTextureScalePolicyElement::set(SoState * const state, const Policy policy)
{
  SoTextureScalePolicyElement::set(state, NULL, policy);
}

//! FIXME: write doc.
SoTextureScalePolicyElement::Policy
SoTextureScalePolicyElement::get(SoState * const state)
{
  return static_cast<Policy>(SoInt32Element::get(classStackIndex, state));
}

//! FIXME: write doc.
SoTextureScalePolicyElement::Policy
SoTextureScalePolicyElement::getDefault(void)
{
  return USE_TEXTURE_QUALITY;
}
