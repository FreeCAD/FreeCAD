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
  \class SoGLMultiTextureEnabledElement Inventor/elements/SoGLMultiTextureEnabledElement.h
  \brief The SoGLMultiTextureEnabledElement class is an element which controls whether texturing is enabled or not.

  \ingroup coin_elements
*/

#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include "coindefs.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/system/gl.h>
#include <Inventor/C/glue/gl.h>
#include <cassert>
#include "rendering/SoGL.h"

SO_ELEMENT_SOURCE(SoGLMultiTextureEnabledElement);

/*!
  \copydetails SoElement::initClass(void)
*/
void
SoGLMultiTextureEnabledElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLMultiTextureEnabledElement, inherited);
}

/*!
  Destructor.
*/
SoGLMultiTextureEnabledElement::~SoGLMultiTextureEnabledElement(void)
{
}

// doc from parent
void
SoGLMultiTextureEnabledElement::init(SoState * state)
{
  inherited::init(state);

  SoAction * action = state->getAction();
  assert(action->isOfType(SoGLRenderAction::getClassTypeId()));
  this->cachecontext = ((SoGLRenderAction*)action)->getCacheContext();
}

// Documented in superclass. Overridden to track GL state.
void
SoGLMultiTextureEnabledElement::push(SoState * state)
{
  SoGLMultiTextureEnabledElement * prev = (SoGLMultiTextureEnabledElement*) this->getNextInStack();

  this->cachecontext = prev->cachecontext;

  // copy state from previous element
  inherited::push(state);
  // capture previous element since we might or might not change the
  // GL state in set/pop
  prev->capture(state);
}

// Documented in superclass. Overridden to track GL state.
void
SoGLMultiTextureEnabledElement::pop(SoState * COIN_UNUSED_ARG(state),
                                    const SoElement * prevTopElement)
{
  SoGLMultiTextureEnabledElement * prev = (SoGLMultiTextureEnabledElement*) prevTopElement;
  const int maxunits = SbMax(this->getMaxUnits(), prev->getMaxUnits());
  
  for (int i = 0; i < maxunits; i++) {
    Mode oldmode = prev->getMode(i);
    Mode newmode =  this->getMode(i);
    if (oldmode != newmode) {
      this->updategl(i, newmode, oldmode);
    }
  }
}

void
SoGLMultiTextureEnabledElement::setElt(const int unit, const int value)
{
  Mode oldmode = this->getMode(unit);
  Mode newmode = (Mode) value;

  if (oldmode != newmode) {
    inherited::setElt(unit, value);
    this->updategl(unit, newmode, oldmode);
  }
}

//
// updates GL state
//
void
SoGLMultiTextureEnabledElement::updategl(const int unit)
{
  const cc_glglue * glue = cc_glglue_instance(this->cachecontext);
  cc_glglue_glActiveTexture(glue, (GLenum) (int(GL_TEXTURE0) + unit));
  if (this->isEnabled(unit)) glEnable(GL_TEXTURE_2D);
  else glDisable(GL_TEXTURE_2D);
  cc_glglue_glActiveTexture(glue, (GLenum) GL_TEXTURE0);

  GLenum glerror =  sogl_glerror_debugging() ? glGetError() : GL_NO_ERROR;
  while (glerror) {
    SoDebugError::postWarning("SoGLMultiTextureEnabledElement::updategl",
                              "glError() = %d\n", glerror);
    glerror = glGetError();
  }
}

void
SoGLMultiTextureEnabledElement::updategl(const int unit, const Mode newvalue, const Mode oldvalue)
{
  const cc_glglue * glue = cc_glglue_instance(this->cachecontext);
  cc_glglue_glActiveTexture(glue, (GLenum) (int(GL_TEXTURE0) + unit));

  switch (oldvalue) {
  case DISABLED:
    break;
  case TEXTURE2D:
    glDisable(GL_TEXTURE_2D);
    break;
  case RECTANGLE:
    glDisable(GL_TEXTURE_RECTANGLE_EXT);
    break;
  case CUBEMAP:
    glDisable(GL_TEXTURE_CUBE_MAP);
    break;
  case TEXTURE3D:
    glDisable(GL_TEXTURE_3D);
    break;
  default:
    assert(0 && "should not happen");
    break;
  }
  switch (newvalue) {
  case DISABLED:
    break;
  case TEXTURE2D:
    glEnable(GL_TEXTURE_2D);
    break;
  case RECTANGLE:
    glEnable(GL_TEXTURE_RECTANGLE_EXT);
    break;
  case CUBEMAP:
    glEnable(GL_TEXTURE_CUBE_MAP);
    break;
  case TEXTURE3D:
    glEnable(GL_TEXTURE_3D);
    break;
  default:
    assert(0 && "should not happen");
    break;
  }
  cc_glglue_glActiveTexture(glue, (GLenum) GL_TEXTURE0);

  GLenum glerror =  sogl_glerror_debugging() ? glGetError() : GL_NO_ERROR;
  while (glerror) {
    SoDebugError::postWarning("SoGLMultiTextureEnabledElement::updategl",
                              "glError() = %d\n", glerror);
    glerror = glGetError();
  }
}

