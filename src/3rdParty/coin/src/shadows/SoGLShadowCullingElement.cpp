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
  \class SoGLShadowCullingElement SoGLShadowCullingElement.h Inventor/annex/FXViz/elements/SoGLShadowCullingElement.h
  \brief The SoGLShadowCullingElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.

  \since Coin 2.5
*/

#include <Inventor/annex/FXViz/elements/SoGLShadowCullingElement.h>
#include <cassert>
#include <Inventor/system/gl.h>
#include "coindefs.h"

/*!
  \fn SoGLShadowCullingElement::Mode

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoGLShadowCullingElement);

/*!
  \copydetails SoElement::initClass(void)
*/
void
SoGLShadowCullingElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLShadowCullingElement, inherited);
}

/*!
  Destructor.
*/

SoGLShadowCullingElement::~SoGLShadowCullingElement(void)
{
}

//! FIXME: write doc.

void
SoGLShadowCullingElement::set(SoState * const state,
                              SoNode * const node,
                              const int mode)
{
  SoInt32Element::set(classStackIndex, state, node, mode);
}

/*!
  Initializes the element to its default value. The default
  value is SoGLShadowCullingElement::AS_IS_CULLING.
*/

void
SoGLShadowCullingElement::init(SoState * state)
{
  inherited::init(state);
  this->data = getDefault();
}

//! FIXME: write doc.

int
SoGLShadowCullingElement::get(SoState * const state)
{
  return SoInt32Element::get(classStackIndex, state);
}

//! FIXME: write doc.

int
SoGLShadowCullingElement::getDefault(void)
{
  return AS_IS_CULLING;
}

void 
SoGLShadowCullingElement::push(SoState * COIN_UNUSED_ARG(state))
{
  SoGLShadowCullingElement * prev = (SoGLShadowCullingElement*) this->getNextInStack();

  this->data = prev->data;
}

void 
SoGLShadowCullingElement::pop(SoState * COIN_UNUSED_ARG(state), const SoElement * prevTopElement)
{
  SoGLShadowCullingElement * prev = (SoGLShadowCullingElement*) prevTopElement;
  if (prev->data != this->data) {
    this->updateGL(prev->data, this->data);
  }
}

void 
SoGLShadowCullingElement::setElt(int32_t value)
{
  if (this->data != value) {
    this->updateGL(this->data, value);
  }
  inherited::setElt(value);
}

void 
SoGLShadowCullingElement::updateGL(int32_t COIN_UNUSED_ARG(oldvalue), int32_t COIN_UNUSED_ARG(value))
{
  // nothing to do yet. We might support more culling modes in the future though
}


#ifdef COIN_TEST_SUITE

BOOST_AUTO_TEST_CASE(initialized)
{
  BOOST_CHECK_MESSAGE(SoGLShadowCullingElement::getClassStackIndex() != -1,
                      "missing class initialization");
}

#endif // COIN_TEST_SUITE
