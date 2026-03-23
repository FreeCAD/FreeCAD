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
  \class SoProfileElement Inventor/elements/SoProfileElement.h
  \brief The SoProfileElement class is yet to be documented.

  \ingroup coin_elements

  FIXME: write doc.
*/

#include <Inventor/elements/SoProfileElement.h>

#include "SbBasicP.h"

#include <Inventor/nodes/SoProfile.h>

/*!
  \fn SoProfileElement::Profile

  FIXME: write doc.
*/

/*!
  \fn SoProfileElement::profiles

  FIXME: write doc.
*/

SO_ELEMENT_SOURCE(SoProfileElement);

// doc from parent
void
SoProfileElement::initClass(void)
{
  SO_ELEMENT_INIT_CLASS(SoProfileElement, inherited);
}

/*!
  The destructor.
*/
SoProfileElement::~SoProfileElement(void)
{
}

/*!
  Adds \a profile to the list of profiles.
*/
void
SoProfileElement::add(SoState * const state,
                      SoProfile * const profile)
{
  SoProfileElement * element = coin_safe_cast<SoProfileElement *>
    (
     getElement(state, classStackIndex)
     );

  if (element) {
    switch (static_cast<SoProfileElement::Profile>(profile->linkage.getValue())) {
    case START_FIRST:
      element->profiles.truncate(0);
      element->profiles.append(profile);
      element->setNodeId(profile);
      break;
    case START_NEW:
      element->profiles.append(profile);
      element->addNodeId(profile);
      break;
    case ADD_TO_CURRENT:
      element->profiles.append(profile);
      element->addNodeId(profile);
      break;
    }
  }
}

/*!
  Returns current list of profiles.
*/
const SoNodeList &
SoProfileElement::get(SoState * const state)
{
  const SoProfileElement * element = coin_assert_cast<const SoProfileElement *>
    (
    getConstElement(state, classStackIndex)
    );
  return element->profiles;
}

// doc from parent
void
SoProfileElement::init(SoState * state)
{
  inherited::init(state);
  this->profiles.truncate(0);
  this->clearNodeIds();
}

// Documented in superclass. Overridden to copy profiles and node ids.
void
SoProfileElement::push(SoState * state)
{
  inherited::push(state);

  SoProfileElement * const prev =
    coin_assert_cast<SoProfileElement *>(this->getNextInStack());

  this->profiles.truncate(0);
  const int numprofiles = prev->profiles.getLength();
  for (int i = 0; i < numprofiles; i++)
    this->profiles.append(prev->profiles[i]);
  this->copyNodeIds(prev);
}
