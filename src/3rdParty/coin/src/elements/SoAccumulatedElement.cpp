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
  \class SoAccumulatedElement SoAccumulatedElement.h Inventor/elements/SoAccumulatedElement.h
  \brief The SoAccumulatedElement class is an abstract class for storing accumulated state.

  \ingroup coin_elements

  This is the superclass of elements where new element data \e
  accumulates with older data.

  The element stores node id values for all nodes accumulated during
  traversal for the current state. These id values are used to
  determine when to invalidate caches.

  \sa SoReplacedElement, SoFloatElement, SoInt32Element
*/

#include <Inventor/elements/SoAccumulatedElement.h>
#include <Inventor/nodes/SoNode.h>
#include <cassert>

#include "SbBasicP.h"

/*!
  \fn SoAccumulatedElement::nodeIds

  Stores the internal list of node id values for nodes accumulated on
  the stack for the element.
*/

SO_ELEMENT_ABSTRACT_SOURCE(SoAccumulatedElement);

/*!
  \copydetails SoElement::initClass(void)
*/

void
SoAccumulatedElement::initClass(void)
{
  SO_ELEMENT_INIT_ABSTRACT_CLASS(SoAccumulatedElement, inherited);
}

/*!
  Destructor.
*/

SoAccumulatedElement::~SoAccumulatedElement(void)
{
}

// Doc in superclass.
void
SoAccumulatedElement::init(SoState * state)
{
  inherited::init(state);
  // this is FALSE until node id's are copied
  this->recursecapture = FALSE;
}

void
SoAccumulatedElement::push(SoState * state)
{
  inherited::push(state);
  // this is FALSE until node id's are copied
  this->recursecapture = FALSE;
}

// Documented in superclass. Overridden to compare node ids.
SbBool
SoAccumulatedElement::matches(const SoElement * element) const
{
  const SoAccumulatedElement * elem = coin_assert_cast<const SoAccumulatedElement *>(element);
  return (elem->nodeIds == this->nodeIds);
}

/*!
  Empty the list of node ids.
*/
void
SoAccumulatedElement::clearNodeIds(void)
{
  this->nodeIds.truncate(0);
  // we do not depend on previous elements any more
  this->recursecapture = FALSE;
}

/*!
  Add the node id of \a node to the list of node ids.
*/
void
SoAccumulatedElement::addNodeId(const SoNode * const node)
{
  this->nodeIds.append(node->getNodeId());
}

/*!
  Empty the list of node ids, and add the id of \a node.
*/
void
SoAccumulatedElement::setNodeId(const SoNode * const node)
{
  this->clearNodeIds();
  this->addNodeId(node);
  // we do not depend on previous elements any more
  this->recursecapture = FALSE;
}

// Documented in superclass. Overridden to copy node ids.
SoElement *
SoAccumulatedElement::copyMatchInfo(void) const
{
  SoAccumulatedElement * element =
    static_cast<SoAccumulatedElement *>(this->getTypeId().createInstance());
  element->copyNodeIds(this);
  return element;
}

/*!
  Convenience method which copies the node ids from \a copyfrom to
  this element.

  \COIN_FUNCTION_EXTENSION
*/
void
SoAccumulatedElement::copyNodeIds(const SoAccumulatedElement * copyfrom)
{
  this->nodeIds = copyfrom->nodeIds;

  // this elements uses data from previous element in stack
  this->recursecapture = TRUE;
}

// Documented in superclass. Overridden to capture more elements.
void
SoAccumulatedElement::captureThis(SoState * state) const
{
  inherited::captureThis(state);

  // we need to recurse if element has copied data from previous
  // element in stack (or nextInStack as SGI was silly enough to call
  // it). This is because the depth of this element might not cause
  // cache to depend on this element, but the previous element(s)
  // might have a depth that will trigger a dependency.
  //                                              pederb, 2001-02-21
  if (this->recursecapture) {
    const SoAccumulatedElement * elem = coin_assert_cast<const SoAccumulatedElement *> (
      this->getNextInStack()
      );
    if (elem) elem->captureThis(state);
  }
}
