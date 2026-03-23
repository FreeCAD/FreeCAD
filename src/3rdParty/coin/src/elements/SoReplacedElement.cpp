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
  \class SoReplacedElement Inventor/elements/SoReplacedElement.h
  \brief The SoReplacedElement class is an abstract element superclass.

  \ingroup coin_elements

  This is the superclass of all elements where the new element data \e
  replaces the old data, and where the data the element stores is not
  just a simple float or integer value.

  Apart from this conceptual difference from its superclass, the
  SoReplacedElement class also overloads the default getElement()
  method to include a node reference. This reference is used to fetch
  the unique node identification number of the node that last changed
  the element.

  The identifier values of nodes in the scene graph is updated upon \e
  any kind of change to a node, so this technique plays an important
  role in the construction, validation and destruction of internal
  scene graph caches.

  \sa SoAccumulatedElement
*/

#include <Inventor/elements/SoReplacedElement.h>

#include "SbBasicP.h"

#include <Inventor/nodes/SoNode.h>
#include <cassert>

/*!
  \var uint32_t SoReplacedElement::nodeId
  \COININTERNAL
*/


SO_ELEMENT_ABSTRACT_SOURCE(SoReplacedElement);


/*!
  \copydetails SoElement::initClass(void)
*/
void
SoReplacedElement::initClass(void)
{
  SO_ELEMENT_INIT_ABSTRACT_CLASS(SoReplacedElement, inherited);
}

/*!
  Destructor.
*/
SoReplacedElement::~SoReplacedElement(void)
{
}

// Documented in superclass.
void
SoReplacedElement::init(SoState * state)
{
  inherited::init(state);
  this->nodeId = 0;
}

// Documented in superclass.
SbBool
SoReplacedElement::matches(const SoElement * element) const
{
  if ((coin_assert_cast<const SoReplacedElement *>(element))->nodeId ==
      this->nodeId)
    return TRUE;
  return FALSE;
}

// Documented in superclass.
SoElement *
SoReplacedElement::copyMatchInfo(void) const
{
  assert(getTypeId().canCreateInstance());
  SoReplacedElement * element =
    static_cast<SoReplacedElement *>(getTypeId().createInstance());
  element->nodeId = this->nodeId;
  return element;
}

// Documented in superclass.
void
SoReplacedElement::print(FILE * file) const
{
  const char * typen = this->getTypeId().getName().getString();
  (void)fprintf(file, "%s[%p]\n", typen, this);
}

/*!
  This function overloads SoElement::getElement() with an extra \a
  node parameter, to let us set the SoReplacedElement::nodeId in the
  element instance before returning.

  SoReplacedElement subclasses should use this method to get writable
  instances.

  The identifier values of nodes in the scene graph is updated upon \e
  any kind of change to a node, so this technique plays an important
  role in the construction, validation and destruction of internal
  scene graph caches.

  \sa SoElement::getElement()
*/
SoElement *
SoReplacedElement::getElement(SoState * const state, const int stackIndex,
                                     SoNode * const node)
{
  SoReplacedElement * elem =
    coin_safe_cast<SoReplacedElement *>(SoElement::getElement(state, stackIndex));
  if (elem) {
    if (node) { elem->nodeId = node->getNodeId(); }
    else { elem->nodeId = 0; }
    return elem;
  }
  return NULL;
}

/*!
  Returns the node identifier for the node that previously updated the
  SoReplacedElement.
*/
SbUniqueId
SoReplacedElement::getNodeId(void) const
{
  return this->nodeId;
}
