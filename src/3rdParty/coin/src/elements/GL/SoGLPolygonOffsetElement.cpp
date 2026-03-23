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
  \class SoGLPolygonOffsetElement Inventor/elements/SoGLPolygonOffsetElement.h
  \brief The SoGLPolygonOffsetElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoGLPolygonOffsetElement.h>
#include "coindefs.h"

#include <cassert>
#include <cstdlib>

#include <Inventor/C/tidbits.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoGLDriverDatabase.h>

#include "rendering/SoGL.h"

SO_ELEMENT_SOURCE(SoGLPolygonOffsetElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLPolygonOffsetElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLPolygonOffsetElement, inherited);
}

/*!
  Destructor.
*/

SoGLPolygonOffsetElement::~SoGLPolygonOffsetElement(void)
{
}

//! FIXME: write doc.

void
SoGLPolygonOffsetElement::init(SoState * stateptr)
{
  inherited::init(stateptr);
  this->state = stateptr;
}

//! FIXME: write doc.

void
SoGLPolygonOffsetElement::push(SoState * stateptr)
{
  SoGLPolygonOffsetElement * prev = (SoGLPolygonOffsetElement*)this->getNextInStack();

  this->style = prev->style;
  this->active = prev->active;
  this->offsetfactor = prev->offsetfactor;
  this->offsetunits = prev->offsetunits;
  this->state = stateptr;
  // capture previous element since we might or might not change the
  // GL state in set/pop
  prev->capture(stateptr);
}

//! FIXME: write doc.

void
SoGLPolygonOffsetElement::pop(SoState * COIN_UNUSED_ARG(stateptr), const SoElement * prevTopElement)
{
  const SoGLPolygonOffsetElement * prev =
    (const SoGLPolygonOffsetElement*)prevTopElement;

  if (this->style != prev->style ||
      this->active != prev->active ||
      this->offsetfactor != prev->offsetfactor ||
      this->offsetunits != prev->offsetunits) {
    this->updategl();
  }
}

//! FIXME: write doc.

void
SoGLPolygonOffsetElement::setElt(float factor, float units,
                                 Style styles, SbBool on)
{
  if (on != this->active ||
      styles != this->style ||
      factor != this->offsetfactor ||
      units != this->offsetunits) {
    this->active = on;
    this->style = styles;
    this->offsetfactor = factor;
    this->offsetunits = units;
    this->updategl();
  }
}


//! FIXME: write doc.

void
SoGLPolygonOffsetElement::updategl(void)
{
  const cc_glglue * w = sogl_glue_instance(this->state);
  if (!SoGLDriverDatabase::isSupported(w, SO_GL_POLYGON_OFFSET)) {
    static SbBool first = TRUE;
    if (first) {
      SoDebugError::postWarning("SoGLPolygonOffsetElement::updategl",
                                "OpenGL driver doesn't support z-buffer "
                                "offsetting");
      first = FALSE;
    }
    return;
  }


  if (this->active) {
    int styles = 0;
    styles |= (this->style & FILLED) ? cc_glglue_FILLED : 0;
    styles |= (this->style & LINES) ? cc_glglue_LINES : 0;
    styles |= (this->style & POINTS) ? cc_glglue_POINTS : 0;
    cc_glglue_glPolygonOffsetEnable(w, TRUE, styles);

    cc_glglue_glPolygonOffset(w, this->offsetfactor, this->offsetunits);
  }
  else { // ! active
    int all = cc_glglue_FILLED | cc_glglue_LINES | cc_glglue_POINTS;
    cc_glglue_glPolygonOffsetEnable(w, FALSE, all);
  }
}
