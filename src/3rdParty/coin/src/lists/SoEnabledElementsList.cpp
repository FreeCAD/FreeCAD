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
  \class SoEnabledElementsList SoEnabledElementsList.h Inventor/lists/SoEnabledElementsList.h
  \brief The SoEnabledElementsList class is a container for type info for element types that are enabled in actions.

  \ingroup coin_actions

  This class is probably not of interest for the application
  programmer.
*/

// FIXME: doesn't handle post-initialization changes very well (fixing
// this also needs some tempering to be done with SoAction, I think.)
// 20000305 mortene.

#include <Inventor/lists/SoEnabledElementsList.h>
#include <cassert>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbMutex.h>
#endif // COIN_THREADSAFE

#ifndef DOXYGEN_SKIP_THIS

class SoEnabledElementsListP {
public:
  int prevmerge;
  SoTypeList elements;
  SoEnabledElementsList * parent;
#ifdef COIN_THREADSAFE
  SbMutex mutex;
#endif // COIN_THREADSAFE
  void lock(void) {
#ifdef COIN_THREADSAFE
    this->mutex.lock();
#endif
  }
  void unlock(void) {
#ifdef COIN_THREADSAFE
    this->mutex.unlock();
#endif
  }
};

static int enable_counter = 0;

#endif // DOXYGEN_SKIP_THIS

#define PRIVATE(obj) ((obj)->pimpl)

/*!
  Constructor.
*/
SoEnabledElementsList::SoEnabledElementsList(SoEnabledElementsList * const parentlist)
{
  PRIVATE(this) = new SoEnabledElementsListP;

  PRIVATE(this)->prevmerge = 0;
  PRIVATE(this)->parent = parentlist;
}

/*!
  Destructor.
*/
SoEnabledElementsList::~SoEnabledElementsList()
{
  delete PRIVATE(this);
}

/*!
  Return the list of enabled elements.
*/
const SoTypeList &
SoEnabledElementsList::getElements(void) const
{
  PRIVATE(this)->lock();
  // check if we need a new merge
  if (PRIVATE(this)->prevmerge != enable_counter) {
    int storedcounter = enable_counter;
    SoEnabledElementsList * plist = (SoEnabledElementsList*) PRIVATE(this)->parent;
    while (plist) {
      ((SoEnabledElementsList*)this)->merge(*plist);
      plist = plist->pimpl->parent;
    }
    // use and restore old counter since it might change during merge
    ((SoEnabledElementsList*)this)->pimpl->prevmerge =
      enable_counter = storedcounter;
  }
  PRIVATE(this)->unlock();
  return PRIVATE(this)->elements;
}

/*!
  Add an \a elementtype to the list of enabled elements if it is not
  enabled already.
*/
void
SoEnabledElementsList::enable(const SoType elementtype, const int stackindex)
{
  while (stackindex >= PRIVATE(this)->elements.getLength())
    PRIVATE(this)->elements.append(SoType::badType());

  SoType currtype = PRIVATE(this)->elements[stackindex];
  if (currtype.isBad() ||
      (elementtype != currtype && elementtype.isDerivedFrom(currtype))) {
    PRIVATE(this)->elements.set(stackindex, elementtype);
    // increment to detect when a new merge is needed
    enable_counter++;
  }
}

/*!
  Enables all the elements from the \a eel list that is enabled in
  this instance.
*/
void
SoEnabledElementsList::merge(const SoEnabledElementsList & eel)
{
  SoType bad = SoType::badType();
  const int num = eel.pimpl->elements.getLength();
  for (int i = 0; i < num; i++) {
    if (eel.pimpl->elements[i] != bad) this->enable(eel.pimpl->elements[i], i);
  }
}

/*!
  Return the current setting of the global counter used to determine
  when lists are out of date.  It is incremented whenever a new
  element is added to a list.
*/
int
SoEnabledElementsList::getCounter(void)
{
  return enable_counter;
}

#undef PRIVATE
