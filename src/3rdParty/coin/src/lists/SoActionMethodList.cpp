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
  \class SoActionMethodList SoActionMethodList.h Inventor/lists/SoActionMethodList.h
  \brief The SoActionMethodList class contains function pointers for action methods.

  \ingroup coin_actions

  An SoActionMethodList contains one function pointer per node
  type. Each action contains an SoActioMethodList to know which
  functions to call during scene graph traversal.
*/

#include <Inventor/lists/SoActionMethodList.h>
#include <Inventor/lists/SoTypeList.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/nodes/SoNode.h>
#include <cassert>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef COIN_THREADSAFE
#include <Inventor/threads/SbMutex.h>
#endif // COIN_THREADSAFE

#ifndef DOXYGEN_SKIP_THIS

class SoActionMethodListP {
public:
  SoActionMethodList * parent;
  int setupnumtypes;
  SbList <SoType> addedtypes;
  SbList <SoActionMethod> addedmethods;

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

#endif // DOXYGEN_SKIP_THIS

/*!
  \typedef void (* SoActionMethod)(SoAction *, SoNode *)

  The type definition for all action method functions.
*/

#define PRIVATE(obj) ((obj)->pimpl)

/*!
  The constructor.  The \a parentlist argument is the parent action's
  action method list.  It can be \c NULL for action method lists that
  are not based on inheriting from a parent action.
*/
SoActionMethodList::SoActionMethodList(SoActionMethodList * const parentlist)
{
  PRIVATE(this) = new SoActionMethodListP;
  PRIVATE(this)->parent = parentlist;
  PRIVATE(this)->setupnumtypes = 0;
}

/*!
  Destructor.
*/
SoActionMethodList::~SoActionMethodList()
{
  delete PRIVATE(this);
}

/*!
  \copydetails SbPList::operator[](const int index) const

  Overloaded from parent to return an SoActionMethod.

  \sa SbPList::operator[]()
*/ 
SoActionMethod &
SoActionMethodList::operator[](const int index)
{
  return (SoActionMethod&)SbPList::operator[](index);
}

/*!
  Add a function pointer to a node type's action method.
*/
void
SoActionMethodList::addMethod(const SoType node, const SoActionMethod method)
{
  assert(node != SoType::badType());
  PRIVATE(this)->lock();
  PRIVATE(this)->addedtypes.append(node);
  PRIVATE(this)->addedmethods.append(method);
  PRIVATE(this)->setupnumtypes = 0; // force a new setUp
  PRIVATE(this)->unlock();
}

// dummy method used for detecting unset action methods
static void unsetActionMethod(SoAction *, SoNode *)
{
}

/*!
  This method must be called as the last initialization step before
  using the list. It fills in \c NULL entries with the parent's
  method.
*/
void
SoActionMethodList::setUp(void)
{
  PRIVATE(this)->lock();
  if (PRIVATE(this)->setupnumtypes != SoType::getNumTypes()) {
    int i, n;

    this->truncate(0); // clear action method list

    // first set all methods that have been set directly through SO_ACTION_ADD_METHOD()
    n = PRIVATE(this)->addedtypes.getLength();
    for (i = 0; i < n; i++) {
      (*this)[SoNode::getActionMethodIndex(PRIVATE(this)->addedtypes[i])] = PRIVATE(this)->addedmethods[i];
    }
    
    // make sure SoNode's action method is set to avoid a NULL action method
    i = SoNode::getActionMethodIndex(SoNode::getClassTypeId());
    if ((*this)[i] == NULL) {
      if (PRIVATE(this)->parent == NULL) {
        (*this)[i] = SoAction::nullAction;
      }
      else {
        // set to a dummy method to detect unset methods in the final pass
        (*this)[i] = unsetActionMethod;
      }
    }

    // for node types with no action method, inherit from parent nodetype(s)
    SoTypeList allnodes;
    SoType::getAllDerivedFrom(SoNode::getClassTypeId(), allnodes);
    n = allnodes.getLength();

    for (i = 0; i < n; i++) {
      SoType type = allnodes[i];
      int idx = SoNode::getActionMethodIndex(type);
      SoActionMethod m = (*this)[idx];
      if (m == NULL) {
        do {
          type = type.getParent();
          m = (*this)[SoNode::getActionMethodIndex(type)];
        } while (m == NULL);
        (*this)[idx] = m;
      }
    }

    // inherit unset methods from parent action
    if (PRIVATE(this)->parent != NULL) {
      PRIVATE(this)->parent->setUp();
      n = this->getLength();
      for (i = 0; i < n; i++) {
        if ((*this)[i] == unsetActionMethod) {
          (*this)[i] = (*PRIVATE(this)->parent)[i];
        }
      }
    }
    // used to detect when a new node has been added
    PRIVATE(this)->setupnumtypes = SoType::getNumTypes();
  }
  PRIVATE(this)->unlock();
}

#undef PRIVATE
