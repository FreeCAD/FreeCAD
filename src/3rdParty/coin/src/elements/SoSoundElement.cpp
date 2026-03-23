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
  \class SoSoundElement Inventor/elements/SoSoundElement.h
  \brief The SoSoundElement is used for optimizing audio rendering and for turning off inactive SoVRMLSound nodes
  \ingroup coin_elements

  \since Coin 2.0
*/

#include <Inventor/elements/SoSoundElement.h>

#include "coindefs.h"
#include "SbBasicP.h"

#include <Inventor/nodes/SoNode.h>

/*!
  \fn SoSoundElement::sceneGraphHasSoundNode

  The sceneGraphHasSoundNode flag is used to stop the SoAudioRenderAction in SoSeparator and SoVRMLGroup nodes for cases where the subgraph does not contain any SoVRMLSound nodes.
*/

/*!
  \fn SoSoundElement::isPartOfActiveSceneGraph

  The isPartOfActiveSceneGraph flag is used to make SoVRMLSound that are below inactive parts of the scene graph (i.e. below a SoSwitch or SoLOD node) stop playing.
*/

/*!
  \fn SoSoundElement::soundNodeIsPlaying

  The soundNodeIsPlaying flag is currently unused.
*/

SO_ELEMENT_SOURCE(SoSoundElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoSoundElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoSoundElement, inherited);
}

/*!
  Destructor.
*/

SoSoundElement::~SoSoundElement(void)
{
}

/*!
  Initializes the element to its default value.
*/

void
SoSoundElement::init(SoState * state)
{
  inherited::init(state);

  this->setDefaultValues();
}

/*!
  Sets the flags.
*/

void
SoSoundElement::set(SoState * const state,
                    SoNode * const COIN_UNUSED_ARG(node),
                    SbBool scenegraphhassoundnode,
                    SbBool soundnodeisplaying,
                    SbBool ispartofactivescenegraph)
{
  SoSoundElement * elem =
    coin_safe_cast<SoSoundElement *>(SoElement::getElement(state, classStackIndex));
  if (elem) {
    elem->scenegraphhassoundnode = scenegraphhassoundnode;
    elem->soundnodeisplaying = soundnodeisplaying;
    elem->ispartofactivescenegraph = ispartofactivescenegraph;
  }
}

/*!
  Sets the sceneGraphHasSoundNode flag.
 */

SbBool
SoSoundElement::setSceneGraphHasSoundNode(SoState * const state,
                                     SoNode * const COIN_UNUSED_ARG(node),
                                          SbBool flag)
{
  SoSoundElement * elem = coin_safe_cast<SoSoundElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    SbBool old = elem->scenegraphhassoundnode;
    elem->scenegraphhassoundnode = flag;
    return old;
  } else
    return FALSE;
}

/*!
  Returns the value of the sceneGraphHasSoundNode flag.
 */

SbBool
SoSoundElement::sceneGraphHasSoundNode(SoState * const state)
{
  const SoSoundElement * elem = coin_assert_cast<const SoSoundElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return elem->scenegraphhassoundnode;
}


/*!
  Sets the soundNodeIsPlaying flag.
 */

SbBool
SoSoundElement::setSoundNodeIsPlaying(SoState * const state,
                                  SoNode * const COIN_UNUSED_ARG(node),
                                  SbBool flag)
{
  SoSoundElement * elem = coin_safe_cast<SoSoundElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    SbBool old = elem->soundnodeisplaying;
    elem->soundnodeisplaying = flag;
    return old;
  } else
    return FALSE;
}

/*!
  Returns the value of the soundNodeIsPlaying flag.
 */

SbBool
SoSoundElement::soundNodeIsPlaying(SoState * const state)
{
  const SoSoundElement * elem = coin_assert_cast<const SoSoundElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return elem->soundnodeisplaying;
}

/*!
  Sets the isPartOfActiveSceneGraph flag.
 */

SbBool
SoSoundElement::setIsPartOfActiveSceneGraph(SoState * const state,
                                       SoNode * const COIN_UNUSED_ARG(node),
                                            SbBool flag)
{
  SoSoundElement *elem = coin_safe_cast<SoSoundElement *>
    (
     SoElement::getElement(state, classStackIndex)
     );
  if (elem) {
    SbBool old = elem->ispartofactivescenegraph;
    elem->ispartofactivescenegraph = flag;
    return old;
  } else
    return FALSE;
}

/*!
  Returns the value of the isPartOfActiveSceneGraph flag.
*/

SbBool
SoSoundElement::isPartOfActiveSceneGraph(SoState * const state)
{
  const SoSoundElement * elem = coin_assert_cast<const SoSoundElement *>
    (
     SoElement::getConstElement(state, classStackIndex)
     );
  return elem->ispartofactivescenegraph;
}

/*!
  Calls the superclass' push method. Initializes the element to the
  default values. Uses previous element's isPartOfActiveSceneGraph
  flag.
*/

void
SoSoundElement::push(SoState * state)
{
  inherited::push(state);

  const SoSoundElement * prev = coin_assert_cast<SoSoundElement *>
    (this->getNextInStack());

  this->scenegraphhassoundnode = FALSE;
  this->soundnodeisplaying = FALSE;
  this->ispartofactivescenegraph = prev->ispartofactivescenegraph;
}

/*!
  Calls the superclass' pop method.
 */

void
SoSoundElement::pop(SoState * COIN_UNUSED_ARG(state), const SoElement * prevTopElement)
{
  const SoSoundElement * prevtop =
    coin_assert_cast<const SoSoundElement *>(prevTopElement);
  this->scenegraphhassoundnode = this->scenegraphhassoundnode |
    prevtop->scenegraphhassoundnode;
  this->soundnodeisplaying = this->soundnodeisplaying |
    prevtop->soundnodeisplaying;
}


/*!
  Initializes the element to the default values. The default values for the sceneGraphHasSoundNode is FALSE. The default value for the isPartOfActiveSceneGraph flag is TRUE. the default value for the soundNodeIsPlaying flag is FALSE.
 */

void
SoSoundElement::setDefaultValues()
{
  this->scenegraphhassoundnode = FALSE;
  this->soundnodeisplaying = FALSE;
  this->ispartofactivescenegraph = TRUE;
}


//! Prints contents of element (not implemented)

void
SoSoundElement::print(FILE * /* file */) const
{
}
