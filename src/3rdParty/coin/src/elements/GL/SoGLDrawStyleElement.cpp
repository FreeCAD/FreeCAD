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
  \class SoGLDrawStyleElement Inventor/elements/SoGLDrawStyleElement.h
  \brief The SoGLDrawStyleElement updates the current draw style in OpenGL.

  \ingroup coin_elements
*/

#include <Inventor/elements/SoGLDrawStyleElement.h>
#include "coindefs.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/system/gl.h>

#include <cassert>

SO_ELEMENT_SOURCE(SoGLDrawStyleElement);

/*!
  \copydetails SoElement::initClass(void)
*/
void
SoGLDrawStyleElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLDrawStyleElement, inherited);
}

/*!
  Destructor.
*/
SoGLDrawStyleElement::~SoGLDrawStyleElement(void)
{
}

// doc in superclass
void
SoGLDrawStyleElement::init(SoState * state)
{
  inherited::init(state);
}

// doc in superclass
void
SoGLDrawStyleElement::push(SoState * state)
{
  SoGLDrawStyleElement * prev = (SoGLDrawStyleElement*)
    this->getNextInStack();
  // copy data to avoid unessesary GL calls
  this->data = prev->data;
  // capture previous element since we might or might not change the
  // GL state in set/pop
  prev->capture(state);
}

// doc in superclass
void
SoGLDrawStyleElement::pop(SoState * COIN_UNUSED_ARG(state),
                          const SoElement * prevTopElement)
{
  SoGLDrawStyleElement * prev = (SoGLDrawStyleElement*) prevTopElement;
  if (this->data != prev->data) this->updategl();
}

// doc in superclass
void
SoGLDrawStyleElement::setElt(int32_t style)
{
  if (style != this->data) {
    this->data = (int32_t)style;
    this->updategl();
  }
}

void
SoGLDrawStyleElement::updategl(void)
{
  switch ((Style)this->data) {
  case SoDrawStyleElement::FILLED:
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  case SoDrawStyleElement::LINES:
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    break;
  case SoDrawStyleElement::POINTS:
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    break;
  case SoDrawStyleElement::INVISIBLE:
    // handled in SoShape::shouldGLRender()
    break;
  default:
    assert(0 && "unsupported switch case");
    break;
  }
}
