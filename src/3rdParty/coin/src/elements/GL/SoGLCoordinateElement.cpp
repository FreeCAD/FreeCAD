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
  \class SoGLCoordinateElement Inventor/elements/SoGLCoordinateElement.h
  \brief The SoGLCoordinateElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoGLCoordinateElement.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <Inventor/system/gl.h>

#include <cassert>

SO_ELEMENT_SOURCE(SoGLCoordinateElement);

/*!
  \copydetails SoElement::initClass(void)
*/
void
SoGLCoordinateElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoGLCoordinateElement, inherited);
}

/*!
  Destructor.
*/
SoGLCoordinateElement::~SoGLCoordinateElement(void)
{
}

/*!
  Send coordinates \a index to GL. Handles both 3D and 4D coordinates.
*/
void
SoGLCoordinateElement::send(const int index) const
{
  if (this->areCoords3D) glVertex3fv((const GLfloat*)(this->coords3D + index));
  else glVertex4fv((const GLfloat*)(this->coords4D + index));
}

//! FIXME: write doc.

//$ EXPORT INLINE
const SbVec3f *
SoGLCoordinateElement::getPtr3() const
{
  return this->coords3D;
}

//! FIXME: write doc.

//$ EXPORT INLINE
const SbVec4f *
SoGLCoordinateElement::getPtr4() const
{
  return this->coords4D;
}
