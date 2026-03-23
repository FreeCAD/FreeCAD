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
  \class SoViewportRegionElement Inventor/elements/SoViewportRegionElement.h
  \brief The SoViewportRegionElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoViewportRegionElement.h>

#include "SbBasicP.h"

#include <cassert>

/*!
  \fn SoViewportRegionElement::viewportRegion

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoViewportRegionElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoViewportRegionElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoViewportRegionElement, inherited);
}

/*!
  Destructor.
*/

SoViewportRegionElement::~SoViewportRegionElement(void)
{
}

// Doc from superclass
void
SoViewportRegionElement::init(SoState * state)
{
  inherited::init(state);
}

//! FIXME: write doc.

SbBool
SoViewportRegionElement::matches(const SoElement * element) const
{
  return this->viewportRegion ==
    coin_assert_cast<const SoViewportRegionElement *>(element)->viewportRegion;
}

//! FIXME: write doc.

SoElement *
SoViewportRegionElement::copyMatchInfo() const
{
  SoViewportRegionElement * elem = static_cast<SoViewportRegionElement *>
    (
     getTypeId().createInstance()
     );
  elem->viewportRegion = this->viewportRegion;
  return elem;
}

//! FIXME: write doc.

void
SoViewportRegionElement::set(SoState * const state,
                             const SbViewportRegion &viewportRegion)
{
  SoViewportRegionElement * elem = coin_safe_cast<SoViewportRegionElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->setElt(viewportRegion);
  }
}

//! FIXME: write doc.

const SbViewportRegion &
SoViewportRegionElement::get(SoState * const state)
{
  const SoViewportRegionElement * elem =
    coin_assert_cast<const SoViewportRegionElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return elem->viewportRegion;
}

//! FIXME: write doc.

void
SoViewportRegionElement::setElt(const SbViewportRegion & viewportRegionarg)
{
  this->viewportRegion = viewportRegionarg;
}

//! FIXME: write doc.

void
SoViewportRegionElement::print(FILE * file) const
{
  fprintf(file, "SoViewportRegionElement[%p]\n", this);
}
