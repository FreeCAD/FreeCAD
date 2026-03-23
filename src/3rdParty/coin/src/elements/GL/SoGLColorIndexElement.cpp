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
  \class SoGLColorIndexElement Inventor/elements/SoGLColorIndexElement.h
  \brief The SoGLColorIndexElement class sets the current OpenGL color.

  \ingroup coin_elements

  This element is only used when the OpenGL canvas is in color index
  mode, i.e. where colors for individual pixels are fetched from a color
  lookup table ("CLUT"). The usual thing to do is to set up a canvas
  in RGBA true color mode.

  One common use for color index mode OpenGL canvases is to use one in
  the overlay planes (which are usually limited to only 2 or 4
  available colors), if supported by the OpenGL hardware and / or
  driver.

  \sa SoColorIndex
*/

#include <Inventor/elements/SoGLColorIndexElement.h>
#include <Inventor/elements/SoGLLazyElement.h>
#include <Inventor/misc/SoState.h>
#include <cassert>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/system/gl.h>

SO_ELEMENT_SOURCE(SoGLColorIndexElement);

/*!
  \copydetails SoElement::initClass(void)
*/
void
SoGLColorIndexElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLColorIndexElement, inherited);
}


// doc in superclass
void
SoGLColorIndexElement::init(SoState * stateptr)
{
  inherited::init(state);
  this->state = stateptr;
}

/*!
  Destructor.
*/
SoGLColorIndexElement::~SoGLColorIndexElement()
{
}

/*!
  Returns \c TRUE if the current GL context is in color index mode.
*/
SbBool
SoGLColorIndexElement::isColorIndexMode(SoState * state)
{
  return SoGLLazyElement::isColorIndex(state);
}

/*!
  Sets current color indices.
*/
void
SoGLColorIndexElement::set(SoState * const state, SoNode * const node,
                           const int32_t numindices,
                           const int32_t * const indices)
{
  SoLazyElement::setColorIndices(state, node, numindices, indices);
}

/*!
  Returns number of color indices in element.
*/
int32_t
SoGLColorIndexElement::getNum(void) const
{
  return SoLazyElement::getInstance(this->state)->getNumColorIndices();
}

/*!
  Returns the current element.
*/
const SoGLColorIndexElement *
SoGLColorIndexElement::getInstance(SoState *state)
{
  return (const SoGLColorIndexElement *)
    state->getElementNoPush(classStackIndex);
}

/*!
  Returns the index'th color index in element.
*/
int32_t
SoGLColorIndexElement::get(const int index) const
{
  assert(index >= 0 && index < this->getNum());
  return SoLazyElement::getColorIndices(this->state)[index];
}

/*!
  Returns the default color index in element.
*/
int32_t
SoGLColorIndexElement::getDefault(void)
{
  return SoLazyElement::getDefaultColorIndex();
}
