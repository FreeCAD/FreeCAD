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
  \class SoMultiTextureCoordinateElement Inventor/elements/SoMultiTextureCoordinateElement.h
  \brief The SoMultiTextureCoordinateElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.

  \COIN_CLASS_EXTENSION

  \since Coin 2.2
*/

#include "coindefs.h"
#include "SbBasicP.h"

#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/lists/SbList.h>
#include <cassert>

#define PRIVATE(obj) obj->pimpl

SoMultiTextureCoordinateElement::UnitData::UnitData()
  : nodeid(0),
    whatKind(DEFAULT),
    funcCB(NULL),
    funcCBData(NULL),
    numCoords(0),
    coords2(NULL),
    coords3(NULL),
    coords4(NULL),
    coordsDimension(2)
{
}

SoMultiTextureCoordinateElement::UnitData::UnitData(const UnitData & org)
  : nodeid(org.nodeid),
    whatKind(org.whatKind),
    funcCB(org.funcCB),
    funcCBData(org.funcCBData),
    numCoords(org.numCoords),
    coords2(org.coords2),
    coords3(org.coords3),
    coords4(org.coords4),
    coordsDimension(org.coordsDimension)
{
}

class SoMultiTextureCoordinateElementP {
public:
  mutable SbList<SoMultiTextureCoordinateElement::UnitData> unitdata;

  void ensureCapacity(int units) const {
    for (int i = this->unitdata.getLength(); i <= units; i++) {
      this->unitdata.append(SoMultiTextureCoordinateElement::UnitData());
    }
  }
};

SO_ELEMENT_CUSTOM_CONSTRUCTOR_SOURCE(SoMultiTextureCoordinateElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoMultiTextureCoordinateElement::initClass()
{
  SO_ELEMENT_INIT_CLASS(SoMultiTextureCoordinateElement, inherited);
}


/*!
  Constructor.
*/
SoMultiTextureCoordinateElement::SoMultiTextureCoordinateElement(void)
{
  PRIVATE(this) = new SoMultiTextureCoordinateElementP;

  this->setTypeId(SoMultiTextureCoordinateElement::classTypeId);
  this->setStackIndex(SoMultiTextureCoordinateElement::classStackIndex);
}

/*!
  Destructor.
*/

SoMultiTextureCoordinateElement::~SoMultiTextureCoordinateElement()
{
  delete PRIVATE(this);
}

//! FIXME: write doc.

void
SoMultiTextureCoordinateElement::setDefault(SoState * const state,
                                            SoNode * const COIN_UNUSED_ARG(node),
                                            const int unit)
{
  if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
    SoGLVBOElement::setTexCoordVBO(state, unit, NULL);
  }
  SoMultiTextureCoordinateElement * element =
    coin_assert_cast<SoMultiTextureCoordinateElement *>
    (SoElement::getElement(state, classStackIndex));

  PRIVATE(element)->ensureCapacity(unit);
  UnitData & ud = PRIVATE(element)->unitdata[unit];
  ud.nodeid = 0;
  ud.whatKind = DEFAULT;
  ud.numCoords = 0;
}

//! FIXME: write doc.

void
SoMultiTextureCoordinateElement::setFunction(SoState * const state,
                                             SoNode * const node,
                                             const int unit,
                                             SoTextureCoordinateFunctionCB * const func,
                                             void * const userdata)
{
  if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
    SoGLVBOElement::setTexCoordVBO(state, unit, NULL);
  }

  SoMultiTextureCoordinateElement * element =
    coin_assert_cast<SoMultiTextureCoordinateElement *>
    (SoElement::getElement(state, classStackIndex));

  PRIVATE(element)->ensureCapacity(unit);
  UnitData & ud = PRIVATE(element)->unitdata[unit];

  ud.nodeid = node->getNodeId();
  ud.funcCB = func;
  ud.funcCBData = userdata;
  ud.whatKind = FUNCTION;
  ud.coords2 = NULL;
  ud.coords3 = NULL;
  ud.coords4 = NULL;
  ud.numCoords = 0;
}

//! FIXME: write doc.

void
SoMultiTextureCoordinateElement::set2(SoState * const state,
                                      SoNode * const node,
                                      const int unit,
                                      const int32_t numCoords,
                                      const SbVec2f * const coords)
{
  if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
    SoGLVBOElement::setTexCoordVBO(state, unit, NULL);
  }
  SoMultiTextureCoordinateElement * element = coin_assert_cast<SoMultiTextureCoordinateElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );

  PRIVATE(element)->ensureCapacity(unit);
  UnitData & ud = PRIVATE(element)->unitdata[unit];

  ud.nodeid = node->getNodeId();
  ud.coordsDimension = 2;
  ud.numCoords = numCoords;
  ud.coords2 = coords;
  ud.coords3 = NULL;
  ud.coords4 = NULL;
  ud.whatKind = EXPLICIT;
}

/*!
  FIXME: write doc.
*/
void
SoMultiTextureCoordinateElement::set3(SoState * const state,
                                      SoNode * const node,
                                      const int unit,
                                      const int32_t numCoords,
                                      const SbVec3f * const coords)
{
  if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
    SoGLVBOElement::setTexCoordVBO(state, unit, NULL);
  }
  SoMultiTextureCoordinateElement * element =
    coin_assert_cast<SoMultiTextureCoordinateElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );

  PRIVATE(element)->ensureCapacity(unit);
  UnitData & ud = PRIVATE(element)->unitdata[unit];

  ud.nodeid = node->getNodeId();
  ud.coordsDimension = 3;
  ud.numCoords = numCoords;
  ud.coords2 = NULL;
  ud.coords3 = coords;
  ud.coords4 = NULL;
  ud.whatKind = EXPLICIT;
}

//! FIXME: write doc.

void
SoMultiTextureCoordinateElement::set4(SoState * const state,
                                      SoNode * const node,
                                      const int unit,
                                      const int32_t numCoords,
                                      const SbVec4f * const coords)
{
  if (state->isElementEnabled(SoGLVBOElement::getClassStackIndex())) {
    SoGLVBOElement::setTexCoordVBO(state, unit, NULL);
  }
  SoMultiTextureCoordinateElement * element =
    coin_assert_cast<SoMultiTextureCoordinateElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );

  PRIVATE(element)->ensureCapacity(unit);
  UnitData & ud = PRIVATE(element)->unitdata[unit];

  ud.nodeid = node->getNodeId();
  ud.coordsDimension = 4;
  ud.numCoords = numCoords;
  ud.coords2 = NULL;
  ud.coords3 = NULL;
  ud.coords4 = coords;
  ud.whatKind = EXPLICIT;
}

//! FIXME: write doc.

const SoMultiTextureCoordinateElement *
SoMultiTextureCoordinateElement::getInstance(SoState * const state)
{
  return coin_safe_cast<const SoMultiTextureCoordinateElement *>
    (getConstElement(state, classStackIndex));
}

/*!
  This method returns texture coordinate for the given point and normal.
  The coordinate is returned as a 4D vector where the r and q coordinates
  may be set to 0 and 1 respectively depending on what texture coordinate
  dimension we're using.

  This method should only be used if the CoordType is FUNCTION.
*/

const SbVec4f &
SoMultiTextureCoordinateElement::get(const int unit,
                                     const SbVec3f & point,
                                     const SbVec3f & normal) const
{
  assert(unit < PRIVATE(this)->unitdata.getLength());
  const UnitData & ud = PRIVATE(this)->unitdata[unit];

  assert((ud.whatKind == FUNCTION ||
          ud.whatKind == TEXGEN) && ud.funcCB);
  return (*(ud.funcCB))(ud.funcCBData, point, normal);
}

//! FIXME: write doc.

const SbVec2f &
SoMultiTextureCoordinateElement::get2(const int unit, const int index) const
{
  assert(unit < PRIVATE(this)->unitdata.getLength());
  const UnitData & ud = PRIVATE(this)->unitdata[unit];

  assert(index >= 0 && index < ud.numCoords);
  assert(ud.whatKind == EXPLICIT);
  if (ud.coordsDimension == 2) {
    return ud.coords2[index];
  }
  else {
    // need an instance we can write to
    SoMultiTextureCoordinateElement * elem = const_cast<SoMultiTextureCoordinateElement *>(this);

    if (ud.coordsDimension == 4) {
      float tmp = ud.coords4[index][3];
      float to2D = tmp == 0.0f ? 1.0f : 1.0f / tmp;

      elem->convert2.setValue(ud.coords4[index][0] * to2D,
                              ud.coords4[index][1] * to2D);
    }
    else { // coordsDimension == 3
      elem->convert2.setValue(ud.coords3[index][0],
                              ud.coords3[index][1]);
    }
    return this->convert2;
  }
}

/*!
  FIXME: write doc.

*/
const SbVec3f &
SoMultiTextureCoordinateElement::get3(const int unit, const int index) const
{
  assert(unit < PRIVATE(this)->unitdata.getLength());
  const UnitData & ud = PRIVATE(this)->unitdata[unit];

  assert(index >= 0 && index < ud.numCoords);
  assert(ud.whatKind == EXPLICIT);
  if (ud.coordsDimension == 3) {
    return ud.coords3[index];
  }
  else {
    // need an instance we can write to
    SoMultiTextureCoordinateElement * elem =
      const_cast<SoMultiTextureCoordinateElement *>(this);

    if (ud.coordsDimension==2) {
      elem->convert3.setValue(ud.coords2[index][0],
                              ud.coords2[index][1],
                              0.0f);
    }
    else { // this->coordsDimension==4
      ud.coords4[index].getReal(elem->convert3);
    }
    return this->convert3;
  }
}

//!  FIXME: write doc.

const SbVec4f &
SoMultiTextureCoordinateElement::get4(const int unit, const int index) const
{
  assert(unit < PRIVATE(this)->unitdata.getLength());
  const UnitData & ud = PRIVATE(this)->unitdata[unit];

  assert(index >= 0 && index < ud.numCoords);
  assert(ud.whatKind == EXPLICIT);
  if (ud.coordsDimension==4) {
    return ud.coords4[index];
  }
  else {
    // need an instance we can write to
    SoMultiTextureCoordinateElement * elem =
      const_cast<SoMultiTextureCoordinateElement *>(this);
    if (ud.coordsDimension == 2) {
      elem->convert4.setValue(ud.coords2[index][0],
                              ud.coords2[index][1],
                              0.0f,
                              1.0f);
    }
    else { // this->coordsDimension==3
      elem->convert4.setValue(ud.coords3[index][0],
                              ud.coords3[index][1],
                              ud.coords3[index][2],
                              1.0f);
    }
    return this->convert4;
  }
}

/*!
  This method is used by shapes.  Three return values are possible.

  DEFAULT means that the shapes should generate their own texture coordinates.

  EXPLICIT means that discrete texture coordinates are stored, and should be
  fetched with get2(), get3() or get4().

  FUNCTION means that get(point, normal) must be used to generate texture
  coordinates.
*/

SoMultiTextureCoordinateElement::CoordType
SoMultiTextureCoordinateElement::getType(SoState * const state, const int unit)
{
  const SoMultiTextureCoordinateElement * element =
    coin_assert_cast<const SoMultiTextureCoordinateElement *>
    (getConstElement(state, classStackIndex));
  return element->getType(unit);
}

//! FIXME: write doc.

// side effect, will increase array size
SoMultiTextureCoordinateElement::CoordType
SoMultiTextureCoordinateElement::getType(const int unit) const
{
  PRIVATE(this)->ensureCapacity(unit);
  const UnitData & ud = PRIVATE(this)->unitdata[unit];
  return ud.whatKind;
}

//! FIXME: write doc.

void
SoMultiTextureCoordinateElement::init(SoState * state)
{
  inherited::init(state);
  PRIVATE(this)->unitdata.truncate(0);
}

//! FIXME: write doc.

//$ EXPORT INLINE
int32_t
SoMultiTextureCoordinateElement::getNum(const int unit) const
{
  PRIVATE(this)->ensureCapacity(unit);
  const UnitData & ud = PRIVATE(this)->unitdata[unit];
  return ud.numCoords;
}

//! FIXME: write doc. (for backwards compatibility. Use getDimension() instead).

//$ EXPORT INLINE
SbBool
SoMultiTextureCoordinateElement::is2D(const int unit) const
{
  assert(unit < PRIVATE(this)->unitdata.getLength());
  const UnitData & ud = PRIVATE(this)->unitdata[unit];
  return (ud.coordsDimension==2);
}

/*!
  FIXME: write doc.
*/
int32_t
SoMultiTextureCoordinateElement::getDimension(const int unit) const
{
  assert(unit < PRIVATE(this)->unitdata.getLength());
  const UnitData & ud = PRIVATE(this)->unitdata[unit];
  return ud.coordsDimension;
}

/*!
  Returns a pointer to the 2D texture coordinate array. This method is not
  part of the OIV API.
*/
const SbVec2f *
SoMultiTextureCoordinateElement::getArrayPtr2(const int unit) const
{
  assert(unit < PRIVATE(this)->unitdata.getLength());
  const UnitData & ud = PRIVATE(this)->unitdata[unit];
  return ud.coords2;
}

/*!
  Returns a pointer to the 3D texture coordinate array.

*/
const SbVec3f *
SoMultiTextureCoordinateElement::getArrayPtr3(const int unit) const
{
  assert(unit < PRIVATE(this)->unitdata.getLength());
  const UnitData & ud = PRIVATE(this)->unitdata[unit];
  return ud.coords3;
}

/*!
  Returns a pointer to the 4D texture coordinate array. This method is not
  part of the OIV API.
*/
const SbVec4f *
SoMultiTextureCoordinateElement::getArrayPtr4(const int unit) const
{
  assert(unit < PRIVATE(this)->unitdata.getLength());
  const UnitData & ud = PRIVATE(this)->unitdata[unit];
  return ud.coords4;
}

void
SoMultiTextureCoordinateElement::push(SoState * COIN_UNUSED_ARG(state))
{
  SoMultiTextureCoordinateElement * prev =
    coin_assert_cast<SoMultiTextureCoordinateElement *>
    (this->getNextInStack());
  
  PRIVATE(this)->unitdata = PRIVATE(prev)->unitdata;
}

SbBool
SoMultiTextureCoordinateElement::matches(const SoElement * elem) const
{
  const SoMultiTextureCoordinateElement * e =
    coin_assert_cast<const SoMultiTextureCoordinateElement *>(elem);
  if (PRIVATE(e)->unitdata.getLength() != PRIVATE(this)->unitdata.getLength()) return FALSE;
  
  for (int i = 0; i < PRIVATE(this)->unitdata.getLength(); i++) {
    if (PRIVATE(e)->unitdata[i].nodeid != PRIVATE(this)->unitdata[i].nodeid) {
      return FALSE;
    }
  }
  return TRUE;
}

SoElement *
SoMultiTextureCoordinateElement::copyMatchInfo(void) const
{
  SoMultiTextureCoordinateElement * elem =
    static_cast<SoMultiTextureCoordinateElement *>(getTypeId().createInstance());
  PRIVATE(elem)->unitdata = PRIVATE(this)->unitdata;
  return elem;
}

/*!
  Returns the per-unit data for this element.
*/
SoMultiTextureCoordinateElement::UnitData &
SoMultiTextureCoordinateElement::getUnitData(const int unit)
{
  assert(unit < PRIVATE(this)->unitdata.getLength());
  return PRIVATE(this)->unitdata[unit];
}

const SoMultiTextureCoordinateElement::UnitData &
SoMultiTextureCoordinateElement::getUnitData(const int unit) const
{
  assert(unit < PRIVATE(this)->unitdata.getLength());
  return PRIVATE(this)->unitdata[unit];
}

int 
SoMultiTextureCoordinateElement::getMaxUnits() const
{
  return PRIVATE(this)->unitdata.getLength();
}

#undef PRIVATE
