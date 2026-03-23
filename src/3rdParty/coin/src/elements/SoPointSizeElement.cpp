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
  \class SoPointSizeElement Inventor/elements/SoPointSizeElement.h
  \brief The SoPointSizeElement changes the point size setting of the render state.

  \ingroup coin_elements

  Requests from the scene graph to change the point size when rendering
  point primitives will be made through this element.

  The SoPointSizeElement class itself is just a generic abstraction
  for the underlying So*PointSizeElement classes with code specific
  for each supported immediate mode rendering library.
*/

#include <Inventor/elements/SoPointSizeElement.h>


#include <cassert>

SO_ELEMENT_SOURCE(SoPointSizeElement);

/*!
  \copydetails SoElement::initClass(void)
*/
void
SoPointSizeElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoPointSizeElement, inherited);
}

/*!
  Destructor.
*/
SoPointSizeElement::~SoPointSizeElement(void)
{
}

/*!
  Initializes the element to its default value. The default
  value for point size is 0.0.
*/
void
SoPointSizeElement::init(SoState * state)
{
  inherited::init(state);
  this->data = SoPointSizeElement::getDefault();
}

/*!
  Static method for setting the current \a pointSize value in the
  given traversal \a state.
*/
void
SoPointSizeElement::set(SoState * const state, SoNode * const node,
                        const float pointSize)
{
  inherited::set(SoPointSizeElement::classStackIndex, state, node, pointSize);
}

/*!
  Static method for setting the current \a pointSize value in the
  given traversal \a state.
*/
void
SoPointSizeElement::set(SoState * const state, const float pointSize)
{
  SoPointSizeElement::set(state, NULL, pointSize);
}

/*!
  Static method for returning the current point size setting in the
  given traversal \a state.
*/
float
SoPointSizeElement::get(SoState * const state)
{
  return inherited::get(SoPointSizeElement::classStackIndex, state);
}

/*!
  Returns default point size setting.
*/
float
SoPointSizeElement::getDefault(void)
{
  return 0.0f;
}
