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
  \class SoGLLineWidthElement Inventor/elements/SoGLLineWidthElement.h
  \brief The SoGLLineWidthElement class changes the line width setting of the OpenGL render state.

  \ingroup coin_elements

  Requests from the scene graph to change the line width when rendering
  OpenGL line primitives will be made through this element, which
  forwards it to the appropriate native OpenGL call.

  The Coin library does not place any bounds on the values of the
  line widths, but be aware that the range and granularity of what is
  valid line widths depends on the underlying OpenGL
  implementation. Application programmers using line primitives
  (typically through the SoLineSet or SoIndexedLineSet nodes) should
  consider these boundary values. They can be acquired by running the
  following code from within a valid OpenGL context:

  \code
    GLfloat bounds[2];
    glGetFloatv(GL_LINE_WIDTH_RANGE, bounds);
    GLfloat granularity[1];
    glGetFloatv(GL_LINE_WIDTH_GRANULARITY, granularity);
  \endcode

  Another, perhaps more convenient, way of acquiring the OpenGL
  implementation limits with regard to point sizes is to use the
  So\@Gui\@GLWidget\::getPointSizeLimits() method in the GUI "glue" interface
  library you are using (SoQt, SoXt, SoGtk, SoWin, ...).
*/

#include <Inventor/elements/SoGLLineWidthElement.h>
#include "coindefs.h"

#include <cfloat>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/system/gl.h>

#include <cassert>
#include <Inventor/errors/SoDebugError.h>

// Important note: do _not_ use a "static const" variable instead, as
// it is then not given that it will be initialized before the static
// "sizerange" class variable array below.
#define RANGE_NOT_CHECKED FLT_MAX

float SoGLLineWidthElement::sizerange[2] = { RANGE_NOT_CHECKED, -1.0f};


SO_ELEMENT_SOURCE(SoGLLineWidthElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLLineWidthElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLLineWidthElement, inherited);
}

/*!
  Destructor.
*/
SoGLLineWidthElement::~SoGLLineWidthElement(void)
{
}

// doc in superclass
void
SoGLLineWidthElement::init(SoState * stateptr)
{
  inherited::init(stateptr);
}

// doc in superclass
void
SoGLLineWidthElement::push(SoState * stateptr)
{
  SoGLLineWidthElement * prev = (SoGLLineWidthElement*)this->getNextInStack();
  this->data = prev->data;
  // capture previous element since we might or might not change the
  // GL state in set/pop
  prev->capture(stateptr);
}

// doc in superclass
void
SoGLLineWidthElement::pop(SoState * COIN_UNUSED_ARG(stateptr), const SoElement * prevTopElement)
{
  SoGLLineWidthElement * prev = (SoGLLineWidthElement*)prevTopElement;
  if (this->data != prev->data) {
    this->updategl();
  }
}

// doc in superclass
void
SoGLLineWidthElement::setElt(float width)
{
  if (width != this->data) {
    this->data = width;
    this->updategl();
  }
}

// private
void
SoGLLineWidthElement::updategl(void)
{
  if (SoGLLineWidthElement::sizerange[0] == RANGE_NOT_CHECKED) {
    GLfloat vals[2];
    glGetFloatv(GL_LINE_WIDTH_RANGE, vals);

    // Matthias Koenig reported on coin-discuss that the OpenGL
    // implementation on SGI Onyx 2 InfiniteReality returns 0 for the
    // lowest line width, but it will still set the return value of
    // glGetError() to GL_INVALID_VALUE if this size is attempted
    // used. This is a workaround for what looks like an OpenGL bug.

    if (vals[0] <= 0.0f) { vals[0] = SbMin(1.0f, vals[1]); }

    SoGLLineWidthElement::sizerange[0] = vals[0];
    SoGLLineWidthElement::sizerange[1] = vals[1];
  }

  float useval = this->data;

  // 0.0f is used as a "dummy" default value by our superclass and by
  // SoDrawStyle::lineWidth, so handle that case outside of the
  // range check below.

  if (this->data == 0.0f) { useval = 1.0f; }

  // Range checks.

  if (useval < SoGLLineWidthElement::sizerange[0]) {
    useval = SoGLLineWidthElement::sizerange[0];
  }
  if (useval > SoGLLineWidthElement::sizerange[1]) {
    useval = SoGLLineWidthElement::sizerange[1];
  }


  if (COIN_DEBUG) {
    // Detect invalid values and warn the application programmer.
    // (0.0f is used as a "dummy" default value by our superclass and
    // by SoDrawStyle::lineWidth, so ignore that case.)
    if ((useval != this->data) && (this->data != 0.0f)) {
      // Only warn once for each case.
      static SbBool warn_below = TRUE;
      static SbBool warn_above = TRUE;
      if ((warn_below && (useval > this->data)) ||
          (warn_above && (useval < this->data))) {
        if (useval > this->data) { warn_below = FALSE; }
        if (useval < this->data) { warn_above = FALSE; }
        SoDebugError::postWarning("SoGLLineWidthElement::updategl",
                                  "%f is outside the legal range of [%f, %f] "
                                  "for this OpenGL implementation's "
                                  "glLineWidth() settings. It was now clamped.\n\n"
                                  "See the documentation of SoGLLineWidthElement for "
                                  "information on how the application programmer should "
                                  "acquire the boundary values for the legal "
                                  "range.",
                                  this->data,
                                  SoGLLineWidthElement::sizerange[0],
                                  SoGLLineWidthElement::sizerange[1]);
      }
    }
  }

  // Forward to OpenGL state.

  glLineWidth(useval);
}

#undef RANGE_NOT_CHECKED
