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
  \class SoGLMultiTextureMatrixElement Inventor/elements/SoGLMultiTextureMatrixElement.h
  \brief The SoGLMultiTextureMatrixElement class is used to update the OpenGL texture matrix.

  \ingroup coin_elements

  Since (for some weird reason) most OpenGL implementations have a very
  small texture matrix stack, and since the matrix stack also is broken
  on many OpenGL implementations, the texture matrix is always loaded
  into OpenGL. We do not push() and pop() matrices.
*/

#include <Inventor/elements/SoGLMultiTextureMatrixElement.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/C/glue/gl.h>
#include <cassert>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/system/gl.h>

SO_ELEMENT_SOURCE(SoGLMultiTextureMatrixElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLMultiTextureMatrixElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLMultiTextureMatrixElement, inherited);
}

/*!
  Destructor.
*/

SoGLMultiTextureMatrixElement::~SoGLMultiTextureMatrixElement(void)
{
}

// doc from parent
void
SoGLMultiTextureMatrixElement::init(SoState * state)
{
  inherited::init(state);

  SoAction * action = state->getAction();
  assert(action->isOfType(SoGLRenderAction::getClassTypeId()));
  // fetch cache context from action since SoGLCacheContextElement
  // might not be initialized yet.
  SoGLRenderAction * glaction = (SoGLRenderAction*) action;
  this->cachecontext = glaction->getCacheContext();
}

// doc from parent
void
SoGLMultiTextureMatrixElement::push(SoState * state)
{
  inherited::push(state);
  SoGLMultiTextureMatrixElement * prev = (SoGLMultiTextureMatrixElement*)
    this->getNextInStack();

  this->cachecontext = prev->cachecontext;

  // capture previous element since we might or might not change the
  // GL state in set/pop
  prev->capture(state);
}

// doc from parent
void
SoGLMultiTextureMatrixElement::pop(SoState * state,
                                   const SoElement * prevTopElement)
{
  inherited::pop(state, prevTopElement);

  const SoGLMultiTextureMatrixElement * prev = 
    static_cast<const SoGLMultiTextureMatrixElement *> (prevTopElement);
  
  SbMatrix identity = SbMatrix::identity();
  const int numunits = SbMax(this->getNumUnits(),
                             prev->getNumUnits());
  for (int i = 0; i < numunits; i++) {
    const SbMatrix & thism = 
      (i < this->getNumUnits()) ?
      this->getUnitData(i).textureMatrix : identity;
    
    const SbMatrix & prevm = 
      (i < prev->getNumUnits()) ? 
      prev->getUnitData(i).textureMatrix : identity;
    
    if (thism != prevm) {
      this->updategl(i);
    }
  }
}

// doc from parent
void
SoGLMultiTextureMatrixElement::multElt(const int unit, const SbMatrix & matrix)
{
  inherited::multElt(unit, matrix);
  this->updategl(unit);
}

void
SoGLMultiTextureMatrixElement::setElt(const int unit, const SbMatrix & matrix)
{
  inherited::setElt(unit, matrix);
  this->updategl(unit);
}


// updates GL state
void
SoGLMultiTextureMatrixElement::updategl(const int unit) const
{
  const cc_glglue * glue = cc_glglue_instance(this->cachecontext);
  if (unit != 0) {
    cc_glglue_glActiveTexture(glue, (GLenum) (int(GL_TEXTURE0) + unit));
  }
  glMatrixMode(GL_TEXTURE);
  if (unit < this->getNumUnits()) {
    glLoadMatrixf(this->getUnitData(unit).textureMatrix[0]);
  }
  else {
    glLoadIdentity();
  }
  glMatrixMode(GL_MODELVIEW);
  if (unit != 0) {
    cc_glglue_glActiveTexture(glue, (GLenum) GL_TEXTURE0);
  }
}

