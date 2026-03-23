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
  \class SoDrawStyleElement Inventor/elements/SoDrawStyleElement.h
  \brief The SoDrawStyleElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoDrawStyleElement.h>

#include <Inventor/elements/SoShapeStyleElement.h>


#include <cassert>

/*!
  \fn SoDrawStyleElement::Style

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoDrawStyleElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoDrawStyleElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoDrawStyleElement, inherited);
}

/*!
  Destructor.
*/

SoDrawStyleElement::~SoDrawStyleElement(void)
{
}

//! FIXME: write doc.

void
SoDrawStyleElement::init(SoState * state)
{
  inherited::init(state);
  this->data = getDefault();
}

//! FIXME: write doc.

void
SoDrawStyleElement::set(SoState * const state,
                        SoNode * const node,
                        const Style style)
{
  SoInt32Element::set(classStackIndex, state, node, static_cast<int32_t>(style));
  SoShapeStyleElement::setDrawStyle(state, static_cast<int32_t>(style));
}

//! FIXME: write doc.

//$ EXPORT INLINE
void
SoDrawStyleElement::set(SoState * const state, const Style style)
{
  set(state, NULL, style);
}

//! FIXME: write doc.

//$ EXPORT INLINE
SoDrawStyleElement::Style
SoDrawStyleElement::get(SoState * const state)
{
  return static_cast<Style>(inherited::get(classStackIndex, state));
}

//! FIXME: write doc.

//$ EXPORT INLINE
SoDrawStyleElement::Style
SoDrawStyleElement::getDefault()
{
  return FILLED;
}
