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
  \class SoGLVBOElement Inventor/elements/SoGLVBOElement.h
  \brief The SoGLVBOElement class is used to store VBO state.

  \ingroup coin_elements

  FIXME: write doc.

  \COIN_CLASS_EXTENSION

  \since Coin 2.5
*/

#include <Inventor/elements/SoGLVBOElement.h>

#include <cassert>

#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/C/glue/gl.h>
#include <Inventor/misc/SoGLDriverDatabase.h>

#include "rendering/SoGL.h"
#include "glue/glp.h"
#include "rendering/SoVBO.h"

#define PRIVATE(obj) obj->pimpl

class SoGLVBOElementP {
 public:

  SoVBO * vertexvbo;
  SoVBO * normalvbo;
  SoVBO * colorvbo;
  SbList <SoVBO*> texcoordvbo;
};

SO_ELEMENT_CUSTOM_CONSTRUCTOR_SOURCE(SoGLVBOElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLVBOElement::initClass()
{
  SO_ELEMENT_INIT_CLASS(SoGLVBOElement, inherited);
}


/*!
  Constructor.
*/
SoGLVBOElement::SoGLVBOElement(void)
{
  PRIVATE(this) = new SoGLVBOElementP;

  this->setTypeId(SoGLVBOElement::classTypeId);
  this->setStackIndex(SoGLVBOElement::classStackIndex);
}

/*!
  Destructor.
*/

SoGLVBOElement::~SoGLVBOElement()
{
  delete PRIVATE(this);
}

/*!
  Sets the vertex VBO.
*/
void
SoGLVBOElement::setVertexVBO(SoState * state, SoVBO * vbo)
{
  SoGLVBOElement * elem = getElement(state);
  PRIVATE(elem)->vertexvbo = vbo;
}

/*!
  Sets the normal VBO.
*/
void
SoGLVBOElement::setNormalVBO(SoState * state, SoVBO * vbo)
{
  SoGLVBOElement * elem = getElement(state);
  PRIVATE(elem)->normalvbo = vbo;
}

/*!
  Sets the color VBO.
*/
void
SoGLVBOElement::setColorVBO(SoState * state, SoVBO * vbo)
{
  SoGLVBOElement * elem = getElement(state);
  PRIVATE(elem)->colorvbo = vbo;
}

/*!
  Sets the texture coordinate VBO.
*/
void
SoGLVBOElement::setTexCoordVBO(SoState * state, const int unit, SoVBO * vbo)
{
  SoGLVBOElement * elem = getElement(state);
  const int n = PRIVATE(elem)->texcoordvbo.getLength();
  for (int i = n; i <= unit; i++) {
    PRIVATE(elem)->texcoordvbo.append(NULL);
  }
  PRIVATE(elem)->texcoordvbo[unit] = vbo;
}

// doc in parent
void
SoGLVBOElement::init(SoState * COIN_UNUSED_ARG(state))
{
  PRIVATE(this)->vertexvbo = NULL;
  PRIVATE(this)->normalvbo = NULL;
  PRIVATE(this)->colorvbo = NULL;
  PRIVATE(this)->texcoordvbo.truncate(0);
}

// doc in parent
void
SoGLVBOElement::push(SoState * COIN_UNUSED_ARG(state))
{
  SoGLVBOElement * prev = (SoGLVBOElement *)
    this->getNextInStack();

  PRIVATE(this)->vertexvbo = PRIVATE(prev)->vertexvbo;
  PRIVATE(this)->normalvbo = PRIVATE(prev)->normalvbo;
  PRIVATE(this)->colorvbo = PRIVATE(prev)->colorvbo;
  PRIVATE(this)->texcoordvbo.truncate(0);

  for (int i = 0; i < PRIVATE(prev)->texcoordvbo.getLength(); i++) {
    PRIVATE(this)->texcoordvbo.append(PRIVATE(prev)->texcoordvbo[i]);
  }
}

// doc in parent
void
SoGLVBOElement::pop(SoState * COIN_UNUSED_ARG(state), const SoElement * COIN_UNUSED_ARG(prevtopelement))
{
  // nothing to do
}

// doc in parent
SbBool
SoGLVBOElement::matches(const SoElement * COIN_UNUSED_ARG(elt)) const
{
  assert(0 && "should never get here");
  return TRUE;
}

// doc in parent
SoElement *
SoGLVBOElement::copyMatchInfo(void) const
{
  assert(0 && "should never get here");
  return NULL;
}

/*!
  Returns a writable element instance.
*/
SoGLVBOElement *
SoGLVBOElement::getElement(SoState * state)
{
  return (SoGLVBOElement*) state->getElement(classStackIndex);
}

/*!
  Returns a read-only element instance.
*/
const SoGLVBOElement *
SoGLVBOElement::getInstance(SoState * state)
{
  return (SoGLVBOElement*) state->getConstElement(classStackIndex);
}

SoVBO *
SoGLVBOElement::getVertexVBO(void) const
{
  return PRIVATE(this)->vertexvbo;
}

SoVBO *
SoGLVBOElement::getNormalVBO(void) const
{
  return PRIVATE(this)->normalvbo;
}

SoVBO *
SoGLVBOElement::getColorVBO(void) const
{
  return PRIVATE(this)->colorvbo;
}

int
SoGLVBOElement::getNumTexCoordVBO(void) const
{
  return PRIVATE(this)->texcoordvbo.getLength();
}

SoVBO *
SoGLVBOElement::getTexCoordVBO(const int idx) const
{
  if (idx < PRIVATE(this)->texcoordvbo.getLength()) {
    return PRIVATE(this)->texcoordvbo[idx];
  }
  return NULL;
}

/*!
  Returns \a TRUE if VBO is supported for the current context,
  and if numdata is between the limits set for VBO rendering.

*/
SbBool
SoGLVBOElement::shouldCreateVBO(SoState * state, const int numdata)
{
  const cc_glglue * glue = sogl_glue_instance(state);
  // don't use SoGLCacheContextElement to find the current cache
  // context since we don't want this call to create a cache dependency
  // on SoGLCacheContextElement.
  return
    SoGLDriverDatabase::isSupported(glue, SO_GL_FRAMEBUFFER_OBJECT) &&
    SoVBO::shouldCreateVBO(state, glue->contextid, numdata);
}

#undef PRIVATE
