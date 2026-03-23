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
  \class SoPolygonOffsetElement Inventor/elements/SoPolygonOffsetElement.h
  \brief The SoPolygonOffsetElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoPolygonOffsetElement.h>

#include "SbBasicP.h"

#include <cassert>

/*!
  \fn SoPolygonOffsetElement::Style

  FIXME: write doc.
*/

/*!
  \fn SoPolygonOffsetElement::style

  FIXME: write doc.
*/

/*!
  \fn SoPolygonOffsetElement::active

  FIXME: write doc.
*/

/*!
  \fn SoPolygonOffsetElement::offsetfactor

  FIXME: write doc.
*/

/*!
  \fn SoPolygonOffsetElement::offsetunits

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoPolygonOffsetElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoPolygonOffsetElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoPolygonOffsetElement, inherited);
}

/*!
  Destructor.
*/

SoPolygonOffsetElement::~SoPolygonOffsetElement(void)
{
}

/*!
  Initializes the element to its default values. The default
  value for SoPolygonOffsetElement::offsetfactor is 0.0,
  for SoPolygonOffsetElement::offsetunits is 0.0, for
  SoPolygonOffsetElement::style is SoPolygonOffsetElement::FILLED,
  and for SoPolygonOffsetElement::active is on.
*/

void
SoPolygonOffsetElement::init(SoState * state)
{
  inherited::init(state);
  SoPolygonOffsetElement::getDefault(this->offsetfactor,
                                     this->offsetunits,
                                     this->style,
                                     this->active);
}

//! FIXME: write doc.

void
SoPolygonOffsetElement::set(SoState * state, SoNode * node,
                            float factor, float units, Style styles, SbBool on)
{
  SoPolygonOffsetElement * elem = coin_safe_cast<SoPolygonOffsetElement *>
    (
     SoReplacedElement::getElement(state, classStackIndex, node)
     );
  if (elem) {
    elem->setElt(factor, units, styles, on);
  }
}

//! FIXME: write doc.

void
SoPolygonOffsetElement::get(SoState * state, float & factor, float & units,
                            Style & styles, SbBool & on)
{
  const SoPolygonOffsetElement * elem =
    coin_assert_cast<const SoPolygonOffsetElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );

  factor = elem->offsetfactor;
  units = elem->offsetunits;
  styles = elem->style;
  on = elem->active;
}

//! FIXME: write doc.

void
SoPolygonOffsetElement::setElt(float factor, float units,
                               Style styles, SbBool on)
{
  this->offsetfactor = factor;
  this->offsetunits = units;
  this->style = styles;
  this->active = on;
}

//! FIXME: write doc.

void
SoPolygonOffsetElement::getDefault(float & factor, float & units,
                                   Style & styles, SbBool & on)
{
  factor = 0.0f;
  units = 0.0f;
  styles = SoPolygonOffsetElement::FILLED;
  on = FALSE;
}
