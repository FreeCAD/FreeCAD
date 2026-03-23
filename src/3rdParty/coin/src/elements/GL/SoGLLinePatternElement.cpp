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
  \class SoGLLinePatternElement Inventor/elements/SoGLLinePatternElement.h
  \brief The SoGLLinePatternElement class changes the line stipple pattern
         of the OpenGL render state.

  \ingroup coin_elements

  Requests from the scene graph to change the stipple pattern when rendering
  OpenGL line primitives will be made through this element, which forwards
  it to the appropriate native OpenGL call.
*/

#include <Inventor/elements/SoGLLinePatternElement.h>
#include "coindefs.h"


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/system/gl.h>

#include <cassert>

SO_ELEMENT_SOURCE(SoGLLinePatternElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLLinePatternElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLLinePatternElement, inherited);
}

/*!
  Destructor.
*/

SoGLLinePatternElement::~SoGLLinePatternElement(void)
{
}

/*!
  Initializes element in state to default value.
*/

void
SoGLLinePatternElement::init(SoState * state)
{
  inherited::init(state);
}

/*!
  Creates new element in stack.
*/

void
SoGLLinePatternElement::push(SoState * state)
{
  SoGLLinePatternElement * prev = (SoGLLinePatternElement*)
    this->getNextInStack();

  this->data = prev->data;
  // capture element since we might or might not change the GL state
  prev->capture(state);
}

/*!
  Removes element from stack.
*/

void
SoGLLinePatternElement::pop(SoState * COIN_UNUSED_ARG(state),
                            const SoElement * prevTopElement)
{
  SoGLLinePatternElement * prev = (SoGLLinePatternElement*) prevTopElement;
  if (this->data != prev->data) this->updategl();
}

//! Called whenever element value is set. Triggers GL update.

void
SoGLLinePatternElement::setElt(int32_t pattern)
{
  if (pattern != this->data) {
    this->data = pattern;
    this->updategl();
  }
}

/*!
  Applies line stipple patterns to OpenGL state.
*/

void
SoGLLinePatternElement::updategl()
{
  //
  // FIXME: store flag to keep enable/disable state, pederb 990624
  //
  if ((this->data & 0xffff) == (int32_t) CONTINUOUS) {
    glDisable(GL_LINE_STIPPLE);
  }
  else {
    // Enable line stipple before setting the pattern. This is
    // needed to work around a bug in the nVidia 2.1.1 drivers.
    glEnable(GL_LINE_STIPPLE);
    glLineStipple((GLint) (this->data >> 16), (GLushort) (this->data & 0xffff));
  }
}
