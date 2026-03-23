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
  \class SoLocalBBoxMatrixElement Inventor/elements/SoLocalBBoxMatrixElement.h
  \brief The SoLocalBBoxMatrixElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include "SbBasicP.h"

#include <Inventor/elements/SoLocalBBoxMatrixElement.h>
#include <Inventor/misc/SoState.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  \fn SoLocalBBoxMatrixElement::localMatrix

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoLocalBBoxMatrixElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoLocalBBoxMatrixElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoLocalBBoxMatrixElement, inherited);
}

/*!
  Destructor.
*/

SoLocalBBoxMatrixElement::~SoLocalBBoxMatrixElement(void)
{
}

/*!
  Initializes the element to its default value. The default
  value for localMatrix and modelInverseMatrix is the identity matrix.
*/

void
SoLocalBBoxMatrixElement::init(SoState * state)
{
  inherited::init(state);
  this->localMatrix.makeIdentity();
  this->modelInverseMatrix.makeIdentity();
}

//! FIXME: write doc.

void
SoLocalBBoxMatrixElement::push(SoState * state)
{
  inherited::push(state);
  SoLocalBBoxMatrixElement * prev =
    coin_assert_cast<SoLocalBBoxMatrixElement*>(this->getNextInStack());
  this->localMatrix = prev->localMatrix;

  // avoid cache dependencies by using the state getElement method
  const SoModelMatrixElement * modelelem = coin_assert_cast<const SoModelMatrixElement*>
    (
     state->getConstElement(SoModelMatrixElement::getClassStackIndex())
     );
  // FIXME: is this really sensible caching? If push() is called more
  // often than set() (the only place where it is actually used), I
  // guess not. 20020905 mortene.
  this->modelInverseMatrix = modelelem->getModelMatrix().inverse();
}

//! FIXME: write doc.

SbBool
SoLocalBBoxMatrixElement::matches(const SoElement * /* element */) const
{
#if COIN_DEBUG
  SoDebugError::postInfo("SoLocalBBoxMatrixElement::matches",
                         "This method should never be called for this element.");
#endif // COIN_DEBUG
  return TRUE;
}

//! FIXME: write doc.

SoElement *
SoLocalBBoxMatrixElement::copyMatchInfo(void) const
{
#if COIN_DEBUG
  SoDebugError::postInfo("SoLocalBBoxMatrixElement::copyMatchInfo",
                         "This method should never be called for this element.");
#endif // COIN_DEBUG
  return NULL;
}

//! FIXME: write doc.

void
SoLocalBBoxMatrixElement::makeIdentity(SoState * const state)
{
  SoLocalBBoxMatrixElement * element;
  element = coin_safe_cast<SoLocalBBoxMatrixElement *>
    (
     SoElement::getElement(state, getClassStackIndex())
     );
  if (element) {
    element->localMatrix.makeIdentity();
    // inverse model matrix is set in push(), no need to set it here
  }
}

//! FIXME: write doc.

void
SoLocalBBoxMatrixElement::set(SoState * const state,
                              const SbMatrix & matrix)
{
  SoLocalBBoxMatrixElement * element = coin_safe_cast<SoLocalBBoxMatrixElement *>
    (
     SoElement::getElement(state, getClassStackIndex())
     );
  if (element) {
    element->localMatrix = matrix * element->modelInverseMatrix;
  }
}

//! FIXME: write doc.

void
SoLocalBBoxMatrixElement::mult(SoState * const state,
                               const SbMatrix & matrix)
{
  SoLocalBBoxMatrixElement * element = coin_safe_cast<SoLocalBBoxMatrixElement *>
    (
     SoElement::getElement(state, getClassStackIndex())
     );

  if (element) {
    element->localMatrix.multLeft(matrix);
  }
}

//! FIXME: write doc.

void
SoLocalBBoxMatrixElement::translateBy(SoState * const state,
                                      const SbVec3f & translation)
{
  SoLocalBBoxMatrixElement * element = coin_safe_cast<SoLocalBBoxMatrixElement *>
    (
     SoElement::getElement(state, getClassStackIndex())
     );

  if (element) {
    SbMatrix matrix;
    matrix.setTranslate(translation);
    element->localMatrix.multLeft(matrix);
  }
}

//! FIXME: write doc.

void
SoLocalBBoxMatrixElement::rotateBy(SoState * const state,
                                   const SbRotation & rotation)
{
  SoLocalBBoxMatrixElement * element = coin_safe_cast<SoLocalBBoxMatrixElement *>
    (
     SoElement::getElement(state, getClassStackIndex())
     );
  if (element) {
    SbMatrix matrix;
    matrix.setRotate(rotation);
    element->localMatrix.multLeft(matrix);
  }
}

//! FIXME: write doc.

void
SoLocalBBoxMatrixElement::scaleBy(SoState * const state,
                                  const SbVec3f & scaleFactor)
{
  SoLocalBBoxMatrixElement * element = coin_safe_cast<SoLocalBBoxMatrixElement *>
    (
     SoElement::getElement(state, getClassStackIndex())
     );

  if (element) {
    SbMatrix matrix;
    matrix.setScale(scaleFactor);
    element->localMatrix.multLeft(matrix);
  }
}

//! FIXME: write doc.

SbMatrix
SoLocalBBoxMatrixElement::pushMatrix(SoState * const state)
{
  // use getElementNoPush to avoid element push
  SoLocalBBoxMatrixElement * elem = coin_assert_cast<SoLocalBBoxMatrixElement *>
    (
     state->getElementNoPush(classStackIndex)
     );
  return elem->localMatrix;
}

//! FIXME: write doc.

void
SoLocalBBoxMatrixElement::popMatrix(SoState * const state,
                                    const SbMatrix & matrix)
{
  // Important: use getElementNoPush to avoid a push on element
  SoLocalBBoxMatrixElement *elem = coin_assert_cast<SoLocalBBoxMatrixElement*>
    (
     state->getElementNoPush(classStackIndex)
    );
  elem->localMatrix = matrix;
}

//! FIXME: write doc.

void
SoLocalBBoxMatrixElement::resetAll(SoState * const state)
{
  SoLocalBBoxMatrixElement * element =
    coin_safe_cast<SoLocalBBoxMatrixElement*>
    (
     state->getElement(getClassStackIndex())
     );
  while (element) {
    element->localMatrix.makeIdentity();
    element = coin_safe_cast<SoLocalBBoxMatrixElement*>(element->getNextInStack());
  }
}

//! FIXME: write doc.

const SbMatrix &
SoLocalBBoxMatrixElement::get(SoState * const state)
{
  const SoLocalBBoxMatrixElement * element =
    coin_assert_cast<const SoLocalBBoxMatrixElement *>
    (
     SoElement::getConstElement(state, getClassStackIndex())
     );
  return element->localMatrix;
}
