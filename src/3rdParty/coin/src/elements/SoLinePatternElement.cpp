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
  \class SoLinePatternElement Inventor/elements/SoLinePatternElement.h
  \brief The SoLinePatternElement class defines the line stipple pattern.

  \ingroup coin_elements

  Line stippling is used to mask out fragments of a line.
*/

#include <Inventor/elements/SoLinePatternElement.h>
#include <Inventor/errors/SoDebugError.h>


#include <cassert>

SO_ELEMENT_SOURCE(SoLinePatternElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoLinePatternElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoLinePatternElement, inherited);
}

/*!
  Destructor.
*/

SoLinePatternElement::~SoLinePatternElement(void)
{
}

/*!
  Sets the given pattern in the state.
*/

void
SoLinePatternElement::set(SoState * const state,
                          SoNode * const node,
                          const int32_t pattern,
                          const int32_t factor)
{
  int32_t factorClamped = factor;

#if COIN_DEBUG
  if (factor < 1) {
    factorClamped = 1;
    SoDebugError::postWarning("SoLinePatternElement::set", 
                              "Factor out of range (%d). Clamped to 1.", factor);
  } else if (factor > 256) {
    factorClamped = 256;
    SoDebugError::postWarning("SoLinePatternElement::set",
                              "Factor out of range (%d). Clamped to 256.", factor);
  }
#endif // COIN_DEBUG

  // pattern and scale factor are stored as single value (pattern: 0 - 15, factor: 16-24)
  SoInt32Element::set(classStackIndex, state, node, (pattern & 0xffff) | ((factorClamped & 0x1ff) << 16));
}

/*!
  Initializes element in state to default value.
*/

void
SoLinePatternElement::init(SoState * state)
{
  inherited::init(state);

  this->data = SoLinePatternElement::getDefault() | (SoLinePatternElement::getDefaultScaleFactor() << 16);
}

/*!
  Sets the given pattern in the state.
*/

//$ EXPORT INLINE
void
SoLinePatternElement::set(SoState * const state, const int32_t pattern,
                          const int32_t factor)
{
  set(state, NULL, pattern, factor);
}

/*!
  Returns line stipple pattern from state.
*/

//$ EXPORT INLINE
int32_t
SoLinePatternElement::get(SoState * const state)
{
  return SoInt32Element::get(classStackIndex, state) & 0xffff;
}

/*!
  Returns default line stipple pattern.
*/

//$ EXPORT INLINE
int32_t
SoLinePatternElement::getDefault()
{
  return CONTINUOUS;
}

/*!
  Returns line stipple pattern scale factor from state.
*/

//$ EXPORT INLINE
int32_t
SoLinePatternElement::getScaleFactor(SoState * const state)
{
  return SoInt32Element::get(classStackIndex, state) >> 16;
}

/*!
  Returns default line stipple pattern scale factor.
*/

//$ EXPORT INLINE
int32_t
SoLinePatternElement::getDefaultScaleFactor()
{
  return 1;
}
