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
  \class SoGLUpdateAreaElement Inventor/elements/SoGLUpdateAreaElement.h
  \brief The SoGLUpdateAreaElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoGLUpdateAreaElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/system/gl.h>

/*!
  \fn SoGLUpdateAreaElement::origin

  FIXME: write doc
*/

/*!
  \fn SoGLUpdateAreaElement::size

  FIXME: write doc
*/

SO_ELEMENT_SOURCE(SoGLUpdateAreaElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLUpdateAreaElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLUpdateAreaElement, inherited);
}

/*!
  Destructor.
*/

SoGLUpdateAreaElement::~SoGLUpdateAreaElement()
{
}

// doc from parent
void
SoGLUpdateAreaElement::init(SoState * state)
{
  inherited::init(state);
  this->origin = getDefaultOrigin();
  this->size = getDefaultSize();

  // set these to dummy values. scissor test will be disabled
  this->screensize.setValue(0, 0);
  this->screenorigin.setValue(0, 0);

  // scissorstate is used to keep track of current scissor
  // test state.
  this->scissorstate = FALSE;

  // disabled by default
  glDisable(GL_SCISSOR_TEST);
}

// doc from parent
void
SoGLUpdateAreaElement::push(SoState * state)
{
  inherited::push(state);
  SoGLUpdateAreaElement * prev = (SoGLUpdateAreaElement*)
    this->getNextInStack();
  this->scissorstate = prev->scissorstate;
}

// doc from parent
void
SoGLUpdateAreaElement::pop(SoState * state,
                           const SoElement * prevTopElement)
{
  this->scissorstate = ((SoGLUpdateAreaElement*)prevTopElement)->scissorstate;
  this->updategl();
  inherited::pop(state, prevTopElement);
}

// doc from parent
SbBool
SoGLUpdateAreaElement::matches(const SoElement * element) const
{
  const SoGLUpdateAreaElement * elem = (SoGLUpdateAreaElement*) element;
  return
    this->origin == elem->origin &&
    this->size == elem->size;
}

// doc from parent
SoElement *
SoGLUpdateAreaElement::copyMatchInfo() const
{
  SoGLUpdateAreaElement * elem = (SoGLUpdateAreaElement*) this->getTypeId().createInstance();
  elem->origin = this->origin;
  elem->size = this->size;
  return elem;
}


/*!
  Sets the update area. This can, for instance, be used when
  rendering in the front buffer, to only render parts of the scene
  during a window expose event.  \a origin and \a size is in
  normalized window coordinates.
*/
void
SoGLUpdateAreaElement::set(SoState * const state,
                           const SbVec2f & origin,
                           const SbVec2f & size)
{
  SoGLUpdateAreaElement * e = (SoGLUpdateAreaElement *)
    inherited::getElement(state, SoGLUpdateAreaElement::classStackIndex);
  if (e) {
    e->origin = origin;
    e->size = size;
    SbVec2s winsize = SoViewportRegionElement::get(state).getWindowSize();
    e->screenorigin[0] = (short) (origin[0]*float(winsize[0]));
    e->screenorigin[1] = (short) (origin[1]*float(winsize[1]));
    e->screensize[0] = (short) (size[0]*float(winsize[0]));
    e->screensize[1] = (short) (size[1]*float(winsize[1]));

    e->updategl();
  }
}


/*!
  Returns the current update area.

  \sa set()
*/
SbBool
SoGLUpdateAreaElement::get(SoState * const state,
                           SbVec2f & origin,
                           SbVec2f & size)
{
  const SoGLUpdateAreaElement * e = (const SoGLUpdateAreaElement *)
    inherited::getConstElement(state, SoGLUpdateAreaElement::classStackIndex);
  origin = e->origin;
  size = e->size;

  return e->isDefault();
}

/*!
  Returns the default update area origin, (0,0).
*/
SbVec2f
SoGLUpdateAreaElement::getDefaultOrigin(void)
{
  return SbVec2f(0.0f, 0.0f);
}

/*!
  Returns the default update area size, (1,1).
*/
SbVec2f
SoGLUpdateAreaElement::getDefaultSize(void)
{
  return SbVec2f(1.0f, 1.0f);
}

// return TRUE if element contains the default values
SbBool
SoGLUpdateAreaElement::isDefault(void) const
{
  return
    this->origin == getDefaultOrigin() &&
    this->size == getDefaultSize();
}

//
// keeps GL in sync
//
void
SoGLUpdateAreaElement::updategl(void)
{
  if (this->isDefault()) {
    if (this->scissorstate) {
      glDisable(GL_SCISSOR_TEST);
      this->scissorstate = FALSE;
    }
  }
  else {
    if (!this->scissorstate) {
      glEnable(GL_SCISSOR_TEST);
      this->scissorstate = TRUE;
    }
    glScissor((GLint) this->screenorigin[0],
              (GLint) this->screenorigin[1],
              (GLint) this->screensize[0],
              (GLint) this->screensize[1]);
  }
}
