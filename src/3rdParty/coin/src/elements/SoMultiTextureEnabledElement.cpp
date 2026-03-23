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
  \class SoMultiTextureEnabledElement Inventor/elements/SoMultiTextureEnabledElement.h
  \brief The SoMultiTextureEnabledElement class is an element which stores whether texturing is enabled or not.

  \ingroup coin_elements

  \COIN_CLASS_EXTENSION

  \since Coin 2.2
*/

/*!
  \fn void SoMultiTextureEnabledElement::set(SoState * state, const SbBool enabled)
  
  \since Coin-3
*/

/*!
  \fn void SoMultiTextureEnabledElement::set(SoState * state, SoNode* node, const SbBool enabled)
  
  \since Coin-3.
*/

#include "SbBasicP.h"
#include "coindefs.h"

#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/lists/SbList.h>

#include "coindefs.h"

class SoMultiTextureEnabledElementP {
 public:
  SbList <SbBool> enabled;
  SbList<SoMultiTextureEnabledElement::Mode> mode;
};

#define PRIVATE(obj) obj->pimpl

SO_ELEMENT_CUSTOM_CONSTRUCTOR_SOURCE(SoMultiTextureEnabledElement);

/*!
  \copydetails SoElement::initClass(void)
*/
void
SoMultiTextureEnabledElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoMultiTextureEnabledElement, inherited);
}

/*!
  Constructor.
*/
SoMultiTextureEnabledElement::SoMultiTextureEnabledElement(void)
{
  PRIVATE(this) = new SoMultiTextureEnabledElementP;

  this->setTypeId(SoMultiTextureEnabledElement::classTypeId);
  this->setStackIndex(SoMultiTextureEnabledElement::classStackIndex);
}

/*!
  Destructor.
*/
SoMultiTextureEnabledElement::~SoMultiTextureEnabledElement(void)
{
  delete PRIVATE(this);
}

/*!
  Sets the state of this element.
*/
void
SoMultiTextureEnabledElement::set(SoState * state,
                                  SoNode * COIN_UNUSED_ARG(node),
                                  const int unit,
                                  const SbBool enabled)
{
  SoMultiTextureEnabledElement * elem =
    coin_assert_cast<SoMultiTextureEnabledElement *>
    (
     state->getElement(classStackIndex)
     );

  elem->setElt(unit, enabled);
  
  if (unit == 0) {
    // FIXME: check all units?
    SoShapeStyleElement::setTextureEnabled(state, enabled);
  }
}


// doc from parent
void
SoMultiTextureEnabledElement::init(SoState * COIN_UNUSED_ARG(state))
{
  PRIVATE(this)->mode.truncate(0);
  PRIVATE(this)->enabled.truncate(0);
}

/*!
  Return current state of this element for \a unit.
*/
SbBool
SoMultiTextureEnabledElement::get(SoState * state, const int unit)
{
  const SoMultiTextureEnabledElement * elem =
    coin_assert_cast<const SoMultiTextureEnabledElement *>
    (SoElement::getConstElement(state, classStackIndex));

  if (unit < PRIVATE(elem)->enabled.getLength()) {
    return PRIVATE(elem)->enabled[unit];
  }
  return FALSE;
}

/*!
  virtual element set function.
*/
void
SoMultiTextureEnabledElement::setElt(const int unit, const int mode_in)
{
  Mode mode = static_cast<Mode>(mode_in);
  for (int i = PRIVATE(this)->enabled.getLength(); i <= unit; i++) { 
    PRIVATE(this)->enabled.append(FALSE);
    PRIVATE(this)->mode.append(DISABLED);
  }
  PRIVATE(this)->enabled[unit] = mode != DISABLED;
  PRIVATE(this)->mode[unit] = mode;
}

/*!
  Returns a pointer to a boolean array. TRUE means unit is enabled and
  that texture coordinates must be sent to the unit. \a lastenabled
  is set to the last enabled unit.

*/
const SbBool *
SoMultiTextureEnabledElement::getEnabledUnits(SoState * state,
                                              int & lastenabled)
{
  const SoMultiTextureEnabledElement * elem =
    coin_assert_cast<const SoMultiTextureEnabledElement *>
    (SoElement::getConstElement(state, classStackIndex));

  int i = PRIVATE(elem)->enabled.getLength()-1;
  while (i >= 0) {
    if (PRIVATE(elem)->enabled[i]) break;
    i--;
  }
  if (i >= 0) {
    lastenabled = i;
    return PRIVATE(elem)->enabled.getArrayPtr();
  }
  lastenabled = -1;
  return NULL;
}

/*!
  Returns TRUE if unit is enabled (Mode == DISABLED).
*/
SbBool
SoMultiTextureEnabledElement::isEnabled(const int unit) const
{
  if (unit < PRIVATE(this)->enabled.getLength()) {
    return PRIVATE(this)->enabled[unit];
  }
  return FALSE;
}

// doc in parent
void
SoMultiTextureEnabledElement::push(SoState * COIN_UNUSED_ARG(state))
{
  SoMultiTextureEnabledElement * prev = coin_assert_cast<SoMultiTextureEnabledElement *>
    (this->getNextInStack());

  PRIVATE(this)->mode = PRIVATE(prev)->mode;
  PRIVATE(this)->enabled = PRIVATE(prev)->enabled;
}

SbBool
SoMultiTextureEnabledElement::matches(const SoElement * elem) const
{
  const SoMultiTextureEnabledElement * e =
    coin_assert_cast<const SoMultiTextureEnabledElement *>(elem);
  if (PRIVATE(e)->mode.getLength() != PRIVATE(this)->mode.getLength()) return FALSE;
  
  for (int i = 0; i < PRIVATE(e)->mode.getLength(); i++) {
    if (PRIVATE(e)->mode[i] != PRIVATE(this)->mode[i]) {
      return FALSE;
    }
  }
  return TRUE;
}

SoElement *
SoMultiTextureEnabledElement::copyMatchInfo(void) const
{
  SoMultiTextureEnabledElement * elem =
    static_cast<SoMultiTextureEnabledElement *>(getTypeId().createInstance());

  for (int i = 0; i < PRIVATE(this)->mode.getLength(); i++) {
    PRIVATE(elem)->mode.append(PRIVATE(this)->mode[i]);
  }
  return elem;
}

/*!
  Returns the mode of all units. Also returns the last enabled unit
  in \a lastenabled.

  \since Coin 2.5
*/
const SoMultiTextureEnabledElement::Mode *
SoMultiTextureEnabledElement::getActiveUnits(SoState * state, int & lastenabled)
{
  const SoMultiTextureEnabledElement * elem =
    coin_assert_cast<const SoMultiTextureEnabledElement *>
    (SoElement::getConstElement(state, classStackIndex));
  
  int i = PRIVATE(elem)->mode.getLength()-1;
  while (i >= 0) {
    if (PRIVATE(elem)->mode[i] != DISABLED) break;
    i--;
  }
  if (i >= 0) {
    lastenabled = i;
    return PRIVATE(elem)->mode.getArrayPtr();
  }
  return NULL;
}

/*!
  Enable RECTANGLE texture mode.

  \since Coin 2.5
*/
void
SoMultiTextureEnabledElement::enableRectangle(SoState * state,
                                              SoNode * COIN_UNUSED_ARG(node),
                                              const int unit)
{
  SoMultiTextureEnabledElement * elem =
    coin_assert_cast<SoMultiTextureEnabledElement *>
    (state->getElement(classStackIndex));
  elem->setElt(unit, static_cast<int>(RECTANGLE));
}

/*!
  Enable CUBEMAP texture mode.

  \since Coin 2.5
*/
void
SoMultiTextureEnabledElement::enableCubeMap(SoState * state,
                                            SoNode * COIN_UNUSED_ARG(node),
                                            const int unit)
{
  SoMultiTextureEnabledElement * elem = coin_assert_cast<SoMultiTextureEnabledElement *>
    (state->getElement(classStackIndex));

  elem->setElt(unit, static_cast<int>(CUBEMAP));
}

/*!
  Enable Texture3 texture mode.

  \since Coin 4.0
*/
void
SoMultiTextureEnabledElement::enableTexture3(SoState * state,
                                             SoNode * COIN_UNUSED_ARG(node),
                                             const int unit)
{
  SoMultiTextureEnabledElement * elem = coin_assert_cast<SoMultiTextureEnabledElement *>
    (state->getElement(classStackIndex));
  
  elem->setElt(unit, static_cast<int>(TEXTURE3D));
}

/*!

  Disable all active texture units. Convenient when all textures needs
  to be disabled before rendering.

  \since Coin 2.5
*/
void
SoMultiTextureEnabledElement::disableAll(SoState * state)
{
  int lastenabled;
  const SbBool * enabled = getEnabledUnits(state, lastenabled);
  if (enabled) {
    SoMultiTextureEnabledElement * elem =
      coin_assert_cast<SoMultiTextureEnabledElement *>
      (state->getElement(classStackIndex));

    for (int i = 0; i <= lastenabled; i++) {
      if (enabled[i]) {
        elem->setElt(i, FALSE);
      }
    }
  }
  SoShapeStyleElement::setTextureEnabled(state, FALSE);
}

/*!
  Returns the mode for a texture unit.

  \since Coin 2.5
*/
SoMultiTextureEnabledElement::Mode
SoMultiTextureEnabledElement::getMode(SoState * state, const int unit)
{
  const SoMultiTextureEnabledElement * elem =
    coin_assert_cast<const SoMultiTextureEnabledElement *>
    (SoElement::getConstElement(state, classStackIndex));

  return elem->getMode(unit);
}

/*!
  Returns the max number of texture units enabled/disabled
  \since Coin 4.0
 */
int 
SoMultiTextureEnabledElement::getMaxUnits() const
{
  return PRIVATE(this)->mode.getLength();
}

//
// returns the texture mode for a unit.
//
SoMultiTextureEnabledElement::Mode
SoMultiTextureEnabledElement::getMode(const int unit) const
{
  if (unit < PRIVATE(this)->mode.getLength()) {
    return PRIVATE(this)->mode[unit];
  }
  return DISABLED;
}

#undef PRIVATE
