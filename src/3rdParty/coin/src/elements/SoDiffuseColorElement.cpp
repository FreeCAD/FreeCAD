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
  \class SoDiffuseColorElement Inventor/elements/SoDiffuseColorElement.h
  \brief The SoDiffuseColorElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoDiffuseColorElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/SbColor.h>
#include <Inventor/errors/SoDebugError.h>
#include <cassert>
#include <cstdlib>

#include "SbBasicP.h"

SO_ELEMENT_SOURCE(SoDiffuseColorElement);

/*!
  \fn static SoType SoDiffuseColorElement::getClassTypeId(void)

  This static method returns the class type.
*/

/*!
  \fn static int SoDiffuseColorElement::getClassStackIndex(void)

  This static method returns the state stack index for the class.
*/

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoDiffuseColorElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoDiffuseColorElement, inherited);
}

//! FIXME: write doc.

void
SoDiffuseColorElement::init(SoState * stateptr)
{
  inherited::init(stateptr);
  this->state = stateptr;
}

/*!
  Destructor.
*/

SoDiffuseColorElement::~SoDiffuseColorElement()
{
}

//! FIXME: write doc.

void
SoDiffuseColorElement::set(SoState * const state, SoNode * const node,
                           const int32_t numcolors,
                           const SbColor * const colors)
{
  SoDiffuseColorElement * elem = const_cast<SoDiffuseColorElement *>
    (
     SoDiffuseColorElement::getInstance(state)
     );

  SoLazyElement::setDiffuse(state, node, numcolors, colors, &elem->colorpacker);
}

//! FIXME: write doc.

void
SoDiffuseColorElement::set(SoState * const state, SoNode * const node,
                           const int32_t numcolors,
                           const uint32_t * const colors,
                           const SbBool packedtransparency)
{
  SoLazyElement::setPacked(state, node, numcolors, colors, packedtransparency);
}

SbBool
SoDiffuseColorElement::isPacked() const
{
  SoLazyElement * lazy = SoLazyElement::getInstance(this->state);
  return lazy->isPacked();
}

//! FIXME: write doc.

int32_t
SoDiffuseColorElement::getNum(void) const
{
  SoLazyElement * lazy = SoLazyElement::getInstance(this->state);
  return lazy->getNumDiffuse();
}

/*!
  Returns the color array. Don't use this unless
  SoDiffuseColorElement::isPacked() returns \c FALSE.

  This method is not part of the original SGI Open Inventor v2.1 API.

  \since Coin 1.0
*/
const SbColor *
SoDiffuseColorElement::getColorArrayPtr(void) const
{
  SoLazyElement * lazy = SoLazyElement::getInstance(this->state);

#if COIN_DEBUG
  if (lazy->isPacked()) {
    SoDebugError::postWarning("SoDiffuseColorElement::getColorArrayPtr",
                              "colors are packed -- use getPackedArrayPtr() "
                              "instead");
  }
#endif // COIN_DEBUG
  return lazy->getDiffusePointer();
}

/*!
  Returns the packed color array. Don't use this unless
  SoDiffuseColorElement::isPacked() returns \c TRUE.

  This method is not part of the original SGI Open Inventor v2.1 API.

  \since Coin 1.0
*/
const uint32_t *
SoDiffuseColorElement::getPackedArrayPtr(void) const
{
  SoLazyElement * lazy = SoLazyElement::getInstance(this->state);

#if COIN_DEBUG
  if (!lazy->isPacked()) {
    SoDebugError::postWarning("SoDiffuseColorElement::getPackedArrayPtr",
                              "colors are *not* packed -- use "
                              "getColorArrayPtr() instead");
  }
#endif // COIN_DEBUG
  return lazy->getPackedPointer();
}

//! FIXME: write doc.

const SbColor &
SoDiffuseColorElement::get(const int index) const
{
  return SoLazyElement::getDiffuse(this->state, index);
}

SbBool
SoDiffuseColorElement::hasPackedTransparency(void) const
{
  return SoLazyElement::getInstance(this->state)->isTransparent();
}

//! FIXME: write doc.

const SoDiffuseColorElement *
SoDiffuseColorElement::getInstance(SoState *state)
{
  return coin_assert_cast<const SoDiffuseColorElement *>
    (
    state->getElementNoPush(classStackIndex)
    );
}
