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
  \class SoGLDepthBufferElement Inventor/elements/SoGLDepthBufferElement.h
  \brief The SoGLDepthBufferElement controls the OpenGL depth buffer.

  \ingroup coin_elements
  \COIN_CLASS_EXTENSION
  \since Coin 3.0
*/
#include <Inventor/elements/SoGLDepthBufferElement.h>
#include "coindefs.h"

#include <cassert>

#include <Inventor/system/gl.h>

SO_ELEMENT_SOURCE(SoGLDepthBufferElement);

/*!
  \copydetails SoElement::initClass(void)
*/
void
SoGLDepthBufferElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLDepthBufferElement, inherited);
}

/*!
  Destructor.
*/
SoGLDepthBufferElement::~SoGLDepthBufferElement(void)
{
}

// doc in superclass
void
SoGLDepthBufferElement::init(SoState * state)
{
  inherited::init(state);
}

/*!
  Internal Coin method.
*/
void
SoGLDepthBufferElement::push(SoState * state)
{
  SoGLDepthBufferElement * prev =
    static_cast<SoGLDepthBufferElement *>(this->getNextInStack());
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
SoGLDepthBufferElement::pop(SoState * COIN_UNUSED_ARG(state),
                            const SoElement * prevTopElement)
{
  const SoGLDepthBufferElement * prev =
    static_cast<const SoGLDepthBufferElement *>(prevTopElement);
  if (this->test != prev->test || this->write != prev->write ||
      this->function != prev->function || this->range != prev->range) {
    this->updategl();
  }
}

/*!
  Set this element's values.
*/
void
SoGLDepthBufferElement::setElt(SbBool test, SbBool write, DepthWriteFunction function, SbVec2f range)
{
  SbBool update =
    (test != this->test) ||
    (write != this->write) ||
    (function != this->function) ||
    (range != this->range);

  inherited::setElt(test, write, function, range);

  if (update) {
    this->updategl();
  }
}

/*!
  This method performs the OpenGL updates.
*/
void
SoGLDepthBufferElement::updategl(void) const
{
  if (this->test) {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }

  if (this->write) {
    glDepthMask(GL_TRUE);
  } else {
    glDepthMask(GL_FALSE);
  }

  switch (this->function) {
  case NEVER:     glDepthFunc(GL_NEVER);     break;
  case ALWAYS:    glDepthFunc(GL_ALWAYS);    break;
  case LESS:      glDepthFunc(GL_LESS);      break;
  case LEQUAL:    glDepthFunc(GL_LEQUAL);    break;
  case EQUAL:     glDepthFunc(GL_EQUAL);     break;
  case GEQUAL:    glDepthFunc(GL_GEQUAL);    break;
  case GREATER:   glDepthFunc(GL_GREATER);   break;
  case NOTEQUAL:  glDepthFunc(GL_NOTEQUAL);  break;
  default: assert(!"unknown depth function");
  }

  glDepthRange(this->range[0], this->range[1]);
}
