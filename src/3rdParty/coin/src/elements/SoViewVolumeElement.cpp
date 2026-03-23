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
  \class SoViewVolumeElement Inventor/elements/SoViewVolumeElement.h
  \brief The SoViewVolumeElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoViewVolumeElement.h>

#include "SbBasicP.h"

#include <cassert>

/*!
  \fn SoViewVolumeElement::viewVolume

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoViewVolumeElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoViewVolumeElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoViewVolumeElement, inherited);
}

/*!
  Destructor.
*/

SoViewVolumeElement::~SoViewVolumeElement(void)
{
}

//! FIXME: write doc.

void
SoViewVolumeElement::set(SoState * const state,
                         SoNode * const node,
                         const SbViewVolume & viewVolume)
{
  SoViewVolumeElement * element = coin_safe_cast<SoViewVolumeElement *>
    (
     SoReplacedElement::getElement(state, classStackIndex, node)
     );
  if (element) {
    element->viewVolume = viewVolume;
  }
}

//! FIXME: write doc.

const SbViewVolume &
SoViewVolumeElement::get(SoState * const state)
{
  const SoViewVolumeElement * element = coin_assert_cast<const SoViewVolumeElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return element->viewVolume;
}

/*!
  Initializes the element to its default value. The default
  value is rectangular box with 
  (left: -1.0, right: 1.0, bottom: -1.0, top: 1.0, near: -1.0, far: 1.0).
*/

void
SoViewVolumeElement::init(SoState * state)
{
  inherited::init(state);
  this->viewVolume.ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
}
