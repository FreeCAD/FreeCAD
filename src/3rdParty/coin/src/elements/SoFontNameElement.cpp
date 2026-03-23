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
  \class SoFontNameElement Inventor/elements/SoFontNameElement.h
  \brief The SoFontNameElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include "tidbitsp.h"
#include "SbBasicP.h"

#include <Inventor/elements/SoFontNameElement.h>

#include <cassert>



SbName * SoFontNameElement::defaultfontname = NULL;

/*!
  \fn SoFontNameElement::fontName

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoFontNameElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoFontNameElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoFontNameElement, inherited);

  SoFontNameElement::defaultfontname = new SbName("defaultFont");

  coin_atexit(reinterpret_cast<coin_atexit_f *>(SoFontNameElement::clean), CC_ATEXIT_NORMAL);
}

void
SoFontNameElement::clean(void)
{
  delete SoFontNameElement::defaultfontname;
}

/*!
  Destructor.
*/

SoFontNameElement::~SoFontNameElement()
{
}

//! FIXME: write doc.

void
SoFontNameElement::set(SoState * const state,
                       SoNode * const node,
                       const SbName fontName)
{
  SoFontNameElement * element =
    coin_safe_cast<SoFontNameElement *>
    (
     SoReplacedElement::getElement(state, classStackIndex, node)
     );

  if (element) {
    element->fontName = fontName;
  }
}

//! FIXME: write doc.

const SbName &
SoFontNameElement::get(SoState * const state)
{
  const SoFontNameElement * element = coin_assert_cast<const SoFontNameElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return element->fontName;
}

//! FIXME: write doc.

SbBool
SoFontNameElement::matches(const SoElement * element) const
{
  if (this == element)
    return TRUE;
  if (element->getTypeId() != SoFontNameElement::getClassTypeId())
    return FALSE;
  if (this->fontName != coin_assert_cast<const SoFontNameElement *>(element)->fontName)
    return FALSE;
  return TRUE;
}

//! FIXME: write doc.

SoElement *
SoFontNameElement::copyMatchInfo(void) const
{
  SoFontNameElement * element = static_cast<SoFontNameElement *>
    (
     SoFontNameElement::getClassTypeId().createInstance()
     );
  element->fontName = this->fontName;
  element->nodeId = this->nodeId;
  return element;
}

//! FIXME: write doc.

void
SoFontNameElement::print(FILE * file) const
{
  fprintf(file, "SoFontNameElement[%p]: font = %s\n", this,
           this->fontName.getString());
}

// Doc from superclass.

void
SoFontNameElement::init(SoState * state)
{
  inherited::init(state);
  this->fontName = *SoFontNameElement::defaultfontname;
}

//! FIXME: write doc.

SbName
SoFontNameElement::getDefault(void)
{
  return *SoFontNameElement::defaultfontname;
}
