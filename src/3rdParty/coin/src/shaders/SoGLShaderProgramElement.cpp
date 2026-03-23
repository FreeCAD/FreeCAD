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
  \class SoGLShaderProgramElement Inventor/elements/SoGLShaderProgramElement.h
  \brief The SoGLShaderProgramElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoGLShaderProgramElement.h>

#include <cassert>

#include <Inventor/elements/SoGLCacheContextElement.h>
#include "SoGLShaderProgram.h"

// *************************************************************************

SO_ELEMENT_SOURCE(SoGLShaderProgramElement);

// *************************************************************************

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLShaderProgramElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLShaderProgramElement, inherited);
}

/*!
  Destructor.
*/

SoGLShaderProgramElement::~SoGLShaderProgramElement()
{
  this->shaderProgram = NULL;
}

void
SoGLShaderProgramElement::init(SoState *state)
{
  inherited::init(state);
  this->shaderProgram = NULL;
  this->enabled = FALSE;
}

void
SoGLShaderProgramElement::enable(SoState * const state, const SbBool onoff)
{
  SoGLShaderProgramElement* element =
    (SoGLShaderProgramElement*) SoElement::getElement(state,classStackIndex);
  element->enabled = onoff;
  element->objectids.truncate(0);
  
  if (element->shaderProgram) {
    if (onoff) {
      if (!element->shaderProgram->isEnabled()) element->shaderProgram->enable(state);
    }
    else {
      if (element->shaderProgram->isEnabled()) element->shaderProgram->disable(state);
    }
    element->shaderProgram->getShaderObjectIds(element->objectids);
  }
}

void
SoGLShaderProgramElement::set(SoState* const state, SoNode *const node,
                              SoGLShaderProgram* program)
{
  SoGLShaderProgramElement* element =
    (SoGLShaderProgramElement*)inherited::getElement(state,classStackIndex, node);
  
  if (program != element->shaderProgram) {
    if (element->shaderProgram) element->shaderProgram->disable(state);
  }
  element->shaderProgram = program;
  element->enabled = FALSE;
  element->objectids.truncate(0);
  if (program) program->getShaderObjectIds(element->objectids);
  // don't enable new program here. The node will call enable()
  // after setting up all the objects
}

SoGLShaderProgram *
SoGLShaderProgramElement::get(SoState *state)
{
  const SoElement *element = getConstElement(state, classStackIndex);
  assert(element);
  return ((const SoGLShaderProgramElement *)element)->shaderProgram;
}

void
SoGLShaderProgramElement::push(SoState * state)
{
  SoGLShaderProgramElement * prev = (SoGLShaderProgramElement *) getNextInStack();
  assert(prev);
  this->shaderProgram = prev->shaderProgram;
  this->enabled = prev->enabled;
  this->nodeId = prev->nodeId;
  this->objectids = prev->objectids;
  // capture previous element since we might or might not change the
  // GL state in set/pop
  prev->capture(state);
}

void
SoGLShaderProgramElement::pop(SoState * state, const SoElement * prevTopElement)
{
  SoGLShaderProgramElement * elem = (SoGLShaderProgramElement *)prevTopElement;
  if (this->shaderProgram != elem->shaderProgram) {
    if (elem->shaderProgram) {
      elem->shaderProgram->disable(state);
      elem->enabled = FALSE;
    }
    if (this->shaderProgram) {
      if (this->enabled) this->shaderProgram->enable(state);
    }
  }
  else if (this->shaderProgram) {
    if (this->enabled != elem->enabled) {
      if (this->enabled) this->shaderProgram->enable(state);
      else this->shaderProgram->disable(state);
    }
  }
  elem->shaderProgram = NULL;
}


SbBool
SoGLShaderProgramElement::matches(const SoElement * element) const
{
  SoGLShaderProgramElement * elem = (SoGLShaderProgramElement*) element;
  return (this->enabled == elem->enabled) && (this->objectids == elem->objectids);
}

SoElement *
SoGLShaderProgramElement::copyMatchInfo(void) const
{
  SoGLShaderProgramElement * elem = 
    (SoGLShaderProgramElement*) inherited::copyMatchInfo();
  
  elem->enabled = this->enabled;
  elem->objectids = this->objectids;
  return elem;
}
