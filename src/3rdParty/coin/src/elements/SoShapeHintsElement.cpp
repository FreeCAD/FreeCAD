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
  \class SoShapeHintsElement Inventor/elements/SoShapeHintsElement.h
  \brief The SoShapeHintsElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoShapeHintsElement.h>

#include "SbBasicP.h"

#include <Inventor/elements/SoLazyElement.h>

#include <cassert>

SO_ELEMENT_SOURCE(SoShapeHintsElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoShapeHintsElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoShapeHintsElement, inherited);
}

/*!
  Destructor.
*/

SoShapeHintsElement::~SoShapeHintsElement(void)
{
}

/*!
  Initializes the element to its default values. The default
  value for vertexOrdering is SoShapeHintsElement::UNKNOWN_ORDERING,
  for shapeType is SoShapeHintsElement::UNKNOWN_SHAPE_TYPE, and
  for faceType is SoShapeHintsElement::CONVEX.
*/

void
SoShapeHintsElement::init(SoState * state)
{
  inherited::init(state);
  this->vertexOrdering = getDefaultVertexOrdering();
  this->shapeType = getDefaultShapeType();
  this->faceType = getDefaultFaceType();
}

//! FIXME: write doc.

void
SoShapeHintsElement::push(SoState * state)
{
  inherited::push(state);
  SoShapeHintsElement * prev = coin_assert_cast<SoShapeHintsElement *>
    (this->getNextInStack());
  this->vertexOrdering = prev->vertexOrdering;
  this->shapeType = prev->shapeType;
  this->faceType = prev->faceType;
}

void
SoShapeHintsElement::pop(SoState * state, const SoElement * prevtopelement)
{
  inherited::pop(state, prevtopelement);
}

//! FIXME: write doc.

SbBool
SoShapeHintsElement::matches(const SoElement * element) const
{
  const SoShapeHintsElement * elem =
    coin_assert_cast<const SoShapeHintsElement *>(element);
  return (this->vertexOrdering == elem->vertexOrdering &&
          this->shapeType == elem->shapeType &&
          this->faceType == elem->faceType);

}

//! FIXME: write doc.

SoElement *
SoShapeHintsElement::copyMatchInfo() const
{
  SoShapeHintsElement *elem = static_cast<SoShapeHintsElement *>
    (
     getTypeId().createInstance()
     );
  elem->vertexOrdering = this->vertexOrdering;
  elem->shapeType = this->shapeType;
  elem->faceType = this->faceType;
  return elem;
}

//! FIXME: write doc.

void
SoShapeHintsElement::set(SoState * const state,
                         SoNode * const /* node */,
                         const VertexOrdering vertexOrdering,
                         const ShapeType shapeType,
                         const FaceType faceType)
{
  SoShapeHintsElement * elem = coin_safe_cast<SoShapeHintsElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->setElt(vertexOrdering, shapeType, faceType);
    elem->updateLazyElement(state);
  }
}

//! FIXME: write doc.

void
SoShapeHintsElement::get(SoState * const state,
                         VertexOrdering & vertexOrdering,
                         ShapeType & shapeType,
                         FaceType & faceType)
{
  const SoShapeHintsElement * elem = coin_assert_cast<const SoShapeHintsElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  vertexOrdering = elem->vertexOrdering;
  shapeType = elem->shapeType;
  faceType = elem->faceType;
}

//! FIXME: write doc.

SoShapeHintsElement::VertexOrdering
SoShapeHintsElement::getVertexOrdering(SoState * const state)
{
  const SoShapeHintsElement * elem = coin_assert_cast<const SoShapeHintsElement*>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return elem->vertexOrdering;
}

//! FIXME: write doc.

SoShapeHintsElement::ShapeType
SoShapeHintsElement::getShapeType(SoState * const state)
{
  const SoShapeHintsElement * elem = coin_assert_cast<const SoShapeHintsElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return elem->shapeType;
}

//! FIXME: write doc.

SoShapeHintsElement::FaceType
SoShapeHintsElement::getFaceType(SoState * const state)
{
  const SoShapeHintsElement * elem = coin_assert_cast<const SoShapeHintsElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return elem->faceType;
}

//! FIXME: write doc.

void
SoShapeHintsElement::print(FILE * /* file */) const
{
}

//! FIXME: write doc.

void
SoShapeHintsElement::setElt(VertexOrdering vertexOrderingarg,
                            ShapeType shapeTypearg,
                            FaceType faceTypearg)
{
  if (vertexOrderingarg != ORDERING_AS_IS) {
    this->vertexOrdering = vertexOrderingarg;
  }
  if (shapeTypearg != SHAPE_TYPE_AS_IS) {
    this->shapeType = shapeTypearg;
  }
  if (faceTypearg != FACE_TYPE_AS_IS) {
    this->faceType = faceTypearg;
  }
}

//! FIXME: write doc.

//$ EXPORT INLINE
void SoShapeHintsElement::set(SoState * const state,
                              const VertexOrdering vertexOrdering,
                              const ShapeType shapeType,
                              const FaceType faceType)
{
  set(state, NULL, vertexOrdering, shapeType, faceType);
}

//! FIXME: write doc.

//$ EXPORT INLINE
SoShapeHintsElement::VertexOrdering
SoShapeHintsElement::getDefaultVertexOrdering()
{
  return UNKNOWN_ORDERING;
}

//! FIXME: write doc.

//$ EXPORT INLINE
SoShapeHintsElement::ShapeType
SoShapeHintsElement::getDefaultShapeType()
{
  return UNKNOWN_SHAPE_TYPE;
}

//! FIXME: write doc.

//$ EXPORT INLINE
SoShapeHintsElement::FaceType
SoShapeHintsElement::getDefaultFaceType()
{
  return CONVEX;
}

void
SoShapeHintsElement::updateLazyElement(SoState * state)
{
  if (state->isElementEnabled(SoLazyElement::getClassStackIndex())) {
    SoLazyElement::setVertexOrdering(state, this->vertexOrdering == CLOCKWISE ?
                                     SoLazyElement::CW : SoLazyElement::CCW);
    SoLazyElement::setTwosideLighting(state, this->vertexOrdering != UNKNOWN_ORDERING &&
                                      this->shapeType == UNKNOWN_SHAPE_TYPE);
    SoLazyElement::setBackfaceCulling(state, this->vertexOrdering != UNKNOWN_ORDERING &&
                                      this->shapeType == SOLID);
  }
}
