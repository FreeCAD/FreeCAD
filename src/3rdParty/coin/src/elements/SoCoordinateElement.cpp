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
  \class SoCoordinateElement Inventor/elements/SoCoordinateElement.h
  \brief The SoCoordinateElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoCoordinateElement.h>

#include <cassert>

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/elements/SoGLVBOElement.h>

#include "tidbitsp.h"
#include "SbBasicP.h"

// static variables
SbVec3f * SoCoordinateElement::initialdefaultcoords = NULL;

/*!
  \fn SoCoordinateElement::numCoords

  FIXME: write doc.
*/

/*!
  \fn SoCoordinateElement::coords3D

  FIXME: write doc.
*/

/*!
  \fn SoCoordinateElement::coords4D

  FIXME: write doc.
*/

/*!
  \fn SoCoordinateElement::areCoords3D

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoCoordinateElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoCoordinateElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoCoordinateElement, inherited);

  SoCoordinateElement::initialdefaultcoords = new SbVec3f(0.0f, 0.0f, 0.0f);

  coin_atexit(reinterpret_cast<coin_atexit_f *>(SoCoordinateElement::clean), CC_ATEXIT_NORMAL);
}

// Clean up internal resource usage.
void
SoCoordinateElement::clean(void)
{
#if COIN_DEBUG
  delete SoCoordinateElement::initialdefaultcoords;
#endif // COIN_DEBUG
}

/*!
  Destructor.
*/

SoCoordinateElement::~SoCoordinateElement(void)
{
}

//! FIXME: write doc.

void
SoCoordinateElement::init(SoState * state)
{
  inherited::init(state);
  this->numCoords = 1;
  this->coords3D = SoCoordinateElement::initialdefaultcoords;
  this->coords4D = NULL;
  this->areCoords3D = TRUE;
}

//! FIXME: write doc.

void
SoCoordinateElement::set3(SoState * const state,
                          SoNode * const node,
                          const int32_t numCoords,
                          const SbVec3f * const coords)
{
  if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
    SoGLVBOElement::setVertexVBO(state, NULL);
  }
  SoCoordinateElement * elem =
    coin_safe_cast<SoCoordinateElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );

  if (elem) {
    elem->areCoords3D = TRUE;
    elem->coords3D = coords;
    elem->numCoords = numCoords;
    elem->nodeId = node->getNodeId();
  }
}

//! FIXME: write doc.

void
SoCoordinateElement::set4(SoState * const state,
                          SoNode * const node,
                          const int32_t numCoords,
                          const SbVec4f * const coords)
{
  if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
    SoGLVBOElement::setVertexVBO(state, NULL);
  }
  SoCoordinateElement * elem = coin_safe_cast<SoCoordinateElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->areCoords3D = FALSE;
    elem->coords4D = coords;
    elem->numCoords = numCoords;
    elem->nodeId = node->getNodeId();
  }
}

//! FIXME: write doc.

const SbVec3f &
SoCoordinateElement::get3(const int index) const
{
  assert(index >= 0 && index < this->numCoords);

  if (areCoords3D) return this->coords3D[index];
  else {
    // hack around const
    SoCoordinateElement  * elem = const_cast<SoCoordinateElement *>(this);
    this->coords4D[index].getReal(elem->dummy3D);
    return this->dummy3D;
  }
}

//! FIXME: write doc.

const SbVec4f &
SoCoordinateElement::get4(const int index) const
{
  assert(index >= 0 && index < this->numCoords);

  if (!areCoords3D) return this->coords4D[index];
  else {
    // hack around const
    SoCoordinateElement * elem = const_cast<SoCoordinateElement *>(this);
    const SbVec3f &vec = this->coords3D[index];
    elem->dummy4D[0] = vec[0];
    elem->dummy4D[1] = vec[1];
    elem->dummy4D[2] = vec[2];
    elem->dummy4D[3] = 1.0f;
    return this->dummy4D;
  }
}

//! FIXME: write doc.

const SoCoordinateElement *
SoCoordinateElement::getInstance(SoState * const state)
{
  return coin_assert_cast<const SoCoordinateElement * >
    (
     getConstElement(state, classStackIndex)
     );
}

//! FIXME: write doc.

//$ EXPORT INLINE
int32_t
SoCoordinateElement::getNum(void) const
{
  return this->numCoords;
}

//! FIXME: write doc.

//$ EXPORT INLINE
SbBool
SoCoordinateElement::is3D() const
{
  return this->areCoords3D;
}

/*!
  Returns a pointer to the 3D coordinate array. Don't use this method
  unless SoCoordinateElement::is3D() returns \c TRUE.

  This method is not part of the original SGI Open Inventor v2.1 API.

  \since Coin 1.0
*/
const SbVec3f *
SoCoordinateElement::getArrayPtr3() const
{
#if COIN_DEBUG
  if (!this->is3D()) {
    SoDebugError::postWarning("SoDiffuseColorElement::getArrayPtr3",
                              "coordinates are *not* 3D -- use "
                              "getArrayPtr4() instead");
  }
#endif // COIN_DEBUG

  return this->coords3D;
}

/*!
  Returns a pointer to the 4D coordinate array. Don't use this method
  unless SoCoordinateElement::is3D() returns \c FALSE.

  This method is not part of the original SGI Open Inventor v2.1 API.

  \since Coin 1.0
*/
const SbVec4f *
SoCoordinateElement::getArrayPtr4() const
{
#if COIN_DEBUG
  if (this->is3D()) {
    SoDebugError::postWarning("SoDiffuseColorElement::getArrayPtr4",
                              "coordinates are *not* 4D -- use "
                              "getArrayPtr3() instead");
  }
#endif // COIN_DEBUG

  return this->coords4D;
}

//! FIXME: write doc.

//$ EXPORT INLINE
SbVec3f
SoCoordinateElement::getDefault3()
{
  return SbVec3f(0, 0, 0);
}

//! FIXME: write doc.

//$ EXPORT INLINE
SbVec4f
SoCoordinateElement::getDefault4()
{
  return SbVec4f(0, 0, 0, 1);
}

//! FIXME: write doc.

void
SoCoordinateElement::print(FILE * /* file */) const
{
}
