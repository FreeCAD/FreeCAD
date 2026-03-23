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
  \class SoBumpMapMatrixElement Inventor/elements/SoBumpMapMatrixElement.h
  \brief The SoBumpMapMatrixElement class is used to manage the bump map matrix stack.

  \ingroup coin_elements

  The bump map matrix is used to transform bump map coordinates before
  being used to map bump maps onto polygons.
*/

#include <Inventor/elements/SoBumpMapMatrixElement.h>
#include "coindefs.h" // COIN_OBSOLETED()
#include "SbBasicP.h"

/*!
  \fn SoBumpMapMatrixElement::bumpMapMatrix

  The matrix.
*/

SO_ELEMENT_SOURCE(SoBumpMapMatrixElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoBumpMapMatrixElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoBumpMapMatrixElement, inherited);
}

/*!
  Destructor.
*/
SoBumpMapMatrixElement::~SoBumpMapMatrixElement(void)
{
}

/*!
  Sets current texture matrix to identity.
*/
void
SoBumpMapMatrixElement::makeIdentity(SoState * const state,
                                     SoNode * const node)
{
  SoBumpMapMatrixElement * elem = coin_assert_cast<SoBumpMapMatrixElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->makeEltIdentity();
    if (node) elem->setNodeId(node);
  }
}

/*!
  Sets the current texture matrix to \a matrix.

  This method is an extension versus the Open Inventor API.
*/
void
SoBumpMapMatrixElement::set(SoState * const state,
                            SoNode * const node,
                            const SbMatrix & matrix)
{
  SoBumpMapMatrixElement * elem =
       coin_assert_cast<SoBumpMapMatrixElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->setElt(matrix);
    if (node) elem->setNodeId(node);
  }
}

/*!
  Multiplies \a matrix into the current texture matrix.
*/
void
SoBumpMapMatrixElement::mult(SoState * const state,
                             SoNode * const node,
                             const SbMatrix & matrix)
{
  SoBumpMapMatrixElement * elem = coin_assert_cast< SoBumpMapMatrixElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->multElt(matrix);
    if (node) elem->addNodeId(node);
  }
}


/*!
  Appends \a translation to the current texture matrix.
*/
void
SoBumpMapMatrixElement::translateBy(SoState * const state,
                                    SoNode * const node,
                                    const SbVec3f & translation)
{
  SoBumpMapMatrixElement * elem = coin_assert_cast< SoBumpMapMatrixElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->translateEltBy(translation);
    if (node) elem->addNodeId(node);
  }
}

/*!
  Appends \a rotation to the current texture matrix.
*/
void
SoBumpMapMatrixElement::rotateBy(SoState * const state,
                                 SoNode * const node,
                                 const SbRotation & rotation)
{
  SoBumpMapMatrixElement * elem = coin_assert_cast<SoBumpMapMatrixElement*>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->rotateEltBy(rotation);
    if (node) elem->addNodeId(node);
  }
}

/*!
  Appends \a scaleFactor to the current texture matrix.
*/
void
SoBumpMapMatrixElement::scaleBy(SoState * const state,
                                SoNode * const node,
                                const SbVec3f & scaleFactor)
{
  SoBumpMapMatrixElement * elem = coin_assert_cast<SoBumpMapMatrixElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    elem->scaleEltBy(scaleFactor);
    if (node) elem->addNodeId(node);
  }
}


/*!
  Returns current texture matrix.
*/
const SbMatrix &
SoBumpMapMatrixElement::get(SoState * const state)
{
  const SoBumpMapMatrixElement * elem = coin_assert_cast<const SoBumpMapMatrixElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return elem->getElt();
}

/*!
  virtual method which is called from makeIdentity().
  Sets element matrix to identity.
*/
void
SoBumpMapMatrixElement::makeEltIdentity(void)
{
  this->bumpMapMatrix.makeIdentity();
}

/*!
  virtual method which is called from set(). Sets the texture matrix
  to \a matrix.

  This method is an extension versus the Open Inventor API.
*/
void
SoBumpMapMatrixElement::setElt(const SbMatrix & matrix)
{
  this->bumpMapMatrix = matrix;
}

/*!
  virtual method which is called from mult(). Multiplies \a matrix
  into element matrix.
*/
void
SoBumpMapMatrixElement::multElt(const SbMatrix & matrix)
{
  this->bumpMapMatrix.multLeft(matrix);
}

/*!
  virtual method which is called from translateBy().
  Appends \a translation to the element matrix.
*/
void
SoBumpMapMatrixElement::translateEltBy(const SbVec3f & translation)
{
  SbMatrix matrix = SbMatrix::identity();
  matrix.setTranslate(translation);
  this->bumpMapMatrix.multLeft(matrix);
}

/*!
  virtual method which is called from rotateBy().
  Appends \a rotation to the element matrix.
*/
void
SoBumpMapMatrixElement::rotateEltBy(const SbRotation & rotation)
{
  SbMatrix matrix = SbMatrix::identity();
  matrix.setRotate(rotation);
  this->bumpMapMatrix.multLeft(matrix);
}

/*!
  virtual method which is called from scaleBy().
  Append \a scaleFactor to the element matrix.
*/
void
SoBumpMapMatrixElement::scaleEltBy(const SbVec3f & scaleFactor)
{
  SbMatrix matrix = SbMatrix::identity();
  matrix.setScale(scaleFactor);
  this->bumpMapMatrix.multLeft(matrix);
}

/*!
  Returns element matrix. Called from get().
*/
const SbMatrix &
SoBumpMapMatrixElement::getElt(void) const
{
  return this->bumpMapMatrix;
}

// doc from parent
void
SoBumpMapMatrixElement::init(SoState * state)
{
  inherited::init(state);
  this->bumpMapMatrix.makeIdentity();
  this->clearNodeIds();
}

// Documented in superclass. Overridden to copy current matrix and
// update accumulated node ids.
void
SoBumpMapMatrixElement::push(SoState * state)
{
  inherited::push(state);

  const SoBumpMapMatrixElement * prev =
    coin_assert_cast<const SoBumpMapMatrixElement *>(this->getNextInStack());
  this->bumpMapMatrix = prev->bumpMapMatrix;

  // make sure node ids are accumulated properly
  this->copyNodeIds(prev);
}
