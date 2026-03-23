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
  \class SoMultiTextureMatrixElement Inventor/elements/SoMultiTextureMatrixElement.h
  \brief The SoMultiTextureMatrixElement class is used to manage the texture matrix stack for texture units &gt; 0.

  \ingroup coin_elements

  The texture matrix is used to transform texture coordinates before
  being used to map textures onto polygons.

  \since Coin 2.2
*/

#include "SbBasicP.h"

#include <Inventor/elements/SoMultiTextureMatrixElement.h>
#include <Inventor/lists/SbList.h>

#define PRIVATE(obj) obj->pimpl

class SoMultiTextureMatrixElementP {
public:
  void ensureCapacity(int unit) const {
    while (unit >= this->unitdata.getLength()) {
      this->unitdata.append(SoMultiTextureMatrixElement::UnitData());
    }
  }
  mutable SbList<SoMultiTextureMatrixElement::UnitData> unitdata;
};

SO_ELEMENT_CUSTOM_CONSTRUCTOR_SOURCE(SoMultiTextureMatrixElement);

/*!
  \copydetails SoElement::initClass(void)
*/
void
SoMultiTextureMatrixElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoMultiTextureMatrixElement, inherited);
}

/*!
  Constructor.
 */
SoMultiTextureMatrixElement::SoMultiTextureMatrixElement(void)
{
  PRIVATE(this) = new SoMultiTextureMatrixElementP;

  this->setTypeId(SoMultiTextureMatrixElement::classTypeId);
  this->setStackIndex(SoMultiTextureMatrixElement::classStackIndex);
}

/*!
  Destructor.
*/
SoMultiTextureMatrixElement::~SoMultiTextureMatrixElement(void)
{
  delete PRIVATE(this);
}


void
SoMultiTextureMatrixElement::set(SoState * const state, SoNode * const node, const int unit, const SbMatrix & matrix)
{
  SoMultiTextureMatrixElement * elem = coin_assert_cast<SoMultiTextureMatrixElement *>
    (SoElement::getElement(state, classStackIndex));
  elem->setElt(unit, matrix);
  if (node) elem->addNodeId(node);
}


/*!
  Multiplies \a matrix into the current texture matrix.
*/
void
SoMultiTextureMatrixElement::mult(SoState * const state,
                                  SoNode * const node,
                                  const int unit,
                                  const SbMatrix & matrix)
{
  SoMultiTextureMatrixElement * elem = coin_assert_cast<SoMultiTextureMatrixElement *>
    (SoElement::getElement(state, classStackIndex));
  elem->multElt(unit, matrix);
  if (node) elem->addNodeId(node);
}

/*!
  Returns current texture matrix.
*/
const SbMatrix &
SoMultiTextureMatrixElement::get(SoState * const state, const int unit)
{
  const SoMultiTextureMatrixElement * elem =
    coin_assert_cast<const SoMultiTextureMatrixElement *>
    (SoElement::getConstElement(state, classStackIndex));
  return elem->getElt(unit);
}

SoMultiTextureMatrixElement::UnitData &
SoMultiTextureMatrixElement::getUnitData(const int unit)
{
  PRIVATE(this)->ensureCapacity(unit);
  return PRIVATE(this)->unitdata[unit];
}

const SoMultiTextureMatrixElement::UnitData &
SoMultiTextureMatrixElement::getUnitData(const int unit) const
{
  PRIVATE(this)->ensureCapacity(unit);
  return PRIVATE(this)->unitdata[unit];
}

int 
SoMultiTextureMatrixElement::getNumUnits() const
{
  return PRIVATE(this)->unitdata.getLength();
}


/*!
  virtual method which is called from mult(). Multiplies \a matrix
  into element matrix.
*/
void
SoMultiTextureMatrixElement::multElt(const int unit, const SbMatrix & matrix)
{
  PRIVATE(this)->ensureCapacity(unit);
  PRIVATE(this)->unitdata[unit].textureMatrix.multLeft(matrix);
}

/*!
  virtual method which is called from set(). Sets \a matrix
  into element matrix.
*/
void
SoMultiTextureMatrixElement::setElt(const int unit, const SbMatrix & matrix)
{
  PRIVATE(this)->ensureCapacity(unit);
  PRIVATE(this)->unitdata[unit].textureMatrix = matrix;
}

/*!
  Returns element matrix. Called from get().
*/
const SbMatrix &
SoMultiTextureMatrixElement::getElt(const int unit) const
{
  PRIVATE(this)->ensureCapacity(unit);
  return PRIVATE(this)->unitdata[unit].textureMatrix;
}

// doc from parent
void
SoMultiTextureMatrixElement::init(SoState * state)
{
  inherited::init(state);
  this->clearNodeIds();
}

// Documented in superclass. Overridden to copy current matrix and
// update accumulated node ids.
void
SoMultiTextureMatrixElement::push(SoState * state)
{
  inherited::push(state);

  const SoMultiTextureMatrixElement * prev =
    coin_assert_cast<const SoMultiTextureMatrixElement *>
    (this->getNextInStack());
  
  PRIVATE(this)->unitdata = PRIVATE(prev)->unitdata;
  // make sure node ids are accumulated properly
  this->copyNodeIds(prev);
}

#undef PRIVATE
