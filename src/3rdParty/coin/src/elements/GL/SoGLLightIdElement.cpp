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
  \class SoGLLightIdElement Inventor/elements/SoGLLightIdElement.h
  \brief The SoGLLightIdElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

// *************************************************************************

#include <Inventor/elements/SoGLLightIdElement.h>

#include <cassert>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/C/glue/gl.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/system/gl.h>

#include "glue/glp.h"
#include "rendering/SoGL.h"

// *************************************************************************

SO_ELEMENT_SOURCE(SoGLLightIdElement);

// *************************************************************************

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLLightIdElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLLightIdElement, inherited);
}

/*!
  Destructor.
*/

SoGLLightIdElement::~SoGLLightIdElement(void)
{
}

//! FIXME: write doc.

void
SoGLLightIdElement::init(SoState * state)
{
  inherited::init(state);
  this->data = getDefault();
}

//! FIXME: write doc.

void
SoGLLightIdElement::push(SoState * state)
{
  inherited::push(state);
  this->data = ((SoGLLightIdElement*)this->getNextInStack())->data;
}

//! FIXME: write doc.

void
SoGLLightIdElement::pop(SoState * state,
                        const SoElement * prevTopElement)
{
  // capture element since we change the GL state here
  this->capture(state);

  int idx = this->data + 1;
  int prevdata = ((SoGLLightIdElement*)prevTopElement)->data;
  // disable used light sources
  while (idx <= prevdata) {
    glDisable((GLenum)((int32_t)GL_LIGHT0 + idx));
    idx++;
  }
}

//! FIXME: write doc.

int32_t
SoGLLightIdElement::increment(SoState * const state,
                              SoNode * const /* node */)
{
  SoGLLightIdElement * element = (SoGLLightIdElement *)
    getElement(state, getClassStackIndex());

  if (element) {
    const cc_glglue * glue = sogl_glue_instance(state);
    element->data++;
    int maxl = cc_glglue_get_max_lights(glue);

    if (element->data >= maxl) {
      element->data--;
#if COIN_DEBUG
      static SbBool warn = TRUE;

      if (warn) { // warn only once
        warn = FALSE;
        SoDebugError::postWarning("SoGLLightIdElement::increment",
                                  "Number of concurrent light sources in "
                                  "scene exceeds %d, which is the maximum "
                                  "number of concurrent light sources "
                                  "supported by this OpenGL implementation. "
                                  "Some light sources will be ignored.\n\n"

                                  "(Note to application "
                                  "programmers: this error is often caused by "
                                  "a missing SoState::pop() call in extension "
                                  "shape nodes -- audit your GLRender() "
                                  "method(s)).",

                                  maxl);
      }
#endif
      return -1;
    }
    glEnable((GLenum)((int32_t)GL_LIGHT0 + element->data));

    return element->data;
  }
  return -1;
}

//! FIXME: write doc.

int32_t
SoGLLightIdElement::getMaxGLSources(void)
{
  // FIXME: should also make a likewise method available as part of
  // the So*GLWidget classes. 20020802 mortene.

  SoDebugError::postWarning("SoGLLightIdElement::getMaxGLSources",
                            "This function is obsoleted. It should not "
                            "be used because its interface is fubar: "
                            "the number of light sources available from "
                            "the OpenGL driver depends on the context, and "
                            "this function does not know which context this "
                            "information is requested for.");

  GLint val;
  glGetIntegerv(GL_MAX_LIGHTS, &val);

  GLenum err = sogl_glerror_debugging() ? glGetError() : GL_NO_ERROR;
  assert(err == GL_NO_ERROR &&
         "GL error when calling glGetInteger() -- no current GL context?");

  return (int32_t)val;
}

//! FIXME: write doc.

int32_t
SoGLLightIdElement::increment(SoState * const state)
{
  return increment(state, NULL);
}

//! FIXME: write doc.

int32_t
SoGLLightIdElement::get(SoState * const state)
{
  return SoInt32Element::get(classStackIndex, state);
}

//! FIXME: write doc.

int32_t
SoGLLightIdElement::getDefault()
{
  return -1;
}
