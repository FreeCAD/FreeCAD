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
  \class SoLightAttenuationElement Inventor/elements/SoLightAttenuationElement.h
  \brief The SoLightAttenuationElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include "SbBasicP.h"
#include "tidbitsp.h"

#include <Inventor/elements/SoLightAttenuationElement.h>

#include <cassert>
#include <cstdlib>

// Dynamically allocated to avoid problems on systems which doesn't
// handle static constructors.
static SbVec3f * defaultattenuation = NULL;

extern "C" {

static void
SoLightAttenuationElement_cleanup_func(void)
{
  delete defaultattenuation;
}

} // extern "C"

/*!
  \fn SoLightAttenuationElement::lightAttenuation

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoLightAttenuationElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoLightAttenuationElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoLightAttenuationElement, inherited);
  defaultattenuation = new SbVec3f;
  defaultattenuation->setValue(0.0f, 0.0f, 1.0f);
  coin_atexit(SoLightAttenuationElement_cleanup_func, CC_ATEXIT_NORMAL);
}

/*!
  Destructor.
*/

SoLightAttenuationElement::~SoLightAttenuationElement(void)
{
}

//! FIXME: write doc.

void
SoLightAttenuationElement::set(SoState * const state,
                               SoNode * const node,
                               const SbVec3f & lightAttenuation)
{
  SoLightAttenuationElement * element =
    coin_safe_cast<SoLightAttenuationElement *>
    (
     SoReplacedElement::getElement(state, classStackIndex, node)
     );
  if (element) {
    element->lightAttenuation = lightAttenuation;
  }
}

//! FIXME: write doc.

const SbVec3f &
SoLightAttenuationElement::get(SoState * const state)
{
  const SoLightAttenuationElement * element = coin_assert_cast<const SoLightAttenuationElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return element->lightAttenuation;
}

//! FIXME: write doc.

SbBool
SoLightAttenuationElement::matches(const SoElement * element) const
{
  if (this->lightAttenuation !=
      coin_assert_cast<const SoLightAttenuationElement *>(element)->lightAttenuation)
    return FALSE;
  return TRUE;
}

//! FIXME: write doc.

SoElement *
SoLightAttenuationElement::copyMatchInfo() const
{
  SoLightAttenuationElement * element = static_cast<SoLightAttenuationElement *>
    (SoLightAttenuationElement::getClassTypeId().createInstance());
  element->lightAttenuation = this->lightAttenuation;
  return element;
}

//! FIXME: write doc.

void
SoLightAttenuationElement::print(FILE * file) const
{
  fprintf(file, "SoLightAttenuationElement[%p]: attenuation = ", this);
  fprintf(file, "<%f, %f, %f>\n",
           this->lightAttenuation[0],
           this->lightAttenuation[1],
           this->lightAttenuation[2]);
}

//! FIXME: write doc.

void
SoLightAttenuationElement::init(SoState * state)
{
  inherited::init(state);
  this->lightAttenuation = SoLightAttenuationElement::getDefault();
}

//! FIXME: write doc.

const SbVec3f &
SoLightAttenuationElement::getDefault(void)
{
  return *defaultattenuation;
}
