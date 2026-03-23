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
  \class SoTextureOverrideElement Inventor/elements/SoTextureOverrideElement.h
  \brief The SoTextureOverrideElement makes it possible to override texture elements.

  \ingroup coin_elements
*/


#include <Inventor/elements/SoTextureOverrideElement.h>

#include "coindefs.h"
#include "SbBasicP.h"

#include <cassert>

SO_ELEMENT_SOURCE(SoTextureOverrideElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoTextureOverrideElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoTextureOverrideElement, inherited);
}

/*!
  Destructor.
*/

SoTextureOverrideElement::~SoTextureOverrideElement(void)
{
}

//!

SbBool
SoTextureOverrideElement::matches(const SoElement *element) const
{
  return coin_assert_cast<const SoTextureOverrideElement *>(element)->flags == this->flags;
}

//!

SoElement *
SoTextureOverrideElement::copyMatchInfo() const
{
  SoTextureOverrideElement * elem =
    static_cast<SoTextureOverrideElement *>(this->getTypeId().createInstance());
  elem->flags = this->flags;
  return elem;
}

//!

void
SoTextureOverrideElement::init(SoState * COIN_UNUSED_ARG(state))
{
  this->flags = 0;
}

//!

void
SoTextureOverrideElement::push(SoState *state)
{
  inherited::push(state);
  const SoTextureOverrideElement * prev =
    coin_assert_cast<SoTextureOverrideElement *>
    (
     this->getNextInStack()
     );
  this->flags = prev->flags;
}

//!

SbBool
SoTextureOverrideElement::getQualityOverride(SoState *state)
{
  const SoTextureOverrideElement * const element =
    coin_assert_cast<const SoTextureOverrideElement *>
    (
     getConstElement(state, classStackIndex)
     );
  return (element->flags & TEXTURE_QUALITY) != 0;
}

//!

SbBool
SoTextureOverrideElement::getImageOverride(SoState *state)
{
  const SoTextureOverrideElement * const element =
    coin_assert_cast<const SoTextureOverrideElement *>
    (
     getConstElement(state, classStackIndex)
     );
  return (element->flags & TEXTURE_IMAGE) != 0;
}

SbBool
SoTextureOverrideElement::getBumpMapOverride(SoState *state)
{
  const SoTextureOverrideElement * const element =
    coin_assert_cast<const SoTextureOverrideElement *>
    (
     getConstElement(state, classStackIndex)
     );
  return (element->flags & BUMP_MAP) != 0;
}

//!

void
SoTextureOverrideElement::setQualityOverride(SoState *state, const SbBool value)
{
  SoTextureOverrideElement * const element =
    coin_safe_cast<SoTextureOverrideElement *>
    (
     getElement(state, classStackIndex)
     );
  if (element) {
    if (value)
      element->flags |= TEXTURE_QUALITY;
    else
      element->flags &= ~TEXTURE_QUALITY;
  }
}

//!

void
SoTextureOverrideElement::setImageOverride(SoState *state, const SbBool value)
{
  SoTextureOverrideElement * const element =
    coin_safe_cast<SoTextureOverrideElement *>(getElement(state, classStackIndex));
  if (element) {
    if (value)
      element->flags |= TEXTURE_IMAGE;
    else
      element->flags &= ~TEXTURE_IMAGE;
  }
}

//!

void
SoTextureOverrideElement::setBumpMapOverride(SoState *state, const SbBool value)
{
  SoTextureOverrideElement * const element =
    coin_safe_cast<SoTextureOverrideElement *>(getElement(state, classStackIndex));
  if (element) {
    if (value)
      element->flags |= BUMP_MAP;
    else
      element->flags &= ~BUMP_MAP;
  }
}

//!
void
SoTextureOverrideElement::print(FILE *fp) const
{
  fprintf(fp, "SoTextureOverrideElement::flags: 0x%x\n", this->flags);
}
