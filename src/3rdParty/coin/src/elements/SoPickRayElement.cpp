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
  \class SoPickRayElement Inventor/elements/SoPickRayElement.h
  \brief The SoPickRayElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoPickRayElement.h>

#include "SbBasicP.h"

/*!
  \fn SoPickRayElement::volume

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoPickRayElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoPickRayElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoPickRayElement, inherited);
}

/*!
  Destructor.
*/

SoPickRayElement::~SoPickRayElement(void)
{
}

/*!
  Initializes the element to its default value.
*/

void
SoPickRayElement::init(SoState * state)
{
    inherited::init(state);
}

//! FIXME: write doc.

SbBool
SoPickRayElement::matches(const SoElement * /* element */) const
{
  // should always return false; not part of cache consideration
  return FALSE;
}

//! FIXME: write doc.

SoElement *
SoPickRayElement::copyMatchInfo() const
{
  SoPickRayElement * element =
    static_cast<SoPickRayElement *>(getTypeId().createInstance());
  // no use copying any data. matches() will always return false.
  return element;
}

//! FIXME: write doc.

void
SoPickRayElement::set(SoState * const state,
                      const SbViewVolume & volume)
{
  SoPickRayElement * elem = coin_safe_cast<SoPickRayElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->volume = volume;
  }
}

//! FIXME: write doc.

const SbViewVolume &
SoPickRayElement::get(SoState * const state)
{
  const SoPickRayElement * elem = coin_assert_cast<const SoPickRayElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return elem->volume;
}

//! FIXME: write doc.
