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
  \class SoWindowElement Inventor/elements/SoWindowElement.h
  \brief The SoWindowElement class is used to store current window attributes.

  \ingroup coin_elements

  In Coin, this element is not API-compatible with SGI Inventor, since
  it contains platform specific stuff, which we want to avoid.

  Instead of the platform specific types we use void pointers. We're
  sorry for any inconvenience this might cause people using this element.
*/

#include <Inventor/elements/SoWindowElement.h>

#include "SbBasicP.h"

#include <cassert>

/*!
  \fn SoWindowElement::window

  The window id.
*/

/*!
  \fn SoWindowElement::context

  The current context.
*/

/*!
  \fn SoWindowElement::display

  The current display.
*/

/*!
  \fn SoWindowElement::glRenderAction

  The current render action.
*/

SO_ELEMENT_SOURCE(SoWindowElement);

/*!
  \copydetails SoElement::initClass(void)
*/
void
SoWindowElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoWindowElement, inherited);
}

/*!
  Destructor.
*/
SoWindowElement::~SoWindowElement(void)
{
}

// doc in parent
void
SoWindowElement::init(SoState * state)
{
  inherited::init(state);
  this->window = 0;
  this->context = NULL;
  this->display = NULL;
  this->glRenderAction = NULL;
}

// doc in parent
void
SoWindowElement::push(SoState * state)
{
  inherited::push(state);
}

// doc in parent
void
SoWindowElement::pop(SoState * state,
                     const SoElement * prevTopElement)
{
  inherited::pop(state, prevTopElement);
}

// doc in parent
SbBool
SoWindowElement::matches(const SoElement * /* element */) const
{
  assert(0 && "should never be called.");
  return TRUE;
}

// doc in parent
SoElement *
SoWindowElement::copyMatchInfo(void) const
{
  assert(0 && "should never be called.");
  return NULL;
}

/*!
  Sets data for this element.
*/
void
SoWindowElement::set(SoState * state,
                     void * window,
                     void * context,
                     void * display,
                     SoGLRenderAction * action)
{
  SoWindowElement * elem = coin_safe_cast<SoWindowElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->window = window;
    elem->context = context;
    elem->display = display;
    elem->glRenderAction = action;
  }
}


/*!
  Returns data for this element.
*/
void
SoWindowElement::get(SoState * state,
                     void * & window,
                     void * & context,
                     void * & display,
                     SoGLRenderAction * & action)
{
  const SoWindowElement * elem = coin_assert_cast<const SoWindowElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );

  window = elem->window;
  context = elem->context;
  display = elem->display;
  action = elem->glRenderAction;
}
