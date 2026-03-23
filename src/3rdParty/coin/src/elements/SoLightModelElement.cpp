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
  \class SoLightModelElement Inventor/elements/SoLightModelElement.h
  \brief The SoLightModelElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include "coindefs.h"
#include "SbBasicP.h"

#include <Inventor/elements/SoLightModelElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <cassert>

/*!
  \fn SoLightModelElement::Model

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoLightModelElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoLightModelElement::initClass()
{
  SO_ELEMENT_INIT_CLASS(SoLightModelElement, inherited);
}

/*!
  Destructor.
*/

SoLightModelElement::~SoLightModelElement()
{
}

// Doc from superclass

void
SoLightModelElement::init(SoState * /* state */)
{
}

//! FIXME: write doc.

void
SoLightModelElement::set(SoState * const state, const Model model)
{
  SoLazyElement::setLightModel(state, static_cast<int32_t>(model));
}

//! FIXME: write doc.

void
SoLightModelElement::set(SoState * const state, SoNode * const COIN_UNUSED_ARG(node),
                         const Model model)
{
  SoLazyElement::setLightModel(state, static_cast<int32_t>(model));
}

//! FIXME: write doc.

SoLightModelElement::Model
SoLightModelElement::get(SoState * const state)
{
  return static_cast<SoLightModelElement::Model>(SoLazyElement::getLightModel(state));
}

//! FIXME: write doc.

SoLightModelElement::Model
SoLightModelElement::getDefault()
{
  return static_cast<SoLightModelElement::Model>(SoLazyElement::getDefaultLightModel());
}

//! FIXME: write doc

const SoLightModelElement *
SoLightModelElement::getInstance(SoState *state)
{
  //FIXME: Can this function or any of the similar functions ever
  //return NULL? BFG 20080916
  return coin_assert_cast<const SoLightModelElement *>
    (
     state->getElementNoPush(classStackIndex)
     );
}
