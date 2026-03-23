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
  \class SoState SoState.h Inventor/misc/SoState.h
  \brief The SoState class manages the Coin scene graph traversal state data.

  \ingroup coin_general

  The SoState class is used by actions derived from the SoAction
  class. It manages the scene graph state as stacks of elements (i.e.
  instances of classes derived from SoElement).

  For more information on the inner workings of traversal states in
  Coin, we recommend the book &laquo;The Inventor Toolmaker&raquo; (ISBN
  0-201-62493-1), also available at SGI's <a
  href="http://techpubs.sgi.com/library">online library</a>. Search
  for "Toolmaker".
*/
// FIXME: should link to example(s) sourcecode extending the library
// by setting up new elements and/or actions. 20010716 mortene.

/*!
  \fn const SoElement * SoState::getConstElement(const int stackIndex) const

  This method returns a pointer to the top element of the given element
  stack.  The element is read-only and must not be changed under any
  circumstances or strange side-effects will occur.

  Note that this function will assert if the element with the given
  stack identity value is not presently on the state stack. To check
  whether or not an element is present in the stack, use
  SoState::isElementEnabled().
*/

/*!
  \fn SbBool SoState::isCacheOpen(void) const

  Returns whether a cache is open.
*/

/*!
  \fn SbBool SoState::isElementEnabled(const int stackindex) const

  This method returns TRUE if the element with the given element stack
  index is enabled, and FALSE otherwise.
*/

/*!
  SoElement * SoState::getElementNoPush(const int stackindex) const

  This method returns a pointer to a writable element without
  checking for state depth.  Use with care.
*/

#include <Inventor/misc/SoState.h>

#include <Inventor/SbName.h>
#include <Inventor/elements/SoElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/lists/SoTypeList.h>
#include <Inventor/lists/SbList.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "rendering/SoGL.h"

// *************************************************************************

// Internal class used to store which elements are pushed for a depth.
// This makes it possible to avoid searching through all elements
// and testing depth in pop().
class sostate_pushstore {
public:
  sostate_pushstore(void) {
    this->next = this->prev = NULL;
  }
  SbList <int> elements;
  sostate_pushstore * next;
  sostate_pushstore * prev;
};

// class to store private data members
class SoStateP {
public:
  SoAction * action;
  SoElement ** initial;
  int depth;
  SbBool ispopping;
  class sostate_pushstore * pushstore;
};

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

/*!
  The constructor.  The \a theAction argument is the action object the state
  is part of, and the \a enabledElements argument is an SoTypeList of the
  elements that are enabled for this action.

  The constructor pushes a default element onto the indexes of all the
  enabled element stacks.  SoElement::push() is not called on the initial
  elements in the SoState stacks, but SoElement::init() is.
*/

SoState::SoState(SoAction * theAction, const SoTypeList & enabledelements)
{
  PRIVATE(this) = new SoStateP;

  PRIVATE(this)->action = theAction;
  PRIVATE(this)->depth = 0;
  PRIVATE(this)->ispopping = FALSE;
  this->cacheopen = FALSE;

  int i;

  this->numstacks = SoElement::getNumStackIndices() ;

  // the stack member can be accessed from inline methods, and is
  // therefore not moved to the private class.
  this->stack = new SoElement * [this->numstacks];
  PRIVATE(this)->initial = new SoElement * [this->numstacks];

  for (i = 0; i < this->numstacks; i++) {
    PRIVATE(this)->initial[i] = NULL;
    this->stack[i] = NULL;
  }

  const int numelements = enabledelements.getLength();
  for (i = 0; i < numelements; i++) {
    SoType type = enabledelements[i];
    assert(type.isBad() || type.canCreateInstance());
    if (!type.isBad()) {
      SoElement * const element = (SoElement *) type.createInstance();
      element->setDepth(PRIVATE(this)->depth);
      const int stackindex = element->getStackIndex();
      this->stack[stackindex] = element;
      PRIVATE(this)->initial[stackindex] = element;
      element->init(this); // called for first element in state stack
    }
  }
  PRIVATE(this)->pushstore = new sostate_pushstore;
}

/*!
  The destructor.

  Note that when destruction happens, lagging events caused by lazy evaluation
  won't be performed.
*/

SoState::~SoState(void)
{
  for (int i = 0; i < this->numstacks; i++) {
    SoElement * elem = PRIVATE(this)->initial[i];
    SoElement * next;
    while (elem) {
      next = elem->nextup;
      delete elem;
      elem = next;
    }
  }

  delete[] PRIVATE(this)->initial;
  delete[] this->stack;

  sostate_pushstore * item = PRIVATE(this)->pushstore;
  while (item->prev) item = item->prev; // go to first item
  while (item) {
    sostate_pushstore * next = item->next;
    delete item;
    item = next;
  }
  delete PRIVATE(this);
}

/*!
  This method returns the pointer to the action instance given to the
  constructor.
*/

SoAction *
SoState::getAction(void) const
{
  return PRIVATE(this)->action;
}

/*!
  This method returns a modifiable instance of the element on the top of
  the stack with the given \a stackindex.  Because of lazy programming,
  this function may need to do some work, so SoState::getConstElement()
  should be used instead whenever possible.
*/

SoElement *
SoState::getElement(const int stackindex)
{
  // catch attempts at setting an element from another element's pop()
  // method (yes, I did this stupid mistake myself and spent a long
  // time debugging it, pederb, 2007-08-01)
  assert(!PRIVATE(this)->ispopping);

  if (!this->isElementEnabled(stackindex)) return NULL;
  SoElement * element = this->stack[stackindex];

#if 0 // debug
  SoDebugError::postInfo("SoState::getElement",
                         "stackindex: %d, element: %p ('%s'), "
                         "stackdepth: %d, pushstack: %s",
                         stackindex, element,
                         element->getTypeId().getName().getString(),
                         element->getDepth(),
                         (element->getDepth() < PRIVATE(this)->depth) ?
                         "yes" : "no");
#endif // debug

  if (element->getDepth() < PRIVATE(this)->depth) { // create elt of correct depth
    SoElement * next = element->nextup;
    if (! next) { // allocate new element
      next = (SoElement *) element->getTypeId().createInstance();
      next->nextdown = element;
      element->nextup = next;
    }
    next->setDepth(PRIVATE(this)->depth);
    next->push(this);
    this->stack[stackindex] = next;
    element = next;
    PRIVATE(this)->pushstore->elements.append(stackindex);
  }
  return element;
}

/*!
  This method pushes the state one level down.  This saves the state so it can
  be changed and later restored to this state by calling SoState::pop().

  The push and pop mechanism is performed lazily for efficiency reasons (avoids
  a lot of memory allocation and copying).  Only when a state element is
  actually going to be changed, that element will be pushed for real.
*/

void
SoState::push(void)
{
  if (PRIVATE(this)->pushstore->next == NULL) {
    sostate_pushstore * store = new sostate_pushstore;
    store->prev = PRIVATE(this)->pushstore;
    PRIVATE(this)->pushstore->next = store;
  }
  PRIVATE(this)->pushstore = PRIVATE(this)->pushstore->next;
  PRIVATE(this)->pushstore->elements.truncate(0);
  PRIVATE(this)->depth++;
}

/*!
  This method pops the state to restore it to a previous state.
  Pops are performed eagerly but the code is very tight so there is
  no reason to worry about efficiency.
*/

void
SoState::pop(void)
{
  PRIVATE(this)->ispopping = TRUE;
  PRIVATE(this)->depth--;
  int n = PRIVATE(this)->pushstore->elements.getLength();
  if (n) {
    const int * array = PRIVATE(this)->pushstore->elements.getArrayPtr();
    for (int i = n-1; i >= 0; i--) {
      int idx = array[i];
      SoElement * elem = this->stack[idx];
      SoElement * prev = elem->nextdown;
      assert(prev);
      prev->pop(this, elem);
      this->stack[idx] = prev;
    }
  }
  PRIVATE(this)->pushstore->elements.truncate(0);
  PRIVATE(this)->pushstore = PRIVATE(this)->pushstore->prev;
  PRIVATE(this)->ispopping = FALSE;
}

/*!
  This method is just for debugging purposes.
*/

void
SoState::print(FILE * const file) const
{
  fprintf(file, "SoState[%p]: depth = %d\n", this, PRIVATE(this)->depth);
  fprintf(file, "  enabled elements {\n");
  for (int i = 0; i < this->numstacks; i++) {
    SoElement * element;
    if ((element = this->stack[i]) != NULL)
      fprintf(file, "    %s\n",
               element->getTypeId().getName().getString());
  }
  fprintf(file, "  }\n");
}

/*!
  This method returns the current depth of the state stack.

  The depth is "virtual", not necessarily physical.
*/

int
SoState::getDepth(void) const
{
  return PRIVATE(this)->depth;
}

/*!
  Controls whether a cache is open.
*/
void
SoState::setCacheOpen(const SbBool open)
{
  this->cacheopen = open;
}

#undef PRIVATE
