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
  \class SoGLClipPlaneElement Inventor/elements/SoGLClipPlaneElement.h
  \brief The SoGLClipPlaneElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoGLClipPlaneElement.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/system/gl.h>
#include <Inventor/errors/SoDebugError.h>

#include "rendering/SoGL.h"

// *************************************************************************

SO_ELEMENT_SOURCE(SoGLClipPlaneElement);

// *************************************************************************

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoGLClipPlaneElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLClipPlaneElement, inherited);
}

/*!
  Destructor.
*/

SoGLClipPlaneElement::~SoGLClipPlaneElement(void)
{
}

//! FIXME: write doc.

void
SoGLClipPlaneElement::init(SoState * state)
{
  inherited::init(state);
}

//! FIXME: write doc.

void
SoGLClipPlaneElement::pop(SoState * state,
                          const SoElement * prevTopElement)
{
  this->capture(state);
  const SoGLClipPlaneElement * prev = (const SoGLClipPlaneElement*)
    prevTopElement;

  // disable used planes
  for (int i = prev->startIndex; i < prev->getNum(); i++)
    glDisable((GLenum)((int)GL_CLIP_PLANE0 + i));

  inherited::pop(state, prevTopElement);
}

//! FIXME: write doc.

int
SoGLClipPlaneElement::getMaxGLPlanes(void)
{
  // FIXME: should also make a likewise method available as part of
  // the So*GLWidget classes. 20020802 mortene.

  SoDebugError::postWarning("SoGLClipPlaneElement::getMaxGLPlanes",
                            "This function is obsoleted. It should not "
                            "be used because its interface is fubar: "
                            "the number of clip planes available from "
                            "the OpenGL driver depends on the context, and "
                            "this function does not know which context this "
                            "information is requested for.");

  GLint val;
  glGetIntegerv(GL_MAX_CLIP_PLANES, &val);

  GLenum err = sogl_glerror_debugging() ? glGetError() : GL_NO_ERROR;
  assert(err == GL_NO_ERROR &&
         "GL error when calling glGetInteger() -- no current GL context?");

  return (int)val;
}

//! FIXME: write doc.

void
SoGLClipPlaneElement::addToElt(const SbPlane & plane,
                               const SbMatrix & modelMatrix)
{
  int idxadd = getNum(); // num planes before this one
  inherited::addToElt(plane, modelMatrix); // store plane
  SbVec3f norm = plane.getNormal();
  double equation[4];
  equation[0] = norm[0];
  equation[1] = norm[1];
  equation[2] = norm[2];
  equation[3] = - plane.getDistanceFromOrigin();
  glClipPlane((GLenum)((int)GL_CLIP_PLANE0 + idxadd), equation);
  glEnable((GLenum)((int)GL_CLIP_PLANE0 + idxadd));
}
