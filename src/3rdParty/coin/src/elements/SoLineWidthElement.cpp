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
  \class SoLineWidthElement Inventor/elements/SoLineWidthElement.h
  \brief The SoLineWidthElement class changes the line width setting of the render state.

  \ingroup coin_elements

  Requests from the scene graph to change the line width when rendering
  line primitives will be made through this element, which forwards it
  to the appropriate native call in the underlying rendering library.

  Subsequent nodes rendering line primitives will use the width
  setting (for instance SoLineSet nodes).
*/

#include <Inventor/elements/SoLineWidthElement.h>


#include <cassert>

SO_ELEMENT_SOURCE(SoLineWidthElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoLineWidthElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoLineWidthElement, inherited);
}

/*!
  Destructor.
*/
SoLineWidthElement::~SoLineWidthElement()
{
}

/*!
  Initializes the element to its default value. The default
  value is 0.0.
*/
void
SoLineWidthElement::init(SoState * state)
{
  inherited::init(state);
  this->data = SoLineWidthElement::getDefault();
}

/*!
  Set up the current state's \a lineWidth value.
 */
void
SoLineWidthElement::set(SoState * const state, SoNode * const node,
                        const float lineWidth)
{
  SoFloatElement::set(classStackIndex, state, node, lineWidth);
}

/*!
  Set up the current state's \a lineWidth value.
 */
void
SoLineWidthElement::set(SoState * const state, const float lineWidth)
{
  SoLineWidthElement::set(state, NULL, lineWidth);
}

/*!
  Returns the current line width value.
 */
float
SoLineWidthElement::get(SoState * const state)
{
  return SoFloatElement::get(classStackIndex, state);
}

/*!
  Returns the default line width value if no value has been set
  explicitly.
 */
float
SoLineWidthElement::getDefault(void)
{
  // 0 is an indicator value which means "use the default value of the
  // rendering-library specific So*LineWidth element".
  return 0.0f;
}
