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
  \class SoShadowStyleElement SoShadowStyleElement.h Inventor/annex/FXViz/elements/SoShadowStyleElement.h
  \brief The SoShadowStyleElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.

  \since Coin 2.5
*/

#include <Inventor/annex/FXViz/elements/SoShadowStyleElement.h>


#include <cassert>

SO_ELEMENT_SOURCE(SoShadowStyleElement);

/*!
  \copydetails SoElement::initClass(void)
*/
void
SoShadowStyleElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoShadowStyleElement, inherited);
}

/*!
  Destructor.
*/

SoShadowStyleElement::~SoShadowStyleElement(void)
{
}

//! FIXME: write doc.

void
SoShadowStyleElement::set(SoState * const state,
                          SoNode * const node,
                          const int style)
{
  SoInt32Element::set(classStackIndex, state, node, style);
}

/*!
  Initializes the element to its default value. The default
  value is SoShadowStyleElement::CASTS_SHADOW_AND_SHADOWED.
*/

void
SoShadowStyleElement::init(SoState * state)
{
  inherited::init(state);
  this->data = getDefault();
}

//! FIXME: write doc.

void
SoShadowStyleElement::set(SoState * const state, const int style)
{
  SoShadowStyleElement::set(state, NULL, style);
}

//! FIXME: write doc.

int
SoShadowStyleElement::get(SoState * const state)
{
  return SoInt32Element::get(classStackIndex, state);
}

//! FIXME: write doc.

int
SoShadowStyleElement::getDefault(void)
{
  return CASTS_SHADOW_AND_SHADOWED;
}


#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  BOOST_CHECK_MESSAGE(SoShadowStyleElement::getClassStackIndex() != -1,
                      "missing class initialization");
}

#endif // COIN_TEST_SUITE
