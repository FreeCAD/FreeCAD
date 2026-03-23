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
  \class SoInt32Element SoInt32Element.h Inventor/elements/SoInt32Element.h
  \brief The SoInt32Element class is the base class for elements that simply store a 32-bit integer.

  \ingroup coin_elements

  This is the superclass of elements where the new element data \e
  replaces the old data, and where the data the element stores is a
  simple 32-bit integer value.

  This element is like a convenient light-weight version of the
  SoReplacedElement. It differs from the SoReplacedElement in that the
  set() and get() methods are already implemented, since it is known
  that subclasses will still contain just a single 32-bit integer
  value.

  \sa SoReplacedElement, SoFloatElement, SoAccumulatedElement
*/

#include "SbBasicP.h"

#include <Inventor/elements/SoInt32Element.h>


#include <cassert>

/*!
  \var int32_t SoInt32Element::data

  The 32-bit integer value of the element.
*/

SO_ELEMENT_ABSTRACT_SOURCE(SoInt32Element);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoInt32Element::initClass(void)
{
  SO_ELEMENT_INIT_ABSTRACT_CLASS(SoInt32Element, inherited);
}

/*!
  Destructor.
*/

SoInt32Element::~SoInt32Element(void)
{
}

// documented in superclass
SbBool
SoInt32Element::matches(const SoElement * element) const
{
    assert(element);
    if (getTypeId() != element->getTypeId())
        return FALSE;
    if (coin_assert_cast<const SoInt32Element *>(element)->data != this->data)
        return FALSE;
    return TRUE;
}

// documented in superclass
SoElement *
SoInt32Element::copyMatchInfo(void) const
{
    assert(getTypeId().canCreateInstance());
    SoInt32Element * element =
        static_cast<SoInt32Element *>(getTypeId().createInstance());
    element->data = this->data;

    // DEPRECATED 19980807 pederb. copyMatchInfo should only copy
    // information needed in matches(). An exact copy is not needed.
    //
    //    element->dataNode = this->dataNode;
    return element;
}

// documented in superclass
void
SoInt32Element::print(FILE * file) const
{
  (void)fprintf(file, "%s[%p]: data = %d\n",
                getTypeId().getName().getString(), this, this->data);
}

/*!
  Static method for setting the \a value of an element in the given \a
  state at the given stack \a index.
 */
void
SoInt32Element::set(const int index,
                    SoState * const state,
                    SoNode * const /* node */,
                    const int32_t value)
{
  SoInt32Element * element;
  element = coin_safe_cast<SoInt32Element *>(getElement(state, index));
  if (element)
    element->setElt(value);
}

/*!
  Static method for setting the \a value of an element in the given \a
  state at the given \a stackIndex.
 */
void
SoInt32Element::set(const int index, SoState * const state,
                    const int32_t value)
{
  set(index, state, NULL, value);
}

/*!
  Static method to fetch the value of the element of this type from
  the given \a state at the given stack \a index.
 */
int32_t
SoInt32Element::get(const int index,
                    SoState * const state)
{
  const SoInt32Element * element;
  element = coin_safe_cast<const SoInt32Element *>(getConstElement(state, index)); //, NULL );
  if (element)
    return element->data;
  return 0;
}

/*!
  Set element value.
 */
void
SoInt32Element::setElt(int32_t value)
{
  this->data = value;
}

/*!
  Initializes the element to its default value. The default
  value for the int32 value is 0.
*/

void
SoInt32Element::init(SoState * state)
{
  inherited::init(state);
  this->data = 0;
}
