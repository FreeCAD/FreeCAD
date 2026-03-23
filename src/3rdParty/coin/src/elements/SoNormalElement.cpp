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
  \class SoNormalElement Inventor/elements/SoNormalElement.h
  \brief The SoNormalElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include "SbBasicP.h"

#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <cassert>

/*!
  \fn SoNormalElement::numNormals
  FIXME: write doc.
*/

/*!
  \fn SoNormalElement::normals
  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoNormalElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoNormalElement::initClass()
{
  SO_ELEMENT_INIT_CLASS(SoNormalElement, inherited);
}

/*!
  Destructor.
*/

SoNormalElement::~SoNormalElement()
{
}

//! FIXME: write doc.

void
SoNormalElement::set(SoState * const state,
                     SoNode * const node,
                     const int32_t numNormals,
                     const SbVec3f * const normals,
                     const SbBool unitLength)
{
  if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
    SoGLVBOElement::setNormalVBO(state, NULL);
  }
  SoNormalElement * elem = coin_safe_cast<SoNormalElement *>
    (
     SoReplacedElement::getElement(state, classStackIndex, node)
     );
  if (elem) {
    elem->normals = normals;
    elem->numNormals = numNormals;
    elem->unitLength = unitLength;
  }
}

/*!
  Initializes the element to its default value. The default
  values are: for normals NULL, for numNormals 0, and for the
  unitLength flag FALSE.
*/

void
SoNormalElement::init(SoState *state)
{
  inherited::init(state);
  this->normals = NULL;
  this->numNormals = 0;
  this->unitLength = FALSE;
}

//! FIXME: write doc.

const SoNormalElement *
SoNormalElement::getInstance(SoState * const state)
{
  return coin_assert_cast<const SoNormalElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
}

//! FIXME: write doc.

//$ EXPORT INLINE
int32_t
SoNormalElement::getNum(void) const
{
  return this->numNormals;
}

//! FIXME: write doc.

//$ EXPORT INLINE
const SbVec3f &
SoNormalElement::get(const int index) const
{
  assert(index >= 0 && index < getNum());
  return this->normals[index];
}

/*!
  Returns a pointer to the normal array. This method is not part of the OIV API.
*/
const SbVec3f *
SoNormalElement::getArrayPtr(void) const
{
  return this->normals;
}

//$ EXPORT INLINE
SbBool
SoNormalElement::areNormalsUnitLength(void) const
{
  return this->unitLength;
}
