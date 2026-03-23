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
  \class SoDepthBufferElement Inventor/elements/SoDepthBufferElement.h
  \brief The SoDepthBufferElement controls the depth buffer settings.

  \ingroup coin_elements
  \COIN_CLASS_EXTENSION
  \since Coin 3.0
*/

#include <Inventor/elements/SoDepthBufferElement.h>

#include <cassert>

#include "coindefs.h"
#include "SbBasicP.h"

SO_ELEMENT_SOURCE(SoDepthBufferElement);

/*!
  \fn static SoType SoDepthBufferElement::getClassTypeId(void)

  This static method returns the class type.
*/

/*!
  \fn static int SoDepthBufferElement::getClassStackIndex(void)

  This static method returns the state stack index for the class.
*/

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoDepthBufferElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoDepthBufferElement, inherited);
}

/*!
  Destructor.
*/
SoDepthBufferElement::~SoDepthBufferElement(void)
{
}

/*!
  Internal Coin method.
*/
void
SoDepthBufferElement::init(SoState * state)
{
  inherited::init(state);
  this->test = TRUE;
  this->write = TRUE;
  this->function = LEQUAL;
  this->range.setValue(0.0f, 1.0f);
}

/*!
  Internal Coin method.
*/
void
SoDepthBufferElement::push(SoState * state)
{
  const SoDepthBufferElement * prev = coin_assert_cast<const SoDepthBufferElement *>
    (
     this->getNextInStack()
     );
  this->test = prev->test;
  this->write = prev->write;
  this->function = prev->function;
  this->range = prev->range;
  prev->capture(state);
}

/*!
  Internal Coin method.
*/
void
SoDepthBufferElement::pop(SoState * COIN_UNUSED_ARG(state),
                          const SoElement * COIN_UNUSED_ARG(prevTopElement))
{
}

/*!
  Set this element's values.
*/
void
SoDepthBufferElement::set(SoState * state,
                          SbBool test,
                          SbBool write,
                          DepthWriteFunction function,
                          SbVec2f range)
{
  SoDepthBufferElement * elem =
    static_cast<SoDepthBufferElement *>(SoElement::getElement(state, classStackIndex));

  elem->setElt(test, write, function, range);
}

/*!
  Fetches this element's values.
*/
void
SoDepthBufferElement::get(SoState * state,
                          SbBool & test_out,
                          SbBool & write_out,
                          DepthWriteFunction & function_out,
                          SbVec2f & range_out)
{
  const SoDepthBufferElement * elem =
    static_cast<const SoDepthBufferElement *>(SoElement::getConstElement(state, classStackIndex));
  test_out = elem->test;
  write_out = elem->write;
  function_out = elem->function;
  range_out = elem->range;
}

/*!
  Returns the depth test enabled state.
*/
SbBool
SoDepthBufferElement::getTestEnable(SoState * state)
{
  const SoDepthBufferElement * elem =
    static_cast<const SoDepthBufferElement *>(SoElement::getConstElement(state, classStackIndex));
  return elem->test;
}

/*!
  Returns the depth buffer write enabled state.
*/
SbBool
SoDepthBufferElement::getWriteEnable(SoState * state)
{
  const SoDepthBufferElement * elem =
    static_cast<const SoDepthBufferElement *>(SoElement::getConstElement(state, classStackIndex));
  return elem->write;
}

/*!
  Returns the set depth buffer write function.
*/
SoDepthBufferElement::DepthWriteFunction
SoDepthBufferElement::getFunction(SoState * state)
{
  const SoDepthBufferElement * elem =
    static_cast<const SoDepthBufferElement *>(SoElement::getConstElement(state, classStackIndex));
  return elem->function;
}

/*!
  Returns the depth buffer value range used.
*/
SbVec2f
SoDepthBufferElement::getRange(SoState * state)
{
  const SoDepthBufferElement * elem =
    static_cast<const SoDepthBufferElement *>(SoElement::getConstElement(state, classStackIndex));
  return elem->range;
}

/*!
  Internal Coin method.
*/
SbBool
SoDepthBufferElement::matches(const SoElement * element) const
{
  const SoDepthBufferElement * elem =
    static_cast<const SoDepthBufferElement *>(element);

  return (elem->test == this->test)
    && (elem->write == this->write)
    && (elem->function == this->function)
    && (elem->range == this->range);
}

/*!
  Internal Coin method.
*/
SoElement *
SoDepthBufferElement::copyMatchInfo(void) const
{
  SoDepthBufferElement * elem =
    static_cast<SoDepthBufferElement *>(this->getTypeId().createInstance());
  elem->test = this->test;
  elem->write = this->write;
  elem->function = this->function;
  elem->range = this->range;
  return elem;
}

/*!
  Virtual method to set the state to get derived elements updated.
*/
void
SoDepthBufferElement::setElt(SbBool test, SbBool write, DepthWriteFunction function, SbVec2f range)
{
  this->test = test;
  this->write = write;
  this->function = function;
  this->range = range;
}
