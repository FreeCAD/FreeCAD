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
  \class SoFloatElement Inventor/elements/SoFloatElement.h
  \brief SoFloatElement is an abstract base class for elements that consists of a single float value.

  \ingroup coin_elements

  This is the superclass of elements where the new element data \e
  replaces the old data, and where the data the element stores is a
  simple single precision floating point value.

  This element is like a convenient light-weight version of the
  SoReplacedElement. It differs from the SoReplacedElement in that the
  set() and get() methods are already implemented, since it is known
  that subclasses will still contain just a single float value.

  \sa SoReplacedElement, SoInt32Element, SoAccumulatedElement
*/

#include "coindefs.h"
#include "SbBasicP.h"

#include <Inventor/elements/SoFloatElement.h>
#include <cassert>

SO_ELEMENT_ABSTRACT_SOURCE(SoFloatElement);

/*!
  \var float SoFloatElement::data

  The floating point value of the element.
*/

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoFloatElement::initClass(void)
{
  SO_ELEMENT_INIT_ABSTRACT_CLASS(SoFloatElement, inherited);
}

/*!
  Destructor.
*/
SoFloatElement::~SoFloatElement(void)
{
}

// doc in super
SbBool
SoFloatElement::matches(const SoElement * element) const
{
  assert(element);
  if (getTypeId() != element->getTypeId()) { return FALSE; }
  if (this->data != (coin_assert_cast<const SoFloatElement *>(element)->data)) {
    return FALSE;
  }
  return TRUE;
}

// doc in super
SoElement *
SoFloatElement::copyMatchInfo(void) const
{
  // SoElement::copyMatchInfo is abstract
  //    inherited::copyMatchInfo();
  assert(getTypeId().canCreateInstance());
  SoFloatElement * element = static_cast<SoFloatElement *>(getTypeId().createInstance());
  element->data = this->data;
  // DEPRECATED 980807 pederb. copyMatchInfo() should only copy
  // information needed in matches(). An exact copy is not needed.
  //    element->dataNode = this->dataNode;
  return element;
}

// doc in super
void
SoFloatElement::print(FILE * file) const
{
  (void)fprintf(file, "%s[%p]: data = %f\n",
                this->getTypeId().getName().getString(), this, this->data);
}

/*!
  Static method for setting the \a value of an element in the given \a
  state at the given stack \a index.
 */
void
SoFloatElement::set(const int index,
                    SoState * const state,
                    SoNode * const COIN_UNUSED_ARG(node),
                    const float value)
{
  SoFloatElement * element =
    coin_safe_cast<SoFloatElement *>
    (
     SoFloatElement::getElement(state, index)
     );
  if (element) {
    element->setElt(value);
  }
}

/*!
  Static method for setting the \a value of an element in the given \a
  state at the given \a stackIndex.
 */
void
SoFloatElement::set(const int stackIndex, SoState * const state,
                    const float value)
{
  SoFloatElement::set(stackIndex, state, NULL, value);
}

/*!
  Static method to fetch the value of the element of this type from
  the given \a state at the given stack \a index.
 */
float
SoFloatElement::get(const int index, SoState * const state)
{
  const SoFloatElement * element = coin_safe_cast<const SoFloatElement *>
    (
     getConstElement(state, index)
     ); //, NULL );
  if (element) { return element->data; }
  return 0.0f;
}

/*!
  Set element value.
 */
void
SoFloatElement::setElt(float value)
{
  this->data = value;
}

/*!
  Initializes the element to its default value. The default
  value is 0.0.
*/
void
SoFloatElement::init(SoState * state)
{
  inherited::init(state);
  this->data = 0.0f;
}
