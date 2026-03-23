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
  \class SoEnvironmentElement Inventor/elements/SoEnvironmentElement.h
  \brief The SoEnvironmentElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoEnvironmentElement.h>


#include <cassert>

#include "SbBasicP.h"

/*!
  \fn SoEnvironmentElement::FogType

  FIXME: write doc.
*/

/*!
  \fn SoEnvironmentElement::ambientIntensity

  FIXME: write doc.
*/

/*!
  \fn SoEnvironmentElement::ambientColor

  FIXME: write doc.
*/

/*!
  \fn SoEnvironmentElement::attenuation

  FIXME: write doc.
*/

/*!
  \fn SoEnvironmentElement::fogType

  FIXME: write doc.
*/

/*!
  \fn SoEnvironmentElement::fogColor

  FIXME: write doc.
*/

/*!
  \fn SoEnvironmentElement::fogVisibility

  FIXME: write doc.
*/

/*!
  \fn SoEnvironmentElement::fogStart

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoEnvironmentElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoEnvironmentElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoEnvironmentElement, inherited);
}

/*!
  Destructor.
*/

SoEnvironmentElement::~SoEnvironmentElement(void)
{
}

//! FIXME: write doc.

void
SoEnvironmentElement::set(SoState * const state,
                          SoNode * const node,
                          const float ambientIntensity,
                          const SbColor & ambientColor,
                          const SbVec3f & attenuation,
                          const int32_t fogType,
                          const SbColor & fogColor,
                          const float fogVisibility,
                          const float fogStart)
{
  SoEnvironmentElement * element =
    coin_safe_cast<SoEnvironmentElement *>
    (
     SoReplacedElement::getElement(state, classStackIndex, node)
     );
  if (element) {
    element->setElt(state, ambientIntensity, ambientColor, attenuation,
                    fogType, fogColor, fogVisibility, fogStart);
  }
}

//! FIXME: write doc.

void
SoEnvironmentElement::get(SoState * const state,
                          float & ambientIntensity,
                          SbColor & ambientColor,
                          SbVec3f & attenuation,
                          int32_t & fogType,
                          SbColor & fogColor,
                          float & fogVisibility,
                          float & fogStart)
{
  const SoEnvironmentElement * element = coin_assert_cast<const SoEnvironmentElement *>
    (
    SoElement::getConstElement(state, classStackIndex)
    );

  ambientIntensity = element->ambientIntensity;
  ambientColor = element->ambientColor;
  attenuation = element->attenuation;
  fogType = element->fogType;
  fogColor = element->fogColor;
  fogVisibility = element->fogVisibility;
  fogStart = element->fogStart;
}

//! FIXME: write doc.

void
SoEnvironmentElement::getDefault(float & ambientIntensity,
                                 SbColor & ambientColor,
                                 SbVec3f & attenuation,
                                 int32_t & fogType,
                                 SbColor & fogColor,
                                 float & fogVisibility,
                                 float & fogStart)
{
  ambientIntensity = 0.2f;
  ambientColor = SbColor(1.0f, 1.0f, 1.0f);
  attenuation = SbVec3f(0.0f, 0.0f, 1.0f);
  fogType = NONE;
  fogColor = SbColor(1.0f, 1.0f, 1.0f);
  fogVisibility = 0.0f;
  fogStart = 0.0f;
}

//! FIXME: write doc.

float
SoEnvironmentElement::getAmbientIntensity(SoState * const state)
{
  const SoEnvironmentElement * element = coin_assert_cast<const SoEnvironmentElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return element->ambientIntensity;
}

//! FIXME: write doc.

float
SoEnvironmentElement::getFogVisibility(SoState * const state)
{
  const SoEnvironmentElement * element = coin_assert_cast<const SoEnvironmentElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return element->fogVisibility;
}

//! FIXME: write doc.

const SbVec3f &
SoEnvironmentElement::getLightAttenuation(SoState * const state)
{
  const SoEnvironmentElement * element = coin_assert_cast<const SoEnvironmentElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return element->attenuation;
}

//! FIXME: write doc.

const SbColor &
SoEnvironmentElement::getAmbientColor(SoState * const state)
{
  const SoEnvironmentElement * element = coin_assert_cast<const SoEnvironmentElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return element->ambientColor;
}

//! FIXME: write doc.

const SbColor &
SoEnvironmentElement::getFogColor(SoState * const state)
{
  const SoEnvironmentElement * element = coin_assert_cast<const SoEnvironmentElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return element->fogColor;
}

//! FIXME: write doc.

int32_t
SoEnvironmentElement::getFogType(SoState * const state)
{
  const SoEnvironmentElement * element = coin_assert_cast<const SoEnvironmentElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return element->fogType;
}

//! FIXME: write doc.

void
SoEnvironmentElement::print(FILE * file) const
{
  fprintf(file, "SoEnvironmentElement[%p]\n", this);
}

/*!
  Initializes the element to its default value. The defaults
  for the environment settings are:

    SoEnvironmentElement::ambientIntensity = 0.2
    SoEnvironmentElement::ambientColor = SbColor(1.0, 1.0, 1.0)
    SoEnvironmentElement::attenuation = SbVec3f(0.0, 0.0, 1.0)
    SoEnvironmentElement::fogType = SoEnvironmentElement::NONE
    SoEnvironmentElement::fogColor = SbColor(1.0, 1.0, 1.0)
    SoEnvironmentElement::fogVisibility = 0.0
    SoEnvironmentElement::fogStart = 0.0
*/

void
SoEnvironmentElement::init(SoState * state)
{
  inherited::init(state);
  this->getDefault(this->ambientIntensity, this->ambientColor, this->attenuation,
                   fogType, fogColor, fogVisibility, fogStart);
}

//! FIXME: doc
void
SoEnvironmentElement::setElt(SoState * const,
                             const float ambientIntensityarg,
                             const SbColor & ambientColorarg,
                             const SbVec3f & attenuationarg,
                             const int32_t fogTypearg,
                             const SbColor & fogColorarg,
                             const float fogVisibilityarg,
                             const float fogStartarg)
{
  this->ambientIntensity = ambientIntensityarg;
  this->ambientColor = ambientColorarg;
  this->attenuation = attenuationarg;
  this->fogType = fogTypearg;
  this->fogColor = fogColorarg;
  this->fogVisibility = fogVisibilityarg;
  this->fogStart = fogStartarg;
}
