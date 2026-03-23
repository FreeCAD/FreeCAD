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
  \class SoModelMatrixElement Inventor/elements/SoModelMatrixElement.h
  \brief The SoModelMatrixElement class is used to manage the current transformation.

  \ingroup coin_elements

  SoModelMatrixElement contains the object-to-world matrix.

  The world-to-camera transformation is stored in the
  SoViewingMatrixElement class.

  Note that one thing that can be a little confusing with the API is
  that SoModelMatrixElement does not contain the same matrix as the
  OpenGL \c GL_MODELVIEW matrix.

  \sa SoViewingMatrixElement
*/

#include "coindefs.h"
#include "SbBasicP.h"

#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/SbVec3f.h>
#include <cassert>

// defines for the flags member
#define FLG_IDENTITY   0x1  // modelMatrix is identity
#define FLG_CULLMATRIX 0x2  // cullMatrix is set
#define FLG_COMBINED   0x4  // the combined matrix is calculated

/*!
  \fn SoModelMatrixElement::modelMatrix

  FIXME: write doc.
*/

/*!
  \fn SoModelMatrixElement::cullMatrix

  FIXME: write doc.
*/

/*!
  \fn SoModelMatrixElement::combinedMatrix

  FIXME: write doc.
*/


/*!
  \fn SoModelMatrixElement::flags

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoModelMatrixElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoModelMatrixElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoModelMatrixElement, inherited);
}

/*!
  Destructor.
*/

SoModelMatrixElement::~SoModelMatrixElement(void)
{
}

// doc from parent
SbBool
SoModelMatrixElement::matches(const SoElement * element) const
{
  return inherited::matches(element);
}

/*!
  Sets the current model matrix to the identity matrix.
*/
void
SoModelMatrixElement::makeIdentity(SoState * const state,
                                   SoNode * const node)
{
  SoModelMatrixElement * elem =
    coin_safe_cast<SoModelMatrixElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->makeEltIdentity();
    if (node) elem->setNodeId(node);
  }
}

/*!
  Sets the current model matrix to \a matrix.
*/
void
SoModelMatrixElement::set(SoState * const state,
                          SoNode * const node,
                          const SbMatrix & matrix)
{
  SoModelMatrixElement *elem =
    coin_safe_cast<SoModelMatrixElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );

  if (elem) {
    elem->setElt(matrix);
    if (node) elem->setNodeId(node);
  }
}

/*!
  Sets the current cull matrix.
*/
void
SoModelMatrixElement::setCullMatrix(SoState * state, SoNode * COIN_UNUSED_ARG(node),
                                    const SbMatrix & matrix)
{
  SoModelMatrixElement * elem =
    coin_safe_cast<SoModelMatrixElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );

  if (elem) {
    elem->cullMatrix = matrix;
    elem->flags |= FLG_CULLMATRIX;
    elem->flags &= ~FLG_COMBINED;
  }
}

/*!
  Multiplies \a matrix into the model matrix.
*/
void
SoModelMatrixElement::mult(SoState * const state,
                           SoNode * const node,
                           const SbMatrix &matrix)
{
  SoModelMatrixElement * elem = coin_safe_cast<SoModelMatrixElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );

  if (elem) {
    elem->multElt(matrix);
    if (node) elem->addNodeId(node);
  }
}


/*!
  Appends \a translation to the model matrix.
*/
void
SoModelMatrixElement::translateBy(SoState * const state,
                                  SoNode * const node,
                                  const SbVec3f &translation)
{
  SoModelMatrixElement * elem = coin_safe_cast<SoModelMatrixElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->translateEltBy(translation);
    if (node) elem->addNodeId(node);
  }
}


/*!
  Appends \a rotation to the model matrix.
*/
void
SoModelMatrixElement::rotateBy(SoState * const state,
                               SoNode * const node,
                               const SbRotation & rotation)
{
  SoModelMatrixElement * elem = coin_safe_cast<SoModelMatrixElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->rotateEltBy(rotation);
    if (node) elem->addNodeId(node);
  }
}

/*!
  Appends \a scaleFactor to the model matrix.
*/
void
SoModelMatrixElement::scaleBy(SoState * const state,
                              SoNode * const node,
                              const SbVec3f & scaleFactor)
{
  SoModelMatrixElement * elem = coin_safe_cast<SoModelMatrixElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );

  if (elem) {
    elem->scaleEltBy(scaleFactor);
    if (node) elem->addNodeId(node);
  }
}

/*!
  Used by SoTransformSeparator to store and restore model matrix.
  Don't use it for any other reason.
*/
SbMatrix
SoModelMatrixElement::pushMatrix(SoState * const state)
{
  // use SoState::getElementNoPush() instead of
  // SoElement::getConstElement() to avoid cache dependencies
  SoModelMatrixElement * elem = coin_assert_cast<SoModelMatrixElement *>
    (
     state->getElementNoPush(classStackIndex)
     );
  return elem->pushMatrixElt();
}

/*!
  Used by SoTransformSeparator to store and restore model matrix.
  Don't use it for any other reason.
*/
void
SoModelMatrixElement::popMatrix(SoState * const state,
                                const SbMatrix & matrix)
{
  // use SoState::getElementNoPush() instead of
  // SoElement::getConstElement() to avoid cache dependencies
  SoModelMatrixElement * elem = coin_assert_cast<SoModelMatrixElement *>
    (
     state->getElementNoPush(classStackIndex)
     );
  elem->popMatrixElt(matrix);
}

/*!
  Returns the combined cull and model matrix. This matrix
  is cached.
*/
const SbMatrix &
SoModelMatrixElement::getCombinedCullMatrix(SoState * const state)
{
  const SoModelMatrixElement * elem = coin_assert_cast<const SoModelMatrixElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  if (!(elem->flags & FLG_COMBINED)) {
    // Need to change this element, so cast away the const (_don't_
    // use the getElement() method, as it may invoke a
    // push()). --mortene
    SoModelMatrixElement * e = const_cast<SoModelMatrixElement *>(elem);
    e->combinedMatrix = e->modelMatrix;
    if (e->flags & FLG_CULLMATRIX)
      e->combinedMatrix.multRight(e->cullMatrix);
    e->flags |= FLG_COMBINED;
  }
  return elem->combinedMatrix;
}

/*!
  Returns the current model matrix.
*/
const SbMatrix &
SoModelMatrixElement::get(SoState * const state)
{
  const SoModelMatrixElement * elem = coin_assert_cast<const SoModelMatrixElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return elem->modelMatrix;
}

/*!
  Returns the current model matrix. Sets \a isIdentity to
  TRUE if the model matrix is known to be an identity matrix.
*/
const SbMatrix &
SoModelMatrixElement::get(SoState * const state,
                          SbBool & isIdentity)
{
  const SoModelMatrixElement * elem = coin_assert_cast<const SoModelMatrixElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  if (elem->flags & FLG_IDENTITY) isIdentity = TRUE;
  else isIdentity = FALSE;
  return elem->modelMatrix;
}

/*!
  virtual method which is called from the static method
  makeIdentity(). Sets element model matrix to identity.
*/
void
SoModelMatrixElement::makeEltIdentity(void)
{
  this->modelMatrix.makeIdentity();
  this->flags = FLG_IDENTITY;
}

/*!
  virtual method which is called from the static method
  set(). Sets element model matrix to \a matrix.
*/
void
SoModelMatrixElement::setElt(const SbMatrix & matrix)
{
  this->modelMatrix = matrix;
  this->flags &= ~(FLG_IDENTITY|FLG_COMBINED);
}

/*!
  virtual method which is called from the static method
  mult(). Multiplies \a matrix into element model matrix.
*/
void
SoModelMatrixElement::multElt(const SbMatrix & matrix)
{
  this->modelMatrix.multLeft(matrix);
  this->flags &= ~(FLG_IDENTITY|FLG_COMBINED);
}

/*!
  virtual method which is called from the static method
  translateBy(). Appends \a translation to element model matrix.
*/
void
SoModelMatrixElement::translateEltBy(const SbVec3f & translation)
{
  SbMatrix matrix = SbMatrix::identity();
  matrix.setTranslate(translation);
  this->modelMatrix.multLeft(matrix);
  this->flags &= ~(FLG_IDENTITY|FLG_COMBINED);
}

/*!
  virtual method which is called from the static method
  rotateBy(). Appends \a rotation to element model matrix.
*/
void
SoModelMatrixElement::rotateEltBy(const SbRotation & rotation)
{
  SbMatrix matrix = SbMatrix::identity();
  matrix.setRotate(rotation);
  this->modelMatrix.multLeft(matrix);
  this->flags &= ~(FLG_IDENTITY|FLG_COMBINED);
}

/*!
  virtual method which is called from the static method
  scaleBy(). Appends \a scaleFactor to element model matrix.
*/
void
SoModelMatrixElement::scaleEltBy(const SbVec3f & scaleFactor)
{
  SbMatrix matrix = SbMatrix::identity();
  matrix.setScale(scaleFactor);
  this->modelMatrix.multLeft(matrix);
  this->flags &= ~(FLG_IDENTITY|FLG_COMBINED);
}

/*!
  virtual method which is called from the static method
  pushMatrix(). Returns current model matrix.
*/
SbMatrix
SoModelMatrixElement::pushMatrixElt()
{
  return this->modelMatrix;
}

/*!
  virtual method which is called from the static method
  popMatrix(). Restores model matrix to the matrix returned from
  pushMatrix().
*/
void
SoModelMatrixElement::popMatrixElt(const SbMatrix & matrix)
{
  this->modelMatrix = matrix;
}

/*!
  Initializes the element to its default value. The default
  value for modelMatrix is the identity matrix.
*/
void
SoModelMatrixElement::init(SoState * state)
{
  inherited::init(state);
  this->modelMatrix.makeIdentity();
  this->flags = FLG_IDENTITY;
  this->clearNodeIds();
}

// Documented in superclass. Overridden to copy matrices and
// accumulate node ids.
void
SoModelMatrixElement::push(SoState * state)
{
  inherited::push(state);
  const SoModelMatrixElement * prev =
    coin_assert_cast<SoModelMatrixElement *>
    (
     this->getNextInStack()
     );

  this->modelMatrix = prev->modelMatrix;
  this->flags = prev->flags;
  // only copy when needed
  if (prev->flags & FLG_CULLMATRIX)
    this->cullMatrix = prev->cullMatrix;
  if (prev->flags & FLG_COMBINED)
    this->combinedMatrix = prev->combinedMatrix;

  // make sure node ids are accumulated properly
  this->copyNodeIds(prev);
}

/*!
  Returns the current model matrix.
*/
const SbMatrix &
SoModelMatrixElement::getModelMatrix(void) const
{
  return this->modelMatrix;
}

#undef FLG_IDENTITY
#undef FLG_CULLMATRIX
#undef FLG_COMBINED
